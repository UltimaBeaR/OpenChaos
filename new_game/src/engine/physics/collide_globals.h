#ifndef ENGINE_PHYSICS_COLLIDE_GLOBALS_H
#define ENGINE_PHYSICS_COLLIDE_GLOBALS_H

#include "engine/physics/collide.h"

// uc_orig: dprod (fallen/Source/collide.cpp)
// Shared dot product accumulator used across geometry helper functions.
extern SLONG dprod;

// uc_orig: cprod (fallen/Source/collide.cpp)
// Shared cross product accumulator used across geometry helper functions.
extern SLONG cprod;

// uc_orig: global_on (fallen/Source/collide.cpp)
// Set by nearest_point_on_line_and_dist: 1 if projected point lies on the segment, 0 if at an endpoint.
extern SLONG global_on;

// uc_orig: next_col_vect (fallen/Source/collide.cpp)
// Index of the next free slot in col_vects[]. Starts at 1 (slot 0 unused).
extern UWORD next_col_vect;

// uc_orig: next_col_vect_link (fallen/Source/collide.cpp)
// Index of the next free slot in col_vects_links[]. Starts at 1.
extern UWORD next_col_vect_link;

// uc_orig: walk_links (fallen/Source/collide.cpp)
// Pool of walkable-surface linked-list nodes. 30000 entries, 4 bytes each = 120KB.
extern struct WalkLink walk_links[MAX_WALK_POOL];

// uc_orig: next_walk_link (fallen/Source/collide.cpp)
// Index of the next free slot in walk_links[]. Starts at 1.
extern UWORD next_walk_link;

// uc_orig: MAX_ALREADY (fallen/Source/collide.cpp)
// Maximum number of already-processed facets remembered per slide_along call.
// Defined here (rather than collide.h) to avoid a circular include.
#define MAX_ALREADY 50

// uc_orig: already (fallen/Source/collide.cpp)
// Per-call cache of facet indices already tested during one slide_along pass.
extern UWORD already[MAX_ALREADY];

// uc_orig: max_facet_find (fallen/Source/collide.cpp)
// Debug counter tracking the highest number of facets found in a single cell.
extern UWORD max_facet_find;

// uc_orig: last_slide_colvect (fallen/Source/collide.cpp)
// Index of the last facet/colvect that deflected movement in slide_along.
// Set to fence_colvect if the slide was along a fence.
extern SLONG last_slide_colvect;

// uc_orig: last_slide_dist (fallen/Source/collide.cpp)
// Distance slid along the wall in the last slide_along call.
extern SLONG last_slide_dist;

// uc_orig: actual_sliding (fallen/Source/collide.cpp)
// Set to UC_TRUE inside slide_along when the movement vector was actually
// deflected (i.e. the person hit a wall).
extern SLONG actual_sliding;

// uc_orig: slide_door (fallen/Source/collide.cpp)
// Non-zero when slide_along crossed a door facet; carries the door storey index.
extern SLONG slide_door;

// uc_orig: slide_ladder (fallen/Source/collide.cpp)
// Non-zero when slide_along touched a ladder facet.
extern SLONG slide_ladder;

// uc_orig: slide_into_warehouse (fallen/Source/collide.cpp)
// NULL if no warehouse was entered; otherwise holds the warehouse index.
extern SLONG slide_into_warehouse;

// uc_orig: slide_outof_warehouse (fallen/Source/collide.cpp)
// Companion to slide_into_warehouse for warehouse exit detection.
extern SLONG slide_outof_warehouse;

// uc_orig: slid_along_fence (fallen/Source/collide.cpp)
// UC_TRUE if the current slide_along call deflected off a fence facet.
extern UBYTE slid_along_fence;

// uc_orig: fence_colvect (fallen/Source/collide.cpp)
// Index of the fence facet that was last slid along.
extern UWORD fence_colvect;

// ========================================================================
// collide_against_things scratch buffer (chunk 4)
// ========================================================================

// uc_orig: MAX_COL_WITH (fallen/Source/collide.cpp)
// Maximum number of Things that collide_against_things inspects per frame.
#define MAX_COL_WITH 16

// uc_orig: col_with_things (fallen/Source/collide.cpp)
// Scratch buffer for THING_find_sphere results in collide_against_things / drop_on_heads.
extern THING_INDEX col_with_things[MAX_COL_WITH];

// ========================================================================
// LOS (Line of Sight) globals (chunk 5)
// ========================================================================

// uc_orig: save_stack (fallen/Source/collide.cpp)
// Shared LOS ray endpoints: set by there_is_a_los, read by check_vector_against_mapsquare
// and check_vector_against_mapsquare_objects during cell traversal.
// Named struct type extracted from the original anonymous struct for extern linkage.
struct SaveStack
{
    SLONG x1;
    SLONG my_y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;
};
extern SaveStack save_stack;

// uc_orig: los_failure_x (fallen/Source/collide.cpp)
// World X coordinate of the last LOS failure point (where sight was blocked).
extern SLONG los_failure_x;

// uc_orig: los_failure_y (fallen/Source/collide.cpp)
// World Y coordinate of the last LOS failure point.
extern SLONG los_failure_y;

// uc_orig: los_failure_z (fallen/Source/collide.cpp)
// World Z coordinate of the last LOS failure point.
extern SLONG los_failure_z;

// uc_orig: los_failure_dfacet (fallen/Source/collide.cpp)
// DFacet index that blocked the last LOS check (NULL if blocked by terrain).
extern SLONG los_failure_dfacet;

// uc_orig: los_done (fallen/Source/collide.cpp)
// Ring buffer of the last 4 dfacet indices checked — avoids re-testing the same facet twice per cell.
extern SLONG los_done[4];

// uc_orig: los_wptr (fallen/Source/collide.cpp)
// Write pointer into los_done[]; incremented mod 4.
extern SLONG los_wptr;

// uc_orig: los_v_x (fallen/Source/collide.cpp)
// Scratch X position used in there_is_a_los underground check.
extern SLONG los_v_x;

// uc_orig: los_v_y (fallen/Source/collide.cpp)
// Scratch Y position used in there_is_a_los underground check.
extern SLONG los_v_y;

// uc_orig: los_v_z (fallen/Source/collide.cpp)
// Scratch Z position used in there_is_a_los underground check.
extern SLONG los_v_z;

// uc_orig: los_v_dx (fallen/Source/collide.cpp)
// Total X delta for the current LOS ray.
extern SLONG los_v_dx;

// uc_orig: los_v_dy (fallen/Source/collide.cpp)
// Total Y delta for the current LOS ray.
extern SLONG los_v_dy;

// uc_orig: los_v_dz (fallen/Source/collide.cpp)
// Total Z delta for the current LOS ray.
extern SLONG los_v_dz;

// uc_orig: los_v_mx (fallen/Source/collide.cpp)
// Current map X cell during LOS ray traversal.
extern SLONG los_v_mx;

// uc_orig: los_v_mz (fallen/Source/collide.cpp)
// Current map Z cell during LOS ray traversal.
extern SLONG los_v_mz;

// uc_orig: los_v_end_mx (fallen/Source/collide.cpp)
// Target map X cell for the LOS ray.
extern SLONG los_v_end_mx;

// uc_orig: los_v_end_mz (fallen/Source/collide.cpp)
// Target map Z cell for the LOS ray.
extern SLONG los_v_end_mz;

// uc_orig: last_mav_square_x (fallen/Source/collide.cpp)
// Map X cell of the last MAV cell traversed by there_is_a_los_mav / there_is_a_los_car.
extern UBYTE last_mav_square_x;

// uc_orig: last_mav_square_z (fallen/Source/collide.cpp)
// Map Z cell of the last MAV cell traversed.
extern UBYTE last_mav_square_z;

// uc_orig: last_mav_dx (fallen/Source/collide.cpp)
// Direction X of the wall hit by there_is_a_los_car (-1, 0, or 1).
extern SBYTE last_mav_dx;

// uc_orig: last_mav_dz (fallen/Source/collide.cpp)
// Direction Z of the wall hit by there_is_a_los_car (-1, 0, or 1).
extern SBYTE last_mav_dz;

// ========================================================================
// slide_around_box scratch globals (chunk 5b)
// ========================================================================

// uc_orig: tried (fallen/Source/collide.cpp)
// Bitmask tracking which slide directions have already been attempted
// in slide_around_box. Avoids re-checking directions that lead to NOGO squares.
// Comment in original: "should be locals but stack overflow".
extern SLONG tried;

// uc_orig: used_this_go (fallen/Source/collide.cpp)
// Records which slide direction was chosen in the current slide_around_box iteration.
extern SLONG used_this_go;

// uc_orig: failed (fallen/Source/collide.cpp)
// Loop flag: non-zero while slide_around_box still needs to retry a new direction.
extern SLONG failed;

// ========================================================================
// COLLIDE_fastnav — fastnav bits array (chunk 5b)
// ========================================================================

// uc_orig: COLLIDE_fastnav (fallen/Source/collide.cpp)
// Bit array: one bit per PAP_HI map cell. 1 = fastnav allowed (no nearby DFacets).
// Allocated externally (see COLLIDE_calc_fastnav_bits). PAP_SIZE_HI rows,
// each row is a (PAP_SIZE_HI >> 3) byte array packed as UBYTE.
extern COLLIDE_Fastnavrow* COLLIDE_fastnav;

// ========================================================================
// Fall-off flags (chunk 4)
// ========================================================================

// uc_orig: FALL_OFF_FLAG_TRUE (fallen/Source/collide.cpp)
// Set when move_thing determines the person has walked off a face edge.
#define FALL_OFF_FLAG_TRUE      (1 << 0)

// uc_orig: FALL_OFF_FLAG_FIRE_ESCAPE (fallen/Source/collide.cpp)
// Set when the person is walking on a fire-escape face (affects fall-off behaviour).
#define FALL_OFF_FLAG_FIRE_ESCAPE (1 << 1)

// uc_orig: FALL_OFF_FLAG_DONT_GRAB (fallen/Source/collide.cpp)
// Suppresses ledge-grab when falling off a WMOVE or special prim face.
#define FALL_OFF_FLAG_DONT_GRAB (1 << 2)

#endif // ENGINE_PHYSICS_COLLIDE_GLOBALS_H
