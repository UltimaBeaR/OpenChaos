#ifndef ENGINE_PHYSICS_COLLIDE_GLOBALS_H
#define ENGINE_PHYSICS_COLLIDE_GLOBALS_H

// Temporary: Game.h provides THING_INDEX and other base types needed by fallen/Headers/collide.h
#include "Game.h"
#include "fallen/Headers/collide.h"

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
// Set to TRUE inside slide_along when the movement vector was actually
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
// TRUE if the current slide_along call deflected off a fence facet.
extern UBYTE slid_along_fence;

// uc_orig: fence_colvect (fallen/Source/collide.cpp)
// Index of the fence facet that was last slid along.
extern UWORD fence_colvect;

#endif // ENGINE_PHYSICS_COLLIDE_GLOBALS_H
