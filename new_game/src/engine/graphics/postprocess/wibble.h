#ifndef ENGINE_GRAPHICS_POSTPROCESS_WIBBLE_H
#define ENGINE_GRAPHICS_POSTPROCESS_WIBBLE_H

#include "engine/core/types.h"

// Horizontal per-row shift of a rectangular backbuffer region, creating the
// wavy "wibble" look used for puddle reflections. Coordinates are in game-
// virtual (DisplayWidth × DisplayHeight) pixels; scaling to real framebuffer
// pixels and the actual GPU draw happen in the backend (ge_apply_wibble).
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
