#ifndef MAP_SUPERMAP_GLOBALS_H
#define MAP_SUPERMAP_GLOBALS_H

#include "engine/core/types.h"

// Global state for the supermap system: allocation cursors, level list, per-frame counters.

// uc_orig: level_index (fallen/Source/supermap.cpp)
extern ULONG level_index;

// uc_orig: SUPERMAP_counter (fallen/Source/supermap.cpp)
extern UBYTE SUPERMAP_counter[2];

// uc_orig: first_walkable_prim_point (fallen/Source/supermap.cpp)
extern SLONG first_walkable_prim_point;
// uc_orig: number_of_walkable_prim_points (fallen/Source/supermap.cpp)
extern SLONG number_of_walkable_prim_points;
// uc_orig: first_walkable_prim_face4 (fallen/Source/supermap.cpp)
extern SLONG first_walkable_prim_face4;
// uc_orig: number_of_walkable_prim_faces4 (fallen/Source/supermap.cpp)
extern SLONG number_of_walkable_prim_faces4;

// uc_orig: people_types (fallen/Source/supermap.cpp)
extern SWORD people_types[50];
// uc_orig: DONT_load (fallen/Source/supermap.cpp)
extern ULONG DONT_load;

// Allocation cursors for supermap arrays; initialised to 1 (0 = unused sentinel).
// Declared extern in world/map/supermap.h — definitions live here.
// uc_orig: next_dbuilding (fallen/Source/supermap.cpp)
extern SLONG next_dbuilding;
// uc_orig: next_dfacet (fallen/Source/supermap.cpp)
extern SLONG next_dfacet;
// uc_orig: next_dstyle (fallen/Source/supermap.cpp)
extern SLONG next_dstyle;
// uc_orig: next_dwalkable (fallen/Source/supermap.cpp)
extern SLONG next_dwalkable;
// uc_orig: next_dstorey (fallen/Source/supermap.cpp)
extern SWORD next_dstorey;
// uc_orig: next_paint_mem (fallen/Source/supermap.cpp)
extern SWORD next_paint_mem;
// uc_orig: next_facet_link (fallen/Source/supermap.cpp)
extern SWORD next_facet_link;
// uc_orig: facet_link_count (fallen/Source/supermap.cpp)
extern SWORD facet_link_count;
// uc_orig: next_inside_mem (fallen/Source/supermap.cpp)
extern SLONG next_inside_mem;

// Level-list entry: maps .ucm mission name to .map filename, level index, and person skip mask.
// uc_orig: Levels (fallen/Source/supermap.cpp)
struct Levels {
    CBYTE* name;
    CBYTE* map_name;
    UWORD level;
    ULONG dontload;
};

// Full mission list, null-terminated by a zero level index.
// uc_orig: levels (fallen/Source/supermap.cpp)
extern struct Levels levels[];

#endif // MAP_SUPERMAP_GLOBALS_H
