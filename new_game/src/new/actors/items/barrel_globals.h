#ifndef ACTORS_ITEMS_BARREL_GLOBALS_H
#define ACTORS_ITEMS_BARREL_GLOBALS_H

#include "actors/items/barrel.h"

// uc_orig: BARREL_sphere (fallen/Source/barrel.cpp)
extern BARREL_Sphere* BARREL_sphere;

// uc_orig: BARREL_sphere_last (fallen/Source/barrel.cpp)
extern SLONG BARREL_sphere_last;

// uc_orig: BARREL_barrel (fallen/Source/barrel.cpp)
extern Barrel* BARREL_barrel;

// uc_orig: BARREL_barrel_upto (fallen/Source/barrel.cpp)
extern SLONG BARREL_barrel_upto;

// uc_orig: BARREL_fx_rate (fallen/Source/barrel.cpp)
extern SLONG BARREL_fx_rate;

// uc_orig: found_barrel (fallen/Source/barrel.cpp)
// Scratch buffer for THING_find_sphere results inside barrel processing.
#define BARREL_MAX_FIND 16
extern THING_INDEX found_barrel[BARREL_MAX_FIND];

#endif // ACTORS_ITEMS_BARREL_GLOBALS_H
