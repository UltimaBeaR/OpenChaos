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
