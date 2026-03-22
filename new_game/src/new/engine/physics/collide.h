#ifndef ENGINE_PHYSICS_COLLIDE_H
#define ENGINE_PHYSICS_COLLIDE_H

// Temporary: Game.h provides THING_INDEX and other base types needed by fallen/Headers/collide.h
#include "Game.h"
#include "fallen/Headers/collide.h"
#include "engine/physics/collide_globals.h"

// ========================================================================
// Geometry math — 2D line and segment utilities
// ========================================================================

// uc_orig: two4_line_intersection (fallen/Source/collide.cpp)
// Returns 0 (no intersection), 1 (collinear), or 2 (proper intersection)
// for the 2D segment pair (x1,y1)-(x2,y2) and (x3,y3)-(x4,y4).
UBYTE two4_line_intersection(SLONG x1, SLONG y1, SLONG x2, SLONG y2, SLONG x3, SLONG y3, SLONG x4, SLONG y4);

// uc_orig: clear_all_col_info (fallen/Source/collide.cpp)
// Stub — was used to reset debug collision info. No-op in this build.
void clear_all_col_info(void);

// uc_orig: get_height_along_vect (fallen/Source/collide.cpp)
// Interpolates Y at position (ax,az) along a CollisionVect segment.
SLONG get_height_along_vect(SLONG ax, SLONG az, struct CollisionVect* p_vect);

// uc_orig: get_height_along_facet (fallen/Source/collide.cpp)
// Interpolates Y at position (ax,az) along a DFacet segment.
SLONG get_height_along_facet(SLONG ax, SLONG az, struct DFacet* p_facet);

// uc_orig: check_big_point_triangle_col (fallen/Source/collide.cpp)
// Returns 1 if 2D point (x,y) lies inside the triangle (ux,uy)-(vx,vy)-(wx,wy), 0 otherwise.
UBYTE check_big_point_triangle_col(SLONG x, SLONG y, SLONG ux, SLONG uy, SLONG vx, SLONG vy, SLONG wx, SLONG wy);

// uc_orig: point_in_quad_old (fallen/Source/collide.cpp)
// Returns non-zero if (px,pz) lies within the prim quad face at (x,y,z).
SLONG point_in_quad_old(SLONG px, SLONG pz, SLONG x, SLONG y, SLONG z, SWORD face);

// uc_orig: dist_to_line (fallen/Source/collide.cpp)
// Returns distance from point (a,b) to the nearest point on segment (x1,z1)-(x2,z2).
// Also sets global dprod and cprod as side effects.
SLONG dist_to_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b);

// uc_orig: which_side (fallen/Source/collide.cpp)
// Returns the cross product sign of (a,b) relative to directed segment (x1,z1)-(x2,z2).
// Negative = right side, positive = left side.
SLONG which_side(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b);

// uc_orig: calc_along_vect (fallen/Source/collide.cpp)
// Returns how far along a DFacet segment the point (ax,az) projects, in 8.8 fixed point.
SLONG calc_along_vect(SLONG ax, SLONG az, struct DFacet* p_vect);

// uc_orig: signed_dist_to_line_with_normal (fallen/Source/collide.cpp)
// Signed distance from (a,b) to segment (x1,z1)-(x2,z2); returns outward normal vector
// and whether the projected point lies on the segment (*on).
void signed_dist_to_line_with_normal(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b,
    SLONG* dist, SLONG* vec_x, SLONG* vec_z, SLONG* on);

// uc_orig: signed_dist_to_line_with_normal_mark (fallen/Source/collide.cpp)
// Like signed_dist_to_line_with_normal but returns the endpoint-to-point vector when
// the projection falls off the segment end (instead of the segment normal).
void signed_dist_to_line_with_normal_mark(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b,
    SLONG* dist, SLONG* vec_x, SLONG* vec_z, SLONG* on);

// uc_orig: nearest_point_on_line (fallen/Source/collide.cpp)
// Finds the nearest point on segment (x1,z1)-(x2,z2) to (a,b).
void nearest_point_on_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_z);

// uc_orig: nearest_point_on_line_and_dist (fallen/Source/collide.cpp)
// Nearest point on segment to (a,b); returns distance. Sets global_on to 1 if on-segment.
SLONG nearest_point_on_line_and_dist(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_z);

// uc_orig: nearest_point_on_line_and_dist_calc_y (fallen/Source/collide.cpp)
// Like nearest_point_on_line_and_dist but also interpolates Y along the 3D segment.
SLONG nearest_point_on_line_and_dist_calc_y(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2,
    SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_y, SLONG* ret_z);

// uc_orig: distance_to_line (fallen/Source/collide.cpp)
// Wrapper: returns distance from (a,b) to nearest point on segment (x1,z1)-(x2,z2).
SLONG distance_to_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b);

// uc_orig: nearest_point_on_line_and_dist_and_along (fallen/Source/collide.cpp)
// Like nearest_point_on_line_and_dist but also returns *ret_along (0..255 along the segment).
SLONG nearest_point_on_line_and_dist_and_along(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b,
    SLONG* ret_x, SLONG* ret_z, SLONG* ret_along);

// ========================================================================
// Height queries
// ========================================================================

// uc_orig: calc_height_at (fallen/Source/collide.cpp)
// Returns ground height at (x,z). Stub — body commented out in original; returns 0.
SLONG calc_height_at(SLONG x, SLONG z);

// ========================================================================
// Collision layer classification
// ========================================================================

// uc_orig: collision_storey (fallen/Source/collide.cpp)
// Returns 1 if the storey type participates in collision detection.
SLONG collision_storey(SLONG type);

// ========================================================================
// Debug visualisation (no-op in release, early return on entry)
// ========================================================================

// uc_orig: highlight_face (fallen/Source/collide.cpp)
// Debug: draws wireframe of a prim quad face. Always returns early.
void highlight_face(SLONG face);

// uc_orig: highlight_rface (fallen/Source/collide.cpp)
// Debug: draws wireframe of a roof face. Always returns early.
void highlight_rface(SLONG rface);

// uc_orig: highlight_quad (fallen/Source/collide.cpp)
// Debug: draws wireframe of a quad at a given world position. Always returns early.
void highlight_quad(SLONG face, SLONG face_x, SLONG face_y, SLONG face_z);

// uc_orig: vect_intersect_wall (fallen/Source/collide.cpp)
// Stub — was intended to find walls intersecting a vector. Returns 0.
SLONG vect_intersect_wall(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);

#endif // ENGINE_PHYSICS_COLLIDE_H
