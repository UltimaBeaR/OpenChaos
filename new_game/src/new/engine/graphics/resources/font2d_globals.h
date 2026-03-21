#ifndef ENGINE_GRAPHICS_RESOURCES_FONT2D_GLOBALS_H
#define ENGINE_GRAPHICS_RESOURCES_FONT2D_GLOBALS_H

#include "engine/graphics/resources/font2d.h"

// Per-character UV info for the full character set (alphabetic + punctuation + accented chars).
// uc_orig: FONT2D_letter (fallen/DDEngine/Source/font2d.cpp)
extern FONT2D_Letter FONT2D_letter[];

// Ordered list of punctuation and accented characters matching the atlas layout.
// uc_orig: FONT2D_punct (fallen/DDEngine/Source/font2d.cpp)
extern CBYTE FONT2D_punct[];

// Strikethrough offset state — persisted across calls when bUseLastOffset is true.
// uc_orig: DST_offset1 (fallen/DDEngine/Source/font2d.cpp)
extern SLONG DST_offset1;
// uc_orig: DST_offset2 (fallen/DDEngine/Source/font2d.cpp)
extern SLONG DST_offset2;

// x-coordinate of the rightmost character drawn by the last DrawString call.
// uc_orig: FONT2D_rightmost_x (fallen/DDEngine/Headers/font2d.h)
extern SLONG FONT2D_rightmost_x;
// x-coordinate of the leftmost character drawn during right-justify layout.
// uc_orig: FONT2D_leftmost_x (fallen/DDEngine/Headers/font2d.h)
extern SLONG FONT2D_leftmost_x;

// Temporary TGA pixel buffer used during FONT2D_init for atlas scanning (freed after init).
// Type is void* to avoid pulling assets/tga.h into the engine/graphics layer.
// Actual type is TGA_Pixel[256][256]; cast done in font2d.cpp.
// uc_orig: FONT2D_data (fallen/DDEngine/Source/font2d.cpp)
extern void* font2d_data;

#endif // ENGINE_GRAPHICS_RESOURCES_FONT2D_GLOBALS_H
