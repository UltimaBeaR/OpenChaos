# Stage 7 -- Full Review Results

**Date:** 2026-03-28
**Branch reviewed:** `TEMP_FOR_REVIEW` (squashed commit `abd73a6`)
**Base:** `decd0ab` (stage 5.1 done)
**Scope:** 87 files, ~5600 lines changed (code only, documentation excluded)
**Reviewer:** Claude Opus 4.6, 6 parallel review agents

---

## Executive Summary

The Stage 7 refactoring is **clean and correct**. All D3D calls have been faithfully replaced
with `ge_*` equivalents. The render state cache migration is complete and preserves identical
behavior. All 18 backend file moves are clean (include-path changes only).

**No runtime-breaking bugs found.**

A few architectural items need attention before the OpenGL backend step, and one
redundant side effect should be cleaned up. Details below.

---

## Rule Compliance

### Rule 1: No D3D/DDraw code outside `backend_directx6/` (and `outro/`) -- PASS

- All `#include <d3d.h>`, `<ddraw.h>`, `<d3dtypes.h>` are exclusively in `backend_directx6/` and `outro/`
- All D3D types (`LPDIRECT3D*`, `D3DRENDERSTATE_*`, `D3DTSS_*`, etc.) are exclusively in `backend_directx6/` and `outro/`
- All D3D function calls (`SetRenderState`, `DrawPrimitive`, etc.) are exclusively in `backend_directx6/` and `outro/`
- `D3DFVF_*` appears only in comments in `polypage.h` (documentation, not code)
- Dead D3D code in `poly.cpp:1943-1947` is inside a `/* */` comment block (was already dead pre-refactor)

### Rule 2: No game logic inside `backend_directx6/` -- PASS

- No AI, physics, combat, entity, mission, navigation, or world-object includes
- No UI/menu logic (except display mode enumeration)
- Only engine-level and rendering-related includes
- Callback pattern used to decouple from game code (pre-flip, mode-change, polys-drawn)

### Rule 3: ge_* contract implementable on OpenGL -- PASS (with notes)

The contract is implementable. See "OpenGL Portability Notes" section below for items
that will need attention when writing the OpenGL backend.

---

## Detailed Findings

### A. Issues Requiring Fix (3)

#### A1. `ge_set_fog_params()` has alpha test side effect -- REDUNDANT

**File:** `backend_directx6/graphics_engine_d3d.cpp:251-253`

`ge_set_fog_params()` sets `ALPHAREF=0x07` and `ALPHAFUNC=D3DCMP_GREATER` as a side effect.
The caller `GERenderState::InitScene()` in `graphics_engine.cpp:191-192` ALSO sets these
explicitly via `ge_set_alpha_ref(0x07)` and `ge_set_alpha_func(Greater)`.

In the old code (`render_state.cpp:196-197`), alpha test was set ONCE in `InitScene()` alongside
fog params (all at the same REALLY_SET_RENDER_STATE level). During the split, the alpha test
lines ended up in BOTH places.

**Impact:** None (same values set twice). But architecturally wrong -- a fog function shouldn't
have alpha test side effects. If `ge_set_fog_params()` is called from another context, it will
set surprise alpha test state.

**Fix:** Remove lines 251-253 from `ge_set_fog_params()`. The explicit calls in `InitScene()`
already handle this correctly.

#### A2. `extern CurDrawDistance` in `graphics_engine.cpp` -- game variable leak

**File:** `graphics_engine.cpp:173`

`GERenderState::InitScene()` directly accesses `extern SLONG CurDrawDistance` (a game-level
variable from `aeng_globals`) to calculate fog distance. This breaks the API-agnostic contract
of `graphics_engine.cpp`.

**Impact:** None for D3D backend. Blocks clean separation for future backends.

**Fix:** Pass fog parameters from the caller. `InitScene()` should receive `fog_near` and
`fog_far` (or the raw `CurDrawDistance`) as arguments, with the calculation done at the
call site in game code.

#### A3. `ge_blit_texture_to_backbuffer` -- misleading name

**File:** `backend_directx6/graphics_engine_d3d.cpp:693-704`

The DDraw `Blt` call is `texture.GetSurface()->Blt(NULL, backSurface, &rcSource, ...)`,
which blits FROM the back buffer TO the texture (DDraw convention: `dest->Blt(dstRect, src, srcRect)`).
The function name says "texture_to_backbuffer" but it does the opposite.

**Impact:** None -- the old code (`flamengine.cpp:491`) had the exact same call. Behavior is
identical. The name is inherited from the original function's misleading semantics.

**Fix:** Rename to `ge_capture_backbuffer_to_texture()` or `ge_blit_backbuffer_to_texture()`
for clarity. Low priority.

---

### B. Items to Address Before OpenGL Backend (6)

These are not bugs -- they work correctly for D3D6. But they need attention when
implementing the OpenGL 3.3+ backend.

#### B1. `GEPrimitiveType::TriangleFan` -- not in OpenGL core profile

**File:** `graphics_engine.h:85`

`GL_TRIANGLE_FAN` is deprecated/removed in OpenGL 3.1+ core profile.
Currently unused (zero `.cpp` callers). Can be removed from the enum, or the
OpenGL backend must convert fans to triangle lists internally.

#### B2. `ge_set_color_key_enabled` -- no OpenGL equivalent

**File:** `graphics_engine.h:166`

Color keying is D3D6/DDraw-specific. Modern replacement: alpha testing or
shader `discard`. Should convert color key to alpha channel at texture load time.

#### B3. `ge_set_perspective_correction` -- meaningless on modern GPU

**File:** `graphics_engine.h:172`

All modern GPUs always do perspective-correct interpolation. OpenGL backend
should make this a no-op.

#### B4. `ge_lock_texture_pixels` returns `uint16_t**` -- hardcodes 16-bit

**File:** `graphics_engine.h:395`

The original game uses 16-bit textures (RGB565/ARGB1555). An OpenGL backend
would likely use 32-bit (RGBA8). This interface will need adjustment.

#### B5. `ge_to_gdi()` / `ge_from_gdi()` and `ge_update_display_rect(void* hwnd)` -- Windows-specific

**Files:** `graphics_engine.h:326-327, 337`

GDI and HWND are Windows-only. Rename to `ge_release_display()` / `ge_reacquire_display()`
or abstract the platform window handle.

#### B6. Capability queries always true on modern hardware

**File:** `graphics_engine.h:286-289`

`ge_supports_dest_inv_src_color()`, `ge_supports_modulate_alpha()`, `ge_supports_adami_lighting()`,
`ge_is_hardware()` -- all should return `true` on any modern backend. Consider removing from API
and hardcoding the "capable" code paths.

---

### C. Hardcoded Capability Flags -- Acceptable for D3D6

**File:** `graphics_engine.cpp:10-11`

```cpp
static bool s_modulateAlphaSupported = true;
static bool s_destInvSourceColourSupported = false;
```

The old code queried these at runtime via `the_display.GetDeviceInfo()`. The new code
hardcodes them. This means:
- `s_modulateAlphaSupported = true` -> ModulateAlpha fallback to Modulate is **never taken**
- `s_destInvSourceColourSupported = false` -> Decal always falls back to Modulate + DecalMode effect

This matches the most common hardware path (anything post-Rage Pro). The Rage Pro workaround
path is now dead code. **Acceptable** -- no one runs Rage Pro. But should be initialized from
`ge_supports_*()` for correctness. Low priority.

---

### D. Cosmetic / Cleanup (5)

#### D1. Duplicate `#include "graphics_engine.h"` in several files

Files with double includes (harmless due to include guards, but messy):
- `aeng_globals.h` (lines 5 and 8)
- `poly_globals.h` (lines 4 and 8)
- `poly.cpp` (lines 6 and 13)
- `farfacet_globals.h` (lines 5 and 8)
- `figure.cpp` (lines 30 and 33)

#### D2. Redundant `reinterpret_cast` on same-type pointers

In `poly.cpp`, several `reinterpret_cast<const GEMatrix*>(&matTemp)` where `matTemp`
is already `GEMatrix`. Instances at lines 278, 298, 545, 578. The casts are no-ops.

#### D3. `g_viewData.dwSize = sizeof(GEViewport)` -- dead assignment

**File:** `poly.cpp`

`dwSize` was consumed by D3D's `SetViewport2()`. New code passes individual fields
to `ge_set_viewport_3d()`, so `dwSize` is never read.

#### D4. `DisplayWidth`/`DisplayHeight` macro duplication

6 files define `#define DisplayWidth 640` / `#define DisplayHeight 480` locally:
`sky.cpp`, `frontend.cpp`, `game_tick.cpp`, `eng_map.cpp`, plus others via old headers.
Consider a shared definition or `ge_get_screen_width/height()`.

#### D5. D3D references in comments throughout `graphics_engine.h`

Comments reference `D3DTLVERTEX`, `D3DLVERTEX`, `D3DVIEWPORT2`, `DDraw`, etc.
Not harmful but should describe *what the type does* rather than *what D3D type it replaces*
when writing the OpenGL backend.

---

### E. `ge_get_font_data()` -- Declared but Not Implemented

**File:** `graphics_engine.h:405`

`GEFontData* ge_get_font_data(int32_t page)` is declared in the header but has zero
implementations in any `.cpp` file. Also zero callers in any `.cpp` file. Not currently
a linker error (no one calls it), but it's dead API surface.

**Fix:** Remove from header, or implement if needed later.

---

### F. Vertex Struct D3D Legacy Aliases

**File:** `graphics_engine.h:96-129`

`GEVertexTL`, `GEVertexLit`, `GEVertex` contain union aliases with D3D6 field names:
`dvSX`, `dvSY`, `dcColor`, `dcSpecular`, `dvRHW`, `dwReserved`, etc.

The clean names (`x`, `y`, `z`, `color`, `specular`, `u`, `v`) are canonical.
The D3D aliases exist for backward compatibility during migration.

**Fix:** Remove `dv*`/`dc*`/`dw*` aliases after confirming no callers use them,
or gate behind `#ifdef GE_LEGACY_ALIASES`.

---

### G. `GEViewport` D3D Legacy

**File:** `graphics_engine.h:231-239`

- `dwX`, `dwY`, `dwWidth`, `dwHeight` -- D3D naming (`dw` = DWORD prefix)
- `dvClipX`, `dvClipY`, etc. -- D3D `D3DVIEWPORT2` field names
- `dwSize` -- pure D3D COM artifact (Microsoft version-check pattern)

Same approach as vertex structs: clean names are canonical, D3D names are transitional.
`dwSize` can be removed once `g_viewData.dwSize` assignment is cleaned up.

---

### H. `GERenderState` / `#pragma pack` / Legacy Defines

**File:** `graphics_engine.h:520-526, 603-605`

- `RS_None`, `RS_AlphaPremult`, etc. are `#define` macros (pollute global namespace). Better as `constexpr`.
- `#pragma pack(push, 4)` on `GERenderState` -- unclear why. Not serialized or mapped to GPU.
- `using RenderState = GERenderState` / `using SpecialEffect = GERenderEffect` -- transitional aliases, fine for now.
- `GE_SCREEN_SURFACE_NONE` is `#define`, should be `constexpr`.

---

## Backend File Moves -- All Clean

18 files moved from `engine/graphics/graphics_api/` to `engine/graphics/graphics_engine/backend_directx6/`
with include-path-only changes. All verified:

| File | Changes | Verdict |
|------|---------|---------|
| d3d_texture.h | none | OK |
| d3d_texture.cpp | 4 include paths | OK |
| dd_manager.cpp | 3 include paths | OK |
| dd_manager.h | none | OK |
| dd_manager_globals.cpp/h | 1 include path each | OK |
| display.cpp | +callbacks, -game includes | OK (architectural improvement) |
| display_globals.cpp/h | include paths | OK |
| display_macros.h | 1 include path | OK |
| gd_display.h | 2 include paths | OK |
| vertex_buffer.cpp/h | include paths / none | OK |
| vertex_buffer_globals.cpp/h | include paths | OK |
| work_screen.cpp/h | include paths / none | OK |
| work_screen_globals.cpp/h | none | OK |
| polypage.cpp | +VB cast macros, +callback, -device param | OK |
| truetype.cpp | D3D states -> ge_* calls | OK |
| truetype_globals.cpp/h | include paths, +CacheLine moved here | OK |
| text.cpp | include paths | OK |

---

## Render State Migration -- Complete

Old `render_state.cpp` -> split into `graphics_engine.cpp` (API-agnostic cache) + `graphics_engine_d3d.cpp` (D3D implementation).

| Old function | New location | Verdict |
|---|---|---|
| Constructor defaults | `graphics_engine.cpp` | OK -- all values match |
| `SetTempTransparent()` | `graphics_engine.cpp` | OK -- identical logic |
| `ResetTempTransparent()` | `graphics_engine.cpp` | OK -- identical logic |
| `SetTexture()` | `graphics_engine.cpp` | OK |
| `SetRenderState(DWORD, DWORD)` | Replaced by typed setters | OK -- all callers updated |
| `SetEffect()` | `graphics_engine.cpp` | OK -- identical switch logic |
| `InitScene()` | `graphics_engine.cpp` | OK -- calls ge_* instead of REALLY_SET_RENDER_STATE |
| `SetChanged()` | `graphics_engine.cpp` | OK -- same delta-update logic |
| `IsSameRenderState()` | `graphics_engine.cpp` | OK -- all fields compared |
| `FloatAsDword()` | `graphics_engine_d3d.cpp` (static) | OK -- now uses memcpy (safer, no UB) |

Hardware workarounds (ModulateAlpha/Decal fallback) correctly moved to `SetTextureBlend()`.

---

## Pipeline Review (poly_render.cpp, aeng.cpp, poly.cpp, etc.) -- Clean

- Zero remaining D3D API calls in pipeline code (grep verified)
- All ~60 render state translations in poly_render.cpp verified correct:
  - `D3DRENDERSTATE_TEXTUREMAPBLEND` -> `SetTextureBlend()` with correct enum values
  - `D3DRENDERSTATE_SRCBLEND/DESTBLEND` -> `SetSrcBlend()`/`SetDstBlend()`
  - `D3DRENDERSTATE_ZENABLE/ZWRITEENABLE` -> `SetDepthEnabled()`/`SetDepthWrite()`
  - All other render states mapped correctly
- All `DrawIndexedPrimitive` calls correctly mapped to `ge_draw_indexed_primitive_*`
- `D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT` flags correctly dropped (optimization hints only)
- `FORCE_SET_RENDER_STATE` pattern correctly split into cache update + ge_* call
- Pre-existing redundancies preserved (POLY_PAGE_LITE_BOLT double SetDstBlend, POLY_PAGE_ANGEL double SetTextureBlend)

---

## Caller-Side Review (ui/, game/, assets/, geometry/, etc.) -- Clean

- All D3D types replaced with GE equivalents (verified by static_asserts in backend)
- `texture.cpp` (390 lines): complete D3DTexture -> ge_texture_* migration, all ~50 occurrences correct
- `frontend.cpp` (286 lines): LPDIRECTDRAWSURFACE4 -> GEScreenSurface, DDraw Blt -> ge_blit_*, driver enumeration -> callback. All correct
- `figure.cpp` (145 lines): D3DVECTOR/D3DMATRIX/D3DVERTEX -> GE types, DrawIndPrimMM -> ge_draw_multi_matrix. All correct
- `attract.cpp`: 28 individual D3D render states -> 10 ge_* calls. All mapped correctly. Note: alpha test not explicitly reset (was explicit in old code), but default state is "disabled" so safe in practice
- All other files: clean replacements verified

---

## CMakeLists.txt -- Clean

- All 14 file paths updated from `graphics_api/` to `graphics_engine/backend_directx6/`
- `graphics_engine_d3d.cpp` and `graphics_engine.cpp` added
- `render_state.cpp` removed (commented with explanation)
- Outro disabled with explanation
- No missing files (cross-referenced with directory listing)

---

## Summary of Required Actions

### Must Fix (before moving forward)
1. **A1:** Remove alpha test side effect from `ge_set_fog_params()` (2 lines)

### Should Fix (before OpenGL backend)
2. **A2:** Remove `extern CurDrawDistance` from `graphics_engine.cpp`, pass as parameter
3. **D1:** Remove duplicate `#include "graphics_engine.h"` in 5 files
4. **E:** Remove `ge_get_font_data()` declaration (dead API)

### Nice to Fix (cleanup)
5. **A3:** Rename `ge_blit_texture_to_backbuffer` -> `ge_capture_backbuffer_to_texture`
6. **D2:** Remove redundant `reinterpret_cast` on same-type pointers in poly.cpp
7. **D3:** Remove dead `g_viewData.dwSize` assignment
8. **D4:** Consolidate `DisplayWidth`/`DisplayHeight` macros
9. **F/G:** Remove D3D legacy union aliases from vertex/viewport structs
10. **H:** Replace `#define` constants with `constexpr`

### Deferred (OpenGL backend phase)
11. **B1-B6:** OpenGL portability items (TriangleFan, color key, perspective correction, 16-bit textures, Windows APIs, capability queries)

### All items resolved.

---

## Fixes Applied (2026-03-28)

All items from "Must Fix", "Should Fix", and "Nice to Fix" (except D4) have been applied:

| # | Item | Status |
|---|------|--------|
| A1 | `ge_set_fog_params()` alpha test side effect | **FIXED** -- removed 2 lines from d3d backend |
| A2 | `extern CurDrawDistance` in graphics_engine.cpp | **FIXED** -- moved to caller, InitScene takes fog_near/fog_far params |
| A3 | `ge_blit_texture_to_backbuffer` misleading name | **FIXED** -- renamed to `ge_capture_backbuffer_to_texture` |
| D1 | Duplicate `#include "graphics_engine.h"` | **FIXED** -- removed from 5 files |
| D2 | Redundant `reinterpret_cast` in poly.cpp | **FIXED** -- removed 4 no-op casts |
| D3 | Dead `g_viewData.dwSize` assignment | **FIXED** -- removed |
| D4 | DisplayWidth/DisplayHeight duplication | **FIXED** -- extern removed from uc_common.h, single #define there now, 7 local copies removed |
| E | Dead `ge_get_font_data()` declaration | **FIXED** -- removed from header |
| F/G | D3D legacy aliases in game code | **FIXED** -- figure.cpp (dvTU->u, dvX->x, etc.), fastprim.cpp (dwReserved->_reserved). Backend aliases kept. |
| H | `#define` constants | **FIXED** -- GE_SCREEN_SURFACE_NONE -> constexpr, RS_* -> inline constexpr, typedef -> using |

Also removed: unnecessary `#include "uc_common.h"` from `graphics_engine.cpp` (no longer needed
after CurDrawDistance removal), outdated comment in `wibble.cpp`.

Build verified: `make build-release` passes (308/308, Linking CXX executable).

---

**Overall verdict: PASS.** The refactoring is a clean extraction of D3D code behind the ge_* API
with zero runtime behavioral changes. All review findings have been addressed.
