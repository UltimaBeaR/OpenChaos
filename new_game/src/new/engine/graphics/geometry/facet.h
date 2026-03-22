#ifndef ENGINE_GRAPHICS_GEOMETRY_FACET_H
#define ENGINE_GRAPHICS_GEOMETRY_FACET_H

#include "core/types.h"
#include "engine/graphics/pipeline/poly.h"    // POLY_Point
#include "engine/lighting/night.h"             // NIGHT_Colour
#include "engine/graphics/geometry/facet_globals.h"  // all facet globals (FacetRows, FacetDiffY, flip, etc.)
#include "fallen/Headers/building.h"           // Temporary: DFacet, TEXTURE_PIECE_*

// =====================================================================
// Exterior building wall/roof renderer.
//
// A "facet" is one wall segment of a building: from (x[0],z[0]) to
// (x[1],z[1]) and Height tiers tall. Facets are stored in a global
// DFacet array and indexed by integer.
//
// Main entry points called from aeng.cpp:
//   FACET_draw          — fast path for plain normal-storey walls
//   FACET_draw_rare     — all special wall types (doors, fences, cables)
//   FACET_draw_walkable — draws walkable roof geometry for a building
// =====================================================================

// uc_orig: grid_height_at_world (fallen/DDEngine/Source/facet.cpp)
// Returns terrain height at world-space (x, z). Used by chunk 2+ rendering.
float grid_height_at_world(float x, float z);

// uc_orig: facet_rand (fallen/DDEngine/Source/facet.cpp)
// Simple LCG pseudo-random generator used to vary wall texture tiles.
ULONG facet_rand(void);

// uc_orig: set_facet_seed (fallen/DDEngine/Source/facet.cpp)
// Resets the facet RNG seed, called before drawing each facet to ensure
// deterministic tile placement on repeated frames.
void set_facet_seed(SLONG seed);

// uc_orig: texture_quad (fallen/DDEngine/Source/facet.cpp)
// Selects the D3D texture page for one wall quad and writes u/v coordinates
// into the four POLY_Points. Sets the global `flip` as a side-effect.
// texture_style < 0: per-segment paint override; >= 0: standard tiled style.
SLONG texture_quad(POLY_Point* quad[4], SLONG texture_style, SLONG pos, SLONG count, SLONG flipx = 0);

// uc_orig: get_texture_page (fallen/DDEngine/Source/facet.cpp)
// Like texture_quad but only returns page and writes *rflip; does not touch u/v.
SLONG get_texture_page(SLONG texture_style, SLONG pos, SLONG count, UBYTE* rflip);

// uc_orig: texture_quad2 (fallen/DDEngine/Source/facet.cpp)
// Sets u/v for a quad using a direct texture_piece index (no positional choice).
SLONG texture_quad2(POLY_Point* quad[4], SLONG texture_style, SLONG texture_piece);

// uc_orig: texture_tri2 (fallen/DDEngine/Source/facet.cpp)
// Like texture_quad2 but for a triangle (3 points).
SLONG texture_tri2(POLY_Point* quad[3], SLONG texture_style, SLONG texture_piece);

// uc_orig: build_fence_poles (fallen/DDEngine/Source/facet.cpp)
// Draws the 3D fence post geometry along a wall segment as a series of
// small triangular prism columns.
void build_fence_poles(float sx, float sy, float sz, float fdx, float fdz, SLONG count, float* rdx, float* rdz, SLONG style);

// uc_orig: cable_draw (fallen/DDEngine/Source/facet.cpp)
// Draws a hanging cable/wire as billboard quads with catenary sag.
void cable_draw(struct DFacet* p_facet);

// uc_orig: DRAW_stairs (fallen/DDEngine/Source/facet.cpp)
// Draws a staircase prim for an indoor storey connection.
void DRAW_stairs(SLONG stair, SLONG storey, UBYTE fade);

// uc_orig: draw_insides (fallen/DDEngine/Source/facet.cpp)
// Draws all facets and stair prims for one indoor storey room.
void draw_insides(SLONG indoor_index, SLONG room, UBYTE fade);

// uc_orig: DRAW_door (fallen/DDEngine/Source/facet.cpp)
// Draws a 3-panel door as a set of quads.
void DRAW_door(float sx, float sy, float sz, float fx, float fy, float fz, float block_height, SLONG count, ULONG fade_alpha, SLONG style, SLONG flipx = 0);

// uc_orig: draw_wall_thickness (fallen/DDEngine/Source/facet.cpp)
// Draws the top horizontal cap quad of an inside-facing wall.
void draw_wall_thickness(struct DFacet* p_facet, ULONG fade_alpha);

// uc_orig: FACET_barbedwire_top (fallen/DDEngine/Source/facet.cpp)
// Draws barbed wire sprites along the top of a fence/wall facet.
void FACET_barbedwire_top(struct DFacet* p_facet);

// uc_orig: MakeFacetPoints (fallen/DDEngine/Source/facet.cpp)
// Transforms a wall segment into a 2D grid of screen-space POLY_buffer[] points.
void MakeFacetPoints(float sx, float sy, float sz, float dx, float dz, float block_height,
    SLONG height, SLONG max_height, NIGHT_Colour* col, SLONG foundation, SLONG count, SLONG invisible, SLONG hug);

// uc_orig: FillFacetPoints (fallen/DDEngine/Source/facet.cpp)
// Converts two consecutive rows in POLY_buffer[] into textured quads.
void FillFacetPoints(SLONG count, ULONG base_row, SLONG foundation, SLONG facet_backwards, SLONG style_index, float block_height, SLONG reverse_texture);

// uc_orig: MakeFacetPointsCommon (fallen/DDEngine/Source/facet.cpp)
// Optimised MakeFacetPoints for the common NORMAL storey path.
void MakeFacetPointsCommon(float sx, float sy, float sz, float dx, float dz, float block_height,
    SLONG height, NIGHT_Colour* col, SLONG foundation, SLONG count, SLONG hug);

// uc_orig: FillFacetPointsCommon (fallen/DDEngine/Source/facet.cpp)
// Optimised FillFacetPoints for the common non-reversed forward path.
void FillFacetPointsCommon(SLONG count, ULONG base_row, SLONG foundation, SLONG style_index, float block_height);

// uc_orig: FACET_draw (fallen/DDEngine/Source/facet.cpp)
// Fast-path renderer for STOREY_TYPE_NORMAL plain wall facets.
void FACET_draw(SLONG facet, UBYTE alpha);

// uc_orig: FACET_draw_rare (fallen/DDEngine/Source/facet.cpp)
// Full-featured renderer for special facet types: doors, fences, cables,
// barbed wire, multi-textured warehouse walls, etc.
void FACET_draw_rare(SLONG facet, UBYTE alpha);

// uc_orig: FACET_draw_quick (fallen/DDEngine/Source/facet.cpp)
// Alternate version of FACET_draw used from the SUPERFACET cache path.
void FACET_draw_quick(SLONG facet, UBYTE alpha);

// uc_orig: FACET_draw_walkable (fallen/DDEngine/Source/facet.cpp)
// Draws roof/walkable surface geometry for a building.
void FACET_draw_walkable(SLONG build);

// uc_orig: FACET_draw_walkable_old (fallen/DDEngine/Source/facet.cpp)
// Older walkable draw path (kept for reference).
void FACET_draw_walkable_old(SLONG build);

// uc_orig: DRAW_ladder (fallen/DDEngine/Source/facet.cpp)
// Draws a sewer ladder geometry.
void DRAW_ladder(struct DFacet* p_facet);

// uc_orig: FACET_project_crinkled_shadow (fallen/DDEngine/Source/facet.cpp)
// Projects a shadow onto a crinkle-bump-mapped facet surface.
void FACET_project_crinkled_shadow(SLONG facet);

// uc_orig: FACET_draw_ns_ladder (fallen/DDEngine/Headers/facet.h)
// Draws a sewer (NS) ladder by index and coordinate range.
void FACET_draw_ns_ladder(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG height);

#endif // ENGINE_GRAPHICS_GEOMETRY_FACET_H
