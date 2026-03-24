#ifndef UI_CAMERA_CAM_H
#define UI_CAMERA_CAM_H

#include "core/types.h"
#include "actors/core/thing.h"
#include "ui/camera/cam_globals.h"

// High-level camera API for game code. Sets camera mode, focus, zoom, shake,
// and reads back camera position/orientation. All operations affect the active
// camera used by the rendering engine.

// ---- Camera modes ----

// uc_orig: CAM_MODE_CRAPPY (fallen/Headers/cam.h)
#define CAM_MODE_CRAPPY       0
// uc_orig: CAM_MODE_NORMAL (fallen/Headers/cam.h)
#define CAM_MODE_NORMAL       1
// uc_orig: CAM_MODE_STATIONARY (fallen/Headers/cam.h)
#define CAM_MODE_STATIONARY   2
// uc_orig: CAM_MODE_BEHIND (fallen/Headers/cam.h)
#define CAM_MODE_BEHIND       3
// uc_orig: CAM_MODE_FIRST_PERSON (fallen/Headers/cam.h)
#define CAM_MODE_FIRST_PERSON 4
// uc_orig: CAM_MODE_THIRD_PERSON (fallen/Headers/cam.h)
#define CAM_MODE_THIRD_PERSON 5
// uc_orig: CAM_MODE_SHOOT_NORMAL (fallen/Headers/cam.h)
#define CAM_MODE_SHOOT_NORMAL 6
// uc_orig: CAM_MODE_FIGHT_NORMAL (fallen/Headers/cam.h)
#define CAM_MODE_FIGHT_NORMAL 7

// ---- Lens types ----

// uc_orig: CAM_LENS_NORMAL (fallen/Headers/cam.h)
#define CAM_LENS_NORMAL    0
// uc_orig: CAM_LENS_WIDEANGLE (fallen/Headers/cam.h)
#define CAM_LENS_WIDEANGLE 1
// uc_orig: CAM_LENS_ZOOM (fallen/Headers/cam.h)
#define CAM_LENS_ZOOM      2

// ---- Predefined camera types ----
// Convenience wrappers that set mode, focus, and zoom together.

// uc_orig: CAM_TYPE_STANDARD (fallen/Headers/cam.h)
#define CAM_TYPE_STANDARD 1
// uc_orig: CAM_TYPE_LOWER (fallen/Headers/cam.h)
#define CAM_TYPE_LOWER    2
// uc_orig: CAM_TYPE_BEHIND (fallen/Headers/cam.h)
#define CAM_TYPE_BEHIND   3
// uc_orig: CAM_TYPE_WIDE (fallen/Headers/cam.h)
#define CAM_TYPE_WIDE     4
// uc_orig: CAM_TYPE_ZOOM (fallen/Headers/cam.h)
#define CAM_TYPE_ZOOM     5

// Sets camera mode, focus, and zoom to a preconfigured type.
// uc_orig: CAM_set_type (fallen/Headers/cam.h)
void CAM_set_type(SLONG type);

// ---- Camera setters ----

// uc_orig: CAM_set_mode (fallen/Headers/cam.h)
void CAM_set_mode(SLONG mode);

// uc_orig: CAM_set_focus (fallen/Headers/cam.h)
void CAM_set_focus(Thing* focus);

// uc_orig: CAM_set_zoom (fallen/Headers/cam.h)
void CAM_set_zoom(SLONG zoom);

// uc_orig: CAM_set_collision (fallen/Headers/cam.h)
void CAM_set_collision(SLONG boolean);

// Applies a camera shake of the given magnitude (0=none, 255=max).
// uc_orig: CAM_set_shake (fallen/Headers/cam.h)
void CAM_set_shake(UBYTE amount);

// Sets first-person delta angles (yaw/pitch offset from the look direction).
// uc_orig: CAM_set_dangle (fallen/Headers/cam.h)
void CAM_set_dangle(SLONG dyaw, SLONG dpitch);

// Positions the camera for watching the player climb out of the sewers.
// uc_orig: CAM_set_to_leave_sewers_position (fallen/Headers/cam.h)
void CAM_set_to_leave_sewers_position(Thing*);

// ---- Camera getters ----

// uc_orig: CAM_get_mode (fallen/Headers/cam.h)
SLONG CAM_get_mode(void);

// Returns current camera yaw in radians.
// uc_orig: CAM_get_ryaw (fallen/Headers/cam.h)
float CAM_get_ryaw(void);

// Returns all camera position and orientation values.
// uc_orig: CAM_get (fallen/Headers/cam.h)
void CAM_get(
    SLONG* world_x, SLONG* world_y, SLONG* world_z,
    SLONG* yaw, SLONG* pitch, SLONG* roll,
    float* radians_yaw, float* radians_pitch, float* radians_roll);

// uc_orig: CAM_get_pos (fallen/Headers/cam.h)
void CAM_get_pos(SLONG* world_x, SLONG* world_y, SLONG* world_z);

// ---- Camera update ----

// Runs the camera simulation for one game tick.
// uc_orig: CAM_process (fallen/Headers/cam.h)
void CAM_process(void);

#endif // UI_CAMERA_CAM_H
