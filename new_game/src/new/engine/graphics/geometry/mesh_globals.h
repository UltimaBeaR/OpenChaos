#ifndef ENGINE_GRAPHICS_GEOMETRY_MESH_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_MESH_GLOBALS_H

#include "core/types.h"

// uc_orig: MESH_Crumple (fallen/DDEngine/Source/mesh.cpp)
// Per-vertex random displacement used for generic crumple levels (damaged objects).
typedef struct
{
    float dx;
    float dy;
    float dz;

} MESH_Crumple;

// uc_orig: MESH_Crumple2 (fallen/DDEngine/Source/mesh.cpp)
// Compact per-vertex displacement for car zone crumple (6 body zones, 5 levels, 8 variations).
typedef struct
{
    SBYTE dx, dy, dz;
} MESH_Crumple2;

// uc_orig: MESH_NUM_CRUMPLES (fallen/DDEngine/Source/mesh.cpp)
#define MESH_NUM_CRUMPLES 5
// uc_orig: MESH_NUM_CRUMPVALS (fallen/DDEngine/Source/mesh.cpp)
#define MESH_NUM_CRUMPVALS 8

// uc_orig: MESH_crumple (fallen/DDEngine/Source/mesh.cpp)
// Random crumple offsets per damage level and variation, filled by MESH_init().
extern MESH_Crumple MESH_crumple[MESH_NUM_CRUMPLES][MESH_NUM_CRUMPVALS];

// uc_orig: car_crumples (fallen/DDEngine/Source/mesh.cpp)
// Per-zone damage level array set by MESH_set_crumple(), used during the next draw call with crumple=-1.
extern UBYTE* car_crumples;
// uc_orig: car_assign (fallen/DDEngine/Source/mesh.cpp)
// Per-vertex body zone index array set by MESH_set_crumple(), used during the next draw call with crumple=-1.
extern UBYTE* car_assign;

// uc_orig: MESH_car_crumples (fallen/DDEngine/Source/mesh.cpp)
// Pre-baked car body zone crumple offsets: [damage_level][variation][zone].
// Zone 0..5 = front/back/left/right/top/bottom body zones.
extern MESH_Crumple2 MESH_car_crumples[MESH_NUM_CRUMPLES][MESH_NUM_CRUMPVALS][6];

#endif // ENGINE_GRAPHICS_GEOMETRY_MESH_GLOBALS_H
