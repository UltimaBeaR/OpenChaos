---
name: perf-diag
description: "Graphics performance diagnostics — temporary instrumentation of the rendering pipeline with perf counters. Use this skill whenever investigating low FPS, slow rendering, frame drops, or when asked to profile the rendering pipeline. Also use when a macOS build feels slow or when comparing rendering performance across platforms."
---

# Graphics Performance Diagnostics

Temporarily instrument the rendering pipeline with performance counters to find what's eating frame time. **All counters are temporary** — remove them after diagnosis.

## Timer API

```cpp
#include "engine/platform/sdl3_bridge.h"

uint64_t t0 = sdl3_get_performance_counter();
// ... work ...
uint64_t t1 = sdl3_get_performance_counter();
double ms = (double)(t1 - t0) * 1000.0 / (double)sdl3_get_performance_frequency();
```

All output goes to `stderr` → captured in `build/Debug/stderr.log` or `build/Release/stderr.log` by the Makefile. Read the log with the Read tool after the user runs the game.

---

## Instrumentation Points

Work top-down: start with the game loop to find which phase is slow, then drill into the specific subsystem.

### 1. Game Loop — `src/game/game.cpp`

In the main game loop (the `while (SHELL_ACTIVE && ...)` block), wrap key sections:

```cpp
// At top of loop body:
uint64_t frame_start = sdl3_get_performance_counter();

// Around draw_screen():
uint64_t draw_t0 = sdl3_get_performance_counter();
draw_screen();
uint64_t draw_t1 = sdl3_get_performance_counter();

// Around screen_flip() + lock_frame_rate():
uint64_t flip_t0 = sdl3_get_performance_counter();
screen_flip();
uint64_t flip_t1 = sdl3_get_performance_counter();
lock_frame_rate(env_frame_rate);
uint64_t lock_t1 = sdl3_get_performance_counter();

// Print every 60 frames:
uint64_t freq = sdl3_get_performance_frequency();
static uint32_t _fc = 0;
if (++_fc % 60 == 0) {
    fprintf(stderr, "[frame] draw=%.1fms flip=%.1fms lock=%.1fms frame=%.1fms\n",
        (double)(draw_t1 - draw_t0) * 1000.0 / (double)freq,
        (double)(flip_t1 - flip_t0) * 1000.0 / (double)freq,
        (double)(lock_t1 - flip_t1) * 1000.0 / (double)freq,
        (double)(lock_t1 - frame_start) * 1000.0 / (double)freq);
}
```

**Reading results:**
- `draw` high → rendering bottleneck (drill into city breakdown below)
- `flip` high → GPU/vsync bottleneck (check swap interval, driver)
- `lock` high → frame limiter working correctly (this is "spare" time — good)
- `frame` ≈ 33ms at 30fps target; much higher = something is slow

### 2. City Renderer — `src/engine/graphics/pipeline/aeng.cpp`

`AENG_draw_city()` is the main outdoor render function with ~15 distinct stages. Use a PROF_MARK macro at the top and place markers between stages:

```cpp
// At top of AENG_draw_city(), after draw_all_boxes():
uint64_t _prof_freq = sdl3_get_performance_frequency();
uint64_t _prof_prev = sdl3_get_performance_counter();
static uint32_t _prof_cnt = 0;
bool _prof_log = (++_prof_cnt % 60 == 0);

#define PROF_MARK(name) do { \
    if (_prof_log) { \
        uint64_t _now = sdl3_get_performance_counter(); \
        fprintf(stderr, "  [city] %-20s %5.1fms\n", name, \
            (double)(_now - _prof_prev) * 1000.0 / (double)_prof_freq); \
        _prof_prev = _now; \
    } \
} while(0)
```

Place `PROF_MARK("stage_name");` before existing `BreakTime()` calls at these stages:
- `visibility` — after view frustum culling
- `rotate_points` — after point rotation loop
- `shadows` — after shadow rendering
- `reflections` — after puddle reflections
- `poly_flush_1` — after first POLY_frame_draw
- `poly_flush_2` — after second polygon flush
- `floors` — after floor rendering
- `prims` — after primitive rendering
- `facets` — after facet rendering
- `things` — after thing/character rendering
- `effects` — after particles/ribbons/sparks
- `poly_flush_main` — after main POLY_frame_draw
- `dirt+flush` — after dirt + final flush

End with `#undef PROF_MARK` before `Pyros_EndOfFrameMarker()`.

Also in `AENG_draw()`, wrap the city/warehouse draw + cleanup:

```cpp
uint64_t _p0 = sdl3_get_performance_counter();
// ... draw city/warehouse ...
uint64_t _p1 = sdl3_get_performance_counter();
AENG_get_rid_of_deleteme_squares();
AENG_get_rid_of_unused_dfcache_lighting(UC_FALSE);
uint64_t _p2 = sdl3_get_performance_counter();
static uint32_t _pc = 0;
if (++_pc % 60 == 0)
    fprintf(stderr, "[aeng] city=%.1fms cleanup=%.1fms\n",
        (double)(_p1 - _p0) * 1000.0 / (double)sdl3_get_performance_frequency(),
        (double)(_p2 - _p1) * 1000.0 / (double)sdl3_get_performance_frequency());
```

### 3. Polygon Flush — `src/engine/graphics/pipeline/poly.cpp`

`POLY_frame_draw()` batches and submits all accumulated polygons. Track timing and draw counts:

```cpp
// At top of POLY_frame_draw():
uint64_t _pfd_t0 = sdl3_get_performance_counter();
uint32_t _pfd_renders = 0;  // batched page renders
uint32_t _pfd_single = 0;   // per-poly draws (alpha sort path)
```

Increment `_pfd_renders++` after each `pa->Render()` and `_pfd_single++` after each `DrawBatchedPolys` or `DrawSinglePoly` call.

```cpp
// After ge_end_scene():
uint64_t _pfd_t1 = sdl3_get_performance_counter();
double _pfd_ms = (double)(_pfd_t1 - _pfd_t0) * 1000.0
    / (double)sdl3_get_performance_frequency();
static uint32_t _pfd_cnt = 0;
if (_pfd_ms > 5.0 || (++_pfd_cnt % 120 == 0))
    fprintf(stderr, "[poly_flush] %.1fms renders=%u singles=%u alpha=%d\n",
        _pfd_ms, _pfd_renders, _pfd_single, PolyPage::AlphaSortEnabled() ? 1 : 0);
```

**Key indicator:** high `singles` count means per-polygon alpha-sort draws are active — very expensive on macOS (see below).

### 4. GL Draw Call Stats — `src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp`

Track total draw calls and triangles per frame:

```cpp
// Add near top of file:
static uint32_t s_draw_calls = 0;
static uint32_t s_draw_tris  = 0;
```

Add `s_draw_calls++; s_draw_tris += count / 3;` (or `index_count / 3`) at the end of:
- `ge_draw_primitive()`
- `ge_draw_indexed_primitive()`
- `ge_draw_indexed_primitive_vb()`

Print and reset in `ge_flip()`:
```cpp
static uint32_t _gl_fc = 0;
if (++_gl_fc % 60 == 0)
    fprintf(stderr, "[GL stats] draws=%u tris=%u\n", s_draw_calls, s_draw_tris);
s_draw_calls = 0;
s_draw_tris  = 0;
```

---

## Known Issue: macOS GL→Metal Overhead

macOS deprecated OpenGL — all GL calls go through a Metal translation layer with higher per-call overhead than native GL on Windows/Linux.

**Symptom:** game runs at acceptable FPS on Windows but <15fps on macOS.

**Pattern found (2026-04):** Alpha-sorted `DrawSinglePoly` calls in `POLY_frame_draw` were doing ~350 individual GL draw calls per flush, each ~0.2ms on Metal translation = 70ms+ per flush.

**Fixes applied:**
- Alpha sort disabled for OpenGL backend (`PolyPage::s_AlphaSort = false`, `EnableAlphaSort()` no-op for non-D3D)
- GL state caching in core.cpp: program bind cache (`s_cached_program`), uniform snapshot comparison (`UniformSnapshot` + `memcmp`), deferred VAO/program unbind (only at frame boundary in `ge_flip()`)
- Batched alpha-sort path added (`DrawBatchedPolys` — groups consecutive same-page polygons into one draw)

These fixes are permanent and live in the codebase. The profiling counters used to find them are what this skill helps re-add when needed.

---

## Cleanup Reminder

After diagnosing the issue, **remove all profiling code:**
- Delete timer variables, `PROF_MARK` macro, fprintf calls
- Remove added `#include "engine/platform/sdl3_bridge.h"` from files that didn't have it
- Verify with `make build-release` and `make build-debug`
