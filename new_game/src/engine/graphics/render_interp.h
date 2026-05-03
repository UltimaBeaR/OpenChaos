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
#include "engine/core/types.h" // SLONG
#include "engine/core/fmatrix.h" // Matrix33 (BoneInterpTransform)
#include "engine/graphics/geometry/pose_composer.h" // POSE_MAX_BONES

#include <stdint.h> // uint32_t (g_render_interp_frame_counter)

struct Thing;
struct GameKeyFrameElement;

// Cross-anim blend state queried by the render path. When `active` is true
// the renderer composes the displayed pose as
//   lerp(  lerp(old_ae1[i], old_ae2[i], old_tween),
//          live_new_pose_for_part_i,
//          blend_t  )
// for each body part `i`. live_new_pose comes from the live (post-
// RenderInterpFrame) DrawTween fields the caller already has.
struct RenderInterpBlend {
    bool active;
    GameKeyFrameElement* old_ae1;  // FirstElement of pre-transition CurrentFrame
    GameKeyFrameElement* old_ae2;  // FirstElement of pre-transition NextFrame
    SLONG old_tween;               // pre-transition AnimTween, [0, 256)
    float blend_t;                 // [0, 1] — 0=fully old pose, 1=fully new pose
};

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
// Draw.Tweened. CLASS_VEHICLE's separate yaw/tilt/roll
// (Genus.Vehicle->Angle/Tilt/Roll, SLONG, used by draw_car/make_car_matrix)
// are also captured. Other DrawType angle storage (DrawMesh.Angle for static
// rotating mesh, DT_BIKE) is not interpolated in this iteration.
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

// Capture the cutscene-camera global state (EWAY_cam_x/y/z/yaw/pitch/lens).
// Call once per physics tick after EWAY_process_camera updates these globals.
// In-game cutscenes (EWAY) write a separate camera state to these globals
// and EWAY_grab_camera copies them into FC_cam[*] inside the render path,
// overwriting our FC_cam apply with raw post-tick values. Capturing them
// here lets render_interp_apply_eway_camera replace those raw values with
// interpolated ones during AENG_draw.
void render_interp_capture_eway_camera(void);

// Capture the entire DIRT pool (leaves, brass, cans, blood, snow, etc — see
// DIRT_TYPE_* in world_objects/dirt.h). Call once per physics tick after
// DIRT_process so the most recent post-tick values are captured. The frame
// scope (RenderInterpFrame) substitutes lerped pos/angles back into the
// live DIRT_dirt entries during render so debris moves smoothly between
// physics ticks instead of judder-stepping.
void render_interp_capture_dirt(void);

// Mark a DIRT slot as just-teleported (recycle, explicit reposition, etc.).
// Invalidates the snapshot so the next capture goes through the !valid
// (first-capture) path and prev=curr=new — render does not lerp across
// the teleport. Call directly after writing the new x/y/z. `idx` must be
// in [0, DIRT_MAX_DIRT). Safe to call when render-interp is not enabled
// (becomes a cheap snapshot wipe). uc_orig: behaviour analogous to
// render_interp_mark_teleport for Things.
void render_interp_mark_dirt_teleport(int idx);

// Render-time replacement of EWAY_grab_camera's just-written values with
// interpolated ones, when render-interp is enabled and a valid snapshot
// exists. Returns true if substitution happened. Call from the renderer
// immediately after EWAY_grab_camera writes its raw post-tick state into
// fc->x/y/z/yaw/pitch/roll/lens.
bool render_interp_apply_eway_camera(
    SLONG* cam_x, SLONG* cam_y, SLONG* cam_z,
    SLONG* cam_yaw, SLONG* cam_pitch, SLONG* cam_roll,
    SLONG* cam_lens);

// Mark camera teleport (cutscene cut, sudden focus change).
void render_interp_mark_camera_teleport(FC_Cam* fc);

// Query cross-anim blend state for `p_thing`. Sets out->active=false if the
// thing is not currently in a blend. Otherwise fills the four fields the
// renderer needs to mix the pre-transition pose with the live post-transition
// pose for each body part. Safe to call any time during render path.
void render_interp_get_blend(Thing* p_thing, RenderInterpBlend* out);

// Phase 2 debug: retrieve the captured PELVIS world position from the per-
// bone pose snapshot (curr value, not lerped). Returns true and writes
// (x, y, z) iff the snapshot is valid and bones_curr[0] holds usable data.
// Used by the PEL_SNAP debug label in aeng.cpp to verify capture round-trip
// matches PEL_NEW (live compose) — both come from the same composer output
// path, so the labels should overlap exactly. TODO: remove once Phase 3
// apply path consumes the snapshot directly.
bool render_interp_debug_get_pelvis_world(Thing* p_thing, SLONG* out_x, SLONG* out_y, SLONG* out_z);

// Phase 3 apply API: per-bone interpolated world transform — exactly the
// (off_x/y/z, mat_final) pair that figure.cpp's draw functions feed to
// POLY_set_local_rotation per body part.
struct BoneInterpTransform {
    float pos_x, pos_y, pos_z;     // world position in figure.cpp's "off_x/y/z" space
    Matrix33 rot;                   // world rotation matrix (mat_final equivalent, fixed-point ×32768)
};

// Frame counter, incremented on every RenderInterpFrame ctor. Used as a
// cache invalidation key by consumers (e.g. figure.cpp's per-Thing pose
// cache) so they refresh their cached data each render frame regardless
// of whether g_render_alpha happens to repeat across frames.
extern uint32_t g_render_interp_frame_counter;

// Phase 3 apply: compute interpolated world transform for every bone.
// Walks the per-bone snapshot (g_pose_snaps[idx]) at g_render_alpha:
//   bone[0] (PELVIS): lerp(prev, curr) world pos, slerp world rot.
//   bones[1..14]: lerp(prev, curr) parent-local pos + slerp parent-local rot,
//                 then chain through parent's interpolated world transform.
//                 Hierarchy reconstruction preserves rigid bone connections
//                 even at large rotational deltas per tick.
//
// `out` must point to an array of 15 BoneInterpTransform.
// Returns false if the snapshot is invalid or master interp toggle is off
// or the world-pose flag is disabled — caller should fall back to the legacy
// figure.cpp pose-composition path.
bool render_interp_compute_pose(Thing* p_thing, BoneInterpTransform out[POSE_MAX_BONES]);

// Cached variant of render_interp_compute_pose. Returns a pointer to internal
// storage holding the interpolated pose for this Thing in the current render
// frame, or nullptr if the pose is unavailable (interp off, world-pose flag
// off, snapshot invalid for this Thing).
//
// Cache is keyed by (Thing*, g_render_interp_frame_counter). Multiple
// consumers within one render frame (main body, water reflection, shadow
// projection) all hit the same cached data — hierarchy walk happens at
// most once per Thing per frame regardless of how many draw paths consume
// it. Cross-Thing transitions invalidate naturally because the pointer
// changes; cross-frame transitions invalidate via the frame counter bumped
// in RenderInterpFrame::ctor.
//
// Returned pointer is valid until the next call with a different Thing or
// the next render frame, whichever comes first. Treat as read-only.
const BoneInterpTransform* render_interp_get_cached_pose(Thing* p_thing);

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
