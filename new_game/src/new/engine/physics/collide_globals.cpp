#include "engine/physics/collide_globals.h"

// uc_orig: dprod (fallen/Source/collide.cpp)
SLONG dprod = 0;

// uc_orig: cprod (fallen/Source/collide.cpp)
SLONG cprod = 0;

// uc_orig: global_on (fallen/Source/collide.cpp)
SLONG global_on = 0;

// uc_orig: next_col_vect (fallen/Source/collide.cpp)
UWORD next_col_vect = 1;

// uc_orig: next_col_vect_link (fallen/Source/collide.cpp)
UWORD next_col_vect_link = 1;

// uc_orig: walk_links (fallen/Source/collide.cpp)
struct WalkLink walk_links[MAX_WALK_POOL];

// uc_orig: next_walk_link (fallen/Source/collide.cpp)
UWORD next_walk_link = 1;

// uc_orig: already (fallen/Source/collide.cpp)
UWORD already[MAX_ALREADY];

// uc_orig: max_facet_find (fallen/Source/collide.cpp)
UWORD max_facet_find = 0;

// uc_orig: last_slide_colvect (fallen/Source/collide.cpp)
SLONG last_slide_colvect = 0;

// uc_orig: last_slide_dist (fallen/Source/collide.cpp)
SLONG last_slide_dist = 0;

// uc_orig: actual_sliding (fallen/Source/collide.cpp)
SLONG actual_sliding = 0;

// uc_orig: slide_door (fallen/Source/collide.cpp)
SLONG slide_door = 0;

// uc_orig: slide_ladder (fallen/Source/collide.cpp)
SLONG slide_ladder = 0;

// uc_orig: slide_into_warehouse (fallen/Source/collide.cpp)
SLONG slide_into_warehouse = 0;

// uc_orig: slide_outof_warehouse (fallen/Source/collide.cpp)
SLONG slide_outof_warehouse = 0;

// uc_orig: slid_along_fence (fallen/Source/collide.cpp)
UBYTE slid_along_fence = 0;

// uc_orig: fence_colvect (fallen/Source/collide.cpp)
UWORD fence_colvect = 0;

// uc_orig: col_with_things (fallen/Source/collide.cpp)
THING_INDEX col_with_things[MAX_COL_WITH];

// ========================================================================
// LOS (Line of Sight) globals (chunk 5)
// ========================================================================

// uc_orig: save_stack (fallen/Source/collide.cpp)
SaveStack save_stack;

// uc_orig: los_failure_x (fallen/Source/collide.cpp)
SLONG los_failure_x = 0;

// uc_orig: los_failure_y (fallen/Source/collide.cpp)
SLONG los_failure_y = 0;

// uc_orig: los_failure_z (fallen/Source/collide.cpp)
SLONG los_failure_z = 0;

// uc_orig: los_failure_dfacet (fallen/Source/collide.cpp)
SLONG los_failure_dfacet = 0;

// uc_orig: los_done (fallen/Source/collide.cpp)
SLONG los_done[4] = { 0, 0, 0, 0 };

// uc_orig: los_wptr (fallen/Source/collide.cpp)
SLONG los_wptr = 0;

// uc_orig: los_v_x (fallen/Source/collide.cpp)
SLONG los_v_x = 0;

// uc_orig: los_v_y (fallen/Source/collide.cpp)
SLONG los_v_y = 0;

// uc_orig: los_v_z (fallen/Source/collide.cpp)
SLONG los_v_z = 0;

// uc_orig: los_v_dx (fallen/Source/collide.cpp)
SLONG los_v_dx = 0;

// uc_orig: los_v_dy (fallen/Source/collide.cpp)
SLONG los_v_dy = 0;

// uc_orig: los_v_dz (fallen/Source/collide.cpp)
SLONG los_v_dz = 0;

// uc_orig: los_v_mx (fallen/Source/collide.cpp)
SLONG los_v_mx = 0;

// uc_orig: los_v_mz (fallen/Source/collide.cpp)
SLONG los_v_mz = 0;

// uc_orig: los_v_end_mx (fallen/Source/collide.cpp)
SLONG los_v_end_mx = 0;

// uc_orig: los_v_end_mz (fallen/Source/collide.cpp)
SLONG los_v_end_mz = 0;

// uc_orig: last_mav_square_x (fallen/Source/collide.cpp)
UBYTE last_mav_square_x = 0;

// uc_orig: last_mav_square_z (fallen/Source/collide.cpp)
UBYTE last_mav_square_z = 0;

// uc_orig: last_mav_dx (fallen/Source/collide.cpp)
SBYTE last_mav_dx = 0;

// uc_orig: last_mav_dz (fallen/Source/collide.cpp)
SBYTE last_mav_dz = 0;
