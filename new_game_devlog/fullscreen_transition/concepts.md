# Concepts & pipeline flow

## Glossary

**`DisplayWidth` / `DisplayHeight`** — preprocessor macros in
[`uc_common.h`](../../new_game/src/engine/platform/uc_common.h). **Always 640
and 480.** They define the **virtual game coordinate space** — the reference
frame the original 1999 code drew into. Every hardcoded layout (HUD panel
positions, health bar width, button coords, etc.) assumes this space.

**`RealDisplayWidth` / `RealDisplayHeight`** — runtime `SLONG` globals,
defined in
[`core.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp).
Set at startup (and will be set again on resolution change, once we support
it) from `sdl3_window_get_drawable_size()`. **Actual framebuffer size in
physical pixels.** On 1920×1080 fullscreen these are 1920 and 1080.

**`POLY_screen_width` / `POLY_screen_height`** — globals set every frame by
`POLY_begin()` in
[`poly.cpp`](../../new_game/src/engine/graphics/pipeline/poly.cpp). The
**virtual 3D render space**. Before our changes these were always 640×480.
After our Hor+ fix:
- `POLY_screen_width = DisplayHeight × (RealW / RealH)` → widens on
  widescreen. At 4:3 it reduces to 640 (backwards-compatible).
- `POLY_screen_height = DisplayHeight` (480) for full-screen rendering,
  `DisplayHeight / 2` (240) for splitscreen halves.

Every 3D vertex, after the software perspective pass
(`POLY_perspective`), comes out in this virtual space.

**`PolyPage::s_XScale` / `s_YScale`** — static floats in
[`polypage.cpp`](../../new_game/src/engine/graphics/pipeline/polypage.cpp),
set via `PolyPage::SetScaling()` from `game_mode_changed()` in
[`game.cpp`](../../new_game/src/game/game.cpp). The **final multiplier**
applied to every vertex X / Y before GL submission. After our uniform-scale
fix both are equal: `s_XScale = s_YScale = RealH / 480`. This is what turns
virtual-pixel coords into real-pixel coords.

**Hor+ / Vert- / Pillarbox / Letterbox** — aspect ratio strategies:
- **Hor+** = keep vertical FOV, widen horizontal on wider screens. What we
  do for 3D now.
- **Vert-** = keep horizontal FOV, squash vertical on wider screens. Old
  games did this.
- **Pillarbox** = black bars on left/right; keep a 4:3 image centered.
  What the HUD is effectively doing now (unintentionally).
- **Letterbox** = black bars on top/bottom; keep a wider image fit to
  vertical height.

`stage12.md` spec says the rendered image must fill the monitor (no
letter/pillar box). 3D already satisfies this via Hor+; HUD doesn't yet.

## Vertex pipeline flow

How a single vertex goes from world position to screen pixel:

```
world (x,y,z)
   │
   │ POLY_transform: camera matrix (yaw/pitch/roll) +
   │                 MATRIX_skew (skew = H/W aspect, baked-in FOV)
   ▼
view (vx,vy,vz)
   │
   │ POLY_perspective: divide by z, add mid_x/mid_y offset,
   │                   multiply by mul_x/mul_y (= 0.5×POLY_screen_{width,height}/ZCLIP)
   ▼
virtual screen pixels: (pt->X, pt->Y)
    range  0..POLY_screen_width  ×  0..POLY_screen_height
    (on 1080p: 0..853 × 0..480)
   │
   │ PolyPage::AddFan: multiply by s_XScale, s_YScale (both = RealH/480 = 2.25)
   ▼
real framebuffer pixels
    range  0..RealDisplayWidth  ×  0..RealDisplayHeight
    (on 1080p: 0..1920 × 0..1080)
   │
   │ submitted to OpenGL with an identity-ish projection matrix (the GL side
   │ doesn't do perspective — it's already been done in software above)
   ▼
gl_FragCoord
```

Why this matters:
- **3D world** goes through the full chain, scales correctly with `s_YScale`
  (since `POLY_screen_width` widens to match aspect).
- **HUD** skips the `POLY_transform` / `POLY_perspective` step — it enters
  at the `AddFan` step directly, with coords already in virtual 640×480
  pixel space. Multiplying by `s_YScale = 2.25` gives a 1440×1080 region,
  i.e. a pillarboxed 4:3 area inside the 16:9 framebuffer.
- **Post-process passes** (wibble) need to accept bboxes in virtual 640×480
  space (that's what the caller supplies, since the caller is the 3D
  pipeline's post-perspective coords) and scale them with the **same
  formula as `s_YScale`**, i.e. `× RealH / 480` on BOTH axes. (This was the
  recent wibble bug — it used `× RealW/640` on X, which only coincides
  with `× RealH/480` on 4:3 monitors.)

## Where scale is picked

- `s_XScale = s_YScale = RealH / DisplayHeight` — set in
  [`game_mode_changed()`](../../new_game/src/game/game.cpp) (called from the
  graphics-engine mode-change callback).
- Any other code that wants to map virtual → real pixels must use **the
  same formula**, i.e. `× RealH / DisplayHeight`. Deviating from this
  (e.g. using `× RealW / DisplayWidth`) silently works on 4:3 and silently
  breaks on everything else.
