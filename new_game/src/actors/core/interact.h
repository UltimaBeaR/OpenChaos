#ifndef ACTORS_CORE_INTERACT_H
#define ACTORS_CORE_INTERACT_H

#include "world/environment/building_types.h" // CABLE_ALONG_*, FACET_FLAG_*, STOREY_TYPE_*
#include "world/map/supermap.h"               // DFacet (used in function params)
#include "actors/core/thing.h"
#include "actors/core/thing_globals.h"
#include "assets/anim_globals.h"

// Returns fractional position along a cable facet for world XZ point.
// Result in CABLE_ALONG_SHIFT fixed-point units.
// uc_orig: get_cable_along (fallen/Source/interact.cpp)
SLONG get_cable_along(SLONG facet, SLONG ax, SLONG az);

// Returns the Y height of a cable facet at a given position along it (0..CABLE_ALONG_MAX).
// uc_orig: find_cable_y_along (fallen/Source/interact.cpp)
SLONG find_cable_y_along(struct DFacet* p_facet, SLONG along);

// Finds the nearest valid grab point on a cable facet.
// Returns 1 if a valid grab was found, 0 otherwise.
// uc_orig: check_grab_cable_facet (fallen/Source/interact.cpp)
SLONG check_grab_cable_facet(SLONG facet, SLONG* grab_x, SLONG* grab_y, SLONG* grab_z, SLONG* grab_angle, SLONG radius, SLONG dy, SLONG x, SLONG y, SLONG z);

// Finds the nearest valid grab point on a ladder facet.
// Returns 1 if found, -1 if above the ladder (still falling), 0 otherwise.
// uc_orig: check_grab_ladder_facet (fallen/Source/interact.cpp)
SLONG check_grab_ladder_facet(SLONG facet, SLONG* grab_x, SLONG* grab_y, SLONG* grab_z, SLONG* grab_angle, SLONG radius, SLONG dy, SLONG x, SLONG y, SLONG z);

// Searches for a grabbable surface (edge, cable, or ladder) near the given world position.
// Returns 1 if a grab point was found. *type: 0=surface, 1=cable, 2=ladder.
// uc_orig: find_grab_face (fallen/Source/interact.cpp)
SLONG find_grab_face(
    SLONG x, SLONG y, SLONG z,
    SLONG radius, SLONG dy, SLONG angle,
    SLONG* grab_x, SLONG* grab_y, SLONG* grab_z,
    SLONG* grab_angle,
    SLONG ignore_faces_from_this_building,
    SLONG trench,
    SLONG* type,
    Thing* p_person);

// Simplified grab search for sewer trench areas (4 adjacent map squares).
// uc_orig: find_grab_face_in_sewers (fallen/Source/interact.cpp)
SLONG find_grab_face_in_sewers(
    SLONG x, SLONG y, SLONG z,
    SLONG radius, SLONG dy, SLONG angle,
    SLONG* grab_x, SLONG* grab_y, SLONG* grab_z,
    SLONG* grab_angle);

// Computes the world-space position of a bone attachment point for a Thing.
// Uses tween interpolation between current and next animation frame.
// uc_orig: calc_sub_objects_position (fallen/Source/interact.cpp)
void calc_sub_objects_position(Thing* p_mthing, SLONG tween, UWORD object, SLONG* x, SLONG* y, SLONG* z);

// Variant with fixed-point 8-bit precision output.
// uc_orig: calc_sub_objects_position_fix8 (fallen/Source/interact.cpp)
void calc_sub_objects_position_fix8(Thing* p_mthing, SLONG tween, UWORD object, SLONG* x, SLONG* y, SLONG* z);

// Variant that takes explicit frame pointers instead of reading from Thing.
// uc_orig: calc_sub_objects_position_keys (fallen/Source/interact.cpp)
void calc_sub_objects_position_keys(Thing* p_mthing, SLONG tween, UWORD object, SLONG* x, SLONG* y, SLONG* z, struct GameKeyFrame* frame1, struct GameKeyFrame* frame2);

// Variant that does not reference a Thing at all — takes two keyframes directly.
// uc_orig: calc_sub_objects_position_global (fallen/Source/interact.cpp)
void calc_sub_objects_position_global(GameKeyFrame* cur_frame, GameKeyFrame* next_frame, SLONG tween, UWORD object, SLONG* x, SLONG* y, SLONG* z);

#endif // ACTORS_CORE_INTERACT_H
