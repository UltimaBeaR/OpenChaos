---
name: hud-rendering
description: >
  In-game 2D HUD rendering reference — drawing rectangles, filled quads, lines,
  textured sprites, bars, gauges, backdrops, any non-text primitive on screen.
  TRIGGER when: about to draw ANY 2D element on the HUD that isn't text —
  rects, filled quads, outlines, progress bars, health bars, icons, sprites,
  debug overlays, modal backdrops, screen-space polygons, full-screen fades,
  minimap shapes. Trigger even if the primitive is a one-off debug rectangle,
  a placeholder backdrop for a new panel, or a quick visual indicator.
  Also trigger when encountering or about to use ANY of these functions:
  AENG_draw_rect, AENG_draw_rectr, POLY_add_rect, POLY_add_quad, POLY_add_line,
  PANEL_draw_quad, PANEL_draw_health_bar, PANEL_draw_timer, PANEL_draw_face,
  PANEL_draw_local_health, PANEL_start, PANEL_finish, draw_all_boxes.
  Also trigger on keywords (English or Russian): "HUD overlay", "2D primitive",
  "screen-space rect", "backdrop", "bar", "gauge", "health bar", "progress bar",
  "minimap shape", "modal panel", "fullscreen fade", "alpha blend overlay",
  "полоска", "прямоугольник на экран", "нарисовать на hud", "ui рисовать",
  "фон панели", "чёрная подложка", "отладочный прямоугольник".
  DO NOT use for text — use the `text-rendering` skill instead.
  This skill prevents wasted time from shadow-corruption bugs, invisible
  rendering, and wrong alpha/z-order choices.
---

# HUD Rendering — 2D primitives (non-text)

All functions for drawing 2D graphics on screen: filled rectangles, outlined
quads, lines, textured sprites, bars, gauges, backdrops. **For text — use the
`text-rendering` skill instead.**

## Quick decision guide

| Need | Function | Page / notes |
|------|----------|--------------|
| Solid-color screen-space rect | `AENG_draw_rect` | `POLY_PAGE_COLOUR` |
| Semi-transparent rect | `AENG_draw_rect` + alpha in color | `POLY_PAGE_COLOUR_ALPHA`, alpha in high byte |
| Deferred 2D rect (queued, batch) | `AENG_draw_rectr` + `draw_all_boxes()` | Internal queue |
| Textured quad (sprite) | `PANEL_draw_quad` / `POLY_add_quad` | Specific page for the texture |
| 2D line / bar outline | `POLY_add_line` | `POLY_PAGE_COLOUR` or `POLY_PAGE_COLOUR_ALPHA` |
| Ready-made health bar | `PANEL_draw_health_bar(x, y, percentage)` | Uses AENG_draw_rect internally |
| Ready-made countdown timer | `PANEL_draw_timer(time, x, y)` | Queued, drawn by `PANEL_draw_buffered` |
| 3D-world-space debug geometry | `AENG_world_line`, `AENG_world_quad` | Render pass only; see shadow-corruption notes |

## The #1 rule: wrap in a batch pair

**Every 2D primitive call queued into POLY pages needs a matching batch wrapper.** This is the single most important thing in this skill.

```cpp
PANEL_start();              // POLY_frame_init — starts the 2D batch
AENG_draw_rect(...);        // queue geometry
POLY_add_line(...);         // more geometry
// ... more draws ...
PANEL_finish();             // POLY_frame_draw — flushes the batch to screen
```

Without `PANEL_finish`:
1. Geometry is **not drawn this frame** — it sits in the page buffer unflushed.
2. **Next frame's POLY_frame_init clears the page**, but the VB slots are reused by the shadow pass. Your rect ends up rendered as a ghostly white artifact on the ground — the classic "shadow corruption" bug.

Root cause and full forensic write-up: `new_game_devlog/shadow_corruption_investigation.md`. The devlog describes this for `AENG_world_line`, but the same mechanism affects `AENG_draw_rect`, `POLY_add_rect`, `POLY_add_quad`, `FONT2D_DrawString` — anything that feeds POLY pages.

**You don't need a wrapper** if you're calling from inside `OVERLAY_handle`, `PANEL_draw_health_bar`, `PANEL_draw_buffered`, or similar — those already live inside a start/finish pair opened by their caller. But if you're writing a new module that draws on its own, open your own batch.

## Where in the frame to insert the call

The game loop renders HUD between `draw_screen()` and `screen_flip()`. Safe
insertion points for your own overlay:

| Location | Safe? | Why |
|----------|-------|-----|
| Inside `OVERLAY_handle` | ✅ yes | Already wrapped by `PANEL_start/PANEL_finish` |
| Immediately after `OVERLAY_handle`, with own `PANEL_start/PANEL_finish` | ✅ yes | Inside render pass, own batch |
| Before `draw_screen()` | ❌ no | `draw_screen` calls `POLY_frame_init` internally and clears your batch |
| After `GAMEMENU_draw()` | ❌ no | `GAMEMENU_draw` calls `POLY_frame_init` internally; anything queued after is orphaned → next-frame shadow reuse |
| From game tick (process_controls, AI, etc.) | ❌ no | Pre-render; same shadow corruption as world_line |

**Reference implementations:**
- Simple HUD primitive called from inside an overlay:
  `new_game/src/ui/hud/panel.cpp:172` — `PANEL_draw_health_bar`
- Full-screen modal overlay with own start/finish pair:
  `new_game/src/engine/debug/input_debug/input_debug.cpp` — `input_debug_render`

## Alpha blending

Two different poly pages handle color vs color+alpha:

| Page | Blending | Color format |
|------|----------|--------------|
| `POLY_PAGE_COLOUR` | Solid (no alpha) | `0xRRGGBB` |
| `POLY_PAGE_COLOUR_ALPHA` | `SrcAlpha/InvSrcAlpha` | `0xAARRGGBB` — **alpha in HIGH byte** |

So for a semi-transparent backdrop:
```cpp
// 80% opaque black full-screen backdrop
AENG_draw_rect(0, 0, POLY_screen_width, POLY_screen_height,
               0xCC000000,              // 0xCC = 204/255 ≈ 80% alpha
               10,                      // layer
               POLY_PAGE_COLOUR_ALPHA); // alpha-blend page
```

The alpha bits in `0x??000000` are ignored on `POLY_PAGE_COLOUR` — the page
doesn't enable blending. Using the wrong page is the most common reason a
"semi-transparent" overlay comes out fully opaque or invisible.

Blending state is set up in `new_game/src/engine/graphics/pipeline/poly_render.cpp`
under the `POLY_PAGE_COLOUR_ALPHA` case.

## Layer parameter (z-order)

`AENG_draw_rect` and `AENG_draw_rectr` take a `layer` int. **Lower layer =
closer to camera (in front).** This is counter-intuitive — the name
suggests "higher layer is on top" but the pipeline does the opposite.

Why: `AENG_draw_rect` stores `POLY_Point.Z = 1.0 - layer*0.0001`, then
`PolyPoint2D::SetSC` flips it back to `sz = 1.0 - Z = layer*0.0001`,
which goes into the shader as depth. Under standard GL_LESS depth test,
smaller `sz` wins — so `layer=1` sits in front of `layer=2`.

Canonical reference: `PANEL_draw_health_bar` in `ui/hud/panel.cpp:172`
draws the black background with `layer=2` and the red fill with
`layer=1`. The fill renders on top.

| Use case | Typical layer | Position |
|----------|---------------|----------|
| Foreground accents (dots, fills over backing rects) | 1 | closest |
| Regular HUD element (health bar fill, icon) | 1–3 | close |
| Widget backgrounds (bar backing, box outlines) | 2–4 | behind accents |
| Full-screen modal backdrop | 10+ | farthest, behind HUD |

**Common trap:** drawing a backing rect at layer N and an accent dot at
layer N+1. The dot's depth value is `larger` than the background's, so
the depth test rejects it and nothing appears. You need layer N for
the background and layer N-1 for the accent.

Within the same layer value, draw order determines z — later = on top.
Depth write is on for `POLY_PAGE_COLOUR`, so once a pixel is written
with a given depth, later pixels at the same screen position with a
larger depth (higher layer) are rejected.

For `POLY_add_quad` / `POLY_add_line`, z comes from the `POLY_Point.Z` field
you set on the vertices (typical HUD quad: `Z = 0.99999f` to push it near the
far plane and let the page's render state handle layering).

## Screen dimensions and coordinate space

Rects here use **logical 640×480** coordinates which the pipeline scales
to the actual window. The handy globals are from `engine/graphics/pipeline/poly.h`:

```cpp
extern float POLY_screen_width;   // logical width  (== DisplayWidth  == 640)
extern float POLY_screen_height;  // logical height (== DisplayHeight == 480)
```

They are aliases for the `DisplayWidth` / `DisplayHeight` constants (640/480)
from `engine/platform/uc_common.h`. Cast to `SLONG` when passing to
`AENG_draw_rect`.

⚠️ **If you add text labels to your widgets**, the text path (`FONT_buffer_add`,
`FONT_draw`) uses **literal window pixels**, not logical coords. On a window
larger than 640×480 your labels will not line up with the rects. Either:

- Use `FONT2D_DrawString` / `CONSOLE_text_at` — same logical space as rects.
- Or scale manually: `x_px = x_logical * RealDisplayWidth / 640`.

Full details and a ready-made helper pattern in the `text-rendering` skill
under "Mixing text and HUD rects".

## Function reference

### AENG_draw_rect(x, y, w, h, col, layer, page)
- **Header:** `engine/graphics/pipeline/aeng.h`
- **Immediate** — builds a quad and calls `POLY_add_rect` inline. Your call
  site must be inside an active POLY batch (between `PANEL_start` and
  `PANEL_finish`, or inside code that already wraps you).
- **`col`:** `0xRRGGBB` for `POLY_PAGE_COLOUR`, `0xAARRGGBB` for
  `POLY_PAGE_COLOUR_ALPHA`.
- **`layer`:** int, **lower = in front** (see "Layer parameter" section above).
- **`page`:** usually `POLY_PAGE_COLOUR` or `POLY_PAGE_COLOUR_ALPHA`.

### AENG_draw_rectr(x, y, w, h, col, layer, page)
- **Header:** `engine/graphics/pipeline/aeng_globals.h`
- **Deferred** — appends to an internal `rrect[]` array; flushed later when
  `draw_all_boxes()` is called, which itself calls `AENG_draw_rect`.
- Used when you want to build a list of rects outside an active batch and
  flush them together later. Most callers don't need this — `AENG_draw_rect`
  inside your own `PANEL_start/PANEL_finish` is simpler.

### POLY_add_rect(p1, width, height, page, sort_to_front)
- **Header:** `engine/graphics/pipeline/poly.h`
- Lower-level: takes a `POLY_Point*` as the top-left vertex + explicit
  width/height. Uses the vertex color and Z. Called by `AENG_draw_rect`.
- Reach for this when you need per-vertex color or custom z.

### POLY_add_quad(quad, page, no_texture, sort_to_front)
- **Header:** `engine/graphics/pipeline/poly.h`
- Fully general 4-vertex quad. `quad` is `POLY_Point* [4]`. Each vertex
  carries its own position, color, UV. Used for textured sprites and
  gradient-filled shapes.
- Reference usage: screensaver bars in `ui/hud/panel.cpp:2625–2648`.

### POLY_add_line(p1, p2, width1, width2, page, sort_to_front)
- **Header:** `engine/graphics/pipeline/poly.h`
- 2D billboard line as a thin quad. `width1`/`width2` in pixels at endpoints
  (taper supported). Uses vertex color. Use `POLY_PAGE_COLOUR_ALPHA` for
  soft outlines, `POLY_PAGE_COLOUR` for solid.
- Reference usage: `PANEL_draw_gun_sight` crosshair in `ui/hud/panel.cpp:338`.

### PANEL_draw_quad(left, top, right, bottom, page, colour, ...)
- **Header:** `ui/hud/panel.h`
- Convenience wrapper for screen-axis-aligned textured quad with UVs from
  `(0,0)` to `(1,1)`. Reach for this when drawing HUD sprites from a texture
  page.

### PANEL_draw_health_bar(x, y, percentage)
- **Header:** `ui/hud/panel.h`
- Ready-made two-segment bar (black background + red fill). Clamps
  `percentage` to [0,100]. Uses `AENG_draw_rect` internally with fixed
  `HEALTH_BAR_WIDTH`/`HEIGHT` constants. Intended to be called from inside
  an overlay.

### PANEL_draw_timer(time, x, y)
- **Header:** `ui/hud/panel.h`
- **Deferred** — stores the request in `PANEL_store[]`. Flushed by
  `PANEL_draw_buffered()` which is called from `OVERLAY_handle`. `time` is
  in hundredths of a second.

### PANEL_start() / PANEL_finish()
- **Header:** `ui/hud/panel.h`
- The HUD-scoped wrappers around `POLY_frame_init` and `POLY_frame_draw`.
  Use these when writing new HUD code — they're the idiomatic batch pair for
  2D primitives in this codebase.

## Rules

- **Never add a 2D primitive without a batch wrapper.** If your call is not
  inside `OVERLAY_handle` or a similar pre-wrapped context, open your own
  `PANEL_start`/`PANEL_finish` pair around it. The shadow-corruption bug is
  **silent** at compile time and **intermittent** at runtime (only shows on
  specific camera angles or during shadow passes) — easy to ship by accident.

- **Check the insertion point in the frame.** If the call site is in
  `game_tick`, AI, physics, or before `draw_screen()`, it's wrong regardless
  of wrapper. Move it to `OVERLAY_handle` or the post-`OVERLAY_handle` slot.

- **Pick the right page for alpha.** If the rect should be transparent,
  use `POLY_PAGE_COLOUR_ALPHA` AND put alpha in the high byte of the color.
  Using `POLY_PAGE_COLOUR` with an ARGB color gives you fully opaque (alpha
  bits are ignored).

- **Layer matters when things overlap — lower = in front.** Full-screen
  backdrops use a high layer (e.g. 10+) so they sit *behind* regular HUD.
  Regular HUD uses 1–3. Accents that must sit on top of a backing rect
  need a *lower* layer than the backing rect, not higher.

- **Use game-shared constants for screen dims.** `POLY_screen_width/height`
  are floats and update with the window. Don't hardcode 640×480 unless
  you're writing legacy-compat code.

## Shadow corruption — the #1 failure mode

If your rect shows up as a grid of white squares on the ground, or as
ghostly replicas of characters' shadows, you've hit the VB-slot-reuse bug:

1. Your primitive queued into a POLY page.
2. No `PANEL_finish` / `POLY_frame_draw` closed the batch this frame.
3. Next frame's `POLY_frame_init` cleared the pages; the VBs went back to
   the pool.
4. Shadow pass grabbed the same VB slots, inherited your (stale) geometry
   as its vertex buffer → shadows render as copies of your rect's shape,
   smeared across the scene.

**Fix:** wrap the call in `PANEL_start` / `PANEL_finish` (or put it inside
an already-wrapped context like `OVERLAY_handle`). If you're still seeing
artifacts after wrapping, recheck the insertion point — it might be after
`GAMEMENU_draw` or another place where a later `POLY_frame_init` clears
your work.

Full investigation: `new_game_devlog/shadow_corruption_investigation.md`.

## Details

- Primitives and pages: `new_game/src/engine/graphics/pipeline/aeng.h`,
  `aeng.cpp`, `poly.h`, `poly.cpp`.
- Blending setup per page: `new_game/src/engine/graphics/pipeline/poly_render.cpp`.
- HUD wrappers: `new_game/src/ui/hud/panel.{h,cpp}`.
- Working overlay example: `new_game/src/engine/debug/input_debug/input_debug.cpp`.
