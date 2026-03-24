#ifndef BUILDINGS_BUILD_GLOBALS_H
#define BUILDINGS_BUILD_GLOBALS_H

// and everything else required by light.h and building.h.
#include "game/game_types.h"
#include "engine/graphics/lighting/light.h"

// Per-vertex lighting results for building primitives, indexed by prim point index.
// Filled by LIGHT_building_use_normals; declared as extern in light.h.
// uc_orig: LIGHT_building_point (fallen/DDEngine/Source/build.cpp)
extern LIGHT_Colour LIGHT_building_point[RMAX_PRIM_POINTS];

// Ambient light colour used when drawing buildings.
// Declared as extern in light.h; defined here.
// uc_orig: LIGHT_amb_colour (fallen/DDEngine/Source/build.cpp)
extern LIGHT_Colour LIGHT_amb_colour;

#endif // BUILDINGS_BUILD_GLOBALS_H
