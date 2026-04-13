# Far-building pop-in — FARFACET silhouette system

Issue: distant buildings appeared abruptly at the draw-distance edge on top of the skybox, even in retail-like settings. Fix: revive the FARFACET silhouette LOD system that MuckyFoot already wrote for the retail PC release, and port it properly onto our OpenGL backend.

## What FARFACET does

Builds simplified silhouette meshes of every building on the map at load time, stored per-square in `FARFACET_square[][]`. During the frame it paints those silhouettes onto the framebuffer as a flat "layer on top of the skybox" before any 3D scene geometry, so the zone past the draw distance stays covered by a building silhouette instead of hard-popping when real walls arrive.

## Why it was disabled in our source

Pre-release PC source has a gate:

```c
#ifndef TARGET_DC
    if (!Keys[KB_R]) return;
#endif
```

On Dreamcast always on, on PC only while holding R. The retail PC release removed this gate. We did the same in our port.

## Porting changes for the GL backend

### Stale world matrix

`FARFACET_draw` submits via `ge_draw_indexed_primitive_lit`, which transforms world-space LVERT geometry through `g_matWorld * g_matProjection`. Pre-release D3D6 relied on fixed-function state being set by earlier draws. Our GL path needs an explicit refresh, otherwise far-facets use a stale matrix and bob up/down with the camera pitch:

```c
POLY_set_local_rotation_none();  // uploads current camera view to g_matWorld
```

### `MY_PROJ_MATRIX_SCALE` was a no-op in GL

Pre-release multiplied all 16 elements of the projection matrix by 1.01 as a "Z-bias" to avoid z-fight with real walls. In homogeneous coordinates scaling the entire matrix cancels out after the perspective divide (`v*k/w*k = v/w`). D3D6 fixed-function may have interacted with RHW separately, but our GL pipeline sees it as identity. Removed the hack entirely.

### Render ordering: flat overlay, no depth

Conceptually far-facets are a 2D layer painted over the skybox:

- Skybox drawn first (no depth write).
- Far-facets drawn next with **depth test OFF, depth write OFF**. They composite onto the framebuffer with alpha blend.
- Opaque 3D scene drawn after, with normal depth — overwrites the far-facet layer wherever real geometry exists.
- Translucent pass as usual.

Render state in `FARFACET_renderstate` (init-time):

```c
SetDepthEnabled(false);
SetDepthWrite(false);
SetSrcBlend(SrcAlpha);
SetDstBlend(InvSrcAlpha);
SetAlphaBlendEnabled(true);
```

### Alpha tied to fog density (shader)

The alpha of a far-facet pixel mirrors the fog factor at its distance:

```
vz_tl = v_view_z * 64.0           // convert lit-path v_view_z to TL-path scale
ff = clamp((u_fog_far - vz_tl) / (u_fog_far - u_fog_near), 0, 1)
color.a = 1.0 - ff
```

- Where real walls are still un-fogged (close): `ff = 1` → `alpha = 0`, far-facet invisible, real geometry dominates.
- Where real walls would be fully fogged (past fog_far): `ff = 0` → `alpha = 1`, opaque silhouette.
- In between: smooth ramp that exactly mirrors the wall fog curve so the transition between "real wall fading to fog" and "far-facet emerging from transparency" is seamless.

**Critical scale note:** the lit-path (`ge_draw_indexed_primitive_lit`) stores `rhw = 1/cw`, giving `v_view_z = cw`. The TL-path (`POLY_transform_c`) stores `rhw = POLY_ZCLIP_PLANE/z = (1/64)/z`, giving `v_view_z = 64*z`. **Lit-path `v_view_z` is ~64× smaller than TL-path for the same world distance.** The fog uniforms (`u_fog_near ≈ 38.4`, `u_fog_far ≈ 60.8`) are tuned for the TL scale, so shader multiplies lit `v_view_z` by 64 before comparing.

### Culling tweak

The per-square cull test used to check only the centre of a 4×4 tile square (~4096 world units wide). Whole squares flickered on/off as their centre crossed the draw-distance threshold. Fix: include the half-diagonal (`2897 ≈ 4096/2 * √2`) so any square whose far edge reaches past the draw distance is rendered.

```c
const float SQUARE_RADIUS = 2897.0F;
if (dprod + SQUARE_RADIUS < (float)CurDrawDistance) continue;
```

## F10 debug mode

Toggle via `F9 → bangunsnotgames → F10`. Runs only after the cheat is entered (gated by `allow_debug_keys`).

When on (`g_farfacet_debug != 0`):

- `AENG_draw_city` returns immediately after `FARFACET_draw`, skipping all level geometry. Only skybox + far-facet layer is visible.
- `FARFACET_draw` passes `ge_set_farfacet_mode(2)` to the shader. In that mode the shader paints:
  - **Purple** (`vec3(0.63, 0.13, 0.94)`) where alpha is ≥ 0.99 (fully opaque silhouette).
  - **Green** (`vec3(0.0, 1.0, 0.0)`) anywhere alpha < 0.99 (in the fade band).

This lets you visually verify the fade zone and silhouette body when tuning draw distance or the fog range, without any real walls blocking the view. Console shows "farfacet debug on/off" on each press.

Normal mode (`g_farfacet_debug == 0`) uses `ge_set_farfacet_mode(1)` — RGB comes from `u_fog_color`, alpha from the fog-density formula above.

## Files touched

- `new_game/src/engine/graphics/geometry/farfacet.cpp` — removed `Keys[KB_R]` gate, removed matrix-scale hack, added `POLY_set_local_rotation_none`, render-state setup (no depth, alpha blend), mode 1/2 toggle, square radius cull.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/shaders/common_frag.glsl` — new `uniform int u_farfacet_mode`, alpha formula, debug split colours.
- `new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp` — uniform plumbing for `u_farfacet_mode`, `ge_set_farfacet_mode(int)`.
- `new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h` — `ge_set_farfacet_mode(int)` declaration.
- `new_game/src/engine/graphics/pipeline/aeng.cpp` — `if (g_farfacet_debug) return;` after `FARFACET_draw`.
- `new_game/src/game/game_tick_globals.{cpp,h}` — `g_farfacet_debug` global.
- `new_game/src/game/game.cpp` — F10 toggle with edge-detect.

## Known residual issues

- **Non-building objects** (bridges, elevated platforms, decor) are not part of the far-facet silhouette mesh. They still pop in at the normal draw-distance edge. Future work: extend far-facet generation to cover them, or add per-object distance-fade on the CPU side.
- **Retail also shows a mild transition glitch** between full-detail and silhouette mode for some buildings. Not worse than retail.
