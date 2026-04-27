#ifndef BUILDINGS_BUILDING_GLOBALS_H
#define BUILDINGS_BUILDING_GLOBALS_H

#include "engine/core/types.h"
#include "buildings/building_types.h" // TXTY, DXTXTY

// uc_orig: next_roof_bound (fallen/Source/Building.cpp)
extern UWORD next_roof_bound;

// uc_orig: textures_xy (fallen/Source/Building.cpp)
extern struct TXTY textures_xy[200][5];

// uc_orig: dx_textures_xy (fallen/Source/Building.cpp)
extern struct DXTXTY dx_textures_xy[200][5];

// uc_orig: textures_flags (fallen/Source/Building.cpp)
extern UBYTE textures_flags[200][5];

#endif // BUILDINGS_BUILDING_GLOBALS_H
