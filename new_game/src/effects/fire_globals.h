#ifndef EFFECTS_FIRE_GLOBALS_H
#define EFFECTS_FIRE_GLOBALS_H

#include "effects/fire.h"

// Global flame pool. Index 0 is never used (sentinel).
// uc_orig: FIRE_flame (fallen/Source/fire.cpp)
extern FIRE_Flame FIRE_flame[FIRE_MAX_FLAMES];
// uc_orig: FIRE_flame_free (fallen/Source/fire.cpp)
extern SLONG FIRE_flame_free;

// Active fire slots.
// uc_orig: FIRE_fire (fallen/Source/fire.cpp)
extern FIRE_Fire FIRE_fire[FIRE_MAX_FIRE];
// uc_orig: FIRE_fire_last (fallen/Source/fire.cpp)
extern SLONG FIRE_fire_last;

// Renderer iterator state (FIRE_get_start / FIRE_get_next).
// uc_orig: FIRE_get_z (fallen/Source/fire.cpp)
extern UBYTE FIRE_get_z;
// uc_orig: FIRE_get_xmin (fallen/Source/fire.cpp)
extern UBYTE FIRE_get_xmin;
// uc_orig: FIRE_get_xmax (fallen/Source/fire.cpp)
extern UBYTE FIRE_get_xmax;
// uc_orig: FIRE_get_fire_upto (fallen/Source/fire.cpp)
extern UBYTE FIRE_get_fire_upto;
// uc_orig: FIRE_get_flame (fallen/Source/fire.cpp)
extern UBYTE FIRE_get_flame;
// uc_orig: FIRE_get_info (fallen/Source/fire.cpp)
extern FIRE_Info FIRE_get_info;
// uc_orig: FIRE_get_point (fallen/Source/fire.cpp)
extern FIRE_Point FIRE_get_point[FIRE_MAX_POINTS];

#endif // EFFECTS_FIRE_GLOBALS_H
