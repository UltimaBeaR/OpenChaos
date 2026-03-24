#ifndef ENGINE_GRAPHICS_GEOMETRY_AA_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_AA_GLOBALS_H

#include "engine/core/types.h"

// uc_orig: AA_MAX_SPAN_X (fallen/DDEngine/Source/aa.cpp)
#define AA_MAX_SPAN_X 33
// uc_orig: AA_MAX_SPAN_Y (fallen/DDEngine/Source/aa.cpp)
#define AA_MAX_SPAN_Y 33

// uc_orig: AA_Span (fallen/DDEngine/Source/aa.cpp)
typedef struct {
    SLONG rhs_min, rhs_max, lhs_min, lhs_max;
    SLONG pixel[AA_MAX_SPAN_X];
} AA_Span;

// uc_orig: AA_span (fallen/DDEngine/Source/aa.cpp)
extern AA_Span AA_span[AA_MAX_SPAN_Y];

#endif // ENGINE_GRAPHICS_GEOMETRY_AA_GLOBALS_H
