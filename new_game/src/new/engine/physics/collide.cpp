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
// Temporary: edmap.h for DFacet, CollisionVect
#include "fallen/Headers/edmap.h"

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
#include "fallen/Headers/animate.h"     // SUB_OBJECT_LEFT_FOOT, COP_ROPER_ANIM_LADDER_END_L, ANIM_OFF_LADDER_TOP, ACTION_CLIMBING
#include "actors/core/interact.h"       // calc_sub_objects_position
#include "actors/core/state.h"          // set_generic_person_state_function, STATE_CLIMB_LADDER
#include "actors/core/thing.h"          // move_thing_on_map
#include "actors/characters/person.h"   // person_splash, set_person_climb_ladder, set_person_drop_down, locked_anim_change_end_type
#include "world/map/pap.h"              // PAP_calc_height_at, PAP_calc_height_at_thing
#include "engine/graphics/resources/console.h"  // CONSOLE_text
#include "engine/input/keyboard_globals.h"      // ControlFlag

// Temporary: add_debug_line (aeng.cpp, not yet migrated)
extern void add_debug_line(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG colour);

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
        *on = FALSE;

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
        *on = FALSE;

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
    *on = TRUE;

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
        *on = FALSE;

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
        *on = FALSE;

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
    *on = TRUE;

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
// Tries to put p_thing onto the ladder at facet index.  Returns TRUE on
// success.  Note: the call path from interfac.cpp is commented out in the
// pre-release source, so players cannot grab ladders from below; AI uses
// set_person_climb_ladder directly.
SLONG mount_ladder(Thing* p_thing, SLONG facet)
{
    if (ok_to_mount_ladder(p_thing, &dfacets[facet])) {
        set_person_climb_ladder(p_thing, facet);

        return TRUE;
    }

    return FALSE;
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

    SLONG best_dist = INFINITY;
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
