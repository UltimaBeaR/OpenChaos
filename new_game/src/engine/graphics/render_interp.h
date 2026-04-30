#ifndef ENGINE_GRAPHICS_RENDER_INTERP_H
#define ENGINE_GRAPHICS_RENDER_INTERP_H

// Render-side interpolation between physics snapshots.
//
// Physics ticks at a fixed rate (20 Hz design). Render runs faster. Without
// interpolation, render frames between physics ticks show identical positions
// — visible as judder. We snapshot the previous and current physics-tick
// state for moving Things and the camera, then apply interpolated values
// in-place at draw time using alpha = physics_acc_ms / phys_step_ms.
//
// Architecture: one frame-scope (RenderInterpFrame) at the top of draw_screen
// applies interpolated state directly into Thing.WorldPos / Tweened angles
// and FC_cam[0] for the duration of the frame, then restores originals on
// destruction. This way every reader inside the render path sees the
// interpolated values without per-call-site plumbing.

#include "camera/fc.h"

struct Thing;

// Interpolation factor in [0, 1). 0 = at previous tick, ~1 = just before next.
// Recomputed by the game loop after the physics-tick while-loop.
extern float g_render_alpha;

// Master enable for render interpolation. When false, RenderInterpFrame
// becomes a no-op — render reads the live (post-tick) state directly,
// so the world judders as if at physics rate. Capture keeps running so
// re-enabling at runtime gives instant smooth motion. Toggle hotkey: 3.
extern bool g_render_interp_enabled;

// Reset all snapshots. Call when the world resets (mission load, map change)
// to prevent lerping from a stale previous position.
void render_interp_reset(void);

// Capture a Thing's post-tick state. Call once per physics tick, after
// physics has run, for every moving Thing that should be interpolated.
// Position is always captured. Angle/Tilt/Roll are captured for Tween-based
// drawables (persons, animals, bats — DT_TWEEN family) where they live in
// Draw.Tweened. Other DrawType angle storage (Genus.Vehicle->Draw.Angle for
// cars, etc.) is not interpolated in this iteration.
void render_interp_capture(Thing* p_thing);

// Suppress lerp on the next frame for a Thing (after teleport / respawn).
// Effect: prev = curr on the next capture, so lerp returns curr exactly.
void render_interp_mark_teleport(Thing* p_thing);

// Invalidate snapshot for a Thing slot. Call from free_thing — prevents
// the next occupant of this slot from interpolating from the previous
// occupant's stale position. Without this, a freed-then-reallocated slot
// has valid=true with the dead Thing's pose; the first capture of the new
// occupant slides prev=old_pose, curr=new_pose and we lerp across the map.
void render_interp_invalidate(Thing* p_thing);

// Camera capture. Splitscreen unused — pass &FC_cam[0].
void render_interp_capture_camera(FC_Cam* fc);

// Mark camera teleport (cutscene cut, sudden focus change).
void render_interp_mark_camera_teleport(FC_Cam* fc);

// Frame-scope: at construction, walks all valid snapshots and writes
// interpolated values directly into the live Thing.WorldPos / Tweened
// angles and into FC_cam[0]. At destruction, restores the captured-at-tick
// values back so non-render code (logic, AI, physics next tick) sees the
// authoritative position.
//
// Place one of these at the top of draw_screen() — it should encompass the
// entire render frame including AENG_set_camera_radians, FIGURE_draw, and
// every other reader of Thing.WorldPos in the render path.
class RenderInterpFrame {
public:
    RenderInterpFrame();
    ~RenderInterpFrame();

    RenderInterpFrame(const RenderInterpFrame&) = delete;
    RenderInterpFrame& operator=(const RenderInterpFrame&) = delete;
};

#endif // ENGINE_GRAPHICS_RENDER_INTERP_H
