// Global variable definitions for the prim (static world mesh object) system.

#include "buildings/prim_globals.h"
#include "engine/core/types.h" // CBYTE, SLONG, UWORD

// uc_orig: prim_info (fallen/Source/Prim.cpp)
PrimInfo* prim_info;

// uc_orig: prim_names (fallen/Source/Prim.cpp)
CBYTE prim_names[MAX_PRIM_OBJECTS][32];

// uc_orig: each_point (fallen/Source/Prim.cpp)
UBYTE each_point[MAX_POINTS_PER_PRIM];

// uc_orig: found_aprim (fallen/Source/Prim.cpp)
UWORD found_aprim[MAX_FIND_ANIM_PRIMS];
