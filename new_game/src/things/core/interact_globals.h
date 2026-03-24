#ifndef THINGS_CORE_INTERACT_GLOBALS_H
#define THINGS_CORE_INTERACT_GLOBALS_H

#include "engine/platform/uc_common.h"
#include "world/environment/prim_types.h" // PrimFace3/4, FACE_FLAG_*, PRIM_OBJ_*, etc.

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
// uc_orig: best_x (fallen/Source/interact.cpp)
extern SLONG best_x;
// uc_orig: best_y (fallen/Source/interact.cpp)
extern SLONG best_y;
// uc_orig: best_z (fallen/Source/interact.cpp)
extern SLONG best_z;
// uc_orig: best_angle (fallen/Source/interact.cpp)
extern SLONG best_angle;

// Temporary rotation matrix and offset vector used by calc_sub_objects_position().
// uc_orig: r_matrix (fallen/Source/interact.cpp)
extern struct Matrix33 r_matrix;
// uc_orig: offset (fallen/Source/interact.cpp)
extern struct Matrix31 offset;
// uc_orig: matrix (fallen/Source/interact.cpp)
extern SLONG matrix[9];

#endif // THINGS_CORE_INTERACT_GLOBALS_H
