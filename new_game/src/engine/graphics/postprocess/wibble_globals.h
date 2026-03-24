#ifndef ENGINE_GRAPHICS_POSTPROCESS_WIBBLE_GLOBALS_H
#define ENGINE_GRAPHICS_POSTPROCESS_WIBBLE_GLOBALS_H

#include "engine/core/types.h"

// Horizontal warp multipliers and gamma multipliers for the two wibble waves.
// uc_orig: mul_y1 (fallen/DDEngine/Source/wibble.cpp)
extern SLONG wibble_mul_y1;
// uc_orig: mul_y2 (fallen/DDEngine/Source/wibble.cpp)
extern SLONG wibble_mul_y2;
// uc_orig: mul_g1 (fallen/DDEngine/Source/wibble.cpp)
extern SLONG wibble_mul_g1;
// uc_orig: mul_g2 (fallen/DDEngine/Source/wibble.cpp)
extern SLONG wibble_mul_g2;

// Per-scanline shift amounts for the two wibble waves.
// uc_orig: shift1 (fallen/DDEngine/Source/wibble.cpp)
extern SLONG wibble_shift1;
// uc_orig: shift2 (fallen/DDEngine/Source/wibble.cpp)
extern SLONG wibble_shift2;

#endif // ENGINE_GRAPHICS_POSTPROCESS_WIBBLE_GLOBALS_H
