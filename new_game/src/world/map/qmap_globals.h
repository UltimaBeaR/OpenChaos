#ifndef WORLD_MAP_QMAP_GLOBALS_H
#define WORLD_MAP_QMAP_GLOBALS_H

#include "world/map/qmap.h"

// All global arrays and counters for the QMAP world data structures.
// Definitions are in qmap_globals.cpp; declarations are repeated here
// as externs for use across translation units.

// uc_orig: QMAP_texture (fallen/Source/qmap.cpp)
extern UWORD QMAP_texture[QMAP_MAX_TEXTURES];
// uc_orig: QMAP_texture_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_texture_upto;

// uc_orig: QMAP_style (fallen/Source/qmap.cpp)
extern QMAP_Style QMAP_style[QMAP_MAX_STYLES];
// uc_orig: QMAP_style_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_style_upto;

// uc_orig: QMAP_road (fallen/Source/qmap.cpp)
extern QMAP_Road QMAP_road[QMAP_MAX_ROADS];
// uc_orig: QMAP_road_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_road_upto;

// uc_orig: QMAP_cube (fallen/Source/qmap.cpp)
extern QMAP_Cube QMAP_cube[QMAP_MAX_CUBES];
// uc_orig: QMAP_cube_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_cube_upto;

// uc_orig: QMAP_gtex (fallen/Source/qmap.cpp)
extern QMAP_Gtex QMAP_gtex[QMAP_MAX_GTEXES];
// uc_orig: QMAP_gtex_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_gtex_upto;

// uc_orig: QMAP_cable (fallen/Source/qmap.cpp)
extern QMAP_Cable QMAP_cable[QMAP_MAX_CABLES];
// uc_orig: QMAP_cable_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_cable_upto;

// uc_orig: QMAP_height (fallen/Source/qmap.cpp)
extern SBYTE QMAP_height[QMAP_MAX_HEIGHTS];
// uc_orig: QMAP_height_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_height_upto;

// uc_orig: QMAP_hmap (fallen/Source/qmap.cpp)
extern QMAP_Hmap QMAP_hmap[QMAP_MAX_HMAPS];
// uc_orig: QMAP_hmap_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_hmap_upto;

// uc_orig: QMAP_fence (fallen/Source/qmap.cpp)
extern QMAP_Fence QMAP_fence[QMAP_MAX_FENCES];
// uc_orig: QMAP_fence_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_fence_upto;

// uc_orig: QMAP_light (fallen/Source/qmap.cpp)
extern QMAP_Light QMAP_light[QMAP_MAX_LIGHTS];
// uc_orig: QMAP_light_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_light_upto;

// uc_orig: QMAP_prim (fallen/Source/qmap.cpp)
extern QMAP_Prim QMAP_prim[QMAP_MAX_PRIMS];
// uc_orig: QMAP_prim_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_prim_upto;

// uc_orig: QMAP_all (fallen/Source/qmap.cpp)
extern UWORD QMAP_all[QMAP_MAX_ALL];
// uc_orig: QMAP_all_upto (fallen/Source/qmap.cpp)
extern SLONG QMAP_all_upto;

// uc_orig: QMAP_map (fallen/Source/qmap.cpp)
extern QMAP_Map QMAP_map[QMAP_MAPSIZE][QMAP_MAPSIZE];

// uc_orig: QMAP_point (fallen/Source/qmap.cpp)
extern QMAP_Point QMAP_point[QMAP_MAX_POINTS];
// uc_orig: QMAP_point_free (fallen/Source/qmap.cpp)
extern UWORD QMAP_point_free;

// uc_orig: QMAP_face (fallen/Source/qmap.cpp)
extern QMAP_Face QMAP_face[QMAP_MAX_FACES];
// uc_orig: QMAP_face_free (fallen/Source/qmap.cpp)
extern UWORD QMAP_face_free;

#endif // WORLD_MAP_QMAP_GLOBALS_H
