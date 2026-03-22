#ifndef WORLD_ENVIRONMENT_BUILDING_H
#define WORLD_ENVIRONMENT_BUILDING_H

#include "fallen/Headers/building.h"   // Temporary: building types (FBuilding etc.), all public building API

// =====================================================================
// Building geometry construction system.
// Reads FBuilding/FStorey/FWall data and emits renderable geometry
// (prim_points[], prim_faces4[], prim_faces3[]) and walkability data.
//
// Most of the public API (place_building_at, create_city, etc.) is in
// fallen/Headers/building.h — this file adds chunk-specific helpers.
// =====================================================================

// Stub: registers a collision vector into the physics system.
// In this pre-release codebase the function does nothing (returns 0).
// The full game uses this to register wall/ramp/ladder geometry.
// uc_orig: insert_collision_vect (fallen/Source/Building.cpp)
SLONG insert_collision_vect(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2,
                             UBYTE prim_type, UBYTE prim_extra, SWORD face);

// Accumulates page usage statistics for texture atlases.
// uc_orig: add_page_countxy (fallen/Source/Building.cpp)
void add_page_countxy(SLONG tx, SLONG ty, SLONG page);

// Appends a roof bounding box to roof_bounds[] and returns its index.
// uc_orig: add_bound_box (fallen/Source/Building.cpp)
SLONG add_bound_box(UBYTE minx, UBYTE maxx, UBYTE minz, UBYTE maxz, SWORD y);

// Map abstraction layer: read/write map data routed through build_mode.
// In BUILD_MODE_DX routes to the PAP supermap; BUILD_MODE_EDITOR routes to edit_map[].

// uc_orig: get_map_walkable (fallen/Source/Building.cpp)
SLONG get_map_walkable(SLONG x, SLONG z);

// uc_orig: set_map_walkable (fallen/Source/Building.cpp)
void set_map_walkable(SLONG x, SLONG z, SLONG walkable);

// uc_orig: get_map_texture (fallen/Source/Building.cpp)
SLONG get_map_texture(SLONG x, SLONG z);

// uc_orig: set_map_texture (fallen/Source/Building.cpp)
void set_map_texture(SLONG x, SLONG z, SLONG texture);

// uc_orig: get_map_height (fallen/Source/Building.cpp)
SLONG get_map_height(SLONG x, SLONG z);

// uc_orig: get_roof_height (fallen/Source/Building.cpp)
SLONG get_roof_height(SLONG x, SLONG z);

// uc_orig: set_map_flag (fallen/Source/Building.cpp)
SLONG set_map_flag(SLONG x, SLONG z, SLONG flag);

// uc_orig: mask_map_flag (fallen/Source/Building.cpp)
SLONG mask_map_flag(SLONG x, SLONG z, SLONG flag);

// uc_orig: get_map_flags (fallen/Source/Building.cpp)
SLONG get_map_flags(SLONG x, SLONG z);

// uc_orig: set_map_height (fallen/Source/Building.cpp)
void set_map_height(SLONG x, SLONG z, SLONG y);

// Returns 1 if (x,z) is within the current map boundaries.
// uc_orig: in_map_range (fallen/Source/Building.cpp)
SLONG in_map_range(SLONG x, SLONG z);

// Places a game Thing for a tile coordinate (editor: MapThing, DX: Thing with CLASS_BUILDING).
// uc_orig: place_thing_on_map (fallen/Source/Building.cpp)
void place_thing_on_map(SLONG x, SLONG z, SLONG thing);

// Sets the floor height at map tiles along the line from (x1,z1) to (x2,z2).
// uc_orig: set_vect_floor_height (fallen/Source/Building.cpp)
void set_vect_floor_height(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG y);

// Deterministic PRNG for building geometry generation.
// uc_orig: build_rand (fallen/Source/Building.cpp)
SLONG build_rand(void);

// uc_orig: set_build_seed (fallen/Source/Building.cpp)
void set_build_seed(SLONG seed);

// Registers a face as a walkable surface for the map tile grid.
// uc_orig: add_walk_face_to_map (fallen/Source/Building.cpp)
void add_walk_face_to_map(SWORD face, SLONG x, SLONG z);

// Rasterizes a triangle into walkable map tiles via add_walk_face_to_map().
// uc_orig: scan_walk_triangle (fallen/Source/Building.cpp)
void scan_walk_triangle(SLONG x0, SLONG y0, SLONG z0,
                        SLONG x1, SLONG y1, SLONG z1,
                        SLONG x2, SLONG y2, SLONG z2, SLONG face);

// Registers a quad face as walkable by rasterizing its two triangles.
// uc_orig: add_quad_to_walkable_list (fallen/Source/Building.cpp)
void add_quad_to_walkable_list(SWORD face);

// Stub: tri walkable registration (body is commented out in original).
// uc_orig: add_tri_to_walkable_list (fallen/Source/Building.cpp)
void add_tri_to_walkable_list(SWORD face);

// Appends a vertex to prim_points[] (integer) and AENG_dx_prim_points[] (float).
// uc_orig: add_point (fallen/Source/Building.cpp)
void add_point(SLONG x, SLONG y, SLONG z);

// Wall point generators: produce evenly-spaced points along a wall segment.
// uc_orig: build_row_wall_points_at_y (fallen/Source/Building.cpp)
SLONG build_row_wall_points_at_y(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall);

// uc_orig: build_row_wall_points_at_floor_alt (fallen/Source/Building.cpp)
SLONG build_row_wall_points_at_floor_alt(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall);

// uc_orig: build_row_wall_only_points_at_y (fallen/Source/Building.cpp)
SLONG build_row_wall_only_points_at_y(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall);

// uc_orig: build_row_wall_only_points_at_floor_alt (fallen/Source/Building.cpp)
SLONG build_row_wall_only_points_at_floor_alt(SLONG dy, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall);

// uc_orig: build_row_window_depth_points_at_y (fallen/Source/Building.cpp)
SLONG build_row_window_depth_points_at_y(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall);

// Scanline edge-list helpers for roof polygon rasterization.
// uc_orig: set_cut_blocks (fallen/Source/Building.cpp)
void set_cut_blocks(SLONG x, SLONG z);

// uc_orig: set_cut_blocks_z (fallen/Source/Building.cpp)
void set_cut_blocks_z(SLONG x, SLONG z);

// uc_orig: scan_line_z (fallen/Source/Building.cpp)
void scan_line_z(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG flag);

// uc_orig: scan_line (fallen/Source/Building.cpp)
UBYTE scan_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG flag);

// uc_orig: build_edge_list (fallen/Source/Building.cpp)
SLONG build_edge_list(SLONG storey, SLONG flag);

// uc_orig: build_more_edge_list (fallen/Source/Building.cpp)
void build_more_edge_list(SLONG min_z, SLONG max_z, SLONG storey, SLONG flag);

// uc_orig: scan_bottom_line (fallen/Source/Building.cpp)
void scan_bottom_line(SLONG x, SLONG z, SLONG x2, SLONG y);

// uc_orig: build_bottom_edge_list (fallen/Source/Building.cpp)
void build_bottom_edge_list(SLONG storey, SLONG y);

// Frees all edge-list memory pools allocated by build_edge_list().
// uc_orig: bin_edge_list (fallen/Source/Building.cpp)
void bin_edge_list(void);

// Dumps edge-list to debug output (no-op in release builds).
// uc_orig: dump_edge_list (fallen/Source/Building.cpp)
void dump_edge_list(UWORD size);

// UV texture mapping for free-standing (non-wall) triangle and quad geometry.
// Both use the texture at map tile (mx, mz) and apply rotation.
// uc_orig: build_free_tri_texture_info (fallen/Source/Building.cpp)
void build_free_tri_texture_info(struct PrimFace3* p_f3, SLONG mx, SLONG mz);

// uc_orig: build_free_quad_texture_info (fallen/Source/Building.cpp)
void build_free_quad_texture_info(struct PrimFace4* p_f4, SLONG mx, SLONG mz);

// Creates triangle faces along a 45-degree diagonal section of a roof.
// uc_orig: scan_45 (fallen/Source/Building.cpp)
void scan_45(SLONG x1, SLONG z1, SLONG dx, SLONG dz);

// Builds a decorative parapet/rim at the top of a storey roof.
// Returns the new Y after the ledge is built.
// uc_orig: build_storey_lip (fallen/Source/Building.cpp)
SLONG build_storey_lip(SLONG storey, SLONG y);

// Registers a rectangular roof or ledge as a walkable platform.
// uc_orig: create_walkable_structure (fallen/Source/Building.cpp)
void create_walkable_structure(SLONG left, SLONG right, SLONG top, SLONG bottom, SLONG y, SLONG sp, SLONG sf4, SLONG dy);

// Sets FACE_FLAG_OTHER_SPLIT on a quad if it is non-planar (vertices at different heights).
// uc_orig: calc_face_split (fallen/Source/Building.cpp)
void calc_face_split(struct PrimFace4* p_f4);

// Builds flat or gently sloped roof geometry using the scanline edge list.
// Returns a bounding box handle, or 0 if no valid roof tiles were found.
// uc_orig: build_easy_roof (fallen/Source/Building.cpp)
SLONG build_easy_roof(SLONG min_x, SLONG edge_min_z, SLONG max_x, SLONG depth, SLONG y, SLONG face_wall, SLONG flag);

#endif // WORLD_ENVIRONMENT_BUILDING_H
