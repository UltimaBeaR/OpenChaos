#ifndef THINGS_ITEMS_GRENADE_GLOBALS_H
#define THINGS_ITEMS_GRENADE_GLOBALS_H

#include "engine/core/types.h"

struct Thing;

// uc_orig: Grenade (fallen/Source/grenade.cpp)
// File-private struct in original; exposed here only for _globals linkage.
struct Grenade {
    Thing* owner;   // Person who threw the grenade.
    SLONG x;
    SLONG y;
    SLONG z;
    SWORD yaw;
    SWORD pitch;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SWORD dyaw;
    SWORD dpitch;
    UWORD timer;    // Countdown to explosion.
    UWORD rsvd;
};

// uc_orig: MAX_GRENADES (fallen/Source/grenade.cpp)
#define MAX_GRENADES 6

// uc_orig: GrenadeArray (fallen/Source/grenade.cpp)
// Pool of active grenades. Was static in original.
extern Grenade GrenadeArray[MAX_GRENADES];

// uc_orig: global_g (fallen/Source/grenade.cpp)
// Pointer to the last created grenade; used by show_grenade_path to simulate trajectory.
extern Grenade* global_g;

#endif // THINGS_ITEMS_GRENADE_GLOBALS_H
