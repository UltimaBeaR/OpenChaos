# Resolution / aspect audit

Exhaustive inventory of every place in `new_game/src/` where resolution
or aspect ratio influences behavior. Use as reference when touching
render / UI / input code. See [`concepts.md`](concepts.md) for the
terminology (`DisplayWidth` macro vs `ScreenWidth/Height` globals vs
`POLY_screen_*` etc).

Verdicts:
- **✅ works** — aspect-aware, confirmed correct on widescreen.
- **🔴 broken** — known wrong on non-4:3. Fix tracked in
  [`issues.md`](issues.md).
- **🟡 verify** — looks fine but not tested at widescreen, could hide a
  bug.
- **⚪ low prio / legacy** — works but uses literal `640`/`480` for
  historical reasons; cleanup later.

## 3D rendering pipeline

- ✅ [`poly.cpp` `POLY_camera_set`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  — per-frame camera setup. `POLY_screen_width = DisplayHeight ×
  (ScreenWidth / ScreenHeight)` — no per-frame aspect clamp (FBO is
  pre-clamped in `OpenDisplay`). Sets `PolyPage::s_XScale/s_YScale`
  to uniform `ScreenHeight / DisplayHeight`. `s_XOffset/s_YOffset =
  0` (3D viewport fills FBO edge-to-edge). Recomputes
  `POLY_sprite_scale = (DisplayWidth/2/ZCLIP) / (OC_FOV_MULTIPLIER ×
  auto_zoom)` for world-size billboards. See
  [`concepts.md`](concepts.md) "Vertex pipeline flow".
- ✅ [`aeng.cpp` `AENG_draw`](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  — `AENG_lens = fc->lens × … / (OC_FOV_MULTIPLIER × auto_zoom)`.
  Must be applied here, not inside `POLY_camera_set`: both render
  and frustum culling read from this value.
- ✅ [`aeng.cpp` `AENG_calc_gamut`](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  — draw-distance FOV correction: `width *= POLY_screen_width /
  POLY_screen_height; width /= lens`. Receives widened lens
  automatically via `AENG_LENS`.
- ✅ [`aeng.cpp` `AENG_clear_viewport`](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  — clears the FBO to sky / black (outdoor) or plain black (indoor /
  sewers). No bar painting — outer bars live in the composition layer
  after FBO-as-virtual-screen refactor; FBO always fills its own
  aspect range edge-to-edge.
- ✅ [`poly_globals.cpp:33-36`](../../new_game/src/engine/graphics/pipeline/poly_globals.cpp#L33)
  — `POLY_screen_width/height/mul_x/mul_y/mid_x/mid_y/cam_aspect/cam_lens`
  declarations.
- ✅ [`farfacet.cpp` `FARFACET_draw`](../../new_game/src/engine/graphics/geometry/farfacet.cpp)
  — far-facet cone (`width *= POLY_screen_width / POLY_screen_height;
  width /= lens`): same pattern as `AENG_calc_gamut`.
- ✅ [`sprite.cpp` `SPRITE_draw*` family](../../new_game/src/engine/graphics/geometry/sprite.cpp)
  — sprite size via `POLY_world_length_to_screen` (reads
  `POLY_sprite_scale`). On-screen culling against `POLY_screen_*`.
- ✅ [`shape.cpp` `SHAPE_droplet` / `SHAPE_smoke_trail` etc.](../../new_game/src/engine/graphics/geometry/shape.cpp)
  — rain drops, smoke, trails — all via `POLY_world_length_to_screen`.
- ✅ [`bloom.cpp` `BLOOM_draw` / `BLOOM_flare_draw`](../../new_game/src/engine/graphics/postprocess/bloom.cpp)
  — flare/bloom culling against `POLY_screen_*`; glow via
  `SPRITE_draw_rotated` → `POLY_sprite_scale` → correctly
  aspect-independent.
- ✅ [`pyro.cpp` fire sprites](../../new_game/src/effects/combat/pyro.cpp)
  — pyro size via `POLY_world_length_to_screen`.
- ✅ [`panel.cpp` 3D accuracy rings](../../new_game/src/ui/hud/panel.cpp)
  — weapon crosshair rings via `POLY_world_length_to_screen`.
- ✅ [`poly.cpp` `POLY_add_line` / `POLY_add_line_tex_uv`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  — 3D billboard line widths via `POLY_sprite_scale`.
- ✅ [`poly.cpp` `POLY_get_sphere_circle`](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  — sphere-to-screen radius via `POLY_sprite_scale`.
- ✅ [`poly_render.cpp` / `figure.cpp` MM-path](../../new_game/src/engine/graphics/geometry/figure.cpp)
  — characters. Bakes `g_viewData.dwX + dwWidth/2` and
  `g_dw3DStuffY + g_dw3DStuffHeight/2` into per-bone matrices. Both
  values are `0 + ScreenWidth/2` and `0 + ScreenHeight/2` in the
  default camera path (no pillar/letter centring inside the FBO), so
  POLY-path's `s_XOffset = s_YOffset = 0` agrees — both paths emit
  into the same `[0..ScreenWidth] × [0..ScreenHeight]` pixel range.

## Per-object bounding boxes in virtual space

- ✅ [`aeng.cpp:1425-1426`](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L1425)
  — rain-drop random spawn in virtual 640×480 coords. Correct use of
  `DisplayWidth / DisplayHeight`.
- ✅ [`aeng.cpp:4425-4426`](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L4425),
  [`:4506-4507`](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L4506)
  — moon / figure reflection bbox clamping. The bbox consumers
  (wibble etc.) expect virtual coords, so using the macro is correct.
- ✅ [`aeng.cpp:7601-7632`](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L7601)
  — debug FPS text centering: `DisplayWidth >> 1`.

## Post-process

- ✅ [`wibble.cpp`](../../new_game/src/engine/graphics/postprocess/wibble.cpp)
  — water puddle wibble. Uniform bbox scaling by `ScreenHeight /
  DisplayHeight` on both axes (recent fix).
- 🔴 [`wibble_frag.glsl:51-52`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/wibble_frag.glsl#L51)
  — wibble amplitude doesn't scale with resolution. See
  [`issues.md`](issues.md).
- ✅ [`wibble_effect.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/wibble_effect.cpp)
  — GPU wibble pass. Scratch texture allocated at actual framebuffer
  size. All uniforms in real pixels.

## Framebuffer / texture / memory allocations

- ✅ `s_work_screen_buf` / `WorkScreen*` / `WorkWindowRect` —
  deleted. See [`issues.md`](issues.md) resolved section.
- ✅ Font atlases — fixed 256×256 texture, independent of screen size.
- ✅ Wibble scratch texture — dynamic.

## HUD / UI

- ✅ [`ui/frontend/frontend.cpp`](../../new_game/src/ui/frontend/frontend.cpp)
  — attract/title/briefing/transitions migrated to `ui_coords` framed
  region (Stage 3a). The `if (RealDisplayWidth == 640)` fallback was
  dropped.
- ✅ [`ui/menus/widget.cpp`](../../new_game/src/ui/menus/widget.cpp)
  — main menu, options, save/load, pause. Framed via `push_ui_mode`.
- 🔴 [`ui/hud/panel.cpp`](../../new_game/src/ui/hud/panel.cpp)
  — in-game HUD: health bar, radar, weapon indicator, ammo, body-part
  damage, wristwatch. Hardcoded positions in virtual 640×480 space.
  Currently pillarboxed in the 4:3 framed centre. Needs per-element
  anchors (Stage 3c) — see [`issues.md`](issues.md).
- 🔴 [`ui/hud/overlay.cpp`](../../new_game/src/ui/hud/overlay.cpp)
  — damage / book / kick overlays + panel frames. Same Stage 3c scope.
- 🟡 [`ui/hud/eng_map.cpp:1197-1198`](../../new_game/src/ui/hud/eng_map.cpp#L1197)
  — map screen-size init. Uses `DisplayWidth / DisplayHeight` virtual —
  needs a playthrough to confirm.
- ⚪ [`engine/debug/input_debug/input_debug.cpp:21-22, 54-55, 266-334`](../../new_game/src/engine/debug/input_debug/input_debug.cpp#L54)
  — debug overlay. Uses both `ScreenWidth/Height` (for virtual → FBO
  pixel conversion) and `POLY_screen_*` (for backdrop sizing).
  Dev-only; shares HUD pillarbox issue but low priority.

## Input / mouse

- ✅ [`mouse.cpp` `mouse_on_move` / `mouse_on_button`](../../new_game/src/engine/input/mouse.cpp)
  — SDL3 events arrive in real-window pixel coords; mapped via
  `composition_window_to_fbo` to scene-FBO coords before updating
  `MouseX/Y`. Events in outer pillar / letterbox bars are dropped.
  `MouseX/Y` are in FBO pixels from here on.
- ✅ [`mouse.cpp` `RecenterMouse`](../../new_game/src/engine/input/mouse.cpp)
  — warps OS cursor to window centre, then anchors
  `OldMouseX/Y = ScreenWidth/2, ScreenHeight/2` (FBO centre) for delta
  tracking.
- ✅ [`game_tick.cpp:3446-3447`](../../new_game/src/game/game_tick.cpp#L3446)
  — mouse raytrace hit detection:
  `hitx = MouseX × DisplayWidth / ScreenWidth`. Maps FBO → virtual.
- ✅ [`input_debug.cpp:54-55`](../../new_game/src/engine/debug/input_debug/input_debug.cpp#L54)
  — `to_px_x / to_px_y`: inverse transform virtual → FBO.
- ✅ [`composition.cpp` `composition_window_to_fbo`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp)
  — the one allowed mapping between real-window pixels and FBO
  pixels. Consulted only by `mouse.cpp`.

## Display / viewport setup

- ✅ [`core.cpp` `OpenDisplay`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)
  — reads `sdl3_window_get_drawable_size()`, multiplies by
  `OC_RENDER_SCALE`, clamps aspect to
  `[OC_FOV_MIN_ASPECT, OC_FOV_CAP_ASPECT]`, publishes result as
  `ScreenWidth/Height` (= scene FBO size) and calls `composition_init`.
  Outer pillar / letterbox bars arise when the physical aspect is
  outside the clamp range and live only in the composition layer.
- ✅ [`composition.cpp` `composition_blit`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp)
  — end-of-frame pass. Computes aspect-fit dst rect centred in the
  real backbuffer, clears the backbuffer to black (outer bars),
  blits the scene FBO into dst via FXAA + bilinear upscale. The one
  layer of code that reconciles "FBO size" with "real backbuffer
  size"; all game code stays in FBO coords.
- ✅ [`overlay.cpp:296`](../../new_game/src/ui/hud/overlay.cpp#L296)
  — `ge_set_viewport(0, 0, ScreenWidth, ScreenHeight)` resets the
  viewport to the full FBO before HUD draw.
- ✅ [`sdl3_bridge.cpp`](../../new_game/src/engine/platform/sdl3_bridge.cpp)
  — `sdl3_window_create` queries `SDL_GetDesktopDisplayMode` when
  fullscreen + forces borderless-desktop mode.

## Outro / cutscenes / video

- ✅ [`outro_os_globals.cpp`](../../new_game/src/outro/core/outro_os_globals.cpp)
  — outro has its own `OS_cam_matrix` / `OS_cam_lens` / separate
  `MATRIX_skew`; not affected by `OC_FOV_MULTIPLIER` / `auto_zoom`
  by design (non-interactive cinematic with fixed composition).
  Outro framing into the 4:3 region is handled upstream via
  `ui_coords` (see [`ui_coords_plan.md`](ui_coords_plan.md)).
- ✅ [`core.cpp` `ge_video_draw_and_swap`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)
  — Bink intro videos. Rendered into the scene FBO (no more
  default-framebuffer bypass); aspect-fit into the 4:3 framed region
  inside the FBO (`ui_coords::g_frame_*`), then composition's
  aspect-fit blit + outer bars apply automatically.

## Splitscreen

- ✅ [`poly.cpp` splitscreen cases](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  — three cases (none / top / bottom). Aspect math correct on any
  aspect. Both halves use the same `AENG_lens` (with multiplier +
  auto_zoom applied), so pillar/letter bars and auto-zoom FOV apply
  uniformly across both halves.

---

## Summary

- **Core 3D pipeline**: fully aspect-aware across the full clamp
  range (`[MIN..CAP]`). The scene FBO is aspect-clamped at creation;
  outer pillar/letterbox bars live only in the composition layer.
  Auto-zoom compensates inside the `[MIN..base]` zone. Culling
  (`AENG_calc_gamut`, `POLY_sphere_visible`, farfacet, rain cone)
  syncs with render via `AENG_lens` as the single source.
- **Sprite / line / sphere sizes**: aspect-independent via
  `POLY_sprite_scale`; compensated for `OC_FOV_MULTIPLIER × auto_zoom`;
  unaffected by cutscene lens.
- **Menus / frontend / outro / videos**: framed via `ui_coords`
  pipeline inside the FBO. Inner bars around the 4:3 region are
  painted by the game itself as ordinary UI draws (separate layer
  from composition's outer bars).
- **Mouse**: events mapped from real-window pixels to FBO pixels in
  `mouse_on_move/button`; events in outer bars are dropped.
- **In-game HUD**: still pillarboxed in the 4:3 centre. Needs
  per-element anchors (Stage 3c).
- **Open items**: wibble amplitude, FXAA replacement /
  UI-after-composition split, moon bug cluster. See
  [`issues.md`](issues.md).
