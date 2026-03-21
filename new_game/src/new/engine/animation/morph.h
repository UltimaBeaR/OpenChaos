#ifndef ENGINE_ANIMATION_MORPH_H
#define ENGINE_ANIMATION_MORPH_H

// Vertex morph keyframe data for pigeon mesh animation.
// Each morph is a set of 3D vertex positions loaded from an .ASC (3DS Max ASCII) file.
// Vertex interpolation between morphs is done by the renderer (see DT_TWEEN draw path).

#include "core/types.h"

// uc_orig: MORPH_PIGEON_WALK1 (fallen/Headers/morph.h)
#define MORPH_PIGEON_WALK1 0
// uc_orig: MORPH_PIGEON_WALK2 (fallen/Headers/morph.h)
#define MORPH_PIGEON_WALK2 1
// uc_orig: MORPH_PIGEON_STAND (fallen/Headers/morph.h)
#define MORPH_PIGEON_STAND 0
// uc_orig: MORPH_PIGEON_PECK (fallen/Headers/morph.h)
#define MORPH_PIGEON_PECK 0
// uc_orig: MORPH_PIGEON_HEADCOCK (fallen/Headers/morph.h)
#define MORPH_PIGEON_HEADCOCK 0
// uc_orig: MORPH_NUMBER (fallen/Headers/morph.h)
#define MORPH_NUMBER 2

// A single vertex position in a morph keyframe.
// uc_orig: MORPH_Point (fallen/Headers/morph.h)
typedef struct {
    SWORD x;
    SWORD y;
    SWORD z;
} MORPH_Point;

// Loads all MORPH_NUMBER morphs from their .ASC source files.
// uc_orig: MORPH_load (fallen/Source/morph.cpp)
void MORPH_load(void);

// Returns a pointer to the first vertex of the given morph index.
// uc_orig: MORPH_get_points (fallen/Source/morph.cpp)
MORPH_Point* MORPH_get_points(SLONG morph);

// Returns the number of vertices in the given morph.
// uc_orig: MORPH_get_num_points (fallen/Source/morph.cpp)
SLONG MORPH_get_num_points(SLONG morph);

#endif // ENGINE_ANIMATION_MORPH_H
