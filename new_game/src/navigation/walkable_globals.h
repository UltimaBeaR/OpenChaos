#ifndef NAVIGATION_WALKABLE_GLOBALS_H
#define NAVIGATION_WALKABLE_GLOBALS_H

// Global vertex scratch buffers used by get_height_on_face_quad64_at.
// They live in global storage to avoid PSX stack overflow.

#include "engine/core/types.h"

// uc_orig: gh_vx (fallen/Source/walkable.cpp)
extern SLONG gh_vx[4];
// uc_orig: gh_vy (fallen/Source/walkable.cpp)
extern SLONG gh_vy[4];
// uc_orig: gh_vz (fallen/Source/walkable.cpp)
extern SLONG gh_vz[4];

#endif // NAVIGATION_WALKABLE_GLOBALS_H
