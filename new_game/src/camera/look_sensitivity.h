#ifndef CAMERA_LOOK_SENSITIVITY_H
#define CAMERA_LOOK_SENSITIVITY_H

#include "engine/platform/uc_common.h" // SLONG

// ======================================================================
// Camera-look sensitivity (OpenChaos -- no original)
// ======================================================================
//
// One sensitivity knob PER DEVICE (0..1) scales BOTH axes (horizontal
// orbit + vertical) together, linearly between two compile-time bounds:
//     scale = ABS_MIN + (ABS_MAX - ABS_MIN) * sensitivity
//   ABS_MIN -- absolute rotation rate felt at sensitivity 0 (slow end)
//   ABS_MAX -- absolute rotation rate felt at sensitivity 1 (fast end)
//
// The live sensitivity (0..1) and the vertical-invert flag come from
// config.json at runtime (section "mouse" / "gamepad", keys
// camera_orbit_sensitivity / camera_orbit_invert_y) via the accessors at
// the bottom. ABS_MIN/ABS_MAX and the calibration knobs below stay
// compile-time.
//
// INVARIANTS (do not break):
//   * V/H ratio is LOCKED to the shipping feel: vertical is DERIVED from
//     horizontal (never tuned separately), so both axes always move in
//     lockstep at the exact ratio the game ships with.
//   * UNIFORM across branches: a sensitivity delta speeds up every look
//     branch of a device by the SAME proportion (orbit and zoom/L1 look
//     both scale off the same per-device scale below).

// ----------------------------------------------------------------------
// >>> TUNE THESE <<<  (compile-time bounds; the live 0..1 sensitivity is
// in config.json, see the accessors at the bottom)
// ----------------------------------------------------------------------
// Mouse: yaw scale bounds in Q8 fixed-point game-angle units per pixel.
constexpr SLONG MOUSE_SENS_ABS_MIN = 30; // sensitivity 0 (slow)
constexpr SLONG MOUSE_SENS_ABS_MAX = 400; // sensitivity 1 (fast)

// Gamepad right stick: yaw bounds in game-angle units/tick at full deflection.
constexpr SLONG STICK_SENS_ABS_MIN = 20; // sensitivity 0 (slow)
constexpr SLONG STICK_SENS_ABS_MAX = 150; // sensitivity 1 (fast)

// Zoom / L1 / E look speed as a fraction of the NORMAL orbit speed (both
// devices). The first-person zoom look turns the character instead of
// orbiting the camera, but runs at this fraction of normal at ANY
// sensitivity (it tracks the same per-device scale, then scales).
constexpr float ZOOM_LOOK_RATIO = 0.7f; // L1 look = 0.7x normal speed

// ----------------------------------------------------------------------
// DERIVED -- do NOT edit (locks the V/H ratio and cross-axis uniformity)
// ----------------------------------------------------------------------
// Shipping V/H pairing: at STICK_YAW_REF units/tick of yaw the height
// delta is STICK_HEIGHT_BASE. This single ratio derives vertical from
// horizontal on BOTH devices (~205.6 height-units per yaw game-unit), so
// the V/H ratio never changes with sensitivity.
constexpr SLONG STICK_YAW_REF = 61; // game-angle units/tick (ref)
constexpr SLONG STICK_HEIGHT_BASE = 0x3100; // height-units/tick (ref)

// L1/zoom vertical speed relative to L1 horizontal, per device (calibrated
// by feel: look-pitch and character-yaw reach the screen with different
// gains, so the value that "feels equal" isn't 1.0 in raw terms).
constexpr float ZOOM_MOUSE_VH = 0.8f;
constexpr float ZOOM_STICK_VH = 0.8f;

// ----------------------------------------------------------------------
// Runtime values (backed by config.json, lazily loaded on first use)
// ----------------------------------------------------------------------
// camera_orbit_sensitivity (0..1) and camera_orbit_invert_y (bool), per
// device, under the "mouse" and "gamepad" config sections. Defined in
// look_sensitivity.cpp.
float CAM_mouse_orbit_sensitivity(); // 0..1
float CAM_gamepad_orbit_sensitivity(); // 0..1
bool CAM_mouse_invert_y();
bool CAM_gamepad_invert_y();

// ----------------------------------------------------------------------
// Derived scales (runtime; depend on the live sensitivity above)
// ----------------------------------------------------------------------
// Mouse horizontal (linear lerp of the bounds by the live sensitivity).
inline SLONG mouse_yaw_scale_q8()
{
    return MOUSE_SENS_ABS_MIN
        + (SLONG)((MOUSE_SENS_ABS_MAX - MOUSE_SENS_ABS_MIN) * CAM_mouse_orbit_sensitivity());
}
// Mouse vertical, derived from mouse yaw via the reference ratio.
inline SLONG mouse_height_scale()
{
    return mouse_yaw_scale_q8() * STICK_HEIGHT_BASE / (STICK_YAW_REF * 256);
}

// Stick horizontal (linear lerp of the bounds by the live sensitivity).
inline SLONG stick_yaw_scale()
{
    return STICK_SENS_ABS_MIN
        + (SLONG)((STICK_SENS_ABS_MAX - STICK_SENS_ABS_MIN) * CAM_gamepad_orbit_sensitivity());
}
// Stick vertical, derived from stick yaw via the same reference ratio.
inline SLONG stick_height_scale()
{
    return stick_yaw_scale() * STICK_HEIGHT_BASE / STICK_YAW_REF;
}

// Zoom / L1 / E look scales (yaw shares the normal orbit-yaw units).
inline SLONG zoom_mouse_yaw_q8() { return (SLONG)(mouse_yaw_scale_q8() * ZOOM_LOOK_RATIO); }
inline SLONG zoom_mouse_pitch_q8() { return (SLONG)(zoom_mouse_yaw_q8() * ZOOM_MOUSE_VH); }
inline SLONG zoom_stick_yaw() { return (SLONG)(stick_yaw_scale() * ZOOM_LOOK_RATIO); }
inline SLONG zoom_stick_pitch() { return (SLONG)(zoom_stick_yaw() * ZOOM_STICK_VH); }

#endif // CAMERA_LOOK_SENSITIVITY_H
