#ifndef ENGINE_GRAPHICS_PIPELINE_DRAW2D_H
#define ENGINE_GRAPHICS_PIPELINE_DRAW2D_H

#include "engine/core/types.h"

// 2D screen-space drawing utilities: filled boxes, triangles, and textured quads.
// Used by the UI and widget system for flat overlays drawn on top of the 3D scene.
// Coordinates are in screen pixels.

// Draws a filled 2D box from (x,y) to (ox,oy) using an additive or subtractive alpha page.
// flag != 0 uses subtractive blending, flag == 0 uses additive blending.
// depth controls z-buffer placement (0..255, mapped to 0.0..1.0).
// uc_orig: DRAW2D_Box (fallen/DDEngine/Source/drawxtra.cpp)
void DRAW2D_Box(SLONG x, SLONG y, SLONG ox, SLONG oy, SLONG rgb, UBYTE flag, UBYTE depth = 128);

// Like DRAW2D_Box but uses an explicit texture page instead of choosing by flag.
// uc_orig: DRAW2D_Box_Page (fallen/DDEngine/Source/drawxtra.cpp)
void DRAW2D_Box_Page(SLONG x, SLONG y, SLONG ox, SLONG oy, SLONG rgb, SLONG page, UBYTE depth = 128);

// Draws a 2D triangle with vertices (x,y), (ox,oy), (tx,ty).
// flag selects blending mode same as DRAW2D_Box.
// uc_orig: DRAW2D_Tri (fallen/DDEngine/Source/drawxtra.cpp)
void DRAW2D_Tri(SLONG x, SLONG y, SLONG ox, SLONG oy, SLONG tx, SLONG ty, SLONG rgb, UBYTE flag);

// Draws a textured 2D quad from (x,y) to (ox,oy) with UV coords (u,v)-(ou,ov).
// uc_orig: DRAW2D_Sprite (fallen/DDEngine/Source/drawxtra.cpp)
void DRAW2D_Sprite(SLONG x, SLONG y, SLONG ox, SLONG oy, float u, float v, float ou, float ov, SLONG page, SLONG rgb);

#endif // ENGINE_GRAPHICS_PIPELINE_DRAW2D_H
