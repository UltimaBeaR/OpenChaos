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
