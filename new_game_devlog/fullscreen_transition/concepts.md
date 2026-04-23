# Concepts & pipeline flow

Terminology + pipeline flow for the 3D/HUD rendering path, updated
through the FOV cap / letterbox floor / auto-zoom / sprite-scale
work. Read this before touching `POLY_camera_set`, `AENG_clear_viewport`,
`PolyPage::AddFan`, or anything that sets camera lens / aspect.

## Glossary

### Virtual / real dimensions

**`DisplayWidth` / `DisplayHeight`** — preprocessor macros in
[`uc_common.h`](../../new_game/src/engine/platform/uc_common.h). **Always 640
and 480.** They define the **virtual game coordinate space** — the reference
frame the original 1999 code drew into. Every hardcoded layout (HUD panel
positions, health bar width, button coords, etc.) assumes this space.

**`RealDisplayWidth` / `RealDisplayHeight`** — runtime `SLONG` globals,
defined in
[`core.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp).
After the render-scale change, **these hold the scene FBO size, not the
physical window** (FBO = physical × `OC_RENDER_SCALE`). Despite the
misleading name — see the pending "Rename `RealDisplayWidth/Height`"
task in [`issues.md`](issues.md). `gl_context_get_width/height` is
hooked to the same FBO dimensions.

### Virtual 3D render space

**`POLY_screen_width` / `POLY_screen_height`** — globals set every frame
by `POLY_camera_set()` in
[`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp). The
**virtual 3D render space** that `POLY_perspective` projects into.

```
POLY_screen_width  = DisplayHeight × effective_aspect
POLY_screen_height = DisplayHeight (full-screen) or DisplayHeight/2 (splitscreen half)
```

`effective_aspect` is clamped to `[OC_FOV_MIN_ASPECT, OC_FOV_CAP_ASPECT]`:
- `real_aspect > CAP`: `effective = CAP` → pillarbox bars left/right.
- `real_aspect < MIN`: `effective = MIN` → letterbox bars top/bottom.
- Otherwise: `effective = real_aspect` → scene fills the screen.

Every 3D vertex, after the software perspective pass
(`POLY_perspective`), comes out in `[0..POLY_screen_width] × [0..POLY_screen_height]`
virtual coords. A `POLY_sprite_scale` file-scope float is also
recomputed here — it's the "world length → virtual pixels" multiplier
used by billboards / lines / sphere radii (see `POLY_sprite_scale`
below).

### Scale factors (virtual → real pixel conversion)

**`PolyPage::s_XScale` / `s_YScale`** — static floats in
[`polypage.cpp`](../../new_game/src/engine/graphics/pipeline/polypage.cpp).
The **final multiplier** applied to every vertex X / Y before GL
submission. Two initialisation paths, one overrides the other:

1. `PolyPage::SetScaling()` called from `game_mode_changed()` in
   [`game.cpp`](../../new_game/src/game/game.cpp) — sets
   `s_XScale = s_YScale = RealH / DisplayHeight`. This is a fallback
   for non-3D code paths (menus using UI mode use their own scale;
   initial frame before `POLY_camera_set` runs).
2. **`POLY_camera_set` overrides per frame** with a uniform "fit"
   scale — `min(RealW / POLY_screen_width, RealH / DisplayHeight)`.
   Picks whichever axis is limiting so the virtual render rect fits
   inside the real framebuffer without overshoot. For aspects in
   `[MIN, CAP]` this equals `RealH / DisplayHeight` (same as path 1).
   For aspects outside the range, the axis-limited scale shrinks the
   render rect — pillarbox or letterbox bars fill the unused space.

**`PolyPage::s_XOffset` / `s_YOffset`** — static floats, added to
vertex X / Y **after** scaling. Used to translate the POLY-path
render region to the same pixel-coordinate origin as the MM-path
(figure.cpp characters — bakes `dwX + dwWidth/2` into bone matrices).
Set in `POLY_camera_set` to `(base_x, base_y)` = the pillar/letter
offsets (see below). Zero on aspects inside `[MIN, CAP]`, non-zero
outside. Also reused by the UI-mode stack (temporary override for
framed UI, snapshot/restore around `push_ui_mode` / `pop_ui_mode`).

### Aspect clamps & compensation

**`OC_FOV_CAP_ASPECT`** — config constant in
[`config.h`](../../new_game/src/config.h). Default `16.0F / 9.0F`.
Maximum horizontal aspect the 3D scene is rendered at. Wider windows
get pillarboxed to this aspect. Protects against rectilinear
fish-eye at ultra-wide FOVs.

**`OC_FOV_MIN_ASPECT`** — default `4.0F / 3.0F`. Minimum horizontal
aspect. Taller (narrower) windows get letterboxed to this aspect.
Symmetric to CAP.

**`OC_FOV_MULTIPLIER`** — default `1.0F`. User-facing FOV slider.
Divides the live camera lens in `aeng.cpp`'s `AENG_lens = …`
assignments. Higher value = wider FOV (zoom out). Does not change
projection type — fish-eye returns if cranked high.

**`auto_zoom`** — computed in `aeng.cpp` (where `AENG_lens` is set)
and again in `poly.cpp` (`POLY_camera_set`, for `POLY_sprite_scale`).
Compensation factor when aspect is narrower than 4:3:

```
if real_aspect >= base_aspect (4/3):  auto_zoom = 1
else:                                 auto_zoom = base_aspect / max(real_aspect, MIN)
```

Applied by **dividing `AENG_lens`** by `auto_zoom × OC_FOV_MULTIPLIER`.
This widens the FOV (smaller lens = wider camera) so the character's
pixel size stays bounded by `RealW`, not `RealH`. Extra screen
height fills with more sky / ground instead of pixel-blowing up
the character.

**Must be applied at the `AENG_lens` source, not inside
`POLY_camera_set`**, because `AENG_calc_gamut` (map-square frustum
cull) also consumes the same lens value via its own `width /= lens`
computation. Applying the zoom only inside `POLY_camera_set`
desyncs gamut culling from rendering → geometry visible in the
widened render path gets pre-culled → black cut-outs on the
periphery.

**`base_x` / `base_y`** — local variables in `POLY_camera_set`,
real-pixel offsets that centre the clamped render rect inside the
real framebuffer:

```
render_w = POLY_screen_width × fit_scale     (real pixels, ≤ RealW)
render_h = DisplayHeight     × fit_scale     (real pixels, ≤ RealH)
base_x   = (RealW - render_w) × 0.5F
base_y   = (RealH - render_h) × 0.5F
```

One of the two is always `0` (whichever axis the fit scale is
limited by). Used for: (1) `g_viewData.dwX` / `dwY` viewport origin
(also propagated to `g_dw3DStuffY` so `figure.cpp` MM-path builds
correct bone matrices); (2) `PolyPage::s_XOffset` / `s_YOffset` so
POLY-path vertices land in the same pixel space as MM-path
characters.

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
- **Pillarbox** = black bars on left/right; scene centered at a
  narrower aspect. What we do when `real_aspect > CAP`.
- **Letterbox** = black bars on top/bottom; scene centered at a
  wider aspect. What we do when `real_aspect < MIN`.
- **Auto-zoom** (our addition) = in `[MIN, 4/3]` don't letterbox,
  instead zoom the camera out so horizontal FOV stays 4:3-equivalent
  and the extra vertical space fills with world.

## Vertex pipeline flow

How a single vertex goes from world position to framebuffer pixel:

```
world (x,y,z)
   │
   │ POLY_transform: MATRIX_MUL by POLY_cam_matrix
   │  └── POLY_cam_matrix has MATRIX_skew baked in with:
   │      aspect = POLY_screen_height / POLY_screen_width   (clamped aspect)
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
    e.g. on 1920×1080 with no cap: 0..853 × 0..480
         on 2560×1080 (CAP=16/9 pillar): 0..853 × 0..480
         on 480×1920 (MIN=4/3 letter):   0..640 × 0..480
   │
   │ PolyPage::AddFan:
   │   X = pt->X × s_XScale + s_XOffset
   │   Y = pt->Y × s_YScale + s_YOffset
   │   (s_XScale = s_YScale = fit_scale, computed in POLY_camera_set;
   │    s_XOffset = base_x, s_YOffset = base_y, the pillar/letter centering)
   ▼
real framebuffer pixels: (X, Y)
    range dwX..dwX+dwWidth × dwY..dwY+dwHeight
    e.g. on 2560×1080 pillar: 320..2240 × 0..1080
         on 480×1920 letter:  0..480 × 240..1680
   │
   │ GL: tl_vert.glsl shader maps screen-pixels → NDC relative to
   │     u_viewport (dwX, dwY, dwWidth, dwHeight). glViewport
   │     positions the output at the centered render region.
   ▼
gl_FragCoord
```

Characters (`figure.cpp` MM-path) follow a different route — they
build bone matrices from `g_matWorld × g_matProjection`, bake
`dwX + dwWidth/2` and `dwY + dwHeight/2` into per-bone matrix, and
are submitted as pre-transformed vertices (`ge_draw_multi_matrix`).
The **output pixel range is the same** `[dwX..dwX+dwWidth] ×
[dwY..dwY+dwHeight]` as POLY-path: `s_XOffset`/`s_YOffset` on the
POLY side is what makes both paths agree.

### Why each step matters

- **3D world** goes through the full chain. Its pixel size tracks:
  - `RealH` in `[base, CAP]` (Hor+ unchanged).
  - `RealW` in `[MIN, base]` (auto-zoom makes it width-based).
  - `RealW` below MIN (letterbox MIN-aspect scale is width-limited).
  - `RealH` above CAP (pillar CAP-aspect scale is height-limited).

- **HUD** skips the `POLY_transform` / `POLY_perspective` step — it
  enters at the `AddFan` step directly, with coords already in
  virtual 640×480 pixel space. Multiplying by the per-frame fit
  scale gives a region smaller than the real framebuffer on
  widescreen → pillarboxed. The UI-mode stack overrides with its
  own 4:3 framed coords (see [`ui_coords_plan.md`](ui_coords_plan.md))
  for menus / frontend / loading. In-game HUD migration to
  anchored coords is Stage 3c (see [`issues.md`](issues.md)).

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
  space and scale with `× RealH / 480` on **both** axes (not
  `× RealW / 640` on X — the old wibble bug; see resolved section
  in [`issues.md`](issues.md)).

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

3. **POLY-path and MM-path must emit pixels in the same
   framebuffer range**. POLY gets the pillar/letter offset via
   `s_XOffset`/`s_YOffset`. MM gets it via `dwX + dwWidth/2`
   baked in per-bone matrix. If you add a third render path,
   make it consume `g_viewData.dwX/dwY/dwWidth/dwHeight` or
   `PolyPage::s_XOffset/s_YOffset` — don't reintroduce a
   viewport-unaware origin.

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

6. **`AENG_clear_viewport` paints the pillar/letter bars** using
   `ge_fill_rect` (scissored `glClear`) **after** the main
   sky-/black- clear. The bar geometry replicates
   `POLY_camera_set`'s `render_x / render_y / render_w / render_h`
   formula so edges abut with no 1-pixel gap. If you change the
   render-rect formula in one place, update the other.

## Where scale is picked

Live, per frame, in `POLY_camera_set`:

```cpp
effective_aspect = clamp(real_aspect, OC_FOV_MIN_ASPECT, OC_FOV_CAP_ASPECT);
POLY_screen_width = DisplayHeight × effective_aspect;

scale_w_fit = RealW / POLY_screen_width;
scale_h_fit = RealH / DisplayHeight;
fit_scale   = min(scale_w_fit, scale_h_fit);

PolyPage::s_XScale = PolyPage::s_YScale = fit_scale;

base_x = (RealW - POLY_screen_width × fit_scale) × 0.5;
base_y = (RealH - DisplayHeight     × fit_scale) × 0.5;
g_viewData.dwX = base_x;
g_viewData.dwY = base_y + (POLY_screen_mid_y - fMyMulY) × fit_scale;
PolyPage::s_XOffset = base_x;
PolyPage::s_YOffset = base_y;

auto_zoom = (real_aspect < base_aspect)
          ? base_aspect / max(real_aspect, MIN)
          : 1;
POLY_sprite_scale = (DisplayWidth × 0.5 / POLY_ZCLIP_PLANE)
                  / (OC_FOV_MULTIPLIER × auto_zoom);
```

And at `AENG_lens` source in aeng.cpp (feeds the `POLY_cam_lens` +
all culling paths):

```cpp
AENG_lens = fc->lens × stuff / (OC_FOV_MULTIPLIER × auto_zoom);
```

Any other code that wants to map virtual → real pixels must use
`PolyPage::s_XScale` / `s_YScale` (current frame values) — **not**
the `RealH / DisplayHeight` formula directly, which only coincides
with the fit scale for aspects in `[MIN, CAP]`.
