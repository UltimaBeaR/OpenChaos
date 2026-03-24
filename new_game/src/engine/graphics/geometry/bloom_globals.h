#ifndef ENGINE_GRAPHICS_GEOMETRY_BLOOM_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_BLOOM_GLOBALS_H

#include "engine/core/types.h"

// Colour palette for lens flare quads, indexed by (i >> 2) + 3 where i in -12..14.
// 7 RGB entries, each an array of 3 bytes (R, G, B).
// uc_orig: flare_table (fallen/DDEngine/Source/drawxtra.cpp)
extern const UBYTE flare_table[7][3];

#endif // ENGINE_GRAPHICS_GEOMETRY_BLOOM_GLOBALS_H
