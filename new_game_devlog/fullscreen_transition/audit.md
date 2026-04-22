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

- ✅ [`poly.cpp:150-240`](../../new_game/src/engine/graphics/pipeline/poly.cpp#L150)
  — `POLY_begin`. Sets `POLY_screen_width = DisplayHeight × RealW/RealH`
  (Hor+). Splitscreen halves adjust `POLY_screen_height`.
- ✅ [`poly_globals.cpp:33-36`](../../new_game/src/engine/graphics/pipeline/poly_globals.cpp#L33)
  — declarations.
- ✅ [`aeng.cpp:571, 670`](../../new_game/src/engine/graphics/pipeline/aeng.cpp#L571)
  — draw-distance FOV correction: `width *= POLY_screen_width / POLY_screen_height`.
- ✅ [`farfacet.cpp:472-473`](../../new_game/src/engine/graphics/geometry/farfacet.cpp#L472)
  — far-facet aspect FOV: same pattern.
- ✅ [`sprite.cpp:34, 128, 224, 322`](../../new_game/src/engine/graphics/geometry/sprite.cpp#L34)
  — sprite on-screen culling against `POLY_screen_*`.
- ✅ [`bloom.cpp:45, 48-49`](../../new_game/src/engine/graphics/postprocess/bloom.cpp#L45)
  — flare/bloom culling and center computed from `ge_get_screen_*`
  (RealDisplay).
- ✅ [`pyro.cpp:1157`](../../new_game/src/effects/combat/pyro.cpp#L1157)
  — pyro effect culling against `POLY_screen_*`.

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

- 🔴 [`ui/hud/panel.cpp`](../../new_game/src/ui/hud/panel.cpp)
  — lines 998, 1004, 1011, 1013, 1054, 1063, 1069, 1079, 1090. All HUD
  panels draw in virtual 640×480 coords with hardcoded positions.
  Pillarboxed on widescreen.
- 🔴 [`ui/hud/overlay.cpp`](../../new_game/src/ui/hud/overlay.cpp)
  — overlays (damage, book, kick, frames). `:296` resets GL viewport to
  RealDisplay dims (correct), but draws are in virtual coords
  (pillarboxed).
- 🟡 [`ui/frontend/frontend.cpp`](../../new_game/src/ui/frontend/frontend.cpp)
  — attract/title/briefing. Has `if (RealDisplayWidth == 640)` branch at
  `:397-402`, switch on `RealDisplayWidth` at `:1794`. Works on non-640
  widths but the literal checks mark legacy assumptions.
- 🔴 [`ui/menus/widget.cpp`](../../new_game/src/ui/menus/widget.cpp)
  — menus. Mouse hit-testing via global cursor pos minus window pos.
  No explicit screen-size literals, but shares HUD draw pipeline.
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

- 🟡 [`outro_os_globals.cpp:34-37, 48-80`](../../new_game/src/outro/core/outro_os_globals.cpp#L34)
  — `OS_screen_width/height` init from `ge_get_screen_width/height()`
  (RealDisplay). Correct. `OS_cam_aspect` etc. not yet audited for
  aspect handling — see [`issues.md`](issues.md).

## Splitscreen

- ✅ [`poly.cpp:159-226`](../../new_game/src/engine/graphics/pipeline/poly.cpp#L159)
  — three cases (none / top / bottom). Aspect math is correct for any
  aspect. Not actively used in the game right now.

---

## Summary

- **Core 3D pipeline**: fully aspect-aware. No known issues.
- **Post-process**: one open amplitude-scaling item (wibble).
- **HUD / UI**: systematically broken (shared pipeline issue); single
  fix pass unblocks it.
- **Legacy buffers** (`WorkScreen*`): need an audit, then either
  documentation or rewrite.
- **Outro / menus / frontend transitions**: not yet tested at widescreen
  — verification pending.
