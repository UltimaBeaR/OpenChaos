#ifndef ENGINE_GRAPHICS_GEOMETRY_OVAL_H
#define ENGINE_GRAPHICS_GEOMETRY_OVAL_H

#include "engine/core/types.h"

// Oval type constants — passed to OVAL_add to select the shadow texture page.
// uc_orig: OVAL_TYPE_OVAL (fallen/DDEngine/Headers/oval.h)
#define OVAL_TYPE_OVAL 0
// uc_orig: OVAL_TYPE_SQUARE (fallen/DDEngine/Headers/oval.h)
#define OVAL_TYPE_SQUARE 1

// Draws a soft oval (or square) shadow under an object projected onto the heightfield.
// x/y/z: center in world coords (8 bits per mapsquare).
// size:     half-size of the oval before elongation.
// elongate: stretch factor along the angle direction.
// angle:    rotation angle in radians.
// type:     OVAL_TYPE_OVAL or OVAL_TYPE_SQUARE.
// uc_orig: OVAL_add (fallen/DDEngine/Headers/oval.h)
void OVAL_add(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG size,
    float elongate = 1.0F,
    float angle = 0.0F,
    SLONG type = OVAL_TYPE_OVAL);

#endif // ENGINE_GRAPHICS_GEOMETRY_OVAL_H
