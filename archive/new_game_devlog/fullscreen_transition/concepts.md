# Concepts & pipeline flow

Terminology + pipeline flow for the 3D/HUD rendering path after the
FBO-as-virtual-screen refactor. Read this before touching
`POLY_camera_set`, `AENG_clear_viewport`, `PolyPage::AddFan`, the
composition layer, or anything that sets camera lens / aspect.

## Glossary

### Virtual / real dimensions

**`DisplayWidth` / `DisplayHeight`** — preprocessor macros in
[`uc_common.h`](../../new_game/src/engine/platform/uc_common.h). **Always 640
and 480.** They define the **virtual game coordinate space** — the reference
frame the original 1999 code drew into. Every hardcoded layout (HUD panel
positions, health bar width, button coords, etc.) assumes this space.

**`ScreenWidth` / `ScreenHeight`** — runtime `SLONG` globals, defined in
[`core.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp).
Hold the **scene FBO size** — the game's "screen". The FBO is sized as
`physical_window × OC_RENDER_SCALE`, then aspect-clamped to
`[OC_FOV_MIN_ASPECT, OC_FOV_CAP_ASPECT]` in `OpenDisplay`. Game code
reads these as the one source of truth for "how big is the screen I'm
drawing to"; the composition layer at frame end handles the gap between
FBO size and real backbuffer size. `gl_context_get_width/height` is
hooked to the same FBO dimensions.

**Real backbuffer size** — queried directly from SDL3 via
`sdl3_window_get_drawable_size`, visible **only** to the composition
layer and the mouse-event mapping code. Everywhere else, code reads
`ScreenWidth/Height` instead.

### Virtual 3D render space

**`POLY_screen_width` / `POLY_screen_height`** — globals set every frame
by `POLY_camera_set()` in
[`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp). The
**virtual 3D render space** that `POLY_perspective` projects into.

```
POLY_screen_width  = DisplayHeight × real_aspect
POLY_screen_height = DisplayHeight (full-screen) or DisplayHeight/2 (splitscreen half)

where real_aspect = ScreenWidth / ScreenHeight
```

No per-frame aspect clamping — the scene FBO is already pre-clamped at
creation time (`OpenDisplay`), so `real_aspect` lives inside
`[OC_FOV_MIN_ASPECT, OC_FOV_CAP_ASPECT]` unconditionally.

Every 3D vertex, after the software perspective pass
(`POLY_perspective`), comes out in `[0..POLY_screen_width] × [0..POLY_screen_height]`
virtual coords. A `POLY_sprite_scale` file-scope float is also
recomputed here — it's the "world length → virtual pixels" multiplier
used by billboards / lines / sphere radii (see `POLY_sprite_scale`
below).

### Scale factors (virtual → FBO pixel conversion)

**`PolyPage::s_XScale` / `s_YScale`** — static floats in
[`polypage.cpp`](../../new_game/src/engine/graphics/pipeline/polypage.cpp).
The **final multiplier** applied to every vertex X / Y before GL
submission. Two initialisation paths, one overrides the other:

1. `PolyPage::SetScaling()` called from `game_mode_changed()` in
   [`game.cpp`](../../new_game/src/game/game.cpp) — sets
   `s_XScale = s_YScale = ScreenHeight / DisplayHeight`. Fallback for
   non-3D code paths before `POLY_camera_set` runs.
2. **`POLY_camera_set` overrides per frame** with the same uniform
   scale — `ScreenHeight / DisplayHeight`. With the FBO pre-clamped
   to `[MIN, CAP]`, the pre-refactor two-axis `min(scale_w, scale_h)`
   "fit" reduced to a single ratio; the uniform scale preserves
   character proportions unconditionally and the viewport always
   fills the FBO edge-to-edge.

**`PolyPage::s_XOffset` / `s_YOffset`** — static floats, added to
vertex X / Y **after** scaling. Always `0` in the default camera
path — the 3D viewport fills the FBO edge-to-edge (no inner
centring). The UI-mode stack overrides them with a per-anchor origin
for framed UI (snapshot/restore around `push_ui_mode` /
`pop_ui_mode`).

### Aspect clamps & compensation

**`OC_FOV_CAP_ASPECT`** — config constant in
[`config.h`](../../new_game/src/config.h). Default `18.0F / 9.0F` (2.0).
Maximum horizontal aspect the scene FBO is sized at. Physical windows
wider than this get pillarboxed (outer bars, painted by composition)
with the FBO clamped to `CAP`. Protects against rectilinear fish-eye at
ultra-wide FOVs.

**`OC_FOV_MIN_ASPECT`** — default `2.0F / 3.0F` (0.667). Minimum
horizontal aspect. Taller / narrower physical windows get letterboxed
(outer bars, painted by composition) with the FBO clamped to `MIN`.
Symmetric to CAP. Avoids "character huge" at narrow horizontal FOVs.

**`OC_FOV_MULTIPLIER`** — default `1.0F`. User-facing FOV slider.
Divides the live camera lens in `aeng.cpp`'s `AENG_lens = …`
assignments. Higher value = wider FOV (zoom out). Does not change
projection type — fish-eye returns if cranked high.

**`auto_zoom`** — computed in `aeng.cpp` (where `AENG_lens` is set)
and again in `poly.cpp` (`POLY_camera_set`, for `POLY_sprite_scale`).
Compensation factor when FBO aspect is narrower than 4:3:

```
if fbo_aspect >= base_aspect (4/3):  auto_zoom = 1
else:                                auto_zoom = base_aspect / max(fbo_aspect, MIN)
```

Applied by **dividing `AENG_lens`** by `auto_zoom × OC_FOV_MULTIPLIER`.
This widens the FOV (smaller lens = wider camera) so the character's
pixel size stays bounded by `ScreenWidth`, not `ScreenHeight`. Extra
screen height fills with more sky / ground instead of pixel-blowing up
the character.

**Must be applied at the `AENG_lens` source, not inside
`POLY_camera_set`**, because `AENG_calc_gamut` (map-square frustum
cull) also consumes the same lens value via its own `width /= lens`
computation. Applying the zoom only inside `POLY_camera_set`
desyncs gamut culling from rendering → geometry visible in the
widened render path gets pre-culled → black cut-outs on the
periphery.

**`POLY_sprite_scale`** — file-scope float in
[`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp).
"World length → virtual screen pixels" scalar used by
`POLY_world_length_to_screen`, `POLY_add_line*`, and
`POLY_get_sphere_circle`. Recomputed per frame in `POLY_camera_set`:

```
POLY_sprite_scale = (DisplayWidth × 0.5 / POLY_ZCLIP_PLANE)
                    / (OC_FOV_MULTIPLIER × auto_zoom)
```

The first factor is the 4:3 design-time value of `POLY_screen_mul_x`
— pinning sprites at aspect-independent size. Dividing by
`(OC_FOV_MULTIPLIER × auto_zoom)` shrinks sprites in proportion
with the two zoom-outs we apply on top of the game's own lens, so
they stay sized relative to the character. Does **not** include
the game's baseline `POLY_cam_lens` (which is ~2.25 by default
from `fc->lens = 0x24000`) — that would 2.25× every sprite, and
original-game cutscene lens changes never resized sprites anyway.

### Aspect-ratio strategies

**Hor+ / Vert- / Pillarbox / Letterbox**:
- **Hor+** = keep vertical FOV, widen horizontal on wider screens.
  What we do in `[4/3, CAP]`.
- **Vert-** = keep horizontal FOV, squash vertical on wider screens.
  Old games. Not what we do.
- **Outer pillarbox** = black bars on left/right of the real
  backbuffer (outside the FBO), painted by the **composition layer**
  when physical window aspect > CAP. Game blind to them.
- **Outer letterbox** = black bars on top/bottom of the real
  backbuffer, painted by composition when physical window aspect < MIN.
- **Inner bars** = black bars around the framed 4:3 UI region (menus,
  frontend, loading, pause) when the FBO aspect is not 4:3. Painted
  by the game itself as ordinary UI draws inside the FBO — separate
  architectural layer from outer bars.
- **Auto-zoom** (our addition) = in `[MIN, 4/3]` don't letterbox the
  gameplay view, instead zoom the camera out so horizontal FOV stays
  4:3-equivalent and the extra vertical space fills with world.

## Vertex pipeline flow

How a single vertex goes from world position to real backbuffer pixel:

```
world (x,y,z)
   │
   │ POLY_transform: MATRIX_MUL by POLY_cam_matrix
   │  └── POLY_cam_matrix has MATRIX_skew baked in with:
   │      aspect = POLY_screen_height / POLY_screen_width   (FBO aspect)
   │      zoom   = POLY_cam_lens = AENG_lens (already divided by
   │               OC_FOV_MULTIPLIER × auto_zoom in aeng.cpp)
   │      scale  = 1 / view_dist
   ▼
view (vx,vy,vz)
   │
   │ POLY_perspective: divide by z, add mid_x/mid_y offset,
   │                   multiply by mul_x/mul_y
   │                   (mul_x/mul_y = POLY_screen_{width,height}/2/ZCLIP)
   ▼
virtual screen pixels: (pt->X, pt->Y)
    range  0..POLY_screen_width × 0..POLY_screen_height
    e.g. on FBO 1920×1080:            0..853   × 0..480
         on FBO 2880×1440 (CAP clamp): 0..960  × 0..480
         on FBO 480×720  (MIN clamp):  0..320  × 0..480
   │
   │ PolyPage::AddFan:
   │   X = pt->X × s_XScale + s_XOffset
   │   Y = pt->Y × s_YScale + s_YOffset
   │   (s_XScale = s_YScale = ScreenHeight / DisplayHeight,
   │    s_XOffset = s_YOffset = 0 in default camera mode)
   ▼
scene FBO pixels: (X, Y)
    range 0..ScreenWidth × 0..ScreenHeight
    e.g. FBO 1920×1080: 0..1920 × 0..1080
         FBO 2880×1440: 0..2880 × 0..1440
         FBO 480×720:   0..480  × 0..720
   │
   │ GL: tl_vert.glsl shader maps FBO pixels → NDC relative to
   │     u_viewport (g_viewData.dwX/dwY/dwWidth/dwHeight = full FBO
   │     by default; cutscene `wideify` / splitscreen override dwY).
   ▼
gl_FragCoord (inside scene FBO)
   │
   │ Composition pass (composition_blit):
   │   Computes aspect-fit dst rect centred in the real backbuffer,
   │   clears the backbuffer to black (outer bars), blits the FBO
   │   into dst via FXAA + bilinear upscale.
   ▼
real backbuffer pixels
```

Characters (`figure.cpp` MM-path) follow a different route — they
build bone matrices from `g_matWorld × g_matProjection`, bake
`dwX + dwWidth/2` and `dwY + dwHeight/2` into per-bone matrix, and
are submitted as pre-transformed vertices (`ge_draw_multi_matrix`).
Output range is the same `[0..ScreenWidth] × [0..ScreenHeight]` FBO
pixels as POLY-path — both paths emit into the same FBO-origin
coordinate space because `s_XOffset = s_YOffset = g_viewData.dwX =
g_viewData.dwY = 0`.

### Why each step matters

- **3D world** goes through the full chain. Its pixel size tracks:
  - `ScreenHeight` in `[base, CAP]` (Hor+ unchanged).
  - `ScreenWidth` in `[MIN, base]` (auto-zoom makes it width-based).
  - `ScreenWidth` at the MIN clamp (FBO height was shrunk to hit MIN).
  - `ScreenHeight` at the CAP clamp (FBO width was shrunk to hit CAP).

- **HUD** skips the `POLY_transform` / `POLY_perspective` step — it
  enters at the `AddFan` step directly, with coords already in
  virtual 640×480 pixel space. Multiplying by the per-frame uniform
  scale gives the 4:3 region inside the FBO → inner-bar gutters on
  non-4:3 FBOs. The UI-mode stack overrides with its own 4:3 framed
  coords (see [`ui_coords_plan.md`](ui_coords_plan.md)) for menus /
  frontend / loading. In-game HUD migration to anchored coords is
  Stage 3c (see [`issues.md`](issues.md)).

- **Sprite / billboard sizes** (lamps, flares, moon, rain, lines)
  go through `POLY_world_length_to_screen` etc. which use
  `POLY_sprite_scale`. Sized at the 4:3 baseline regardless of
  aspect, compensated for auto-zoom + `OC_FOV_MULTIPLIER`, not
  affected by cutscene lens.

- **Frustum / map-square culling** (`AENG_calc_gamut`,
  `POLY_sphere_visible`, `FARFACET_draw`, `POLY_add_line_tex_uv`
  line widths, rain cone in `AENG_draw_rain`) all consume
  `AENG_lens`. Because the lens divisor is applied at the
  `AENG_lens` source, all these paths agree with the render path
  on what's in-view.

- **Post-process passes** (wibble) accept bboxes in virtual 640×480
  space and scale with `× ScreenHeight / 480` on **both** axes (not
  `× ScreenWidth / 640` on X — the old wibble bug; see resolved
  section in [`issues.md`](issues.md)).

- **Composition layer** — one-place reconciliation of FBO size and
  real backbuffer size. Computes aspect-fit dst rect, clears outer
  bars black, blits FBO via FXAA + upscale. No game code except the
  mouse-event mapper (`composition_window_to_fbo`) sees the real
  backbuffer.

## Invariants — don't break these

1. **Auto-zoom and `OC_FOV_MULTIPLIER` are applied at `AENG_lens`
   source** ([aeng.cpp](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
   `AENG_draw`), never inside `POLY_camera_set`. All consumers
   (render + culling paths) must see the same lens.

2. **`POLY_cam_lens` is ~2.25 by default** (from `fc->lens =
   0x24000 / 65536`). **Do not** multiply sprite scales by raw
   `POLY_cam_lens` — that silently inflates sprites 2.25× at the
   4:3 default. If you need lens compensation for sprite size,
   divide by the factors **we added** (`OC_FOV_MULTIPLIER × auto_zoom`)
   only — mirror the `POLY_sprite_scale` formula.

3. **POLY-path and MM-path must emit pixels in the same FBO range**.
   Both start at the FBO origin `(0, 0)` and fill to
   `(ScreenWidth, ScreenHeight)`. If you add a third render path,
   make it consume `g_viewData.dwX/dwY/dwWidth/dwHeight` or
   `PolyPage::s_XOffset/s_YOffset` (both zero in the default camera
   path) — don't reintroduce a viewport-unaware origin.

4. **`PolyPage::s_XScale / s_YScale` are overridden per frame**
   by `POLY_camera_set`. `SetScaling()` from `game_mode_changed()`
   is only a fallback for non-3D paths. If you add a new mode-
   level init, don't assume `SetScaling()` output persists — it's
   overwritten on the first `POLY_camera_set` call each frame.

5. **Sprite / line / sphere sizes go through `POLY_sprite_scale`**,
   not `POLY_screen_mul_x` directly. The historical
   `POLY_screen_mul_x` is aspect-aware (widens on widescreen) and
   was correct for perspective projection, wrong for symmetric
   sizes. Call sites that need the symmetric scalar all migrated
   to `POLY_sprite_scale` — don't add a fresh `POLY_screen_mul_x`
   reference for those use cases.

6. **Outer pillar / letterbox bars live in the composition layer,
   not in game code.** `AENG_clear_viewport` clears the FBO to sky /
   black and returns — it does not paint any bars. The FBO always
   fills its own aspect range edge-to-edge. The composition layer's
   `composition_blit` computes the aspect-fit dst rect, clears the
   real backbuffer to black (outer bars), and draws the FBO into dst.

7. **The game never reads the real window size.** Only
   `composition.cpp` and `sdl3_bridge.cpp` plus the mouse-event
   mapper may query the physical backbuffer. Everywhere else uses
   `ScreenWidth/Height` = FBO size. If you find yourself reaching
   for `sdl3_window_get_drawable_size` in game code, back out — the
   FBO *is* the screen.

## Where scale is picked

Live, per frame, in `POLY_camera_set`:

```cpp
real_aspect       = ScreenWidth / ScreenHeight;   // already in [MIN, CAP]
POLY_screen_width = DisplayHeight × real_aspect;

uniform_scale     = ScreenHeight / DisplayHeight;
PolyPage::s_XScale = PolyPage::s_YScale = uniform_scale;

g_viewData.dwX = 0;
g_viewData.dwY = (POLY_screen_mid_y - fMyMulY) × uniform_scale;  // splitscreen / cutscene wideify only
g_viewData.dwWidth  = ScreenWidth;     // uniform_scale × POLY_screen_width × 2 × ZCLIP / 2
g_viewData.dwHeight = ScreenHeight;    // similarly, up to wideify cutscene letterbox

PolyPage::s_XOffset = PolyPage::s_YOffset = 0;

auto_zoom = (real_aspect < base_aspect)
          ? base_aspect / max(real_aspect, MIN)
          : 1;
POLY_sprite_scale = (DisplayWidth × 0.5 / POLY_ZCLIP_PLANE)
                  / (OC_FOV_MULTIPLIER × auto_zoom);
```

At FBO creation, in `OpenDisplay`:

```cpp
// Step 1: render scale.
scaled_w = int(phys_w × OC_RENDER_SCALE + 0.5);
scaled_h = int(phys_h × OC_RENDER_SCALE + 0.5);

// Step 2: aspect clamp.
scaled_aspect = scaled_w / scaled_h;
if scaled_aspect > OC_FOV_CAP_ASPECT:
    fbo_w = int(scaled_h × OC_FOV_CAP_ASPECT + 0.5);  fbo_h = scaled_h;
elif scaled_aspect < OC_FOV_MIN_ASPECT:
    fbo_w = scaled_w;                                 fbo_h = int(scaled_w / OC_FOV_MIN_ASPECT + 0.5);
else:
    fbo_w = scaled_w;                                 fbo_h = scaled_h;

ScreenWidth  = fbo_w;
ScreenHeight = fbo_h;
```

And at `AENG_lens` source in aeng.cpp (feeds the `POLY_cam_lens` +
all culling paths):

```cpp
AENG_lens = fc->lens × stuff / (OC_FOV_MULTIPLIER × auto_zoom);
```

Any other code that wants to map virtual → FBO pixels must use
`PolyPage::s_XScale` / `s_YScale` (current frame values) — which
are always the uniform `ScreenHeight / DisplayHeight` scalar.
