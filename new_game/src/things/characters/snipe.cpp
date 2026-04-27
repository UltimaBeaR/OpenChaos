#include "engine/core/macros.h"
#include "things/characters/snipe.h"
#include "things/characters/snipe_globals.h"

// uc_orig: SNIPE_LENS_END (fallen/Source/snipe.cpp) — was #define inside SNIPE_mode_on, moved to file scope since SNIPE_process uses it
#define SNIPE_LENS_END (30 << 16)

// uc_orig: SNIPE_process (fallen/Source/snipe.cpp)
void SNIPE_process()
{
#define SNIPE_PITCH_MAX (+300 << 16)
#define SNIPE_PITCH_MIN (-300 << 16)

    if (SNIPE_pitch > SNIPE_PITCH_MAX) {
        SNIPE_dpitch -= SNIPE_pitch - SNIPE_PITCH_MAX >> 4;
    }
    if (SNIPE_pitch < SNIPE_PITCH_MIN) {
        SNIPE_dpitch -= SNIPE_pitch - SNIPE_PITCH_MIN >> 4;
    }

    SNIPE_pitch += SNIPE_dpitch;
    SNIPE_yaw += SNIPE_dyaw;

    SNIPE_dyaw -= SNIPE_dyaw >> 3;
    SNIPE_dpitch -= SNIPE_dpitch >> 3;

    if (SNIPE_cam_lens < SNIPE_LENS_END) {
        SNIPE_dlens += 0x100;
    }

    if (SNIPE_dlens > 0x1000) {
        SNIPE_dlens = 0x1000;
    }

    SNIPE_cam_lens += SNIPE_dlens;
    SNIPE_dlens -= SNIPE_dlens >> 3;

    if (WITHIN(SNIPE_dpitch, -20, +20)) {
        SNIPE_dpitch = 0;
    }
    if (WITHIN(SNIPE_dyaw, -20, +20)) {
        SNIPE_dyaw = 0;
    }
    if (WITHIN(SNIPE_dlens, -20, +20)) {
        SNIPE_dlens = 0;
    }

    SNIPE_cam_yaw = SNIPE_yaw & ((2048 << 16) - 1);
    SNIPE_cam_pitch = SNIPE_pitch & ((2048 << 16) - 1);
}
