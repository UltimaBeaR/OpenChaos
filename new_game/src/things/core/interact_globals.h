#ifndef THINGS_CORE_INTERACT_GLOBALS_H
#define THINGS_CORE_INTERACT_GLOBALS_H

#include "engine/platform/uc_common.h"
#include "buildings/prim_types.h" // PrimFace3/4, FACE_FLAG_*, PRIM_OBJ_*, etc.

// Best-candidate grab point storage used by find_grab_face().
// These hold coordinates/angle across loop iterations to track the best found grab point.
// uc_orig: grab_px (fallen/Source/interact.cpp)
extern SLONG grab_px[4];
// uc_orig: grab_py (fallen/Source/interact.cpp)
extern SLONG grab_py[4];
// uc_orig: grab_pz (fallen/Source/interact.cpp)
extern SLONG grab_pz[4];
// uc_orig: best_dist (fallen/Source/interact.cpp)
extern SLONG best_dist;

// Temporary rotation matrix and offset vector used by calc_sub_objects_position().
// uc_orig: offset (fallen/Source/interact.cpp)
extern struct Matrix31 offset;
// uc_orig: matrix (fallen/Source/interact.cpp)
extern SLONG matrix[9];

#endif // THINGS_CORE_INTERACT_GLOBALS_H
