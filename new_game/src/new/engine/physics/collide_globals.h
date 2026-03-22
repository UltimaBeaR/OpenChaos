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

#endif // ENGINE_PHYSICS_COLLIDE_GLOBALS_H
