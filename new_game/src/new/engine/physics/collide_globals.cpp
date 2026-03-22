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
