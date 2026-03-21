#ifndef WORLD_NAVIGATION_WALKABLE_H
#define WORLD_NAVIGATION_WALKABLE_H

// Walkable surface queries: finding and measuring surfaces that characters can stand on.
// A "walkable face" is either a DFacet (building roof/stairs, positive index) or a
// RoofFace4 (procedural roof quad, negative index). The PAP heightfield is the fallback.
// All walkable faces are treated as 10% larger than their real geometry for collision purposes.

#include "core/types.h"

// Aliases for calc_height_on_face; maps to get_height_on_face_quad64_at.
// uc_orig: calc_height_on_face (fallen/Headers/walkable.h)
#define calc_height_on_face(x, z, face, new_y) get_height_on_face_quad64_at(x, z, face, new_y)

// Flags for the ignore_height_flag parameter of find_face_for_this_pos.
// uc_orig: FIND_ANYFACE (fallen/Headers/walkable.h)
#define FIND_ANYFACE 1
// uc_orig: FIND_FACE_BELOW (fallen/Headers/walkable.h)
#define FIND_FACE_BELOW 2
// uc_orig: FIND_FACE_NEAR_BELOW (fallen/Headers/walkable.h)
#define FIND_FACE_NEAR_BELOW 3

// Returns true (1) if point (px,pz) is inside the quad defined by prim_faces4[face],
// placed at world offset (x,y,z). Quads are tested as 10% larger for robustness.
// uc_orig: point_in_quad (fallen/Source/walkable.cpp)
SLONG point_in_quad(SLONG px, SLONG pz, SLONG x, SLONG y, SLONG z, SWORD face);

// Returns whether (rx,rz) projects onto prim_faces4[face], and fills *height with the
// interpolated Y at that point. Uses 16.16 fixed-point barycentric math.
// uc_orig: get_height_on_face_quad64_at (fallen/Source/walkable.cpp)
SLONG get_height_on_face_quad64_at(SLONG rx, SLONG rz, SWORD face, SLONG* height);

// Returns the face index that (x,z) maps to, and fills *ret_face with that index.
// uc_orig: find_height_for_this_pos (fallen/Source/walkable.cpp)
SLONG find_height_for_this_pos(SLONG x, SLONG z, SLONG* ret_face);

// Returns true (1) if (x,z) is within the walkable extent of prim/roof face at index.
// uc_orig: is_thing_on_this_quad (fallen/Source/walkable.cpp)
SLONG is_thing_on_this_quad(SLONG x, SLONG z, SLONG face);

// Computes the interpolated height on a RoofFace4 at (x,z) and stores in *ret_y.
// Returns the face index if on the face, 0 otherwise.
// uc_orig: calc_height_on_rface (fallen/Source/walkable.cpp)
SLONG calc_height_on_rface(SLONG x, SLONG z, SWORD face, SLONG* new_y);

// Main surface-finding function. Searches for a walkable face beneath (x,y,z).
// Returns face index > 0 (DFacet), < 0 (RoofFace4), GRAB_FLOOR (terrain), or 0 (nothing).
// ignore_height_flag: 0=standard, FIND_ANYFACE=1, FIND_FACE_NEAR_BELOW=3.
// uc_orig: find_face_for_this_pos (fallen/Source/walkable.cpp)
SLONG find_face_for_this_pos(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG* ret_y,
    SLONG ignore_building,
    UBYTE ignore_height_flag);

// Returns the slope steepness of a RoofFace4 at (x,z) and stores the downslope angle in *angle.
// uc_orig: RFACE_on_slope (fallen/Source/walkable.cpp)
SLONG RFACE_on_slope(SLONG face, SLONG x, SLONG z, SLONG* angle);

// Removes the roof face above mapsquare (map_x, map_z) from the walkable list.
// uc_orig: WALKABLE_remove_rface (fallen/Source/walkable.cpp)
void WALKABLE_remove_rface(UBYTE map_x, UBYTE map_z);

#endif // WORLD_NAVIGATION_WALKABLE_H
