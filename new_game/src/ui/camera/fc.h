#ifndef UI_CAMERA_FC_H
#define UI_CAMERA_FC_H

// Game camera system ("final camera"). UC-specific: follows Darci, cops, vehicles.
// FC_MAX_CAMS=2 for split-screen support; FC_cam[1] is unused in the shipped game.
// CAM_MORE_IN=0.75F: PC camera is 25% closer to the player than the PSX version.

#include "engine/platform/uc_common.h"
#include "things/core/thing.h"
#include "missions/game_types.h"

// uc_orig: FC_Cam (fallen/Headers/fc.h)
typedef struct {
    // What the camera is looking at.
    Thing* focus;       // NULL => camera is inactive
    SLONG focus_x;
    SLONG focus_y;
    SLONG focus_z;
    SLONG focus_yaw;
    UBYTE focus_in_warehouse;

    SLONG x;
    SLONG y;
    SLONG z;
    SLONG want_x;
    SLONG want_y;
    SLONG want_z;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG yaw;
    SLONG pitch;
    SLONG roll;
    SLONG want_yaw;
    SLONG want_pitch;
    SLONG want_roll;
    SLONG lens;
    SLONG toonear;
    SLONG rotate;
    SLONG nobehind;
    SLONG lookabove;
    UBYTE shake;
    SLONG cam_dist;
    SLONG cam_height;
    SLONG toonear_dist;
    SLONG toonear_x;
    SLONG toonear_y;
    SLONG toonear_z;
    SLONG toonear_yaw;
    SLONG toonear_pitch;
    SLONG toonear_roll;
    SLONG toonear_focus_yaw;
    SLONG smooth_transition;
} FC_Cam;

// uc_orig: FC_MAX_CAMS (fallen/Headers/fc.h)
#define FC_MAX_CAMS 2

// uc_orig: FC_init (fallen/Headers/fc.h)
void FC_init(void);

// uc_orig: FC_look_at (fallen/Headers/fc.h)
void FC_look_at(SLONG cam, UWORD thing_index);

// uc_orig: FC_move_to (fallen/Headers/fc.h)
void FC_move_to(SLONG cam, SLONG world_x, SLONG world_y, SLONG world_z);

// uc_orig: FC_change_camera_type (fallen/Headers/fc.h)
void FC_change_camera_type(SLONG cam, SLONG cam_type);

// uc_orig: FC_rotate_left (fallen/Headers/fc.h)
void FC_rotate_left(SLONG cam);

// uc_orig: FC_rotate_right (fallen/Headers/fc.h)
void FC_rotate_right(SLONG cam);

// uc_orig: FC_kill_player_cam (fallen/Headers/fc.h)
void FC_kill_player_cam(Thing* p_thing);

// uc_orig: FC_process (fallen/Headers/fc.h)
void FC_process(void);

// uc_orig: FC_can_see_person (fallen/Headers/fc.h)
SLONG FC_can_see_person(SLONG cam, Thing* p_person);

// uc_orig: FC_position_for_lookaround (fallen/Headers/fc.h)
void FC_position_for_lookaround(SLONG cam, SLONG pitch);

// uc_orig: FC_force_camera_behind (fallen/Headers/fc.h)
void FC_force_camera_behind(SLONG cam);

// uc_orig: FC_setup_initial_camera (fallen/Headers/fc.h)
void FC_setup_initial_camera(SLONG cam);

// uc_orig: FC_explosion (fallen/Headers/fc.h)
void FC_explosion(SLONG x, SLONG y, SLONG z, SLONG force);

#endif // UI_CAMERA_FC_H
