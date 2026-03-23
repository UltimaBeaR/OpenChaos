#ifndef WORLD_MAP_PAP_GLOBALS_H
#define WORLD_MAP_PAP_GLOBALS_H

// Global map data arrays: the two-tier heightfield that describes the game world.

#include "world/map/pap.h"

// uc_orig: PAP_lo (fallen/Source/pap.cpp)
// Lo-res map: 32x32 grid, each cell covers 1024 world units.
extern MEM_PAP_Lo* PAP_lo;

// uc_orig: PAP_hi (fallen/Source/pap.cpp)
// Hi-res map: 128x128 grid, each cell covers 256 world units.
extern MEM_PAP_Hi* PAP_hi;

#endif // WORLD_MAP_PAP_GLOBALS_H
