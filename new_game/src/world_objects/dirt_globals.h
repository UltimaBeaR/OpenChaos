#ifndef WORLD_OBJECTS_DIRT_GLOBALS_H
#define WORLD_OBJECTS_DIRT_GLOBALS_H

#include "world_objects/dirt.h"

// uc_orig: DIRT_Tree (fallen/Source/dirt.cpp)
typedef struct
{
    UWORD x;
    UWORD z;
    UBYTE inrange;
    UBYTE padding;
} DIRT_Tree;

// uc_orig: DIRT_MAX_TREES (fallen/Source/dirt.cpp)
#define DIRT_MAX_TREES 64

// uc_orig: DIRT_tree (fallen/Source/dirt.cpp)
extern DIRT_Tree DIRT_tree[DIRT_MAX_TREES];
// uc_orig: DIRT_tree_upto (fallen/Source/dirt.cpp)
extern SLONG DIRT_tree_upto;

// uc_orig: DIRT_prob_leaf (fallen/Source/dirt.cpp)
extern SLONG DIRT_prob_leaf;
// uc_orig: DIRT_prob_can (fallen/Source/dirt.cpp)
extern SLONG DIRT_prob_can;
// uc_orig: DIRT_prob_pigeon (fallen/Source/dirt.cpp)
extern SLONG DIRT_prob_pigeon;

// uc_orig: DIRT_pigeon_map_x1 (fallen/Source/dirt.cpp)
extern SLONG DIRT_pigeon_map_x1;
// uc_orig: DIRT_pigeon_map_z1 (fallen/Source/dirt.cpp)
extern SLONG DIRT_pigeon_map_z1;
// uc_orig: DIRT_pigeon_map_x2 (fallen/Source/dirt.cpp)
extern SLONG DIRT_pigeon_map_x2;
// uc_orig: DIRT_pigeon_map_z2 (fallen/Source/dirt.cpp)
extern SLONG DIRT_pigeon_map_z2;

// uc_orig: DIRT_focus_x (fallen/Source/dirt.cpp)
extern SLONG DIRT_focus_x;
// uc_orig: DIRT_focus_z (fallen/Source/dirt.cpp)
extern SLONG DIRT_focus_z;
// uc_orig: DIRT_focus_radius (fallen/Source/dirt.cpp)
extern SLONG DIRT_focus_radius;
// uc_orig: DIRT_focus_first (fallen/Source/dirt.cpp)
extern SLONG DIRT_focus_first;

// uc_orig: DIRT_dirt (fallen/Source/dirt.cpp)
extern DIRT_Dirt DIRT_dirt[DIRT_MAX_DIRT];

// DIRT_check was static in original; lives in globals per project rule.
// uc_orig: DIRT_check (fallen/Source/dirt.cpp)
extern UWORD DIRT_check;

#endif // WORLD_OBJECTS_DIRT_GLOBALS_H
