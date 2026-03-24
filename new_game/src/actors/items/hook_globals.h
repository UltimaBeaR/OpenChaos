#ifndef ACTORS_ITEMS_HOOK_GLOBALS_H
#define ACTORS_ITEMS_HOOK_GLOBALS_H

#include "engine/core/types.h"

// Internal struct for individual string/rope segment points.
// uc_orig: HOOK_Point (fallen/Source/hook.cpp)
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
    SWORD dx;
    SWORD dy;
    SWORD dz;
    UWORD alive;
} HOOK_Point;

// uc_orig: HOOK_point (fallen/Source/hook.cpp)
extern HOOK_Point HOOK_point[256]; // indexed 0..HOOK_NUM_POINTS-1

// How many points have been unreeled from the coil.
// uc_orig: HOOK_reeled (fallen/Source/hook.cpp)
extern UBYTE HOOK_reeled;

// Current hook state (STILL/SPINNING/FLYING).
// uc_orig: HOOK_state (fallen/Source/hook.cpp)
extern UBYTE HOOK_state;

// uc_orig: HOOK_grapple_yaw (fallen/Source/hook.cpp)
extern SLONG HOOK_grapple_yaw;
// uc_orig: HOOK_grapple_pitch (fallen/Source/hook.cpp)
extern SLONG HOOK_grapple_pitch;

// uc_orig: HOOK_spin_x (fallen/Source/hook.cpp)
extern SLONG HOOK_spin_x;
// uc_orig: HOOK_spin_y (fallen/Source/hook.cpp)
extern SLONG HOOK_spin_y;
// uc_orig: HOOK_spin_z (fallen/Source/hook.cpp)
extern SLONG HOOK_spin_z;
// uc_orig: HOOK_spin_speed (fallen/Source/hook.cpp)
extern SLONG HOOK_spin_speed;

// Countdown ticks for settling after the hook comes to rest.
// uc_orig: HOOK_countdown (fallen/Source/hook.cpp)
extern SLONG HOOK_countdown;

#endif // ACTORS_ITEMS_HOOK_GLOBALS_H
