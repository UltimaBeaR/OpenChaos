#ifndef ENGINE_GRAPHICS_GEOMETRY_CONE_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_CONE_GLOBALS_H

#include "core/types.h"
#include "engine/graphics/pipeline/poly.h"

// File-private struct for cone base points; exposed here for _globals extern arrays.
// uc_orig: CONE_Point (fallen/DDEngine/Source/cone.cpp)
typedef struct
{
    float x;
    float y;
    float z;
    ULONG colour;
    POLY_Point pp;

} CONE_Point;

// uc_orig: CONE_MAX_POINTS (fallen/DDEngine/Source/cone.cpp)
#define CONE_MAX_POINTS 64

// uc_orig: CONE_origin_x (fallen/DDEngine/Source/cone.cpp)
extern float CONE_origin_x;
// uc_orig: CONE_origin_y (fallen/DDEngine/Source/cone.cpp)
extern float CONE_origin_y;
// uc_orig: CONE_origin_z (fallen/DDEngine/Source/cone.cpp)
extern float CONE_origin_z;
// uc_orig: CONE_origin_colour (fallen/DDEngine/Source/cone.cpp)
extern ULONG CONE_origin_colour;
// uc_orig: CONE_end_x (fallen/DDEngine/Source/cone.cpp)
extern float CONE_end_x;
// uc_orig: CONE_end_y (fallen/DDEngine/Source/cone.cpp)
extern float CONE_end_y;
// uc_orig: CONE_end_z (fallen/DDEngine/Source/cone.cpp)
extern float CONE_end_z;

// uc_orig: CONE_point (fallen/DDEngine/Source/cone.cpp)
extern CONE_Point CONE_point[CONE_MAX_POINTS];
// uc_orig: CONE_point_upto (fallen/DDEngine/Source/cone.cpp)
extern SLONG CONE_point_upto;

// uc_orig: CONE_COLVECT_DONE (fallen/DDEngine/Source/cone.cpp)
#define CONE_COLVECT_DONE 4

// uc_orig: CONE_colvect_done (fallen/DDEngine/Source/cone.cpp)
extern SLONG CONE_colvect_done[CONE_COLVECT_DONE];
// uc_orig: CONE_colvect_done_upto (fallen/DDEngine/Source/cone.cpp)
extern SLONG CONE_colvect_done_upto;

#endif // ENGINE_GRAPHICS_GEOMETRY_CONE_GLOBALS_H
