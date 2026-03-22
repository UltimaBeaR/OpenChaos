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

// Temporary: calc_height_on_face / calc_height_on_rface (not yet migrated)
extern void calc_height_on_face(SLONG x, SLONG z, SLONG face, SLONG* ret_y);
extern void calc_height_on_rface(SLONG x, SLONG z, SLONG face, SLONG* ret_y);

// Temporary: set_tween_for_dy (actors/characters/person.cpp, already migrated)
extern void set_tween_for_dy(Thing* p_person, SLONG dy);

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
