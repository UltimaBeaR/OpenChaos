#ifndef BUILDINGS_BUILD2_H
#define BUILDINGS_BUILD2_H

#include "engine/core/types.h"

// Build2: city spatial index construction, facet/walkable registration, ladder geometry.
// Functions here populate the PAP lo-res map with DFacet and walkable face
// linked lists used for collision detection and AI queries at runtime.

// Shrinks and offsets a ladder facet's endpoints for tighter collision detection.
// Called by add_facet_to_map for STOREY_TYPE_LADDER facets, and also by Building.cpp.
// uc_orig: calc_ladder_ends (fallen/Source/build2.cpp)
void calc_ladder_ends(SLONG* x1, SLONG* z1, SLONG* x2, SLONG* z2);

// Rebuilds the entire city collision structure from scratch.
// Clears ColVectHead/Walkable in PAP lo-res, runs mark_naughty_facets to cull
// invisible facets, then re-registers every facet and walkable face.
// Called after level load and after elevator/dynamic geometry changes.
// uc_orig: build_quick_city (fallen/Source/build2.cpp)
void build_quick_city(void);

// Registers a walkable face in the PAP lo-res Walkable linked list.
// Positive face index: prim_faces4[] quad; negative index: roof_faces4[-face] quad.
// uc_orig: attach_walkable_to_map (fallen/Source/build2.cpp)
void attach_walkable_to_map(SLONG face);

// Removes a walkable face from the PAP lo-res Walkable linked list.
// Only works for prim_faces4[] quads (positive face index).
// uc_orig: remove_walkable_from_map (fallen/Source/build2.cpp)
void remove_walkable_from_map(SLONG face);

// Registers a dfacet in the PAP lo-res ColVectHead linked list at each
// mapsquare it spans. Used for character and vehicle wall collision.
// uc_orig: add_facet_to_map (fallen/Source/build2.cpp)
void add_facet_to_map(SLONG dfacet);

// Removes DFacets that are completely hidden by adjacent solid facets
// and updates MAV car-navigation grid.
// Must be called after build_quick_city().
// uc_orig: BUILD_car_facets (fallen/Source/build2.cpp)
void BUILD_car_facets(void);

#endif // BUILDINGS_BUILD2_H
