#pragma once

// Compile-time debug toggles for render-side interpolation.
// Sibling to debug_config.h — split out because there are many independent
// interpolation paths and we need to A/B isolate which one causes a given
// visual artifact.
//
// Master toggle is key `3` at runtime (`g_render_interp_enabled`). When the
// master is OFF, NO interpolation runs anywhere regardless of these flags.
// When master is ON, only the paths whose flag is `true` here run; others
// behave as if the master was off for that path.
//
// Default = all true → standard fully-interpolated behaviour.
// Set individual flag to false to skip just that path while leaving others
// active.
//
// All flags are compile-time `static constexpr bool` to make the dead
// branches drop out at -O2 — zero release-build cost.

namespace ri_cfg {

// === Things (CLASS_PERSON, CLASS_PROJECTILE, CLASS_ANIMAL, etc.) ===

// Lerp Thing.WorldPos between physics ticks. Without this, body positions
// snap on each physics tick (judder at 20 Hz).
static constexpr bool INTERP_THING_POS = true;

// Lerp Draw.Tweened->Angle/Tilt/Roll (whole-body orientation for the Tween
// DrawType family — DT_ROT_MULTI for persons, DT_ANIM_PRIM for animals/
// bats, DT_TWEEN generic). Without this, body orientation snaps each tick.
static constexpr bool INTERP_THING_TWEEN_ANGLE = true;

// Substitute Draw.Tweened->AnimTween + CurrentFrame + NextFrame so vertex
// morph between keyframes follows render-time alpha instead of stepping
// per physics tick. Without this, anim playback judders at physics rate
// (visible especially on 60+ FPS render with 20 Hz physics).
static constexpr bool INTERP_THING_ANIM_MORPH = true;

// Cross-anim per-body-part blend during animation transitions
// (render_interp_get_blend, queried by figure.cpp morph functions). Without
// this, anim transition is a discrete pose swap (no fade between old and
// new pose's body parts).
static constexpr bool INTERP_THING_CROSS_ANIM_BLEND = true;

// World-space per-bone pose snapshot (Phase 3 of world_pose_snapshot_plan.md).
// When true, figure.cpp queries the per-bone snapshot in render_interp.cpp and
// overrides the per-bone (off_x/y/z, mat_final) computed via the legacy
// AnimTween/keyframe substitution path. This is the "render the lerp of two
// physics-tick world poses" architecture that supersedes the AnimTween virtual-
// extended-coordinate trick + cross-anim blend hacks. False = legacy path
// (current dt->* substitution behaviour). True = new path. Phase 5 will delete
// the legacy code entirely once this flag has been validated.
static constexpr bool INTERP_THING_WORLD_POSE = true;

// === Debug visualisation (off by default in normal play) ===

// Per-character debug labels in 3D world (PEL, ROOT, PEL_NEW, PEL_SNAP).
// Used during Phase 0/1/2 verification to confirm the visible pelvis bone is
// a smooth interpolation anchor, the pure pose composer matches figure.cpp's
// render output, and snapshot capture round-trips correctly. Off by default —
// turn on to inspect again. Render path skips them at compile time when false
// (zero release-build cost).
static constexpr bool DEBUG_POSE_LABELS = false;

// === Vehicles ===

// Lerp Genus.Vehicle->Angle/Tilt/Roll (separate from Tweened angles —
// vehicles use their own SLONG yaw/tilt/roll that drive make_car_matrix).
// Without this, car orientation snaps on each tick (visible during sharp
// turns at 20 Hz physics).
static constexpr bool INTERP_VEHICLE_ANGLE = true;

// === Cameras ===

// Lerp FC_cam[0] x/y/z/yaw/pitch/roll. Without this, the player camera
// snaps each physics tick (player body lerps but camera doesn't, so body
// appears to slide relative to view).
static constexpr bool INTERP_FC_CAM = true;

// Lerp the EWAY cutscene camera (separate state in EWAY_cam_x/y/z/yaw/
// pitch/lens globals). Without this, in-game cutscene cameras snap each
// physics tick.
static constexpr bool INTERP_EWAY_CAM = true;

// === DIRT pool (leaves, brass, cans, blood, snow, etc.) ===

// Lerp DIRT_dirt[i].x/y/z/yaw/pitch/roll for active entries. Without this,
// debris (especially leaves blown by player passage) judders at physics
// rate.
static constexpr bool INTERP_DIRT = true;

} // namespace ri_cfg
