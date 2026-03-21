#ifndef ENGINE_GRAPHICS_PIPELINE_POLY_RENDER_GLOBALS_H
#define ENGINE_GRAPHICS_PIPELINE_POLY_RENDER_GLOBALS_H

#include "engine/graphics/pipeline/poly.h"

// uc_orig: PageOrder (fallen/DDEngine/Source/polyrenderstate.cpp)
// Sorted order of page indices for rendering. Built by POLY_init_render_states().
extern SLONG PageOrder[POLY_NUM_PAGES];

// uc_orig: PageOrdered (fallen/DDEngine/Source/polyrenderstate.cpp)
// Tracks which pages have been placed into PageOrder during sort.
extern bool PageOrdered[POLY_NUM_PAGES];

#endif // ENGINE_GRAPHICS_PIPELINE_POLY_RENDER_GLOBALS_H
