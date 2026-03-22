// Chunk 7+ of Building.cpp — chunks 1–6 migrated to new/world/environment/building.cpp.
// This file contains the remaining functions (original lines ~1353+).
// Globals and chunks 1–6 functions now live in new/world/environment/building*.

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

// Forward declarations for functions still defined later in this file (chunk 7+).
void calc_building_normals(void);

// Chunks 1–6 migrated: all functions through insert_recessed_wall_vect now in new/
// -- make_cable_taut_along, make_cable_flabby, create_suspended_light,
//    build_cable, build_cable_old, build_cables,
//    build_fence_points_and_faces, build_high_chain_fence, build_height_fence,
//    build_thick_wall_polys, build_brick_wall, build_fence, build_whole_fence,
//    process_external_pieces, mark_map_with_ladder, setup_storey_data,
//    find_connect_wall, insert_recessed_wall_vect

// Shared constant needed by chunk 7+ functions.
#define RECESS_SIZE (32)

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
