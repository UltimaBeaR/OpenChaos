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

// Draws a 2D triangle with vertices (x,y), (ox,oy), (tx,ty).
// flag selects blending mode same as DRAW2D_Box.
// uc_orig: DRAW2D_Tri (fallen/DDEngine/Source/drawxtra.cpp)
void DRAW2D_Tri(SLONG x, SLONG y, SLONG ox, SLONG oy, SLONG tx, SLONG ty, SLONG rgb, UBYTE flag);

#endif // ENGINE_GRAPHICS_PIPELINE_DRAW2D_H
