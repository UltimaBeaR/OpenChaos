#ifndef WORLD_MAP_SUPERMAP_H
#define WORLD_MAP_SUPERMAP_H

#include "core/types.h"
// Structure definitions for DFacet, DBuilding, DWalkable, DStorey, DInsideRect,
// MAX_DFACETS etc. come from fallen/Headers/supermap.h (not redirected — structs live there).
// Note: fallen/Headers/supermap.h does NOT include world/map/supermap.h, so no circular.
#include "fallen/Headers/supermap.h"
// STOREY_TYPE_*, FACET_FLAG_* constants from building.h (not yet migrated to new/).
#include "fallen/Headers/building.h"

// Supermap: pre-computed spatial index of all buildings, facets, and walkables.
// Loaded from the binary level file (.pam) via load_super_map().

// ---- Globals ----

// uc_orig: level_index (fallen/Source/supermap.cpp)
extern ULONG level_index;

// Generation stamps for spatial de-duplication (one per camera/query pass).
// When DFacet.Counter[which] == SUPERMAP_counter[which], facet was already visited this frame.
// uc_orig: SUPERMAP_counter (fallen/Source/supermap.cpp)
extern UBYTE SUPERMAP_counter[2];

// Prim array ranges occupied by walkable geometry loaded from the level file.
// uc_orig: first_walkable_prim_point (fallen/Source/supermap.cpp)
extern SLONG first_walkable_prim_point;
// uc_orig: number_of_walkable_prim_points (fallen/Source/supermap.cpp)
extern SLONG number_of_walkable_prim_points;
// uc_orig: first_walkable_prim_face4 (fallen/Source/supermap.cpp)
extern SLONG first_walkable_prim_face4;
// uc_orig: number_of_walkable_prim_faces4 (fallen/Source/supermap.cpp)
extern SLONG number_of_walkable_prim_faces4;

// Person class skip bitmask for current level (set by get_level_no()).
// uc_orig: people_types (fallen/Source/supermap.cpp)
extern SWORD people_types[50];
// uc_orig: DONT_load (fallen/Source/supermap.cpp)
extern ULONG DONT_load;

// ---- Functions ----

// uc_orig: load_super_map (fallen/Source/supermap.cpp)
void load_super_map(MFFileHandle handle, SLONG save_type);

// uc_orig: add_sewer_ladder (fallen/Source/supermap.cpp)
void add_sewer_ladder(SLONG x1, SLONG z1, SLONG x2, SLONG z2,
                      SLONG bottom, SLONG height, SLONG link);

// uc_orig: find_electric_fence_dbuilding (fallen/Source/supermap.cpp)
SLONG find_electric_fence_dbuilding(SLONG world_x, SLONG world_y, SLONG world_z, SLONG range);

// uc_orig: set_electric_fence_state (fallen/Source/supermap.cpp)
void set_electric_fence_state(SLONG dbuilding, SLONG onoroff);

// uc_orig: SUPERMAP_counter_increase (fallen/Source/supermap.cpp)
void SUPERMAP_counter_increase(UBYTE which);

// Returns level index for mission name, or 0 if not found. Sets DONT_load bitmask.
// uc_orig: get_level_no (fallen/Source/supermap.cpp)
SLONG get_level_no(CBYTE* name);

// Returns InsideStorey index (1-based) for world point (x,y,z), or 0 if outdoors.
// Also fills *room with the room ID within that InsideStorey.
// uc_orig: calc_inside_for_xyz (fallen/Source/supermap.cpp)
UWORD calc_inside_for_xyz(SLONG x, SLONG y, SLONG z, UWORD* room);

// Dev tool: iterates all levels, generates texture clump files, then exits.
// Called from game_startup() when TEXTURE_create_clump mode is active.
// uc_orig: make_all_clumps (fallen/Source/supermap.cpp)
void make_all_clumps(void);

#endif // WORLD_MAP_SUPERMAP_H
