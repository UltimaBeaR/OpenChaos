#ifndef ENGINE_GRAPHICS_POSTPROCESS_WIBBLE_H
#define ENGINE_GRAPHICS_POSTPROCESS_WIBBLE_H

#include "engine/core/types.h"

// Horizontally distorts a rectangular region of the locked back buffer in-place,
// creating a wavy "wibble" effect. Operates on raw framebuffer memory.
//
// ALL THESE FUNCTIONS MUST ONLY BE CALLED WHEN THE SCREEN IS LOCKED.
// Make sure the bounding boxes are all well on screen.
//
// uc_orig: WIBBLE_simple (fallen/DDEngine/Headers/wibble.h)
void WIBBLE_simple(
    SLONG x1, SLONG y1,
    SLONG x2, SLONG y2,
    UBYTE wibble_y1,
    UBYTE wibble_y2,
    UBYTE wibble_g1,
    UBYTE wibble_g2,
    UBYTE wibble_s1,
    UBYTE wibble_s2);

#endif // ENGINE_GRAPHICS_POSTPROCESS_WIBBLE_H
