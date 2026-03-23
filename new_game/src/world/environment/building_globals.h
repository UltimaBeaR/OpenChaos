#ifndef WORLD_ENVIRONMENT_BUILDING_GLOBALS_H
#define WORLD_ENVIRONMENT_BUILDING_GLOBALS_H

#include "core/types.h"                // SWORD, UWORD, SLONG, UBYTE, etc.
#include "core/vector.h"               // SVector (needed by building.h for PrimNormal typedef)
#include "fallen/Headers/building.h"   // Temporary: FBuilding, FStorey, FWall, FWindow, BuildingFacet, BuildingObject, BoundBox, TXTY, DXTXTY, TextureInfo, RoomID, MAX_*
#include "fallen/Headers/edmap.h"      // Temporary: DepthStrip, EDIT_MAP_WIDTH, EDIT_MAP_DEPTH

// uc_orig: edit_map (fallen/Source/Building.cpp)
extern struct DepthStrip edit_map[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];

// uc_orig: tex_map (fallen/Source/Building.cpp)
extern UWORD tex_map[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];

// uc_orig: edit_map_roof_height (fallen/Source/Building.cpp)
extern SBYTE edit_map_roof_height[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];

// uc_orig: start_point (fallen/Source/Building.cpp)
extern SLONG start_point[200];

// uc_orig: next_roof_bound (fallen/Source/Building.cpp)
// NOTE: already declared extern in fallen/Headers/building.h
extern UWORD next_roof_bound;

// uc_orig: background_prim (fallen/Source/Building.cpp)
extern UWORD background_prim;

// uc_orig: end_prim_point (fallen/Source/Building.cpp)
extern UWORD end_prim_point;

// uc_orig: end_prim_face4 (fallen/Source/Building.cpp)
extern UWORD end_prim_face4;

// uc_orig: end_prim_face3 (fallen/Source/Building.cpp)
extern UWORD end_prim_face3;

// uc_orig: end_prim_object (fallen/Source/Building.cpp)
extern UWORD end_prim_object;

// uc_orig: end_prim_multi_object (fallen/Source/Building.cpp)
extern UWORD end_prim_multi_object;

// uc_orig: next_building_object (fallen/Source/Building.cpp)
extern UWORD next_building_object;

// uc_orig: end_building_object (fallen/Source/Building.cpp)
extern UWORD end_building_object;

// uc_orig: next_building_facet (fallen/Source/Building.cpp)
extern UWORD next_building_facet;

// uc_orig: end_building_facet (fallen/Source/Building.cpp)
extern UWORD end_building_facet;

// uc_orig: diff_page_count1 (fallen/Source/Building.cpp)
extern UWORD diff_page_count1;

// uc_orig: diff_page_count2 (fallen/Source/Building.cpp)
extern UWORD diff_page_count2;

// uc_orig: page_count (fallen/Source/Building.cpp)
extern UWORD page_count[64 * 8];

// uc_orig: window_list (fallen/Source/Building.cpp)
// NOTE: also declared extern in fallen/Headers/building.h
extern struct FWindow window_list[MAX_WINDOWS];

// uc_orig: wall_list (fallen/Source/Building.cpp)
extern struct FWall wall_list[MAX_WALLS];

// uc_orig: storey_list (fallen/Source/Building.cpp)
extern struct FStorey storey_list[MAX_STOREYS];

// uc_orig: building_list (fallen/Source/Building.cpp)
extern struct FBuilding building_list[MAX_BUILDINGS];

// uc_orig: building_facets (fallen/Source/Building.cpp)
extern struct BuildingFacet building_facets[MAX_BUILDING_FACETS];

// uc_orig: building_objects (fallen/Source/Building.cpp)
extern struct BuildingObject building_objects[MAX_BUILDING_OBJECTS];

// uc_orig: build_mode (fallen/Source/Building.cpp)
extern SLONG build_mode;

// uc_orig: room_ids (fallen/Source/Building.cpp)
extern struct RoomID room_ids[MAX_INSIDE_STOREYS];

// uc_orig: next_inside (fallen/Source/Building.cpp)
extern SLONG next_inside;

// uc_orig: floor_texture_sizes (fallen/Source/Building.cpp)
extern UWORD floor_texture_sizes[];

// uc_orig: textures_xy (fallen/Source/Building.cpp)
extern struct TXTY textures_xy[200][5];

// uc_orig: dx_textures_xy (fallen/Source/Building.cpp)
extern struct DXTXTY dx_textures_xy[200][5];

// uc_orig: textures_flags (fallen/Source/Building.cpp)
extern UBYTE textures_flags[200][5];

// uc_orig: WindowCount (fallen/Source/Building.cpp)
extern SLONG WindowCount;

// uc_orig: block_min_x (fallen/Source/Building.cpp)
extern SLONG block_min_x;

// uc_orig: block_max_x (fallen/Source/Building.cpp)
extern SLONG block_max_x;

// uc_orig: Edge (fallen/Source/Building.cpp)
// Node in a per-scanline linked list of X-crossings used by the polygon rasterizer.
struct Edge {
    SWORD X;
    UBYTE Type;
    UBYTE Count;
    SWORD Next;
    SWORD Prev;
};

// uc_orig: edge_pool_ptr (fallen/Source/Building.cpp)
extern struct Edge* edge_pool_ptr;

// Internal edge-list scanner state — originally file-scope statics but needed by multiple chunks.
// uc_orig: edge_heads_ptr (fallen/Source/Building.cpp)
extern UWORD* edge_heads_ptr;

// uc_orig: next_edge (fallen/Source/Building.cpp)
extern ULONG next_edge;

// uc_orig: edge_min_z (fallen/Source/Building.cpp)
extern ULONG edge_min_z;

// uc_orig: flag_blocks (fallen/Source/Building.cpp)
extern SLONG* flag_blocks;

// uc_orig: flag_blocks2 (fallen/Source/Building.cpp)
extern SLONG* flag_blocks2;

// uc_orig: cut_blocks (fallen/Source/Building.cpp)
extern UWORD* cut_blocks;

// uc_orig: global_y (fallen/Source/Building.cpp)
extern SLONG global_y;

// uc_orig: build_seed (fallen/Source/Building.cpp)
// LCG seed for build_rand(). Starts at 0x12345678.
extern SLONG build_seed;

// Build-time position state (current building being processed).
// Originally file-scope statics in Building.cpp; exposed for multi-chunk access.
// uc_orig: build_x (fallen/Source/Building.cpp)
extern SLONG build_x;

// uc_orig: build_y (fallen/Source/Building.cpp)
extern SLONG build_y;

// uc_orig: build_z (fallen/Source/Building.cpp)
extern SLONG build_z;

// uc_orig: build_min_y (fallen/Source/Building.cpp)
extern SLONG build_min_y;

// uc_orig: build_max_y (fallen/Source/Building.cpp)
extern SLONG build_max_y;

// Internal macros shared between chunks
// uc_orig: FLOOR_LADDER (fallen/Source/Building.cpp)
#define FLOOR_LADDER (1 << 6)

// uc_orig: MAX_BOUND_SIZE (fallen/Source/Building.cpp)
#define MAX_BOUND_SIZE (200)

// uc_orig: SORT_LEVEL_LONG_LEDGE (fallen/Source/Building.cpp)
#define SORT_LEVEL_LONG_LEDGE 1

// uc_orig: SORT_LEVEL_FIRE_ESCAPE (fallen/Source/Building.cpp)
#define SORT_LEVEL_FIRE_ESCAPE 3

// uc_orig: ALT_SHIFT (fallen/Source/Building.cpp)
#define ALT_SHIFT (3)

// edge type flags used by building scanner
// uc_orig: SIDEWAY_EDGE (fallen/Source/Building.cpp)
#define SIDEWAY_EDGE (1)

// uc_orig: NORMAL_EDGE (fallen/Source/Building.cpp)
#define NORMAL_EDGE (2)

// uc_orig: CUT_BLOCK_TOP (fallen/Source/Building.cpp)
#define CUT_BLOCK_TOP (0)

// uc_orig: CUT_BLOCK_BOTTOM (fallen/Source/Building.cpp)
#define CUT_BLOCK_BOTTOM (1)

// uc_orig: CUT_BLOCK_LEFT (fallen/Source/Building.cpp)
#define CUT_BLOCK_LEFT (2)

// uc_orig: CUT_BLOCK_RIGHT (fallen/Source/Building.cpp)
#define CUT_BLOCK_RIGHT (3)

// uc_orig: INSIDE (fallen/Source/Building.cpp)
#define BUILDING_INSIDE (0)

// uc_orig: ON_EDGE (fallen/Source/Building.cpp)
#define BUILDING_ON_EDGE (1)

// uc_orig: OUTSIDE (fallen/Source/Building.cpp)
#define BUILDING_OUTSIDE (2)

// uc_orig: BOUNDS (fallen/Source/Building.cpp)
#define BOUNDS(x, z)   \
    if (x < min_x)     \
        min_x = x;     \
    if (x > max_x)     \
        max_x = x;     \
    if (z < min_z)     \
        min_z = z;     \
    if (z > max_z)     \
        max_z = z;

// Rotation encoding packed into high bits of lookup_roof[] entries.
// uc_orig: ROT1 (fallen/Source/Building.cpp)
#define ROT1 (3 << 8)

// uc_orig: ROT2 (fallen/Source/Building.cpp)
#define ROT2 (2 << 8)

// uc_orig: ROT3 (fallen/Source/Building.cpp)
#define ROT3 (1 << 8)

// Roof tile auto-tiling lookup table.
// Indexed by a bitmask of 8 surrounding tile occupancy (256 entries).
// Low 8 bits = texture atlas index; high 2 bits = rotation (see ROT1/ROT2/ROT3).
// uc_orig: lookup_roof (fallen/Source/Building.cpp)
extern UWORD lookup_roof[256];

// Index of the building currently being processed during geometry construction.
// uc_orig: current_building (fallen/Source/Building.cpp)
extern SLONG current_building;

// PSX-style UV coordinate lookup table for create_a_quad / create_a_tri.
// Indexed by texture_piece when texture_style==0. Entries are (Page, Tx, Ty).
// uc_orig: texture_xy2 (fallen/Source/Building.cpp)
extern struct TXTY texture_xy2[];

// Fire escape face connection table: relative face offsets for each face slot.
// Indexed via id_offset[] to walk the face chain for a fire escape section.
// uc_orig: face_offsets (fallen/Source/Building.cpp)
extern SWORD face_offsets[];

// Maximum dimension for the flat-fill grid used by flat_fill_a_quad_of_points().
// uc_orig: MAX_SIZE (fallen/Source/Building.cpp)
#define MAX_SIZE 20

// Scratch grid for quad point index mapping (MAX_SIZE x MAX_SIZE).
// uc_orig: sp (fallen/Source/Building.cpp)
extern SLONG building_sp[MAX_SIZE][MAX_SIZE];

// Scratch X/Z coordinate arrays for the flat-fill grid rows/columns.
// uc_orig: xp (fallen/Source/Building.cpp)
extern SLONG building_xp[MAX_SIZE];

// uc_orig: zp (fallen/Source/Building.cpp)
extern SLONG building_zp[MAX_SIZE];

// Scratch buffers for build_skylight: wall outline X/Z and strip counts.
// uc_orig: sx (fallen/Source/Building.cpp)
extern SLONG building_sx[200];

// uc_orig: sz (fallen/Source/Building.cpp)
extern SLONG building_sz[200];

// uc_orig: numb (fallen/Source/Building.cpp)
extern SLONG building_numb[200];

// Scratch X/Z buffers for build_ledge2 outline generation.
// uc_orig: sx_l2 (fallen/Source/Building.cpp)
extern SLONG building_sx_l2[30];

// uc_orig: sz_l2 (fallen/Source/Building.cpp)
extern SLONG building_sz_l2[30];

// Per-slot start indices into face_offsets[] for each fire escape segment ID (0–5).
// uc_orig: id_offset (fallen/Source/Building.cpp)
extern UWORD id_offset[];

// Face type flag for fire escape faces — used by next_connected_face().
// uc_orig: FACE_TYPE_FIRE_ESCAPE (fallen/Source/Building.cpp)
#define FACE_TYPE_FIRE_ESCAPE (1 << 0)

// Scratch buffer of prim_point row start indices for staircase geometry.
// Entry [99] holds the start of the staircase side-overlay points.
// uc_orig: sp_stairs (fallen/Source/Building.cpp)
extern SLONG sp_stairs[300];

#endif // WORLD_ENVIRONMENT_BUILDING_GLOBALS_H
