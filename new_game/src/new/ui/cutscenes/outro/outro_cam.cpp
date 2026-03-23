#include "ui/cutscenes/outro/outro_cam.h"
#include "ui/cutscenes/outro/outro_cam_globals.h"
#include "fallen/outro/os.h"     // Temporary: OS_* render functions
#include "fallen/outro/key.h"    // Temporary: KEY_on[], KEY_shift, KEY_* scancodes
#include "fallen/outro/Matrix.h" // Temporary: MATRIX_calc, MATRIX_MUL

#define CAM_TYPE_LOCKED 0
#define CAM_TYPE_FREE 1
#define CAM_TYPE_NUMBER 2

// uc_orig: CAM_init (fallen/outro/cam.cpp)
void CAM_init(void)
{
    CAM_type = CAM_TYPE_LOCKED;
    CAM_yaw = 3.60F;
    CAM_pitch = -0.58F;
    CAM_dist = 56.00F;
    CAM_lens = 2.00F;
    CAM_focus_y = 20.00F;

    CAM_process();
}

// uc_orig: CAM_process (fallen/outro/cam.cpp)
void CAM_process(void)
{
    // TAB toggles between locked-to-focus and free-fly camera modes.
    if (KEY_on[KEY_TAB]) {
        KEY_on[KEY_TAB] = 0;

        CAM_type += 1;

        if (CAM_type >= CAM_TYPE_NUMBER) {
            CAM_type = 0;
        }
    }

#define CAM_SPEED_MOVE (2.000F)
#define CAM_SPEED_TURN (0.060F)

    switch (CAM_type) {
    case CAM_TYPE_LOCKED:

        if (KEY_on[KEY_HOME]) {
            CAM_dist -= CAM_SPEED_MOVE;
        }
        if (KEY_on[KEY_END]) {
            CAM_dist += CAM_SPEED_MOVE;
        }

        if (KEY_on[KEY_UP]) {
            CAM_pitch += CAM_SPEED_TURN;
        }
        if (KEY_on[KEY_DOWN]) {
            CAM_pitch -= CAM_SPEED_TURN;
        }

        if (KEY_on[KEY_LEFT]) {
            CAM_yaw -= CAM_SPEED_TURN;
        }
        if (KEY_on[KEY_RIGHT]) {
            CAM_yaw += CAM_SPEED_TURN;
        }

        MATRIX_calc(
            CAM_matrix,
            CAM_yaw,
            CAM_pitch,
            0.0F);

        CAM_x = CAM_focus_x - CAM_matrix[6] * CAM_dist;
        CAM_y = CAM_focus_y - CAM_matrix[7] * CAM_dist;
        CAM_z = CAM_focus_z - CAM_matrix[8] * CAM_dist;

        break;

    case CAM_TYPE_FREE:

        MATRIX_calc(
            CAM_matrix,
            CAM_yaw,
            CAM_pitch,
            0.0F);

        if (KEY_on[KEY_UP]) {
            if (KEY_shift) {
                CAM_x += CAM_matrix[3] * CAM_SPEED_MOVE;
                CAM_y += CAM_matrix[4] * CAM_SPEED_MOVE;
                CAM_z += CAM_matrix[5] * CAM_SPEED_MOVE;
            } else {
                CAM_x += CAM_matrix[6] * CAM_SPEED_MOVE;
                CAM_y += CAM_matrix[7] * CAM_SPEED_MOVE;
                CAM_z += CAM_matrix[8] * CAM_SPEED_MOVE;
            }
        }

        if (KEY_on[KEY_DOWN]) {
            if (KEY_shift) {
                CAM_x -= CAM_matrix[3] * CAM_SPEED_MOVE;
                CAM_y -= CAM_matrix[4] * CAM_SPEED_MOVE;
                CAM_z -= CAM_matrix[5] * CAM_SPEED_MOVE;
            } else {
                CAM_x -= CAM_matrix[6] * CAM_SPEED_MOVE;
                CAM_y -= CAM_matrix[7] * CAM_SPEED_MOVE;
                CAM_z -= CAM_matrix[8] * CAM_SPEED_MOVE;
            }
        }

        if (KEY_on[KEY_LEFT]) {
            CAM_x -= CAM_matrix[0] * CAM_SPEED_MOVE;
            CAM_y -= CAM_matrix[1] * CAM_SPEED_MOVE;
            CAM_z -= CAM_matrix[2] * CAM_SPEED_MOVE;
        }

        if (KEY_on[KEY_RIGHT]) {
            CAM_x += CAM_matrix[0] * CAM_SPEED_MOVE;
            CAM_y += CAM_matrix[1] * CAM_SPEED_MOVE;
            CAM_z += CAM_matrix[2] * CAM_SPEED_MOVE;
        }

        if (KEY_on[KEY_HOME]) {
            CAM_pitch -= CAM_SPEED_TURN;
        }
        if (KEY_on[KEY_END]) {
            CAM_pitch += CAM_SPEED_TURN;
        }

        if (KEY_on[KEY_DELETE]) {
            CAM_yaw -= CAM_SPEED_TURN;
        }
        if (KEY_on[KEY_PAGEDOWN]) {
            CAM_yaw += CAM_SPEED_TURN;
        }

        MATRIX_calc(
            CAM_matrix,
            CAM_yaw,
            CAM_pitch,
            0.0F);

        break;

    default:
        ASSERT(0);
        break;
    }
}
