#include "engine/physics/collide.h"
#include "engine/physics/collide_globals.h"

// Temporary: Game.h for base types (QDIST2, ASSERT, CBYTE, etc.)
#include "Game.h"
// Temporary: pap.h for PAP_2LO, PAP_hi, PAP_FLAG_ROOF_EXISTS, MAVHEIGHT, ROOF_HIDDEN_GET_FACE, etc.
#include "fallen/Headers/pap.h"
// Temporary: missions/memory_globals.h for prim_faces4, prim_points, roof_faces4
#include "missions/memory_globals.h"
// Temporary: statedef.h for SUB_STATE_RUNNING_JUMP_FLY, STOREY_TYPE_* constants
#include "statedef.h"
#include "world/environment/edmap.h"

// Temporary: e_draw_3d_line (graphics debug line)
extern void e_draw_3d_line(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);

// Temporary: allow_debug_keys (Controls.cpp)
extern BOOL allow_debug_keys;

// Temporary: is_thing_on_this_quad (not yet migrated)
extern SLONG is_thing_on_this_quad(SLONG x, SLONG z, SLONG face);

// Temporary: walkable.h provides calc_height_on_face (macro alias to get_height_on_face_quad64_at),
// calc_height_on_rface, find_face_for_this_pos, FIND_ANYFACE, FIND_FACE_NEAR_BELOW, GRAB_FLOOR.
#include "world/navigation/walkable.h"

// Temporary: set_tween_for_dy (actors/characters/person.cpp, already migrated)
extern void set_tween_for_dy(Thing* p_person, SLONG dy);

// Temporary: person-related headers for chunk 2 ladder/feet functions
#include "fallen/Headers/mav.h"         // MAVHEIGHT macro
#include "actors/characters/anim_ids.h"     // SUB_OBJECT_LEFT_FOOT, COP_ROPER_ANIM_LADDER_END_L, ANIM_OFF_LADDER_TOP, ACTION_CLIMBING
#include "actors/core/interact.h"       // calc_sub_objects_position
#include "actors/core/state.h"          // set_generic_person_state_function, STATE_CLIMB_LADDER
#include "actors/core/thing.h"          // move_thing_on_map
#include "actors/characters/person.h"   // person_splash, set_person_climb_ladder, set_person_drop_down, locked_anim_change_end_type
#include "world/map/pap.h"              // PAP_calc_height_at, PAP_calc_height_at_thing
#include "engine/graphics/resources/console.h"  // CONSOLE_text
#include "engine/input/keyboard_globals.h"      // ControlFlag

// Temporary: add_debug_line (aeng.cpp, not yet migrated)
extern void add_debug_line(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG colour);

// Temporary: darci_globals.h for just_started_falling_off_backwards
#include "actors/characters/darci_globals.h"
// Temporary: supermap.h for SUPERMAP_counter_increase, SUPERMAP_counter
#include "world/map/supermap.h"
// Temporary: supermap_globals.h for next_dfacet
#include "world/map/supermap_globals.h"
// Temporary: anim_globals.h for next_prim_face4
#include "assets/anim_globals.h"

// Temporary: prim.h for slide_along_prim, prim_get_collision_model, get_prim_info,
// PRIM_OBJ_SPIKE, PRIM_COLLIDE_*, FACE_FLAG_FIRE_ESCAPE, FACE_FLAG_WMOVE, FACE_FLAG_PRIM
#include "fallen/Headers/prim.h"
// Temporary: ob.h for OB_find, OB_avoid, OB_Info, OB_ob, OB_ob_upto
#include "world/map/ob.h"
#include "world/map/ob_globals.h"
// Temporary: vehicle.h for get_vehicle_body_prim, get_vehicle_body_offset, get_vehicle_driver
#include "actors/vehicles/vehicle.h"
// Temporary: pcom.h for PCOM_person_a_hates_b, PCOM_attack_happened, PCOM_set_person_ai_flee_place
#include "ai/pcom.h"
// Temporary: combat.h for people_allowed_to_hit_each_other
#include "ai/combat.h"
// Temporary: inside2.h for find_inside_room
#include "world/navigation/inside2.h"
// Temporary: barrel.h for BARREL_hit_with_sphere
#include "actors/items/barrel.h"
// Temporary: mist.h for MIST_gust
#include "effects/mist.h"
// Temporary: dirt.h for DIRT_gust (not yet migrated to new/)
#include "fallen/Headers/dirt.h"
// Temporary: ai/mav_globals.h for MAV_opt (used in there_is_a_los_mav, there_is_a_los_car)
#include "ai/mav_globals.h"
// Temporary: ui/camera/fc.h for FC_explosion (camera shake on shockwave)
#include "ui/camera/fc.h"
// Temporary: actors/animals/bat.h for BAT_apply_hit
#include "actors/animals/bat.h"
// Temporary: engine/graphics/pipeline/aeng.h for AENG_world_line (fastnav debug visualisation)
#include "engine/graphics/pipeline/aeng.h"
// Temporary: world/environment/build2.h for add_facet_to_map (insert_collision_facets)
#include "world/environment/build2.h"
// Temporary: world/map/pap.h already included for PAP_calc_height_at; also needs PAP_calc_map_height_at
#include "world/map/pap.h"

// uc_orig: BLOCK_SIZE (fallen/Source/collide.cpp)
// Undefs any prior definition and sets it to 64, the height increment per storey level.
#undef BLOCK_SIZE
#define BLOCK_SIZE (1 << 6)

// uc_orig: SAME_SIGNS (fallen/Source/collide.cpp)
// True if two integers have the same sign (2's complement specific).
#define SAME_SIGNS(a, b) (((SLONG)((ULONG)a ^ (ULONG)b)) >= 0)

// uc_orig: two4_line_intersection (fallen/Source/collide.cpp)
UBYTE two4_line_intersection(SLONG x1, SLONG my_y1, SLONG x2, SLONG y2, SLONG x3, SLONG y3, SLONG x4, SLONG y4)
{
    SLONG ax, bx, cx, ay, by, cy, d, e, f;
    short x1lo, x1hi;
    short y1lo, y1hi;

    ax = x2 - x1;
    bx = x3 - x4;

    if (ax < 0) {
        x1lo = (SWORD)x2;
        x1hi = (SWORD)x1;
    } else {
        x1hi = (SWORD)x2;
        x1lo = (SWORD)x1;
    }

    if (bx > 0) {
        if (x1hi < (SWORD)x4 || (SWORD)x3 < x1lo)
            return (0);
    } else {
        if (x1hi < (SWORD)x3 || (SWORD)x4 < x1lo)
            return (0);
    }

    ay = y2 - my_y1;
    by = y3 - y4;

    if (ay < 0) {
        y1lo = (SWORD)y2;
        y1hi = (SWORD)my_y1;
    } else {
        y1hi = (SWORD)y2;
        y1lo = (SWORD)my_y1;
    }

    if (by > 0) {
        if (y1hi < (SWORD)y4 || (SWORD)y3 < y1lo)
            return (0);
    } else {
        if (y1hi < (SWORD)y3 || (SWORD)y4 < y1lo)
            return (0);
    }

    cx = x1 - x3;
    cy = my_y1 - y3;

    d = by * cx - bx * cy;
    f = ay * bx - ax * by;
    if (f > 0) {
        if (d < 0 || d > f)
            return (0);
    } else {
        if (d > 0 || d < f)
            return (0);
    }

    e = ax * cy - ay * cx;

    if (f > 0) {
        if (e < 0 || e > f)
            return (0);
    } else {
        if (e > 0 || e < f)
            return (0);
    }
    if (f == 0)
        return (1);

    return (2);
}

// uc_orig: clear_all_col_info (fallen/Source/collide.cpp)
void clear_all_col_info(void)
{
    // Stub — was used for debug collision reset.
}

// uc_orig: get_height_along_vect (fallen/Source/collide.cpp)
SLONG get_height_along_vect(SLONG ax, SLONG az, struct CollisionVect* p_vect)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG x1;
    SLONG my_y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG along, y;

    x1 = p_vect->X[0];
    my_y1 = p_vect->Y[0];
    z1 = p_vect->Z[0];

    x2 = p_vect->X[1];
    y2 = p_vect->Y[1];
    z2 = p_vect->Z[1];

    dx = x2 - x1;
    dy = y2 - my_y1;
    dz = z2 - z1;

    ax -= x1;
    az -= z1;

    if (abs(dx) > abs(dz)) {
        if (dx == 0) {
            dz = 1;
        }

        along = (ax << 8) / dx;
    } else {
        if (dz == 0) {
            dz = 1;
        }

        along = (az << 8) / dz;
    }

    y = my_y1 + ((dy * along) >> 8);

    return y;
}

// uc_orig: get_height_along_facet (fallen/Source/collide.cpp)
SLONG get_height_along_facet(SLONG ax, SLONG az, struct DFacet* p_facet)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG x1;
    SLONG my_y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG along, y;

    // Quick early-out for flat segment.
    if (p_facet->Y[0] == p_facet->Y[1]) {
        return p_facet->Y[0];
    }

    x1 = p_facet->x[0] << 8;
    my_y1 = p_facet->Y[0];
    z1 = p_facet->z[0] << 8;

    x2 = p_facet->x[1] << 8;
    y2 = p_facet->Y[1];
    z2 = p_facet->z[1] << 8;

    dx = x2 - x1;
    dy = y2 - my_y1;
    dz = z2 - z1;

    ax -= x1;
    az -= z1;

    if (abs(dx) > abs(dz)) {
        if (dx == 0) {
            dz = 1;
        }

        along = (ax << 8) / dx;
    } else {
        if (dz == 0) {
            dz = 1;
        }

        along = (az << 8) / dz;
    }

    y = my_y1 + ((dy * along) >> 8);

    return y;
}

// uc_orig: TSHIFT (fallen/Source/collide.cpp)
// Fixed-point shift used in barycentric triangle test (8 bits of precision).
#define TSHIFT 8
// uc_orig: check_big_point_triangle_col (fallen/Source/collide.cpp)
UBYTE check_big_point_triangle_col(SLONG x, SLONG y, SLONG ux, SLONG uy, SLONG vx, SLONG vy, SLONG wx, SLONG wy)
{
    SLONG s, t, top, bot, res;
    top = (y - uy) * (wx - ux) + (ux - x) * (wy - uy);
    bot = (vy - uy) * (wx - ux) - (vx - ux) * (wy - uy);

    if (bot == 0)
        return 0;

    s = (top << TSHIFT) / bot;
    if (s < 0)
        return 0;
    if ((wx - ux) == 0)
        t = ((y << TSHIFT) - (uy << TSHIFT) - s * (vy - uy)) / (wy - uy);
    else
        t = ((x << TSHIFT) - (ux << TSHIFT) - s * (vx - ux)) / (wx - ux);
    if (t < 0)
        return 0;

    res = s + t;
    if (res < (1 << TSHIFT)) {
        return 1; // inside triangle
    } else
        return 0; // outside triangle
}

// uc_orig: point_in_quad_old (fallen/Source/collide.cpp)
SLONG point_in_quad_old(SLONG px, SLONG pz, SLONG x, SLONG y, SLONG z, SWORD face)
{
    SLONG x1, my_y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
    SLONG ret;

    x1 = x + prim_points[prim_faces4[face].Points[0]].X;
    my_y1 = y + prim_points[prim_faces4[face].Points[0]].Y;
    z1 = z + prim_points[prim_faces4[face].Points[0]].Z;

    x2 = x + prim_points[prim_faces4[face].Points[1]].X;
    y2 = y + prim_points[prim_faces4[face].Points[1]].Y;
    z2 = z + prim_points[prim_faces4[face].Points[1]].Z;

    x3 = x + prim_points[prim_faces4[face].Points[2]].X;
    y3 = y + prim_points[prim_faces4[face].Points[2]].Y;
    z3 = z + prim_points[prim_faces4[face].Points[2]].Z;

    x4 = x + prim_points[prim_faces4[face].Points[3]].X;
    y4 = y + prim_points[prim_faces4[face].Points[3]].Y;
    z4 = z + prim_points[prim_faces4[face].Points[3]].Z;

    ret = check_big_point_triangle_col(px - x1, pz - z1, x1 - x1, z1 - z1, x2 - x1, z2 - z1, x3 - x1, z3 - z1);
    if (ret)
        return (ret);
    else
        return (check_big_point_triangle_col(px - x2, pz - z2, x2 - x2, z2 - z2, x4 - x2, z4 - z2, x3 - x2, z3 - z2));
}

// uc_orig: dist_to_line (fallen/Source/collide.cpp)
// Sets global dprod and cprod as side effects; used by callers that read them afterward.
SLONG dist_to_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b)
{
    SLONG dx, dz;
    SLONG da, db;
    SLONG m;

    SLONG dist;

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        da = abs(da);
        db = abs(db);
        dist = QDIST2(da, db);
        return (dist);
    }

    dx = x1 - x2;
    dz = z1 - z2;

    da = a - x2;
    db = b - z2;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        da = abs(da);
        db = abs(db);
        dist = QDIST2(da, db);
        return (dist);
    }

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    cprod = da * dz - db * dx;
    dx = abs(dx);
    dz = abs(dz);

    cprod = abs(cprod);

    m = QDIST2(dx, dz);

    dist = cprod / m;

    return (dist);
}

// uc_orig: which_side (fallen/Source/collide.cpp)
SLONG which_side(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b)
{
    SLONG dx;
    SLONG dz;

    SLONG da;
    SLONG db;

    // Local cprod shadows the global — intentional, this function uses it as a temporary only.
    SLONG cprod;

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    cprod = da * dz - db * dx;

    return cprod;
}

// uc_orig: calc_along_vect (fallen/Source/collide.cpp)
SLONG calc_along_vect(SLONG ax, SLONG az, struct DFacet* p_vect)
{
    SLONG dx;
    SLONG dz;

    SLONG x1;
    SLONG z1;

    SLONG x2;
    SLONG z2;

    SLONG along;

    x1 = p_vect->x[0] << 8;
    z1 = p_vect->z[0] << 8;

    x2 = p_vect->x[1] << 8;
    z2 = p_vect->z[1] << 8;

    dx = x2 - x1;
    dz = z2 - z1;

    ax -= x1;
    az -= z1;

    if (abs(dx) > abs(dz)) {
        if (dx == 0) {
            dz = 1;
        }

        along = (ax << 8) / dx;
    } else {
        if (dz == 0) {
            dz = 1;
        }

        along = (az << 8) / dz;
    }

    return along;
}

// uc_orig: signed_dist_to_line_with_normal (fallen/Source/collide.cpp)
// Normal is always the perpendicular of the direction vector.
// Returns the endpoint-to-point direction (unchanged) when projection is off-segment.
void signed_dist_to_line_with_normal(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b,
    SLONG* dist,
    SLONG* vec_x,
    SLONG* vec_z,
    SLONG* on)
{
    SLONG dx;
    SLONG dz;

    SLONG da;
    SLONG db;

    SLONG len;

    dx = x2 - x1;
    dz = z2 - z1;

    *vec_x = dz;
    *vec_z = -dx;

    da = a - x1;
    db = b - z1;

    cprod = da * dz - db * dx;
    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        // Nearest point on segment is (x1,z1).
        *dist = QDIST2(abs(da), abs(db));
        *on = UC_FALSE;

        if (cprod < 0) {
            *dist = -*dist;
        }

        return;
    }

    dx = x1 - x2;
    dz = z1 - z2;

    da = a - x2;
    db = b - z2;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        // Nearest point on segment is (x2,z2).
        *dist = QDIST2(abs(da), abs(db));
        *on = UC_FALSE;

        if (cprod < 0) {
            *dist = -*dist;
        }

        return;
    }

    // Perpendicular projection lies within the segment.
    len = QDIST2(abs(dx), abs(dz));

    *dist = cprod / len;
    *vec_x = -dz;
    *vec_z = dx;
    *on = UC_TRUE;

    return;
}

// uc_orig: signed_dist_to_line_with_normal_mark (fallen/Source/collide.cpp)
// Like signed_dist_to_line_with_normal, but overrides vec_x/vec_z with endpoint-to-point
// direction when the projection falls off the segment end.
void signed_dist_to_line_with_normal_mark(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b,
    SLONG* dist,
    SLONG* vec_x,
    SLONG* vec_z,
    SLONG* on)
{
    SLONG dx;
    SLONG dz;

    SLONG da;
    SLONG db;

    SLONG len;

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    cprod = da * dz - db * dx;
    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        // Nearest point on segment is (x1,z1).
        *vec_x = da;
        *vec_z = db;

        *dist = QDIST2(abs(da), abs(db));
        *on = UC_FALSE;

        if (cprod < 0) {
            *dist = -*dist;
        }

        return;
    }

    dx = x1 - x2;
    dz = z1 - z2;

    da = a - x2;
    db = b - z2;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        // Nearest point on segment is (x2,z2).
        *vec_x = da;
        *vec_z = db;

        *dist = QDIST2(abs(da), abs(db));
        *on = UC_FALSE;

        if (cprod < 0) {
            *dist = -*dist;
        }

        return;
    }

    // Perpendicular projection lies within the segment.
    len = QDIST2(abs(dx), abs(dz));

    *dist = cprod / len;
    *vec_x = -dz;
    *vec_z = dx;
    *on = UC_TRUE;

    return;
}

// uc_orig: nearest_point_on_line (fallen/Source/collide.cpp)
void nearest_point_on_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_z)
{
    SLONG dx, dz;
    SLONG da, db;
    SLONG along;

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        *ret_x = x1;
        *ret_z = z1;
        return;
    }

    dx = x1 - x2;
    dz = z1 - z2;

    da = a - x2;
    db = b - z2;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        *ret_x = x2;
        *ret_z = z2;
        return;
    }

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    dprod = da * dx + db * dz;

    dx = abs(dx);
    dz = abs(dz);

    dprod = abs(dprod);

    along = dprod / ((dx * dx + dz * dz) >> 8);

    dx = x2 - x1;
    dz = z2 - z1;
    *ret_x = x1 + ((dx * along) >> 8);
    *ret_z = z1 + ((dz * along) >> 8);

    return;
}

// uc_orig: nearest_point_on_line_and_dist (fallen/Source/collide.cpp)
SLONG nearest_point_on_line_and_dist(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_z)
{
    SLONG dx, dz;
    SLONG da, db;
    SLONG m;
    SLONG along;
    SLONG dist;

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        da = abs(da);
        db = abs(db);
        dist = QDIST2(da, db);
        *ret_x = x1;
        *ret_z = z1;
        global_on = 0;
        return (dist);
    }

    dx = x1 - x2;
    dz = z1 - z2;

    da = a - x2;
    db = b - z2;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        da = abs(da);
        db = abs(db);
        dist = QDIST2(da, db);
        *ret_x = x2;
        *ret_z = z2;
        global_on = 0;
        return (dist);
    }

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    cprod = da * dz - db * dx;
    dprod = da * dx + db * dz;

    dx = abs(dx);
    dz = abs(dz);

    cprod = abs(cprod);
    dprod = abs(dprod);

    along = dprod / (((dx * dx + dz * dz) >> 8) + 1);

    m = QDIST2(dx, dz) + 1;

    dist = cprod / m;

    dx = x2 - x1;
    dz = z2 - z1;
    *ret_x = x1 + ((dx * along) >> 8);
    *ret_z = z1 + ((dz * along) >> 8);
    global_on = 1;

    return (dist);
}

// uc_orig: nearest_point_on_line_and_dist_calc_y (fallen/Source/collide.cpp)
SLONG nearest_point_on_line_and_dist_calc_y(SLONG x1, SLONG my_y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_y, SLONG* ret_z)
{
    SLONG dx, dz, dy;
    SLONG da, db;
    SLONG m;
    SLONG along;
    SLONG dist;

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        da = abs(da);
        db = abs(db);
        dist = QDIST2(da, db);
        *ret_x = x1;
        *ret_y = my_y1;
        *ret_z = z1;
        return (dist);
    }

    dx = x1 - x2;
    dz = z1 - z2;

    da = a - x2;
    db = b - z2;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        da = abs(da);
        db = abs(db);
        dist = QDIST2(da, db);
        *ret_x = x2;
        *ret_y = y2;
        *ret_z = z2;
        return (dist);
    }

    dx = x2 - x1;
    dy = y2 - my_y1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    cprod = da * dz - db * dx;
    dprod = da * dx + db * dz;

    dx = abs(dx);
    dz = abs(dz);

    cprod = abs(cprod);
    dprod = abs(dprod);

    along = dprod / (((dx * dx + dz * dz) >> 8) + 1);

    m = QDIST2(dx, dz) + 1;

    dist = cprod / m;

    dx = x2 - x1;
    dz = z2 - z1;
    *ret_x = x1 + ((dx * along) >> 8);
    *ret_y = my_y1 + ((dy * along) >> 8);
    *ret_z = z1 + ((dz * along) >> 8);

    return (dist);
}

// uc_orig: distance_to_line (fallen/Source/collide.cpp)
SLONG distance_to_line(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG a, SLONG b)
{
    SLONG nearest_x;
    SLONG nearest_z;

    SLONG dist = nearest_point_on_line_and_dist(
        x1, z1,
        x2, z2,
        a, b,
        &nearest_x,
        &nearest_z);

    return dist;
}

// uc_orig: nearest_point_on_line_and_dist_and_along (fallen/Source/collide.cpp)
SLONG nearest_point_on_line_and_dist_and_along(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_z, SLONG* ret_along)
{
    SLONG dx, dz;
    SLONG da, db;
    SLONG m;
    SLONG along;
    SLONG dist;

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        da = abs(da);
        db = abs(db);
        dist = QDIST2(da, db);
        *ret_x = x1;
        *ret_z = z1;
        *ret_along = 0;
        return (dist);
    }

    dx = x1 - x2;
    dz = z1 - z2;

    da = a - x2;
    db = b - z2;

    dprod = da * dx + db * dz;

    if (dprod <= 0) {
        da = abs(da);
        db = abs(db);
        dist = QDIST2(da, db);
        *ret_x = x2;
        *ret_z = z2;
        *ret_along = 255;
        return (dist);
    }

    dx = x2 - x1;
    dz = z2 - z1;

    da = a - x1;
    db = b - z1;

    cprod = da * dz - db * dx;
    dprod = da * dx + db * dz;

    dx = abs(dx);
    dz = abs(dz);

    cprod = abs(cprod);
    dprod = abs(dprod);

    along = dprod / ((dx * dx + dz * dz) >> 8);

    m = QDIST2(dx, dz);

    dist = cprod / m;

    dx = x2 - x1;
    dz = z2 - z1;
    *ret_x = x1 + ((dx * along) >> 8);
    *ret_z = z1 + ((dz * along) >> 8);

    *ret_along = along;
    return (dist);
}

// uc_orig: calc_height_at (fallen/Source/collide.cpp)
// Body was commented out in original source; entire implementation was in /*...*/ block.
// Returns 0 always.
SLONG calc_height_at(SLONG x, SLONG z)
{
    return 0;
}

// uc_orig: collision_storey (fallen/Source/collide.cpp)
SLONG collision_storey(SLONG type)
{
    switch (type) {
    case STOREY_TYPE_NORMAL:
    case STOREY_TYPE_NORMAL_FOUNDATION:
    case STOREY_TYPE_WALL:
    case STOREY_TYPE_FENCE:
    case STOREY_TYPE_FENCE_BRICK:
    case STOREY_TYPE_FENCE_FLAT:
    case STOREY_TYPE_NOT_REALLY_A_STOREY_TYPE_BUT_A_VALUE_TO_PUT_IN_THE_PRIM_TYPE_FIELD_OF_COLVECTS_GENERATED_BY_INSIDE_BUILDINGS:
    case STOREY_TYPE_LADDER:
    case STOREY_TYPE_TRENCH:
    case STOREY_TYPE_SKYLIGHT:
    case STOREY_TYPE_STAIRCASE:
    case STOREY_TYPE_CABLE:
    case STOREY_TYPE_OUTSIDE_DOOR:
        return (1);
    default:
        return (0);
    }
}

// uc_orig: highlight_face (fallen/Source/collide.cpp)
// Debug visualisation — always returns immediately (early return preserved from original).
void highlight_face(SLONG face)
{
    return;
    if (face > 0) {
        SLONG face_x, face_y, face_z;
        SLONG p0, p1, p2, p3;

        face_x = 0;
        face_y = 0;
        face_z = 0;

        p0 = prim_faces4[face].Points[0];
        p1 = prim_faces4[face].Points[1];
        p2 = prim_faces4[face].Points[2];
        p3 = prim_faces4[face].Points[3];

        e_draw_3d_line(prim_points[p1].X + face_x, prim_points[p1].Y + face_y, prim_points[p1].Z + face_z,
            prim_points[p3].X + face_x, prim_points[p3].Y + face_y, prim_points[p3].Z + face_z);

        e_draw_3d_line(
            prim_points[p3].X + face_x, prim_points[p3].Y + face_y, prim_points[p3].Z + face_z,
            prim_points[p2].X + face_x, prim_points[p2].Y + face_y, prim_points[p2].Z + face_z);

        e_draw_3d_line(
            prim_points[p2].X + face_x, prim_points[p2].Y + face_y, prim_points[p2].Z + face_z,
            prim_points[p0].X + face_x, prim_points[p0].Y + face_y, prim_points[p0].Z + face_z);
    }
}

// uc_orig: highlight_rface (fallen/Source/collide.cpp)
void highlight_rface(SLONG rface)
{
    return;
    if (rface > 0) {
        SLONG x, y, z;

        x = (roof_faces4[rface].RX & 127) << 8;
        y = roof_faces4[rface].Y;
        z = (roof_faces4[rface].RZ & 127) << 8;

        e_draw_3d_line(x, y, z, x + 256, y, z);
        e_draw_3d_line(x + 256, y, z, x + 256, y, z + 256);
        e_draw_3d_line(x + 256, y, z + 256, x, y, z + 256);
        e_draw_3d_line(x, y, z + 256, x, y, z);
    }
}

// uc_orig: highlight_quad (fallen/Source/collide.cpp)
void highlight_quad(SLONG face, SLONG face_x, SLONG face_y, SLONG face_z)
{
    return;

    if (face > 0) {
        SLONG p0, p1, p2, p3;

        p0 = prim_faces4[face].Points[0];
        p1 = prim_faces4[face].Points[1];
        p2 = prim_faces4[face].Points[2];
        p3 = prim_faces4[face].Points[3];

        e_draw_3d_line(prim_points[p0].X + face_x, prim_points[p0].Y + face_y, prim_points[p0].Z + face_z,
            prim_points[p1].X + face_x, prim_points[p1].Y + face_y, prim_points[p1].Z + face_z);

        e_draw_3d_line(prim_points[p1].X + face_x, prim_points[p1].Y + face_y, prim_points[p1].Z + face_z,
            prim_points[p3].X + face_x, prim_points[p3].Y + face_y, prim_points[p3].Z + face_z);

        e_draw_3d_line(
            prim_points[p3].X + face_x, prim_points[p3].Y + face_y, prim_points[p3].Z + face_z,
            prim_points[p2].X + face_x, prim_points[p2].Y + face_y, prim_points[p2].Z + face_z);

        e_draw_3d_line(
            prim_points[p2].X + face_x, prim_points[p2].Y + face_y, prim_points[p2].Z + face_z,
            prim_points[p0].X + face_x, prim_points[p0].Y + face_y, prim_points[p0].Z + face_z);
    }
}

// uc_orig: vect_intersect_wall (fallen/Source/collide.cpp)
// Stub — never implemented; returns 0 always.
SLONG vect_intersect_wall(SLONG x1, SLONG my_y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2)
{
    return (0);
}

// ========================================================================
// Face / walkable surface queries
// ========================================================================

// uc_orig: find_face_near_y (fallen/Source/collide.cpp)
// Searches the LO-res PAP cells around (x,z) for a walkable face whose
// height is within [neg_dy, pos_dy] of y.  Also checks a roof face if one
// is flagged for that cell.  Returns the face index (negative = roof face)
// and writes the surface height into *ret_y.  Returns 0 if none found.
SLONG find_face_near_y(MAPCO16 x, MAPCO16 y, MAPCO16 z, SLONG ignore_faces_of_this_building, Thing* p_person, SLONG neg_dy, SLONG pos_dy, SLONG* ret_y)
{
    SLONG mx, mz;
    SLONG index;
    SLONG check_face;
    SLONG new_y, dy;

    SLONG mx1 = (x - 0x200) >> PAP_SHIFT_LO;
    SLONG mz1 = (z - 0x200) >> PAP_SHIFT_LO;

    SLONG mx2 = (x + 0x200) >> PAP_SHIFT_LO;
    SLONG mz2 = (z + 0x200) >> PAP_SHIFT_LO;

    SATURATE(mx1, 0, PAP_SIZE_LO - 1);
    SATURATE(mz1, 0, PAP_SIZE_LO - 1);

    SATURATE(mx2, 0, PAP_SIZE_LO - 1);
    SATURATE(mz2, 0, PAP_SIZE_LO - 1);

    if (PAP_hi[x >> 8][z >> 8].Flags & PAP_FLAG_ROOF_EXISTS) {

        check_face = ROOF_HIDDEN_GET_FACE(x >> 8, z >> 8);
        new_y = MAVHEIGHT(x >> 8, z >> 8) << 6;
        dy = y - new_y;

        if (dy > neg_dy - 100 && dy < pos_dy) {
            *ret_y = new_y;

            return (check_face);
        } else if (dy > -128) {
            if (p_person->SubState == SUB_STATE_RUNNING_JUMP_FLY) {
                set_tween_for_dy(p_person, y - new_y);
            }
        }
    }

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            index = PAP_2LO(mx, mz).Walkable;

            while (index) {
                SLONG offset_y;
                check_face = index;

                {
                    if (check_face > 0 && prim_faces4[check_face].ThingIndex == ignore_faces_of_this_building) {
                        // Skip — don't collide with this building's own face.
                    } else {
                        if (is_thing_on_this_quad(x, z, check_face)) {
                            if (check_face < 0) {
                                calc_height_on_rface(x, z, -check_face, &new_y);
                                offset_y = 100;
                            } else {
                                calc_height_on_face(x, z, check_face, &new_y);
                                offset_y = 0;
                            }

                            dy = y - new_y;

                            if (ControlFlag && allow_debug_keys) {
                                CBYTE str[100];
                                sprintf(str, " land on walkable dy %d >%d  <%d \n", dy, neg_dy, pos_dy);
                                CONSOLE_text(str, 10000);
                            }
                            if (dy > neg_dy - offset_y && dy < pos_dy) {
                                *ret_y = new_y;

                                return (check_face);
                            } else
                                if (dy > -128) {
                                    if (p_person->SubState == SUB_STATE_RUNNING_JUMP_FLY) {
                                        set_tween_for_dy(p_person, y - new_y);
                                    }
                                    MSG_add(" LAND on face y  %d peep y %d miss dy=%d ", new_y, y, y - new_y);
                                }
                        }
                    }

                    if (index < 0)
                        index = roof_faces4[-index].Next;
                    else
                        index = prim_faces4[index].WALKABLE;
                }
            }
        }
    *ret_y = 0;
    return (0);
}

// uc_orig: find_alt_for_this_pos (fallen/Source/collide.cpp)
// Returns the ground height (in world units) at position (x,z).
// Checks a 2x2 region of LO-res PAP cells for a walkable face; if none
// found falls back to terrain height from PAP_calc_height_at.
SLONG find_alt_for_this_pos(SLONG x, SLONG z)
{
    SLONG mx;
    SLONG mz;
    SLONG facey;
    SLONG index;
    SLONG groundy;
    SLONG count;

    SLONG mx1 = x - 0x200 >> PAP_SHIFT_LO;
    SLONG mz1 = z - 0x200 >> PAP_SHIFT_LO;

    SLONG mx2 = x + 0x200 >> PAP_SHIFT_LO;
    SLONG mz2 = z + 0x200 >> PAP_SHIFT_LO;

    SATURATE(mx1, 0, PAP_SIZE_LO - 1);
    SATURATE(mz1, 0, PAP_SIZE_LO - 1);

    SATURATE(mx2, 0, PAP_SIZE_LO - 1);
    SATURATE(mz2, 0, PAP_SIZE_LO - 1);

    if (PAP_hi[x >> 8][z >> 8].Flags & PAP_FLAG_ROOF_EXISTS) {
        return (MAVHEIGHT(x >> 8, z >> 8) << 6);
    }

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            index = PAP_2LO(mx, mz).Walkable;
            count = 0;

            while (index && count++ < 1000) {
                ASSERT(index >= 0);

                if (is_thing_on_this_quad(x, z, index)) {
                    if (index < 0) {
                        calc_height_on_rface(x, z, -index, &facey);
                    } else {
                        calc_height_on_face(x, z, index, &facey);
                    }

                    return facey;
                }

                if (index < 0) {
                    index = roof_faces4[-index].Next;

                } else {
                    index = prim_faces4[index].WALKABLE;
                }
                ASSERT(count < 800);
            }
        }

    groundy = PAP_calc_height_at(x, z) + 5;

    return groundy;
}

// ========================================================================
// Ladder helpers
// ========================================================================

// uc_orig: correct_pos_for_ladder (fallen/Source/collide.cpp)
// Computes the world position and approach angle for a ladder facet.
// The midpoint of the facet's two endpoints is the snap position; the
// angle is perpendicular to the facet edge, offset so the person faces
// into the ladder.  scale shifts the snap point along the ladder normal.
void correct_pos_for_ladder(struct DFacet* p_facet, SLONG* px, SLONG* pz, SLONG* angle, SLONG scale)
{
    SLONG x1, z1, x2, z2, dx, dz;

    x1 = p_facet->x[0] << 8;
    z1 = p_facet->z[0] << 8;

    x2 = p_facet->x[1] << 8;
    z2 = p_facet->z[1] << 8;

    *px = (x1 + x2) >> 1;
    *pz = (z1 + z2) >> 1;

    dx = x2 - x1;
    dz = z2 - z1;

    {
        *angle = Arctan(-dx, dz) + 1024 + 512;
        if (*angle < 0)
            *angle = 2048 + *angle;
        *angle = *angle & 2047;
    }

    *px += (dz * scale) >> 10;
    *pz -= (dx * scale) >> 10;
}

// uc_orig: ok_to_mount_ladder (fallen/Source/collide.cpp)
// Returns 1 if the thing is within QDIST2 < 75 of the ladder's midpoint.
SLONG ok_to_mount_ladder(struct Thing* p_thing, struct DFacet* p_facet)
{
    SLONG dx, dz, px, pz, angle;

    correct_pos_for_ladder(p_facet, &px, &pz, &angle, 256);

    dx = abs(px - (p_thing->WorldPos.X >> 8));
    dz = abs(pz - (p_thing->WorldPos.Z >> 8));

    if (QDIST2(dx, dz) < 75) {
        return (1);
    } else {
        return (0);
    }
}

// uc_orig: mount_ladder (fallen/Source/collide.cpp)
// Tries to put p_thing onto the ladder at facet index.  Returns UC_TRUE on
// success.  Note: the call path from interfac.cpp is commented out in the
// pre-release source, so players cannot grab ladders from below; AI uses
// set_person_climb_ladder directly.
SLONG mount_ladder(Thing* p_thing, SLONG facet)
{
    if (ok_to_mount_ladder(p_thing, &dfacets[facet])) {
        set_person_climb_ladder(p_thing, facet);

        return UC_TRUE;
    }

    return UC_FALSE;
}

// uc_orig: set_person_climb_down_onto_ladder (fallen/Source/collide.cpp)
// Snaps the person to the ladder midpoint and starts the climb-down-onto-
// ladder animation.  Used when the player steps off the edge of a roof.
// Returns 1 on success, 0 if too far from the ladder.
SLONG set_person_climb_down_onto_ladder(Thing* p_person, SLONG colvect)
{
    SLONG x, z, dx, dz, angle;
    SLONG dist = 64;
    GameCoord new_position;

    if (p_person->Genus.Person->PersonType == PERSON_ROPER)
        dist = -64;

    correct_pos_for_ladder(&dfacets[colvect], &x, &z, &angle, dist);

    dx = abs(x - (p_person->WorldPos.X >> 8));
    dz = abs(z - (p_person->WorldPos.Z >> 8));

    if (QDIST2(dx, dz) > 75) {
        return (0);
    }

    new_position.X = x << 8;
    new_position.Y = p_person->WorldPos.Y;
    new_position.Z = z << 8;

    p_person->Draw.Tweened->Angle = angle;

    move_thing_on_map(p_person, &new_position);

    set_generic_person_state_function(p_person, STATE_CLIMB_LADDER);

    if (p_person->Genus.Person->PersonType == PERSON_ROPER)
        locked_anim_change_end_type(p_person, SUB_OBJECT_LEFT_FOOT, COP_ROPER_ANIM_LADDER_END_L, ANIM_TYPE_ROPER);
    else if (p_person->Genus.Person->PersonType == PERSON_COP || p_person->Genus.Person->PersonType == PERSON_THUG_GREY || p_person->Genus.Person->PersonType == PERSON_THUG_RASTA || p_person->Genus.Person->PersonType == PERSON_THUG_RED)
        locked_anim_change_end_type(p_person, SUB_OBJECT_LEFT_FOOT, COP_ROPER_ANIM_LADDER_END_L, ANIM_TYPE_ROPER);
    else

        locked_anim_change_end_type(p_person, SUB_OBJECT_LEFT_FOOT, ANIM_OFF_LADDER_TOP, p_person->Genus.Person->AnimType);

    p_person->Velocity = 0;
    p_person->Genus.Person->Action = ACTION_CLIMBING;
    p_person->SubState = SUB_STATE_CLIMB_DOWN_ONTO_LADDER;
    p_person->Genus.Person->OnFacet = colvect;
    p_person->OnFace = 0;

    p_person->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C;
    return (1);
}

// uc_orig: find_nearby_ladder_colvect_radius (fallen/Source/collide.cpp)
// Scans LO-res PAP cells within radius of (mid_x,mid_z) looking for a
// STOREY_TYPE_LADDER facet.  Returns the index of the closest one found,
// or 0 if none.
SLONG find_nearby_ladder_colvect_radius(
    SLONG mid_x,
    SLONG mid_z,
    SLONG radius)
{
    SLONG mx;
    SLONG mz;

    SLONG dx;
    SLONG dz;

    SLONG dist;

    SLONG minx;
    SLONG minz;

    SLONG maxx;
    SLONG maxz;

    SLONG v_list;
    SLONG i_vect;

    SLONG best_dist = UC_INFINITY;
    SLONG best_vect = NULL;

    struct DFacet* p_vect;

    minx = mid_x - radius >> PAP_SHIFT_LO;
    minz = mid_z - radius >> PAP_SHIFT_LO;

    maxx = mid_x + radius >> PAP_SHIFT_LO;
    maxz = mid_z + radius >> PAP_SHIFT_LO;

    for (mx = minx; mx <= maxx; mx++)
        for (mz = minz; mz <= maxz; mz++) {
            if (WITHIN(mx, 0, PAP_SIZE_LO - 1) && WITHIN(mz, 0, PAP_SIZE_LO - 1)) {
                SLONG exit = 0;
                v_list = PAP_2LO(mx, mz).ColVectHead;

                if (v_list)
                    while (!exit) {
                        i_vect = facet_links[v_list];
                        if (i_vect < 0) {
                            i_vect = -i_vect;
                            exit = 1;
                        }
                        p_vect = &dfacets[i_vect];

                        if (p_vect->FacetType == STOREY_TYPE_LADDER) {
                            dx = (p_vect->x[0] + p_vect->x[1] << 7) - mid_x;
                            dz = (p_vect->z[0] + p_vect->z[1] << 7) - mid_z;

                            dist = abs(dx) + abs(dz);

                            if (dist < best_dist) {
                                best_dist = dist;
                                best_vect = i_vect;
                            }
                        }

                        v_list++;
                    }
            }
        }

    return best_vect;
}

// uc_orig: LADDER_NEARBY_RADIUS (fallen/Source/collide.cpp)
// Search radius for find_nearby_ladder_colvect (in block-level coordinates).
#define LADDER_NEARBY_RADIUS 100

// uc_orig: find_nearby_ladder_colvect (fallen/Source/collide.cpp)
// Wrapper: finds the nearest ladder facet within LADDER_NEARBY_RADIUS (100)
// units of the thing's current position.
SLONG find_nearby_ladder_colvect(Thing* p_thing)
{
    SLONG mid_x;
    SLONG mid_z;

    SLONG ladder;

    mid_x = p_thing->WorldPos.X >> 8;
    mid_z = p_thing->WorldPos.Z >> 8;

    ladder = find_nearby_ladder_colvect_radius(mid_x, mid_z, LADDER_NEARBY_RADIUS);

    return ladder;
}

// ========================================================================
// Person foot / height utilities
// ========================================================================

// uc_orig: set_feet_to_y (fallen/Source/collide.cpp)
// Repositions the person's WorldPos.Y so that the left foot sub-object
// lands at new_y world units.
void set_feet_to_y(Thing* p_person, SLONG new_y)
{
    SLONG x, y, z;
    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_FOOT, &x, &y, &z);
    MSG_add(" set feet to y %d, foot y %d \n", new_y, y);

    new_y -= y;

    p_person->WorldPos.Y = (new_y) << 8;
}

// uc_orig: height_above_anything (fallen/Source/collide.cpp)
// Returns how far the given body part (sub-object index) is above the
// nearest surface.  Positive = above, negative = below / clipped through.
// Returns 0 immediately if the person is inside a building.
// Note: the FIND_ANYFACE branch is unreachable due to the hardcoded if(1).
SLONG height_above_anything(Thing* p_person, SLONG body_part, SWORD* onface)
{
    SLONG on_face;
    SLONG fx, fy, fz, new_y;

    *onface = 0;

    if (p_person->Genus.Person->InsideIndex)
        return (0);

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, body_part, &fx, &fy, &fz);

    fx += p_person->WorldPos.X >> 8;
    fy += p_person->WorldPos.Y >> 8;
    fz += p_person->WorldPos.Z >> 8;

    // FIND_ANYFACE branch is dead code — the condition is always true.
    if (1 || p_person->Genus.Person->Ware)
        on_face = find_face_for_this_pos(fx, fy, fz, &new_y, 0, FIND_FACE_NEAR_BELOW);
    else
        on_face = find_face_for_this_pos(fx, fy, fz, &new_y, 0, FIND_ANYFACE);

    if (on_face) {
        *onface = on_face;
        return (fy - new_y);
    } else {
        new_y = PAP_calc_height_at_thing(p_person, fx, fz);
        *onface = 0;
        return (fy - new_y);
    }
}

// uc_orig: plant_feet (fallen/Source/collide.cpp)
// Snaps the person's feet to the nearest surface after movement.
// Foot XZ is zeroed (matches predict_collision_with_face's coordinate
// convention), only foot Y from the animation is used.
// Returns:  1 = snapped to a prim face,
//           0 = dropping (surface too far below),
//          -1 = snapped to terrain floor.
SLONG plant_feet(Thing* p_person)
{
    SLONG on_face;
    SLONG fx, fy, fz, new_y;

    if (p_person->Genus.Person->InsideIndex)
        return (0);

    person_splash(p_person, -1);

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_FOOT, &fx, &fy, &fz);

    // Keep XZ the same as predict_collision_with_face to avoid disagreement.
    fx = 0;
    fz = 0;

    fx += p_person->WorldPos.X >> 8;
    fy += p_person->WorldPos.Y >> 8;
    fz += p_person->WorldPos.Z >> 8;

    on_face = find_face_for_this_pos(fx, fy, fz, &new_y, 0, 0);

    if (on_face == GRAB_FLOOR) {
        MSG_add(" PLANT FEET  on grab floor \n");
        set_feet_to_y(p_person, new_y);
        p_person->OnFace = 0;
        return (-1);
    } else if (on_face) {
        MSG_add(" PLANT FEETa  on face at %d old y %d\n", new_y, p_person->WorldPos.Y >> 8);
        if (new_y < fy - 60) {
            MSG_add(" big drop so fall2 \n");
            p_person->OnFace = 0;
            set_person_drop_down(p_person, PERSON_DROP_DOWN_KEEP_VEL);

            return (0);
        }

        set_feet_to_y(p_person, new_y);
        p_person->OnFace = on_face;
        return (1);
    } else {
        new_y = PAP_calc_height_at_thing(p_person, fx, fz);

        MSG_add(" PLANT FEET  at y %d old y %d %d\n", new_y, p_person->WorldPos.Y >> 8, fy);

        if (new_y < fy - 30) {
            MSG_add(" big drop so fall \n");
            p_person->OnFace = 0;
            set_person_drop_down(p_person, PERSON_DROP_DOWN_KEEP_VEL);

            return (0);
        }

        if (fy > new_y + 20) {
            // Have height so must fall — no-op in this branch.
        } else {
            MSG_add(" small drop so teleport to alt \n");
            p_person->OnFace = 0;
            set_feet_to_y(p_person, new_y);
            return (-1);
        }
    }

    return (0);
}

// ========================================================================
// Character radius queries
// ========================================================================

// uc_orig: get_person_radius (fallen/Source/collide.cpp)
// Returns the collision radius for a person type (for wall sliding).
SLONG get_person_radius(SLONG type)
{
    switch (type) {
    case PERSON_COP:
        return (40);
    case PERSON_DARCI:
        return (30);
    }

    return (50);
}

// uc_orig: get_person_radius2 (fallen/Source/collide.cpp)
// Returns a secondary (tighter) collision radius for a person type.
SLONG get_person_radius2(SLONG type)
{
    switch (type) {
    case PERSON_COP:
        return (40);
    case PERSON_DARCI:
        return (30);
    }

    return (35);
}

// ========================================================================
// Fence height helper
// ========================================================================

// uc_orig: get_fence_height (fallen/Source/collide.cpp)
// Converts a packed fence height field to world units.
// h==2 is a special case that maps to 85 instead of the formula h*64.
SLONG get_fence_height(SLONG h)
{
    if (h == 2)
        return (85);
    else
        return (h * 64);
}

// ========================================================================
// slide_along support
// ========================================================================

// uc_orig: step_back_along_vect (fallen/Source/collide.cpp)
// Walks (x2,z2) backwards along its delta from (x1,z1) until it crosses
// to the required side of the wall segment (vx1,vz1)-(vx2,vz2).
// side_required: 1 = this side (positive cross product), 0 = other side.
void step_back_along_vect(SLONG x1, SLONG z1, SLONG* x2, SLONG* z2, SLONG vx1, SLONG vz1, SLONG vx2, SLONG vz2, SLONG side_required)
{
    SLONG dx, dz;
    SLONG side;

    dx = *x2 - x1;
    dz = *z2 - z1;

    dx >>= 2;
    dz >>= 2;

    dx += SIGN(*x2 - x1);
    dz += SIGN(*z2 - z1);

    while (1) {
        *x2 -= dx;
        *z2 -= dz;
        side = which_side(vx1, vz1, vx2, vz2, *x2, *z2);

        if ((side <= 0 && side_required == 1) || (side >= 0 && side_required == 0) || (side == 0)) {
            // Still on the wrong side — keep stepping.
        } else {
            SLONG norm_x, norm_z, dist, on;
            // Now on the correct side.
            signed_dist_to_line_with_normal_mark(vx1, vz1, vx2, vz2, *x2, *z2, &dist, &norm_x, &norm_z, &on);

            if (side_required == 1) {
                ASSERT(dist >= 0);
            } else {
                ASSERT(dist <= 0);
            }

            return;
        }
    }
}

// uc_orig: EXTRA_RADIUS (fallen/Source/collide.cpp)
// Extra radius added at facet endpoints to avoid corner-clipping.
#define EXTRA_RADIUS 10

// uc_orig: VEC_SHIFT (fallen/Source/collide.cpp)
// Fixed-point shift for normalised wall-normal vectors in slide_along.
#define VEC_SHIFT (17)

// uc_orig: VEC_LENGTH (fallen/Source/collide.cpp)
// Length of normalised vector at VEC_SHIFT precision.
#define VEC_LENGTH (1 << VEC_SHIFT)

// uc_orig: MAX_ALREADY — defined in collide_globals.h to avoid circular include.

// ========================================================================
// NOGO grid collision macros — local to slide_along.
// ========================================================================

// uc_orig: NOGO_COLLIDE_XS (fallen/Source/collide.cpp)
#define NOGO_COLLIDE_XS (1 << 0)
// uc_orig: NOGO_COLLIDE_XL (fallen/Source/collide.cpp)
#define NOGO_COLLIDE_XL (1 << 1)
// uc_orig: NOGO_COLLIDE_ZS (fallen/Source/collide.cpp)
#define NOGO_COLLIDE_ZS (1 << 2)
// uc_orig: NOGO_COLLIDE_ZL (fallen/Source/collide.cpp)
#define NOGO_COLLIDE_ZL (1 << 3)
// uc_orig: NOGO_COLLIDE_SS (fallen/Source/collide.cpp)
#define NOGO_COLLIDE_SS (1 << 4)
// uc_orig: NOGO_COLLIDE_LS (fallen/Source/collide.cpp)
#define NOGO_COLLIDE_LS (1 << 5)
// uc_orig: NOGO_COLLIDE_SL (fallen/Source/collide.cpp)
#define NOGO_COLLIDE_SL (1 << 6)
// uc_orig: NOGO_COLLIDE_LL (fallen/Source/collide.cpp)
#define NOGO_COLLIDE_LL (1 << 7)
// uc_orig: NOGO_COLLIDE_WIDTH (fallen/Source/collide.cpp)
// Sub-mapsquare distance from a NOGO border at which sliding kicks in.
#define NOGO_COLLIDE_WIDTH (0x5800)

// uc_orig: slide_along (fallen/Source/collide.cpp)
// Main wall-sliding function: moves the endpoint (*x2,*y2,*z2) to slide against
// nearby DFacets within radius.  Also applies NOGO-grid repulsion.
// Returns UC_FALSE; collision state is written to the slide_* globals.
SLONG slide_along(
    SLONG x1, SLONG my_y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG extra_wall_height,
    SLONG radius,
    ULONG flags)
{
    SWORD minx;
    SWORD minz;
    SWORD maxx;
    SWORD maxz;

    SWORD y_top;
    SWORD y_bot;

    SWORD mx;
    SWORD mz;

    SLONG fx1;
    SLONG fz1;
    SLONG fx2;
    SLONG fz2;

    SLONG dx;
    SLONG dz;
    SLONG dist;

    SWORD f_list;
    SWORD i_facet;

    UWORD last_slide;

    DFacet* df;
    UBYTE exit;
    UBYTE fence = 0;
    UBYTE reverse;

    slid_along_fence = 0;
    fence_colvect = 0;

    extern UBYTE just_started_falling_off_backwards;

    if (just_started_falling_off_backwards) {
        radius += radius >> 1;
    }

    last_slide = 0;
    last_slide_colvect = NULL;
    slide_door = 0;
    slide_ladder = 0;
    actual_sliding = UC_FALSE;
    slide_into_warehouse = 0;
    slide_outof_warehouse = UC_FALSE;

    // Slide along NOGO-flagged hi-res grid squares.
    {
        UBYTE collide = 0;

        mx = *x2 >> 16;
        mz = *z2 >> 16;

        if (WITHIN(mx, 1, PAP_SIZE_HI - 2) && WITHIN(mz, 1, PAP_SIZE_HI - 2)) {
            if (PAP_2HI(mx - 1, mz).Flags & PAP_FLAG_NOGO) {
                collide |= NOGO_COLLIDE_XS;
            }
            if (PAP_2HI(mx + 1, mz).Flags & PAP_FLAG_NOGO) {
                collide |= NOGO_COLLIDE_XL;
            }
            if (PAP_2HI(mx, mz - 1).Flags & PAP_FLAG_NOGO) {
                collide |= NOGO_COLLIDE_ZS;
            }
            if (PAP_2HI(mx, mz + 1).Flags & PAP_FLAG_NOGO) {
                collide |= NOGO_COLLIDE_ZL;
            }

            if (PAP_2HI(mx - 1, mz - 1).Flags & PAP_FLAG_NOGO) {
                collide |= NOGO_COLLIDE_SS;
            }
            if (PAP_2HI(mx + 1, mz - 1).Flags & PAP_FLAG_NOGO) {
                collide |= NOGO_COLLIDE_LS;
            }
            if (PAP_2HI(mx - 1, mz + 1).Flags & PAP_FLAG_NOGO) {
                collide |= NOGO_COLLIDE_SL;
            }
            if (PAP_2HI(mx + 1, mz + 1).Flags & PAP_FLAG_NOGO) {
                collide |= NOGO_COLLIDE_LL;
            }

            if (flags & SLIDE_ALONG_FLAG_CARRYING) {
                // Don't go up/down even quarter-height blocks.
                if (MAVHEIGHT(mx, mz) != MAVHEIGHT(mx - 1, mz)) {
                    collide |= NOGO_COLLIDE_XS;
                }
                if (MAVHEIGHT(mx, mz) != MAVHEIGHT(mx + 1, mz)) {
                    collide |= NOGO_COLLIDE_XL;
                }
                if (MAVHEIGHT(mx, mz) != MAVHEIGHT(mx, mz - 1)) {
                    collide |= NOGO_COLLIDE_ZS;
                }
                if (MAVHEIGHT(mx, mz) != MAVHEIGHT(mx, mz + 1)) {
                    collide |= NOGO_COLLIDE_ZL;
                }

                if (MAVHEIGHT(mx, mz) != MAVHEIGHT(mx - 1, mz - 1)) {
                    collide |= NOGO_COLLIDE_SS;
                }
                if (MAVHEIGHT(mx, mz) != MAVHEIGHT(mx + 1, mz - 1)) {
                    collide |= NOGO_COLLIDE_LS;
                }
                if (MAVHEIGHT(mx, mz) != MAVHEIGHT(mx - 1, mz + 1)) {
                    collide |= NOGO_COLLIDE_SL;
                }
                if (MAVHEIGHT(mx, mz) != MAVHEIGHT(mx + 1, mz + 1)) {
                    collide |= NOGO_COLLIDE_LL;
                }
            }

            if (collide & NOGO_COLLIDE_XS) {
                if ((*x2 & 0xffff) < NOGO_COLLIDE_WIDTH) {
                    *x2 &= ~0xffff;
                    *x2 |= NOGO_COLLIDE_WIDTH;
                }
            }

            if (collide & NOGO_COLLIDE_XL) {
                if ((*x2 & 0xffff) > 0x10000 - NOGO_COLLIDE_WIDTH) {
                    *x2 &= ~0xffff;
                    *x2 |= 0x10000 - NOGO_COLLIDE_WIDTH;
                }
            }

            if (collide & NOGO_COLLIDE_ZS) {
                if ((*z2 & 0xffff) < NOGO_COLLIDE_WIDTH) {
                    *z2 &= ~0xffff;
                    *z2 |= NOGO_COLLIDE_WIDTH;
                }
            }

            if (collide & NOGO_COLLIDE_ZL) {
                if ((*z2 & 0xffff) > 0x10000 - NOGO_COLLIDE_WIDTH) {
                    *z2 &= ~0xffff;
                    *z2 |= 0x10000 - NOGO_COLLIDE_WIDTH;
                }
            }

            if (collide & (NOGO_COLLIDE_SS | NOGO_COLLIDE_LS | NOGO_COLLIDE_SL | NOGO_COLLIDE_LL)) {
                dx = *x2 & 0xffff;
                dz = *z2 & 0xffff;

                if (collide & (NOGO_COLLIDE_LS | NOGO_COLLIDE_LL)) {
                    dx = 0x10000 - dx;
                }
                if (collide & (NOGO_COLLIDE_SL | NOGO_COLLIDE_LL)) {
                    dz = 0x10000 - dz;
                }

                dist = QDIST2(dx, dz) + 1;

                if (dist < NOGO_COLLIDE_WIDTH) {
                    dx = dx * NOGO_COLLIDE_WIDTH / dist;
                    dz = dz * NOGO_COLLIDE_WIDTH / dist;

                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;

                    if (collide & (NOGO_COLLIDE_LS | NOGO_COLLIDE_LL)) {
                        dx = 0x10000 - dx;
                    }
                    if (collide & (NOGO_COLLIDE_SL | NOGO_COLLIDE_LL)) {
                        dz = 0x10000 - dz;
                    }

                    *x2 |= dx;
                    *z2 |= dz;
                }
            }
        }
    }

    if (COLLIDE_can_i_fastnav(x1 >> 16, z1 >> 16) && COLLIDE_can_i_fastnav(*x2 >> 16, *z2 >> 16)) {
        // Both squares are fastnav — no colvects nearby, skip everything.
        return UC_FALSE;
    }

    // Build the lo-res cell bounding box to search for DFacets.
    radius <<= 1; // Search in twice the radius.

    minx = ((*x2 >> 8)) - radius >> PAP_SHIFT_LO;
    minz = ((*z2 >> 8)) - radius >> PAP_SHIFT_LO;

    maxx = ((*x2 >> 8)) + radius >> PAP_SHIFT_LO;
    maxz = ((*z2 >> 8)) + radius >> PAP_SHIFT_LO;

    radius <<= 7; // Convert radius to 16-bits-per-mapsquare units.

    // Use frame counter to avoid processing the same DFacet twice.
    SUPERMAP_counter_increase(0);

    SATURATE(minx, 0, PAP_SIZE_LO - 1);
    SATURATE(minz, 0, PAP_SIZE_LO - 1);
    SATURATE(maxx, 0, PAP_SIZE_LO - 1);
    SATURATE(maxz, 0, PAP_SIZE_LO - 1);

    for (mx = minx; mx <= maxx; mx++)
        for (mz = minz; mz <= maxz; mz++) {

            exit = UC_FALSE;
            f_list = PAP_2LO(mx, mz).ColVectHead;

            if (!f_list) {
                continue;
            }

            while (!exit) {

                i_facet = facet_links[f_list++];
                ASSERT(i_facet < next_dfacet);

                if (i_facet < 0) {
                    i_facet = -i_facet;
                    exit = UC_TRUE;
                }

                df = &dfacets[i_facet];

                if (df->Counter[0] == SUPERMAP_counter[0]) {
                    // Already visited this DFacet this frame.
                    continue;
                }

                df->Counter[0] = SUPERMAP_counter[0];

                if (df->FacetType == STOREY_TYPE_CABLE) {
                    continue;
                }

                if (df->FacetType == STOREY_TYPE_OUTSIDE_DOOR && (df->FacetFlags & FACET_FLAG_OPEN)) {
                    continue;
                }

                // Determine the Y range this facet blocks.
                if (df->FacetType == STOREY_TYPE_FENCE_FLAT || df->FacetType == STOREY_TYPE_FENCE || df->FacetType == STOREY_TYPE_FENCE_BRICK || df->FacetType == STOREY_TYPE_OUTSIDE_DOOR) {
                    y_top = get_fence_top(x1 >> 8, z1 >> 8, i_facet);
                    y_bot = get_fence_bottom(x1 >> 8, z1 >> 8, i_facet) - 0x48;

                    fence = UC_TRUE;
                } else {
                    fence = 0;
                    y_bot = df->Y[0] - 64;
                    y_top = df->Y[0] + (df->Height * df->BlockHeight << 2);

                    if (df->FHeight || df->FacetFlags & FACET_FLAG_HUG_FLOOR) {
                        // Foundations extend all the way to the bottom — can never walk under them.
                        y_bot = -0x7fff;
                    }
                }

                SLONG ignore_this_facet = UC_FALSE;

                if (just_started_falling_off_backwards) {
                    // Only collide with tall normal facets when falling off backwards.
                    ignore_this_facet = UC_TRUE;

                    if (df->FacetType == STOREY_TYPE_NORMAL) {
                        if (y_top > ((my_y1 >> 8) + 0x80)) {
                            ignore_this_facet = UC_FALSE;
                            y_bot -= 0x100;
                        }
                    }
                }

                // Extra height kludge: lets the player step over low walls.
                y_top += extra_wall_height;

                if (WITHIN(my_y1 >> 8, y_bot, y_top) && !ignore_this_facet) {

                    fx1 = df->x[0] << 16;
                    fz1 = df->z[0] << 16;
                    fx2 = df->x[1] << 16;
                    fz2 = df->z[1] << 16;

                    if (fx1 == fx2) {
                        // Vertical (Z-axis-aligned) facet.
                        reverse = UC_FALSE;

                        if (fz1 > fz2) {
                            SWAP(fz1, fz2);
                            reverse = UC_TRUE;
                        }

                        if (WITHIN(*z2, fz1 - radius, fz2 + radius) && (WITHIN(*x2, fx1 - radius, fx2 + radius) || (x1 < fx1 && *x2 > fx1) || (x1 > fx1 && *x2 < fx1))) {
                            if (*z2 < fz1 && z1 < fz1) {
                                // Near the fz1 endpoint — push away from it.
                                dx = *x2 - fx1;
                                dz = *z2 - fz1;

                                dist = QDIST2(abs(dx), abs(dz));

                                if (dist < radius) {
                                    *x2 = fx1 + dx * (radius >> 8) / ((dist >> 8) + 1);
                                    *z2 = fz1 + dz * (radius >> 8) / ((dist >> 8) + 1);
                                }
                            } else if (*z2 > fz2 && z1 > fz2) {
                                // Near the fz2 endpoint — push away from it.
                                dx = *x2 - fx2;
                                dz = *z2 - fz2;

                                dist = QDIST2(abs(dx), abs(dz));

                                if (dist < radius) {
                                    *x2 = fx2 + dx * (radius >> 8) / ((dist >> 8) + 1);
                                    *z2 = fz2 + dz * (radius >> 8) / ((dist >> 8) + 1);
                                }
                            } else {
                                if (df->FacetType == STOREY_TYPE_DOOR && !(flags & SLIDE_ALONG_FLAG_JUMPING)) {
                                    // Walking through a door: update the appropriate warehouse/door globals
                                    // and return immediately (no slide).
                                    if (reverse) {
                                        if (*x2 > fx1) {
                                            if (dbuildings[df->Building].Type == BUILDING_TYPE_WAREHOUSE) {
                                                slide_into_warehouse = df->Building;
                                            } else {
                                                slide_door = df->DStorey;
                                            }
                                        } else {
                                            if (dbuildings[df->Building].Type == BUILDING_TYPE_WAREHOUSE) {
                                                slide_outof_warehouse = UC_TRUE;
                                            }
                                        }
                                    } else {
                                        if (*x2 < fx1) {
                                            if (dbuildings[df->Building].Type == BUILDING_TYPE_WAREHOUSE) {
                                                slide_into_warehouse = df->Building;
                                            } else {
                                                slide_door = df->DStorey;
                                            }
                                        } else {
                                            if (dbuildings[df->Building].Type == BUILDING_TYPE_WAREHOUSE) {
                                                slide_outof_warehouse = UC_TRUE;
                                            }
                                        }
                                    }

                                    return UC_FALSE;
                                }

                                if (x1 < fx1) {
                                    *x2 = fx1 - radius;
                                } else {
                                    *x2 = fx1 + radius;
                                }
                            }

                            actual_sliding = UC_TRUE;
                            last_slide_colvect = i_facet;

                            if (fence) {
                                slid_along_fence = UC_TRUE;
                                fence_colvect = i_facet;
                            }

                            if (df->FacetType == STOREY_TYPE_LADDER) {
                                slide_ladder = i_facet;
                            }
                        }
                    } else {
                        ASSERT(fz1 == fz2);

                        // Horizontal (X-axis-aligned) facet.
                        reverse = UC_FALSE;

                        if (fx1 > fx2) {
                            SWAP(fx1, fx2);
                            reverse = UC_TRUE;
                        }

                        if (WITHIN(*x2, fx1 - radius, fx2 + radius) && (WITHIN(*z2, fz1 - radius, fz2 + radius) || (z1 < fz1 && *z2 > fz1) || (z1 > fz1 && *z2 < fz1))) {
                            if (*x2 < fx1 && x1 < fx1) {
                                // Near the fx1 endpoint — push away from it.
                                dx = *x2 - fx1;
                                dz = *z2 - fz1;

                                dist = QDIST2(abs(dx), abs(dz));

                                if (dist < radius) {
                                    *x2 = fx1 + dx * (radius >> 8) / ((dist >> 8) + 1);
                                    *z2 = fz1 + dz * (radius >> 8) / ((dist >> 8) + 1);
                                }
                            } else if (*x2 > fx2 && x1 > fx2) {
                                // Near the fx2 endpoint — push away from it.
                                dx = *x2 - fx2;
                                dz = *z2 - fz2;

                                dist = QDIST2(abs(dx), abs(dz));

                                if (dist < radius) {
                                    *x2 = fx2 + dx * (radius >> 8) / ((dist >> 8) + 1);
                                    *z2 = fz2 + dz * (radius >> 8) / ((dist >> 8) + 1);
                                }
                            } else {
                                if (df->FacetType == STOREY_TYPE_DOOR && !(flags & SLIDE_ALONG_FLAG_JUMPING)) {
                                    if (reverse) {
                                        if (*z2 < fz1) {
                                            if (dbuildings[df->Building].Type == BUILDING_TYPE_WAREHOUSE) {
                                                slide_into_warehouse = df->Building;
                                            } else {
                                                slide_door = df->DStorey;
                                            }
                                        } else {
                                            if (dbuildings[df->Building].Type == BUILDING_TYPE_WAREHOUSE) {
                                                slide_outof_warehouse = UC_TRUE;
                                            }
                                        }
                                    } else {
                                        if (*z2 > fz1) {
                                            if (dbuildings[df->Building].Type == BUILDING_TYPE_WAREHOUSE) {
                                                slide_into_warehouse = df->Building;
                                            } else {
                                                slide_door = df->DStorey;
                                            }
                                        } else {
                                            if (dbuildings[df->Building].Type == BUILDING_TYPE_WAREHOUSE) {
                                                slide_outof_warehouse = UC_TRUE;
                                            }
                                        }
                                    }

                                    return UC_FALSE;
                                }

                                if (z1 < fz1) {
                                    *z2 = fz1 - radius;
                                } else {
                                    *z2 = fz1 + radius;
                                }
                            }

                            actual_sliding = UC_TRUE;
                            last_slide_colvect = i_facet;

                            if (fence) {
                                slid_along_fence = UC_TRUE;
                                fence_colvect = i_facet;
                            }

                            if (df->FacetType == STOREY_TYPE_LADDER) {
                                slide_ladder = i_facet;
                            }
                        }
                    }
                }
            }
        }

    if (slid_along_fence) {
        // Always report fence as the last collider so the fence-climbing code triggers.
        last_slide_colvect = fence_colvect;
    }

    return UC_FALSE;
}

// uc_orig: cross_door (fallen/Source/collide.cpp)
// Tests whether a movement from (x1,y1,z1) to (x2,y2,z2) crosses a door facet:
//   returns -1 = blocked inside, 0 = no crossing, 1 = exited building.
SLONG cross_door(SLONG x1, SLONG my_y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2, SLONG radius)
{
    SLONG i;

    SLONG minx;
    SLONG minz;
    SLONG maxx;
    SLONG maxz;

    SLONG y_top;
    SLONG y_bot;

    SLONG mx;
    SLONG mz;

    SLONG dist;
    SLONG norm_x;
    SLONG norm_z;
    SLONG on;

    SLONG v_list;
    SLONG i_vect;

    SLONG num_slides;
    SLONG last_slide;

    DFacet* p_vect;

    SLONG already_upto = 0;

    x1 >>= 8;
    z1 >>= 8;
    my_y1 >>= 8;

    x2 = (x2) >> 8;
    y2 = (y2) >> 8;
    z2 = (z2) >> 8;

    minx = ((x2)) - radius >> PAP_SHIFT_LO;
    minz = ((z2)) - radius >> PAP_SHIFT_LO;

    maxx = ((x2)) + radius >> PAP_SHIFT_LO;
    maxz = ((z2)) + radius >> PAP_SHIFT_LO;

    num_slides = 0;
    last_slide = 0;
    last_slide_colvect = NULL;
    slide_door = 0;
    actual_sliding = UC_FALSE;

    for (mx = minx; mx <= maxx; mx++)
        for (mz = minz; mz <= maxz; mz++) {
            SLONG fence = 0;
            if (WITHIN(mx, 0, PAP_SIZE_LO - 1) && WITHIN(mz, 0, PAP_SIZE_LO - 1)) {
                SLONG exit = 0;

                v_list = PAP_2LO(mx, mz).ColVectHead;

                if (v_list)
                    while (!exit) {
                        i_vect = facet_links[v_list];
                        if (i_vect < 0) {
                            i_vect = -i_vect;
                            exit = 1;
                        }
                        p_vect = &dfacets[i_vect];

                        if (p_vect->FacetType == STOREY_TYPE_DOOR) {
                            for (i = already_upto - 1; i >= 0; i--) {
                                if (already[i] == i_vect) {
                                    goto next_colvect;
                                }
                            }

                            if (already_upto < MAX_ALREADY) {
                                ASSERT(WITHIN(already_upto, 0, MAX_ALREADY - 1));

                                already[already_upto++] = i_vect;
                            }

                            {
                                SLONG vect_y;

                                {
                                    {
                                        SLONG height;

                                        height = (p_vect->Height * p_vect->BlockHeight) << 2;
                                        fence = 0;
                                        vect_y = p_vect->Y[0];

                                        y_bot = vect_y - 64;
                                        y_top = vect_y + height;

                                        if (p_vect->FacetType != STOREY_TYPE_CABLE)
                                            if (p_vect->FHeight) {
                                                y_bot = -UC_INFINITY;
                                            }
                                    }
                                }
                            }

                            if (WITHIN(my_y1, y_bot, y_top)) {
                                SLONG side;
                                SLONG start_index = 0, end_index = 1;

                                signed_dist_to_line_with_normal_mark(
                                    p_vect->x[start_index] << 8, p_vect->z[start_index] << 8,
                                    p_vect->x[end_index] << 8, p_vect->z[end_index] << 8,
                                    x2, z2,
                                    &dist,
                                    &norm_x,
                                    &norm_z,
                                    &on);

                                {
                                    if (!on) {

                                    } else {
                                        side = which_side(
                                            p_vect->x[start_index] << 8, p_vect->z[start_index] << 8,
                                            p_vect->x[end_index] << 8, p_vect->z[end_index] << 8,
                                            x1, z1);

                                        if (dist < 0 && dist > -60 && side < 0) {
                                            return (-1);
                                        }
                                        if (dist < 0 && side > 0) {
                                            ASSERT(0);
                                            return (-1);
                                        } else if (dist > 0 && side < 0) {
                                            return (1);
                                        }
                                    }
                                }
                            }
                        }

                    next_colvect:;

                        v_list++;
                    }
            }
        }

    return 0;
}

// uc_orig: bump_person (fallen/Source/collide.cpp)
// Pushes (*x2,*y2,*z2) back to start when two people's radii would overlap,
// and sets p_person->InWay to the blocking thing index.
SLONG bump_person(Thing* p_person, THING_INDEX index, SLONG x1, SLONG my_y1, SLONG z1, SLONG* x2, SLONG* y2, SLONG* z2)
{
    SLONG bump_radius, my_radius;
    Thing* p_bumped;

    SLONG ddx, ddy, ddz, dist;

    SLONG ex, ey, ez;

    ex = *x2 >> 8;
    ey = *y2 >> 8;
    ez = *z2 >> 8;

    p_bumped = TO_THING(index);

    bump_radius = get_person_radius(p_bumped->Genus.Person->PersonType);
    my_radius = get_person_radius(p_person->Genus.Person->PersonType);

    x1 >>= 8;
    my_y1 >>= 8;
    z1 >>= 8;

    *x2 >>= 8;
    *y2 >>= 8;
    *z2 >>= 8;

    ddx = abs(ex - (p_bumped->WorldPos.X >> 8));
    ddy = abs(ey - (p_bumped->WorldPos.Y >> 8));
    ddz = abs(ez - (p_bumped->WorldPos.Z >> 8));

    dist = QDIST2(ddx, ddz);

    if (dist < (bump_radius + my_radius) && ddy < 150) {
        SLONG dist2;
        SLONG odx, odz;

        odx = abs((x1) - (p_bumped->WorldPos.X >> 8));
        odz = abs((z1) - (p_bumped->WorldPos.Z >> 8));

        dist2 = QDIST2(odx, odz);

        if (dist < dist2) {
            SLONG angle;

            angle = Arctan(-ddx, ddz) + 1024;

            *x2 = x1;
            *z2 = z1;

            p_person->Genus.Person->InWay = index;

            *x2 <<= 8;
            *y2 <<= 8;
            *z2 <<= 8;

            return (0);
        }
    }

    *x2 <<= 8;
    *y2 <<= 8;
    *z2 <<= 8;

    return (0);
}

// uc_orig: EDGE_WIDTH (fallen/Source/collide.cpp)
// Sub-mapsquare half-width margin used by slide_along_redges.
#define EDGE_WIDTH 0x4000

// uc_orig: slide_along_edges (fallen/Source/collide.cpp)
// Constrains (*x2,*z2) to stay inside the walkable prim quad face4 by
// sliding against each edge flagged FACE_FLAG_SLIDE_EDGE.
void slide_along_edges(
    SLONG face4,
    SLONG x1, SLONG z1,
    SLONG* x2, SLONG* z2)
{
    SLONG i;
    SLONG p1;
    SLONG p2;

    SLONG ex1, ez1;
    SLONG ex2, ez2;

    SLONG dex;
    SLONG dez;

    SLONG elen;

    SLONG vecx;
    SLONG vecz;

    SLONG ox;
    SLONG oz;

    UBYTE point_order[4] = { 0, 1, 3, 2 };

    ASSERT(WITHIN(face4, 1, next_prim_face4));

    PrimFace4* f4 = &prim_faces4[face4];

    ox = 0;
    oz = 0;

    for (i = 0; i < 4; i++) {
        if (f4->FaceFlags & (FACE_FLAG_SLIDE_EDGE << i)) {
            p1 = f4->Points[point_order[(i + 0) & 0x3]];
            p2 = f4->Points[point_order[(i + 1) & 0x3]];

            ex1 = prim_points[p1].X + ox;
            ez1 = prim_points[p1].Z + oz;

            ex2 = prim_points[p2].X + ox;
            ez2 = prim_points[p2].Z + oz;

            dex = ex2 - ex1;
            dez = ez2 - ez1;

            vecx = (*x2 >> 8) - ex1;
            vecz = (*z2 >> 8) - ez1;

            cprod = dex * vecz - dez * vecx;

            if (cprod < 0) {
                // Endpoint is outside this edge; project it back onto the face.
                elen = QDIST2(abs(dex), abs(dez));
                cprod = (cprod * 256) / (elen * elen);

                *x2 += dez * cprod;
                *z2 -= dex * cprod;

                // Move slightly away from the edge to prevent sticking.
                *x2 -= SIGN(dez) << (3 + 8);
                *z2 += SIGN(dex) << (3 + 8);
            }
        }
    }
}

// uc_orig: slide_along_redges (fallen/Source/collide.cpp)
// Constrains (*x2,*z2) to stay inside a roof quad face by clamping against
// each edge marked RFACE_FLAG_SLIDE_EDGE, or against MAV height transitions
// for hidden (flat-terrain) roof faces.
void slide_along_redges(
    SLONG face4,
    SLONG x1, SLONG z1,
    SLONG* x2, SLONG* z2)
{
    SLONG i;

    SLONG mx;
    SLONG mz;

    SLONG height1;
    SLONG height2;

    SLONG hard_edge;

    RoofFace4* f4;

    if (IS_ROOF_HIDDEN_FACE(-face4)) {
        f4 = NULL;
        mx = ROOF_HIDDEN_X(-face4);
        mz = ROOF_HIDDEN_Z(-face4);

        mx = *x2 >> 16;
        mz = *z2 >> 16;

        height1 = MAVHEIGHT(mx, mz);
    } else {
        f4 = &roof_faces4[face4];
        mx = f4->RX & 127;
        mz = f4->RZ & 127;
        height1 = 0;
    }

    for (i = 0; i < 4; i++) {

        hard_edge = UC_FALSE;

        if (f4) {
            hard_edge = f4->DrawFlags & (RFACE_FLAG_SLIDE_EDGE << i);
        } else {
            const struct
            {
                SBYTE dx;
                SBYTE dz;

            } dir[4] = {
                { 0, -1 },
                { 1, 0 },
                { 0, 1 },
                { -1, 0 }
            };

            if (WITHIN(mx + dir[i].dx, 0, PAP_SIZE_HI - 1) && WITHIN(mz + dir[i].dz, 0, PAP_SIZE_HI - 1)) {
                height2 = MAVHEIGHT(mx + dir[i].dx, mz + dir[i].dz);

                if (abs(height1 - height2) > 1) {
                    hard_edge = UC_TRUE;
                }
            }
        }

        if (hard_edge) {
            switch (i) {
            case 0: // ZS
                if ((*z2 & 0xffff) < EDGE_WIDTH) {
                    *z2 &= ~0xffff;
                    *z2 |= EDGE_WIDTH;
                }
                break;

            case 1: // XL
                if ((*x2 & 0xffff) > 0x10000 - EDGE_WIDTH) {
                    *x2 &= ~0xffff;
                    *x2 |= 0x10000 - EDGE_WIDTH;
                }
                break;

            case 2: // ZL
                if ((*z2 & 0xffff) > 0x10000 - EDGE_WIDTH) {
                    *z2 &= ~0xffff;
                    *z2 |= 0x10000 - EDGE_WIDTH;
                }
                break;

            case 3: // XS
                if ((*x2 & 0xffff) < EDGE_WIDTH) {
                    *x2 &= ~0xffff;
                    *x2 |= EDGE_WIDTH;
                }
                break;

            default:
                ASSERT(0);
                break;
            }
        }
    }
}

// ============================================================================
// Chunk 4: move_thing_quick, collide_against_objects, collide_against_things,
//          drop_on_heads, move_thing
// ============================================================================

// Temporary: forward declarations for functions not yet migrated to new/
// collide_with_circle is defined in old/fallen/Source/collide.cpp (chunk 6+)
SLONG collide_with_circle(SLONG cx, SLONG cz, SLONG cradius, SLONG* x2, SLONG* z2);

// uc_orig: move_thing_quick (fallen/Source/collide.cpp)
// Moves a thing by (dx,dy,dz) without any collision detection.
// Simply adds the delta to WorldPos and updates MapWho lists.
ULONG move_thing_quick(SLONG dx, SLONG dy, SLONG dz, Thing* p_thing)
{
    GameCoord new_position;

    new_position.X = p_thing->WorldPos.X + dx;
    new_position.Y = p_thing->WorldPos.Y + dy;
    new_position.Z = p_thing->WorldPos.Z + dz;

    move_thing_on_map(p_thing, &new_position);
    return (0);
}

// uc_orig: collide_against_objects (fallen/Source/collide.cpp)
// Slides (x2,y2,z2) against all OB-placed objects (street furniture: lamps, bins, etc.)
// near the destination point. Returns UC_TRUE if any collision occurred.
// Ignores the OB object whose prim face the thing is currently standing on.
SLONG collide_against_objects(
    Thing* p_thing,
    SLONG radius,
    SLONG x1, SLONG my_y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2)
{
    OB_Info* oi;

    SLONG old_x2;
    SLONG old_y2;
    SLONG old_z2;

    SLONG mx;
    SLONG mz;

    SLONG y_top;

    SLONG xmin, zmin;
    SLONG xmax, zmax;

    SLONG ans;

    PrimInfo* pi;

    ans = UC_FALSE;

    xmin = (*x2 >> 8) - 0x180 >> PAP_SHIFT_LO;
    zmin = (*z2 >> 8) - 0x180 >> PAP_SHIFT_LO;

    xmax = (*x2 >> 8) + 0x180 >> PAP_SHIFT_LO;
    zmax = (*z2 >> 8) + 0x180 >> PAP_SHIFT_LO;

    SATURATE(xmin, 0, PAP_SIZE_LO - 1);
    SATURATE(zmin, 0, PAP_SIZE_LO - 1);

    SATURATE(xmax, 0, PAP_SIZE_LO - 1);
    SATURATE(zmax, 0, PAP_SIZE_LO - 1);

    // If standing on a prim face, don't collide with that prim.
    SLONG ignore_prim = NULL;

    if (p_thing->OnFace > 0) {
        if (prim_faces4[p_thing->OnFace].FaceFlags & FACE_FLAG_PRIM) {
            // ThingIndex in this case holds -(ob_index).
            ignore_prim = -prim_faces4[p_thing->OnFace].ThingIndex;
        }
    }

    for (mx = xmin; mx <= xmax; mx++)
        for (mz = zmin; mz <= zmax; mz++) {
            for (oi = OB_find(mx, mz); oi->prim; oi++) {
                if (oi->index == ignore_prim) {
                    continue;
                }

                switch (prim_get_collision_model(oi->prim)) {
                case PRIM_COLLIDE_BOX:
                case PRIM_COLLIDE_SMALLBOX:

                    old_x2 = *x2;
                    old_y2 = *y2;
                    old_z2 = *z2;

                    if (slide_along_prim(
                            oi->prim,
                            oi->x,
                            oi->y,
                            oi->z,
                            oi->yaw,
                            x1, my_y1, z1,
                            x2, y2, z2,
                            radius,
                            prim_get_collision_model(oi->prim) == PRIM_COLLIDE_SMALLBOX,
                            UC_FALSE)) {
                        ans = UC_TRUE;

                        if (p_thing) {
                            if (p_thing->Class == CLASS_PERSON) {
                                if (p_thing->Genus.Person->PlayerID) {
                                    // Player walking backwards: check if they can sit on this prim.
                                    if (p_thing->SubState == SUB_STATE_WALKING_BACKWARDS) {
                                        if (oi->prim == 105 || oi->prim == 101 || oi->prim == 110 || oi->prim == 89 || oi->prim == 126 || oi->prim == 95 || oi->prim == 102) {
                                            SLONG dangle = oi->yaw - p_thing->Draw.Tweened->Angle;

                                            dangle &= 2047;

                                            if (dangle > 1024) {
                                                dangle -= 2048;
                                            }

                                            if (abs(dangle) < 220) {
                                                set_person_sit_down(p_thing);
                                                return ans;
                                            }
                                        }
                                    }
                                } else {
                                    // NPC: steer away from the prim.
                                    SLONG angle;
                                    SLONG dangle;

                                    angle = p_thing->Draw.Tweened->Angle;
                                    dangle = OB_avoid(
                                        oi->x,
                                        oi->y,
                                        oi->z,
                                        oi->yaw,
                                        oi->prim,
                                        x1, z1,
                                        *x2, *z2);

                                    angle += dangle * 256;
                                    angle &= 2047;

                                    p_thing->Draw.Tweened->Angle = angle;
                                }
                            }
                        }
                    }

                    break;

                case PRIM_COLLIDE_NONE:
                    break;

                case PRIM_COLLIDE_CYLINDER:

                    pi = get_prim_info(oi->prim);

                    y_top = oi->y + pi->maxy;

                    if (oi->prim == PRIM_OBJ_SPIKE) {
                        y_top = 0xc0;
                    }

                    if ((my_y1 >> 8) < y_top) {
                        if (slide_around_circle(
                                oi->x << 8,
                                oi->z << 8,
                                radius + 0x40 << 8,
                                x1, z1, x2, z2)) {
                            ans = UC_TRUE;

                            if (p_thing) {
                                if (p_thing->Class == CLASS_PERSON) {
                                    if (oi->prim == PRIM_OBJ_SPIKE) {
                                        set_person_dead(
                                            p_thing,
                                            NULL,
                                            PERSON_DEATH_TYPE_OTHER,
                                            UC_FALSE,
                                            0);
                                    }

                                    if (p_thing->Genus.Person->PlayerID) {
                                        // Player walking backwards: check if they can sit.
                                        if (p_thing->SubState == SUB_STATE_WALKING_BACKWARDS) {
                                            if (oi->prim == 70 || oi->prim == 30 || oi->prim == 208 || oi->prim == 215 || oi->prim == 250 || oi->prim == 251 || oi->prim == 252) {
                                                SLONG dx = oi->x - (p_thing->WorldPos.X >> 8);
                                                SLONG dz = oi->z - (p_thing->WorldPos.Z >> 8);

                                                SLONG lx = SIN(p_thing->Draw.Tweened->Angle) >> 8;
                                                SLONG lz = COS(p_thing->Draw.Tweened->Angle) >> 8;

                                                SLONG len = QDIST2(abs(dx), abs(dz)) + 1;

                                                dx = dx * 256 / len;
                                                dz = dz * 256 / len;

                                                SLONG dprod = lx * dx + dz * lz >> 8;

                                                if (dprod > 200) {
                                                    set_person_sit_down(p_thing);
                                                    return ans;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    break;

                default:
                    ASSERT(0);
                    break;
                }
            }
        }

    return ans;
}

// uc_orig: collide_against_things (fallen/Source/collide.cpp)
// Slides (x2,y2,z2) against nearby Things: persons, vehicles, furniture, pyro, bats.
// Returns UC_TRUE if any collision occurred.
SLONG collide_against_things(
    Thing* p_thing,
    SLONG radius,
    SLONG x1, SLONG my_y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2)
{
    UBYTE i;
    UBYTE col_with_upto;
    Thing* col_thing;
    ULONG collide_types;
    SLONG ans;

    SLONG on;

    ans = UC_FALSE;

    on = person_is_on(p_thing);

    if (on == PERSON_ON_PRIM || on == PERSON_ON_METAL) {
        // On a prim/metal surface: only collide with other persons.
        collide_types = (1 << CLASS_PERSON);
    } else {
        collide_types = (1 << CLASS_PERSON) | (1 << CLASS_FURNITURE) | (1 << CLASS_VEHICLE) | (1 << CLASS_ANIM_PRIM) | (1 << CLASS_PYRO) | (1 << CLASS_PLAT) | (1 << CLASS_BAT) | (1 << CLASS_BIKE);
    }

    col_with_upto = THING_find_sphere(
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Y >> 8,
        p_thing->WorldPos.Z >> 8,
        radius + 0x1a0,
        col_with_things,
        MAX_COL_WITH,
        collide_types);

    ASSERT(col_with_upto <= MAX_COL_WITH);

    for (i = 0; i < col_with_upto; i++) {
        col_thing = TO_THING(col_with_things[i]);

        if (col_thing->State == STATE_DEAD || col_thing->State == STATE_DYING) {
            if (col_thing->Class == CLASS_VEHICLE) {
                // Dead cars still block movement.
            } else {
                continue;
            }
        }

        switch (col_thing->Class) {
        case CLASS_PERSON:

            if (col_thing != p_thing) {
                if (p_thing->SubState == SUB_STATE_RUNNING_SKID_STOP) {
                    SLONG tx2, tz2;
                    SLONG radius;

                    if (p_thing->Genus.Person->PlayerID)
                        radius = 66 << 8;
                    else
                        radius = 50 << 8;
                    /*
                                                    {
                                                            SLONG	cx,cz,cy,r,ang,x1,z1,x2,z2;
                                                            cx=col_thing->WorldPos.X>>8;
                                                            cy=col_thing->WorldPos.Y>>8;
                                                            cz=col_thing->WorldPos.Z>>8;
                                                            r=radius>>8;

                                                            x1=cx+(r*COS(0)>>16);
                                                            z1=cz+(r*SIN(0)>>16);
                                                            for(ang=0;ang<2048;ang+=128)
                                                            {
                                                                    x2=cx+(r*COS(ang)>>16);
                                                                    z2=cz+(r*SIN(ang)>>16);

                                                                    add_debug_line(x1,cy+10,z1,x2,cy+10,z2,0xffffff);
                                                                    x1=x2;
                                                                    z1=z2;

                                                            }
                                                    }

                                                    add_debug_line(x1,10,z1,col_thing->WorldPos.X>>8,10,col_thing->WorldPos.Z>>8,0xff00ff);
                    */

                    tx2 = *x2;
                    tz2 = *z2;
                    {
                        if (slide_around_circle(
                                col_thing->WorldPos.X,
                                col_thing->WorldPos.Z,
                                radius,
                                x1, z1, &tx2, &tz2)) {
                            sweep_feet(col_thing, p_thing, 0);
                            ans = UC_TRUE;
                        }
                    }
                } else if (p_thing->SubState == SUB_STATE_STEP_FORWARD) {
                    // During step-forward (attack), stop completely instead of sliding.
                    if (collide_with_circle(
                            col_thing->WorldPos.X,
                            col_thing->WorldPos.Z,
                            50 << 8,
                            x2, z2)) {
                        *x2 = x1;
                        *z2 = z1;
                        ans = UC_TRUE;
                    }
                } else {
                    if (p_thing->Genus.Person->pcom_ai == PCOM_AI_CIV && col_thing->Genus.Person->pcom_ai == PCOM_AI_CIV && col_thing < p_thing) {
                        // Ordering on civs to prevent deadlock at lamp-posts.
                    } else if (col_thing->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING && col_thing->Genus.Person->pcom_ai_arg == THING_NUMBER(p_thing)) {
                        // Don't collide with followers.
                    } else {
                        if (slide_around_circle(
                                col_thing->WorldPos.X,
                                col_thing->WorldPos.Z,
                                50 << 8,
                                x1, z1, x2, z2)) {
                            ans = UC_TRUE;
                        }
                    }
                }
            }

            break;

        case CLASS_PLAT:

            if (slide_along_prim(
                    col_thing->Draw.Mesh->ObjectId,
                    col_thing->WorldPos.X >> 8,
                    col_thing->WorldPos.Y >> 8,
                    col_thing->WorldPos.Z >> 8,
                    col_thing->Draw.Mesh->Angle,
                    x1, my_y1, z1,
                    x2, y2, z2,
                    radius,
                    UC_FALSE,
                    UC_FALSE)) {
                ans = UC_TRUE;
            }

            break;

        case CLASS_VEHICLE:

        {
            SLONG prim;

            if (p_thing->Genus.Person->InCar == THING_NUMBER(col_thing)) {
                // Already in this car, skip.
            } else {
                prim = get_vehicle_body_prim(col_thing->Genus.Vehicle->Type);

                if (slide_along_prim(
                        prim,
                        col_thing->WorldPos.X >> 8,
                        col_thing->WorldPos.Y - get_vehicle_body_offset(col_thing->Genus.Vehicle->Type) >> 8,
                        col_thing->WorldPos.Z >> 8,
                        col_thing->Genus.Vehicle->Angle,
                        x1, my_y1, z1,
                        x2, y2, z2,
                        radius,
                        UC_FALSE,
                        UC_FALSE)) {
                    ans = UC_TRUE;

                    if (p_thing && p_thing->Class == CLASS_PERSON) {
                        if (col_thing->Velocity > 200) {
                            /*

                            Thing *p_driver = get_vehicle_driver(col_thing);

                            knock_person_down(
                                    p_thing,
                                    100 + (col_thing->Velocity >> 4),
                                    col_thing->WorldPos.X >> 8,
                                    col_thing->WorldPos.Z >> 8,
                                    p_driver);

                            MFX_play_thing(THING_NUMBER(p_thing),S_THUMP_SQUISH,MFX_REPLACE,p_thing);

                            return;

                            */

                        } else {
                            if (!p_thing->Genus.Person->PlayerID) {
                                // NPC: steer away from vehicle.
                                SLONG angle;
                                SLONG dangle;

                                angle = p_thing->Draw.Tweened->Angle;
                                dangle = OB_avoid(
                                    col_thing->WorldPos.X >> 8,
                                    col_thing->WorldPos.Y >> 8,
                                    col_thing->WorldPos.Z >> 8,
                                    col_thing->Genus.Vehicle->Angle,
                                    prim,
                                    x1, z1,
                                    *x2, *z2);

                                angle += dangle * 160;
                                angle &= 2047;

                                p_thing->Draw.Tweened->Angle = angle;
                            }
                        }
                    }
                }
            }
        }

        break;
        case CLASS_PYRO:
            switch (col_thing->Genus.Pyro->PyroType) {
            case PYRO_BONFIRE:
            case PYRO_FLICKER:
                if (p_thing->Class == CLASS_PERSON) {
                    if ((p_thing->Genus.Person->Flags & FLAG2_PERSON_INVULNERABLE) || (p_thing->Genus.Person->pcom_bent & PCOM_BENT_PLAYERKILL)) {
                        // Invulnerable or marked for player-kill: don't catch fire.
                    } else {
                        SLONG dist2;
                        SLONG odx, ody, odz;

                        {
                            odx = abs((p_thing->WorldPos.X >> 8) - (col_thing->WorldPos.X >> 8));
                            odz = abs((p_thing->WorldPos.Z >> 8) - (col_thing->WorldPos.Z >> 8));

                            dist2 = QDIST2(odx, odz);

                            if (p_thing->Genus.Person->pcom_ai == PCOM_AI_CIV || p_thing->Genus.Person->pcom_move == PCOM_MOVE_WANDER) {
                                if (dist2 < 200) {
                                    // Wandering civs flee instead of catching fire.
                                    PCOM_set_person_ai_flee_place(
                                        p_thing,
                                        col_thing->WorldPos.X >> 8,
                                        col_thing->WorldPos.Z >> 8);

                                    return ans;
                                }
                            }

                            if (dist2 < 100) {
                                SLONG fx;
                                SLONG fy;
                                SLONG fz;

                                calc_sub_objects_position(
                                    p_thing,
                                    p_thing->Draw.Tweened->AnimTween,
                                    SUB_OBJECT_LEFT_FOOT,
                                    &fx,
                                    &fy,
                                    &fz);

                                fy += p_thing->WorldPos.Y >> 8;

                                ody = abs(fy - (col_thing->WorldPos.Y >> 8));

                                if (ody < 0x50) {
                                    p_thing->Flags |= FLAGS_BURNING;
                                }
                            }
                        }
                    }
                }
            }
            break;

        case CLASS_BAT:

            if (col_thing->Genus.Bat->type == BAT_TYPE_BALROG) {
                SLONG dy = col_thing->WorldPos.Y - my_y1 >> 8;

                if (abs(dy) < 0x180) {
                    SLONG dx = abs(col_thing->WorldPos.X - x1 >> 8);
                    SLONG dz = abs(col_thing->WorldPos.Z - z1 >> 8);

                    if (QDIST2(dx, dz) < 0x100) {
                        if (p_thing->State != STATE_JUMPING && p_thing->State != STATE_DANGLING) {
                            knock_person_down(
                                p_thing,
                                50,
                                col_thing->WorldPos.X >> 8,
                                col_thing->WorldPos.Z >> 8,
                                col_thing);
                        }
                    }
                }
            }

            break;

        default:
            ASSERT(0);
            break;
        }
    }

    return ans;
}

// uc_orig: drop_on_heads (fallen/Source/collide.cpp)
// If p_thing is falling fast (DY < -15000), check if their left foot lands on
// someone's head and kill that person.
void drop_on_heads(Thing* p_thing)
{
    UBYTE i;
    UBYTE col_with_upto;
    UBYTE collide_or_not;
    Thing* col_thing;
    ULONG collide_types;
    SLONG fx, fy, fz, hx, hy, hz;

    if (p_thing->DY > -15000)
        return;

    collide_types = (1 << CLASS_PERSON);

    calc_sub_objects_position(p_thing, p_thing->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_FOOT, &fx, &fy, &fz);
    fx += p_thing->WorldPos.X >> 8;
    fy += p_thing->WorldPos.Y >> 8;
    fz += p_thing->WorldPos.Z >> 8;

    col_with_upto = THING_find_sphere(
        fx,
        fy,
        fz,
        200,
        col_with_things,
        MAX_COL_WITH,
        collide_types | (1 << 31));

    ASSERT(col_with_upto <= MAX_COL_WITH);

    for (i = 0; i < col_with_upto; i++) {
        SLONG dx, dy, dz;
        col_thing = TO_THING(col_with_things[i]);

        ASSERT(col_thing->Class == CLASS_PERSON);

        if (col_thing->State == STATE_DEAD || col_thing->State == STATE_DYING) {
            continue;
        }

        if (col_thing != p_thing) {
            if (col_thing->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) {
                // Don't knock out invulnerable people.
            } else {
                if (people_allowed_to_hit_each_other(col_thing, p_thing))
                    if (PCOM_person_a_hates_b(p_thing, col_thing)) {
                        calc_sub_objects_position(col_thing, col_thing->Draw.Tweened->AnimTween, SUB_OBJECT_HEAD, &hx, &hy, &hz);
                        hx += col_thing->WorldPos.X >> 8;
                        hy += col_thing->WorldPos.Y >> 8;
                        hz += col_thing->WorldPos.Z >> 8;

                        dx = abs(fx - hx);
                        dz = abs(fz - hz);

                        if (QDIST2(dx, dz) < 100) {
                            SLONG miny, maxy;

                            miny = hy - (MAX(abs(p_thing->DY >> 8), 100) + 10);
                            maxy = hy + MAX(abs(p_thing->DY >> 9), 100);

                            if (fy > miny && fy < maxy) {
                                if (col_thing->Genus.Person->PersonType == PERSON_MIB1 || col_thing->Genus.Person->PersonType == PERSON_MIB2 || col_thing->Genus.Person->PersonType == PERSON_MIB3) {
                                    // MIBs are immune to head stomps: only notify AI.
                                    PCOM_attack_happened(col_thing, p_thing);
                                } else {
                                    PCOM_attack_happened(col_thing, p_thing);

                                    SLONG behind = (Random() & 0x1);

                                    if (!is_there_room_behind_person(col_thing, behind)) {
                                        behind = !behind;
                                    }

                                    set_person_dead(col_thing, p_thing, PERSON_DEATH_TYPE_STAY_ALIVE, behind, 3);
                                }
                            }
                        }
                    }
            }
        }
    }
}

// uc_orig: move_thing (fallen/Source/collide.cpp)
// Full physics move for a CLASS_PERSON thing. Applies collision against other things,
// OB objects, wall slides, edge slides, face tracking, and fall-off detection.
ULONG move_thing(
    SLONG dx,
    SLONG dy,
    SLONG dz,
    Thing* p_thing)
{
    SLONG col;
    SLONG radius;
    SLONG new_y;
    SLONG new_face;
    SLONG slid_odd;
    SLONG fall_off_flag = 0;

    extern SLONG yomp_speed;
    extern SLONG sprint_speed;

    GameCoord new_position;

    ASSERT(abs(dx) >> 16 < 2);
    ASSERT(abs(dz) >> 16 < 2);

    if (!p_thing) {
        MSG_add("ERROR: move_slidey_thing(NULL!)");
        return 0;
    }

    ASSERT(WITHIN(p_thing, TO_THING(1), TO_THING(MAX_THINGS - 1)));
    ASSERT(p_thing->Class == CLASS_PERSON);

// uc_orig: THING_RADIUS (fallen/Source/collide.cpp)
#define THING_RADIUS 128

    radius = get_person_radius2(p_thing->Genus.Person->PersonType);

    // HUG_WALL state uses a tighter radius to get closer to walls.
    if (p_thing->State == STATE_HUG_WALL)
        radius >>= 2;

    // In the original, x1/y1/z1/x2/y2/z2 are file-scope globals; here made local.
    SLONG x1 = p_thing->WorldPos.X;
    SLONG my_y1 = p_thing->WorldPos.Y;
    SLONG z1 = p_thing->WorldPos.Z;

    SLONG x2 = x1 + dx;
    SLONG y2 = my_y1 + dy;
    SLONG z2 = z1 + dz;

    if (x2 < 0 || x2 >= 128 << 16 || z2 < 0 || z2 >= 128 << 16) {
        // Don't move off the edge of the map.
        return (0);
    }

    /*
    if(p_thing->Class==CLASS_PERSON)
    {
            if(p_thing->Genus.Person->PlayerID)
            {
extern	void	set_player_visited(UBYTE x,UBYTE z);
                    set_player_visited(x2>>16,z2>>16);

            }
    }
    */

    if (x2 < 0 || z2 < 0 || (x2 >> 16) >= MAP_WIDTH || (z2 >> 16 >= MAP_WIDTH)) {
        return (0);
    }

    slid_odd = UC_FALSE;

    // Collide with other things (persons, vehicles, furniture, etc.)
    slid_odd |= collide_against_things(
        p_thing,
        radius,
        x1, my_y1, z1,
        &x2, &y2, &z2);
    ASSERT(abs(x2 - x1) >> 16 < 2);
    ASSERT(abs(z2 - z1) >> 16 < 2);

    if (p_thing->State == STATE_DYING) {
        // Died during thing collision (e.g. hit spike).
        return 0;
    }

    // Collide with OB street objects (lamp-posts, bins, etc.)
    slid_odd |= collide_against_objects(
        p_thing,
        radius,
        x1, my_y1, z1,
        &x2, &y2, &z2);
    ASSERT(abs(x2 - x1) >> 16 < 2);
    ASSERT(abs(z2 - z1) >> 16 < 2);

    if (slid_odd) {
        p_thing->Genus.Person->SlideOdd += 1;
    } else {
        p_thing->Genus.Person->SlideOdd = 0;
    }

    if (p_thing->OnFace) {

        if (p_thing->OnFace > 0) {
            if (prim_faces4[p_thing->OnFace].FaceFlags & FACE_FLAG_FIRE_ESCAPE) {
                fall_off_flag |= FALL_OFF_FLAG_FIRE_ESCAPE;
            }
        }

        {
            SLONG saflag = 0;
            if (p_thing->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
                saflag |= SLIDE_ALONG_FLAG_CARRYING;
            }

            slide_along(
                x1, my_y1, z1,
                &x2, &y2, &z2,
                SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT,
                radius + 20,
                saflag);
        }

        ASSERT(abs(x2 - x1) >> 16 < 2);
        ASSERT(abs(z2 - z1) >> 16 < 2);

        if (actual_sliding && p_thing->Velocity > yomp_speed) {
            p_thing->Velocity = yomp_speed;
            p_thing->Genus.Person->Mode = PERSON_MODE_RUN;
        }

        void slide_along_edges(SLONG face4, SLONG x1, SLONG z1, SLONG * x2, SLONG * z2);
        void slide_along_edgesr(SLONG face4, SLONG x1, SLONG z1, SLONG * x2, SLONG * z2);

        // Slide along face edges only in combat, for the player walking, or on fire escapes.
        if (p_thing->State == STATE_CIRCLING || p_thing->SubState == SUB_STATE_STEP_FORWARD || ((p_thing->SubState == SUB_STATE_WALKING) && p_thing->Genus.Person->PlayerID) || (fall_off_flag & FALL_OFF_FLAG_FIRE_ESCAPE))
        //		if(p_thing->State==STATE_CIRCLING || (p_thing->SubState==SUB_STATE_WALKING&&p_thing->Genus.Person->PlayerID)||(fall_off_flag&FALL_OFF_FLAG_FIRE_ESCAPE))
        {
            /*
                                    if (p_thing->SubState == SUB_STATE_WALKING_BACKWARDS)
                                    {
                                            //
                                            // But never when walking backwards...
                                            //
                                    }
                                    else
            */
            {
                if (p_thing->OnFace > 0) {
                    slide_along_edges(p_thing->OnFace, x1, z1, &x2, &z2);
                    ASSERT(abs(x2 - x1) >> 16 < 2);
                    ASSERT(abs(z2 - z1) >> 16 < 2);
                } else {
                    slide_along_redges(-p_thing->OnFace, x1, z1, &x2, &z2);
                    ASSERT(abs(x2 - x1) >> 16 < 2);
                    ASSERT(abs(z2 - z1) >> 16 < 2);
                }
            }
        }

        if (is_thing_on_this_quad(x2 >> 8, z2 >> 8, p_thing->OnFace)) {
            MSG_add(" still on same quad ");

            if (actual_sliding) {
                p_thing->Genus.Person->Flags |= FLAG_PERSON_HIT_WALL;
            }
        } else {
            MSG_add(" NOT on same quad ");

            new_face = find_face_for_this_pos(x2 >> 8, y2 >> 8, z2 >> 8, &new_y, 0, 0);

            if (new_face == NULL) {
                // Walked off a face.
                fall_off_flag |= FALL_OFF_FLAG_TRUE;

                if (p_thing->OnFace > 0 && (prim_faces4[p_thing->OnFace].FaceFlags & (FACE_FLAG_WMOVE | FACE_FLAG_PRIM))) {
                    // Don't do this to crates.
                    if (prim_faces4[p_thing->OnFace].FaceFlags & FACE_FLAG_PRIM) {
                        SLONG ob_index = -prim_faces4[p_thing->OnFace].ThingIndex;

                        ASSERT(WITHIN(ob_index, 1, OB_ob_upto - 1));

                        if (OB_ob[ob_index].prim == 129) {
                            // Don't pull up from this prim type.
                            goto do_grab;
                        }
                    }

                    fall_off_flag |= FALL_OFF_FLAG_DONT_GRAB;

                do_grab:;
                }

                p_thing->OnFace = 0;
            } else if (new_face == GRAB_FLOOR) {
                // Stepped off face onto the ground floor.
                p_thing->OnFace = 0;
                fall_off_flag &= ~FALL_OFF_FLAG_TRUE;
                y2 = new_y << 8;
            } else {
                if (actual_sliding) {
                    p_thing->Genus.Person->Flags |= FLAG_PERSON_HIT_WALL;
                }

                if ((y2 >> 8) - new_y > 0x50) {
                    // Long drop to the new face level.
                    fall_off_flag |= FALL_OFF_FLAG_TRUE;

                    if (p_thing->OnFace > 0 && prim_faces4[p_thing->OnFace].FaceFlags & FACE_FLAG_WMOVE) {
                        fall_off_flag |= FALL_OFF_FLAG_DONT_GRAB;
                    }
                } else {
                    // Short drop or small step up.
                    y2 = new_y << 8;
                }

                p_thing->OnFace = new_face;
            }
        }
    } else {
        UWORD person_inside = 0, look_for_face = 0;
        SLONG cd = 0;

        /*

        if(person_inside==0 || look_for_face)
        {
                new_face = find_face_for_this_pos(x2>>8, y2>>8, z2>>8, &new_y, 0,0); //ignore,0);
        }
        else
        {
                new_face=0;
        }

        if (new_face && (new_face!=GRAB_FLOOR))
        {
                ASSERT(abs(new_y - (y2 >> 8)) < 0x100);

                p_thing->OnFace = new_face;

                if( (y2>>8)-new_y > 0x50)
                {
                        // long drop
                        fall_off_flag        |= FALL_OFF_FLAG_TRUE;
                }
                else
                {
                        // short drop or small step up
                        y2=(new_y)<<8;  //+4?
                }
        }
        else

        */
        {
            SLONG ox2, oy2, oz2;

            ox2 = x2;
            oy2 = y2;
            oz2 = z2;

            {

                SLONG saflag = 0;
                if (p_thing->Genus.Person->Flags2 & FLAG2_PERSON_CARRYING) {
                    saflag |= SLIDE_ALONG_FLAG_CARRYING;
                }
                /* this code is never true, as jumping people call slide along from projectile_move in darci.cpp.
                                                if(p_thing->Genus.Person->PlayerID)
                                                {
                                                        if (p_thing->SubState               == SUB_STATE_RUNNING_JUMP||
                                                                p_thing->SubState               == SUB_STATE_RUNNING_JUMP_FLY||
                                                                p_thing->SubState               == SUB_STATE_FLYING_KICK||
                                                                p_thing->SubState               == SUB_STATE_FLYING_KICK_FALL)
                                                        {
                                                                saflag |= SLIDE_ALONG_FLAG_JUMPING;
                                                        }
                                                }
                */

                col = slide_along(
                    x1, my_y1, z1,
                    &x2, &y2, &z2,
                    SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT,
                    radius + 20,
                    saflag);

                if (actual_sliding && p_thing->Velocity > yomp_speed) {
                    p_thing->Velocity = yomp_speed;
                    p_thing->Genus.Person->Mode = PERSON_MODE_RUN;
                }

                new_face = find_face_for_this_pos(x2 >> 8, y2 >> 8, z2 >> 8, &new_y, 0, 0);

                if (new_face && (new_face != GRAB_FLOOR)) {
                    p_thing->OnFace = new_face;

                    if ((y2 >> 8) - new_y > 0x50) {
                        // Long drop to the new face level.
                        fall_off_flag |= FALL_OFF_FLAG_TRUE;
                    } else {
                        // Short drop or small step up.
                        y2 = (new_y) << 8;
                    }
                }

                /*


                if(actual_sliding)
                {
                        SLONG	dx,dy,dz;
                        SLONG	ax,ay,az;
                        SLONG	len,r;

                        dx=(ox2-x1)>>8;
                        dy=(oy2-my_y1)>>8;
                        dz=(oz2-z1)>>8;

                        len=Root(dx*dx+dz*dz);
                        if(len==0)
                                len=1;

                        r=last_slide_dist+15;


                        radius+=25;

                        dx=(dx*radius)/len;
                        dz=(dz*radius)/len;

                        ax=x2+(dx<<8);
                        ay=y2; //+(dy<<8);
                        az=z2+(dz<<8);

                        new_face = find_face_for_this_pos(ax>>8, ay>>8, az>>8, &new_y, ignore_building);

                        if(new_face && (new_face!=GRAB_FLOOR))
                        {
                                x2=ox2;
                                y2=oy2;
                                z2=oz2;
                        }
                        else
                        {

                                p_thing->Genus.Person->Flags|=FLAG_PERSON_HIT_WALL;

                                if(col)
                                {
                                        slide_along(
                                                x1,
                                                my_y1,
                                                z1,
                                           &x2,
                                           &y2,
                                           &z2,
                                            SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT,
                                                radius+20);
                                }
                        }
                }

                */
            }
        }
    }

    if (fall_off_flag & FALL_OFF_FLAG_TRUE) {
        // Workaround for fall-through-the-roof bug: add a fraction of original movement.
        x2 += dx >> 2;
        y2 += dy >> 2;
        z2 += dz >> 2;
    }

    new_position.X = x2;
    new_position.Y = y2;
    new_position.Z = z2;

    // Rustle up some leaves / debris.
    DIRT_gust(
        p_thing,
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Z >> 8,
        new_position.X >> 8,
        new_position.Z >> 8);

    // Swirl the mist.
    MIST_gust(
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Z >> 8,
        new_position.X >> 8,
        new_position.Z >> 8);

    // Hit some barrels.
    BARREL_hit_with_sphere(
        p_thing->WorldPos.X >> 8,
        p_thing->WorldPos.Y >> 8,
        p_thing->WorldPos.Z >> 8,
        0x50);

    move_thing_on_map(p_thing, &new_position);

    /*

    if(slide_door)
    {
            if(slide_door<0)
            {
                    if(p_thing->Genus.Person->PlayerID)
                    {
                            INDOORS_INDEX=0;
                            INDOORS_DBUILDING=0;
                    }
                    p_thing->Genus.Person->InsideIndex=0;
                    p_thing->Genus.Person->InsideRoom=0;
                    p_thing->OnFace=0;
            }
            else
            {
                    p_thing->Genus.Person->InsideIndex=slide_door;
                    p_thing->Genus.Person->InsideRoom=find_inside_room(slide_door,new_position.X>>16,new_position.Z>>16);
                    p_thing->OnFace=0;
                    fall_off=0;
                    if(p_thing->Genus.Person->PlayerID)
                    {
                            INDOORS_INDEX=slide_door;
                            INDOORS_DBUILDING=inside_storeys[slide_door].Building;
                            INDOORS_ROOM=p_thing->Genus.Person->InsideRoom&0xf;
                    }
            }
    }

    */

    if (slide_into_warehouse) {
        p_thing->Genus.Person->Flags |= FLAG_PERSON_WAREHOUSE;
        p_thing->Genus.Person->Ware = dbuildings[slide_into_warehouse].Ware;

        /*

        if (p_thing->Genus.Person->PlayerID && !WARE_in)
        {
                WARE_enter(slide_into_warehouse);
        }

        */
    }

    if (slide_outof_warehouse) {
        p_thing->Genus.Person->Flags &= ~FLAG_PERSON_WAREHOUSE;
        p_thing->Genus.Person->Ware = 0;

        /*

        if (p_thing->Genus.Person->PlayerID && WARE_in)
        {
                WARE_exit();
        }

        */
    }

    // Update the person's inside-room index after movement.
    if (p_thing->Genus.Person->InsideIndex) {
        p_thing->Genus.Person->InsideRoom = find_inside_room(p_thing->Genus.Person->InsideIndex, new_position.X >> 16, new_position.Z >> 16);

        if (p_thing->Genus.Person->PlayerID)
            INDOORS_ROOM = p_thing->Genus.Person->InsideRoom;
    }

    if (p_thing->SubState != SUB_STATE_WALKING || p_thing->Genus.Person->PlayerID == 0)
        if (p_thing->SubState != SUB_STATE_SLIPPING)
            if (fall_off_flag & FALL_OFF_FLAG_TRUE) {
                MSG_add(" fall off \n");

                {
                    SLONG flag = PERSON_DROP_DOWN_KEEP_VEL;

                    if (fall_off_flag & FALL_OFF_FLAG_DONT_GRAB) {
                        flag |= PERSON_DROP_DOWN_OFF_FACE;
                    }

                    set_person_drop_down(p_thing, flag);
                }
            }

    return 0;
}

// ========================================================================
// LOS (Line of Sight) — chunk 5
// ========================================================================

// uc_orig: start_checking_against_a_new_vector (fallen/Source/collide.cpp)
void start_checking_against_a_new_vector(void)
{
    los_wptr = 0;
    los_done[0] = -UC_INFINITY;
    los_done[1] = -UC_INFINITY;
    los_done[2] = -UC_INFINITY;
    los_done[3] = -UC_INFINITY;
}

// uc_orig: check_vector_against_mapsquare (fallen/Source/collide.cpp)
// Returns UC_TRUE if the current save_stack ray intersects a DFacet wall above map cell (map_x, map_z).
// Uses the los_done ring buffer to avoid re-checking the same facet within one ray sweep.
SLONG check_vector_against_mapsquare(
    SLONG map_x,
    SLONG map_z,
    SLONG los_flags)
{
    SLONG ix;
    SLONG iy;
    SLONG iz;
    SLONG along;
    SLONG f_list;
    SLONG i_dfacet;
    SLONG exit;

    SLONG xmin;
    SLONG xmax;
    SLONG ymin;
    SLONG ymax;
    SLONG zmin;
    SLONG zmax;

    DFacet* df;
    PAP_Lo* pl;

    ASSERT(WITHIN(map_x, 0, PAP_SIZE_LO - 1));
    ASSERT(WITHIN(map_z, 0, PAP_SIZE_LO - 1));

    pl = &PAP_2LO(map_x, map_z);

    f_list = pl->ColVectHead;

    if (f_list == NULL) {
        return UC_FALSE;
    }

    SLONG dlx = save_stack.x2 - save_stack.x1;
    SLONG dly = save_stack.y2 - save_stack.my_y1;
    SLONG dlz = save_stack.z2 - save_stack.z1;

    exit = UC_FALSE;

    while (1) {
        ASSERT(WITHIN(f_list, 1, next_facet_link - 1));

        i_dfacet = facet_links[f_list];

        if (i_dfacet == 0) {
            ASSERT(0);
            return UC_FALSE;
        }

        if (i_dfacet < 0) {
            // Last facet in the list for this cell.
            exit = UC_TRUE;
            i_dfacet = -i_dfacet;
        }

        // Skip if we already checked this facet for this ray.
        if (i_dfacet == los_done[0] || i_dfacet == los_done[1] || i_dfacet == los_done[2] || i_dfacet == los_done[3]) {
            // Already tested.
        } else {
            // Record this facet in the ring buffer.
            los_done[los_wptr] = i_dfacet;
            los_wptr = (los_wptr + 1) & 3;

            ASSERT(WITHIN(i_dfacet, 1, next_dfacet - 1));

            df = &dfacets[i_dfacet];

            if (!(los_flags & LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG) && (df->FacetFlags & FACET_FLAG_SEETHROUGH)) {
                // See-through fence — no LOS block.
            } else {
                ASSERT(df->FacetType != STOREY_TYPE_PARTITION);

                if (df->FacetType == STOREY_TYPE_NORMAL || df->FacetType == STOREY_TYPE_FENCE || df->FacetType == STOREY_TYPE_FENCE_FLAT || df->FacetType == STOREY_TYPE_FENCE_BRICK || df->FacetType == STOREY_TYPE_OUTSIDE_DOOR || df->FacetType == STOREY_TYPE_DOOR || df->FacetType == STOREY_TYPE_NORMAL_FOUNDATION) {
                    if (df->x[0] == df->x[1]) {
                        if ((save_stack.x1 <= (df->x[0] << 8) && save_stack.x2 > (df->x[0] << 8)) || (save_stack.x1 >= (df->x[0] << 8) && save_stack.x2 < (df->x[0] << 8))) {
                            // Find how far along the ray the X-aligned wall is crossed.
                            along = ((df->x[0] << 8) - save_stack.x1 << 8) / (save_stack.x2 - save_stack.x1);

                            iy = save_stack.my_y1 + (along * dly >> 8);

                            ymin = df->Y[0];
                            ymax = df->Y[0] + (df->Height * df->BlockHeight << 2);

                            if (WITHIN(iy, ymin, ymax)) {
                                iz = save_stack.z1 + (along * dlz >> 8);

                                zmin = df->z[0] << 8;
                                zmax = df->z[1] << 8;

                                if (zmin > zmax) {
                                    SWAP(zmin, zmax);
                                }

                                if (WITHIN(iz, zmin, zmax)) {
                                    los_failure_x = df->x[0] << 8;
                                    los_failure_y = iy;
                                    los_failure_z = iz;
                                    los_failure_dfacet = i_dfacet;

                                    return UC_TRUE;
                                }
                            }
                        }
                    } else if (df->z[0] == df->z[1]) {
                        if ((save_stack.z1 <= (df->z[0] << 8) && save_stack.z2 > (df->z[0] << 8)) || (save_stack.z1 >= (df->z[0] << 8) && save_stack.z2 < (df->z[0] << 8))) {
                            // Find how far along the ray the Z-aligned wall is crossed.
                            along = ((df->z[0] << 8) - save_stack.z1 << 8) / (save_stack.z2 - save_stack.z1);

                            iy = save_stack.my_y1 + (along * dly >> 8);

                            ymin = df->Y[0];
                            ymax = df->Y[0] + (df->Height * df->BlockHeight << 2);

                            if (WITHIN(iy, ymin, ymax)) {
                                ix = save_stack.x1 + (along * dlx >> 8);

                                xmin = df->x[0] << 8;
                                xmax = df->x[1] << 8;

                                if (xmin > xmax) {
                                    SWAP(xmin, xmax);
                                }

                                if (WITHIN(ix, xmin, xmax)) {
                                    los_failure_x = ix;
                                    los_failure_y = iy;
                                    los_failure_z = df->z[0] << 8;
                                    los_failure_dfacet = i_dfacet;

                                    return UC_TRUE;
                                }
                            }
                        }
                    } else {
                        // No diagonal DFacets allowed.
                        if (df->FacetType == STOREY_TYPE_NORMAL)
                            ASSERT(1);
                    }
                }
            }
        }

        if (exit) {
            return UC_FALSE;
        }

        f_list += 1;
    }
}

// uc_orig: check_vector_against_mapsquare_objects (fallen/Source/collide.cpp)
// Returns UC_TRUE if the current save_stack ray intersects any OB-placed object tagged
// PRIM_DAMAGE_NOLOS, or a vehicle (vans/ambulances always; others only if include_cars).
SLONG check_vector_against_mapsquare_objects(
    SLONG map_x,
    SLONG map_z,
    SLONG include_cars)
{
    SLONG y;
    SLONG dx;
    SLONG dz;
    SLONG along;

    {
        OB_Info* oi;
        PrimInfo* pi;

        for (oi = OB_find(map_x, map_z); oi->prim; oi++) {
            if (prim_objects[oi->prim].damage & PRIM_DAMAGE_NOLOS) {
                pi = get_prim_info(oi->prim);

                {
                    dx = save_stack.x2 - save_stack.x1;
                    dz = save_stack.z2 - save_stack.z1;

                    if (abs(dx) > abs(dz)) {
                        along = (oi->x - save_stack.x1 << 8) / (dx + 1);
                    } else {
                        along = (oi->z - save_stack.z1 << 8) / (dz + 1);
                    }

                    y = save_stack.my_y1 + ((save_stack.y2 - save_stack.my_y1) * along >> 8);

                    if (WITHIN(y, oi->y + pi->miny, oi->y + pi->maxy)) {
                        if (collide_box_with_line(
                                oi->x,
                                oi->z,
                                pi->minx,
                                pi->minz,
                                pi->maxx,
                                pi->maxz,
                                oi->yaw,
                                save_stack.x1, save_stack.z1,
                                save_stack.x2, save_stack.z2)) {
                            return UC_TRUE;
                        }
                    }
                }
            }
        }
    }

    // Check large vehicles for LOS blocking.
    {
        SLONG t_index;
        Thing* p_thing;

        t_index = PAP_2LO(map_x, map_z).MapWho;

        while (t_index) {
            p_thing = TO_THING(t_index);

            if (p_thing->Class == CLASS_VEHICLE) {
                if (include_cars || p_thing->Genus.Vehicle->Type == VEH_TYPE_VAN || p_thing->Genus.Vehicle->Type == VEH_TYPE_AMBULANCE || p_thing->Genus.Vehicle->Type == VEH_TYPE_WILDCATVAN) {
                    {
                        dx = save_stack.x2 - save_stack.x1;
                        dz = save_stack.z2 - save_stack.z1;

                        if (abs(dx) > abs(dz)) {
                            along = ((p_thing->WorldPos.X >> 8) - save_stack.x1 << 8) / (dx + 1);
                        } else {
                            along = ((p_thing->WorldPos.Z >> 8) - save_stack.z1 << 8) / (dz + 1);
                        }

                        y = save_stack.my_y1 + ((save_stack.y2 - save_stack.my_y1) * along >> 8);

                        if (y < (p_thing->WorldPos.Y >> 8) + 0xc0) {
                            if (collide_box_with_line(
                                    p_thing->WorldPos.X >> 8,
                                    p_thing->WorldPos.Z >> 8,
                                    -110, -200,
                                    110, 256,
                                    p_thing->Genus.Vehicle->Angle,
                                    save_stack.x1, save_stack.z1,
                                    save_stack.x2, save_stack.z2)) {
                                return UC_TRUE;
                            }
                        }
                    }
                }
            }

            t_index = p_thing->Child;
        }
    }

    return UC_FALSE;
}

// uc_orig: there_is_a_los_things (fallen/Source/collide.cpp)
// Convenience wrapper: checks LOS between two Things at eye height (+100 units).
SLONG there_is_a_los_things(Thing* p_person_a, Thing* p_person_b, SLONG los_flags)
{
    return (there_is_a_los(
        (p_person_a->WorldPos.X >> 8),
        (p_person_a->WorldPos.Y >> 8) + 100,
        (p_person_a->WorldPos.Z >> 8),
        (p_person_b->WorldPos.X >> 8),
        (p_person_b->WorldPos.Y >> 8) + 100,
        (p_person_b->WorldPos.Z >> 8),
        los_flags));
}

// uc_orig: there_is_a_los (fallen/Source/collide.cpp)
// Bresenham-style LOS ray cast from (x1,my_y1,z1) to (x2,y2,z2).
// Traverses DFacet-indexed PAP LO-res cells; also optionally checks prim objects.
// Returns UC_TRUE if the ray is unobstructed. Fills los_failure_* on failure.
SLONG there_is_a_los(
    SLONG x1, SLONG my_y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2,
    SLONG los_flags)
{
    SLONG frac;

    SLONG xfrac;
    SLONG zfrac;

    save_stack.x1 = x1;
    save_stack.my_y1 = my_y1;
    save_stack.z1 = z1;

    save_stack.x2 = x2;
    save_stack.y2 = y2;
    save_stack.z2 = z2;

    los_v_dx = x2 - x1;
    los_v_dy = y2 - my_y1;
    los_v_dz = z2 - z1;

    los_failure_dfacet = NULL; // might help

    if (abs(los_v_dx) + abs(los_v_dz) < 16) {
        // Nearly vertical ray — treat as unobstructed.
        return UC_TRUE;
    }

    // Underground check: step along the ray and verify no point dips below terrain.
    if (!(los_flags & LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
        SLONG i;
        SLONG dist = QDIST3(abs(los_v_dx), abs(los_v_dy), abs(los_v_dz));
        SLONG steps = (dist >> 9) + 1;
        SLONG cdx = los_v_dx / steps;
        SLONG cdy = los_v_dy / steps;
        SLONG cdz = los_v_dz / steps;

        los_v_x = x1 + cdx;
        los_v_y = my_y1 + cdy;
        los_v_z = z1 + cdz;

        for (i = 0; i < steps; i++) {
            if (los_v_y <= PAP_calc_map_height_at(los_v_x, los_v_z)) {
                los_failure_x = los_v_x;
                los_failure_y = los_v_y;
                los_failure_z = los_v_z;
                los_failure_dfacet = NULL;

                return UC_FALSE;
            }

            los_v_x += cdx;
            los_v_y += cdy;
            los_v_z += cdz;
        }
    }

    start_checking_against_a_new_vector();

    los_v_mx = x1 >> PAP_SHIFT_LO;
    los_v_mz = z1 >> PAP_SHIFT_LO;
    los_v_end_mx = x2 >> PAP_SHIFT_LO;
    los_v_end_mz = z2 >> PAP_SHIFT_LO;

    xfrac = x1 & ((1 << PAP_SHIFT_LO) - 1);
    zfrac = z1 & ((1 << PAP_SHIFT_LO) - 1);

    if (abs(los_v_dx) > abs(los_v_dz)) {
        frac = (los_v_dz << PAP_SHIFT_LO) / los_v_dx;

        if (los_v_dx > 0) {
            los_v_z = z1;
            los_v_z -= frac * xfrac >> PAP_SHIFT_LO;
        } else {
            los_v_z = z1;
            los_v_z += frac * ((1 << PAP_SHIFT_LO) - xfrac) >> PAP_SHIFT_LO;
        }

        while (1) {

            if (WITHIN(los_v_mx, 0, PAP_SIZE_LO - 1) && WITHIN(los_v_mz, 0, PAP_SIZE_LO - 1)) {
                if (check_vector_against_mapsquare(
                        los_v_mx, los_v_mz,
                        los_flags)) {
                    return UC_FALSE;
                }

                if (!(los_flags & LOS_FLAG_IGNORE_PRIMS)) {
                    if (check_vector_against_mapsquare_objects(
                            los_v_mx, los_v_mz,
                            los_flags & LOS_FLAG_INCLUDE_CARS)) {
                        return UC_FALSE;
                    }
                }
            }

            if (los_v_mx == los_v_end_mx && los_v_mz == los_v_end_mz) {
                return UC_TRUE;
            }

            if (los_v_dx > 0) {
                los_v_z += frac;
            } else {
                los_v_z -= frac;
            }

            if ((los_v_z >> PAP_SHIFT_LO) != los_v_mz) {
                // Crossed a Z cell boundary — check the new cell too.
                los_v_mz = los_v_z >> PAP_SHIFT_LO;

                if (WITHIN(los_v_mx, 0, PAP_SIZE_LO - 1) && WITHIN(los_v_mz, 0, PAP_SIZE_LO - 1)) {
                    if (check_vector_against_mapsquare(
                            los_v_mx, los_v_mz,
                            los_flags)) {
                        return UC_FALSE;
                    }

                    if (!(los_flags & LOS_FLAG_IGNORE_PRIMS)) {
                        if (check_vector_against_mapsquare_objects(
                                los_v_mx, los_v_mz,
                                los_flags & LOS_FLAG_INCLUDE_CARS)) {
                            return UC_FALSE;
                        }
                    }
                }
            }

            if (los_v_dx > 0) {
                los_v_mx += 1;

                if (los_v_mx > los_v_end_mx) {
                    return UC_TRUE;
                }
            } else {
                los_v_mx -= 1;

                if (los_v_mx < los_v_end_mx) {
                    return UC_TRUE;
                }
            }
        }
    } else {
        frac = (los_v_dx << PAP_SHIFT_LO) / los_v_dz;

        if (los_v_dz > 0) {
            los_v_x = x1;
            los_v_x -= frac * zfrac >> PAP_SHIFT_LO;
        } else {
            los_v_x = x1;
            los_v_x += frac * ((1 << PAP_SHIFT_LO) - zfrac) >> PAP_SHIFT_LO;
        }

        while (1) {

            if (WITHIN(los_v_mx, 0, PAP_SIZE_LO - 1) && WITHIN(los_v_mz, 0, PAP_SIZE_LO - 1)) {
                if (check_vector_against_mapsquare(
                        los_v_mx, los_v_mz,
                        los_flags)) {
                    return UC_FALSE;
                }

                if (!(los_flags & LOS_FLAG_IGNORE_PRIMS)) {
                    if (check_vector_against_mapsquare_objects(
                            los_v_mx, los_v_mz,
                            los_flags & LOS_FLAG_INCLUDE_CARS)) {
                        return UC_FALSE;
                    }
                }
            }

            if (los_v_mx == los_v_end_mx && los_v_mz == los_v_end_mz) {
                return UC_TRUE;
            }

            if (los_v_dz > 0) {
                los_v_x += frac;
            } else {
                los_v_x -= frac;
            }

            if ((los_v_x >> PAP_SHIFT_LO) != los_v_mx) {
                // Crossed an X cell boundary — check the new cell too.
                los_v_mx = los_v_x >> PAP_SHIFT_LO;

                if (WITHIN(los_v_mx, 0, PAP_SIZE_LO - 1) && WITHIN(los_v_mz, 0, PAP_SIZE_LO - 1)) {
                    if (check_vector_against_mapsquare(
                            los_v_mx, los_v_mz,
                            los_flags)) {
                        return UC_FALSE;
                    }

                    if (!(los_flags & LOS_FLAG_IGNORE_PRIMS)) {
                        if (check_vector_against_mapsquare_objects(
                                los_v_mx, los_v_mz,
                                los_flags & LOS_FLAG_INCLUDE_CARS)) {
                            return UC_FALSE;
                        }
                    }
                }
            }

            if (los_v_dz > 0) {
                los_v_mz += 1;

                if (los_v_mz > los_v_end_mz) {
                    return UC_TRUE;
                }
            } else {
                los_v_mz -= 1;

                if (los_v_mz < los_v_end_mz) {
                    return UC_TRUE;
                }
            }
        }
    }
}

// uc_orig: there_is_a_los_mav (fallen/Source/collide.cpp)
// LOS check at MAV high-res navigation grid resolution.
// Returns 0=clear, 1=blocked by X-wall, 2=blocked by Z-wall.
// Does NOT fill los_failure_*; used for NPC navigation decisions.
SLONG there_is_a_los_mav(
    SLONG x1, SLONG my_y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    SLONG x;
    SLONG z;

    SLONG dx;
    SLONG dz;

    SLONG mx;
    SLONG mz;

    SLONG end_mx;
    SLONG end_mz;

    SLONG sdx;
    SLONG sdz;

    SLONG frac;

    SLONG xfrac;
    SLONG zfrac;

    MAV_Opt* mo;

    dx = x2 - x1;
    dz = z2 - z1;

    SLONG adx = abs(dx);
    SLONG adz = abs(dz);

    sdx = SIGN(dx);
    sdz = SIGN(dz);

    mx = x1 >> PAP_SHIFT_HI;
    mz = z1 >> PAP_SHIFT_HI;
    end_mx = x2 >> PAP_SHIFT_HI;
    end_mz = z2 >> PAP_SHIFT_HI;

    if (mx == end_mx && mz == end_mz) {
        // Movement entirely within one MAV cell.
        return (0);
    }

    xfrac = x1 & ((1 << PAP_SHIFT_HI) - 1);
    zfrac = z1 & ((1 << PAP_SHIFT_HI) - 1);

    if (adx > adz) {
        frac = (dz << PAP_SHIFT_HI) / dx;

        if (dx > 0) {
            z = z1;
            z -= frac * xfrac >> PAP_SHIFT_HI;
        } else {
            z = z1;
            z += frac * ((1 << PAP_SHIFT_HI) - xfrac) >> PAP_SHIFT_HI;
        }

        while (1) {

            if (mx == end_mx && mz == end_mz) {
                return 0;
            }

            if (dx > 0) {
                z += frac;
            } else {
                z -= frac;
            }

            if (WITHIN(mx, 1, PAP_SIZE_HI - 2) && WITHIN(mz, 1, PAP_SIZE_HI - 2)) {
            } else {
                // Off the map edge — assume clear.
                return 0;
            }

            mo = &MAV_opt[MAV_NAV(mx, mz)];

            last_mav_square_x = mx;
            last_mav_square_z = mz;

            if ((z >> PAP_SHIFT_HI) != mz) {
                SLONG direction;

                if ((z >> PAP_SHIFT_HI) > mz) {
                    direction = MAV_DIR_ZL;
                } else {
                    direction = MAV_DIR_ZS;
                }

                if (!(mo->opt[direction] & MAV_CAPS_GOTO)) {
                    SLONG x1, z1, x2, z2;
                    x1 = mx;
                    x2 = mx + (dx > 0) ? 1 : -1;
                    if (direction == MAV_DIR_ZL) {
                        z1 = mz + 1;
                        z2 = z1;
                    } else {
                        z1 = mz;
                        z2 = z1;
                    }

                    return (2);
                }

                mz = z >> PAP_SHIFT_HI;
            }

            mo = &MAV_opt[MAV_NAV(mx, mz)];

            if (dx > 0) {
                if (mx + 1 > end_mx) {
                    return 0;
                }
                if (!(mo->opt[MAV_DIR_XL] & MAV_CAPS_GOTO)) {
                    SLONG x1, z1, x2, z2;
                    x1 = mx + 1;
                    x2 = mx + 1;
                    z1 = mz;
                    z2 = z1 + 1;

                    return (1);
                }
                mx += 1;

            } else {

                if (mx - 1 < end_mx) {
                    return 0;
                }
                if (!(mo->opt[MAV_DIR_XS] & MAV_CAPS_GOTO)) {
                    SLONG x1, z1, x2, z2;
                    x1 = mx;
                    x2 = mx;
                    z1 = mz;
                    z2 = z1 + 1;

                    return (1);
                }
                mx -= 1;
            }
        }
    } else {
        frac = (dx << PAP_SHIFT_HI) / dz;

        if (dz > 0) {
            x = x1;
            x -= frac * zfrac >> PAP_SHIFT_HI;
        } else {
            x = x1;
            x += frac * ((1 << PAP_SHIFT_HI) - zfrac) >> PAP_SHIFT_HI;
        }

        while (1) {

            if (mx == end_mx && mz == end_mz) {
                return 0;
            }

            if (dz > 0) {
                x += frac;
            } else {
                x -= frac;
            }

            if (WITHIN(mx, 1, PAP_SIZE_HI - 2) && WITHIN(mz, 1, PAP_SIZE_HI - 2)) {
            } else {
                return 0;
            }

            last_mav_square_x = mx;
            last_mav_square_z = mz;

            if ((x >> PAP_SHIFT_HI) != mx) {
                SLONG direction;

                if ((x >> PAP_SHIFT_HI) > mx) {
                    direction = MAV_DIR_XL;
                } else {
                    direction = MAV_DIR_XS;
                }

                mo = &MAV_opt[MAV_NAV(mx, mz)];

                if (!(mo->opt[direction] & MAV_CAPS_GOTO)) {
                    SLONG x1, z1, x2, z2;
                    if (direction = MAV_DIR_XL) {
                        x1 = mx + 1;
                        x2 = mx + 1;
                    } else {
                        x1 = mx;
                        x2 = mx;
                    }
                    z1 = mz;
                    z2 = z1 + 1;

                    return (1);
                }

                mx = x >> PAP_SHIFT_HI;
            }

            mo = &MAV_opt[MAV_NAV(mx, mz)];

            if (dz > 0) {
                if (mz + 1 > end_mz) {
                    return 0;
                }
                if (!(mo->opt[MAV_DIR_ZL] & MAV_CAPS_GOTO)) {
                    SLONG x1, z1, x2, z2;

                    x1 = mx;
                    x2 = mx + 1;
                    z1 = (mz + 1);
                    z2 = z1 + 1;

                    return (2);
                }

                mz += 1;

            } else {
                if (mz - 1 < end_mz) {
                    return 0;
                }
                if (!(mo->opt[MAV_DIR_ZS] & MAV_CAPS_GOTO)) {
                    SLONG x1, z1, x2, z2;

                    x1 = mx;
                    x2 = mx + 1;
                    z1 = (mz);
                    z2 = z1;

                    return (2);
                }

                mz -= 1;
            }
        }
    }
}

// uc_orig: there_is_a_los_car (fallen/Source/collide.cpp)
// LOS check using the car navigation grid (MAV_CAR_GOTO), not the person grid.
// Returns 0=clear, 1=blocked by X-wall, 2=blocked by Z-wall.
// Sets last_mav_square_* and last_mav_d* to identify the blocking wall.
SLONG there_is_a_los_car(
    SLONG x1, SLONG my_y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    SLONG x;
    SLONG z;

    SLONG dx;
    SLONG dz;

    SLONG mx;
    SLONG mz;

    SLONG end_mx;
    SLONG end_mz;

    SLONG sdx;
    SLONG sdz;

    SLONG frac;

    SLONG xfrac;
    SLONG zfrac;

    dx = x2 - x1;
    dz = z2 - z1;

    SLONG adx = abs(dx);
    SLONG adz = abs(dz);

    sdx = SIGN(dx);
    sdz = SIGN(dz);

    mx = x1 >> PAP_SHIFT_HI;
    mz = z1 >> PAP_SHIFT_HI;
    end_mx = x2 >> PAP_SHIFT_HI;
    end_mz = z2 >> PAP_SHIFT_HI;

    if (mx == end_mx && mz == end_mz) {
        return (0);
    }

    xfrac = x1 & ((1 << PAP_SHIFT_HI) - 1);
    zfrac = z1 & ((1 << PAP_SHIFT_HI) - 1);

    if (adx > adz) {
        frac = (dz << PAP_SHIFT_HI) / dx;

        if (dx > 0) {
            z = z1;
            z -= frac * xfrac >> PAP_SHIFT_HI;
        } else {
            z = z1;
            z += frac * ((1 << PAP_SHIFT_HI) - xfrac) >> PAP_SHIFT_HI;
        }

        while (1) {

            if (mx == end_mx && mz == end_mz) {
                return 0;
            }

            if (dx > 0) {
                z += frac;
            } else {
                z -= frac;
            }

            if (WITHIN(mx, 1, PAP_SIZE_HI - 2) && WITHIN(mz, 1, PAP_SIZE_HI - 2)) {
            } else {
                return 0;
            }

            if ((z >> PAP_SHIFT_HI) != mz) {
                SLONG direction;

                if ((z >> PAP_SHIFT_HI) > mz) {
                    direction = MAV_DIR_ZL;
                } else {
                    direction = MAV_DIR_ZS;
                }

                if (!MAV_CAR_GOTO(mx, mz, direction)) {
                    last_mav_square_x = mx;
                    last_mav_square_z = mz;
                    last_mav_dx = 0;
                    last_mav_dz = (direction == MAV_DIR_ZL) ? 1 : -1;
                    return (2);
                }

                mz = z >> PAP_SHIFT_HI;
            }

            if (dx > 0) {
                if (mx + 1 > end_mx) {
                    return 0;
                }
                if (!MAV_CAR_GOTO(mx, mz, MAV_DIR_XL)) {
                    last_mav_square_x = mx;
                    last_mav_square_z = mz;
                    last_mav_dx = 1;
                    last_mav_dz = 0;
                    return (1);
                }
                mx += 1;

            } else {

                if (mx - 1 < end_mx) {
                    return 0;
                }
                if (!MAV_CAR_GOTO(mx, mz, MAV_DIR_XS)) {
                    last_mav_square_x = mx;
                    last_mav_square_z = mz;
                    last_mav_dx = -1;
                    last_mav_dz = 0;
                    return (1);
                }
                mx -= 1;
            }
        }
    } else {
        frac = (dx << PAP_SHIFT_HI) / dz;

        if (dz > 0) {
            x = x1;
            x -= frac * zfrac >> PAP_SHIFT_HI;
        } else {
            x = x1;
            x += frac * ((1 << PAP_SHIFT_HI) - zfrac) >> PAP_SHIFT_HI;
        }

        while (1) {

            if (mx == end_mx && mz == end_mz) {
                return 0;
            }

            if (dz > 0) {
                x += frac;
            } else {
                x -= frac;
            }

            if (WITHIN(mx, 1, PAP_SIZE_HI - 2) && WITHIN(mz, 1, PAP_SIZE_HI - 2)) {
            } else {
                return 0;
            }

            if ((x >> PAP_SHIFT_HI) != mx) {
                SLONG direction;

                if ((x >> PAP_SHIFT_HI) > mx) {
                    direction = MAV_DIR_XL;
                } else {
                    direction = MAV_DIR_XS;
                }

                if (!MAV_CAR_GOTO(mx, mz, direction)) {
                    last_mav_square_x = mx;
                    last_mav_square_z = mz;
                    last_mav_dx = (direction == MAV_DIR_XL) ? 1 : -1;
                    last_mav_dz = 0;
                    return (1);
                }

                mx = x >> PAP_SHIFT_HI;
            }

            if (dz > 0) {
                if (mz + 1 > end_mz) {
                    return 0;
                }
                if (!MAV_CAR_GOTO(mx, mz, MAV_DIR_ZL)) {
                    last_mav_square_x = mx;
                    last_mav_square_z = mz;
                    last_mav_dx = 0;
                    last_mav_dz = 1;
                    return (2);
                }

                mz += 1;

            } else {
                if (mz - 1 < end_mz) {
                    return 0;
                }
                last_mav_dz = -1;
                if (!MAV_CAR_GOTO(mx, mz, MAV_DIR_ZS)) {
                    last_mav_square_x = mx;
                    last_mav_square_z = mz;
                    last_mav_dx = 0;
                    last_mav_dz = -1;
                    return (2);
                }

                mz -= 1;
            }
        }
    }
}

// ========================================================================
// Chunk 5b: circle/sausage/box collision, shockwave, fastnav (lines 7481-9155)
// ========================================================================

// uc_orig: slide_around_circle (fallen/Source/collide.cpp)
// Pushes (*x2, *z2) outside a circle of given radius centered at (cx, cz).
// If already outside, returns UC_FALSE. On zero distance, snaps back to (x1, z1).
SLONG slide_around_circle(
    SLONG cx,
    SLONG cz,
    SLONG cradius,
    SLONG x1,
    SLONG z1,
    SLONG* x2,
    SLONG* z2)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;

    dx = *x2 - cx;
    dz = *z2 - cz;

    dist = QDIST2(abs(dx), abs(dz));

    if (dist < cradius) {
        if (dist == 0) {
            *x2 = x1;
            *z2 = z1;
            return UC_TRUE;
        } else {
            // Push out from centre proportionally.
            dx *= (cradius - dist);
            dz *= (cradius - dist);

            dx /= dist;
            dz /= dist;

            *x2 += dx;
            *z2 += dz;

            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// uc_orig: collide_with_circle (fallen/Source/collide.cpp)
// Returns UC_TRUE if (*x2, *z2) is strictly inside the circle (no push-out).
SLONG collide_with_circle(
    SLONG cx,
    SLONG cz,
    SLONG cradius,
    SLONG* x2,
    SLONG* z2)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;

    dx = *x2 - cx;
    dz = *z2 - cz;

    dist = QDIST2(abs(dx), abs(dz));

    if (dist < cradius) {
        return UC_TRUE;
    }

    return UC_FALSE;
}

// uc_orig: in_my_fov (fallen/Source/collide.cpp)
// Returns UC_TRUE if (him_x, him_z) is in the forward 180-degree FOV of an observer
// at (me_x, me_z) looking in direction (lookx, lookz).
SLONG in_my_fov(
    SLONG me_x, SLONG me_z,
    SLONG him_x, SLONG him_z,
    SLONG lookx,
    SLONG lookz)
{
    SLONG dx = him_x - me_x;
    SLONG dz = him_z - me_z;
    SLONG dprod = dx * lookx + dz * lookz;

    if (dprod > 0) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: calc_map_height_at (fallen/Source/collide.cpp)
// Returns the world-space ground height at (x, z).
// Uses MAV height for hidden (underground/sewer) cells; PAP bilinear interpolation otherwise.
SLONG calc_map_height_at(SLONG x, SLONG z)
{
    SLONG mx = x >> 8;
    SLONG mz = z >> 8;

    if (!WITHIN(mx, 0, MAP_WIDTH - 1) || !WITHIN(mz, 0, MAP_HEIGHT - 1)) {
        return 0;
    }

    if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN) {
        return MAVHEIGHT(mx, mz) << 6;
    } else {
        return PAP_calc_height_at(x, z);
    }
}

// uc_orig: collide_against_sausage (fallen/Source/collide.cpp)
// Tests and resolves a point (vx2, vz2) against an axis-aligned sausage shape
// (rectangle + semicircles at each end). Returns UC_TRUE and writes the push-out
// position to (*slide_x, *slide_z) if there is a collision.
// Only works with orthogonal (axis-aligned) sausages (dsx==0 or dsz==0).
SLONG collide_against_sausage(
    SLONG sx1, SLONG sz1,
    SLONG sx2, SLONG sz2,
    SLONG swidth,
    SLONG vx1, SLONG vz1,
    SLONG vx2, SLONG vz2,
    SLONG* slide_x,
    SLONG* slide_z)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;

    if (sx1 > sx2) {
        SWAP(sx1, sx2);
    }
    if (sz1 > sz2) {
        SWAP(sz1, sz2);
    }

    SLONG dsx = sx2 - sx1;
    SLONG dsz = sz2 - sz1;

    ASSERT(dsx == 0 || dsz == 0);

    if (dsx == 0) {
        dx = abs(vx2 - sx1);

        if (dx < swidth) {
            if (WITHIN(vz2, sz1, sz2)) {
                if (vx1 > sx1) {
                    *slide_x = sx1 + swidth;
                    *slide_z = vz2;
                } else {
                    *slide_x = sx1 - swidth;
                    *slide_z = vz2;
                }
                return UC_TRUE;
            }
            // Falls through to circle end-cap check.
        } else {
            return UC_FALSE;
        }
    } else {
        ASSERT(dsz == 0);

        dz = abs(vz2 - sz1);

        if (dz < swidth) {
            if (WITHIN(vx2, sx1, sx2)) {
                if (vz1 > sz1) {
                    *slide_x = vx2;
                    *slide_z = sz1 + swidth;
                } else {
                    *slide_x = vx2;
                    *slide_z = sz1 - swidth;
                }
                return UC_TRUE;
            }
            // Falls through to circle end-cap check.
        } else {
            return UC_FALSE;
        }
    }

    // Two circles at either end of the sausage.
    dx = vx2 - sx1;
    dz = vz2 - sz1;

    dist = QDIST2(abs(dx), abs(dz)) + 1;

    if (dist < swidth) {
        dx *= swidth;
        dz *= swidth;

        dx /= dist;
        dz /= dist;

        *slide_x = sx1 + dx;
        *slide_z = sz1 + dz;

        return UC_TRUE;
    }

    dx = vx2 - sx2;
    dz = vz2 - sz2;

    dist = QDIST2(abs(dx), abs(dz)) + 1;

    if (dist < swidth) {
        dx *= swidth;
        dz *= swidth;

        dx /= dist;
        dz /= dist;

        *slide_x = sx2 + dx;
        *slide_z = sz2 + dz;

        return UC_TRUE;
    }

    return UC_FALSE;
}

// uc_orig: slide_around_sausage (fallen/Source/collide.cpp)
// Slides (*x2, *z2) around the outside of a sausage (capsule) shape defined by
// segment (sx1,sz1)-(sx2,sz2) with radius sradius. Returns UC_TRUE on collision.
SLONG slide_around_sausage(
    SLONG sx1,
    SLONG sz1,
    SLONG sx2,
    SLONG sz2,
    SLONG sradius,
    SLONG x1,
    SLONG z1,
    SLONG* x2,
    SLONG* z2)
{
    SLONG dist;
    SLONG vec_x;
    SLONG vec_z;
    SLONG on;
    SLONG len;
    SLONG overlen;

    SLONG dx;
    SLONG dz;

    signed_dist_to_line_with_normal_mark(
        sx1, sz1,
        sx2, sz2,
        *x2,
        *z2,
        &dist,
        &vec_x,
        &vec_z,
        &on);

    dist = abs(dist);

    if (dist > sradius) {
        return UC_FALSE;
    }

    // Normalise (vec_x, vec_z).
    len = QDIST2(abs(vec_x), abs(vec_z)) + 1;
    overlen = 0x10000 / len;
    vec_x *= overlen;
    vec_z *= overlen;

    // Push (*x2, *z2) to be exactly sradius from the sausage segment.
    dx = MUL64(sradius - dist, vec_x);
    dz = MUL64(sradius - dist, vec_z);

    *x2 += dx;
    *z2 += dz;

    return UC_TRUE;
}

// uc_orig: stop_movement_between (fallen/Source/collide.cpp)
// Returns UC_TRUE if Darci should be blocked from moving between two adjacent
// hi-res map cells. Used to place water-edge collision facets.
SLONG stop_movement_between(
    SLONG mx1,
    SLONG mz1,
    SLONG mx2,
    SLONG mz2)
{
    PAP_Hi* ph1 = &PAP_2HI(mx1, mz1);
    PAP_Hi* ph2 = &PAP_2HI(mx2, mz2);

    SLONG normal = UC_TRUE;

    if ((ph1->Flags & (PAP_FLAG_WATER | PAP_FLAG_HIDDEN)) || (ph2->Flags & (PAP_FLAG_WATER | PAP_FLAG_HIDDEN))) {
        normal = UC_FALSE;
    }

    if ((ph1->Flags & PAP_FLAG_WATER) && !(ph2->Flags & PAP_FLAG_WATER)) {
        // The edge of some water — block movement.
        return UC_TRUE;
    }

    return UC_FALSE;
}

// uc_orig: create_just_collision_facet (fallen/Source/collide.cpp)
// Allocates a new STOREY_TYPE_JUST_COLLISION DFacet for a water or sewer edge
// and registers it on the map. Called by insert_collision_facets.
void create_just_collision_facet(
    SLONG x1,
    SLONG z1,
    SLONG x2,
    SLONG z2)
{
    SLONG dx = x2 - x1;
    SLONG dz = z2 - z1;

    SLONG hx1 = x1 + SIGN(dx) + SIGN(dz);
    SLONG hz1 = z1 + SIGN(dz) - SIGN(dx);

    SLONG hx2 = x2 - SIGN(dx) + SIGN(dz);
    SLONG hz2 = z2 - SIGN(dz) - SIGN(dx);

    SLONG my_y1 = PAP_calc_height_at(hx1, hz1);
    SLONG y2 = PAP_calc_height_at(hx2, hz2);

    if (next_dfacet >= MAX_DFACETS) {
        return;
    }

    SLONG dfacet = next_dfacet++;

    DFacet* df = &dfacets[dfacet];

    df->FacetType = STOREY_TYPE_JUST_COLLISION;
    df->Height = 4;
    df->BlockHeight = 16;
    df->x[0] = x1 >> 8;
    df->Y[0] = my_y1;
    df->z[0] = z1 >> 8;
    df->x[1] = x2 >> 8;
    df->Y[1] = y2;
    df->z[1] = z2 >> 8;
    df->FacetFlags = 0;

    df->StyleIndex = 0;
    df->Building = 0;
    df->DStorey = 0;
    df->FHeight = 0;
    df->Dfcache = 0;
    df->Counter[0] = 0;
    df->Counter[1] = 0;

    add_facet_to_map(dfacet);
}

// uc_orig: insert_collision_facets (fallen/Source/collide.cpp)
// Scans the entire hi-res map for water/sewer edges and inserts
// STOREY_TYPE_JUST_COLLISION facets along them. Called once at level load.
void insert_collision_facets()
{
    SLONG x;
    SLONG z;

    SLONG fx1;
    SLONG fz1;

    SLONG fx2;
    SLONG fz2;

    SLONG fstart;
    SLONG blocked;

    // Lines of constant x.
    for (x = 1; x < PAP_SIZE_HI - 1; x++) {
        // Transitions to lower x neighbour.
        fstart = UC_FALSE;

        for (z = 1; z < PAP_SIZE_HI; z++) {
            if (z == PAP_SIZE_HI - 1) {
                blocked = UC_FALSE;
            } else {
                blocked = stop_movement_between(x, z, x - 1, z);
            }

            if (blocked) {
                if (!fstart) {
                    fx1 = x << PAP_SHIFT_HI;
                    fz1 = z << PAP_SHIFT_HI;
                    fstart = UC_TRUE;
                }
            } else {
                if (fstart) {
                    fx2 = x << PAP_SHIFT_HI;
                    fz2 = z << PAP_SHIFT_HI;
                    fstart = UC_FALSE;
                    create_just_collision_facet(fx1, fz1, fx2, fz2);
                }
            }
        }

        // Transitions to higher x neighbour.
        fstart = UC_FALSE;

        for (z = 1; z < PAP_SIZE_HI; z++) {
            if (z == PAP_SIZE_HI - 1) {
                blocked = UC_FALSE;
            } else {
                blocked = stop_movement_between(x, z, x + 1, z);
            }

            if (blocked) {
                if (!fstart) {
                    fx1 = x + 1 << PAP_SHIFT_HI;
                    fz1 = z << PAP_SHIFT_HI;
                    fstart = UC_TRUE;
                }
            } else {
                if (fstart) {
                    fx2 = x + 1 << PAP_SHIFT_HI;
                    fz2 = z << PAP_SHIFT_HI;
                    fstart = UC_FALSE;
                    create_just_collision_facet(fx2, fz2, fx1, fz1);
                }
            }
        }
    }

    // Lines of constant z.
    for (z = 1; z < PAP_SIZE_HI - 1; z++) {
        // Transitions to lower z neighbour.
        fstart = UC_FALSE;

        for (x = 1; x < PAP_SIZE_HI; x++) {
            if (x == PAP_SIZE_HI - 1) {
                blocked = UC_FALSE;
            } else {
                blocked = stop_movement_between(x, z, x, z - 1);
            }

            if (blocked) {
                if (!fstart) {
                    fx1 = x << PAP_SHIFT_HI;
                    fz1 = z << PAP_SHIFT_HI;
                    fstart = UC_TRUE;
                }
            } else {
                if (fstart) {
                    fx2 = x << PAP_SHIFT_HI;
                    fz2 = z << PAP_SHIFT_HI;
                    fstart = UC_FALSE;
                    create_just_collision_facet(fx2, fz2, fx1, fz1);
                }
            }
        }

        // Transitions to higher z neighbour.
        fstart = UC_FALSE;

        for (x = 1; x < PAP_SIZE_HI; x++) {
            if (x == PAP_SIZE_HI - 1) {
                blocked = UC_FALSE;
            } else {
                blocked = stop_movement_between(x, z, x, z + 1);
            }

            if (blocked) {
                if (!fstart) {
                    fx1 = x << PAP_SHIFT_HI;
                    fz1 = z + 1 << PAP_SHIFT_HI;
                    fstart = UC_TRUE;
                }
            } else {
                if (fstart) {
                    fx2 = x << PAP_SHIFT_HI;
                    fz2 = z + 1 << PAP_SHIFT_HI;
                    fstart = UC_FALSE;
                    create_just_collision_facet(fx1, fz1, fx2, fz2);
                }
            }
        }
    }
}

// uc_orig: slide_around_box (fallen/Source/collide.cpp)
// Slides (*x2, *z2) to the nearest edge of an OBB (oriented bounding box)
// expanded by radius. Retries if the chosen slide direction lands in a NOGO cell.
// Uses globals tried/used_this_go/failed to avoid stack growth (original comment:
// "should be locals but stack overflow").
SLONG slide_around_box(
    SLONG box_mid_x,
    SLONG box_mid_z,
    SLONG box_min_x,
    SLONG box_min_z,
    SLONG box_max_x,
    SLONG box_max_z,
    SLONG box_yaw,
    SLONG radius,
    SLONG x1,
    SLONG z1,
    SLONG* x2,
    SLONG* z2)
{
    SLONG tx1;
    SLONG tz1;
    SLONG tx2;
    SLONG tz2;

    SLONG rx1;
    SLONG rz1;
    SLONG rx2;
    SLONG rz2;

    SLONG dminx;
    SLONG dminz;
    SLONG dmaxx;
    SLONG dmaxz;

    SLONG minx;
    SLONG minz;

    SLONG maxx;
    SLONG maxz;

    SLONG best;
    SLONG best_x;
    SLONG best_z;

    tried = 0;
    used_this_go = 0;
    failed = 1;

    SLONG matrix[4];
    SLONG useangle;

    SLONG sin_yaw;
    SLONG cos_yaw;

    useangle = -box_yaw;
    useangle &= 2047;

    sin_yaw = SIN(useangle);
    cos_yaw = COS(useangle);

    matrix[0] = cos_yaw;
    matrix[1] = sin_yaw;
    matrix[2] = -sin_yaw;
    matrix[3] = cos_yaw;

    // Rotate positions into box-local space.
    tx1 = x1 - box_mid_x;
    tz1 = z1 - box_mid_z;

    tx2 = *x2 - box_mid_x;
    tz2 = *z2 - box_mid_z;

    rx1 = MUL64(tx1, matrix[0]) + MUL64(tz1, matrix[1]);
    rz1 = MUL64(tx1, matrix[2]) + MUL64(tz1, matrix[3]);

    rx2 = MUL64(tx2, matrix[0]) + MUL64(tz2, matrix[1]);
    rz2 = MUL64(tx2, matrix[2]) + MUL64(tz2, matrix[3]);

    // Expand bounding box by radius.
    minx = box_min_x - radius;
    minz = box_min_z - radius;

    maxx = box_max_x + radius;
    maxz = box_max_z + radius;

    // Early out if not inside the expanded box.
    if (rx2 > maxx || rx2 < minx || rz2 > maxz || rz2 < minz) {
        return UC_FALSE;
    }

    // Distances to each face of the expanded box.
    dminx = rx2 - minx;
    dmaxx = maxx - rx2;

    dminz = rz2 - minz;
    dmaxz = maxz - rz2;

    // Transpose matrix for unrotation (inverse of rotation = transpose).
    SWAP(matrix[1], matrix[2]);

    while (failed) {
        if (tried & 2) {
            best = 0x7fffffff;
        } else {
            best = dminx;
            best_x = minx - 1;
            best_z = rz2;
            used_this_go = 1;
        }

        if (!(tried & 4))
            if (dmaxx < best) {
                {
                    best = dmaxx;
                    best_x = maxx - 1;
                    best_z = rz2;
                    used_this_go = 2;
                }
            }

        if (!(tried & 8))
            if (dminz < best) {
                {
                    best = dminz;
                    best_x = rx2;
                    best_z = minz - 1;
                    used_this_go = 3;
                }
            }

        if (!(tried & 16))
            if (dmaxz < best) {
                {
                    best = dmaxz;
                    best_x = rx2;
                    best_z = maxz + 1;
                    used_this_go = 4;
                }
            }

        // Un-rotate back to world space.
        *x2 = MUL64(best_x, matrix[0]) + MUL64(best_z, matrix[1]);
        *z2 = MUL64(best_x, matrix[2]) + MUL64(best_z, matrix[3]);

        *x2 += box_mid_x;
        *z2 += box_mid_z;

        if (PAP_2HI((*x2) >> 8, (*z2) >> 8).Flags & PAP_FLAG_NOGO) {
            tried |= 1 << used_this_go;
            failed = 1;
        } else {
            failed = 0;
        }
    }

    return UC_TRUE;
}

// uc_orig: slide_around_box_lowstack (fallen/Source/collide.cpp)
// Simplified version of slide_around_box without the NOGO retry loop.
// Does not check NOGO, no globals. Used where stack depth is a concern.
inline SLONG slide_around_box_lowstack(
    SLONG box_mid_x,
    SLONG box_mid_z,
    SLONG box_min_x,
    SLONG box_min_z,
    SLONG box_max_x,
    SLONG box_max_z,
    SLONG box_yaw,
    SLONG radius,
    SLONG x1,
    SLONG z1,
    SLONG* x2,
    SLONG* z2)
{
    SLONG tx1;
    SLONG tz1;
    SLONG tx2;
    SLONG tz2;

    SLONG rx1;
    SLONG rz1;
    SLONG rx2;
    SLONG rz2;

    SLONG dminx;
    SLONG dminz;
    SLONG dmaxx;
    SLONG dmaxz;

    SLONG minx;
    SLONG minz;

    SLONG maxx;
    SLONG maxz;

    SLONG best;
    SLONG best_x;
    SLONG best_z;

    SLONG matrix[4];
    SLONG useangle;

    SLONG sin_yaw;
    SLONG cos_yaw;

    useangle = -box_yaw;
    useangle &= 2047;

    sin_yaw = SIN(useangle);
    cos_yaw = COS(useangle);

    matrix[0] = cos_yaw;
    matrix[1] = sin_yaw;
    matrix[2] = -sin_yaw;
    matrix[3] = cos_yaw;

    tx1 = x1 - box_mid_x;
    tz1 = z1 - box_mid_z;

    tx2 = *x2 - box_mid_x;
    tz2 = *z2 - box_mid_z;

    rx1 = MUL64(tx1, matrix[0]) + MUL64(tz1, matrix[1]);
    rz1 = MUL64(tx1, matrix[2]) + MUL64(tz1, matrix[3]);

    rx2 = MUL64(tx2, matrix[0]) + MUL64(tz2, matrix[1]);
    rz2 = MUL64(tx2, matrix[2]) + MUL64(tz2, matrix[3]);

    minx = box_min_x - radius;
    minz = box_min_z - radius;

    maxx = box_max_x + radius;
    maxz = box_max_z + radius;

    if (rx2 > maxx || rx2 < minx || rz2 > maxz || rz2 < minz) {
        return UC_FALSE;
    }

    dminx = rx2 - minx;
    dmaxx = maxx - rx2;

    dminz = rz2 - minz;
    dmaxz = maxz - rz2;

    best = dminx;
    best_x = minx - 1;
    best_z = rz2;

    if (dmaxx < best) {
        best = dmaxx;
        best_x = maxx - 1;
        best_z = rz2;
    }

    if (dminz < best) {
        best = dminz;
        best_x = rx2;
        best_z = minz - 1;
    }

    if (dmaxz < best) {
        best = dmaxz;
        best_x = rx2;
        best_z = maxz + 1;
    }

    // Un-rotate using transpose of the rotation matrix.
    SWAP(matrix[1], matrix[2]);

    *x2 = MUL64(best_x, matrix[0]) + MUL64(best_z, matrix[1]);
    *z2 = MUL64(best_x, matrix[2]) + MUL64(best_z, matrix[3]);

    *x2 += box_mid_x;
    *z2 += box_mid_z;

    return UC_TRUE;
}

// uc_orig: COL_CLIP_XS (fallen/Source/collide.cpp)
// Cohen-Sutherland clip code: point is left of (less than) box minx.
#define COL_CLIP_XS (1 << 0)

// uc_orig: COL_CLIP_XL (fallen/Source/collide.cpp)
// Cohen-Sutherland clip code: point is right of (greater than) box maxx.
#define COL_CLIP_XL (1 << 1)

// uc_orig: COL_CLIP_ZS (fallen/Source/collide.cpp)
// Cohen-Sutherland clip code: point is near (less than) box minz.
#define COL_CLIP_ZS (1 << 2)

// uc_orig: COL_CLIP_ZL (fallen/Source/collide.cpp)
// Cohen-Sutherland clip code: point is far (greater than) box maxz.
#define COL_CLIP_ZL (1 << 3)

// uc_orig: collide_box_with_line (fallen/Source/collide.cpp)
// Returns UC_TRUE if the line segment (lx1,lz1)-(lx2,lz2) intersects the OBB
// defined by [minx..maxx] x [minz..maxz] rotated by yaw around (midx, midz).
// Uses Cohen-Sutherland clip codes in box-local space.
SLONG collide_box_with_line(
    SLONG midx,
    SLONG midz,
    SLONG minx, SLONG minz,
    SLONG maxx, SLONG maxz,
    SLONG yaw,
    SLONG lx1,
    SLONG lz1,
    SLONG lx2,
    SLONG lz2)
{
    SLONG tx1;
    SLONG tz1;
    SLONG tx2;
    SLONG tz2;

    SLONG rx1;
    SLONG rz1;
    SLONG rx2;
    SLONG rz2;

    SLONG ix;
    SLONG iz;

    SLONG matrix[4];
    SLONG useangle;

    SLONG sin_yaw;
    SLONG cos_yaw;

    useangle = -yaw;
    useangle &= 2047;

    sin_yaw = SIN(useangle);
    cos_yaw = COS(useangle);

    matrix[0] = cos_yaw;
    matrix[1] = sin_yaw;
    matrix[2] = -sin_yaw;
    matrix[3] = cos_yaw;

    // Rotate line endpoints into box-local space.
    tx1 = lx1 - midx;
    tz1 = lz1 - midz;

    tx2 = lx2 - midx;
    tz2 = lz2 - midz;

    rx1 = MUL64(tx1, matrix[0]) + MUL64(tz1, matrix[1]);
    rz1 = MUL64(tx1, matrix[2]) + MUL64(tz1, matrix[3]);

    rx2 = MUL64(tx2, matrix[0]) + MUL64(tz2, matrix[1]);
    rz2 = MUL64(tx2, matrix[2]) + MUL64(tz2, matrix[3]);

    UBYTE clip1 = 0;
    UBYTE clip2 = 0;

    if (rx1 < minx) {
        clip1 |= COL_CLIP_XS;
    }
    if (rx1 > maxx) {
        clip1 |= COL_CLIP_XL;
    }
    if (rz1 < minz) {
        clip1 |= COL_CLIP_ZS;
    }
    if (rz1 > maxz) {
        clip1 |= COL_CLIP_ZL;
    }

    if (clip1 == 0) {
        return UC_TRUE;
    }

    if (rx2 < minx) {
        clip2 |= COL_CLIP_XS;
    }
    if (rx2 > maxx) {
        clip2 |= COL_CLIP_XL;
    }
    if (rz2 < minz) {
        clip2 |= COL_CLIP_ZS;
    }
    if (rz2 > maxz) {
        clip2 |= COL_CLIP_ZL;
    }

    if (clip2 == 0) {
        return UC_TRUE;
    }

    UBYTE clip_and = clip1 & clip2;

    if (clip_and) {
        return UC_FALSE;
    }

    UBYTE clip_xor = clip1 ^ clip2;

    if (clip_xor & COL_CLIP_XS) {
        iz = rz1 + (rz2 - rz1) * (minx - rx1) / (rx2 - rx1);

        if (WITHIN(iz, minz, maxz)) {
            return UC_TRUE;
        }
    }

    if (clip_xor & COL_CLIP_XL) {
        iz = rz1 + (rz2 - rz1) * (maxx - rx1) / (rx2 - rx1);

        if (WITHIN(iz, minz, maxz)) {
            return UC_TRUE;
        }
    }

    if (clip_xor & COL_CLIP_ZS) {
        ix = rx1 + (rx2 - rx1) * (minz - rz1) / (rz2 - rz1);

        if (WITHIN(ix, minx, maxx)) {
            return UC_TRUE;
        }
    }

    if (clip_xor & COL_CLIP_ZL) {
        ix = rx1 + (rx2 - rx1) * (maxz - rz1) / (rz2 - rz1);

        // Note: original uses 'iz' (not 'ix') in the WITHIN check here — preserved 1:1.
        if (WITHIN(iz, minx, maxx)) {
            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// uc_orig: SHOCKWAVE_FIND (fallen/Source/collide.cpp)
// Maximum number of Things inspected per create_shockwave call.
#define SHOCKWAVE_FIND 16

// uc_orig: create_shockwave (fallen/Source/collide.cpp)
// Applies area-effect damage to all Things and OB objects within radius of (x,y,z).
// Damage falls off linearly from maxdamage at the centre to 0 at the edge.
// People above hitpoints=30 threshold are knocked down; lighter hits cause recoil.
// Vehicles and Balrogs take damage separately. Camera shakes via FC_explosion.
// just_people=UC_TRUE skips vehicles, specials, and bats.
void create_shockwave(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG radius,
    SLONG maxdamage,
    Thing* p_aggressor, ULONG just_people)
{
    SLONG i;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG hitpoints;

    UWORD found[SHOCKWAVE_FIND];
    SLONG num;

    Thing* p_found;
    ULONG classes;

    // Shake the camera.
    FC_explosion(x, y, z, maxdamage);

    if (just_people) {
        classes = 1 << CLASS_PERSON;
    } else {
        classes = (1 << CLASS_PERSON) | (1 << CLASS_SPECIAL) | (1 << CLASS_VEHICLE) | (1 << CLASS_BAT);
    }
    num = THING_find_sphere(x, y, z, radius, found, SHOCKWAVE_FIND, classes);

    for (i = 0; i < num; i++) {
        p_found = TO_THING(found[i]);

        dx = abs((p_found->WorldPos.X >> 8) - x);
        dy = abs((p_found->WorldPos.Y >> 8) - y);
        dz = abs((p_found->WorldPos.Z >> 8) - z);

        dist = QDIST3(dx, dy, dz);

        extern SLONG is_person_ko(Thing* p_person);

        {
            if (p_found->Class == CLASS_PERSON && !is_person_ko(p_found)) {
                {
                    hitpoints = maxdamage * (radius - dist) / radius;

                    if (p_found->State == STATE_JUMPING || (p_found->State == STATE_MOVEING && p_found->SubState == SUB_STATE_FLIPING)) {
                        hitpoints -= hitpoints >> 1;
                        hitpoints += 1;
                    }

                    if (hitpoints > 30) {
                        knock_person_down(
                            p_found,
                            hitpoints,
                            x,
                            z,
                            p_aggressor);
                    } else {
                        if (p_found->State == STATE_JUMPING || (p_found->State == STATE_MOVEING && p_found->SubState == SUB_STATE_FLIPING)) {
                            // Jumping people don't recoil from a shockwave.
                        } else {
                            set_face_pos(
                                p_found,
                                x,
                                z);

                            set_person_recoil(
                                p_found,
                                ANIM_HIT_FRONT_MID,
                                0);
                        }
                    }
                }
            } else if (p_found->Class == CLASS_SPECIAL) {
                if (p_found->Genus.Special->SpecialType == SPECIAL_MINE) {
                    if (dist < 0x120) {
                        // Mines don't explode immediately; ammo counts down to detonation.
                        p_found->Genus.Special->ammo = (Random() & 0x7) + 5;
                    }
                }
            } else if (p_found->Class == CLASS_VEHICLE) {
                dist -= 0x100;

                if (dist <= 0) {
                    hitpoints = maxdamage;
                } else {
                    hitpoints = maxdamage * (radius - dist) / radius;
                }

                hitpoints <<= 1;

                extern void VEH_reduce_health(
                    Thing* p_car,
                    Thing* p_person,
                    SLONG damage);

                VEH_reduce_health(
                    p_found,
                    p_aggressor,
                    hitpoints >> 1);
            } else if (p_found->Class == CLASS_BAT) {
                // Balrogs are damaged by shockwaves.
                {
                    if (p_aggressor == p_found) {
                        // If the Balrog caused the explosion it doesn't hurt itself.
                    } else {
                        hitpoints = maxdamage * (radius - dist + 0x80) / radius;

                        if (p_found->Genus.Bat->type == BAT_TYPE_BALROG) {
                            // Balrogs are so hard they take quadruple shockwave damage.
                            hitpoints <<= 2;
                        }

                        BAT_apply_hit(
                            p_found,
                            p_aggressor,
                            hitpoints);
                    }
                }
            }
        }
    }

    // Find OB objects within the shockwave.
    if (!just_people) {
        UBYTE mx;
        UBYTE mz;

        SWORD mx1;
        SWORD mz1;
        SWORD mx2;
        SWORD mz2;

        SLONG dx;
        SLONG dy;
        SLONG dz;
        SLONG dist;

        OB_Info* oi;

        mx1 = x - radius >> PAP_SHIFT_LO;
        mz1 = z - radius >> PAP_SHIFT_LO;
        mx2 = x + radius >> PAP_SHIFT_LO;
        mz2 = z + radius >> PAP_SHIFT_LO;

        SATURATE(mx1, 0, PAP_SIZE_LO - 1);
        SATURATE(mz1, 0, PAP_SIZE_LO - 1);
        SATURATE(mx2, 0, PAP_SIZE_LO - 1);
        SATURATE(mz2, 0, PAP_SIZE_LO - 1);

        for (mx = mx1; mx <= mx2; mx++)
            for (mz = mz1; mz <= mz2; mz++) {
                for (oi = OB_find(mx, mz); oi->prim; oi++) {
                    if (prim_objects[oi->prim].damage & PRIM_DAMAGE_DAMAGABLE) {
                        dx = oi->x - x;
                        dy = oi->y - y;
                        dz = oi->z - z;

                        dist = abs(dx) + abs(dy) + abs(dz);

                        if (dist < radius) {
                            hitpoints = maxdamage * (radius - dist) / radius;

                            if (hitpoints > 50) {
                                OB_damage(
                                    oi->index,
                                    x,
                                    z,
                                    oi->x,
                                    oi->z,
                                    p_aggressor);
                            }
                        }
                    }

                    oi += 1;
                }
            }
    }
}

// uc_orig: COLLIDE_find_seethrough_fences (fallen/Source/collide.cpp)
// Marks all fence and outside-door DFacets with FACET_FLAG_SEETHROUGH.
// Called once at level load after all facets are built.
void COLLIDE_find_seethrough_fences()
{
    SLONG i;

    DFacet* df;

    for (i = 1; i < next_dfacet; i++) {
        df = &dfacets[i];

        if (df->FacetType == STOREY_TYPE_FENCE || df->FacetType == STOREY_TYPE_FENCE_FLAT || df->FacetType == STOREY_TYPE_OUTSIDE_DOOR) {
            df->FacetFlags |= FACET_FLAG_SEETHROUGH;
        }
    }
}

// uc_orig: COLLIDE_calc_fastnav_bits (fallen/Source/collide.cpp)
// Rebuilds the COLLIDE_fastnav bitfield. Initially marks every hi-res cell
// as fastnav-capable, then clears cells adjacent to any axis-aligned DFacet.
void COLLIDE_calc_fastnav_bits()
{
    // Mark all squares as fastnav-capable.
    memset(COLLIDE_fastnav, -1, PAP_SIZE_HI * PAP_SIZE_HI >> 3);

    SLONG i;
    SLONG j;
    SLONG k;
    SLONG x;
    SLONG z;
    SLONG dx;
    SLONG dz;
    SLONG mx;
    SLONG mz;
    SLONG len;

    DFacet* df;

    for (i = 1; i < next_dfacet; i++) {
        df = &dfacets[i];

        dx = df->x[1] - df->x[0];
        dz = df->z[1] - df->z[0];

        len = MAX(abs(dx), abs(dz));

        if (!(dx == 0 || dz == 0)) {
            // Skip diagonal facets; fastnav only handles axis-aligned walls.
            continue;
        }

        dx = SIGN(dx);
        dz = SIGN(dz);

        x = df->x[0];
        z = df->z[0];

        for (j = 0; j < len; j++) {
            // Mark the 4 cells surrounding this facet segment as non-fastnav.
            for (k = 0; k < 4; k++) {
                mx = x - (k & 1);
                mz = z - (k >> 1);

                if (WITHIN(mx, 0, PAP_SIZE_HI - 1) && WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
                    COLLIDE_fastnav[mx][mz >> 3] &= ~(1 << (mz & 0x7));
                }
            }

            x += dx;
            z += dz;
        }
    }
}

// uc_orig: COLLIDE_debug_fastnav (fallen/Source/collide.cpp)
// Debug visualisation: draws a cross over each fastnav-capable cell near (world_x, world_z).
void COLLIDE_debug_fastnav(
    SLONG world_x,
    SLONG world_z)
{
    SLONG mx;
    SLONG mz;

    SLONG cx;
    SLONG cy;
    SLONG cz;

    SLONG mx1 = world_x - 0x800 >> 8;
    SLONG mz1 = world_z - 0x800 >> 8;
    SLONG mx2 = world_x + 0x800 >> 8;
    SLONG mz2 = world_z + 0x800 >> 8;

    SATURATE(mx1, 0, PAP_SIZE_HI - 1);
    SATURATE(mz1, 0, PAP_SIZE_HI - 1);
    SATURATE(mx2, 0, PAP_SIZE_HI - 1);
    SATURATE(mz2, 0, PAP_SIZE_HI - 1);

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            if (COLLIDE_can_i_fastnav(mx, mz)) {
                cx = (mx << 8) + 0x80;
                cz = (mz << 8) + 0x80;

                cy = PAP_calc_map_height_at(cx, cz);

                AENG_world_line(
                    cx - 0x10,
                    cy,
                    cz - 0x10,
                    16,
                    0xff00ff,
                    cx + 0x10,
                    cy,
                    cz + 0x10,
                    16,
                    0xff00ff,
                    UC_TRUE);

                AENG_world_line(
                    cx + 0x10,
                    cy,
                    cz - 0x10,
                    16,
                    0xff00ff,
                    cx - 0x10,
                    cy,
                    cz + 0x10,
                    16,
                    0xff00ff,
                    UC_TRUE);
            }
        }
}

// uc_orig: box_box_early_out (fallen/Source/collide.cpp)
// Stub — body not implemented in the original.
void box_box_early_out(
    SLONG box1_mid_x,
    SLONG box1_mid_z,
    SLONG box1_min_x,
    SLONG box1_min_z,
    SLONG box1_max_x,
    SLONG box1_max_z,
    SLONG box1_yaw,
    SLONG box2_mid_x,
    SLONG box2_mid_z,
    SLONG box2_min_x,
    SLONG box2_min_z,
    SLONG box2_max_x,
    SLONG box2_max_z,
    SLONG box2_yaw)
{
}

// uc_orig: box_circle_early_out (fallen/Source/collide.cpp)
// Forward declaration only in original — no definition. Declared here for completeness.
// Stub.
void box_circle_early_out(
    SLONG box1_mid_x,
    SLONG box1_mid_z,
    SLONG box1_min_x,
    SLONG box1_min_z,
    SLONG box1_max_x,
    SLONG box1_max_z,
    SLONG box1_yaw,
    SLONG cx,
    SLONG cz,
    SLONG cradius)
{
}
