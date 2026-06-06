#ifndef CAMERA_CAMERA_MODE_H
#define CAMERA_CAMERA_MODE_H

// Camera behaviour modes — independent of input device.
//
// AUTO    — Classic Urban Chaos third-person camera. Rotation and
//           wall-collision are smoothed (rubber-band feel). Auto
//           features active: get-behind on character motion, idle-return
//           to default height, fight-target follow when target changes.
//           Smoothing amount is tunable via `get_camera_rubberness()`
//           (one knob — controls horizontal/vertical rotation and
//           wall-collision blend in lockstep). Position smoothing
//           (want → fc) is NOT scaled by rubberness — it always runs
//           in both modes so the camera smoothly follows character
//           motion (jumps, stairs, platforms) without snapping.
//
// MANUAL  — Modern FPS/TPS camera. Rotation is instant (snap). Position
//           smoothing remains for character motion (so cam doesn't
//           teleport on jumps/stairs). No auto-rotation triggers — the
//           player's manual input is the SOLE rotation source. Exception:
//           explicit "scene override" moments (car entry animation, etc.)
//           which the camera code drives directly regardless of mode.
//
// Both modes are fully supported for both input devices — the default
// mapping by device is configurable below.
enum CameraMode {
    CAMERA_MODE_AUTO,
    CAMERA_MODE_MANUAL,
};

// Compile-time default mapping from input device to camera mode.
// Edit either of these to retarget a device project-wide (e.g.
// `KBM_DEFAULT_CAMERA_MODE = CAMERA_MODE_AUTO` to make the mouse use
// the rubbery classic camera).
//
// At runtime the active mode is derived by `get_active_camera_mode()`
// from `active_input_device` and these constants. No code path checks
// the input device directly for camera behaviour — only the mode.
constexpr CameraMode KBM_DEFAULT_CAMERA_MODE = CAMERA_MODE_MANUAL;
constexpr CameraMode GAMEPAD_DEFAULT_CAMERA_MODE = CAMERA_MODE_AUTO;

// Rubberness scalar [0..1] for AUTO mode only. Tunes ROTATION rubber
// in lockstep — one knob covering the things that produce a rubber-
// band feel during camera ROTATION (i.e. how the camera reorients,
// orbits, and recovers from wall contact). Specifically:
//   - horizontal/vertical angle smoothing (yaw/pitch/roll lag),
//   - orbit-lag residual on user input (mouse/stick rotation tail),
//   - wall-collision blend (camera orbit pull-in/recovery on walls).
//
// NOT scaled by rubberness (always smoothed, both modes):
//   - Position smoothing (camera following character motion through
//     jumps, stairs, platforms) — always on regardless of mode. Camera
//     never snaps to the character's current position; this is what
//     keeps motion smooth on bumps / curbs / climbs.
//   - Y-convergence (auto-return to default height) — AUTO-mode feature.
//   - Get-behind rates — AUTO-mode feature.
// Rubberness is specifically about ROTATION feel, not translation.
//   0.0  — snap (matches MANUAL feel but with AUTO features enabled).
//   0.5  — current "classic Urban Chaos" feel (rotation 25%/tick, wall
//          60/40 blend). Default — preserves shipping behaviour on
//          gamepad bit-for-bit (modulo tiny float-rounding differences
//          on negative deltas, visually identical).
//   1.0  — maximum laziness (very slow follow, exaggerated rubber band).
//
// MANUAL mode ignores this completely (always snap).
constexpr float CAMERA_RUBBERNESS_DEFAULT = 0.5f;

// Single source of truth: returns the active camera mode based on
// `active_input_device` and the compile-time default mapping above.
// Every camera-behaviour gate in fc.cpp / vis_cam.cpp calls this —
// `active_input_device` is no longer checked directly for camera logic.
CameraMode get_active_camera_mode();

// Convenience predicates.
inline bool camera_is_manual() { return get_active_camera_mode() == CAMERA_MODE_MANUAL; }
inline bool camera_is_auto() { return get_active_camera_mode() == CAMERA_MODE_AUTO; }

// Returns the active rubberness [0..1] (only meaningful when mode is
// AUTO). Currently a constant; will become runtime-configurable in a
// future settings pass.
float get_camera_rubberness();

// Smoothing-alpha helper.
//
// All camera smoothings in this codebase use a "keep" fraction: the share
// of the previous value retained per tick, mixed with the freshly computed
// target. e.g. `new = old * keep + target * (1 - keep)`.
//
// `keep_for_rubberness(current_default_keep)` returns the keep value to
// use at the current rubberness setting, calibrated so that:
//   rubberness == 0.0  → returns 0.0                       (snap)
//   rubberness == 0.5  → returns current_default_keep      (shipping
//                                                          behaviour)
//   rubberness == 1.0  → returns close-to-1                (very lazy:
//                                                          remaining "slack
//                                                          to 1.0" reduced
//                                                          to 1/4 of what
//                                                          it is at 0.5).
// Piecewise-linear interpolation; inputs are clamped.
float keep_for_rubberness(float current_default_keep);

// Orbit-lag helper.
//
// AUTO-mode rotation blocks (mdx mouse, stick X gamepad) keep a small
// angular RESIDUAL on each input tick — the camera position partially
// catches up to want, leaving N game-angle units to be closed by the
// standard position+angle smoothing pass over subsequent ticks. The
// residual + smoothing produces the rubber-band tail.
//
// `orbit_lag_for_rubberness(default_lag_at_half)` returns the residual
// to use at the current rubberness setting:
//   rubberness == 0.0  → returns 0                    (no residual → snap)
//   rubberness == 0.5  → returns default_lag_at_half  (shipping behaviour)
//   rubberness == 1.0  → returns default × 4          (4× the residual,
//                                                       distinctly laggier)
// Piecewise-linear. MANUAL mode never calls this (it does full snap and
// bypasses the sync-with-residual path entirely).
float orbit_lag_for_rubberness(float default_lag_at_half);

#endif // CAMERA_CAMERA_MODE_H
