#include "engine/platform/platform.h"
#include "things/core/interact_globals.h"

// uc_orig: grab_px (fallen/Source/interact.cpp)
SLONG grab_px[4];
// uc_orig: grab_py (fallen/Source/interact.cpp)
SLONG grab_py[4];
// uc_orig: grab_pz (fallen/Source/interact.cpp)
SLONG grab_pz[4];
// uc_orig: best_dist (fallen/Source/interact.cpp)
SLONG best_dist;
// uc_orig: best_x (fallen/Source/interact.cpp)
SLONG best_x;
// uc_orig: best_y (fallen/Source/interact.cpp)
SLONG best_y;
// uc_orig: best_z (fallen/Source/interact.cpp)
SLONG best_z;
// uc_orig: best_angle (fallen/Source/interact.cpp)
SLONG best_angle;

// uc_orig: r_matrix (fallen/Source/interact.cpp)
struct Matrix33 r_matrix;
// uc_orig: offset (fallen/Source/interact.cpp)
struct Matrix31 offset;
// uc_orig: matrix (fallen/Source/interact.cpp)
SLONG matrix[9];
