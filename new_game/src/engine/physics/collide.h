#ifndef ENGINE_PHYSICS_COLLIDE_H
#define ENGINE_PHYSICS_COLLIDE_H

#include "engine/core/types.h"
// Note: engine/ → world/ is a DAG violation, but PAP cell data is required by collision
// detection throughout the engine. This dependency is accepted as known tech debt.
#include "world/map/pap.h"
#include "world/map/pap_globals.h"

// Forward declarations — full definitions in their respective headers.
// Consumers that need globals should also include engine/physics/collide_globals.h.
struct Thing;
struct DFacet;

// ========================================================================
// Pool size limits
// ========================================================================

// uc_orig: MAX_COL_VECT_LINK (fallen/Headers/collide.h)
// Maximum number of collision vector links (per-cell linked list nodes).
#define MAX_COL_VECT_LINK 10000

// uc_orig: MAX_COL_VECT (fallen/Headers/collide.h)
// Maximum number of linear collision barriers stored in the level.
#define MAX_COL_VECT 10000

// uc_orig: MAX_WALK_POOL (fallen/Headers/collide.h)
// Maximum number of walkable surface records (PAP LO-cell linked lists).
#define MAX_WALK_POOL (30000)

// uc_orig: DONT_INTERSECT (fallen/Headers/collide.h)
#define DONT_INTERSECT 0
// uc_orig: DO_INTERSECT (fallen/Headers/collide.h)
#define DO_INTERSECT 1
// uc_orig: COLLINEAR (fallen/Headers/collide.h)
#define COLLINEAR 2

// ========================================================================
// Collision barrier data structures
// ========================================================================

// uc_orig: CollisionVectLink (fallen/Headers/collide.h)
// Links a collision barrier to a LO-res PAP cell (4 bytes). Stored in col_vects_links[].
struct CollisionVectLink {
    UWORD Next;       // next entry in the linked list for this cell (0 = end)
    UWORD VectIndex;  // index into col_vects[]
};

// uc_orig: CollisionVect (fallen/Headers/collide.h)
// A linear collision barrier in 3D (wall, fence, staircase, etc.). ~20 bytes.
// Stored in col_vects[MAX_COL_VECT].
struct CollisionVect {
    SLONG X[2];       // world X of barrier start/end
    SWORD Y[2];       // bottom/top height of barrier
    SLONG Z[2];       // world Z of barrier start/end
    UBYTE PrimType;   // storey type (wall, fence, ladder, etc.)
    UBYTE PrimExtra;  // extra parameter depending on PrimType
    SWORD Face;       // DFacet index, or -1 if not tied to a facet
};

// uc_orig: WalkLink (fallen/Headers/collide.h)
// Walkable surface record (4 bytes). Stored in walk_links[MAX_WALK_POOL].
struct WalkLink {
    UWORD Next;  // next entry in the linked list for this PAP LO cell
    SWORD Face;  // face index (building face the character can walk on)
};

// ========================================================================
// Global collision arrays (declared in collide_globals.h via _globals.cpp)
// ========================================================================

// uc_orig: col_vects_links (fallen/Source/collide.cpp)
// Per-cell linked list nodes binding barriers to PAP LO cells.
extern struct CollisionVectLink col_vects_links[MAX_COL_VECT_LINK];

// uc_orig: col_vects (fallen/Source/collide.cpp)
// Pool of all collision barriers loaded for the current level.
extern struct CollisionVect col_vects[MAX_COL_VECT];

// ========================================================================
// Slide-along flags
// ========================================================================

// uc_orig: SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT (fallen/Headers/collide.h)
#define SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT (-0x50)

// uc_orig: SLIDE_ALONG_FLAG_CRAWL (fallen/Headers/collide.h)
#define SLIDE_ALONG_FLAG_CRAWL    (1 << 0)
// uc_orig: SLIDE_ALONG_FLAG_CARRYING (fallen/Headers/collide.h)
#define SLIDE_ALONG_FLAG_CARRYING (1 << 1)
// uc_orig: SLIDE_ALONG_FLAG_JUMPING (fallen/Headers/collide.h)
#define SLIDE_ALONG_FLAG_JUMPING  (1 << 2)

// ========================================================================
// Line-of-sight flags
// ========================================================================

// uc_orig: LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG (fallen/Headers/collide.h)
#define LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG (1 << 0)
// uc_orig: LOS_FLAG_IGNORE_PRIMS (fallen/Headers/collide.h)
#define LOS_FLAG_IGNORE_PRIMS                 (1 << 1)
// uc_orig: LOS_FLAG_IGNORE_UNDERGROUND_CHECK (fallen/Headers/collide.h)
#define LOS_FLAG_IGNORE_UNDERGROUND_CHECK     (1 << 2)
// uc_orig: LOS_FLAG_INCLUDE_CARS (fallen/Headers/collide.h)
// Only relevant when prims are not ignored.
#define LOS_FLAG_INCLUDE_CARS                 (1 << 3)

// ========================================================================
// Fastnav bit array
// ========================================================================

// uc_orig: COLLIDE_Fastnavrow (fallen/Headers/collide.h)
// One row of the fastnav bit array: one bit per PAP HI cell in the row.
typedef UBYTE COLLIDE_Fastnavrow[PAP_SIZE_HI >> 3];

// uc_orig: COLLIDE_can_i_fastnav (fallen/Headers/collide.h)
// Returns true if the map cell (x,z) allows fast-nav (no nearby DFacets in that cell).
#define COLLIDE_can_i_fastnav(x, z) \
    (!(WITHIN((x), 0, PAP_SIZE_HI - 1) || !WITHIN((z), 0, PAP_SIZE_HI - 1) \
       || (COLLIDE_fastnav[x][(z) >> 3] & (1 << ((z) & 0x7)))))

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

// ========================================================================
// Face / walkable surface queries
// ========================================================================

// uc_orig: find_face_near_y (fallen/Source/collide.cpp)
// Searches PAP LO-res cells near (x,z) for a walkable face within [neg_dy,pos_dy]
// of y.  Returns face index (negative = roof face); writes height to *ret_y.
SLONG find_face_near_y(MAPCO16 x, MAPCO16 y, MAPCO16 z, SLONG ignore_faces_of_this_building, Thing* p_person, SLONG neg_dy, SLONG pos_dy, SLONG* ret_y);

// uc_orig: find_alt_for_this_pos (fallen/Source/collide.cpp)
// Returns the walkable floor height at (x,z), falling back to terrain height.
SLONG find_alt_for_this_pos(SLONG x, SLONG z);

// ========================================================================
// Ladder helpers
// ========================================================================

// uc_orig: correct_pos_for_ladder (fallen/Source/collide.cpp)
// Computes snap position and approach angle for the given ladder DFacet.
void correct_pos_for_ladder(struct DFacet* p_facet, SLONG* px, SLONG* pz, SLONG* angle, SLONG scale);

// uc_orig: ok_to_mount_ladder (fallen/Source/collide.cpp)
// Returns 1 if the thing is close enough (QDIST2 < 75) to mount the ladder facet.
SLONG ok_to_mount_ladder(struct Thing* p_thing, struct DFacet* p_facet);

// uc_orig: mount_ladder (fallen/Source/collide.cpp)
// Attempts to put p_thing onto the ladder at the given facet index.
SLONG mount_ladder(Thing* p_thing, SLONG facet);

// uc_orig: set_person_climb_down_onto_ladder (fallen/Source/collide.cpp)
// Snaps person to ladder and starts the climb-down-onto animation.
SLONG set_person_climb_down_onto_ladder(Thing* p_person, SLONG colvect);

// uc_orig: find_nearby_ladder_colvect_radius (fallen/Source/collide.cpp)
// Returns the nearest STOREY_TYPE_LADDER facet within radius of (mid_x,mid_z).
SLONG find_nearby_ladder_colvect_radius(SLONG mid_x, SLONG mid_z, SLONG radius);

// uc_orig: find_nearby_ladder_colvect (fallen/Source/collide.cpp)
// Returns the nearest ladder facet within LADDER_NEARBY_RADIUS of p_thing.
SLONG find_nearby_ladder_colvect(Thing* p_thing);

// ========================================================================
// Person foot / height utilities
// ========================================================================

// uc_orig: set_feet_to_y (fallen/Source/collide.cpp)
// Adjusts WorldPos.Y so the left-foot sub-object lands at new_y.
void set_feet_to_y(Thing* p_person, SLONG new_y);

// uc_orig: height_above_anything (fallen/Source/collide.cpp)
// Returns the height of body_part above the nearest surface; negative = clipped through.
SLONG height_above_anything(Thing* p_person, SLONG body_part, SWORD* onface);

// uc_orig: plant_feet (fallen/Source/collide.cpp)
// Snaps the person's feet to the nearest surface after movement.
// Returns 1=on face, 0=dropping, -1=on terrain floor.
SLONG plant_feet(Thing* p_person);

// ========================================================================
// Character radius queries
// ========================================================================

// uc_orig: get_person_radius (fallen/Source/collide.cpp)
// Returns the primary collision radius for the given person type.
SLONG get_person_radius(SLONG type);

// uc_orig: get_person_radius2 (fallen/Source/collide.cpp)
// Returns the secondary (tighter) collision radius for the given person type.
SLONG get_person_radius2(SLONG type);

// ========================================================================
// Fence height
// ========================================================================

// uc_orig: get_fence_height (fallen/Source/collide.cpp)
// Converts packed fence height to world units. h==2 maps to 85, otherwise h*64.
SLONG get_fence_height(SLONG h);

// ========================================================================
// Slide-along support
// ========================================================================

// uc_orig: step_back_along_vect (fallen/Source/collide.cpp)
// Walks (x2,z2) back along its delta until it crosses to the required side
// of wall segment (vx1,vz1)-(vx2,vz2).  side_required: 1=this side, 0=other.
void step_back_along_vect(SLONG x1, SLONG z1, SLONG* x2, SLONG* z2,
    SLONG vx1, SLONG vz1, SLONG vx2, SLONG vz2, SLONG side_required);

// ========================================================================
// Main slide collision
// ========================================================================

// uc_orig: slide_along (fallen/Source/collide.cpp)
// Slides endpoint (*x2,*y2,*z2) against DFacets and NOGO grid within radius.
// Writes collision results to slide_* globals. Returns UC_FALSE always.
SLONG slide_along(SLONG x1, SLONG my_y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG extra_wall_height, SLONG radius, ULONG flags);

// uc_orig: cross_door (fallen/Source/collide.cpp)
// Tests if movement (x1,y1,z1)->(x2,y2,z2) crosses a door DFacet.
// Returns -1=blocked inside, 0=no crossing, 1=exited building.
SLONG cross_door(SLONG x1, SLONG my_y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2, SLONG radius);

// uc_orig: bump_person (fallen/Source/collide.cpp)
// Cancels movement if it would bring p_person's radius inside Thing 'index'.
// Sets p_person->InWay on collision. Returns 0.
SLONG bump_person(Thing* p_person, THING_INDEX index,
    SLONG x1, SLONG my_y1, SLONG z1, SLONG* x2, SLONG* y2, SLONG* z2);

// ========================================================================
// Roof/walkable-face edge sliding
// ========================================================================

// uc_orig: slide_along_edges (fallen/Source/collide.cpp)
// Constrains (*x2,*z2) inside prim-quad face4 along FACE_FLAG_SLIDE_EDGE edges.
void slide_along_edges(SLONG face4, SLONG x1, SLONG z1, SLONG* x2, SLONG* z2);

// uc_orig: slide_along_redges (fallen/Source/collide.cpp)
// Constrains (*x2,*z2) inside a roof face by RFACE_FLAG_SLIDE_EDGE edges or
// MAV height transitions (for hidden flat-terrain roof faces).
void slide_along_redges(SLONG face4, SLONG x1, SLONG z1, SLONG* x2, SLONG* z2);

// ========================================================================
// High-level movement functions (chunk 4)
// ========================================================================

// uc_orig: move_thing_quick (fallen/Source/collide.cpp)
// Teleports a thing by (dx,dy,dz) without any collision detection.
// Updates MapWho lists via move_thing_on_map. Used for ragdoll and cutscenes.
ULONG move_thing_quick(SLONG dx, SLONG dy, SLONG dz, Thing* p_thing);

// uc_orig: collide_against_objects (fallen/Source/collide.cpp)
// Slides (x2,y2,z2) against all OB-placed objects (street furniture: lamps, bins, etc.)
// within a 3-cell radius of x2/z2. Returns UC_TRUE if any collision occurred.
SLONG collide_against_objects(
    Thing* p_thing, SLONG radius,
    SLONG x1, SLONG my_y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2);

// uc_orig: collide_against_things (fallen/Source/collide.cpp)
// Slides (x2,y2,z2) against nearby Things (persons, vehicles, furniture, pyro, bats).
// Returns UC_TRUE if any collision occurred.
SLONG collide_against_things(
    Thing* p_thing, SLONG radius,
    SLONG x1, SLONG my_y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2);

// uc_orig: drop_on_heads (fallen/Source/collide.cpp)
// Checks if p_thing is falling fast enough onto another person's head and kills them.
void drop_on_heads(Thing* p_thing);

// uc_orig: move_thing (fallen/Source/collide.cpp)
// Moves a CLASS_PERSON thing by (dx,dy,dz) with full collision detection:
// Things, OB objects, wall slide, edge slide, face tracking, and fall-off detection.
ULONG move_thing(SLONG dx, SLONG dy, SLONG dz, Thing* p_thing);

// ========================================================================
// LOS (Line of Sight) — chunk 5
// ========================================================================

// uc_orig: start_checking_against_a_new_vector (fallen/Source/collide.cpp)
// Resets the los_done ring buffer before a new LOS ray is traced. Call once per new ray.
void start_checking_against_a_new_vector(void);

// uc_orig: check_vector_against_mapsquare (fallen/Source/collide.cpp)
// Returns UC_TRUE if the current save_stack ray intersects a DFacet above map cell (map_x, map_z).
// Fills los_failure_* with the intersection point.
SLONG check_vector_against_mapsquare(SLONG map_x, SLONG map_z, SLONG los_flags);

// uc_orig: check_vector_against_mapsquare_objects (fallen/Source/collide.cpp)
// Returns UC_TRUE if the current save_stack ray intersects an OB-placed object or a large vehicle
// above map cell (map_x, map_z). include_cars: also test all vehicles (not just vans/ambulances).
SLONG check_vector_against_mapsquare_objects(SLONG map_x, SLONG map_z, SLONG include_cars);

// uc_orig: there_is_a_los_mav (fallen/Source/collide.cpp)
// Checks LOS at MAV (high-res navigation) grid resolution: 0=clear, 1=X-wall, 2=Z-wall.
// Uses MAV_CAPS_GOTO to test wall penetration rather than DFacets.
SLONG there_is_a_los_mav(SLONG x1, SLONG my_y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);

// uc_orig: there_is_a_los_car (fallen/Source/collide.cpp)
// Like there_is_a_los_mav but uses MAV_CAR_GOTO — the car navigation grid.
// Returns 0=clear, 1=X-wall, 2=Z-wall. Sets last_mav_square_* and last_mav_d* on hit.
SLONG there_is_a_los_car(SLONG x1, SLONG my_y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);

// ========================================================================
// LOS full-world tests
// ========================================================================

// uc_orig: there_is_a_los_things (fallen/Source/collide.cpp)
// Checks LOS between two Things. Returns UC_TRUE if line of sight is clear.
SLONG there_is_a_los_things(Thing* p_person_a, Thing* p_person_b, SLONG los_flags);

// uc_orig: there_is_a_los (fallen/Source/collide.cpp)
// Checks LOS between two world-space points. Returns UC_TRUE if clear.
SLONG there_is_a_los(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2,
    SLONG los_flag);

// uc_orig: in_my_fov (fallen/Source/collide.cpp)
// Returns UC_TRUE if (him_x,him_z) is within FOV looking from (me_x,me_z)
// in direction (im_looking_x, im_looking_z).
SLONG in_my_fov(
    SLONG me_x, SLONG me_z,
    SLONG him_x, SLONG him_z,
    SLONG im_looking_x,
    SLONG im_looking_z);

// uc_orig: find_nearby_person (fallen/Source/collide.cpp)
// Finds the nearest visible person of the given type within max_range.
// person_type_bits: one bit per person type. Returns THING_INDEX or -1.
THING_INDEX find_nearby_person(
    THING_INDEX me,
    UWORD person_type_bits,
    SLONG max_range);

// ========================================================================
// Intersection query
// ========================================================================

// uc_orig: find_intersected_colvect (fallen/Source/collide.cpp)
// Returns the index of a col_vect that intersects segment (x1,z1)-(x2,z2) at height y,
// preferring ladders and fences. Returns -1 if none found.
SLONG find_intersected_colvect(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG y);

// ========================================================================
// Box / circle intersection tests (early-out helpers)
// ========================================================================

// uc_orig: box_box_early_out (fallen/Headers/collide.h)
// Returns UC_TRUE if the two OBBs definitely do not collide (early exit).
void box_box_early_out(
    SLONG box1_mid_x, SLONG box1_mid_z,
    SLONG box1_min_x, SLONG box1_min_z,
    SLONG box1_max_x, SLONG box1_max_z,
    SLONG box1_yaw,
    SLONG box2_mid_x, SLONG box2_mid_z,
    SLONG box2_min_x, SLONG box2_min_z,
    SLONG box2_max_x, SLONG box2_max_z,
    SLONG box2_yaw);

// uc_orig: slide_around_circle (fallen/Headers/collide.h)
// Slides endpoint (*x2,*z2) around a circle (cx,cz,cradius).
// Returns UC_TRUE if a collision and slide occurred.
SLONG slide_around_circle(
    SLONG cx, SLONG cz, SLONG cradius,
    SLONG x1, SLONG z1,
    SLONG* x2, SLONG* z2);

// uc_orig: collide_box (fallen/Headers/collide.h)
// Returns UC_TRUE if a collision vector intersects a rotated bounding box.
SLONG collide_box(
    SLONG midx, SLONG midy, SLONG midz,
    SLONG minx, SLONG minz,
    SLONG maxx, SLONG maxz,
    SLONG yaw);

// uc_orig: collide_box_with_line (fallen/Headers/collide.h)
// Returns UC_TRUE if a line segment intersects the rotated bounding box.
SLONG collide_box_with_line(
    SLONG midx, SLONG midz,
    SLONG minx, SLONG minz,
    SLONG maxx, SLONG maxz,
    SLONG yaw,
    SLONG lx1, SLONG lz1,
    SLONG lx2, SLONG lz2);

// uc_orig: collide_against_sausage (fallen/Headers/collide.h)
// Orthogonal sausage-shape collision. Slides (vx2,vz2) away from sausage.
// Returns UC_TRUE on collision.
SLONG collide_against_sausage(
    SLONG sx1, SLONG sz1,
    SLONG sx2, SLONG sz2,
    SLONG swidth,
    SLONG vx1, SLONG vz1,
    SLONG vx2, SLONG vz2,
    SLONG* slide_x,
    SLONG* slide_z);

// ========================================================================
// Col-vect management
// ========================================================================

// uc_orig: insert_collision_vect (fallen/Source/collide.cpp)
// Allocates a new CollisionVect and links it into PAP LO cells.
// Returns the index of the new barrier, or -1 on overflow.
SLONG insert_collision_vect(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2,
    UBYTE prim, UBYTE prim_extra, SWORD face);

// uc_orig: remove_collision_vect (fallen/Source/collide.cpp)
// Removes a collision barrier from the pool and unlists it from PAP cells.
void remove_collision_vect(UWORD vect);

// uc_orig: get_point_dist_from_col_vect (fallen/Source/collide.cpp)
// Returns the distance from (x,z) to the barrier at index vect.
// Also writes the clamped nearest point to (*ret_x, *ret_z) offset by new_dist.
SLONG get_point_dist_from_col_vect(SLONG vect, SLONG x, SLONG z,
    SLONG* ret_x, SLONG* ret_z, SLONG new_dist);

// ========================================================================
// Height / face queries
// ========================================================================

// uc_orig: find_face_for_this_pos (fallen/Source/collide.cpp)
// Returns the walkable face index below (x,y,z). ignore_faces_of_this_building: skip one building.
// flag: controls search mode. Returns -1 if no face found.
SLONG find_face_for_this_pos(SLONG x, SLONG y, SLONG z, SLONG* ret_y,
    SLONG ignore_faces_of_this_building, UBYTE flag);

// uc_orig: calc_map_height_at (fallen/Headers/collide.h)
// Returns the map height at (x,z): building top if inside one, else calc_height_at().
SLONG calc_map_height_at(SLONG x, SLONG z);

// ========================================================================
// Level setup helpers
// ========================================================================

// uc_orig: insert_collision_facets (fallen/Source/collide.cpp)
// Inserts STOREY_TYPE_JUST_COLLISION barriers to block Darci at level boundaries.
void insert_collision_facets(void);

// uc_orig: create_shockwave (fallen/Source/collide.cpp)
// Creates an area-of-effect shockwave that damages and knocks down nearby people/things.
// maxdamage: max health loss (person max = 200). just_people: if non-zero, only affect people.
void create_shockwave(
    SLONG x, SLONG y, SLONG z,
    SLONG radius,
    SLONG maxdamage,
    Thing* p_aggressor, ULONG just_people = 0);

// uc_orig: COLLIDE_find_seethrough_fences (fallen/Source/collide.cpp)
// Marks fence facets with see-through textures as FACET_FLAG_SEETHROUGH.
void COLLIDE_find_seethrough_fences(void);

// ========================================================================
// Fastnav helpers
// ========================================================================

// uc_orig: COLLIDE_calc_fastnav_bits (fallen/Source/collide.cpp)
// Builds the fastnav bit array from current DFacets.
void COLLIDE_calc_fastnav_bits(void);

// uc_orig: COLLIDE_debug_fastnav (fallen/Source/collide.cpp)
// Debug: draws a cross over each fastnav square near the given world position.
void COLLIDE_debug_fastnav(SLONG world_x, SLONG world_z);

// ========================================================================
// Circle / sausage / box collision — chunk 5b
// ========================================================================

// uc_orig: collide_with_circle (fallen/Source/collide.cpp)
// Returns UC_TRUE if (*x2, *z2) is strictly inside the circle. Does not push out.
SLONG collide_with_circle(SLONG cx, SLONG cz, SLONG cradius, SLONG* x2, SLONG* z2);

// uc_orig: stop_movement_between (fallen/Source/collide.cpp)
// Returns UC_TRUE if movement between two adjacent hi-res map cells should be blocked
// (water edge; used by insert_collision_facets).
SLONG stop_movement_between(SLONG mx1, SLONG mz1, SLONG mx2, SLONG mz2);

// uc_orig: create_just_collision_facet (fallen/Source/collide.cpp)
// Allocates a STOREY_TYPE_JUST_COLLISION DFacet for a water/sewer edge.
void create_just_collision_facet(SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// uc_orig: slide_around_box (fallen/Source/collide.cpp)
// Slide (*x2, *z2) to the nearest edge of an OBB (oriented bounding box) expanded by radius.
// Retries if the chosen slide direction lands in a NOGO cell.
// Returns UC_TRUE if a collision occurred and x2/z2 were modified.
SLONG slide_around_box(
    SLONG box_mid_x, SLONG box_mid_z,
    SLONG box_min_x, SLONG box_min_z,
    SLONG box_max_x, SLONG box_max_z,
    SLONG box_yaw, SLONG radius,
    SLONG x1, SLONG z1, SLONG* x2, SLONG* z2);

// uc_orig: slide_around_box_lowstack (fallen/Source/collide.cpp)
// Simplified slide_around_box without NOGO retry. Used where stack depth matters.
inline SLONG slide_around_box_lowstack(
    SLONG box_mid_x, SLONG box_mid_z,
    SLONG box_min_x, SLONG box_min_z,
    SLONG box_max_x, SLONG box_max_z,
    SLONG box_yaw, SLONG radius,
    SLONG x1, SLONG z1, SLONG* x2, SLONG* z2);

// uc_orig: box_circle_early_out (fallen/Source/collide.cpp)
// Stub — only declared (no definition) in the original. Body added here for completeness.
void box_circle_early_out(
    SLONG box1_mid_x, SLONG box1_mid_z,
    SLONG box1_min_x, SLONG box1_min_z, SLONG box1_max_x, SLONG box1_max_z,
    SLONG box1_yaw,
    SLONG cx, SLONG cz, SLONG cradius);

// uc_orig: slide_around_sausage (fallen/Source/collide.cpp)
// Slides (*x2, *z2) around the outside of a sausage (capsule) shape defined by
// segment (sx1,sz1)-(sx2,sz2) with radius sradius. Returns UC_TRUE on collision.
SLONG slide_around_sausage(
    SLONG sx1, SLONG sz1,
    SLONG sx2, SLONG sz2,
    SLONG sradius,
    SLONG x1, SLONG z1,
    SLONG* x2, SLONG* z2);

#endif // ENGINE_PHYSICS_COLLIDE_H
