#include <MFStdLib.h>
#include "fallen/Headers/Game.h"
#include "fallen/Headers/pap.h"
#include "fallen/Headers/ns.h"
#include "world/map/supermap.h"
#include "fallen/Headers/fmatrix.h"
#include "fallen/Headers/memory.h"
#include "fallen/Headers/ware.h"
#include "fallen/Headers/mav.h"
#include "actors/characters/anim_ids.h"
#include "fallen/Headers/FMatrix.h"
#include "fallen/Headers/prim_draw.h"
#include "actors/core/interact.h"
#include "actors/core/interact_globals.h"

// Forward declarations for functions not yet migrated to new/.
// uc_orig: nearest_point_on_line_and_dist_and_along (fallen/Source/interact.cpp)
extern SLONG nearest_point_on_line_and_dist_and_along(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_z, SLONG* ret_along);
// uc_orig: nearest_point_on_line_and_dist_calc_y (fallen/Source/interact.cpp)
extern SLONG nearest_point_on_line_and_dist_calc_y(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG a, SLONG b, SLONG* ret_x, SLONG* ret_y, SLONG* ret_z);
// uc_orig: does_fence_lie_along_line (fallen/Source/interact.cpp)
extern SLONG does_fence_lie_along_line(SLONG fx1, SLONG fz1, SLONG fx2, SLONG fz2);
// uc_orig: correct_pos_for_ladder (fallen/Headers/collide.h)
extern void correct_pos_for_ladder(struct DFacet* p_facet, SLONG* px, SLONG* pz, SLONG* angle, SLONG scale);
// uc_orig: person_get_scale (fallen/Source/Person.cpp)
extern SLONG person_get_scale(Thing* p_person);
// uc_orig: WARE_calc_height_at (fallen/Source/ware.cpp)
extern SLONG WARE_calc_height_at(SLONG ware, SLONG x, SLONG z);

// Macro used in find_grab_face wall-check.
// uc_orig: CHECK_WIDTH (fallen/Source/interact.cpp)
#define CHECK_WIDTH 32
// uc_orig: CHECK_FORWARD (fallen/Source/interact.cpp)
#define CHECK_FORWARD 16

// uc_orig: ON_MAP (fallen/Source/interact.cpp)
#define ON_MAP(x, z) (((x) >= 0) && ((z) >= 0) && ((x) < MAP_WIDTH) && ((z) < MAP_HEIGHT))

// Returns angle (0..2047) from a displacement vector (dx, dz).
// uc_orig: calc_angle (fallen/Source/interact.cpp)
static SLONG calc_angle(SLONG dx, SLONG dz)
{
    SLONG angle = 0;
    angle = Arctan(-dx, dz) + 1024;
    angle = angle & 2047;
    return (angle);
}

// Returns signed angular difference between two angles, clamped to [-1024, +1024].
// uc_orig: angle_diff (fallen/Source/interact.cpp)
static SLONG angle_diff(SLONG angle1, SLONG angle2)
{
    SLONG diff;
    diff = angle1 - angle2;
    if (diff > +1024) {
        diff -= 2048;
    } else if (diff < -1024) {
        diff += 2048;
    }
    return diff;
}

// Checks whether the given direction is a valid grab angle against a wall edge.
// Always returns 1 — validation was disabled in the original.
// uc_orig: valid_grab_angle (fallen/Source/interact.cpp)
static SLONG valid_grab_angle(SLONG angle, SLONG dx, SLONG dz)
{
    SLONG wall_angle, diff;
    return (1);

    wall_angle = calc_angle(dx, dz);
    wall_angle += 512;

    diff = angle_diff(angle, wall_angle);
    diff = abs(diff);
    if (diff < 256 || abs(diff - 1024) < 256)
        return (1);
    else {
        return (0);
    }
}

// Returns the Y height of a cable facet at fractional position `along` (0..CABLE_ALONG_MAX).
// The cable sags in a cosine-curve shape with a dip at the midpoint.
// uc_orig: find_cable_y_along (fallen/Source/interact.cpp)
SLONG find_cable_y_along(struct DFacet* p_facet, SLONG along)
{
    SLONG max_at, y;
    SLONG angle_step1, angle_step2, count;

    angle_step1 = (SWORD)p_facet->StyleIndex;
    angle_step2 = (SWORD)p_facet->Building;
    count = p_facet->Height;

    max_at = (512 << CABLE_ALONG_SHIFT) / angle_step1;
    max_at /= count;

    if (along < max_at) {
        SLONG step, angle;
        step = (along * count);
        step = (step * angle_step1) >> CABLE_ALONG_SHIFT;
        angle = (-512 + step + 2048) & 2047;
        y = -(COS(angle) * p_facet->FHeight) >> (16 - 6);
    } else {
        SLONG step, angle;
        step = ((CABLE_ALONG_MAX - along) * count);
        step = -(step * angle_step2) >> CABLE_ALONG_SHIFT;
        angle = (-512 + step + 2048) & 2047;
        y = -(COS(angle) * p_facet->FHeight) >> (16 - 6);
    }

    {
        SLONG dy;
        dy = p_facet->Y[1] - p_facet->Y[0];
        dy = ((dy * along) >> CABLE_ALONG_SHIFT) + p_facet->Y[0];
        y += dy;
    }
    return (y);
}

// Returns fractional position along a cable facet for world XZ point (ax, az).
// Uses axis with larger extent for precision; result is in CABLE_ALONG_SHIFT fixed-point.
// uc_orig: get_cable_along (fallen/Source/interact.cpp)
static SLONG get_cable_along(SLONG facet, SLONG ax, SLONG az)
{
    SLONG dx;
    SLONG dz;
    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;
    SLONG along;
    struct DFacet* p_facet;

    p_facet = &dfacets[facet];

    x1 = p_facet->x[0] << 8;
    x2 = p_facet->x[1] << 8;
    z1 = p_facet->z[0] << 8;
    z2 = p_facet->z[1] << 8;

    dx = x2 - x1;
    dz = z2 - z1;

    ax -= x1;
    az -= z1;

    if (abs(dx) > abs(dz)) {
        along = (ax << CABLE_ALONG_SHIFT) / dx;
    } else {
        if (dz == 0) {
            dz = 1;
        }
        along = (az << CABLE_ALONG_SHIFT) / dz;
    }

    return along;
}

// Finds the nearest grab point on a cable facet within the search radius.
// Returns 1 if found. Uses WITHIN check to reject grabs at cable endpoints.
// uc_orig: check_grab_cable_facet (fallen/Source/interact.cpp)
SLONG check_grab_cable_facet(SLONG facet, SLONG* grab_x, SLONG* grab_y, SLONG* grab_z, SLONG* grab_angle, SLONG radius, SLONG dy, SLONG x, SLONG y, SLONG z)
{
    struct DFacet* p_facet;
    SLONG near_x, near_z, along;
    SLONG dist;
    SLONG cable_y;

    p_facet = &dfacets[facet];

    dist = nearest_point_on_line_and_dist_and_along(
        p_facet->x[0] << 8, p_facet->z[0] << 8,
        p_facet->x[1] << 8, p_facet->z[1] << 8,
        x, z,
        &near_x,
        &near_z,
        &along);

    if (dist > radius) {
        return (0);
    }

    if (!WITHIN(along, 4, 252)) {
        return (0);
    }

    cable_y = find_cable_y_along(p_facet, along << (CABLE_ALONG_SHIFT - 8));

    if (abs(cable_y - y) > dy) {
        return (0);
    }

    *grab_x = near_x;
    *grab_z = near_z;
    *grab_y = cable_y;
    {
        SLONG dx, dz, angle;
        dx = p_facet->x[1] - p_facet->x[0] << 8;
        dz = p_facet->z[1] - p_facet->z[0] << 8;
        *grab_angle = calc_angle(dx, dz);
    }
    return (1);
}

// Finds the nearest grab point on a ladder facet within the search radius.
// Returns 1 if found, -1 if player is above ladder top (should keep falling to grab), 0 if miss.
// uc_orig: check_grab_ladder_facet (fallen/Source/interact.cpp)
SLONG check_grab_ladder_facet(SLONG facet, SLONG* grab_x, SLONG* grab_y, SLONG* grab_z, SLONG* grab_angle, SLONG radius, SLONG dy, SLONG x, SLONG y, SLONG z)
{
    struct DFacet* p_facet;
    SLONG near_x, near_z, along;
    SLONG dist;
    SLONG cable_y;
    SLONG top, bot;

    p_facet = &dfacets[facet];

    dist = nearest_point_on_line_and_dist_and_along(
        p_facet->x[0] << 8, p_facet->z[0] << 8,
        p_facet->x[1] << 8, p_facet->z[1] << 8,
        x, z,
        &near_x,
        &near_z,
        &along);

    if (dist > (radius >> 1) || along == 0 || along == 255) {
        return (0);
    }

    bot = p_facet->Y[0];
    top = bot + p_facet->Height * 64;

    if (y > top || y < bot) {
        if (y > top && y < top + 64) {
            // Still above top of ladder — tell grab_ledge to keep falling until low enough.
            return (-1);
        }
        return (0);
    }

    correct_pos_for_ladder(p_facet, &near_x, &near_z, grab_angle, 256);
    *grab_x = near_x;
    *grab_z = near_z;
    *grab_y = y;
    return (1);
}

// Two-pass search for a grabbable surface near the given world position.
// Pass 0: hi-res roof faces (PAP_hi). Pass 1: lo-res walkable faces (PAP_2LO walkable list).
// Also checks cable and ladder facets in the low-res collision vect list first.
// Returns the face index if found, 0 otherwise. *type: 0=surface, 1=cable, 2=ladder.
// uc_orig: find_grab_face (fallen/Source/interact.cpp)
SLONG find_grab_face(
    SLONG x, SLONG y, SLONG z,
    SLONG radius, SLONG dy, SLONG angle,
    SLONG* grab_x, SLONG* grab_y, SLONG* grab_z,
    SLONG* grab_angle,
    SLONG ignore_building,
    SLONG trench,
    SLONG* type,
    Thing* p_person)
{
    SLONG i;
    SLONG dx;
    SLONG dz;
    SLONG x1, z1;
    SLONG x2, z2;
    SLONG mx, mz;
    SLONG index;
    SLONG face;
    SLONG near_x, near_y, near_z;
    SLONG dist;
    SLONG mid_x, mid_z;
    SLONG angle_to_face;
    SLONG count = 0, count2 = 0;
    SLONG cable = 0;
    SLONG thing;
    Thing* p_thing;
    SLONG pass = 0;

    best_dist = radius;
    *type = 0;

    x1 = x - radius - 0x100 >> PAP_SHIFT_LO;
    z1 = z - radius - 0x100 >> PAP_SHIFT_LO;
    x2 = x + radius + 0x100 >> PAP_SHIFT_LO;
    z2 = z + radius + 0x100 >> PAP_SHIFT_LO;

    SATURATE(x1, 0, PAP_SIZE_LO - 1);
    SATURATE(x2, 0, PAP_SIZE_LO - 1);
    SATURATE(z1, 0, PAP_SIZE_LO - 1);
    SATURATE(z2, 0, PAP_SIZE_LO - 1);

    for (mx = x1; mx <= x2; mx++)
        for (mz = z1; mz <= z2; mz++) {
            SLONG facet, exit;

            index = PAP_2LO(mx, mz).ColVectHead;

            for (exit = 0; exit == 0; index++) {
                facet = facet_links[index];
                if (facet < 0) {
                    exit = 1;
                    facet = -facet;
                }
                if (dfacets[facet].FacetType == STOREY_TYPE_CABLE) {
                    SLONG grab;
                    grab = check_grab_cable_facet(facet, grab_x, grab_y, grab_z, grab_angle, radius + 30, dy, x, y, z);
                    if (grab) {
                        *type = 1;
                        return (facet);
                    }
                } else if (dfacets[facet].FacetType == STOREY_TYPE_LADDER) {
                    SLONG grab;
                    grab = check_grab_ladder_facet(facet, grab_x, grab_y, grab_z, grab_angle, radius, dy, x, y, z);
                    if (grab) {
                        if (grab == -1) {
                            return (0);
                        } else {
                            *type = 2;
                            return (facet);
                        }
                    }
                }
            }
        }

round_again:;

    if (pass == 0) {
        x1 = x - radius - 0x100 >> PAP_SHIFT_HI;
        z1 = z - radius - 0x100 >> PAP_SHIFT_HI;
        x2 = x + radius + 0x100 >> PAP_SHIFT_HI;
        z2 = z + radius + 0x100 >> PAP_SHIFT_HI;
        SATURATE(x1, 0, PAP_SIZE_HI - 1);
        SATURATE(x2, 0, PAP_SIZE_HI - 1);
        SATURATE(z1, 0, PAP_SIZE_HI - 1);
        SATURATE(z2, 0, PAP_SIZE_HI - 1);
    } else {
        x1 = x - radius - 0x100 >> PAP_SHIFT_LO;
        z1 = z - radius - 0x100 >> PAP_SHIFT_LO;
        x2 = x + radius + 0x100 >> PAP_SHIFT_LO;
        z2 = z + radius + 0x100 >> PAP_SHIFT_LO;
        SATURATE(x1, 0, PAP_SIZE_LO - 1);
        SATURATE(x2, 0, PAP_SIZE_LO - 1);
        SATURATE(z1, 0, PAP_SIZE_LO - 1);
        SATURATE(z2, 0, PAP_SIZE_LO - 1);
    }

    for (mx = x1; mx <= x2; mx++)
        for (mz = z1; mz <= z2; mz++) {
            if (pass == 0) {
                if (PAP_hi[mx][mz].Flags & PAP_FLAG_ROOF_EXISTS) {
                    index = ROOF_HIDDEN_GET_FACE(mx, mz);
                } else
                    index = 0;
            } else
                index = PAP_2LO(mx, mz).Walkable;

            while (index && count++ < 100) {
                face = index;

                {
                    SLONG p;
                    SLONG c0;
                    SLONG p0, p1, p2, p3;
                    SLONG face_angle;
                    SLONG dangle;

                    PrimFace4* p_f4;
                    struct RoofFace4* rf;

                    if (face > 0 && prim_faces4[face].ThingIndex == ignore_building) {
                        // Skip faces from the building being ignored.
                    } else {
                        {
                            cable = 0;
                        }

                        if (face > 0) {
                            p_f4 = &prim_faces4[face];
                            for (c0 = 0; c0 < 4; c0++) {
                                p = p_f4->Points[c0];
                                grab_px[c0] = 0 + prim_points[p].X;
                                grab_py[c0] = 0 + prim_points[p].Y;
                                grab_pz[c0] = 0 + prim_points[p].Z;
                            }
                        } else {
                            if (IS_ROOF_HIDDEN_FACE(face)) {
                                rf = 0;
                                grab_px[0] = (mx) << 8;
                                grab_pz[0] = (mz) << 8;
                                grab_py[0] = grab_py[1] = grab_py[2] = grab_py[3] = MAVHEIGHT(mx, mz) << 6;
                            } else {
                                rf = &roof_faces4[-face];
                                grab_px[0] = (rf->RX & 127) << 8;
                                grab_pz[0] = (rf->RZ & 127) << 8;
                                grab_py[0] = rf->Y;
                                grab_py[1] = rf->Y + (rf->DY[0] << ROOF_SHIFT);
                                grab_py[2] = rf->Y + (rf->DY[2] << ROOF_SHIFT);
                                grab_py[3] = rf->Y + (rf->DY[1] << ROOF_SHIFT);
                            }

                            grab_px[1] = grab_px[0] + 256;
                            grab_pz[1] = grab_pz[0];
                            grab_px[2] = grab_px[0];
                            grab_pz[2] = grab_pz[0] + 256;
                            grab_px[3] = grab_px[0] + 256;
                            grab_pz[3] = grab_pz[0] + 256;
                        }

                        if (abs(grab_py[0] - y) < 256 + dy) {
                            MSG_add("***** grab face in in in in range abs(%d - %d) !< %d\n", grab_py[0], y, dy);

                            UBYTE point_order[4] = { 0, 1, 3, 2 };

                            for (i = 0; i < 4; i++) {
                                ULONG faceflag;
                                ULONG edgeflag;

                                if (pass == 0) {
                                    switch (i) {
                                    case 0:
                                        if (abs((MAVHEIGHT(mx, mz - 1) << 6) - grab_py[0]) < 0xa0)
                                            continue;
                                        break;
                                    case 1:
                                        if (abs((MAVHEIGHT(mx + 1, mz) << 6) - grab_py[0]) < 0xa0)
                                            continue;
                                        break;
                                    case 2:
                                        if (abs((MAVHEIGHT(mx, mz + 1) << 6) - grab_py[0]) < 0xa0)
                                            continue;
                                        break;
                                    case 3:
                                        if (abs((MAVHEIGHT(mx - 1, mz) << 6) - grab_py[0]) < 0xa0)
                                            continue;
                                        break;
                                    }
                                }

                                if (face < 0) {
                                    if (rf) {
                                        faceflag = rf->DrawFlags;
                                    }
                                    edgeflag = RFACE_FLAG_SLIDE_EDGE;
                                    edgeflag <<= i;
                                } else {
                                    faceflag = p_f4->FaceFlags;
                                    edgeflag = FACE_FLAG_SLIDE_EDGE;
                                    edgeflag <<= i;
                                }

                                MSG_add(" faceflag %x edgeflag %x \n", faceflag, edgeflag);

                                if ((faceflag & edgeflag) || IS_ROOF_HIDDEN_FACE(face)) {
                                    MSG_add(" useful edge %d \n", i);

                                    p0 = point_order[(i)];
                                    p1 = point_order[(i + 1) & 3];

                                    if (cable) {
                                        face_angle = angle + 512;
                                        face_angle &= 2047;
                                    } else {
                                        dx = grab_px[p1] - grab_px[p0];
                                        dz = grab_pz[p1] - grab_pz[p0];
                                        face_angle = calc_angle(dx, dz);
                                    }

                                    face_angle += 512;
                                    face_angle &= 2047;

                                    dangle = face_angle - angle;
                                    if (dangle < 1024) {
                                        dangle += 2048;
                                    }
                                    if (dangle > 1024) {
                                        dangle -= 2048;
                                    }

                                    MSG_add(" dangle %d <200? \n", dangle);

                                    if (abs(dangle) < 200 || cable) {
                                        dist = nearest_point_on_line_and_dist_calc_y(
                                            grab_px[p0], grab_py[p0], grab_pz[p0],
                                            grab_px[p1], grab_py[p1], grab_pz[p1],
                                            x, z,
                                            &near_x,
                                            &near_y,
                                            &near_z);

                                        if (dist < radius && abs(near_y - y) < dy) {
                                            if (face > 0) {
                                                if (PAP_calc_map_height_at(near_x, near_z) > grab_py[0] - 0x80) {
                                                    // Too near the ground to grab.
                                                } else {
                                                    {
                                                        SLONG fx1;
                                                        SLONG fz1;
                                                        SLONG fx2;
                                                        SLONG fz2;

                                                        fx1 = near_x;
                                                        fz1 = near_z;
                                                        fx2 = near_x;
                                                        fz2 = near_z;

                                                        if (abs(SIN(face_angle)) < abs(COS(face_angle))) {
                                                            fx1 += 256;
                                                            fz1 += 128;
                                                            fz2 += 128;
                                                        } else {
                                                            fz2 += 256;
                                                            fx1 += 128;
                                                            fx2 += 128;
                                                        }

                                                        fx1 &= ~0xff;
                                                        fz1 &= ~0xff;
                                                        fx2 &= ~0xff;
                                                        fz2 &= ~0xff;

                                                        if (!does_fence_lie_along_line(fx1, fz1, fx2, fz2)) {
                                                            *grab_x = near_x;
                                                            *grab_y = near_y;
                                                            *grab_z = near_z;
                                                            *grab_angle = face_angle;
                                                            return face;
                                                        }
                                                    }
                                                }
                                            } else {
                                                SLONG cx1 = near_x;
                                                SLONG cy1 = grab_py[p0];
                                                SLONG cz1 = near_z;
                                                SLONG cx2 = near_x;
                                                SLONG cy2 = grab_py[p0];
                                                SLONG cz2 = near_z;
                                                SLONG cx3 = near_x;
                                                SLONG cy3 = grab_py[p0];
                                                SLONG cz3 = near_z;
                                                SLONG cx4 = near_x;
                                                SLONG cy4 = grab_py[p0];
                                                SLONG cz4 = near_z;

                                                SLONG dx = SIGN((grab_px[p1] - grab_px[p0]) / 8);
                                                SLONG dz = SIGN((grab_pz[p1] - grab_pz[p0]) / 8);

                                                cx1 += (+dx) * CHECK_WIDTH + (-dz) * CHECK_FORWARD;
                                                cz1 += (+dz) * CHECK_WIDTH + (+dx) * CHECK_FORWARD;
                                                cx2 += (-dx) * CHECK_WIDTH + (-dz) * CHECK_FORWARD;
                                                cz2 += (-dz) * CHECK_WIDTH + (+dx) * CHECK_FORWARD;
                                                cx3 += (+dx) * CHECK_WIDTH - (-dz) * CHECK_FORWARD;
                                                cz3 += (+dz) * CHECK_WIDTH - (+dx) * CHECK_FORWARD;
                                                cx4 += (-dx) * CHECK_WIDTH - (-dz) * CHECK_FORWARD;
                                                cz4 += (-dz) * CHECK_WIDTH - (+dx) * CHECK_FORWARD;

                                                SLONG height1;
                                                SLONG height2;
                                                SLONG height3;
                                                SLONG height4;

                                                SLONG face1;
                                                SLONG face2;

                                                if (p_person->Genus.Person->Ware) {
                                                    height1 = WARE_calc_height_at(p_person->Genus.Person->Ware, cx1, cz1);
                                                    height2 = WARE_calc_height_at(p_person->Genus.Person->Ware, cx2, cz2);
                                                    height3 = WARE_calc_height_at(p_person->Genus.Person->Ware, cx3, cz3);
                                                    height4 = WARE_calc_height_at(p_person->Genus.Person->Ware, cx4, cz4);
                                                } else {
                                                    height1 = PAP_calc_map_height_at(cx1, cz1);
                                                    height2 = PAP_calc_map_height_at(cx2, cz2);
                                                    height3 = PAP_calc_map_height_at(cx3, cz3);
                                                    height4 = PAP_calc_map_height_at(cx4, cz4);
                                                }

                                                if (radius < 30 || radius > 90 || abs(cy1 - PAP_calc_map_height_at(x, z)) > 0xa0)
                                                    if (abs(height1 - cy1) < 64 && abs(height2 - cy2) < 64 && height3 <= height1 && height4 <= height2) {
                                                        SLONG dy = height2 - height1;

                                                        if (abs(dy) < 64) {
                                                            {
                                                                SLONG fx1;
                                                                SLONG fz1;
                                                                SLONG fx2;
                                                                SLONG fz2;

                                                                fx1 = near_x;
                                                                fz1 = near_z;
                                                                fx2 = near_x;
                                                                fz2 = near_z;

                                                                if (abs(SIN(face_angle)) < abs(COS(face_angle))) {
                                                                    fx1 += 256;
                                                                    fz1 += 128;
                                                                    fz2 += 128;
                                                                } else {
                                                                    fz2 += 256;
                                                                    fx1 += 128;
                                                                    fx2 += 128;
                                                                }

                                                                fx1 &= ~0xff;
                                                                fz1 &= ~0xff;
                                                                fx2 &= ~0xff;
                                                                fz2 &= ~0xff;

                                                                if (!does_fence_lie_along_line(fx1, fz1, fx2, fz2)) {
                                                                    *grab_x = near_x;
                                                                    *grab_y = grab_py[p0];
                                                                    *grab_z = near_z;
                                                                    *grab_angle = face_angle;
                                                                    MSG_add(" YES YES YES grab Face dist %d  radius %d y %d\n", dist, radius, grab_py[p0]);
                                                                    return face;
                                                                }
                                                            }
                                                        } else {
                                                            MSG_add("TOO NEAR A WALL (%d) !!!!!!", dy);
                                                        }
                                                    } else {
                                                        MSG_add("Couldn't find faces.");
                                                    }
                                            }
                                        } else
                                            MSG_add(" dist %d to far %d \n", dist, radius);
                                    }
                                }
                            }
                        } else
                            MSG_add(" grab face out range abs(%d - %d) !< %d\n", grab_py[0], y, dy);
                    }
                }

                if (pass == 0) {
                    index = 0;
                } else if (index < 0) {
                    index = roof_faces4[-index].Next;
                } else {
                    index = prim_faces4[index].WALKABLE;
                }
            }
        }
    pass++;
    if (pass == 1)
        goto round_again;

    return (0);
}

// Simplified grab search for sewer trench areas.
// Checks 4 adjacent hi-res map squares for NS_Hi trench edges within radius.
// uc_orig: find_grab_face_in_sewers (fallen/Source/interact.cpp)
SLONG find_grab_face_in_sewers(
    SLONG x, SLONG y, SLONG z,
    SLONG radius, SLONG dy, SLONG angle,
    SLONG* grab_x, SLONG* grab_y, SLONG* grab_z,
    SLONG* grab_angle)
{
    SLONG i;
    SLONG mx;
    SLONG mz;
    SLONG dx;
    SLONG dz;
    SLONG floor_height;

    SLONG best_dist = radius;
    SLONG best_x;
    SLONG best_y;
    SLONG best_z;
    SLONG best_angle;

    SLONG dist;
    SLONG dangle;

    NS_Hi* nh;

    const struct {
        SBYTE dx;
        SBYTE dz;
    } order[4] = {
        { +1, 0 },
        { -1, 0 },
        { 0, +1 },
        { 0, -1 }
    };

    for (i = 0; i < 4; i++) {
        dx = order[i].dx;
        dz = order[i].dz;

        mx = (x >> PAP_SHIFT_HI) + dx;
        mz = (z >> PAP_SHIFT_HI) + dz;

        if (PAP_on_map_hi(mx, mz)) {
            nh = &NS_hi[mx][mz];

            if (NS_HI_TYPE(nh) != NS_HI_TYPE_ROCK && NS_HI_TYPE(nh) != NS_HI_TYPE_CURVE && NS_HI_TYPE(nh) != NS_HI_TYPE_NOTHING) {
                floor_height = (nh->bot << 5) + (-0x100 * 32);

                if (abs(floor_height - y) < dy) {
                    switch (i) {
                    case 0:
                        dist = 255 - (x & 0xff);
                        break;
                    case 1:
                        dist = (x & 0xff);
                        break;
                    case 2:
                        dist = 255 - (z & 0xff);
                        break;
                    case 3:
                        dist = (z & 0xff);
                        break;
                    default:
                        ASSERT(0);
                        break;
                    }

                    if (dist < best_dist) {
                        switch (i) {
                        case 0:
                            best_x = (x + 256) & 0xffffff00;
                            best_z = z;
                            best_angle = 1536;
                            break;
                        case 1:
                            best_x = x & 0xffffff00;
                            best_z = z;
                            best_angle = 512;
                            break;
                        case 2:
                            best_z = (z + 256) & 0xffffff00;
                            best_x = x;
                            best_angle = 1024;
                            break;
                        case 3:
                            best_z = z & 0xffffff00;
                            best_x = x;
                            best_angle = 0;
                            break;
                        default:
                            ASSERT(0);
                            break;
                        }

                        dangle = best_angle - angle;
                        if (dangle > 1024) {
                            dangle -= 2048;
                        }
                        if (dangle < -1024) {
                            dangle += 2048;
                        }

                        if (abs(dangle) < 200) {
                            best_dist = dist;
                            best_y = floor_height;
                        }
                    }
                }
            }
        }
    }

    if (best_dist < radius) {
        *grab_x = best_x;
        *grab_y = best_y;
        *grab_z = best_z;
        *grab_angle = best_angle;
        return GRAB_SEWERS;
    }

    return 0;
}

// Computes world-space position of a bone attachment point for a Thing using current animation tween.
// Uses FMATRIX_calc to account for roll; applies character scale if non-default.
// uc_orig: calc_sub_objects_position (fallen/Source/interact.cpp)
void calc_sub_objects_position(Thing* p_mthing, SLONG tween, UWORD object, SLONG* x, SLONG* y, SLONG* z)
{
    struct SVector temp;
    struct GameKeyFrameElement* anim_info;
    struct GameKeyFrameElement* anim_info_next;
    struct Matrix33* rot_mat;
    SLONG wx, wy, wz;
    DrawTween* dt = p_mthing->Draw.Tweened;

    wx = 0;
    wy = 0;
    wz = 0;

    if (object == SUB_OBJECT_PREFERRED_HAND) {
        object = SUB_OBJECT_LEFT_HAND;
    }

    if (p_mthing->Draw.Tweened->CurrentFrame && p_mthing->Draw.Tweened->NextFrame) {
        anim_info = &p_mthing->Draw.Tweened->CurrentFrame->FirstElement[object];
        anim_info_next = &p_mthing->Draw.Tweened->NextFrame->FirstElement[object];

        offset.M[0] = ((anim_info->OffsetX + (((anim_info_next->OffsetX - anim_info->OffsetX) * tween) >> 8)) >> TWEEN_OFFSET_SHIFT) + wx;
        offset.M[1] = ((anim_info->OffsetY + (((anim_info_next->OffsetY - anim_info->OffsetY) * tween) >> 8)) >> TWEEN_OFFSET_SHIFT) + wy;
        offset.M[2] = ((anim_info->OffsetZ + (((anim_info_next->OffsetZ - anim_info->OffsetZ) * tween) >> 8)) >> TWEEN_OFFSET_SHIFT) + wz;

        {
            FMATRIX_calc(
                matrix,
                p_mthing->Draw.Tweened->Angle,
                p_mthing->Draw.Tweened->Tilt,
                p_mthing->Draw.Tweened->Roll);

            if (p_mthing->Class == CLASS_PERSON) {
                SLONG character_scale = person_get_scale(p_mthing);

                if (character_scale != 256) {
                    matrix[0] = (matrix[0] * character_scale) >> 8;
                    matrix[1] = (matrix[1] * character_scale) >> 8;
                    matrix[2] = (matrix[2] * character_scale) >> 8;
                    matrix[3] = (matrix[3] * character_scale) >> 8;
                    matrix[4] = (matrix[4] * character_scale) >> 8;
                    matrix[5] = (matrix[5] * character_scale) >> 8;
                    matrix[6] = (matrix[6] * character_scale) >> 8;
                    matrix[7] = (matrix[7] * character_scale) >> 8;
                    matrix[8] = (matrix[8] * character_scale) >> 8;
                }
            }

            FMATRIX_MUL_BY_TRANSPOSE(
                matrix,
                offset.M[0],
                offset.M[1],
                offset.M[2]);

            *x = offset.M[0];
            *y = offset.M[1];
            *z = offset.M[2];

            if (object == SUB_OBJECT_LEFT_FOOT) {
                MSG_add(" tween %d y %d y-foot_height %d \n", tween, *y, *y - FOOT_HEIGHT);
            }
        }
    } else {
        *x = 0;
        *y = 0;
        *z = 0;
    }
    if (object == SUB_OBJECT_LEFT_FOOT || object == SUB_OBJECT_RIGHT_FOOT)
        *y -= 10;
    if (object == SUB_OBJECT_LEFT_HAND || object == SUB_OBJECT_RIGHT_HAND)
        *y += HAND_HEIGHT;
}

// calc_sub_objects_position variant with fixed-point 8-bit precision output (x256 scale).
// Uses older rotate_obj instead of FMATRIX_calc.
// uc_orig: calc_sub_objects_position_fix8 (fallen/Source/interact.cpp)
void calc_sub_objects_position_fix8(Thing* p_mthing, SLONG tween, UWORD object, SLONG* x, SLONG* y, SLONG* z)
{
    struct SVector temp;
    struct Matrix33 r_matrix;
    struct Matrix31 offset;
    struct GameKeyFrameElement* anim_info;
    struct GameKeyFrameElement* anim_info_next;
    struct Matrix33* rot_mat;
    SLONG wx, wy, wz;

    DrawTween* dt = p_mthing->Draw.Tweened;

    wx = 0;
    wy = 0;
    wz = 0;

    if (p_mthing->Draw.Tweened->CurrentFrame && p_mthing->Draw.Tweened->NextFrame) {
        anim_info = &p_mthing->Draw.Tweened->CurrentFrame->FirstElement[object];
        anim_info_next = &p_mthing->Draw.Tweened->NextFrame->FirstElement[object];

        offset.M[0] = ((anim_info->OffsetX << 8) + (((anim_info_next->OffsetX - anim_info->OffsetX) * tween) >> 0)) >> (TWEEN_OFFSET_SHIFT + 3) + wx;
        offset.M[1] = ((anim_info->OffsetY << 8) + (((anim_info_next->OffsetY - anim_info->OffsetY) * tween) >> 0)) >> (TWEEN_OFFSET_SHIFT + 3) + wy;
        offset.M[2] = ((anim_info->OffsetZ << 8) + (((anim_info_next->OffsetZ - anim_info->OffsetZ) * tween) >> 0)) >> (TWEEN_OFFSET_SHIFT + 3) + wz;

        rotate_obj(
            p_mthing->Draw.Tweened->Tilt,
            p_mthing->Draw.Tweened->Angle,
            p_mthing->Draw.Tweened->Roll,
            &r_matrix);

        matrix_transformZMY((struct Matrix31*)&temp, &r_matrix, &offset);
        *x = (temp.X << 3);
        *y = (temp.Y << 3);
        *z = (temp.Z << 3);
    } else {
        *x = 0;
        *y = 0;
        *z = 0;
    }
    if (object == SUB_OBJECT_LEFT_FOOT || object == SUB_OBJECT_RIGHT_FOOT)
        *y -= FOOT_HEIGHT << 8;
    if (object == SUB_OBJECT_LEFT_HAND || object == SUB_OBJECT_RIGHT_HAND)
        *y += HAND_HEIGHT;
}

// calc_sub_objects_position variant that takes explicit frame pointers instead of reading from Thing.
// uc_orig: calc_sub_objects_position_keys (fallen/Source/interact.cpp)
void calc_sub_objects_position_keys(Thing* p_mthing, SLONG tween, UWORD object, SLONG* x, SLONG* y, SLONG* z, struct GameKeyFrame* frame1, struct GameKeyFrame* frame2)
{
    struct SVector temp;
    struct Matrix33 r_matrix;
    struct Matrix31 offset;
    struct GameKeyFrameElement* anim_info;
    struct GameKeyFrameElement* anim_info_next;
    struct Matrix33* rot_mat;
    SLONG wx, wy, wz;

    DrawTween* dt = p_mthing->Draw.Tweened;

    wx = 0;
    wy = 0;
    wz = 0;

    if (p_mthing->Draw.Tweened->CurrentFrame && p_mthing->Draw.Tweened->NextFrame) {
        anim_info = &frame1->FirstElement[object];
        anim_info_next = &frame2->FirstElement[object];

        offset.M[0] = ((anim_info->OffsetX + (((anim_info_next->OffsetX - anim_info->OffsetX) * tween) >> 8)) >> TWEEN_OFFSET_SHIFT) + wx;
        offset.M[1] = ((anim_info->OffsetY + (((anim_info_next->OffsetY - anim_info->OffsetY) * tween) >> 8)) >> TWEEN_OFFSET_SHIFT) + wy;
        offset.M[2] = ((anim_info->OffsetZ + (((anim_info_next->OffsetZ - anim_info->OffsetZ) * tween) >> 8)) >> TWEEN_OFFSET_SHIFT) + wz;

        rotate_obj(
            p_mthing->Draw.Tweened->Tilt,
            p_mthing->Draw.Tweened->Angle,
            p_mthing->Draw.Tweened->Roll,
            &r_matrix);

        matrix_transformZMY((struct Matrix31*)&temp, &r_matrix, &offset);
        *x = temp.X;
        *y = temp.Y;
        *z = temp.Z;
    } else {
        *x = 0;
        *y = 0;
        *z = 0;
    }
    if (object == SUB_OBJECT_LEFT_FOOT || object == SUB_OBJECT_RIGHT_FOOT)
        *y -= FOOT_HEIGHT;
    if (object == SUB_OBJECT_LEFT_HAND || object == SUB_OBJECT_RIGHT_HAND)
        *y += HAND_HEIGHT;
}

// calc_sub_objects_position variant that does not reference a Thing — takes two raw keyframe pointers.
// uc_orig: calc_sub_objects_position_global (fallen/Source/interact.cpp)
void calc_sub_objects_position_global(GameKeyFrame* cur_frame, GameKeyFrame* next_frame, SLONG tween, UWORD object, SLONG* x, SLONG* y, SLONG* z)
{
    struct Matrix31 offset;
    struct GameKeyFrameElement* anim_info;
    struct GameKeyFrameElement* anim_info_next;
    SLONG wx, wy, wz;

    wx = 0;
    wy = 0;
    wz = 0;

    if (cur_frame && next_frame) {
        anim_info = &cur_frame->FirstElement[object];
        anim_info_next = &next_frame->FirstElement[object];

        *x = (anim_info->OffsetX + (((anim_info_next->OffsetX - anim_info->OffsetX) * tween) >> 8)) >> TWEEN_OFFSET_SHIFT;
        *y = (anim_info->OffsetY + (((anim_info_next->OffsetY - anim_info->OffsetY) * tween) >> 8)) >> TWEEN_OFFSET_SHIFT;
        *z = (anim_info->OffsetZ + (((anim_info_next->OffsetZ - anim_info->OffsetZ) * tween) >> 8)) >> TWEEN_OFFSET_SHIFT;

        *x += wx;
        *y += wy;
        *z += wz;
    } else {
        *x = 0;
        *y = 0;
        *z = 0;
    }
    if (object == SUB_OBJECT_LEFT_FOOT || object == SUB_OBJECT_RIGHT_FOOT)
        *y -= FOOT_HEIGHT;
    if (object == SUB_OBJECT_LEFT_HAND || object == SUB_OBJECT_RIGHT_HAND)
        *y += HAND_HEIGHT;
}
