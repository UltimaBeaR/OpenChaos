#ifndef ENGINE_GRAPHICS_GEOMETRY_AA_H
#define ENGINE_GRAPHICS_GEOMETRY_AA_H

#include "core/types.h"

// Anti-aliased triangle rasterizer.
// Draws a coverage-weighted alpha mask into a bitmap buffer.
// Points are given in clockwise order, in 16-bit fixed-point coordinates.

// uc_orig: AA_draw (fallen/DDEngine/Source/aa.cpp)
void AA_draw(
    UBYTE* bitmap,
    UBYTE x_res,
    UBYTE y_res,
    SLONG pitch,
    SLONG p1x, SLONG p1y,
    SLONG p2x, SLONG p2y,
    SLONG p3x, SLONG p3y);

#endif // ENGINE_GRAPHICS_GEOMETRY_AA_H
