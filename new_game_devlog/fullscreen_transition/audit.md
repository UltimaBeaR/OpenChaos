# Resolution / aspect audit

Exhaustive inventory of every place in `new_game/src/` where resolution
or aspect ratio influences behavior. Use as reference when touching
render / UI / input code. See [`concepts.md`](concepts.md) for the
terminology (`DisplayWidth` macro vs `RealDisplay*` globals vs
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
  — per-frame camera setup. Clamps `effective_aspect` to
  `[OC_FOV_MIN_ASPECT, OC_FOV_CAP_ASPECT]` → pillar/letter when
  outside. Overrides `PolyPage::s_XScale`/`s_YScale` with a
  uniform fit scale. Sets `s_XOffset`/`s_YOffset` = pillar/letter
  offsets (POLY-path and MM-path coord consistency). Recomputes
  `POLY_sprite_scale` = `(DisplayWidth/2/ZCLIP) /
  (OC_FOV_MULTIPLIER × auto_zoom)` for world-size billboards.
  See [`concepts.md`](concepts.md) "Vertex pipeline flow".
- ✅ [`aeng.cpp` `AENG_draw`](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  — `AENG_lens = fc->lens × … / (OC_FOV_MULTIPLIER × auto_zoom)`.
  Must be applied here, not inside `POLY_camera_set`: both render
  and frustum culling read from this value.
- ✅ [`aeng.cpp` `AENG_calc_gamut`](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  — draw-distance FOV correction: `width *= POLY_screen_width /
  POLY_screen_height; width /= lens`. Receives widened lens
  automatically via `AENG_LENS`.
- ✅ [`aeng.cpp` `AENG_clear_viewport`](../../new_game/src/engine/graphics/pipeline/aeng.cpp)
  — paints black pillar/letter bars via `ge_fill_rect` after the
  standard sky/black clear. Bar geometry mirrors
  `POLY_camera_set`'s `render_x/y/w/h` formula — keep them in sync.
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
  `g_dw3DStuffY + g_dw3DStuffHeight/2` into per-bone matrices;
  relies on POLY-path using `s_XOffset = g_viewData.dwX`,
  `s_YOffset = base_y` to emit vertices in the same pixel space.

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
  — water puddle wibble. Uniform bbox scaling by `RealH/DisplayHeight`
  on both axes (recent fix).
- 🔴 [`wibble_frag.glsl:51-52`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/wibble_frag.glsl#L51)
  — wibble amplitude doesn't scale with resolution. See
  [`issues.md`](issues.md).
- ✅ [`wibble_effect.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/wibble_effect.cpp)
  — GPU wibble pass. Scratch texture allocated at actual framebuffer
  size. All uniforms in real pixels.

## Framebuffer / texture / memory allocations

- 🔴 [`core.cpp:2322-2335`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp#L2322)
  — `s_work_screen_buf`, `WorkScreen*` globals, `WorkWindowRect`. D3D6
  legacy, hardcoded 640×480. See [`issues.md`](issues.md).
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
  — debug overlay. Uses both `RealDisplay*` (for pixel conversion) and
  `POLY_screen_*` (for backdrop sizing). Dev-only; shares HUD pillarbox
  issue but low priority.

## Input / mouse

- ✅ [`game_tick.cpp:3446-3447`](../../new_game/src/game/game_tick.cpp#L3446)
  — mouse raytrace hit detection:
  `hitx = MouseX * DisplayWidth / window_width`. Maps real → virtual.
- ✅ [`input_debug.cpp:54-55`](../../new_game/src/engine/debug/input_debug/input_debug.cpp#L54)
  — `to_px_x / to_px_y`: inverse transform virtual → real.

## Display / viewport setup

- ✅ [`core.cpp:2341+`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp#L2341)
  — `OpenDisplay` reads `sdl3_window_get_drawable_size()` into
  `RealDisplayWidth / RealDisplayHeight`.
- ✅ [`overlay.cpp:296`](../../new_game/src/ui/hud/overlay.cpp#L296)
  — `ge_set_viewport(0, 0, RealDisplayWidth, RealDisplayHeight)` resets
  viewport before HUD draw.
- ✅ [`sdl3_bridge.cpp`](../../new_game/src/engine/platform/sdl3_bridge.cpp)
  — `sdl3_window_create` queries `SDL_GetDesktopDisplayMode` when
  fullscreen + forces borderless-desktop mode.

## Outro / cutscenes

- ✅ [`outro_os_globals.cpp`](../../new_game/src/outro/core/outro_os_globals.cpp)
  — outro has its own `OS_cam_matrix` / `OS_cam_lens` / separate
  `MATRIX_skew`; not affected by `OC_FOV_MULTIPLIER` / `auto_zoom`
  by design (non-interactive cinematic with fixed composition).
  Outro framing into the 4:3 region is handled upstream via
  `ui_coords` (see [`ui_coords_plan.md`](ui_coords_plan.md)).

## Splitscreen

- ✅ [`poly.cpp` splitscreen cases](../../new_game/src/engine/graphics/pipeline/poly.cpp)
  — three cases (none / top / bottom). Aspect math correct on any
  aspect. Both halves use the same `AENG_lens` (with multiplier +
  auto_zoom applied), so pillar/letter bars and auto-zoom FOV apply
  uniformly across both halves.

---

## Summary

- **Core 3D pipeline**: fully aspect-aware across the full clamp
  range (`[MIN..CAP]`). Pillar/letter bars applied outside the
  range with auto-zoom compensation inside the `[MIN..base]` zone.
  Culling (`AENG_calc_gamut`, `POLY_sphere_visible`, farfacet, rain
  cone) syncs with render via `AENG_lens` as the single source.
- **Sprite / line / sphere sizes**: aspect-independent via
  `POLY_sprite_scale`; compensated for `OC_FOV_MULTIPLIER × auto_zoom`;
  unaffected by cutscene lens.
- **Menus / frontend / outro / videos**: framed via `ui_coords`
  pipeline.
- **In-game HUD**: still pillarboxed in the 4:3 centre. Needs
  per-element anchors (Stage 3c).
- **Open items**: wibble amplitude, `WorkScreen*` audit, FXAA
  replacement / UI-after-composition split, `RealDisplay*` rename,
  moon bug cluster. See [`issues.md`](issues.md).
