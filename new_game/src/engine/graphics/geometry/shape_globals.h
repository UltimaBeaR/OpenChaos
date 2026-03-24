#ifndef ENGINE_GRAPHICS_GEOMETRY_SHAPE_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_SHAPE_GLOBALS_H

#include "engine/core/types.h"

// Global tint applied to balloon prim vertex colours during MESH_draw_poly.
// Set by SHAPE_draw_balloon() before each call to MESH_draw_poly(PRIM_OBJ_BALLOON,...).
// Read by mesh.cpp via extern.
// uc_orig: SHAPE_balloon_colour (fallen/DDEngine/Source/shape.cpp)
extern ULONG SHAPE_balloon_colour;

#endif // ENGINE_GRAPHICS_GEOMETRY_SHAPE_GLOBALS_H
