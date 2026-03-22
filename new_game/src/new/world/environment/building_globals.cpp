#include "world/environment/building_globals.h"

// uc_orig: edit_map (fallen/Source/Building.cpp)
// Editor-only tile map (BUILD_MODE_EDITOR). In BUILD_MODE_DX the PAP supermap is used instead.
struct DepthStrip edit_map[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH]; // 2MB

// uc_orig: tex_map (fallen/Source/Building.cpp)
// Editor-only texture override map. If non-zero, overrides edit_map texture for that tile.
UWORD tex_map[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];

// uc_orig: edit_map_roof_height (fallen/Source/Building.cpp)
SBYTE edit_map_roof_height[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];

// uc_orig: start_point (fallen/Source/Building.cpp)
SLONG start_point[200];

// uc_orig: next_roof_bound (fallen/Source/Building.cpp)
// Already declared extern in fallen/Headers/building.h; defined here.
UWORD next_roof_bound = 1;

// uc_orig: background_prim (fallen/Source/Building.cpp)
UWORD background_prim = 0;

// uc_orig: end_prim_point (fallen/Source/Building.cpp)
// Initialized at build time to MAX_PRIM_POINTS - 2 after save_table is set up.
UWORD end_prim_point = 0;

// uc_orig: end_prim_face4 (fallen/Source/Building.cpp)
UWORD end_prim_face4 = 0;

// uc_orig: end_prim_face3 (fallen/Source/Building.cpp)
UWORD end_prim_face3 = MAX_PRIM_FACES3 - 2;

// uc_orig: end_prim_object (fallen/Source/Building.cpp)
UWORD end_prim_object = MAX_PRIM_OBJECTS - 2;

// uc_orig: end_prim_multi_object (fallen/Source/Building.cpp)
UWORD end_prim_multi_object = MAX_PRIM_MOBJECTS - 2;

// uc_orig: next_building_object (fallen/Source/Building.cpp)
UWORD next_building_object = 1;

// uc_orig: end_building_object (fallen/Source/Building.cpp)
UWORD end_building_object = MAX_BUILDING_OBJECTS - 2;

// uc_orig: next_building_facet (fallen/Source/Building.cpp)
UWORD next_building_facet = 1;

// uc_orig: end_building_facet (fallen/Source/Building.cpp)
UWORD end_building_facet = MAX_BUILDING_FACETS - 2;

// uc_orig: diff_page_count1 (fallen/Source/Building.cpp)
UWORD diff_page_count1 = 0;

// uc_orig: diff_page_count2 (fallen/Source/Building.cpp)
UWORD diff_page_count2 = 0;

// uc_orig: page_count (fallen/Source/Building.cpp)
UWORD page_count[64 * 8];

// uc_orig: window_list (fallen/Source/Building.cpp)
struct FWindow window_list[MAX_WINDOWS];

// uc_orig: wall_list (fallen/Source/Building.cpp)
struct FWall wall_list[MAX_WALLS];

// uc_orig: storey_list (fallen/Source/Building.cpp)
struct FStorey storey_list[MAX_STOREYS];

// uc_orig: building_list (fallen/Source/Building.cpp)
struct FBuilding building_list[MAX_BUILDINGS];

// uc_orig: building_facets (fallen/Source/Building.cpp)
struct BuildingFacet building_facets[MAX_BUILDING_FACETS];

// uc_orig: building_objects (fallen/Source/Building.cpp)
struct BuildingObject building_objects[MAX_BUILDING_OBJECTS];

// uc_orig: build_mode (fallen/Source/Building.cpp)
// Controls which data source to use: BUILD_MODE_EDITOR or BUILD_MODE_DX.
SLONG build_mode = 1;

// uc_orig: room_ids (fallen/Source/Building.cpp)
struct RoomID room_ids[MAX_INSIDE_STOREYS];

// uc_orig: next_inside (fallen/Source/Building.cpp)
// Next free slot in room_ids[]. Starts at 1 (0 = invalid/null).
SLONG next_inside = 1;

// uc_orig: floor_texture_sizes (fallen/Source/Building.cpp)
UWORD floor_texture_sizes[] = {
    16, 32, 64, 128
};

// uc_orig: textures_xy (fallen/Source/Building.cpp)
struct TXTY textures_xy[200][5];

// uc_orig: dx_textures_xy (fallen/Source/Building.cpp)
struct DXTXTY dx_textures_xy[200][5];

// uc_orig: textures_flags (fallen/Source/Building.cpp)
UBYTE textures_flags[200][5];

// uc_orig: WindowCount (fallen/Source/Building.cpp)
SLONG WindowCount;

// uc_orig: block_min_x (fallen/Source/Building.cpp)
SLONG block_min_x;

// uc_orig: block_max_x (fallen/Source/Building.cpp)
SLONG block_max_x;

// uc_orig: edge_pool_ptr (fallen/Source/Building.cpp)
struct Edge* edge_pool_ptr = 0;

// uc_orig: build_x (fallen/Source/Building.cpp)
SLONG build_x;

// uc_orig: build_y (fallen/Source/Building.cpp)
SLONG build_y;

// uc_orig: build_z (fallen/Source/Building.cpp)
SLONG build_z;

// uc_orig: build_min_y (fallen/Source/Building.cpp)
SLONG build_min_y;

// uc_orig: build_max_y (fallen/Source/Building.cpp)
SLONG build_max_y;

// Internal edge-list scanner state — originally file-scope statics in Building.cpp.
// Exposed here (non-static) so that all chunks (building.cpp + old/Building.cpp) can share them.

// uc_orig: edge_heads_ptr (fallen/Source/Building.cpp)
UWORD* edge_heads_ptr = 0;

// uc_orig: next_edge (fallen/Source/Building.cpp)
ULONG next_edge;

// uc_orig: edge_min_z (fallen/Source/Building.cpp)
ULONG edge_min_z;

// uc_orig: flag_blocks (fallen/Source/Building.cpp)
SLONG* flag_blocks = 0;

// uc_orig: flag_blocks2 (fallen/Source/Building.cpp)
SLONG* flag_blocks2 = 0;

// uc_orig: cut_blocks (fallen/Source/Building.cpp)
UWORD* cut_blocks = 0;

// uc_orig: global_y (fallen/Source/Building.cpp)
SLONG global_y;

// uc_orig: build_seed (fallen/Source/Building.cpp)
// LCG seed for the building geometry deterministic PRNG. Initially 0x12345678.
SLONG build_seed = 0x12345678;
