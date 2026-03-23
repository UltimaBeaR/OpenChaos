#include "world/environment/stair_globals.h"

// uc_orig: STAIR_x1 (fallen/Source/Stair.cpp)
UBYTE STAIR_x1;
// uc_orig: STAIR_z1 (fallen/Source/Stair.cpp)
UBYTE STAIR_z1;
// uc_orig: STAIR_x2 (fallen/Source/Stair.cpp)
UBYTE STAIR_x2;
// uc_orig: STAIR_z2 (fallen/Source/Stair.cpp)
UBYTE STAIR_z2;

// uc_orig: STAIR_roof_height (fallen/Source/Stair.cpp)
UBYTE STAIR_roof_height;

// uc_orig: STAIR_stair (fallen/Source/Stair.cpp)
STAIR_Stair STAIR_stair[STAIR_MAX_STAIRS];
// uc_orig: STAIR_stair_upto (fallen/Source/Stair.cpp)
SLONG STAIR_stair_upto;

// uc_orig: STAIR_storey (fallen/Source/Stair.cpp)
STAIR_Storey STAIR_storey[STAIR_MAX_STOREYS];
// uc_orig: STAIR_storey_upto (fallen/Source/Stair.cpp)
SLONG STAIR_storey_upto;

// uc_orig: STAIR_link (fallen/Source/Stair.cpp)
STAIR_Link STAIR_link[STAIR_MAX_LINKS];
// uc_orig: STAIR_link_upto (fallen/Source/Stair.cpp)
SLONG STAIR_link_upto;

// uc_orig: STAIR_edge (fallen/Source/Stair.cpp)
UBYTE STAIR_edge[STAIR_MAX_SIZE];

// uc_orig: STAIR_rand_seed (fallen/Source/Stair.cpp)
ULONG STAIR_rand_seed;

// uc_orig: STAIR_id_stair (fallen/Source/Stair.cpp)
ID_Stair STAIR_id_stair[STAIR_MAX_PER_STOREY];
