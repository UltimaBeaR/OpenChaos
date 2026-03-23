// Temporary includes: game.h, supermap.h, memory.h (fallen) not yet migrated
#include "fallen/Headers/Game.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/geometry/cone.h"
#include "world/map/pap.h"
#include "world/map/supermap.h"
#include "fallen/Headers/memory.h"
#include <math.h>
#include "engine/graphics/geometry/cone_globals.h"

// uc_orig: CONE_interpolate_colour (fallen/DDEngine/Source/cone.cpp)
// Interpolates between two packed ARGB colours by scalar v in [0,1].
// Uses a fixed-point trick: reads mantissa bits of (32768 + v) as an 8-bit fraction.
static ULONG CONE_interpolate_colour(float v, ULONG colour1, ULONG colour2)
{
    SLONG rb1, rb2, drb, rba;
    SLONG ga1, ga2, dga, gaa;

    union {
        float vfloat;
        ULONG vfixed8;
    };

    SLONG answer;

    if (colour1 == colour2) {
        return colour1;
    }

    if (v < 0.01F) {
        return colour1;
    }
    if (v > 0.99F) {
        return colour2;
    }

    // Extract the fractional part as 8-bit fixed point via float bit pattern trick.
    vfloat = 32768.0F + v;
    vfixed8 &= 0xff;

    // Interpolate red and blue channels simultaneously (they occupy bits 0-7 and 16-23).
    rb1 = colour1 & (0x00ff00ff);
    rb2 = colour2 & (0x00ff00ff);

    if (rb1 != rb2) {
        drb = rb2 - rb1;
        drb *= vfixed8;
        drb >>= 8;
        rba = rb1 + drb;
        rba &= 0x00ff00ff;
    } else {
        rba = rb1;
    }

    // Interpolate green and alpha channels simultaneously.
    ga1 = colour1 >> 8;
    ga2 = colour2 >> 8;

    ga1 &= 0x00ff00ff;
    ga2 &= 0x00ff00ff;

    if (ga1 != ga2) {
        dga = ga2 - ga1;
        dga *= vfixed8;
        gaa = (ga1 << 8) + dga;
        gaa &= 0xff00ff00;
    } else {
        gaa = ga1 << 8;
    }

    answer = rba | gaa;

    return answer;
}

// uc_orig: CONE_insert_points (fallen/DDEngine/Source/cone.cpp)
// Inserts two new points into the CONE_point array after the given index.
// Used when a cone ray crosses the boundary of a clipping polygon.
static void CONE_insert_points(
    SLONG point,
    float x1,
    float y1,
    float z1,
    ULONG colour1,
    float x2,
    float y2,
    float z2,
    ULONG colour2)
{
    SLONG i;

    ASSERT(WITHIN(point, 0, CONE_point_upto - 1));

    if (CONE_point_upto >= CONE_MAX_POINTS - 1) {
        return;
    }

    for (i = CONE_point_upto + 1; i - 2 >= point; i--) {
        CONE_point[i] = CONE_point[i - 2];
    }

    CONE_point[point + 0].x = x1;
    CONE_point[point + 0].y = y1;
    CONE_point[point + 0].z = z1;
    CONE_point[point + 0].colour = colour1;

    CONE_point[point + 1].x = x2;
    CONE_point[point + 1].y = y2;
    CONE_point[point + 1].z = z2;
    CONE_point[point + 1].colour = colour2;

    CONE_point_upto += 2;
}

// uc_orig: CONE_create (fallen/DDEngine/Source/cone.cpp)
void CONE_create(
    float x,
    float y,
    float z,
    float dx,
    float dy,
    float dz,
    float length,
    float radius,
    ULONG colour_start,
    ULONG colour_end,
    SLONG detail)
{
    SLONG i;

    float ax;
    float ay;
    float az;

    float bx;
    float by;
    float bz;

    float px;
    float py;
    float pz;

    float dist;

    CONE_origin_x = x;
    CONE_origin_y = y;
    CONE_origin_z = z;
    CONE_origin_colour = colour_start;

    // Normalize direction.
    dist = dx * dx + dy * dy + dz * dz;
    dist = sqrt(dist);

    dx = dx * (1.0F / dist);
    dy = dy * (1.0F / dist);
    dz = dz * (1.0F / dist);

    // Build two vectors orthogonal to (dx,dy,dz) using a perpendicular trick.
    px = dy;
    py = dz;
    pz = dx;

    // a = d x p (orthogonal to d)
    ax = dy * pz - dz * py;
    ay = dz * px - dx * pz;
    az = dx * py - dy * px;

    // b = d x a (orthogonal to both a and d)
    bx = dy * az - dz * ay;
    by = dz * ax - dx * az;
    bz = dx * ay - dy * ax;

    ASSERT(WITHIN(detail, 0, 256));

    CONE_point_upto = 4;
    CONE_point_upto += detail * (CONE_MAX_POINTS - 4) >> 8;

    CONE_end_x = CONE_origin_x + dx * length;
    CONE_end_y = CONE_origin_y + dy * length;
    CONE_end_z = CONE_origin_z + dz * length;

    // Build the base circle points.
    float angle;
    float along_a;
    float along_b;

    CONE_Point* cp;

    for (i = 0; i < CONE_point_upto; i++) {
        cp = &CONE_point[i];

        angle = float(i) * (2.0F * PI / float(CONE_point_upto));
        along_a = cos(angle) * radius;
        along_b = sin(angle) * radius;

        cp->x = CONE_end_x + along_a * ax + along_b * bx;
        cp->y = CONE_end_y + along_a * ay + along_b * by;
        cp->z = CONE_end_z + along_a * az + along_b * bz;
        cp->colour = colour_end;
    }
}

// uc_orig: CONE_clip (fallen/DDEngine/Source/cone.cpp)
void CONE_clip(
    CONE_Poly p[],
    SLONG num_points)
{
    SLONG i;
    SLONG j;

    SLONG p1;
    SLONG p2;

    float av;
    float aw;

    float bv;
    float bw;

    float along;

    float along_v;
    float along_w;

    float len_v;
    float len_w;

    float overlen_v;
    float overlen_w;

    float dpc;

    float px;
    float py;
    float pz;

    float dx;
    float dy;
    float dz;
    float dist;
    float dist_want;
    float dist_mul;

    float ix;
    float iy;
    float iz;
    ULONG icolour;

    float insert_x1;
    float insert_y1;
    float insert_z1;
    ULONG insert_colour1;

    float insert_x2;
    float insert_y2;
    float insert_z2;
    ULONG insert_colour2;

    float dprod;

    CONE_Poly* pp;
    CONE_Point* cp;

#define CONE_MAX_POLY_POINTS 16

    struct
    {
        float along_v;
        float along_w;

    } along_p[CONE_MAX_POLY_POINTS];

    struct
    {
        UBYTE failed_side;
        UBYTE failed_dprod;
        UWORD shit;
        float ix;
        float iy;
        float iz;
        ULONG icolour;
        float dprod[CONE_MAX_POLY_POINTS];

    } point_info[2],
        *point_info_now,
        *point_info_then,
        *point_info_spare;

#define CONE_SWAP_POINT_INFO(a, b) \
    {                              \
        point_info_spare = (a);    \
        (a) = (b);                 \
        (b) = point_info_spare;    \
    }

    point_info_now = &point_info[0];
    point_info_then = &point_info[1];

    ASSERT(WITHIN(num_points, 3, CONE_MAX_POLY_POINTS));

    // Compute the plane of the clipping polygon from its first, second, and last points.
    CONE_Poly* vp = &p[1];
    CONE_Poly* wp = &p[num_points - 1];

    float vx = vp->x - p[0].x;
    float vy = vp->y - p[0].y;
    float vz = vp->z - p[0].z;

    float wx = wp->x - p[0].x;
    float wy = wp->y - p[0].y;
    float wz = wp->z - p[0].z;

    len_v = sqrt(vx * vx + vy * vy + vz * vz);
    len_w = sqrt(wx * wx + wy * wy + wz * wz);

    overlen_v = 1.0F / len_v;
    overlen_w = 1.0F / len_w;

    // Plane normal.
    float nx = vy * wz - vz * wy;
    float ny = vz * wx - vx * wz;
    float nz = vx * wy - vy * wx;

    // Check which side of the plane the cone origin is on.
    float ox = CONE_origin_x - p[0].x;
    float oy = CONE_origin_y - p[0].y;
    float oz = CONE_origin_z - p[0].z;

    float dpo = ox * nx + oy * ny + oz * nz;

    if (dpo >= 0.0F) {
        // Origin is behind the polygon — nothing to clip.
        return;
    }

    // Project the polygon points onto the v and w basis vectors.
    along_p[0].along_v = 0.0F;
    along_p[0].along_w = 0.0F;

    along_p[1].along_v = len_v;
    along_p[1].along_w = 0.0F;

    along_p[num_points - 1].along_v = 0.0F;
    along_p[num_points - 1].along_w = len_w;

    for (i = 2; i < num_points - 1; i++) {
        pp = &p[i];

        px = pp->x - p[0].x;
        py = pp->y - p[0].y;
        pz = pp->z - p[0].z;

        along_p[i].along_v = (px * vx + py * vy + pz * vz) * overlen_v;
        along_p[i].along_w = (px * wx + py * wy + pz * wz) * overlen_w;
    }

    point_info_then->failed_side = UC_TRUE;

    for (i = 0; i < CONE_point_upto; i++) {
        cp = &CONE_point[i];

        px = cp->x - p[0].x;
        py = cp->y - p[0].y;
        pz = cp->z - p[0].z;

        dpc = px * nx + py * ny + pz * nz;

        if (dpc <= 0.0F) {
            point_info_now->failed_side = UC_TRUE;
        } else {
            point_info_now->failed_side = UC_FALSE;

            // Find intersection of the cone ray with the polygon plane.
            along = dpo / (dpo - dpc);

            ASSERT(WITHIN(along, 0.0F, 1.0F));

            ix = CONE_origin_x + along * (cp->x - CONE_origin_x);
            iy = CONE_origin_y + along * (cp->y - CONE_origin_y);
            iz = CONE_origin_z + along * (cp->z - CONE_origin_z);
            icolour = CONE_interpolate_colour(along, CONE_origin_colour, cp->colour);

            point_info_now->ix = ix;
            point_info_now->iy = iy;
            point_info_now->iz = iz;
            point_info_now->icolour = icolour;

            // Project the intersection onto the polygon plane and test it against each polygon edge.
            px = ix - p[0].x;
            py = iy - p[0].y;
            pz = iz - p[0].z;

            along_v = (px * vx + py * vy + pz * vz) * overlen_v;
            along_w = (px * wx + py * wy + pz * wz) * overlen_w;

            for (j = 0; j < num_points; j++) {
                p1 = j + 0;
                p2 = j + 1;

                if (p2 == num_points) {
                    p2 = 0;
                }

                av = along_p[p2].along_v - along_p[p1].along_v;
                aw = along_p[p2].along_w - along_p[p1].along_w;

                bv = along_v - along_p[p1].along_v;
                bw = along_w - along_p[p1].along_w;

                dprod = av * bv + aw * bw;

                point_info_now->dprod[j] = dprod;

                if (dprod < 0.0F) {
                    // Intersection is outside the polygon.
                    point_info_now->failed_dprod = j;

                    if (point_info_then->failed_side == UC_FALSE && point_info_then->failed_dprod == num_points) {
                        // Previous ray was inside the polygon, so insert a boundary point.
                        along = point_info_then->dprod[j] / (point_info_then->dprod[j] - dprod);

                        insert_x1 = point_info_then->ix + along * (ix - point_info_then->ix);
                        insert_y1 = point_info_then->iy + along * (iy - point_info_then->iy);
                        insert_z1 = point_info_then->iz + along * (iz - point_info_then->iz);
                        insert_colour1 = CONE_interpolate_colour(along, point_info_then->icolour, icolour);

                        // The second inserted point is scaled to match the full ray length.
                        dx = cp->x - CONE_origin_x;
                        dy = cp->y - CONE_origin_y;
                        dz = cp->z - CONE_origin_z;

                        dist_want = sqrt(dx * dx + dy * dy + dz * dz);

                        dx = insert_x1 - CONE_origin_x;
                        dy = insert_y1 - CONE_origin_y;
                        dz = insert_z1 - CONE_origin_z;

                        dist = sqrt(dx * dx + dy * dy + dz * dz);

                        dist_mul = dist_want / dist;

                        dx *= dist_mul;
                        dy *= dist_mul;
                        dz *= dist_mul;

                        insert_x2 = CONE_origin_x + dx;
                        insert_y2 = CONE_origin_y + dy;
                        insert_z2 = CONE_origin_z + dz;
                        insert_colour2 = cp->colour;

                        CONE_insert_points(
                            i,
                            insert_x1,
                            insert_y1,
                            insert_z1,
                            insert_colour1,
                            insert_x2,
                            insert_y2,
                            insert_z2,
                            insert_colour2);

                        i += 2;
                    }

                    goto point_outside_polygon;
                }
            }

            // Intersection is inside the polygon: clamp this cone point to the polygon surface.
            point_info_now->failed_dprod = num_points;

            cp->x = ix;
            cp->y = iy;
            cp->z = iz;
            cp->colour = icolour;

            if (point_info_then->failed_side == UC_FALSE && point_info_then->failed_dprod < num_points) {
                // Previous ray was outside the polygon: insert boundary entry point.
                along = point_info_then->dprod[point_info_then->failed_dprod] / (point_info_then->dprod[point_info_then->failed_dprod] - point_info_now->dprod[point_info_then->failed_dprod]);

                insert_x2 = point_info_then->ix + along * (ix - point_info_then->ix);
                insert_y2 = point_info_then->iy + along * (iy - point_info_then->iy);
                insert_z2 = point_info_then->iz + along * (iz - point_info_then->iz);
                insert_colour2 = CONE_interpolate_colour(along, point_info_then->icolour, icolour);

                ASSERT(WITHIN(i, 1, CONE_point_upto));

                CONE_Point* lp = &CONE_point[i - 1];

                dx = lp->x - CONE_origin_x;
                dy = lp->y - CONE_origin_y;
                dz = lp->z - CONE_origin_z;

                dist_want = sqrt(dx * dx + dy * dy + dz * dz);

                dx = insert_x2 - CONE_origin_x;
                dy = insert_y2 - CONE_origin_y;
                dz = insert_z2 - CONE_origin_z;

                dist = sqrt(dx * dx + dy * dy + dz * dz);

                dist_mul = dist_want / dist;

                dx *= dist_mul;
                dy *= dist_mul;
                dz *= dist_mul;

                insert_x1 = CONE_origin_x + dx;
                insert_y1 = CONE_origin_y + dy;
                insert_z1 = CONE_origin_z + dz;
                insert_colour1 = lp->colour;

                CONE_insert_points(
                    i,
                    insert_x1,
                    insert_y1,
                    insert_z1,
                    insert_colour1,
                    insert_x2,
                    insert_y2,
                    insert_z2,
                    insert_colour2);

                i += 2;
            }

        point_outside_polygon:;
        }

        CONE_SWAP_POINT_INFO(
            point_info_now,
            point_info_then);
    }
}

// uc_orig: CONE_intersect_square (fallen/DDEngine/Source/cone.cpp)
// Clips the cone against all collision facets in the given lo-res map square.
static void CONE_intersect_square(
    SLONG mx,
    SLONG mz)
{
    SLONG i;

    SLONG f_list;
    SLONG facet;
    SLONG exit;

    DFacet* df;

    CONE_Poly poly[4];

    if (!WITHIN(mx, 0, PAP_SIZE_LO - 1) || !WITHIN(mz, 0, PAP_SIZE_LO - 1)) {
        return;
    }

    PAP_Lo* pl = &PAP_2LO(mx, mz);

    f_list = pl->ColVectHead;

    if (f_list) {
        exit = UC_FALSE;

        while (!exit) {
            facet = facet_links[f_list];

            ASSERT(facet);

            if (facet < 0) {
                facet = -facet;
                exit = UC_TRUE;
            }

            // Deduplicate: skip facets we already processed this cone-map traversal.
            for (i = 0; i < CONE_COLVECT_DONE; i++) {
                if (CONE_colvect_done[i] == facet) {
                    goto ignore_this_facet;
                }
            }

            ASSERT(WITHIN(CONE_colvect_done_upto, 0, CONE_COLVECT_DONE - 1));
            ASSERT(CONE_COLVECT_DONE == 4 || CONE_COLVECT_DONE == 8);

            CONE_colvect_done[CONE_colvect_done_upto] = facet;
            CONE_colvect_done_upto += 1;
            CONE_colvect_done_upto &= CONE_COLVECT_DONE - 1;

            df = &dfacets[facet];

            poly[0].x = float(df->x[1] << 8);
            poly[0].y = float(df->Y[1]);
            poly[0].z = float(df->z[1] << 8);

            poly[1].x = float(df->x[1] << 8);
            poly[1].y = float(df->Y[1] + BLOCK_SIZE * df->Height);
            poly[1].z = float(df->z[1] << 8);

            poly[2].x = float(df->x[0] << 8);
            poly[2].y = float(df->Y[0] + BLOCK_SIZE * df->Height);
            poly[2].z = float(df->z[0] << 8);

            poly[3].x = float(df->x[0] << 8);
            poly[3].y = float(df->Y[0]);
            poly[3].z = float(df->z[0] << 8);

            if (df->FacetType == STOREY_TYPE_NORMAL_FOUNDATION) {
                // Foundations extend below ground.
                poly[0].y -= 256.0F;
                poly[3].y -= 256.0F;
            }

            CONE_clip(poly, 4);

        ignore_this_facet:;

            f_list++;
        }
    }

    // Walkable face intersection is commented out in the original.
}

// uc_orig: CONE_intersect_with_map (fallen/DDEngine/Source/cone.cpp)
void CONE_intersect_with_map()
{
    SLONG i;
    SLONG j;
    SLONG k;

    SLONG x;
    SLONG z;

    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;

    SLONG dx;
    SLONG dz;

    SLONG lx;
    SLONG lz;

    SLONG mx;
    SLONG mz;

    SLONG len;
    SLONG steps;

#define CONE_MAX_DONE 4

    struct
    {
        SLONG x;
        SLONG z;

    } done[CONE_MAX_DONE] = { { -1, -1 }, { -1, -1 }, { -1, -1 }, { -1, -1 } };
    SLONG done_upto = 0;

    for (i = 0; i < CONE_COLVECT_DONE; i++) {
        CONE_colvect_done[i] = -1;
    }

    CONE_colvect_done_upto = 0;

    x1 = SLONG(CONE_origin_x);
    z1 = SLONG(CONE_origin_z);

    x2 = SLONG(CONE_end_x);
    z2 = SLONG(CONE_end_z);

    dx = x2 - x1;
    dz = z2 - z1;

    len = QDIST2(abs(dx), abs(dz)) + 1;

    // Four steps per lo-res mapsquare.
    dx = (dx << 8) / len;
    dz = (dz << 8) / len;

    steps = len >> 8;
    steps += 1;

    x = x1;
    z = z1;

    lx = -dz;
    lz = dx;

    for (i = 0; i < steps; i++) {
        for (j = 0; j < 3; j++) {
            mx = x - lx + lx * j >> PAP_SHIFT_LO;
            mz = z - lz + lz * j >> PAP_SHIFT_LO;

            for (k = 0; k < CONE_MAX_DONE; k++) {
                if (done[k].x == mx && done[k].z == mz) {
                    goto already_done_this_square;
                }
            }

            ASSERT(WITHIN(done_upto, 0, CONE_MAX_DONE - 1));

            done[done_upto].x = mx;
            done[done_upto].z = mz;

            ASSERT(CONE_MAX_DONE == 4 || CONE_MAX_DONE == 8);

            done_upto += 1;
            done_upto &= CONE_MAX_DONE - 1;

            CONE_intersect_square(mx, mz);

        already_done_this_square:;
        }

        x += dx;
        z += dz;
    }
}

// uc_orig: CONE_draw (fallen/DDEngine/Source/cone.cpp)
void CONE_draw()
{
    SLONG i;

    SLONG p1;
    SLONG p2;

    POLY_Point ppo;

    POLY_Point* tri[3];

    CONE_Point* cp;
    CONE_Point* cp1;
    CONE_Point* cp2;

    // Project the cone origin.
    POLY_transform(
        CONE_origin_x,
        CONE_origin_y,
        CONE_origin_z,
        &ppo);

    if (!ppo.IsValid()) {
        // Origin is behind the camera: nothing visible.
        return;
    }

    ppo.u = 0.0F;
    ppo.v = 0.0F;
    ppo.colour = CONE_origin_colour;
    ppo.specular = 0x00000000;

    // Project all base circle points.
    for (i = 0; i < CONE_point_upto; i++) {
        cp = &CONE_point[i];

        POLY_transform(
            cp->x,
            cp->y,
            cp->z,
            &cp->pp);

        cp->pp.colour = cp->colour;
        cp->pp.specular = 0x00000000;
        cp->pp.u = 0.0F;
        cp->pp.v = 0.0F;
    }

    // Emit the triangle fan.
    tri[0] = &ppo;

    for (i = 0; i < CONE_point_upto; i++) {
        p1 = i + 0;
        p2 = i + 1;

        if (p2 == CONE_point_upto) {
            p2 = 0;
        }

        tri[1] = &CONE_point[p1].pp;
        tri[2] = &CONE_point[p2].pp;

        if (POLY_valid_triangle(tri)) {
            POLY_add_triangle(tri, POLY_PAGE_ADDITIVE, UC_FALSE);
        }
    }
}
