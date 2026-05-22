---
name: debug-draw-3d
description: How to draw in-world 3D debug primitives (lines, spheres, axes, markers) in OpenChaos — what's available, the historical render-pass guard gotcha, the g_matWorld pollution gotcha, naming conventions, and how to add a new debug primitive. Use this whenever you need to visualise something in world space for diagnostics — bone positions, hit points, ray traces, AI paths, collision shapes, physics queries, sample points, anything you want to look at on top of the rendered scene. NOT for 2D screen-space text overlays — those go through the separate text-rendering / debug-log skills.
---

# In-world 3D debug primitives

For diagnostic visualization of world-space data — joint positions, AI paths,
collision shapes, ray hits, anything you want to see overlaid on the rendered
scene. The primitives are billboard quads at scene depth, not a separate
debug shader pass.

## Available primitives

Both live in [aeng.cpp](../../../new_game/src/engine/graphics/pipeline/aeng.cpp) /
[aeng.h](../../../new_game/src/engine/graphics/pipeline/aeng.h). Both use
`POLY_PAGE_COLOUR` (no texturing, no lighting) and are flushed by
`POLY_frame_draw` inside `AENG_draw` along with the rest of the scene.

```cpp
// Single world-space line. Per-endpoint pixel width and color.
// sort_to_front=UC_TRUE renders over scene geometry.
void AENG_world_line(
    SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    SLONG sort_to_front);

// Wireframe sphere — three perpendicular great circles (XY/XZ/YZ).
// 6 segments per circle (18 lines total). Always sort_to_front.
void AENG_world_sphere(
    SLONG cx, SLONG cy, SLONG cz,
    SLONG radius, SLONG width_px,
    ULONG colour);
```

Coords are MS world units (the same scale as `Thing::WorldPos.X >> 8`).
Colors are `0xAARRGGBB` (alpha is ignored on `POLY_PAGE_COLOUR` — it's
opaque). Pixel widths are screen-space, so they don't shrink with distance.

## DO NOT use game primitives for debug

These render into the same `POLY_PAGE_COLOUR` and look superficially similar,
but they're part of gameplay rendering — using them as debug pollutes the
gameplay rendering path and makes future refactoring (e.g. P2-J removal of
toggle infrastructure) ambiguous between "this draws something the player
sees" and "this draws something a developer sees".

- `SHAPE_sphere` — used for game balloons. Has built-in ambient lighting
  (calls `NIGHT_ambient_at_point`), so the debug ball would dim at night.
- `AENG_world_line_nondebug` — used for grenades, vehicle effects.
- `AENG_world_line_infinite` — used for road rendering.
- `POLY_add_line` / `POLY_add_quad` directly — these are the underlying
  building blocks the AENG_world_* primitives wrap. Going around the
  wrapper means future fixes to the wrapper (like the render-pass guard
  fix below) don't reach you.

If you find yourself reaching for one of these for diagnostics, add a new
debug-named wrapper instead (see "Adding a new primitive" below).

## Naming convention

Within `aeng.h/cpp`, the convention is:

- **Bare name** (`AENG_world_line`, `AENG_world_sphere`) = **debug** primitive.
  Free to call from any debug overlay. Removed at the end of feature work.
- **`_nondebug` suffix** (`AENG_world_line_nondebug`) = **gameplay-use**.
  Stays in shipped builds.

Stick to this when adding new debug primitives — it lets future cleanup
passes find "all debug rendering" by grepping the bare names.

## Historical gotcha: the render-pass guard

`AENG_world_line` used to silently drop every call made during scene
rendering. There was a guard at the top of the function:

```cpp
if (!s_in_render_pass) return;
```

`s_in_render_pass` was set true only at the **very end** of `AENG_draw`,
after `POLY_frame_draw` had already flushed `POLY_Page` to GL. So calls
made during the scene draw (when AI/nav/figure code wants to add debug
lines) hit the guard, returned, and nothing ever rendered. The guard
was over-zealous protection against a corner case ("shadow VB corruption
if added outside the pass") that didn't apply to the normal case.

**Current state**: the guard is gone. `AENG_set_render_pass` is kept as a
no-op shim for API compatibility. `AENG_world_line` always emits, and
`POLY_frame_draw` inside `AENG_draw` flushes whatever was added during
the scene along with everything else.

**Takeaway when adding a new debug primitive**: don't reintroduce the
guard. Add a clear comment that calls are safe from any phase of the
frame (well, any phase before `POLY_frame_draw` runs — that's basically
"during scene draw or earlier"). If you find yourself adding a render-
pass check, ask whether it's actually needed.

## Gotcha: g_matWorld pollution at the call site

`AENG_world_line` calls `POLY_transform(x, y, z, &p)` internally, which
multiplies the coords by the current `g_matWorld`. For most call sites
(AI tick, physics update, navigation overlay) `g_matWorld` is at its
"world-identity" state and absolute world coords go through correctly.

But some scene-drawing code paths leave `g_matWorld` set to a per-object
matrix that's NOT identity. The known case: the figure FK recurse
(`FIGURE_draw_hierarchical_prim_recurse` → bone draws) leaves
`g_matWorld` as the last bone's local transform. If you call
`AENG_world_line` with absolute world coords from that context, the
coords get multiplied by the bone matrix and end up in arbitrary space —
the line either clips off-screen or draws in the wrong place.

**Symptom**: lines invisible or in completely wrong locations when
emitted from inside per-figure / per-bone draw code, but the same call
with the same coords works fine from `game_tick` or another clean spot.

**Fix at the call site**:

```cpp
extern GEMatrix g_matWorld;
const GEMatrix saved_world = g_matWorld;
POLY_set_local_rotation_none();   // writes camera-only world matrix

AENG_world_line(/* absolute world coords */);
AENG_world_sphere(/* absolute world coords */);

g_matWorld = saved_world;         // restore — caller's per-material
                                  // fog calc etc. depend on this
```

`POLY_set_local_rotation_none` rewrites `g_matWorld` with the
camera-only matrix (object at world identity). It's the same helper
`figure_build_screen_xform_bake` uses for the same reason.

If a primitive accepts world coords and doesn't mention `g_matWorld`,
assume it has this gotcha — `POLY_transform` is how the engine maps
3D to screen and it always reads `g_matWorld`.

## Adding a new debug primitive

Two valid approaches:

### 1. Compose from `AENG_world_line` (preferred when geometric)

Sphere, box, cross, axes, frustum — anything that can be expressed as a
bunch of line segments. Goes next to `AENG_world_line` in aeng.cpp.
Inherits any future fixes to the line implementation automatically.

```cpp
// aeng.h
void AENG_world_box(
    SLONG cx, SLONG cy, SLONG cz,
    SLONG half_x, SLONG half_y, SLONG half_z,
    SLONG width_px, ULONG colour);

// aeng.cpp
void AENG_world_box(/* params */) {
    // 12 edges. Each one is a single AENG_world_line call.
    AENG_world_line(/* min corner → x edge end */);
    // ... 11 more
}
```

See `AENG_world_sphere` for a worked example.

### 2. Direct `POLY_transform` + `POLY_add_line` (when you need more control)

Useful if you want per-vertex color interpolation or custom screen-space
math that the line wrapper doesn't expose. Pattern:

```cpp
POLY_Point p1, p2;
POLY_transform(float(x1), float(y1), float(z1), &p1);
POLY_transform(float(x2), float(y2), float(z2), &p2);
if (POLY_valid_line(&p1, &p2)) {
    p1.colour = colour1;
    p1.specular = 0xff000000;
    p2.colour = colour2;
    p2.specular = 0xff000000;
    POLY_add_line(&p1, &p2, width1, width2, POLY_PAGE_COLOUR, sort_to_front);
}
```

`p.specular` must be set to `0xff000000` — the alpha is read from there,
not from the color value. Off-screen / behind-camera lines are rejected
by `POLY_valid_line` so the wrapper doesn't go through the billboard
math for them.

## Call-site sanity checklist

When debug lines aren't showing up:

1. **Is `g_matWorld` at world-identity at the call site?** If you're in
   figure / mesh / per-object draw code, probably not. Save/restore +
   `POLY_set_local_rotation_none` (see gotcha above).
2. **Are the coords absolute world coords?** `Thing::WorldPos.X >> 8` is
   the typical shape. Bone positions in interpolated transforms (the
   `BoneInterpTransform::pos_*` fields) are already in world space at
   scale 1 and can be passed straight in.
3. **Is the line actually inside the view frustum?** `POLY_valid_line`
   rejects fully off-screen segments. Try a much shorter line near a
   known visible point first.
4. **Is the line emitted before `POLY_frame_draw` runs?** Everything
   inside `AENG_draw` (the entire scene draw) is fine. The very narrow
   window between `POLY_frame_draw` and the end of the frame is NOT —
   debug calls there land in pages that were just cleared and won't get
   flushed. If you really need to draw at end-of-frame, do it from
   inside an `AENG_draw_*` helper or from a callback that runs before
   the flush.

## Performance

Each `AENG_world_line` is one billboard quad (2 triangles, 4 verts).
`AENG_world_sphere` is 18 lines = 36 triangles. Both go through the
standard `POLY_PAGE_COLOUR` render state — no per-call state changes.

Cost when no debug calls happen in a frame: **zero**. The primitives
themselves do nothing unless called, and the underlying `POLY_Page` is
the same one gameplay polys use, so there's no extra GL submission.

Cost when calling: roughly the same as drawing the equivalent solid
geometry. Cheap enough to instrument freely — 540 lines per character's
skeleton (15 joints × 18 sphere lines + 14 bone lines) costs about
1000 triangles. Steam Deck handles this without notice.

## Things this skill does NOT cover

- **2D screen-space text** — see the text-rendering / debug-log skills.
  The patterns are completely different: text uses `POLY_PAGE_TEXT` and
  has its own font/layout pipeline.
- **Persistent debug visualizations across frames** — these primitives
  are one-shot per frame. To keep something on screen, call from a
  per-frame hook (e.g. `game_tick` or inside the relevant `_draw_`
  function). There's no debug-line *buffer* you can write into ahead
  of time and forget about.
- **Toggling debug overlays** — gate at the caller, not inside the
  primitive. Common pattern for a "keeper" debug visualization (one
  that survives past the feature-work cleanup):
  1. `bool g_my_overlay_enabled = false;` global with extern header decl.
  2. Key handler in `game_tick.cpp` under `allow_debug_keys` (= the
     `bangunsnotgames` cheat-mode gate) that flips it. CONSOLE_status
     to show the new state.
  3. The actual debug-draw code wrapped in `if (g_my_overlay_enabled)`
     so when off it costs exactly one branch per call site, zero
     POLY_transform / AENG_world_* / matrix work.
  4. Row added to `debug_help.cpp` `s_rows[]` (F1 legend in
     bangunsnotgames mode) AND to `new_game_devlog/debug_keys.md`.
     Both are mandatory — the F1 legend is the in-game discovery
     surface, the markdown is the documentation source of truth.

  Live example: B toggles the per-figure skeleton overlay
  ([figure.cpp](../../../new_game/src/engine/graphics/geometry/figure.cpp)
  inside `figure_build_skin_world_palette`, flag
  `g_skin_debug_draw_skeleton` in
  [bind_palette.h](../../../new_game/src/engine/graphics/geometry/bind_palette.h)).
  Key originally H, moved to B because H was already taken by
  `plan_view_shot()` (Shift+H, top-down map TGA) — when picking a new
  debug key, grep `KB_<letter>` across the codebase first and look for
  collisions both before AND after the implicit `if (!allow_debug_keys)
  return;` gate in `game_tick.cpp`. Two-handler collisions fire BOTH
  handlers on a single key press, with confusing side-effects.
