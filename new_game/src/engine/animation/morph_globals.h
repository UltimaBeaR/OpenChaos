#ifndef ENGINE_ANIMATION_MORPH_GLOBALS_H
#define ENGINE_ANIMATION_MORPH_GLOBALS_H

#include "engine/animation/morph.h"

// Maximum total vertices across all loaded morphs.
// uc_orig: MORPH_MAX_POINTS (fallen/Source/morph.cpp)
#define MORPH_MAX_POINTS 1024

// Flat pool of all vertex positions for all loaded morphs.
// uc_orig: MORPH_point (fallen/Source/morph.cpp)
extern MORPH_Point MORPH_point[MORPH_MAX_POINTS];

// Number of vertices loaded so far across all morphs.
// uc_orig: MORPH_point_upto (fallen/Source/morph.cpp)
extern SLONG MORPH_point_upto;

// Source .ASC file path for each morph index.
// uc_orig: MORPH_filename (fallen/Source/morph.cpp)
extern const char* MORPH_filename[MORPH_NUMBER];

// Internal per-morph metadata (index + vertex count within MORPH_point[]).
// uc_orig: MORPH_Morph (fallen/Source/morph.cpp)
typedef struct {
    UWORD num_points;
    UWORD index;
} MORPH_Morph;

// uc_orig: MORPH_morph (fallen/Source/morph.cpp)
extern MORPH_Morph MORPH_morph[MORPH_NUMBER];

#endif // ENGINE_ANIMATION_MORPH_GLOBALS_H
