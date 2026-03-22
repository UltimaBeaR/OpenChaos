// Chunk 6+ of Building.cpp — chunks 1–5 migrated to new/world/environment/building.cpp.
// This file contains the remaining functions (original lines ~7050+).
// Globals and chunks 1–5 functions now live in new/world/environment/building*.

#include "game.h"
#include "shadow.h"
#include "..\headers\animtmap.h"
#include "pap.h"
#include "supermap.h"
#include "io.h"
#include "memory.h"
#include "..\headers\edmap.h"
#include "..\headers\outline.h"

#include "world/environment/building.h"
#include "world/environment/building_globals.h"

#pragma warning(disable : 4244)

// Forward declarations for functions still defined later in this file (chunk 6+).
void calc_building_normals(void);

// Chunks 1–5 migrated: all functions through get_wall_start_and_end now in new/
// -- append_recessed_wall_prim, append_foundation_wall, append_wall_prim,
//    find_near_prim_point, create_recessed_storey_points, scan_triangle,
//    flag_floor_tiles_for_quad, flag_floor_tiles_for_tri, build_roof, area_of_quad,
//    create_split_quad_into_4, create_split_quad_into_16, create_split_quad_into_48,
//    build_roof_quad, center_object, center_object_about_xz, build_facet,
//    build_building, build_building2, build_prim_object, find_next_last_coord,
//    LedgeInfo, build_single_ledge, dist_between_vertex_and_vector, find_wall_for_fe,
//    build_staircase, get_wall_start_and_end

// Shared constant needed by insert_recessed_wall_vect and other chunk 6+ functions.
#define RECESS_SIZE (32)
//		THING_INDEX    cable_thing,
void make_cable_taut_along(SLONG along, SLONG building, SLONG* x_middle, SLONG* y_middle, SLONG* z_middle)
{
    SLONG i;

    SLONG x;
    SLONG y;
    SLONG z;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG h;

    SLONG V;
    SLONG v1;
    SLONG v2;

    SLONG L;
    SLONG l1;
    SLONG l2;

    SLONG x1;
    SLONG y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG xm;
    SLONG ym;
    SLONG zm;

    SLONG middle;

    SLONG num_points;

    SLONG pstart;
    SLONG pend;

    SLONG p1;
    SLONG p2;

    BuildingFacet* bf;

    //	Thing *p_thing = TO_THING(cable_thing);

    //
    // Make sure this is a building.
    //

    //	ASSERT(p_thing->Class == CLASS_BUILDING);

    //
    // It should only have one facet.
    //

    bf = &building_facets[building_objects[building].FacetHead];

    //
    // Make sure that this facet is a cable.
    //

    if (!(bf->FacetFlags & FACET_FLAG_CABLE))
        return;

    pstart = bf->StartPoint;
    pend = bf->EndPoint - 2;

    x1 = prim_points[pstart].X;
    y1 = prim_points[pstart].Y;
    z1 = prim_points[pstart].Z;

    x2 = prim_points[pend].X;
    y2 = prim_points[pend].Y;
    z2 = prim_points[pend].Z;

    //
    // Work out the length of the cable, L.
    //

    L = 0;

    for (i = bf->StartPoint; i + 2 < bf->EndPoint; i += 2) {
        p1 = i;
        p2 = i + 2;

        dx = abs(prim_points[p2].X - prim_points[p1].X);
        dy = abs(prim_points[p2].Y - prim_points[p1].Y);
        dz = abs(prim_points[p2].Z - prim_points[p1].Z);

        L += QDIST3(dx, dy, dz);
    }
    L -= L >> 2;

    ASSERT(L > 0);

    //
    // Straight line distance between the ends of the cable, V.
    //

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    dz = abs(z2 - z1);

    V = QDIST3(dx, dy, dz);

    L = V + 50;

    //
    // Work out v1 and v2, the lengths along the line broken up
    // by where you are hanging.
    //

    v1 = V * along >> 8;
    v2 = V - v1;

    //
    // Find l1, the length of the cable upto where you are
    // hanging.
    //

    l1 = (L * L + v1 * v1 - v2 * v2) / (2 * L);

    //
    // Now work out h, the distance to hang down by.
    //

    h = Root(l1 * l1 - v1 * v1);

    //
    // We have to move the cable points so they form a triangle shape.
    //

    num_points = bf->EndPoint - bf->StartPoint >> 1;

    ASSERT(num_points >= 3);

    //
    // The bottom of the triangle shape.
    //

    xm = x1 + ((x2 - x1) * along >> 8);
    ym = y1 + ((y2 - y1) * along >> 8);
    zm = z1 + ((z2 - z1) * along >> 8);

    ym -= h;

    //
    // The point to use as the middle point.
    //

    middle = num_points * along >> 8;

    if (middle == 0) {
        middle = 1;
    }

    if (middle == num_points - 1) {
        middle -= 1;
    }

    dx = (xm - x1) / middle;
    dy = (ym - y1) / middle;
    dz = (zm - z1) / middle;

    x = x1;
    y = y1;
    z = z1;

    for (i = 0; i < num_points; i++) {
        if (i == middle) {
            //
            // Recalculate (dx,dy,dz)
            //

            dx = (x2 - xm) / (num_points - middle - 1);
            dy = (y2 - ym) / (num_points - middle - 1);
            dz = (z2 - zm) / (num_points - middle - 1);

            x = xm;
            y = ym;
            z = zm;
        }

        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 0].X = x;
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 0].Y = y;
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 0].Z = z;

        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 1].X = x;
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 1].Y = y + 8;
        AENG_dx_prim_points[bf->StartPoint + (i * 2) + 1].Z = z;

        x += dx;
        y += dy;
        z += dz;
    }

    *x_middle = xm;
    *y_middle = ym;
    *z_middle = zm;
}

void make_cable_flabby(SLONG building)
{
    SLONG i;

    BuildingFacet* bf = &building_facets[building_objects[building].FacetHead];

    //
    // Easy!
    //

    for (i = bf->StartPoint; i < bf->EndPoint; i++) {
        AENG_dx_prim_points[i].X = float(prim_points[i].X);
        AENG_dx_prim_points[i].Y = float(prim_points[i].Y);
        AENG_dx_prim_points[i].Z = float(prim_points[i].Z);
    }
}

#define LIGHT_SIZE BLOCK_SIZE
#define CONE_MULT 5
SLONG create_suspended_light(SLONG x, SLONG y, SLONG z, SLONG flags)
{
    SLONG p1, p2;
    struct PrimFace3* p_f3;

    flags = flags;
    p1 = next_prim_point;
    add_point(x, y, z);
    add_point(x - LIGHT_SIZE, y - LIGHT_SIZE, z - LIGHT_SIZE);
    add_point(x + LIGHT_SIZE, y - LIGHT_SIZE, z - LIGHT_SIZE);
    add_point(x + LIGHT_SIZE, y - LIGHT_SIZE, z + LIGHT_SIZE);
    add_point(x - LIGHT_SIZE, y - LIGHT_SIZE, z + LIGHT_SIZE);

    /*
            p2=next_prim_point-1;
            add_point(x-LIGHT_SIZE*CONE_MULT,y-LIGHT_SIZE*6,z-LIGHT_SIZE*CONE_MULT);
            add_point(x+LIGHT_SIZE*CONE_MULT,y-LIGHT_SIZE*6,z-LIGHT_SIZE*CONE_MULT);
            add_point(x+LIGHT_SIZE*CONE_MULT,y-LIGHT_SIZE*6,z+LIGHT_SIZE*CONE_MULT);
            add_point(x-LIGHT_SIZE*CONE_MULT,y-LIGHT_SIZE*6,z+LIGHT_SIZE*CONE_MULT);
    */

    p_f3 = create_a_tri(p1 + 2, p1 + 1, p1 + 0, 0, 0);
    p_f3->DrawFlags = 0; // POLY_50T;
    p_f3 = create_a_tri(p1 + 3, p1 + 2, p1 + 0, 0, 0);
    p_f3->DrawFlags = 0; // POLY_50T;
    p_f3 = create_a_tri(p1 + 4, p1 + 3, p1 + 0, 0, 0);
    p_f3->DrawFlags = 0; // POLY_50T;
    p_f3 = create_a_tri(p1 + 1, p1 + 4, p1 + 0, 0, 0);
    p_f3->DrawFlags = 0; // POLY_50T;
    /*
            p_f3=create_a_tri(p2+2,p2+1,p1+0,0,28);
            p_f3->DrawFlags=POLY_50T;
            p_f3=create_a_tri(p2+3,p2+2,p1+0,0,28);
            p_f3->DrawFlags=POLY_50T;
            p_f3=create_a_tri(p2+4,p2+3,p1+0,0,28);
            p_f3->DrawFlags=POLY_50T;
            p_f3=create_a_tri(p2+1,p2+4,p1+0,0,28);
            p_f3->DrawFlags=POLY_50T;
    */
    // conv	apply_light_to_map(x,50,z,150);

    return (0);
}

void build_cable(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SWORD wall, SWORD type, SLONG saggysize)
{
    SLONG p1;
    UWORD start_point;
    UWORD start_face3, start_face4;
    struct PrimFace4* p_f4;
    SLONG prim;
    SLONG len, dx, dy, dz, count;
    SLONG px, py, pz;
    SLONG c0;
    SLONG light_x, light_y, light_z;
    SLONG step_angle1, step_angle2, angle;

    wall = wall;
    type = type;
    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    dz = abs(z2 - z1);

    len = QDIST3(dx, dy, dz);
    count = (len << 1) / ELE_SIZE;
    dx = (x2 - x1);
    dy = (y2 - y1);
    dz = (z2 - z1);

    px = x1;
    py = y1;
    pz = z1;
    add_point(px, py, pz);
    add_point(px, py + 8, pz);

    if (dy == 0) {
        step_angle1 = 1024 / count;
        step_angle2 = -step_angle1;
    } else {
        SLONG c1, c2;
        SLONG m;
        SLONG d1, d2;
        if (len == 0)
            len = 1;
        m = (abs(dy) * 190) / len;

        c1 = 128; //== 0.5   along
        c2 = 128; //=0.5

        if (dy < 0) {

            //
            // its going down hill so slow stepangle1 and fast step angle2
            //
            c1 = c1 + m;
            c2 = c2 - m;

        } else {
            c1 = c1 - m;
            c2 = c2 + m;
        }
        if (c1 < 0)
            c1 = 0;
        if (c1 > 256)
            c1 = 256;
        if (c2 < 0)
            c2 = 0;
        if (c2 > 256)
            c2 = 256;

        d1 = ((count * c1) >> 8);
        d2 = ((count * c2) >> 8);

        if (d1 == 0)
            d1 = 1;

        if (d2 == 0)
            d2 = 1;

        step_angle1 = 512 / d1;
        step_angle2 = -512 / d2;
    }
    angle = -512;
    for (c0 = 1; c0 <= count; c0++) {
        SLONG ex, ey, ez;

        angle += step_angle1;
        if (angle >= -30) {
            step_angle1 = step_angle2;
        }

        ex = x1 + (c0 * dx) / count;
        ey = y1 + (c0 * dy) / count;

        //		angle=((c0-(count>>1))*1024)/count;
        //		angle=(angle+2048)&2047;
        ey -= (COS((angle + 2048) & 2047) * saggysize) >> 16;
        ez = z1 + (c0 * dz) / count;
        if (c0 == (count >> 1)) {
            light_x = ex;
            light_y = ey;
            light_z = ez;
        }
        p1 = next_prim_point;
        add_point(ex, ey, ez);
        add_point(ex, ey + 8, ez);
        p_f4 = create_a_quad(p1 - 1, p1 + 1, p1 - 2, p1, 0, 0);
        p_f4->DrawFlags = POLY_FLAG_DOUBLESIDED;
        p_f4->Type = FACE_TYPE_CABLE;
        p_f4->ThingIndex = -wall;
        add_quad_to_walkable_list(next_prim_face4 - 1);
        p_f4->FaceFlags &= ~FACE_FLAG_WALKABLE;

        px = ex;
        py = ey;
        pz = ez;
    }
}

void build_cable_old(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SWORD wall, SWORD type)
{
    SLONG p1;
    UWORD start_point;
    UWORD start_face3, start_face4;
    struct PrimFace4* p_f4;
    SLONG prim;
    SLONG len, dx, dy, dz, count;
    SLONG px, py, pz;
    SLONG c0;
    SLONG light_x, light_y, light_z;

    wall = wall;
    type = type;
    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    dx = abs(x2 - x1);
    dy = abs(y2 - y1);
    dz = abs(z2 - z1);

    len = QDIST3(dx, dy, dz);
    count = (len << 1) / ELE_SIZE;
    dx = (x2 - x1);
    dy = (y2 - y1);
    dz = (z2 - z1);

    px = x1;
    py = y1;
    pz = z1;
    add_point(px, py, pz);
    add_point(px, py + 8, pz);
    //	LogText(" cable len %d count %d dx %d dy %d dz %d \n",len,count,dx,dy,dz);

    for (c0 = 1; c0 <= count; c0++) {
        SLONG ex, ey, ez;
        SLONG angle;

        ex = x1 + (c0 * dx) / count;
        ey = y1 + (c0 * dy) / count;

        angle = ((c0 - (count >> 1)) * 1024) / count;
        angle = (angle + 2048) & 2047;
        ey -= COS(angle) >> 9;
        ez = z1 + (c0 * dz) / count;
        if (c0 == (count >> 1)) {
            light_x = ex;
            light_y = ey;
            light_z = ez;
        }
        p1 = next_prim_point;
        add_point(ex, ey, ez);
        add_point(ex, ey + 8, ez);
        p_f4 = create_a_quad(p1 - 1, p1 + 1, p1 - 2, p1, 0, 0);
        p_f4->DrawFlags = POLY_FLAG_DOUBLESIDED;
        p_f4->Type = FACE_TYPE_CABLE;
        p_f4->ThingIndex = -wall;
        add_quad_to_walkable_list(next_prim_face4 - 1);

        px = ex;
        py = ey;
        pz = ez;
    }

    //	if(px&1)
    //		create_suspended_light(light_x,light_y,light_z,1);

    /*
            p1=next_prim_point;
            add_point(x1,y1,z1);
            add_point(x2,y2,z2);
            add_point(x1,y1-10,z1);
            add_point(x2,y2-10,z2);

            p_f4=create_a_quad(p1+2,p1+3,p1,p1+1,0,0);

            p_f4->DrawFlags|= POLY_FLAG_DOUBLESIDED;
    */
    // prim=build_prim_object(start_point,start_face3,start_face4);

    // place_prim_at(prim,build_x,0,build_z);
}

SLONG build_cables(SWORD storey, SLONG prev_facet)
{
    SLONG wall;
    SLONG x1, y1, z1, x2, y2, z2;
    SLONG start_point, start_face3, start_face4;
    SLONG prim;
    SLONG building;

    wall = storey_list[storey].WallHead;
    x1 = storey_list[storey].DX;
    y1 = storey_list[storey].DY;

    //	y1=BLOCK_SIZE*8;
    z1 = storey_list[storey].DZ;
    y1 += PAP_calc_height_at(x1, z1);
    //	if(y1==0)
    //		y1=BLOCK_SIZE*8;
    while (wall) {

        x2 = wall_list[wall].DX;
        y2 = wall_list[wall].DY;
        z2 = wall_list[wall].DZ;
        y2 += PAP_calc_height_at(x2, z2);

        start_point = next_prim_point;
        start_face3 = next_prim_face3;
        start_face4 = next_prim_face4;

        build_cable(x1, y1, z1, x2, y2, z2, wall, 0, wall_list[wall].TextureStyle2 * 64);
        prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, 0, FACET_FLAG_CABLE, 0);

        //		prim=build_building(start_point,start_face3,start_face4,prev_facet);
        prim = build_building2(start_point, start_face3, start_face4, prev_facet, storey_list[storey].DX, storey_list[storey].DZ);
        building = storey_list[storey].BuildingHead;

        /*

        //
        // Buildings dont have an (x,y,z) anymore... they have a thing index instead.
        //

        building_list[building].X=build_x;
        building_list[building].Y=0; //y1;
        building_list[building].Z=build_z;

        */

        storey_list[storey].Info1 = prim;
        //		LogText(" cable wall %d storey %d building %d xz %d %d \n",wall,storey,building,build_x,build_z);
        THING_INDEX new_thing = place_building_at(building, prim, build_x, 0, build_z);
        insert_collision_vect(x1, y1 + 5000, z1, x2, y2 + 5000, z2, STOREY_TYPE_CABLE, 1, new_thing);

        if (new_thing) {
            TO_THING(new_thing)->Flags |= FLAGS_CABLE_BUILDING;
        }

        wall = wall_list[wall].Next;

        x1 = x2;
        y1 = y2;
        z1 = z2;
    }
    return (0);
}

void build_fence_points_and_faces(SLONG y1, SLONG y2, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG wall, UBYTE posts)
{
    SLONG wcount, wwidth, dx, dz, dist;
    SLONG start_point;
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;
    SLONG px, pz;
    SLONG ya, yb;

    ya = PAP_calc_height_at(x1, z1);
    yb = PAP_calc_height_at(x2, z2);

    insert_collision_vect(x1, ya, z1, x2, yb, z2, STOREY_TYPE_FENCE, 4, -wall);
    insert_collision_vect(x2, yb, z2, x1, ya, z1, STOREY_TYPE_FENCE, 4, -wall);

    texture_style = wall_list[wall].TextureStyle;
    texture = TEXTURE_PIECE_MIDDLE;

    start_point = next_prim_point;

    dx = abs(x2 - x1);
    dz = abs(z2 - z1);

    dist = Root(SDIST2(dx, dz));
    if (dist == 0)
        return;

    wcount = (dist / (BLOCK_SIZE * 4));
    if (wcount == 0)
        wcount = 1;
    wwidth = dist / (wcount);

    dx = (x2 - x1);
    dz = (z2 - z1);

    dx = (dx << 10) / dist;
    dz = (dz << 10) / dist;

    px = ((dz * (10)) >> 10);
    pz = -((dx * (10)) >> 10);

    while (wcount) {
        SLONG p, p1, p2;
        SLONG floor_1, floor_2;
        p = next_prim_point;

        floor_1 = PAP_calc_height_at(x1, z1);

        add_point(x1, y2 + floor_1, z1);
        add_point(x1, y1 + floor_1, z1);

        add_point(x1 + px * 5, y2 + 45 + floor_1, z1 + pz * 5);
        add_point(x1 + px * 1, y2 + 10 + floor_1, z1 + pz * 1);

        x1 = x1 + ((dx * (wwidth - 20)) >> 10);
        z1 = z1 + ((dz * (wwidth - 20)) >> 10);
        floor_2 = PAP_calc_height_at(x1, z1);

        p1 = next_prim_point;

        add_point(x1, y2 + floor_2, z1);
        add_point(x1, y1 + floor_2, z1);

        add_point(x1 + px * 4, y2 + 45 + floor_2, z1 + pz * 4);
        add_point(x1 + px * 1, y2 + 10 + floor_2, z1 + pz * 1);

        p_f4 = create_a_quad(p, p1 + 0, p + 1, p1 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);

        p_f4 = create_a_quad(p + 2, p1 + 2, p + 3, p1 + 3, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);

        p = next_prim_point;
        add_point(x1 + px, y2 + floor_2, z1 + pz);
        add_point(x1 + px, y1 + floor_2, z1 + pz);
        add_point(x1 - px, y2 + floor_2, z1 - pz);
        add_point(x1 - px, y1 + floor_2, z1 - pz);

        add_point(x1 + px * 6, y2 + 50 + floor_2, z1 + pz * 6);
        add_point(x1 + px * 4, y2 + 50 + floor_2, z1 + pz * 4);

        x1 = x1 + ((dx * (20)) >> 10);
        z1 = z1 + ((dz * (20)) >> 10);

        p1 = next_prim_point;
        add_point(x1 + px, y2 + floor_2, z1 + pz);
        add_point(x1 + px, y1 + floor_2, z1 + pz);
        add_point(x1 - px, y2 + floor_2, z1 - pz);
        add_point(x1 - px, y1 + floor_2, z1 - pz);

        add_point(x1 + px * 6, y2 + 50 + floor_2, z1 + pz * 6);
        add_point(x1 + px * 4, y2 + 50 + floor_2, z1 + pz * 4);

        p_f4 = create_a_quad(p, p1, p + 1, p1 + 1, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(p1 + 2, p + 2, p1 + 3, p + 3, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(p + 2, p + 0, p + 3, p + 1, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(p1, p1 + 2, p1 + 1, p1 + 3, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(p + 4, p1 + 4, p, p1, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(p1 + 5, p + 5, p1 + 2, p + 2, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(p1 + 4, p1 + 5, p1, p1 + 2, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(p + 5, p + 4, p + 2, p, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(p + 4, p + 5, p1 + 4, p1 + 5, texture_style, TEXTURE_PIECE_LEFT);
        p_f4->ThingIndex = -wall;
        //		p_f4=create_a_quad(p+0,p+2,p1,p1+2,texture_style,TEXTURE_PIECE_LEFT);
        //		p_f4->ThingIndex=-wall;

        wcount--;
    }
}

void build_high_chain_fence(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height, UBYTE alt_mode)
{
    SLONG c0;
    SLONG sp[10];
    //	SLONG	sf4[10];
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;

    if (alt_mode == 1) {
        sp[0] = build_row_wall_only_points_at_floor_alt(height * 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[1] = build_row_wall_only_points_at_floor_alt(height, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[2] = build_row_wall_only_points_at_floor_alt(0, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
    } else {
        sp[0] = build_row_wall_only_points_at_y(y + height * 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[1] = build_row_wall_only_points_at_y(y + height, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[2] = build_row_wall_only_points_at_y(y, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
    }

    for (c0 = 0; c0 < WindowCount; c0++) {
        texture = TEXTURE_PIECE_MIDDLE;
        if (c0 == 0) {
            texture = TEXTURE_PIECE_RIGHT;
        } else if (c0 == WindowCount - 1) {
            texture = TEXTURE_PIECE_LEFT;
        } else {
            texture = TEXTURE_PIECE_MIDDLE;
        }

        p_f4 = create_a_quad(sp[0] + c0, sp[0] + c0 + 1, sp[1] + c0, sp[1] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);
        p_f4->Type = FACE_TYPE_FENCE;

        p_f4 = create_a_quad(sp[1] + c0, sp[1] + c0 + 1, sp[2] + c0, sp[2] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);
        p_f4->Type = FACE_TYPE_FENCE;
    }
    {
        SLONG ya, yb;
        ya = PAP_calc_height_at(x, z);
        yb = PAP_calc_height_at(wall_list[wall].DX, wall_list[wall].DZ);
        insert_collision_vect(x, ya, z, wall_list[wall].DX, yb, wall_list[wall].DZ, STOREY_TYPE_FENCE_FLAT, 8, -wall);
        insert_collision_vect(wall_list[wall].DX, yb, wall_list[wall].DZ, x, ya, z, STOREY_TYPE_FENCE_FLAT, 8, -wall);
    }
}

void build_height_fence(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height, SLONG alt_mode)
{
    SLONG c0;
    SLONG sp[10];
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;
    UBYTE* ptexture1;
    SLONG tcount1;
    UBYTE* ptexture2;
    SLONG tcount2;
    SLONG count;

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;

    ptexture1 = wall_list[wall].Textures;
    tcount1 = wall_list[wall].Tcount;

    ptexture2 = wall_list[wall].Textures2;
    tcount2 = wall_list[wall].Tcount2;

    if (alt_mode == 1) {
        sp[0] = build_row_wall_only_points_at_floor_alt(height, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[1] = build_row_wall_only_points_at_floor_alt(0, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
    } else {
        sp[0] = build_row_wall_only_points_at_y(y + height, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        sp[1] = build_row_wall_only_points_at_y(y, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
    }

    count = 0;
    for (c0 = 0; c0 < WindowCount; c0++) {
        texture = TEXTURE_PIECE_MIDDLE;
        if (c0 == 0) {
            texture = TEXTURE_PIECE_RIGHT;
        } else if (c0 == WindowCount - 1) {
            texture = TEXTURE_PIECE_LEFT;
        } else {
            texture = TEXTURE_PIECE_MIDDLE;
        }
        if (ptexture1 && (count < tcount1) && ptexture1[count]) {
            p_f4 = create_a_quad_tex(sp[0] + c0, sp[0] + c0 + 1, sp[1] + c0, sp[1] + c0 + 1, ptexture1[count]);

        } else {
            p_f4 = create_a_quad(sp[0] + c0, sp[0] + c0 + 1, sp[1] + c0, sp[1] + c0 + 1, texture_style, texture);
        }
        p_f4->ThingIndex = -wall;
        p_f4->DrawFlags |= (POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED);
        p_f4->Type = FACE_TYPE_FENCE;
        count++;
    }
    {
        SLONG ya, yb;
        ya = PAP_calc_height_at(x, z);
        yb = PAP_calc_height_at(wall_list[wall].DX, wall_list[wall].DZ);
        insert_collision_vect(x, ya, z, wall_list[wall].DX, yb, wall_list[wall].DZ, STOREY_TYPE_FENCE_FLAT, height / 64, -wall);
        insert_collision_vect(wall_list[wall].DX, yb, wall_list[wall].DZ, x, ya, z, STOREY_TYPE_FENCE_FLAT, height / 64, -wall);
    }
}

//   p1 ..
//        ..p2
//           .
//            .
//             .p3
//

/*
SLONG	find_previous_wall(SLONG storey,SLONG wall)
{
        SLONG	w,next;
        w=storey_list[storey].WallHead;
        if(w==wall)
                return(0);
        while(w)
        {
                next=wall_list[w].Next;
                if(next==wall)
                        return(w);
                w=next;
        }
        return(0);
}
*/

#define WALL_WIDTH (60)

void build_thick_wall_polys(SLONG* x, SLONG* z, SLONG y, SLONG height, SLONG flag, SLONG storey, SLONG wall)
{
    SLONG sp[8];
    SLONG c0;
    struct PrimFace4* p_f4;
    SLONG texture, texture_style;

    insert_collision_vect(x[1], y, z[1], x[0], y, z[0], STOREY_TYPE_NORMAL, 1, -wall);
    insert_collision_vect(x[2], y, z[2], x[3], y, z[3], STOREY_TYPE_NORMAL, 1, -wall);

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;

    sp[0] = build_row_wall_only_points_at_y(y + height, x[0], z[0], x[1], z[1], wall);
    sp[1] = build_row_wall_only_points_at_y(y, x[0], z[0], x[1], z[1], wall);

    sp[2] = build_row_wall_only_points_at_y(y + height, x[2], z[2], x[3], z[3], wall);
    sp[3] = build_row_wall_only_points_at_y(y, x[2], z[2], x[3], z[3], wall);
    sp[4] = next_prim_point;

    for (c0 = 0; c0 < WindowCount; c0++) {

        texture = TEXTURE_PIECE_MIDDLE;
        p_f4 = create_a_quad(sp[0] + c0 + 1, sp[0] + c0, sp[1] + c0 + 1, sp[1] + c0, texture_style, texture);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(sp[2] + c0, sp[2] + c0 + 1, sp[3] + c0, sp[3] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;

        p_f4 = create_a_quad(sp[0] + c0, sp[0] + c0 + 1, sp[2] + c0, sp[2] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        add_quad_to_walkable_list(next_prim_face4 - 1);
    }
    if (flag == 1) {
        p_f4 = create_a_quad(sp[0], sp[2], sp[1], sp[3], texture_style, texture);
        p_f4->ThingIndex = -wall;
        insert_collision_vect(x[0], y, z[0], x[2], y, z[2], STOREY_TYPE_FENCE_BRICK, 4, -wall);
    }
    if (flag == 2) {
        p_f4 = create_a_quad(sp[3] - 1, sp[1] - 1, sp[4] - 1, sp[2] - 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        insert_collision_vect(x[1], y, z[1], x[3], y, z[3], STOREY_TYPE_FENCE_BRICK, 4, -wall);
    }
}

//  2    3
//
//  0    1
SLONG build_brick_wall(SLONG storey)
{
    SLONG x[4], z[4];
    SLONG wall, nwall;
    SLONG dx, dz, len;
    SLONG y, height;
    SLONG start_point, start_face3, start_face4;
    SLONG prev_facet;
    SLONG prim;
    SLONG building;

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    y = storey_list[storey].DY;
    height = storey_list[storey].Height;

    x[0] = storey_list[storey].DX;
    z[0] = storey_list[storey].DZ;

    wall = storey_list[storey].WallHead;
    nwall = wall_list[wall].Next;

    x[1] = wall_list[wall].DX;
    z[1] = wall_list[wall].DZ;

    dz = -(x[1] - x[0]);
    dx = (z[1] - z[0]);

    len = Root(dx * dx + dz * dz);
    if (len == 0)
        len = 1;

    dx = (dx * WALL_WIDTH) / len;
    dz = (dz * WALL_WIDTH) / len;

    x[2] = x[0] + dx;
    z[2] = z[0] + dz;

    calc_new_corner_point(x[0], z[0], x[1], z[1], wall_list[nwall].DX, wall_list[nwall].DZ, WALL_WIDTH, &x[3], &z[3]);

    build_thick_wall_polys(&x[0], &z[0], y, height, 1, storey, wall);
    wall = nwall;
    nwall = wall_list[wall].Next;

    while (wall && nwall) {
        x[0] = x[1];
        x[2] = x[3];
        z[0] = z[1];
        z[2] = z[3];

        x[1] = wall_list[wall].DX;
        z[1] = wall_list[wall].DZ;

        calc_new_corner_point(x[0], z[0], x[1], z[1], wall_list[nwall].DX, wall_list[nwall].DZ, WALL_WIDTH, &x[3], &z[3]);

        build_thick_wall_polys(&x[0], &z[0], y, height, 0, storey, wall);
        wall = nwall;
        nwall = wall_list[wall].Next;
    }

    //
    //	last bit
    //
    x[0] = x[1];
    x[2] = x[3];
    z[0] = z[1];
    z[2] = z[3];

    x[1] = wall_list[wall].DX;
    z[1] = wall_list[wall].DZ;
    dz = -(x[1] - x[0]);
    dx = (z[1] - z[0]);

    len = Root(dx * dx + dz * dz);
    if (len == 0)
        len = 1;

    dx = (dx * WALL_WIDTH) / len;
    dz = (dz * WALL_WIDTH) / len;

    x[3] = x[1] + dx;
    z[3] = z[1] + dz;
    build_thick_wall_polys(&x[0], &z[0], y, height, 2, storey, wall);

    prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, 0, FACET_FLAG_NON_SORT, 0);

    prim = build_building(start_point, start_face3, start_face4, prev_facet);
    building = storey_list[storey].BuildingHead;

    /*

    //
    // No (x,y,z), buildings have a ThingIndex instead... it is
    // set in place_building_at()
    //

    building_list[building].X=build_x;
    building_list[building].Y=y;
    building_list[building].Z=build_z;

    */

    place_building_at(building, prim, build_x, 0, build_z);

    return (prev_facet);
}

void build_fence(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height)
{
    SLONG alt_mode = 1; // Stick to floor by default.

    if (storey_list[storey].ExtraFlags & FLAG_STOREY_EXTRA_ONBUILDING) {
        //
        // Dont stick on the floor.
        //

        alt_mode = 0;
    }

    switch (storey_list[storey].StoreyType) {
    case STOREY_TYPE_FENCE: //   3/4
        build_fence_points_and_faces(y, y + ((height * 3) >> 2) + 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall, 1);
        break;
    case STOREY_TYPE_FENCE_BRICK:
        build_height_fence(x, y, z, wall, storey, height, alt_mode);
        break;
    case STOREY_TYPE_FENCE_FLAT:
    case STOREY_TYPE_OUTSIDE_DOOR:
        if (height == 512)
            build_high_chain_fence(x, y, z, wall, storey, BLOCK_SIZE * 4, alt_mode);
        else
            build_height_fence(x, y, z, wall, storey, height, alt_mode);
        break;
    }
    WindowCount = 0;
}

/*
Go Through fence building all the pieces and slap them into a single facet
the facet being a special one that sorts normally
*/

SLONG build_whole_fence(SLONG storey)
{
    SLONG wall, px, pz;
    SLONG height, y;
    UWORD start_point_123 = 99, start_face3_123 = 99, start_face4_123 = 99;
    SLONG prev_facet;
    SLONG prim;
    SLONG building;
    SLONG start_point, start_face3, start_face4;

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    height = storey_list[storey].Height;
    y = storey_list[storey].DY;
    //	y+=build_min_y;

    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;

    wall = storey_list[storey].WallHead;
    while (wall) {
        build_fence(px, y, pz, wall, storey, height);
        px = wall_list[wall].DX;
        pz = wall_list[wall].DZ;
        wall = wall_list[wall].Next;
    }

    prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, 0, FACET_FLAG_NON_SORT, 0);

    prim = build_building(start_point, start_face3, start_face4, prev_facet);
    building = storey_list[storey].BuildingHead;

    /*

    //
    // No (x,y,z), buildings have a ThingIndex instead... it is
    // set in place_building_at()
    //

    building_list[building].X=build_x;
    building_list[building].Y=y;
    building_list[building].Z=build_z;

    */

    place_building_at(building, prim, build_x, 0, build_z);
    return (prev_facet);
}

SLONG process_external_pieces(UWORD building)
{
    SLONG storey, c0 = 0;
    SLONG prev_facet = 0;

    storey = building_list[building].StoreyHead;
    while (storey && c0 < 400) {
        switch (storey_list[storey].StoreyType) {
        case STOREY_TYPE_CABLE:
            prev_facet = build_cables(storey, prev_facet);
            break;
        case STOREY_TYPE_FENCE:
        case STOREY_TYPE_FENCE_FLAT:
        case STOREY_TYPE_OUTSIDE_DOOR:
            if (storey_list[storey].DY == 0)
                prev_facet = build_whole_fence(storey);
            break;
        case STOREY_TYPE_FENCE_BRICK:
            if (storey_list[storey].DY == 0)
                prev_facet = build_whole_fence(storey);
            //				prev_facet=build_brick_wall(storey);
            break;
        case STOREY_TYPE_NORMAL:
            if (storey_list[storey].DY == 0) {
                SLONG wall;
                SLONG x, y, z;
                wall = storey_list[storey].WallHead;
                x = storey_list[storey].DX;
                z = storey_list[storey].DZ;
                y = PAP_calc_height_at(x, z);
                if (y < build_min_y)
                    build_min_y = y;

                if (y > build_max_y)
                    build_max_y = y;

                while (wall) {
                    x = wall_list[wall].DX;
                    z = wall_list[wall].DZ;
                    y = PAP_calc_height_at(x, z);
                    if (y < build_min_y)
                        build_min_y = y;
                    if (y > build_max_y)
                        build_max_y = y;

                    wall = wall_list[wall].Next;
                }
            }
        }
        storey = storey_list[storey].Next;
        c0++;
    }
    if (build_max_y != build_min_y) {
        build_max_y += (ELE_SIZE >> 2) - 1;
        build_max_y &= ~63;
    }

    //	LogText(" fence facet %d \n",prev_facet);
    return (prev_facet);
}

/*   //pre store face  wall/storey links
struct	StoreyLink
{
        SWORD	Face;   //storey or wall
        UWORD	Link;
};

struct	StoreyLink	storey_link_pool[2000];
UWORD	next_storey_link=1;

UWORD	storey_heads[100];


void	build_link_table(UWORD building)
{
        UWORD	storey;

        next_storey_link=1;

        storey=building_list[building].StoreyHead;
        while(storey)
        {
                switch(storey_list[storey].StoreyType)
                {
                        case	STOREY_TYPE_NORMAL:
                                wall=storey_list[storey].WallHead;
                                while(wall)
                                {
                                        find_
                                }

                                break;
                }
                storey=storey_list[storey].Next;
        }
}
*/

void mark_map_with_ladder(SLONG storey)
{
    SLONG x1, z1, x2, z2;
    SLONG dx, dz;
    SLONG wall;

    x1 = storey_list[storey].DX;
    z1 = storey_list[storey].DZ;

    wall = storey_list[storey].WallHead;

    x2 = wall_list[wall].DX;
    z2 = wall_list[wall].DZ;

    dx = x2 - x1;
    dz = z2 - z1;

    x1 += dx / 3;
    z1 += dz / 3;

    x1 += dz >> 3;
    z1 -= dx >> 3;

    set_map_flag(x1 >> ELE_SHIFT, z1 >> ELE_SHIFT, FLOOR_LADDER);
}

void setup_storey_data(UWORD building, SWORD* wall_for_ladder)
{
    SLONG wall, storey;

    storey = building_list[building].StoreyHead;
    while (storey) {
        storey_list[storey].StoreyFlags &= ~FLAG_STOREY_FACET_LINKED;

        switch (storey_list[storey].StoreyType) {
        case STOREY_TYPE_NORMAL:
        case STOREY_TYPE_FENCE:
        case STOREY_TYPE_FENCE_FLAT:
        case STOREY_TYPE_FENCE_BRICK:
        case STOREY_TYPE_OUTSIDE_DOOR:
            wall = storey_list[storey].WallHead;
            while (wall) {
                wall_list[wall].WallFlags &= ~FLAG_WALL_FACET_LINKED;
                wall = wall_list[wall].Next;
            }

            break;
        case STOREY_TYPE_LADDER:

            wall = find_wall_for_fe(storey_list[storey].DX, storey_list[storey].DZ, building_list[building].StoreyHead);
            if (wall >= 0)
                wall_for_ladder[wall] = storey;
            mark_map_with_ladder(storey);
            break;
        }
        storey = storey_list[storey].Next;
    }
}

SLONG find_connect_wall(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG* connect_storey, SLONG storey, UBYTE** ret_tex, UWORD* ret_tcount)
{
    SLONG found = 0;
    SLONG wall;
    SLONG fx1, fz1, fx2, fz2;

    storey = storey_list[storey].Next;

    while (storey && !found) {
        switch (storey_list[storey].StoreyType) {
        case STOREY_TYPE_NORMAL:
            //			case	STOREY_TYPE_FENCE:
            //			case	STOREY_TYPE_FENCE_BRICK:
            //			case	STOREY_TYPE_FENCE_FLAT:
            found = 1;
            break;
        default:
            storey = storey_list[storey].Next;
        }
    }
    if (found) {
        fx1 = storey_list[storey].DX;
        fz1 = storey_list[storey].DZ;
        wall = storey_list[storey].WallHead;

        while (wall) {
            fx2 = wall_list[wall].DX;
            fz2 = wall_list[wall].DZ;
            if (fx1 == x1 && fz1 == z1 && fx2 == x2 && fz2 == z2) {
                *connect_storey = storey;
                *ret_tex = wall_list[wall].Textures;
                *ret_tcount = wall_list[wall].Tcount;
                return (wall);
            }
            fx1 = fx2;
            fz1 = fz2;

            wall = wall_list[wall].Next;
        }
    } else
        return (0);

    return (0);
}

//
// Add colvects around the recess...
//

void insert_recessed_wall_vect(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2,
    UBYTE storey_type,
    UBYTE height,
    SLONG wall)
{
    SLONG x1o;
    SLONG z1o;

    SLONG x2o;
    SLONG z2o;

    SLONG dx;
    SLONG dz;

    SLONG len;

    dx = x2 - x1;
    dz = z2 - z1;

    len = Root(dx * dx + dz * dz);

    if (len == 0) {
        len = 1;
    }

    dx = (dx * RECESS_SIZE) / len;
    dz = (dz * RECESS_SIZE) / len;

    x1o = x1 + dz + (dx >> 1);
    z1o = z1 - dx + (dz >> 1);

    x2o = x2 + dz - (dx >> 1);
    z2o = z2 - dx - (dz >> 1);

    insert_collision_vect(
        x1, y1, z1,
        x2, y2, z2,
        storey_type,
        height * 4,
        wall);

    insert_collision_vect(
        x1, y1, z1,
        x1o, y1, z1o,
        storey_type,
        height * 4,
        wall);

    insert_collision_vect(
        x1o, y1, z1o,
        x2o, y2, z2o,
        storey_type,
        height * 4,
        wall);

    insert_collision_vect(
        x2o, y2, z2o,
        x2, y2, z2,
        storey_type,
        height * 4,
        wall);
}

SLONG build_storey_floor(SLONG storey, SLONG y, SLONG flag)
{
    SLONG min_x = 9999999, max_x = 0, min_z = 9999999, max_z = 0;
    SLONG width, depth;
    SLONG x, z;

    SLONG wall;
    struct PrimFace4* p_f4;
    struct PrimFace3* p_f3;
    SLONG face_wall;
    SLONG angles;
    SLONG building;

    building = storey_list[storey].BuildingHead;
    face_wall = -storey_list[storey].WallHead;

    global_y = get_map_height((storey_list[storey].DX >> ELE_SHIFT), storey_list[storey].DZ >> ELE_SHIFT) << FLOOR_HEIGHT_SHIFT;

    BOUNDS(storey_list[storey].DX, storey_list[storey].DZ);
    wall = storey_list[storey].WallHead;
    while (wall) {
        BOUNDS(wall_list[wall].DX, wall_list[wall].DZ);
        wall = wall_list[wall].Next;
    }

    block_min_x = min_x;
    block_max_x = max_x;

    min_x -= ELE_SIZE;
    min_z -= ELE_SIZE;
    max_x += ELE_SIZE;
    max_z += ELE_SIZE;

    // bounds shound now be set, + bagginess
    width = (max_x - min_x) >> ELE_SHIFT;
    depth = (max_z - min_z) >> ELE_SHIFT;

    edge_min_z = min_z;

    // now step over whole rect, flagging points as either inside or outside or on the edge of the building
    angles = build_edge_list(storey, 0);
    dump_edge_list(depth);

    if (!angles) {
        SLONG bound;
        dump_edge_list(depth);
        bound = build_easy_roof(min_x, edge_min_z, max_x, depth, y, face_wall, flag);
        bin_edge_list();
        return (bound);
    }
    return (0);
}

SLONG build_trench(SLONG prev_facet, SLONG storey)
{
    SLONG start_point, start_face3, start_face4;
    SLONG wall;
    SLONG px, pz;
    SLONG col_vect;
    SLONG bound;
    SLONG min_y;
    //	UBYTE	*textures;
    //	UWORD	tcount;

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    wall = storey_list[storey].WallHead;
    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;
    min_y = PAP_calc_height_at(px, pz);
    while (wall) {
        SLONG y;
        px = wall_list[wall].DX;
        pz = wall_list[wall].DZ;
        y = PAP_calc_height_at(px, pz);
        if (y < min_y)
            min_y = y;
        wall = wall_list[wall].Next;
    }

    //	min_y<<=ALT_SHIFT;

    set_floor_hidden(storey, 1, FLOOR_TRENCH | PAP_FLAG_HIDDEN);
    wall = storey_list[storey].WallHead;
    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;
    //	textures=storey_list[storey].Textures;
    //	tcount=storey_list[storey].Tcount;
    while (wall) {
        SLONG x2, z2;

        x2 = wall_list[wall].DX;
        z2 = wall_list[wall].DZ;
        append_wall_prim(px, min_y, pz, wall, storey, -256); //,textures,tcount);
        set_vect_floor_height(px, pz, x2, z2, min_y >> ALT_SHIFT);

        //
        // Insert col vect backwards for trenches, so we can enter it but not leave it
        //
        col_vect = insert_collision_vect(x2, min_y - 256, z2, px, min_y - 256, pz, STOREY_TYPE_TRENCH, 0, -wall);
        prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, 0, col_vect);

        //	LogText(" create building prim4  next prim_point %d \n",next_prim_point);
        start_point = next_prim_point;
        start_face3 = next_prim_face3;
        start_face4 = next_prim_face4;
        //		textures=wall_list[wall].Textures;
        //		tcount=wall_list[wall].Tcount;

        wall = wall_list[wall].Next;
        px = x2;
        pz = z2;
    }
    bound = build_storey_floor(storey, min_y - 256 + 2, 1);
    prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, FACET_FLAG_ROOF, -bound);

    //	LogText(" create building prim4  next prim_point %d \n",next_prim_point);
    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;
    return (prev_facet);
}

/*

  A facet is used for sorting and quick back face culling (but not yet)
  For Each piece of wall on the bottom storey find vertically identical pieces to become
  a facet.

*/
SWORD wall_for_fe[100];
SWORD wall_for_ladder[100];

SLONG create_building_prim(UWORD building, SLONG* small_y)
{
    UBYTE pass2 = 0;
    SLONG storey;
    SLONG wall;

    SLONG start_point, start_face3, start_face4;
    SLONG mid_point, mid_face4;
    SLONG y = 0, offset_y = 0;

    ULONG obj_start_point;
    ULONG obj_start_face3, obj_start_face4;
    ULONG prev_facet = 0;
    SLONG wall_count = 0;
    SWORD fire_escape_count = 0;
    SWORD ladder_count = 0;
    SLONG first = 0;
    SLONG valid = 0;
    SLONG col_vect;
    UBYTE *textures, tcount;

    SLONG circular;

    if (storey_list[building_list[building].StoreyHead].StoreyType != STOREY_TYPE_NORMAL) {
        circular = 1;
    } else {
        circular = is_storey_circular(building_list[building].StoreyHead);
    }

    build_min_y = 999999;
    build_max_y = -999999;

    //	LogText(" create building prim  next prim_point %d \n",next_prim_point);

    current_building = building;
    *small_y = 99999;

    building_list[building].Walkable = 0;
    memset((UBYTE*)wall_for_fe, 0, 200);
    memset((UBYTE*)wall_for_ladder, 0, 200);

    process_external_pieces(building); // makes seperate buildings
    //	LogText(" create building prim2  next prim_point %d \n",next_prim_point);

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    obj_start_point = start_point;
    obj_start_face3 = start_face3;
    obj_start_face4 = start_face4;

    setup_storey_data(building, &wall_for_ladder[0]); // clear connection flags
    storey = building_list[building].StoreyHead;

    if (circular) {
        //		build_max_y=0;
        if (build_min_y != build_max_y)
            build_max_y -= 256;

        //
        // build_max_y is maximum fllor height the building overlaps rounded up to nearest quarter block -256
        //

        offset_y = build_max_y;
    } else {
        offset_y = 0;
        build_min_y = 0;
        build_max_y = 0;
    }

    building_list[building].OffsetY = build_max_y;
    while (storey) {
        SLONG x1, z1, x2, z2;
        //		LogText("MCD build storey %d \n",storey);

        switch (storey_list[storey].StoreyType) {
        case STOREY_TYPE_FENCE_FLAT:
        case STOREY_TYPE_OUTSIDE_DOOR:
        case STOREY_TYPE_FENCE:
        case STOREY_TYPE_FENCE_BRICK:
            if (storey_list[storey].DY) {
                x1 = storey_list[storey].DX;
                z1 = storey_list[storey].DZ;
                y = storey_list[storey].DY;
                wall = storey_list[storey].WallHead;
                while (wall) {

                    build_fence(x1, y + offset_y, z1, wall, storey, storey_list[storey].Height);

                    x1 = wall_list[wall].DX;
                    z1 = wall_list[wall].DZ;

                    wall = wall_list[wall].Next;

                    prev_facet = build_facet(start_point, mid_point, start_face3, start_face4, mid_face4, prev_facet, 0, col_vect);

                    //	LogText(" create building prim4  next prim_point %d \n",next_prim_point);
                    start_point = next_prim_point;
                    start_face3 = next_prim_face3;
                    start_face4 = next_prim_face4;
                }
            }

            break;
        case STOREY_TYPE_NORMAL:
            valid = 1;
            if (first == 0) {
                set_floor_hidden(storey, 0, PAP_FLAG_HIDDEN);
                first = 1;
            }

            x1 = storey_list[storey].DX;
            z1 = storey_list[storey].DZ;

            wall_count = 0;
            wall = storey_list[storey].WallHead;
            while (wall) {
                SLONG connect_wall, connect_storey, connect_count = 0;

                x2 = wall_list[wall].DX;
                z2 = wall_list[wall].DZ;
                if (!(wall_list[wall].WallFlags & FLAG_WALL_FACET_LINKED)) {
                    y = storey_list[storey].DY;

                    if (y == 0) {
                        SLONG temp_y;
                        temp_y = PAP_calc_height_at(x1, z1);
                        if (temp_y < *small_y)
                            *small_y = temp_y;
                    }
                    //						LogText("MCD normal storey wall append%d at y %d \n",wall,y+offset_y);

                    wall_list[wall].WallFlags |= FLAG_WALL_FACET_LINKED;
                    if ((y) == 0 && (build_min_y != build_max_y) && circular)
                        append_foundation_wall(x1, y + offset_y + 256, z1, wall, storey, storey_list[storey].Height); // this wants the top of the wall
                    else
                        append_wall_prim(x1, y + offset_y, z1, wall, storey, storey_list[storey].Height); //,textures,tcount); //this wants the base of the wall

                    {
                        UBYTE* tex = 0;
                        UWORD tcount = 0;
                        connect_wall = find_connect_wall(x1, z1, x2, z2, &connect_storey, storey, &tex, &tcount);
                        connect_count = 1;
                        if (connect_wall) {
                            //							LogText(" found a connect wall %d\n",connect_wall);

                            while (connect_wall) {
                                SLONG ty;
                                connect_count++;
                                ty = storey_list[connect_storey].DY;
                                wall_list[connect_wall].WallFlags |= FLAG_WALL_FACET_LINKED;
                                append_wall_prim(x1, ty + offset_y, z1, connect_wall, connect_storey, storey_list[connect_storey].Height); //,tex,tcount);
                                connect_wall = find_connect_wall(x1, z1, x2, z2, &connect_storey, connect_storey, &tex, &tcount);
                            }
                        } else {
                            //
                            //
                            //
                        }
                    }

                    if (wall_list[wall].WallFlags & FLAG_WALL_RECESSED) {
                        //
                        // Add colvects around the recess...
                        //

                        insert_recessed_wall_vect(
                            x1, y + offset_y, z1,
                            x2, y + offset_y, z2,
                            STOREY_TYPE_NORMAL,
                            connect_count,
                            -wall);
                    } else {
                        if (y == 0) {
                            col_vect = insert_collision_vect(x1, y + offset_y, z1, x2, y + offset_y, z2, STOREY_TYPE_NORMAL_FOUNDATION, connect_count * 4, -wall);
                        } else
                            col_vect = insert_collision_vect(x1, y + offset_y, z1, x2, y + offset_y, z2, STOREY_TYPE_NORMAL, connect_count * 4, -wall);
                    }

                    mid_point = next_prim_point;
                    //					LogText(" create building prim3  next prim_point %d \n",next_prim_point);
                    mid_face4 = next_prim_face4;
                    if (wall_for_fe[wall_count] && pass2 == 0) {
                        build_firescape(wall_for_fe[wall_count]);
                    }
                    //						if(wall_for_ladder[wall_count]&&pass2==0)
                    //						{
                    //							build_ladder(wall_for_ladder[wall_count]);
                    //						}

                    prev_facet = build_facet(start_point, mid_point, start_face3, start_face4, mid_face4, prev_facet, 0, col_vect);

                    //	LogText(" create building prim4  next prim_point %d \n",next_prim_point);
                    start_point = next_prim_point;
                    start_face3 = next_prim_face3;
                    start_face4 = next_prim_face4;
                }
                //					else
                //						LogText(" allready done through connection \n");

                x1 = x2;
                z1 = z2;
                textures = wall_list[wall].Textures;
                tcount = wall_list[wall].Tcount;

                wall = wall_list[wall].Next;
                wall_count++;
            }
            pass2 = 1;

            break;
        case STOREY_TYPE_LADDER:
            //				build_ladder(storey);
            break;
        case STOREY_TYPE_FIRE_ESCAPE:
            wall = find_wall_for_fe(storey_list[storey].DX, storey_list[storey].DZ, building_list[building].StoreyHead);
            if (wall >= 0)
                wall_for_fe[wall] = storey;
            fire_escape_count++;
            break;
        case STOREY_TYPE_TRENCH:
            prev_facet = build_trench(prev_facet, storey);
            start_point = next_prim_point;
            start_face3 = next_prim_face3;
            start_face4 = next_prim_face4;
            break;
        case STOREY_TYPE_STAIRCASE:
            build_staircase(storey);
            prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, 0, 0);

            start_point = next_prim_point;
            start_face3 = next_prim_face3;
            start_face4 = next_prim_face4;
            break;

            /* moved to setup_storey_data
                                    case	STOREY_TYPE_LADDER:
                                            wall=find_wall_for_fe(storey_list[storey].DX,storey_list[storey].DZ,building_list[building].StoreyHead);
                                            if(wall>=0)
                                                    wall_for_ladder[wall]=storey;
                                            ladder_count++;
                                            break;
            */

        default:
            break;
        }
        //		if(storey_list[storey].StoreyFlags&FLAG_STOREY_ROOF_RIM)
        //			offset_y+=BLOCK_SIZE+(BLOCK_SIZE>>1)-20;

        storey = storey_list[storey].Next;
    }

    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    storey = building_list[building].StoreyHead;
    while (storey) {
        SLONG bound;

        if ((storey_list[storey].StoreyFlags & (FLAG_STOREY_TILED_ROOF | FLAG_STOREY_FLAT_TILED_ROOF)) && (storey_list[storey].StoreyType == STOREY_TYPE_NORMAL)) {
            SLONG flat = 0;
            if (storey_list[storey].StoreyFlags & (FLAG_STOREY_FLAT_TILED_ROOF)) {
                flat = 1;
            }
            bound = build_roof(storey, storey_list[storey].DY + offset_y + storey_list[storey].Height, flat);
        }

        if (0) // storey_list[storey].Roof)
        {
            /*
                                    switch(storey_list[storey_list[storey].Roof].StoreyType)
                                    {
                                            case	STOREY_TYPE_ROOF:
                                                            if(building==3)
                                                            build_roof(storey,storey_list[storey_list[storey].Roof].DY+offset_y);
                                                            break;
                                            case	STOREY_TYPE_ROOF_QUAD:
                                                            if(building==3)
                                                            build_roof_quad(storey,storey_list[storey_list[storey].Roof].DY+offset_y);
                                                            break;
                                            case	STOREY_TYPE_LADDER:
                                                            if(building==3)
                                                            build_ladder(storey);
                                            break;

                                    }
            */
        } else {
            switch (storey_list[storey].StoreyType) {
            case STOREY_TYPE_LADDER:
                build_ladder(storey);
                break;
            case STOREY_TYPE_SKYLIGHT: // where should a skylight be processed?
                build_skylight(storey);
                break;
            }
        }

        prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, FACET_FLAG_ROOF, 0);
        start_point = next_prim_point;
        start_face3 = next_prim_face3;
        start_face4 = next_prim_face4;

        if (storey_list[storey].InsideStorey) {
            SLONG istorey, iwall;
            SLONG added_wall = 0;

            istorey = storey_list[storey].InsideStorey;
            while (istorey) {
                SLONG x1, y1, z1;

                iwall = storey_list[istorey].WallHead;
                x1 = storey_list[istorey].DX;
                y1 = storey_list[istorey].DY;
                z1 = storey_list[istorey].DZ;

                while (iwall) {
                    added_wall = 1;
                    append_wall_prim(x1, y1 + offset_y, z1, iwall, istorey, 256); //,tex,tcount);
                    x1 = wall_list[iwall].DX;
                    z1 = wall_list[iwall].DZ;
                    iwall = wall_list[iwall].Next;
                }
                istorey = storey_list[istorey].Next;
            }

            if (added_wall) {
                SLONG c0;
                /*
                                                for(c0=start_face4;c0<next_prim_face4;c0++)
                                                {
                                                        prim_faces4[c0].DrawFlags|=POLY_FLAG_DOUBLESIDED;
                                                }
                */

                prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, FACET_FLAG_INSIDE, -bound);
                start_point = next_prim_point;
                start_face3 = next_prim_face3;
                start_face4 = next_prim_face4;
            }
        }
        storey = storey_list[storey].Next;
    }

    //	LogText(" create building prim5  next prim_point %d \n",next_prim_point);
    if (valid) {
        prev_facet = build_facet(start_point, next_prim_point, start_face3, start_face4, next_prim_face4, prev_facet, FACET_FLAG_ROOF, 0);
    }
    start_point = next_prim_point;
    start_face3 = next_prim_face3;
    start_face4 = next_prim_face4;

    if (valid) {
        //
        // We want crates to always have the 'crate' texture.
        //

        if (0)
            if (building_list[building].BuildingType == BUILDING_TYPE_CRATE_IN || building_list[building].BuildingType == BUILDING_TYPE_CRATE_OUT) {
                //
                // Retexture the quads...
                //

                SLONG i;

                PrimFace4* p_f4;

                for (i = obj_start_face4; i < next_prim_face4; i++) {
                    p_f4 = &prim_faces4[i];

                    p_f4->UV[0][0] = 7 * 32 + 0;
                    p_f4->UV[0][1] = 7 * 32 + 0;

                    p_f4->UV[1][0] = 7 * 32 + 31;
                    p_f4->UV[1][1] = 7 * 32 + 0;

                    p_f4->UV[2][0] = 7 * 32 + 0;
                    p_f4->UV[2][1] = 7 * 32 + 31;

                    p_f4->UV[3][0] = 7 * 32 + 31;
                    p_f4->UV[3][1] = 7 * 32 + 31;

                    p_f4->TexturePage = 0;
                }
            }

        return (build_building(obj_start_point, obj_start_face3, obj_start_face4, prev_facet));
    } else {
        return (0);
    }
}

void copy_to_game_map(void)
{
    SLONG x, z, c0;
}

void clear_map2(void)
{
    SLONG x, z, c0;

    //
    // clear pap lo
    //
    memset((UBYTE*)&PAP_lo[0][0], 0, sizeof(PAP_Lo) * PAP_SIZE_LO * PAP_SIZE_LO);

    for (x = 0; x < EDIT_MAP_WIDTH; x++)
        for (z = 0; z < EDIT_MAP_DEPTH; z++) {
            mask_map_flag(x, z, (FLOOR_HIDDEN | FLOOR_LADDER | FLOOR_TRENCH));
        }

    next_building_object = 1;
    next_building_facet = 1;
    next_col_vect = 1;
    next_col_vect_link = 1;
    next_walk_link = 1;

    next_dbuilding = 1;
    next_dwalkable = 1;
    next_dfacet = 1;
    next_dstyle = 1;
    next_facet_link = 1;
    facet_link_count = 0;

    memset((UBYTE*)prim_points, 0, sizeof(struct PrimPoint) * MAX_PRIM_POINTS);
    memset((UBYTE*)prim_faces3, 0, sizeof(struct PrimFace3) * MAX_PRIM_FACES3);
    memset((UBYTE*)prim_faces4, 0, sizeof(struct PrimFace4) * MAX_PRIM_FACES4);
    memset((UBYTE*)prim_objects, 0, sizeof(struct PrimObject) * MAX_PRIM_OBJECTS);
    memset((UBYTE*)prim_multi_objects, 0, sizeof(struct PrimMultiObject) * MAX_PRIM_MOBJECTS);
}

void clear_floor_ladder(void)
{
    SLONG x, z;
}

void wibble_floor(void)
{
    SLONG dx, dz;
    return;
    // #ifdef	EDITOR
    for (dx = 0; dx < EDIT_MAP_WIDTH; dx++)
        for (dz = 0; dz < EDIT_MAP_DEPTH; dz++) {
            //		edit_map[dx][dz].Y=(COS((dx*15)&2047)+SIN((dz*15)&2047))>>12;
            set_map_height(dx, dz, (COS((dx * 15) & 2047) + SIN((dz * 15) & 2047)) >> 10);
        }
    // #endif
}

void clip_building_prim(SLONG prim, SLONG x, SLONG y, SLONG z)
{
    SLONG index;
    SLONG best_z = -999999, az;
    struct BuildingFacet* p_facet;
    SLONG sp, ep;
    struct PrimFace4* p_f4;
    struct PrimFace3* p_f3;
    SLONG c0;

    //	LogText(" draw a building %d at %d %d %d\n",building,x,y,z);
    index = building_objects[prim].FacetHead;
    while (index) {
        p_facet = &building_facets[index];

        if (p_facet->EndFace4)
            for (c0 = p_facet->StartFace4; c0 < p_facet->EndFace4; c0++) {
            }

        if (p_facet->EndFace3)
            for (c0 = p_facet->StartFace3; c0 < p_facet->EndFace3; c0++) {
            }

        sp = p_facet->StartPoint;
        ep = p_facet->EndPoint;
        for (c0 = sp; c0 < ep; c0++) {
            SLONG px, py, pz;
            SLONG fy;

            px = prim_points[c0].X + x;
            py = prim_points[c0].Y + y;
            pz = prim_points[c0].Z + z;

            fy = PAP_calc_height_at(px, pz);

            if (py < fy) {
                prim_points[c0].Y += fy - py;
            }
        }

        index = building_facets[index].NextFacet;
    }
}

SLONG calc_win(UWORD* attack, SLONG c1, UWORD* def, SLONG c2)
{
    SLONG b1 = 0, b2 = 0;
    UWORD data[10];
    SLONG a1, a2, d1, d2;
    SLONG wins = 0;

    memcpy((UBYTE*)data, (UBYTE*)def, 10);

    if (data[0] < data[1]) {
        SWAP(data[0], data[1]);
    }
    if (data[1] < data[2]) {
        SWAP(data[1], data[2]);
    }
    if (data[0] < data[1]) {
        SWAP(data[0], data[1]);
    }

    if (c2 > 1) {
        if (data[c1] < data[c1 + 1])
            SWAP(data[c1], data[c1 + 1]);

        if (data[1] > data[c1 + 1]) {
            wins--;
        } else {
            wins++;
        }
    }
    if (data[0] > data[c1]) {
        wins--;
    } else {
        wins++;
    }

    return (wins);
}

void calc_prob(void)
{
    UWORD c[6];
    SLONG total = 0, total_win1 = 0, total_win2 = 0;

    for (c[0] = 1; c[0] < 7; c[0]++)
        for (c[1] = 1; c[1] < 7; c[1]++)
            for (c[2] = 1; c[2] < 7; c[2]++)
                for (c[3] = 1; c[3] < 7; c[3]++)
                    for (c[4] = 1; c[4] < 7; c[4]++) {
                        SLONG win;
                        win = calc_win(&c[0], 3, &c[3], 2);
                        total_win2 += win;
                        win = calc_win(&c[0], 3, &c[3], 1);
                        total_win1 += win;
                        total++;
                    }
}

void fix_furniture(void)
{
    Thing* p_thing;
    SLONG c0;
    p_thing = TO_THING(0);
    for (c0 = 1; c0 < MAX_THINGS; c0++) {
        switch (p_thing->Class) {
        case CLASS_FURNITURE:
            //				if(abs(p_thing->WorldPos.Y)<(10<<8))
            p_thing->WorldPos.Y = PAP_calc_height_at(p_thing->WorldPos.X >> 8, p_thing->WorldPos.Z >> 8) << 8;
            break;
        }
    }
}

void count_floor(void)
{
    SLONG x, z;
    SLONG page, tx, ty;
    UWORD texture;
    for (x = 0; x < 128; x++)
        for (z = 0; z < 128; z++) {
            texture = get_map_texture(x, z);

            tx = ((struct MiniTextureBits*)(&texture))->X;
            ty = ((struct MiniTextureBits*)(&texture))->Y;
            page = (UBYTE)(((struct MiniTextureBits*)(&texture))->Page);
            add_page_countxy(tx, ty, page);
        }
}

void create_city(UBYTE mode)
{
    SLONG c0;
    SLONG bcount = 0;
    SLONG temp_next_prim;
    SLONG temp_next_face3;
    SLONG temp_next_face4;
    SLONG temp_next_point;
    SLONG temp_next_building_object;
    SLONG temp_next_building_facet;

    diff_page_count1 = 0;
    diff_page_count2 = 0;
    memset(page_count, 0, 8 * 64);
    count_floor();
    // calc_prob();

    build_mode = mode;
    //	save_all_prims("temp.sav");
    if (mode == BUILD_MODE_EDITOR) {
        copy_to_game_map();
        clear_map2();
        load_all_individual_prims();
    }

    //	Stuck in by Guy.
    next_building_object = 1;
    next_building_facet = 1;
    next_col_vect = 1;
    next_col_vect_link = 1;
    next_walk_link = 1;
    //

    // REMOVED FOR E3 DEMO
    //	load_all_prims("allprim.sav");

    temp_next_prim = next_prim_object;
    temp_next_face3 = next_prim_face3;
    temp_next_face4 = next_prim_face4;
    temp_next_point = next_prim_point;
    temp_next_building_object = next_building_object;
    temp_next_building_facet = next_building_facet;

    /*
     */

    //	wibble_floor();
    for (c0 = 1; c0 < MAX_BUILDINGS; c0++) {
        SLONG prim;
        if (building_list[c0].BuildingFlags) {
            SLONG y;
            //			printf(" about to create build %d \n",c0);
            prim = create_building_prim(c0, &y);
            y = 0; // build_min_y;
            //			printf(" DONE create prim \n");
            if (prim) {
                /*

                //
                // No (x,y,z), buildings have a ThingIndex instead... it is
                // set in place_building_at()
                //

                building_list[c0].X=build_x;
                building_list[c0].Y=y;
                building_list[c0].Z=build_z;

                */

                // y=calc_height_at(build_x,build_z);
                //			LogText(" place building y %d \n",y);
                place_building_at(c0, prim, build_x, y, build_z);

                extern void save_asc(UWORD building, UWORD version);
                //			save_asc(c0,1);

                // clip_building_prim(prim,build_x,y,build_z);
                //				LogText(" after building %d, nextprimpoint %d \n",bcount,next_prim_point);
                bcount++;
            }
        }
        //		LogText(" next walk link %d \n",next_walk_link);
    }
    extern void apply_global_amb_to_map(void);
    //	printf(" about to light map \n");
    //	apply_global_amb_to_map();

    //	printf(" done light map \n");
    /*
            next_prim_object			 =temp_next_prim;
            next_prim_face3			 =temp_next_face3;
            next_prim_face4			 =temp_next_face4;
            next_prim_point			 =temp_next_point;
            next_building_object= temp_next_building_object;
            next_building_facet = temp_next_building_facet;
    */
    //
    // Calculate the normals of all the buildings.
    //

    //	printf(" about to norm \n");
    calc_building_normals();
    //	printf(" about to clear ladder\n");
    clear_floor_ladder();
    //	printf(" about to shadow\n");
    //	SHADOW_do();
    //	printf(" done shadow\n");

    {
        SLONG high = 0;
        for (c0 = 0; c0 < MAX_WALLS; c0++) {
            if (wall_list[c0].WallFlags)
                high = c0;
        }
    }
    {
        SLONG high = 0;
        for (c0 = 0; c0 < MAX_STOREYS; c0++) {
            if (storey_list[c0].StoreyFlags)
                high = c0;
        }
    }
    {
        SLONG high = 0;
        for (c0 = 0; c0 < MAX_BUILDINGS; c0++) {
            if (building_list[c0].BuildingFlags)
                high = c0;
        }
    }

    for (c0 = 1; c0 < next_dwalkable; c0++) {
        SLONG face;

        for (face = dwalkables[c0].StartFace4; face < dwalkables[c0].EndFace4; face++) {
            prim_faces4[face].ThingIndex = c0; // dwalkables[c0].Building;
            void attach_walkable_to_map(SLONG face);

            {
                SLONG point, c0;
                for (c0 = 0; c0 < 4; c0++) {
                    point = prim_faces4[face].Points[c0];
                }
            }

            attach_walkable_to_map(face);
            prim_faces4[face].FaceFlags |= FACE_FLAG_WALKABLE;
        }
    }

    //	fix_furniture();
}

//**************************************8

void offset_buildings(SLONG x, SLONG y, SLONG z)
{
    SLONG c0;
    for (c0 = 1; c0 < MAX_STOREYS; c0++) {
        if (storey_list[c0].StoreyFlags) {
            storey_list[c0].DX += (SWORD)x;
            storey_list[c0].DY += (SWORD)y;
            //		storey_list[c0].DY=-storey_list[c0].DY+(SWORD)y;
            storey_list[c0].DZ += (SWORD)z;
        }
    }
    for (c0 = 1; c0 < MAX_WALLS; c0++) {
        if (wall_list[c0].WallFlags) {
            wall_list[c0].DX += (SWORD)x;
            wall_list[c0].DY += (SWORD)y; //-wall_list[c0].DY+(SWORD)y;
            wall_list[c0].DZ += (SWORD)z;
        }
    }
    /*
     */
}

/*
void	calc_buildings_screen_box(UWORD	prim,SLONG x,SLONG y,SLONG z,EdRect *rect)
{
        SLONG	c0,flags;
        struct	PrimObject	*p_obj;
        SLONG	sp,ep;
        SLONG	min_x=999999,max_x=-999999,min_y=999999,max_y=-999999;

        p_obj    =&prim_objects[prim];

        sp=p_obj->StartPoint;
        ep=p_obj->EndPoint;

        engine.X-=x<<8;
        engine.Y-=y<<8;
        engine.Z-=z<<8;

        for(c0=sp;c0<ep;c0++)
        {
                //transform all points for this Object
                flags=rotate_point_gte((struct SVector*)&prim_points[c0],&global_res[c0-sp]);
//		if(!(flags & EF_CLIPFLAGS))
                {
                        if(global_res[c0-sp].X<min_x)
                                min_x=global_res[c0-sp].X;

                        if(global_res[c0-sp].X>max_x)
                                max_x=global_res[c0-sp].X;

                        if(global_res[c0-sp].Y<min_y)
                                min_y=global_res[c0-sp].Y;

                        if(global_res[c0-sp].Y>max_y)
                                max_y=global_res[c0-sp].Y;
                }

        }

        engine.X+=x<<8;
        engine.Y+=y<<8;
        engine.Z+=z<<8;
        if(min_x<0)
                min_x=0;
        if(min_y<0)
                min_y=0;

        rect->SetRect(min_x-2,min_y-2,max_x-min_x+4,max_y-min_y+4);
}

void	calc_buildings_world_box(UWORD	prim,SLONG x,SLONG y,SLONG z,EdRect *rect)
{
        SLONG	c0;
        struct	PrimObject	*p_obj;
        SLONG	sp,ep;
        SLONG	min_x=999999,max_x=-999999,min_y=999999,max_y=-999999;

        p_obj    =&prim_objects[prim];

        sp=p_obj->StartPoint;
        ep=p_obj->EndPoint;


        for(c0=sp;c0<ep;c0++)
        {
                global_res[c0-sp].X=prim_points[c0].X+x;
                global_res[c0-sp].Y=prim_points[c0].Y+y;
                global_res[c0-sp].Z=prim_points[c0].Z+z;
                {
                        if(global_res[c0-sp].X<min_x)
                                min_x=global_res[c0-sp].X;

                        if(global_res[c0-sp].X>max_x)
                                max_x=global_res[c0-sp].X;

                        if(global_res[c0-sp].Y<min_y)
                                min_y=global_res[c0-sp].Y;

                        if(global_res[c0-sp].Y>max_y)
                                max_y=global_res[c0-sp].Y;
                }

        }

        if(min_x<0)
                min_x=0;
        if(min_y<0)
                min_y=0;

        rect->SetRect(min_x,min_y,max_x-min_x,max_y-min_y);
}
*/
UWORD is_it_clockwise(SLONG p0, SLONG p1, SLONG p2)
{
    SLONG z;
    SLONG vx, vy, wx, wy;

    vx = global_res[p1].X - global_res[p0].X;
    wx = global_res[p2].X - global_res[p1].X;
    vy = global_res[p1].Y - global_res[p0].Y;
    wy = global_res[p2].Y - global_res[p1].Y;

    z = vx * wy - vy * wx;

    if (z > 0)
        return 1;
    else
        return 0;
}

//
// Calculates the normals of the points in all the buildings.
//
#define MAX_POINTS_PER_BUILDING 12560

extern UBYTE each_point[]; // MAX_POINTS_PER_BUILDING];

void calc_building_normals(void)
{
    SLONG i;
    SLONG j;
    SLONG k;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG num_points;
    SVector fnormal;
    SLONG p_index;

    BuildingObject* p_obj;
    PrimFace3* p_f3;
    PrimFace4* p_f4;
    PrimPoint* p_pt;

    for (i = 1; i < next_building_object; i++) {
        p_obj = &building_objects[i];

        num_points = p_obj->EndPoint - p_obj->StartPoint;

        ASSERT(num_points <= MAX_POINTS_PER_BUILDING);

        //
        // Mark all the points as having zero faces using them.
        //

        memset(each_point, 0, sizeof(UBYTE) * num_points);

        //
        // Work out the normal for each point by going through
        // all the faces.
        //

        for (j = p_obj->StartFace3; j < p_obj->EndFace3; j++) {
            p_f3 = &prim_faces3[j];

            //
            // What is the normal of this face?
            //

            calc_normal(-j, &fnormal);

            //
            // Use this normal to work out the normal of each point that
            // makes up the face.
            //

            for (k = 0; k < 3; k++) {
                p_index = p_f3->Points[k] - p_obj->StartPoint;

                ASSERT(WITHIN(p_index, 0, MAX_POINTS_PER_BUILDING - 1));

                if (each_point[p_index] == 0) {
                    //
                    // This is the only face that we know uses the point,
                    // so make the normal of the point equal to the normal
                    // of the face.
                    //

                    prim_normal[p_f3->Points[k]].X = fnormal.X;
                    prim_normal[p_f3->Points[k]].Y = fnormal.Y;
                    prim_normal[p_f3->Points[k]].Z = fnormal.Z;

                    each_point[p_index] = 1;
                } else {
                    //
                    // Average this faces' normal with the current normal.
                    //

                    prim_normal[p_f3->Points[k]].X *= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Y *= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Z *= each_point[p_index];

                    prim_normal[p_f3->Points[k]].X += fnormal.X;
                    prim_normal[p_f3->Points[k]].Y += fnormal.Y;
                    prim_normal[p_f3->Points[k]].Z += fnormal.Z;

                    each_point[p_index] += 1;

                    prim_normal[p_f3->Points[k]].X /= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Y /= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Z /= each_point[p_index];
                }
            }
        }

        for (j = p_obj->StartFace4; j < p_obj->EndFace4; j++) {
            p_f4 = &prim_faces4[j];

            //
            // What is the normal of this face?
            //

            calc_normal(j, &fnormal);

            //
            // Use this normal to work out the normal of each point that
            // makes up the face.
            //

            for (k = 0; k < 4; k++) {
                p_index = p_f4->Points[k] - p_obj->StartPoint;

                ASSERT(WITHIN(p_index, 0, MAX_POINTS_PER_BUILDING - 1));

                if (each_point[p_index] == 0) {
                    //
                    // This is the only face that we know uses the point,
                    // so make the normal of the point equal to the normal
                    // of the face.
                    //

                    prim_normal[p_f4->Points[k]].X = fnormal.X;
                    prim_normal[p_f4->Points[k]].Y = fnormal.Y;
                    prim_normal[p_f4->Points[k]].Z = fnormal.Z;

                    each_point[p_index] = 1;
                } else {
                    //
                    // Average this faces' normal with the current normal.
                    //

                    prim_normal[p_f4->Points[k]].X *= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Y *= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Z *= each_point[p_index];

                    prim_normal[p_f4->Points[k]].X += fnormal.X;
                    prim_normal[p_f4->Points[k]].Y += fnormal.Y;
                    prim_normal[p_f4->Points[k]].Z += fnormal.Z;

                    each_point[p_index] += 1;

                    prim_normal[p_f4->Points[k]].X /= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Y /= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Z /= each_point[p_index];
                }
            }
        }

        //
        // Normalise the length of each normal to be 256.
        //

        SLONG old_nx;
        SLONG old_ny;
        SLONG old_nz;

        for (j = p_obj->StartPoint; j < p_obj->EndPoint; j++) {
            old_nx = prim_normal[j].X;
            old_ny = prim_normal[j].Y;
            old_nz = prim_normal[j].Z;

            dx = abs(prim_normal[j].X);
            dy = abs(prim_normal[j].Y);
            dz = abs(prim_normal[j].Z);

            dist = dx * dx + dy * dy + dz * dz;
            dist = Root(dist);
            dist += 1;

            prim_normal[j].X <<= 8;
            prim_normal[j].Y <<= 8;
            prim_normal[j].Z <<= 8;

            prim_normal[j].X /= dist;
            prim_normal[j].Y /= dist;
            prim_normal[j].Z /= dist;

            if ((prim_normal[j].X * prim_normal[j].X + prim_normal[j].Y * prim_normal[j].Y + prim_normal[j].Z * prim_normal[j].Z) > 67536) {
                ASSERT(0);
            }
        }
    }
}

//---------------------------------------------------------------

void fn_building_normal(Thing* b_thing)
{
    Switch* the_switch;

    /*
            if(b_thing->SwitchThing)
            {
                    the_switch	=	TO_THING(b_thing->SwitchThing)->Genus.Switch;
                    if(the_switch->Flags&SWITCH_FLAGS_TRIGGERED)
                    {
                            b_thing->Flags	&=	~FLAGS_LOCKED;
                    }
            }
            */
}

//---------------------------------------------------------------
