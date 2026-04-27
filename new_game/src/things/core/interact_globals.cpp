#include "engine/platform/uc_common.h"
#include "things/core/interact_globals.h"

// uc_orig: grab_px (fallen/Source/interact.cpp)
SLONG grab_px[4];
// uc_orig: grab_py (fallen/Source/interact.cpp)
SLONG grab_py[4];
// uc_orig: grab_pz (fallen/Source/interact.cpp)
SLONG grab_pz[4];
// uc_orig: best_dist (fallen/Source/interact.cpp)
SLONG best_dist;

// uc_orig: offset (fallen/Source/interact.cpp)
struct Matrix31 offset;
// uc_orig: matrix (fallen/Source/interact.cpp)
SLONG matrix[9];
