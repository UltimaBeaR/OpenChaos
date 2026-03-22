// Chunk 2+ of Building.cpp — migrated to new/world/environment/building.cpp (chunk 1: lines 1-1520).
// This file contains the remaining functions (lines 1521-9878).
// Globals and chunk-1 functions now live in new/world/environment/building*.

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

void set_floor_hidden(SLONG storey, UWORD lower, UWORD flags);

// Forward declarations for functions defined later in this file (chunk 2+).
struct PrimFace4* create_a_quad(UWORD p1, UWORD p0, UWORD p3, UWORD p2, SWORD texture_style, SWORD texture_piece, SLONG flipx = 0);
void build_face_texture_info(struct PrimFace4* p_f4, UWORD texture);
struct PrimFace3* create_a_tri(UWORD p2, UWORD p1, UWORD p0, SWORD texture_id, SWORD texture_piece);
void build_ledge(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height);
SLONG build_ledge2(SLONG y, SLONG storey, SLONG out, SLONG height, SLONG dip);
SLONG create_strip_points(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG len, SLONG numb, SLONG end_flag);
SLONG build_outline(SLONG* sx, SLONG* sz, SLONG storey, SLONG wall, SLONG y, SLONG out);
void calc_building_normals(void);

// Local lookup table for PSX-style UV coordinates (used by create_a_quad/create_a_tri in this chunk).
// Not a public global — defined here for chunk 2+ use only.
struct TXTY texture_xy2[] = {
    { 0, 0, 0 }, // 0
    { 0, 32, 0 }, // 1
    { 0, 64, 0 }, // 2
    { 0, 0, 32 }, // 3
    { 0, 32, 32 }, // 4
    { 0, 64, 32 }, // 5
    { 0, 96, 32 }, // 6
    { 0, 128, 32 }, // 7
    { 0, 0, 64 }, // 8
    { 0, 32, 64 }, // 9
    { 0, 64, 64 }, // 10
    { 0, 96, 64 }, // 11
    { 0, 128, 64 }, // 12
    { 0, 0, 96 }, // 13
    { 0, 32, 96 }, // 14
    { 0, 64, 96 }, // 15
    { 0, 96, 96 }, // 16
    { 0, 128, 96 }, // 17
    { 0, 0, 128 }, // 18
    { 0, 32, 128 }, // 19
    { 0, 64, 128 }, // 20
    { 0, 96, 128 }, // 21
    { 0, 128, 128 }, // 22
    { 0, 0, 160 }, // 23
    { 0, 32, 160 }, // 24
    { 0, 64, 160 }, // 25
    { 0, 96, 160 }, // 26
    { 0, 128, 160 }, // 27
    { 3, 4 * 32, 6 * 32 }, // 28
    { 0, 0 }
};

// set_UV4 — migrated to new/world/environment/building.cpp (chunk 2)

// build_free_tri_texture_info, build_free_quad_texture_info, scan_45,
// build_storey_lip, current_building (global), create_walkable_structure,
// calc_face_split, ROT1/ROT2/ROT3 macros, lookup_roof[], build_easy_roof
// -- all migrated to new/world/environment/building.cpp (chunk 2)

void clear_reflective_flag(SLONG min_x, SLONG min_z, SLONG max_x, SLONG max_z)
{
    SLONG minx, maxx, minz, maxz;
    SLONG x, z;

    minx = min_x >> ELE_SHIFT;
    maxx = max_x >> ELE_SHIFT;

    minz = min_z >> ELE_SHIFT;
    maxz = max_z >> ELE_SHIFT;

    SATURATE(minx, 0, 127);
    SATURATE(maxx, 0, 127);
    SATURATE(minz, 0, 127);
    SATURATE(maxz, 0, 127);

    for (x = minx; x < maxx; x++)
        for (z = minz; z < maxz; z++) {
        }
}

//
// Returns the OUTLINE_Outline of the given storey.  Returns NULL if
// the storey is not circular.  Non-circular storeys are not defined.
//

OUTLINE_Outline* get_storey_outline(SLONG storey)
{
    OUTLINE_Outline* oo;

    SLONG wall;
    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;

    if (!is_storey_circular(storey)) {
        //
        // Not circular stories don't have outlines.
        //

        return NULL;
    }

    oo = OUTLINE_create(128);

    x1 = storey_list[storey].DX >> 8;
    z1 = storey_list[storey].DZ >> 8;

    wall = storey_list[storey].WallHead;

    while (wall) {
        x2 = wall_list[wall].DX >> 8;
        z2 = wall_list[wall].DZ >> 8;

        OUTLINE_add_line(oo, x1, z1, x2, z2);

        x1 = x2;
        z1 = z2;

        wall = wall_list[wall].Next;
    }

    return oo;
}

SLONG do_storeys_overlap(SLONG s1, SLONG s2)
{
    OUTLINE_Outline* oos;
    OUTLINE_Outline* ool;
    oos = get_storey_outline(s1);
    if (oos == NULL)
        return (0);
    ool = get_storey_outline(s2);
    if (ool == NULL)
        return (0);

    if (OUTLINE_overlap(oos, ool)) {
        return (1);
    } else {
        return (0);
    }
}

// claude-ai: build_roof_grid: Top-level roof builder. Dispatches to build_easy_roof (axis-aligned)
// claude-ai: or the complex angled roof path (for buildings with 45-degree walls, scan_45).
// claude-ai: First applies optional lip/rim geometry (build_storey_lip) if FLAG_STOREY_ROOF_RIM is set.
// claude-ai: Computes the building's full bounding box (including all storeys) for the edge list.
// claude-ai: If storey has a "next" storey above it (SKYLIGHT or NORMAL type at height+1), that
//   storey's outline is also fed into the edge list (build_more_edge_list) to cut holes in the roof.
// claude-ai: angles=0 → easy roof; angles=1 → complex roof with tri fill at diagonal walls.
SLONG build_roof_grid(SLONG storey, SLONG y, SLONG flat_flag)
{
    SLONG min_x = 9999999, max_x = 0, min_z = 9999999, max_z = 0;
    SLONG width, depth;
    SLONG x, z;

    SLONG wall;
    struct PrimFace4* p_f4;
    struct PrimFace3* p_f3;
    SLONG face_wall;
    SLONG angles, sstorey;
    SLONG building;

    building = storey_list[storey].BuildingHead;
    // return;
    if (storey_list[storey].StoreyFlags & (FLAG_STOREY_ROOF_RIM | FLAG_STOREY_ROOF_RIM2))
        y = build_storey_lip(storey, y);
    face_wall = -storey_list[storey].WallHead;

    //	LogText(" build roof grid for storey %d at y %d \n",storey,y);

    //	global_y=edit_map[(storey_list[storey].DX>>ELE_SHIFT)][storey_list[storey].DZ>>ELE_SHIFT].Y<<FLOOR_HEIGHT_SHIFT;
    global_y = get_map_height((storey_list[storey].DX >> ELE_SHIFT), storey_list[storey].DZ >> ELE_SHIFT) << FLOOR_HEIGHT_SHIFT;

    BOUNDS(storey_list[storey].DX, storey_list[storey].DZ);

    sstorey = building_list[building].StoreyHead;
    while (sstorey) {
        wall = storey_list[sstorey].WallHead;
        BOUNDS(storey_list[sstorey].DX, storey_list[sstorey].DZ);
        while (wall) {
            BOUNDS(wall_list[wall].DX, wall_list[wall].DZ);
            wall = wall_list[wall].Next;
        }
        sstorey = storey_list[sstorey].Next;
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

    clear_reflective_flag(min_x, min_z, max_x, max_z);
    set_floor_hidden(storey, 0, PAP_FLAG_REFLECTIVE);

    // now step over whole rect, flagging points as either inside or outside or on the edge of the building
    angles = build_edge_list(storey, 0);

    //	ASSERT(storey!=5);

    //
    // set floor reflective just under this storey
    //

    dump_edge_list(depth);
    if (storey_list[storey].Next) {
        SLONG s;
        SLONG storey_height;
        storey_height = storey_list[storey].Height;

        s = building_list[building].StoreyHead;
        while (s) {
            if (do_storeys_overlap(s, storey) && (storey_list[s].DY == storey_list[storey].DY + storey_height) && (storey_list[s].StoreyType == STOREY_TYPE_SKYLIGHT || storey_list[s].StoreyType == STOREY_TYPE_NORMAL)) {
                build_more_edge_list(min_z, max_z, s, 0);
            }

            s = storey_list[s].Next;
        }
    }

    if (storey == 3) {
    }

    if (!angles) {
        SLONG bound;
        //		if(storey==214)
        //			ASSERT(0);
        dump_edge_list(depth);
        bound = build_easy_roof(min_x, edge_min_z, max_x, depth, y, face_wall, flat_flag); // 0 changed to 1   for flat rooves
        bin_edge_list();
        clear_reflective_flag(min_x, min_z, max_x, max_z);
        return (bound);
    }

    for (z = 0; z < depth; z++) {
        SLONG polarity = 0;
        SLONG edge;
        SLONG dy;
        SLONG prev_x_in = 0;
        edge = edge_heads_ptr[z];

        for (x = min_x - 256; x < max_x; x += ELE_SIZE) {
            SLONG done = 0;
            while (!done && edge) {
                if (x < edge_pool_ptr[edge].X) {
                    if (polarity & 1) {
                        // struct	DepthStrip *me;

                        dy = get_map_height((x >> ELE_SHIFT), z + (edge_min_z >> ELE_SHIFT)) << FLOOR_HEIGHT_SHIFT;
                        add_point(x, y + dy, (z << ELE_SHIFT) + edge_min_z);

                        flag_blocks[(x >> ELE_SHIFT) + z * MAX_BOUND_SIZE] = next_prim_point - 1;
                        //						LogText(" write flag %d at %d,%d \n",next_prim_point-1,x>>ELE_SHIFT,z);
                    }
                    done = 1;
                } else if (x == edge_pool_ptr[edge].X) {
                    // grid[][]
                    polarity++;
                    // if(polarity&1)
                    {
                        struct DepthStrip* me;

                        dy = get_map_height((x >> ELE_SHIFT), z + (edge_min_z >> ELE_SHIFT)) << FLOOR_HEIGHT_SHIFT - global_y;
                        add_point(x, y, (z << ELE_SHIFT) + edge_min_z);
                        flag_blocks[(x >> ELE_SHIFT) + z * MAX_BOUND_SIZE] = next_prim_point - 1;
                    }

                    edge = edge_pool_ptr[edge].Next;
                    done = 1;
                } else if (x > edge_pool_ptr[edge].X) {
                    //
                    // we have just crossed an edge
                    //

                    polarity++;
                    edge = edge_pool_ptr[edge].Next;
                    if (edge == 0) {
                        // if(polarity&1)
                        // flag_blocks[(x>>ELE_SHIFT)+z*MAX_BOUND_SIZE]=INSIDE;
                        // else
                        // flag_blocks[(x>>ELE_SHIFT)+z*MAX_BOUND_SIZE]=OUTSIDE;
                    }
                }
            }
        }
    }
    //	ASSERT(0);
    build_bottom_edge_list(storey, y);

    {
        SLONG wall;
        SLONG px, pz;
        px = storey_list[storey].DX;
        pz = storey_list[storey].DZ - edge_min_z;
        wall = storey_list[storey].WallHead;
        while (wall) {
            SLONG x, z;
            SLONG dx, dz;
            x = wall_list[wall].DX;
            z = wall_list[wall].DZ - edge_min_z;

            dx = x - px;
            dz = z - pz;
            if (abs(dx) == abs(dz) && dx) {
                // 45 degree wall
                scan_45(px, pz, dx, dz);
            }
            px = x;
            pz = z;
            wall = wall_list[wall].Next;
        }
    }

    //	show_grid((max_x>>ELE_SHIFT)-(min_x>>ELE_SHIFT),depth,min_x>>ELE_SHIFT);
    for (z = 0; z < depth; z++) {
        SLONG polarity = 0;
        SLONG edge;
        edge = edge_heads_ptr[z];
        for (x = min_x >> ELE_SHIFT; x < max_x >> ELE_SHIFT; x++) {
            SLONG p0, p1, p2, p3;
            //
            // Build faces by finding quads with defined points
            //
            //  p0   p1
            //
            //	p3	 p2
            p0 = flag_blocks[x + z * MAX_BOUND_SIZE];
            p1 = flag_blocks[x + 1 + z * MAX_BOUND_SIZE];
            p2 = flag_blocks[x + 1 + (z + 1) * MAX_BOUND_SIZE];
            p3 = flag_blocks[x + (z + 1) * MAX_BOUND_SIZE];
            //			LogText(" x %d z %d p01234 %d %d %d %d \n",x,z,p0,p1,p2,p3);
            if (p0 && p1 && p2 && p3) {
                SLONG texture;
                //				LogText(" use poly %d %d \n",x,z);
                // UWORD p1,UWORD p0,UWORD p3,UWORD p2,SWORD	texture_style,SWORD texture_piece)
                p_f4 = create_a_quad(p0, p3, p1, p2, 0, 0);
                if (p_f4) {
                    p_f4->ThingIndex = face_wall;
                    //				LogText(" roof grid quad for map %d %d \n",x,z+(edge_min_z>>ELE_SHIFT));
                    add_quad_to_walkable_list(next_prim_face4 - 1);
                    texture = get_map_texture(x, z + (edge_min_z >> ELE_SHIFT));
                    build_face_texture_info(p_f4, texture);
                }
                //				LogText(" add grid roof face to walkable \n");
            } else if (p0 || p1 || p2 || p3) { // the following code is shit, sorry
                UBYTE exist_flags = 0;

#define TL (1)
#define TR (2)
#define BL (4)
#define BR (8)
                if (p0)
                    exist_flags |= TL;
                if (p1)
                    exist_flags |= TR;
                if (p2)
                    exist_flags |= BR;
                if (p3)
                    exist_flags |= BL;

                switch (exist_flags) {
                    SLONG xt, xb;
                    SLONG zl, zr;
                    SLONG pa, pb;

                case (TR + BR):
                    xt = cut_blocks[x * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    xb = cut_blocks[x * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];

                    if (xt && xb) {
                        //  pa  p1
                        //
                        //	pb	p2
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        p_f4 = create_a_quad(pa, pb, p1, p2, 0, 0);
                        if (p_f4) {
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                        //							LogText(" done tr+br 1 at [%d][%d]\n",x,z);
                    } else if (xt) {
                        //   pa p1
                        //
                        //		p2
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        p_f3 = create_a_tri(p2, p1, pa, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //							LogText(" done tr+br 2 at [%d][%d]\n",x,z);
                    } else if (xb) {
                        //   	 p1
                        //
                        //	  pa p2
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        p_f3 = create_a_tri(p2, p1, pa, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //							LogText(" done tr+br 3 at [%d][%d]\n",x,z);
                    }

                    break;
                case (BL + BR):
                    zl = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];

                    if (zl && zr) {
                        //  pa  pb
                        //
                        //	p3	p2
                        pa = next_prim_point;
                        add_point(x << ELE_SHIFT, y, zl + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                        p_f4 = create_a_quad(pa, p3, pb, p2, 0, 0);
                        if (p_f4) {
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                        //							LogText(" done bl+br 1 at [%d][%d]\n",x,z);
                    } else if (zl) {
                        //
                        //	pa
                        //	p3	 p2
                        pa = next_prim_point;
                        add_point(x << ELE_SHIFT, y, zl + edge_min_z);
                        p_f3 = create_a_tri(p3, p2, pa, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //							LogText(" done bl+br 2 at [%d][%d]\n",x,z);
                    } else if (zr) {
                        //
                        //		 pa
                        //	p3	 p2
                        pa = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                        p_f3 = create_a_tri(p3, p2, pa, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //						LogText(" done bl+br 3 at [%d][%d]\n",x,z);
                    }
                    break;
                case (TL + BL):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    xb = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];

                    if (xt && xb) {
                        //  p0  pa
                        //
                        //	p3	pb
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        p_f4 = create_a_quad(p0, p3, pa, pb, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        //						LogText(" done tl+bl 1 at [%d][%d]\n",x,z);
                    } else if (xt) {
                        //  p0 pa
                        //
                        //	p3
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        p_f3 = create_a_tri(p3, pa, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //						LogText(" done tl+bl 2 at [%d][%d]\n",x,z);
                    } else if (xb) {
                        //  p0
                        //
                        //	p3 pa
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        p_f3 = create_a_tri(p3, pa, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //						LogText(" done tl+bl 3 at [%d][%d]\n",x,z);
                    }
                    /* can not be surely
                                                                    else
                                                                    {
                                                                            zr=cut_blocks[(x)*4+(z)*MAX_BOUND_SIZE*4+CUT_BLOCK_RIGHT];
                                                                            if(zr)
                                                                            {
                                                                                    //  p0
                                                                                    //	    pa
                                                                                    //	p3
                                                                                    pa=next_prim_point;
                                                                                    add_point((x+1)<<ELE_SHIFT,y,zr+edge_min_z);
                                                                                    create_a_tri(p3,pa,p0,0,0);

                                                                            }

                                                                    }
                    */
                    // LogText(" pooerror3\n");
                    break;
                case (TL + TR):
                    zl = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];
                    //					LogText(" zl %x zr %x \n",zl,zr);

                    if (zl && zr) {
                        //  p0  p1
                        //
                        //	pa	pb
                        pa = next_prim_point;
                        add_point(x << ELE_SHIFT, y, zl + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                        p_f4 = create_a_quad(p0, pa, p1, pb, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        //						LogText(" done tl+tr 1 at [%d][%d]\n",x,z);
                    } else if (zl) {
                        //  p0   p1
                        //	pa
                        //
                        pa = next_prim_point;
                        add_point(x << ELE_SHIFT, y, zl + edge_min_z);
                        p_f3 = create_a_tri(pa, p1, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //						LogText(" done tl+tr 2 at [%d][%d]\n",x,z);
                    } else if (zr) {
                        //  p0   p1
                        //		 pa
                        //
                        pa = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                        p_f3 = create_a_tri(pa, p1, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //						LogText(" done tl+tr 3 at [%d][%d]\n",x,z);
                    }
                    break;

                case (TR + BR + BL):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    zl = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];
                    //					LogText("POO1 xt %x zl %x \n",xt,zl);
                    //					LogText(" [%d][%d] \n",x,z);
                    if (xt && zl) {
                        //   pa p1
                        //	pb
                        //	p3	p2
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x) << ELE_SHIFT, y, zl + edge_min_z);

                        p_f4 = create_a_quad(pa, pb, p1, p3, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        p_f3 = create_a_tri(p3, p2, p1, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //						LogText(" done tr+br+bl 1 at [%d][%d]\n",x,z);
                    } else if (xt || zl) {
                        if (xt == (x + 1) << ELE_SHIFT || xt == 0) {
                            //      pa
                            //	pb
                            //	p3	p2
                            pb = next_prim_point;
                            add_point((x) << ELE_SHIFT, y, zl + edge_min_z);
                            p_f4 = create_a_quad(pb, p3, p1, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                            //							LogText(" done tr+br+bl 2 at [%d][%d]\n",x,z);

                        } else if (zl == (z + 1) << ELE_SHIFT || zl == 0) {
                            //   pa p1
                            //
                            //	p3	p2
                            //							LogText("special 1b \n");
                            pa = next_prim_point;
                            add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                            p_f4 = create_a_quad(pa, p3, p1, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                            //						LogText(" done tr+br+bl 3 at [%d][%d]\n",x,z);
                        }
                    }
                    //						else
                    //							create_a_tri(p3,p2,p1,0,0);

                    break;
                case (TL + BR + BL):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];
                    //				LogText("POO2 xt %x zr %x \n",xt,zr);
                    //				LogText(" [%d][%d] \n",x,z);
                    //				LogText(" p0 (%d,%d,%d) p2 (%d,%d,%d) p3 (%d,%d,%d) \n",prim_points[p0].X,prim_points[p0].Y,prim_points[p0].Z,prim_points[p2].X,prim_points[p2].Y,prim_points[p2].Z,prim_points[p3].X,prim_points[p3].Y,prim_points[p3].Z);

                    if (xt && zr) {
                        //  p0 pa
                        //		 pb
                        //	p3	 p2
                        pa = next_prim_point;
                        add_point(xt, y, (z << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);

                        p_f4 = create_a_quad(pb, pa, p2, p0, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        p_f3 = create_a_tri(p3, p2, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //							LogText(" done tl+br+bl 1 at [%d][%d]\n",x,z);
                    } else if (xt || zr) {
                        if (xt == x << ELE_SHIFT || xt == 0) {
                            //  pa
                            //		pb
                            //	p3	p2
                            pb = next_prim_point;
                            add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                            p_f4 = create_a_quad(p0, p3, pb, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));

                        } else if (zr == ((z + 1) << ELE_SHIFT) || zr == 0) {
                            //  p0 pa
                            //
                            //	p3	p2
                            pb = next_prim_point;
                            add_point((xt), y, (z << ELE_SHIFT) + edge_min_z);
                            p_f4 = create_a_quad(p0, p3, pa, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                    }
                    //						else
                    //							create_a_tri(p3,p2,p0,0,0);

                    break;
                case (TL + TR + BL):
                    xb = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];

                    if (xb && zr) {
                        //  p0   p1
                        //		 pb
                        //	p3 pa
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);

                        p_f4 = create_a_quad(pa, pb, p3, p1, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        p_f3 = create_a_tri(p3, p1, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (xb || zr) {
                        if (zr == ((z + 1) << ELE_SHIFT) || zr == 0) {
                            //  p0  p1
                            //
                            //	p3 pa
                            pa = next_prim_point;
                            add_point((xb), y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                            p_f4 = create_a_quad(p0, p3, p1, pa, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));

                        } else if (xb == (x << ELE_SHIFT) || xb == 0) {
                            //  p0  p1
                            //		pb
                            //	pa3
                            pb = next_prim_point;
                            add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);
                            p_f4 = create_a_quad(p0, p3, p1, pb, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                    }
                    //						else
                    //							create_a_tri(p3,p1,p0,0,0);

                    break;

                case (TL + TR + BR):
                    xb = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];
                    zl = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];

                    if (xb && zl) {
                        //  p0  p1
                        //	pb
                        //	 pa	p2
                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x) << ELE_SHIFT, y, zl + edge_min_z);

                        p_f4 = create_a_quad(pb, pa, p0, p2, 0, 0);
                        p_f4->ThingIndex = face_wall;
                        build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        p_f3 = create_a_tri(p0, p2, p1, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    } else if (xb || zl) {
                        if (xb == (x + 1) << ELE_SHIFT || xb == 0) {
                            //  p0  p1
                            //	pb
                            //		p2a
                            pb = next_prim_point;
                            add_point((x) << ELE_SHIFT, y, zl + edge_min_z);
                            p_f4 = create_a_quad(p0, pb, p1, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));

                        } else if (zl == (z) << ELE_SHIFT || zl == 0) {
                            //  pb   p1
                            //
                            //	  pa p2
                            pa = next_prim_point;
                            add_point((xb), y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                            p_f4 = create_a_quad(p0, pb, p1, p2, 0, 0);
                            p_f4->ThingIndex = face_wall;
                            build_free_quad_texture_info(p_f4, x, z + (edge_min_z >> ELE_SHIFT));
                        }
                    }
                    //						else
                    //							create_a_tri(p2,p1,p0,0,0);
                    break;
                case (TL):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    zl = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];
                    //						LogText("SINGLE1 xt %x zl %x \n",xt,zl);
                    //						LogText(" [%d][%d] \n",x,z);

                    if (xt && zl) {
                        //  p0  pa
                        //	pb

                        pa = next_prim_point;
                        add_point(xt, y, ((z) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x) << ELE_SHIFT, y, zl + edge_min_z);

                        p_f3 = create_a_tri(pb, pa, p0, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //							LogText(" done tl 1 at [%d][%d]\n",x,z);
                    }
                    break;
                case (TR):
                    xt = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_TOP];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];
                    //						LogText("SINGLE2 xt %x zr %x \n",xt,zr);
                    //						LogText(" [%d][%d] \n",x,z);

                    if (xt && zr) {
                        //  pa  p1
                        //		pb

                        pa = next_prim_point;
                        add_point(xt, y, ((z) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);

                        p_f3 = create_a_tri(pa, pb, p1, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }
                    break;
                case (BR):
                    xb = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];
                    zr = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_RIGHT];
                    //						LogText("SINGLE3 xb %x zr %x \n",xb,zr);
                    //						LogText(" [%d][%d] \n",x,z);

                    if (xb && zr) {
                        //      pb
                        //	 pa	p2

                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x + 1) << ELE_SHIFT, y, zr + edge_min_z);

                        p_f3 = create_a_tri(pb, pa, p2, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                    }
                    break;
                case (BL):
                    xb = cut_blocks[(x) * 4 + z * MAX_BOUND_SIZE * 4 + CUT_BLOCK_BOTTOM];
                    zl = cut_blocks[(x) * 4 + (z)*MAX_BOUND_SIZE * 4 + CUT_BLOCK_LEFT];
                    //						LogText("SINGLE4 xb %x zl %x \n",xb,zl);
                    //						LogText(" [%d][%d] \n",x,z);

                    if (xb && zl) {
                        //  pb
                        //	p3 pa

                        pa = next_prim_point;
                        add_point(xb, y, ((z + 1) << ELE_SHIFT) + edge_min_z);
                        pb = next_prim_point;
                        add_point((x) << ELE_SHIFT, y, zl + edge_min_z);

                        p_f3 = create_a_tri(p3, pa, pb, 0, 0);
                        build_free_tri_texture_info(p_f3, x, z + (edge_min_z >> ELE_SHIFT));
                        //							LogText(" done bl 1 at [%d][%d]\n",x,z);
                    }
                    break;

                default:
                    break;
                }
            }
        }
    }

    bin_edge_list();
    return (0);
}

// claude-ai: is_storey_circular: Returns TRUE if the storey's wall list forms a closed polygon.
// claude-ai: "Circular" means the last wall's endpoint equals the storey's start point (DX,DZ).
// claude-ai: Non-circular storeys (e.g. fire escapes, ladders) don't have outlines or roof grids.
SLONG is_storey_circular(SLONG storey)
{
    SLONG sx, sz, wall;
    sx = storey_list[storey].DX;
    sz = storey_list[storey].DZ;
    wall = storey_list[storey].WallHead;

    while (wall) {
        if (sx == wall_list[wall].DX && sz == wall_list[wall].DZ) {
            return (1);
        }
        wall = wall_list[wall].Next;
    }
    return (0);
}

// claude-ai: set_floor_hidden: Marks PAP map tiles inside a storey's footprint with the given flags.
// claude-ai: Used to set PAP_FLAG_REFLECTIVE (floor shows reflections) and PAP_FLAG_HIDDEN.
// claude-ai: Works by scanline-filling the storey's polygon footprint (same approach as STAIR module).
// claude-ai: Only works for circular storeys. lower=0 disables the lower-floor logic (dead code, if(0)).
void set_floor_hidden(SLONG storey, UWORD lower, UWORD flags)
{
    SLONG min_x = 9999999, max_x = 0, min_z = 9999999, max_z = 0;
    SLONG width, depth;
    SLONG x, z;

    SLONG wall;
    //	LogText(" set  floor hidden storey %d \n",storey);
    if (!is_storey_circular(storey)) {
        //		LogText(" not circular \n");
        return;
    }

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

    // now step over whole rect, flagging poins as either inside or outside or on the edge of the building
    build_edge_list(storey, 0);
    //	dump_edge_list(depth);

    for (z = 0; z < depth; z++) {
        SLONG polarity = 0;
        SLONG edge;
        edge = edge_heads_ptr[z];
        for (x = min_x; x < max_x; x += ELE_SIZE) {
            SLONG done = 0;
            while (!done && edge) {
                if (x < edge_pool_ptr[edge].X) {
                    if (polarity & 1) {
                        struct DepthStrip* me;

                        set_map_flag(
                            x >> PAP_SHIFT_HI,
                            z + (edge_min_z >> PAP_SHIFT_HI),
                            flags);

                        // hidden
                    }
                    done = 1;
                } else if (x == edge_pool_ptr[edge].X) {
                    // grid[][]
                    polarity += edge_pool_ptr[edge].Count;
                    if (polarity & 1) {
                        struct DepthStrip* me;

                        set_map_flag(
                            x >> PAP_SHIFT_HI,
                            z + (edge_min_z >> PAP_SHIFT_HI),
                            flags);
                    }

                    edge = edge_pool_ptr[edge].Next;
                    done = 1;
                } else if (x > edge_pool_ptr[edge].X) {
                    polarity += edge_pool_ptr[edge].Count;
                    edge = edge_pool_ptr[edge].Next;
                }
            }
        }
    }

    if (0) // lower)
    {
        for (z = 0; z < depth; z++) {
            UWORD pfu, pfl;
            pfu = get_map_flags(min_x >> PAP_SHIFT_HI, z + (edge_min_z >> PAP_SHIFT_HI));
            pfl = get_map_flags(min_x >> PAP_SHIFT_HI, z + (edge_min_z >> PAP_SHIFT_HI) + 1);

            for (x = min_x + (1 << PAP_SHIFT_HI); x < max_x - (1 << PAP_SHIFT_HI); x += (1 << PAP_SHIFT_HI)) {
                UWORD fu, fl;
                fu = get_map_flags(x >> PAP_SHIFT_HI, z + (edge_min_z >> PAP_SHIFT_HI));
                fl = get_map_flags(x >> PAP_SHIFT_HI, z + (edge_min_z >> PAP_SHIFT_HI) + 1);
                if ((fu & fl & pfu & pfl & PAP_FLAG_HIDDEN)) {
                    set_map_height((x >> PAP_SHIFT_HI) + 1, z + (edge_min_z >> PAP_SHIFT_HI) + 1, -256 >> 2);
                }

                pfu = fu;
                pfl = fl;
            }
        }
    }

    bin_edge_list();
}

void build_fe_mid_points(SLONG y, SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG flag)
{
    SLONG dx, dz, dist;

    dx = abs(x2 - x1);
    dz = abs(z2 - z1);

    dist = Root(SDIST2(dx, dz));

    if (dist == 0)
        return;

    dx = (dx * BLOCK_SIZE) / dist;
    dz = (dz * BLOCK_SIZE) / dist;

    add_point(x1 + dx, y, z1 + dz);
    if (flag == 0)
        add_point(x2 - dx, y, z2 - dz);
}

void build_fire_escape_points(UWORD storey, SLONG y, SLONG flag)
{
    SLONG walls[3], count = 0, wall;
    SLONG mx, mz, mx2, mz2;
    SLONG p0 = 0;
    if (flag == 0) {
        add_point(storey_list[storey].DX, y, storey_list[storey].DZ);
        // LogText(" point %d is storey \n",p0++);
    }
    wall = storey_list[storey].WallHead;
    while (wall && count < 3) {
        walls[count++] = wall;
        if (flag == 0) {

            add_point(wall_list[wall].DX, y, wall_list[wall].DZ);
            //			LogText(" point %d is rest of wall \n",p0++);
        }
        wall = wall_list[wall].Next;
    }

    mx = (storey_list[storey].DX + wall_list[walls[2]].DX) >> 1;
    mz = (storey_list[storey].DZ + wall_list[walls[2]].DZ) >> 1;

    mx2 = (wall_list[walls[0]].DX + wall_list[walls[1]].DX) >> 1;
    mz2 = (wall_list[walls[0]].DZ + wall_list[walls[1]].DZ) >> 1;

    if (flag == 0) {
        add_point(mx, y, mz);
        //			LogText(" point %d is middle left \n",p0++);
        add_point(mx2, y, mz2);
        //			LogText(" point %d is middle right \n",p0++);
    }
    build_fe_mid_points(y, mx, mz, mx2, mz2, flag);
    //			LogText(" point %d is middle mid left \n",p0++);
    //			LogText(" point %d is middle mid right \n",p0++);
    build_fe_mid_points(y, wall_list[walls[2]].DX, wall_list[walls[2]].DZ, wall_list[walls[1]].DX, wall_list[walls[1]].DZ, flag);
    //			LogText(" point %d is front mid left \n",p0++);
    //			LogText(" point %d is front mid right \n",p0++);
}

#define PsetUV4(p_f4, x0, y0, x1, y1, x2, y2, x3, y3, page) \
    p_f4->UV[0][0] = (x0);                                  \
    p_f4->UV[0][1] = (y0);                                  \
    p_f4->UV[1][0] = (x1);                                  \
    p_f4->UV[1][1] = (y1);                                  \
    p_f4->UV[2][0] = (x2);                                  \
    p_f4->UV[2][1] = (y2);                                  \
    p_f4->UV[3][0] = (x3);                                  \
    p_f4->UV[3][1] = (y3);                                  \
    p_f4->TexturePage = page;

// claude-ai: build_face_texture_info: Applies a MiniTextureBits-encoded texture to an existing quad face.
// claude-ai: MiniTextureBits layout (16-bit): X (3 bits), Y (3 bits), Page (4 bits), Rot (2 bits), Size (2 bits).
// claude-ai: tx = X<<5, ty = Y<<5 (texture atlas position in 32-pixel units).
// claude-ai: Rot applies 0-3 rotations to the UV coordinates (rot+3 & 3 = counter-rotate convention).
// claude-ai: Used for roof tile textures and other map-texture-driven surfaces.
// claude-ai: PORT NOTE: MiniTextureBits is the packed texture reference format used throughout the map system.
void build_face_texture_info(struct PrimFace4* p_f4, UWORD texture)
{
    UBYTE tx, ty, page;
    SLONG tsize;
    SLONG rot;

    tx = ((struct MiniTextureBits*)(&texture))->X << 5;
    ty = ((struct MiniTextureBits*)(&texture))->Y << 5;
    page = (UBYTE)(((struct MiniTextureBits*)(&texture))->Page);
    tsize = 31; // floor_texture_sizes[((struct	MiniTextureBits*)(&texture))->Size]-1;
    rot = ((struct MiniTextureBits*)(&texture))->Rot;
    rot = (rot + 3) & 3;
    switch (rot) {
    case 0:

        PsetUV4(p_f4, tx, ty, tx + tsize, ty, tx, ty + tsize, tx + tsize, ty + tsize, page);
        break;
    case 1:
        PsetUV4(p_f4, tx + tsize, ty, tx + tsize, ty + tsize, tx, ty, tx, ty + tsize, page);
        break;
    case 2:
        PsetUV4(p_f4, tx + tsize, ty + tsize, tx, ty + tsize, tx + tsize, ty, tx, ty, page);
        break;
    case 3:
        PsetUV4(p_f4, tx, ty + tsize, tx, ty, tx + tsize, ty + tsize, tx + tsize, ty, page);
        break;
    }
}

//   0     1
//
//   2     3
// claude-ai: set_quad_planar_flag: Detects if a quad is non-planar (two triangles have different normals).
// claude-ai: Computes normals for triangles (p0,p2,p1) and (p1,p3,p2), normalizes both to 64 units.
// claude-ai: If normals differ: sets FACE_FLAG_NON_PLANAR on the face.
// claude-ai: Non-planar quads are split into two triangles during rendering (see calc_face_split).
void set_quad_planar_flag(struct PrimFace4* pf4)
{
    SLONG p0, p1, p2, p3;
    SLONG nx, ny, nz, mx, my, mz;
    SLONG vx, vy, vz, wx, wy, wz;

    p0 = pf4->Points[0];
    p1 = pf4->Points[1];
    p2 = pf4->Points[2];
    p3 = pf4->Points[3];

    vx = prim_points[p0].X - prim_points[p2].X;
    vy = prim_points[p0].Y - prim_points[p2].Y;
    vz = prim_points[p0].Z - prim_points[p2].Z;

    wx = prim_points[p1].X - prim_points[p0].X;
    wy = prim_points[p1].Y - prim_points[p0].Y;
    wz = prim_points[p1].Z - prim_points[p0].Z;

    nx = vy * wz - vz * wy;
    ny = vz * wx - vx * wz;
    nz = vx * wy - vy * wx;

    {
        SLONG len;

        len = Root(nx * nx + ny * ny + nz * nz);
        if (len == 0)
            len = 1;
        nx = (nx * 64) / len;
        ny = (ny * 64) / len;
        nz = (nz * 64) / len;
    }

    vx = prim_points[p3].X - prim_points[p1].X;
    vy = prim_points[p3].Y - prim_points[p1].Y;
    vz = prim_points[p3].Z - prim_points[p1].Z;

    wx = prim_points[p2].X - prim_points[p3].X;
    wy = prim_points[p2].Y - prim_points[p3].Y;
    wz = prim_points[p2].Z - prim_points[p3].Z;

    mx = vy * wz - vz * wy;
    my = vz * wx - vx * wz;
    mz = vx * wy - vy * wx;

    {
        SLONG len;

        len = Root(mx * mx + my * my + mz * mz);
        if (len == 0)
            len = 1;
        mx = (mx * 64) / len;
        my = (my * 64) / len;
        mz = (mz * 64) / len;
    }

    if ((nx != mx) || (ny != my) || (nz != mz)) {
        pf4->FaceFlags |= FACE_FLAG_NON_PLANAR;
    }
}

// claude-ai: create_a_quad: Allocates and initializes one quad face (PrimFace4) in the prim buffer.
// claude-ai: Points layout: p0=TL, p1=TR, p2=BL, p3=BR (note: API takes p1,p0,p3,p2 — swapped).
// claude-ai: texture_style=0: uses texture_xy2[] fallback table (simple indexed textures).
// claude-ai: texture_style>0: looks up textures_xy[style][piece] (full building texture system).
// claude-ai: TEXTURE_PIECE_MIDDLE has a 25% chance to use MIDDLE1 or MIDDLE2 variants (visual variety).
// claude-ai: If build_psx: remaps texture pages via page_remap[] for PSX VRAM layout.
// claude-ai: flip: XOR of table flip flag and flipx parameter → controls UV mirror.
// claude-ai: Calls set_quad_planar_flag() to detect non-planar quads (FACE_FLAG_NON_PLANAR).
struct PrimFace4* create_a_quad(UWORD p1, UWORD p0, UWORD p3, UWORD p2, SWORD texture_style, SWORD texture_piece, SLONG flipx)
{
    struct PrimFace4* p4;
    SLONG tx, ty;
    SLONG theight = 31;
    SLONG add_page = 1;
    SLONG page_to;
    SLONG flip;

    if (texture_style == 0)
        add_page = 0;

    p4 = &prim_faces4[next_prim_face4];
    next_prim_face4++;

    p4->Points[0] = p0;
    p4->Points[1] = p1;
    p4->Points[2] = p2;
    p4->Points[3] = p3;
    /*
            p4->Bright[0]=128;
            p4->Bright[1]=128;
            p4->Bright[2]=128;
            p4->Bright[3]=128;
    */
    p4->DrawFlags = POLY_GT;
    p4->FaceFlags = 0;

    set_quad_planar_flag(p4);

    if (texture_style) {
        if (texture_piece == TEXTURE_PIECE_MIDDLE) {
            if ((build_rand() & 3) == 0) {
                if (build_rand() & 1)
                    texture_piece = TEXTURE_PIECE_MIDDLE1;
                else
                    texture_piece = TEXTURE_PIECE_MIDDLE2;
            }
        }

        tx = textures_xy[texture_style][texture_piece].Tx << 5;
        ty = textures_xy[texture_style][texture_piece].Ty << 5;
        p4->TexturePage = textures_xy[texture_style][texture_piece].Page;
        flip = textures_xy[texture_style][texture_piece].Flip;

        if (add_page)
            add_page_countxy(tx >> 5, ty >> 5, p4->TexturePage);

        //		LogText(" USE texture_style tx %d ty %d page %d \n",tx,ty,p4->TexturePage);
        p4->DrawFlags = textures_flags[texture_style][texture_piece];
    } else {
        //		ASSERT(0);
        tx = texture_xy2[texture_piece].Tx;
        ty = texture_xy2[texture_piece].Ty;
        p4->TexturePage = texture_xy2[texture_piece].Page;
        flip = textures_xy[texture_style][texture_piece].Flip;
        if (add_page)
            add_page_countxy(tx >> 5, ty >> 5, p4->TexturePage);
        ASSERT(p4->TexturePage < 15);
    }

    //	ASSERT(p4->TexturePage<15);
    if (flipx)
        flip ^= 1;

    switch (flip) // textures_xy[texture_style][texture_piece].Flip)
    {
    case 0:
        p4->UV[0][0] = tx;
        p4->UV[0][1] = ty;
        p4->UV[1][0] = tx + 31;
        p4->UV[1][1] = ty;
        p4->UV[2][0] = tx;
        p4->UV[2][1] = ty + theight;
        p4->UV[3][0] = tx + 31;
        p4->UV[3][1] = ty + 31;
        break;
    case 1: // flip x
        p4->UV[0][0] = tx + 31;
        p4->UV[0][1] = ty;
        p4->UV[1][0] = tx;
        p4->UV[1][1] = ty;
        p4->UV[2][0] = tx + 31;
        p4->UV[2][1] = ty + theight;
        p4->UV[3][0] = tx;
        p4->UV[3][1] = ty + theight;
        break;
    case 2: // flip y
        p4->UV[0][0] = tx;
        p4->UV[0][1] = ty + 31;
        p4->UV[1][0] = tx + 31;
        p4->UV[1][1] = ty + theight;
        p4->UV[2][0] = tx;
        p4->UV[2][1] = ty;
        p4->UV[3][0] = tx + 31;
        p4->UV[3][1] = ty;
        break;
    case 3: // flip x+y
        p4->UV[0][0] = tx + 31;
        p4->UV[0][1] = ty + 31;
        p4->UV[1][0] = tx;
        p4->UV[1][1] = ty + 31;
        p4->UV[2][0] = tx + theight;
        p4->UV[2][1] = ty;
        p4->UV[3][0] = tx;
        p4->UV[3][1] = ty;
        break;
    }

    /*
            p4->UV[0][0]=tx;
            p4->UV[0][1]=ty;
            p4->UV[1][0]=tx+31;
            p4->UV[1][1]=ty;
            p4->UV[2][0]=tx;
            p4->UV[2][1]=ty+31;
            p4->UV[3][0]=tx+31;
            p4->UV[3][1]=ty+31;
    */
    /*
    if(global_overflow)
    {
            p4=0;
            next_prim_face4--;
    }
    */
    return (p4);
}

struct PrimFace4* create_a_quad_tex(UWORD p1, UWORD p0, UWORD p3, UWORD p2, UWORD texture, SLONG flipx = 0)
{
    struct PrimFace4* p4;
    SLONG tx, ty;
    SLONG flip;
    SLONG page_to;

    p4 = &prim_faces4[next_prim_face4];
    next_prim_face4++;

    p4->Points[0] = p0;
    p4->Points[1] = p1;
    p4->Points[2] = p2;
    p4->Points[3] = p3;

    p4->DrawFlags = POLY_GT;
    p4->FaceFlags = 0;

    tx = (texture & 7) << 5;
    ty = ((texture >> 3) & 7) << 5;
    flip = (texture & 0x80) >> 7;
    p4->TexturePage = (texture & 0x7f) >> 6;
    add_page_countxy(tx >> 5, ty >> 5, p4->TexturePage);

    p4->DrawFlags = POLY_GT;
    if (flipx)
        flip ^= 1;

    if (flip) {
        p4->UV[1][0] = tx;
        p4->UV[1][1] = ty;
        p4->UV[0][0] = tx + 31;
        p4->UV[0][1] = ty;
        p4->UV[3][0] = tx;
        p4->UV[3][1] = ty + 31;
        p4->UV[2][0] = tx + 31;
        p4->UV[2][1] = ty + 31;

    } else {
        p4->UV[0][0] = tx;
        p4->UV[0][1] = ty;
        p4->UV[1][0] = tx + 31;
        p4->UV[1][1] = ty;
        p4->UV[2][0] = tx;
        p4->UV[2][1] = ty + 31;
        p4->UV[3][0] = tx + 31;
        p4->UV[3][1] = ty + 31;
    }
    /*
            if(global_overflow)
            {
                    p4=0;
                    next_prim_face4--;
            }
    */
    return (p4);
}

struct PrimFace3* create_a_tri(UWORD p2, UWORD p1, UWORD p0, SWORD texture_id, SWORD texture_piece)
{
    struct PrimFace3* p3;
    SLONG tx, ty;
    texture_id = texture_id;
    p3 = &prim_faces3[next_prim_face3];
    next_prim_face3++;

    p3->Points[0] = p0;
    p3->Points[1] = p1;
    p3->Points[2] = p2;
    /*
            p3->Bright[0]=128;
            p3->Bright[1]=128;
            p3->Bright[2]=128;
    */

    p3->DrawFlags = POLY_GT;

    tx = texture_xy2[texture_piece].Tx;
    ty = texture_xy2[texture_piece].Ty;
    p3->UV[0][0] = tx;
    p3->UV[0][1] = ty;
    p3->UV[1][0] = tx + 31;
    p3->UV[1][1] = ty;
    p3->UV[2][0] = tx;
    p3->UV[2][1] = ty + 31;

    p3->TexturePage = texture_xy2[texture_piece].Page;
    ASSERT(p3->TexturePage < 15);
    /*
            if(global_overflow)
            {
                    p3=0;
                    next_prim_face3--;
            }
    */
    return (p3);
}

void set_texture_fe(struct PrimFace4* p4, SLONG xw, SLONG xh, SLONG type)
{
    SLONG tx, ty;
    switch (type) {
    case 0:
        tx = 0;
        ty = 6 * 32;
        break;
    case 1:
        tx = 5 * 32;
        ty = 4 * 32;
        break;
    }

    xw = 1;
    xh = 1;

    p4->UV[0][0] = tx;
    p4->UV[0][1] = ty;
    p4->UV[1][0] = tx + 32 * xw;
    p4->UV[1][1] = ty;
    p4->UV[2][0] = tx;
    p4->UV[2][1] = ty + 32 * xh;
    p4->UV[3][0] = tx + 32 * xw;
    p4->UV[3][1] = ty + 32 * xh;
    p4->TexturePage = 1;
}

//     0--------------------------1
//
//
//	   4		6		  7		  5
//
//
//	   3	    8		  9		  2
//

#define FE_FIRST_SLOPE 1
#define FE_PLINTH1 2
#define FE_WALKWAY1 3
#define FE_PLINTH2 4
#define FE_SLOPE2 5

#define FE_FIRST_SLOPE_RAIL -6
#define FE_PLINTH1_RAIL_A -7
#define FE_PLINTH1_RAIL_B -8
#define FE_WALKWAY1_RAIL -9
#define FE_PLINTH2_RAIL_A -10
#define FE_PLINTH2_RAIL_B -11
#define FE_SLOPE2_RAIL -12

SWORD face_offsets[] = {
    0,
    -1, 0, // 1
    1, -2, 0, // 3
    2, 1, 0, // 6
    -1, 12, 0, // 9
    -12, -1, 0 // 12

};

UWORD id_offset[] = {
    0, 1, 3, 6, 9, 12
};
#define FACE_TYPE_FIRE_ESCAPE (1 << 0)

SLONG next_connected_face(SLONG type, SLONG id, SLONG count)
{
    switch (type) {
    case FACE_TYPE_FIRE_ESCAPE:
        SLONG start;

        start = id_offset[id];
        //			LogText(" id %d start %d count %d \n",id,start,count);
        return (face_offsets[start + count]);
        break;
    }
    return (0);
}
// claude-ai: build_firescape: Generates the geometry for an exterior fire escape staircase.
// claude-ai: Iterates storey_list[storey].Height times (one level per iteration).
// claude-ai: Each iteration creates: walkway platforms (FE_WALKWAY1), plinths (FE_PLINTH1/2),
//   sloped ramp faces (FE_FIRST_SLOPE, FE_SLOPE2), and banisters (railing quads).
// claude-ai: Faces use POLY_T|POLY_FLAG_DOUBLESIDED|POLY_FLAG_MASKED (transparent rails).
// claude-ai: SORT_LEVEL_FIRE_ESCAPE=3: render sort priority for these faces.
// claude-ai: Walkable faces are registered via add_quad_to_walkable_list().
// claude-ai: PORT NOTE: Fire escape walkway geometry + walkability needs porting. Collision vects are stubs here.
void build_firescape(SLONG storey)
{
    SLONG y = 0;
    SLONG count = 0;
    //	SLONG	sp[120];
    struct PrimFace4* p4;
    SLONG wall;

    wall = -storey_list[storey].WallHead;

    while (count < storey_list[storey].Height) {

        start_point[count] = next_prim_point;
        if (count == 0) {
            build_fire_escape_points(storey, y, 1);
            build_fire_escape_points(storey, y + BLOCK_SIZE, 1);
        } else {
            build_fire_escape_points(storey, y, 0);
            build_fire_escape_points(storey, y + BLOCK_SIZE, 0);
        }

        if (count > 0) {
            // banisters
            p4 = create_a_quad(start_point[count] + 3 + 10, start_point[count] + 0 + 10, start_point[count] + 3, start_point[count] + 4, 0, 0);
            set_texture_fe(p4, 1, 1, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            p4 = create_a_quad(start_point[count] + 8 + 10, start_point[count] + 3 + 10, start_point[count] + 8, start_point[count] + 3, 0, 0);
            set_texture_fe(p4, 1, 1, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            p4 = create_a_quad(start_point[count] + 2 + 10, start_point[count] + 9 + 10, start_point[count] + 2, start_point[count] + 9, 0, 0);
            set_texture_fe(p4, 1, 1, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            p4 = create_a_quad(start_point[count] + 1 + 10, start_point[count] + 2 + 10, start_point[count] + 1, start_point[count] + 2, 0, 0);
            set_texture_fe(p4, 1, 1, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            p4 = create_a_quad(start_point[count] + 7 + 10, start_point[count] + 6 + 10, start_point[count] + 7, start_point[count] + 6, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);

            // floors
            p4 = create_a_quad(start_point[count], start_point[count] + 1, start_point[count] + 4, start_point[count] + 5, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_WALKWAY1;
            add_quad_to_walkable_list(next_prim_face4 - 1);

            p4 = create_a_quad(start_point[count] + 4, start_point[count] + 6, start_point[count] + 3, start_point[count] + 8, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_PLINTH2;
            add_quad_to_walkable_list(next_prim_face4 - 1);

            p4 = create_a_quad(start_point[count] + 7, start_point[count] + 5, start_point[count] + 9, start_point[count] + 2, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_PLINTH1;
            add_quad_to_walkable_list(next_prim_face4 - 1);
        }

        if (count == 1) {
            // first slope
            // floor
            insert_collision_vect(prim_points[start_point[count - 1]].X, prim_points[start_point[count - 1]].Y, prim_points[start_point[count - 1]].Z,
                prim_points[start_point[count - 1] + 1].X, prim_points[start_point[count - 1] + 1].Y, prim_points[start_point[count - 1] + 1].Z, 0, 0, next_prim_face4);

            p4 = create_a_quad(start_point[count - 1], start_point[count] + 7, start_point[count - 1] + 1, start_point[count] + 9, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->ThingIndex = wall;
            p4->FaceFlags |= FACE_FLAG_WALKABLE;
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_FIRST_SLOPE;
            add_quad_to_walkable_list(next_prim_face4 - 1);

            // bannister
            p4 = create_a_quad(start_point[count] + 9 + 10, start_point[count - 1] + 3, start_point[count] + 9, start_point[count - 1] + 1, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_FIRST_SLOPE_RAIL;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        } else if (count > 1) {
            // continue slope
            p4 = create_a_quad(start_point[count - 1] + 6, start_point[count] + 7, start_point[count - 1] + 8, start_point[count] + 9, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED; //|POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_SLOPE2;
            add_quad_to_walkable_list(next_prim_face4 - 1);

            // rail
            p4 = create_a_quad(start_point[count] + 9 + 10, start_point[count - 1] + 8 + 10, start_point[count] + 9, start_point[count - 1] + 8, 0, 0);
            set_texture_fe(p4, 1, 1, 1);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED | POLY_FLAG_MASKED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4->Type = FACE_TYPE_FIRE_ESCAPE;
            p4->ID = FE_SLOPE2_RAIL;
        }

        count++;
        y += BLOCK_SIZE * 4;
    }
}

//
//
//
//

#define LADDER_SPINE_WIDTH 12
void build_ladder_points(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG y, SLONG flag)
{
    SLONG dx, dz;

    dx = x2 - x1;
    dz = z2 - z1;

    if (dx > 0)
        dx = LADDER_SPINE_WIDTH;
    else if (dx < 0)
        dx = -LADDER_SPINE_WIDTH;

    if (dz > 0)
        dz = LADDER_SPINE_WIDTH;
    else if (dz < 0)
        dz = -LADDER_SPINE_WIDTH;

    if (flag == 1) {
        add_point(x1 - dz, y, z1 + dx);
        add_point(x1, y, z1);
        add_point(x1 + dx, y, z1 + dz);
        add_point(x2 - dx, y, z2 - dz);
        add_point(x2, y, z2);
        add_point(x2 - dz, y, z2 + dx);

    } else {
        dx = (dx * 3) >> 2;
        dz = (dz * 3) >> 2;
        add_point(x1 + dx, y, z1 + dz);
        add_point(x2 - dx, y, z2 - dz);
    }
}

void calc_ladder_ends(SLONG* x1, SLONG* z1, SLONG* x2, SLONG* z2);

void calc_ladder_pos(SLONG* x1, SLONG* z1, SLONG* x2, SLONG* z2, SLONG* y, SLONG* extra_height)
{
    SLONG dx, dz;
    *extra_height = 0;

    calc_ladder_ends(x1, z1, x2, z2);

    if (*y == 0) {
        SLONG min_y, ty;

        min_y = PAP_calc_height_at(*x1, *z1);
        ty = PAP_calc_height_at(*x2, *z2);

        if (ty < min_y)
            min_y = ty;

        *y = min_y;
        *extra_height = abs(min_y) >> 6;
    } else
        *y += build_max_y;

    //	*y+=calc_height_at(*x1,*z1);
}

// 0   1   2   3
//
// 9           4
//
// 8   7   6   5

// w=4 h=3

#define MAX_SIZE 20
SLONG sp[MAX_SIZE][MAX_SIZE];
SLONG xp[MAX_SIZE], zp[MAX_SIZE];

SLONG flat_fill_a_quad_of_points(SLONG start_point, SLONG w, SLONG h, SLONG texture_style, SLONG wall)
{
    SLONG ax, az;
    SLONG y;
    SLONG c0;
    SLONG texture = TEXTURE_PIECE_RIGHT;
    struct PrimFace4* p_f4;

    y = prim_points[start_point].Y;

    for (c0 = 0; c0 < w; c0++) {
        sp[c0][0] = start_point + c0;
        sp[c0][h - 1] = start_point + w + h - 2 + w - c0 - 1;

        xp[c0] = prim_points[start_point + c0].X;
        //		LogText(" fill edge top[%d]=%d  bot[]=%d \n",c0,c0,w+h-2+w-c0-1);
    }

    zp[0] = prim_points[start_point].Z;
    for (c0 = 1; c0 < h - 1; c0++) {
        sp[w - 1][c0] = start_point + w + c0 - 1; // rhs
        sp[0][c0] = start_point + w + w + h - 2 + h - c0 - 2; // lhs

        zp[c0] = prim_points[start_point + w - 1 + c0].Z;
        //		LogText(" fill edge left[%d]=%d  right[]=%d \n",c0,w+w+h-2+h-c0-2,w+c0-1);
    }

    for (ax = 1; ax < w - 1; ax++)
        for (az = 1; az < h - 1; az++) {
            sp[ax][az] = next_prim_point;
            add_point(xp[ax], y, zp[az]);
            //		LogText(" mid point at %d %d \n",ax,az);
        }

    for (az = 0; az < h - 1; az++) {
        for (ax = 0; ax < w - 1; ax++) {
            //			create_a_quad(sp[ax][az],sp[ax+1][az],sp[ax][az+1],sp[ax+1][az+1],0,18);
            p_f4 = create_a_quad(sp[ax][az + 1], sp[ax + 1][az + 1], sp[ax][az], sp[ax + 1][az], texture_style, texture);
            p_f4->ThingIndex = -wall;
            p_f4->Type = FACE_TYPE_SKYLIGHT;
            p_f4->ID = 0;
            add_quad_to_walkable_list(next_prim_face4 - 1);
        }
    }
    return (1);
}

SLONG sx[200], sz[200];
SLONG numb[200];

// claude-ai: build_skylight: Generates geometry for a STOREY_TYPE_SKYLIGHT — a raised rim around a roof hole.
// claude-ai: Creates two rings of points: one at roof level (y) and one raised+inset (y+up, in by 'in' units).
// claude-ai: Outer ring: follows wall list, strip-subdivided for matching with adjacent wall faces.
// claude-ai: Inner ring: build_outline() generates the inset polygon vertices.
// claude-ai: Side faces: quads between inner and outer rings.
// claude-ai: Top: flat_fill_a_quad_of_points() fills the inner rectangle.
// claude-ai: All faces are registered with FACE_TYPE_SKYLIGHT and added to walkable list.
// claude-ai: Returns y+up (the new elevated top surface height).
SLONG build_skylight(SLONG storey)
{
    SLONG count = 0;
    SLONG index, c0;
    SLONG dx, dz;
    SLONG px, pz, rx, rz;
    SLONG x, y, z, wall;
    SLONG ox, oz;
    SLONG pcount;
    SLONG in = -50;
    SLONG up = 50;
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;

    //
    // Create the same number of points inside but raised then polygonize the outside
    //

    // need to fill in strange shape inside
    // or can I assume its a quad

    x = storey_list[storey].DX;
    y = storey_list[storey].DY;
    z = storey_list[storey].DZ;

    y += build_max_y;

    wall = storey_list[storey].WallHead;

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;
    texture = TEXTURE_PIECE_MIDDLE;

    //
    // Build points at roof height
    //
    start_point[0] = next_prim_point;
    ox = x;
    oz = z;
    index = wall;
    count = 0;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        numb[count] = create_strip_points(ox, y, oz, dx, y, dz, 256, 0, 0); // no end point
        count++;
        index = wall_list[index].Next;
        ox = dx;
        oz = dz;
    }

    //
    // Build points above roof and in a bit
    //

    pcount = build_outline(&sx[0], &sz[0], storey, wall, y + up, in);

    start_point[1] = next_prim_point;
    count = 0;
    for (c0 = 0; c0 < pcount; c0++) {
        create_strip_points(sx[c0], y + up, sz[c0], sx[c0 + 1], y + up, sz[c0 + 1], 256, numb[count], 0);
        count++;
    }
    count = start_point[1] - start_point[0];

    for (c0 = 0; c0 < count - 1; c0++) {
        p_f4 = create_a_quad(start_point[1] + c0, start_point[1] + c0 + 1, start_point[0] + c0, start_point[0] + c0 + 1, texture_style, texture);
        p_f4->ThingIndex = -wall;
        p_f4->Type = FACE_TYPE_SKYLIGHT;
        p_f4->ID = 0;
        add_quad_to_walkable_list(next_prim_face4 - 1);
    }
    p_f4 = create_a_quad(start_point[1] + c0, start_point[1], start_point[0] + c0, start_point[0], texture_style, texture);
    p_f4->ThingIndex = -wall;
    p_f4->Type = FACE_TYPE_SKYLIGHT;
    p_f4->ID = 0;
    add_quad_to_walkable_list(next_prim_face4 - 1);

    flat_fill_a_quad_of_points(start_point[1], numb[0] + 1, numb[1] + 1, texture_style, wall);

    return (y + up);
}

// claude-ai: build_ladder: Generates ladder geometry and collision for STOREY_TYPE_LADDER storeys.
// claude-ai: Uses calc_ladder_pos() to find x1,z1,x2,z2 (ladder endpoints) and y (base height).
// claude-ai: Registers three collision vectors: the ladder itself + two side vects to prevent squeezing.
//   - STOREY_TYPE_LADDER with extra=storey height: the climbable surface
//   - STOREY_TYPE_LADDER with extra=0: the solid walls beside the ladder
// claude-ai: Visual geometry: vertical strips (build_ladder_points flag=1 = 6-point profile),
//   horizontal rungs (build_ladder_points flag=0 = 2-point profile, one rung per BLOCK_SIZE).
// claude-ai: build_ladder_old: Unused older version (BLOCK_SIZE*4 spacing instead of count-based).
// claude-ai: PORT NOTE: Ladder collision vectors (STOREY_TYPE_LADDER) need proper implementation.
void build_ladder(SLONG storey)
{
    SLONG y = 0, c0;
    SLONG count = 0;
    //	UWORD	sp[120]; //,spr[10];
    struct PrimFace4* p4;
    SLONG wall;
    SLONG extra_height;

    SLONG x1, z1, x2, z2;
    SLONG wx1, wz1, wx2, wz2;
    SLONG texture_style;

    wall = storey_list[storey].WallHead;
    texture_style = wall_list[wall].TextureStyle;

    wx1 = x1 = storey_list[storey].DX;
    wz1 = z1 = storey_list[storey].DZ;

    wx2 = x2 = wall_list[wall].DX;
    wz2 = z2 = wall_list[wall].DZ;

    wall = -wall;
    y = storey_list[storey].DY;
    calc_ladder_pos(&x1, &z1, &x2, &z2, &y, &extra_height);

    insert_collision_vect(x1, y, z1, x2, y, z2, STOREY_TYPE_LADDER, storey_list[storey].Height, wall);

    //
    // These extra colvects stop the player from squeezing between the
    // wall and the ladder.
    //

    insert_collision_vect(
        wx1, y, wz1,
        x1, y, z1,
        STOREY_TYPE_LADDER,
        storey_list[storey].Height,
        wall);

    insert_collision_vect(
        x2, y, z2,
        wx2, y, wz2,
        STOREY_TYPE_LADDER,
        storey_list[storey].Height,
        wall);

    //
    // These extra colvects stop the player from squeezing between the
    // wall and the ladder.
    //

    insert_collision_vect(
        wx1, y, wz1,
        x1, y, z1,
        STOREY_TYPE_LADDER,
        0,
        wall);

    insert_collision_vect(
        x2, y, z2,
        wx2, y, wz2,
        STOREY_TYPE_LADDER,
        0,
        wall);

    //
    // build edges
    //

    //   plan view
    //
    //	 0			 5
    //   1 2       3 4
    {
        SLONG height, size, count;

        //		height=(storey_list[storey].Height+extra_height)*64;
        height = (storey_list[storey].Height) * 64;

        count = height >> 8;
        if (count == 0) {
            count += 1;
        }
        size = height / count;

        start_point[0] = next_prim_point;
        build_ladder_points(x1, z1, x2, z2, y, 1);

        for (c0 = 1; c0 <= count; c0++) {
            start_point[c0] = next_prim_point;
            build_ladder_points(x1, z1, x2, z2, y + size * c0, 1);

            p4 = create_a_quad(start_point[c0] + 0, start_point[c0] + 1, start_point[c0 - 1] + 0 + 0, start_point[c0 - 1] + 1 + 0, texture_style, TEXTURE_PIECE_MIDDLE);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[c0] + 1, start_point[c0] + 2, start_point[c0 - 1] + 1 + 0, start_point[c0 - 1] + 2 + 0, texture_style, TEXTURE_PIECE_MIDDLE);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[c0] + 3, start_point[c0] + 4, start_point[c0 - 1] + 3 + 0, start_point[c0 - 1] + 4 + 0, texture_style, TEXTURE_PIECE_MIDDLE);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[c0] + 4, start_point[c0] + 5, start_point[c0 - 1] + 4 + 0, start_point[c0 - 1] + 5 + 0, texture_style, TEXTURE_PIECE_MIDDLE);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }
    }

    //
    // now create rungs
    //

    for (c0 = 0; c0 < storey_list[storey].Height; c0++) {
        SLONG spr;
        spr = next_prim_point;

        build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * (c0 + 1) - 8, 0);
        build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * (c0 + 1), 0);

        p4 = create_a_quad(spr + 2, spr + 2 + 1, spr + 0, spr + 1, texture_style, TEXTURE_PIECE_LEFT);
        p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
        p4->ThingIndex = wall;
        OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
    }
}
void build_ladder_old(SLONG storey)
{
    SLONG y = 0, c0;
    SLONG count = 0;
    //	UWORD	sp[120]; //,spr[10];
    struct PrimFace4* p4;
    SLONG wall;
    SLONG extra_height;

    SLONG x1, z1, x2, z2;

    wall = storey_list[storey].WallHead;

    x1 = storey_list[storey].DX;
    z1 = storey_list[storey].DZ;

    x2 = wall_list[wall].DX;
    z2 = wall_list[wall].DZ;

    wall = -wall;
    y = storey_list[storey].DY;
    calc_ladder_pos(&x1, &z1, &x2, &z2, &y, &extra_height);

    insert_collision_vect(x1, y, z1, x2, y, z2, STOREY_TYPE_LADDER, 0, wall);
    //	prim_points[start_point[count-1]].X,prim_points[start_point[count-1]].Y,prim_points[start_point[count-1]].Z,
    //									prim_points[start_point[count-1]+1].X,prim_points[start_point[count-1]+1].Y,prim_points[start_point[count-1]+1].Z,0,0,next_prim_face4);

    while (count < (storey_list[storey].Height)) {

        start_point[count] = next_prim_point;
        if (count == 0) {
            //   plan view
            //
            //	 0			 5
            //   1 2       3 4
            build_ladder_points(x1, z1, x2, z2, y, 1);
            build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * 4, 1);
        } else {
            build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * 4, 1);
        }

        if (count == 0) {
            p4 = create_a_quad(start_point[count] + 0 + 6, start_point[count] + 1 + 6, start_point[count] + 0 + 0, start_point[count] + 1 + 0, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 1 + 6, start_point[count] + 2 + 6, start_point[count] + 1 + 0, start_point[count] + 2 + 0, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 3 + 6, start_point[count] + 4 + 6, start_point[count] + 3 + 0, start_point[count] + 4 + 0, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 4 + 6, start_point[count] + 5 + 6, start_point[count] + 4 + 0, start_point[count] + 5 + 0, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }
        if (count) {
            p4 = create_a_quad(start_point[count] + 0, start_point[count] + 1, start_point[count - 1] + 0, start_point[count - 1] + 1, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 1, start_point[count] + 2, start_point[count - 1] + 1, start_point[count - 1] + 2, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 3, start_point[count] + 4, start_point[count - 1] + 3, start_point[count - 1] + 4, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(start_point[count] + 4, start_point[count] + 5, start_point[count - 1] + 4, start_point[count - 1] + 5, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }

        //
        // now create rungs
        //

        for (c0 = 0; c0 < 4; c0++) {
            SLONG spr;
            spr = next_prim_point;

            build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * (c0 + 1) - 8, 0);
            build_ladder_points(x1, z1, x2, z2, y + BLOCK_SIZE * (c0 + 1), 0);

            p4 = create_a_quad(spr + 2, spr + 2 + 1, spr + 0, spr + 1, 0, 0);
            p4->DrawFlags = POLY_T | POLY_FLAG_DOUBLESIDED;
            p4->ThingIndex = wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }

        // floor
        //			insert_collision_vect(prim_points[start_point[count-1]].X,prim_points[start_point[count-1]].Y,prim_points[start_point[count-1]].Z,
        //									prim_points[start_point[count-1]+1].X,prim_points[start_point[count-1]+1].Y,prim_points[start_point[count-1]+1].Z,0,0,next_prim_face4);

        count++;
        y += BLOCK_SIZE * 4;
    }
}

//   x1->x2->x3

// see diag 5.2 page 164 of van dam 1

SLONG calc_sin_from_cos(SLONG sin)
{
    SLONG cos;

    cos = (Root((1 << 14) - ((sin * sin) >> 14)));
    cos = cos << 7;
    return (cos);
}

void calc_new_corner_point(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG x3, SLONG z3, SLONG width, SLONG* res_x, SLONG* res_z)
{

    SLONG vx, vz, dist;
    SLONG wx, wz;
    SLONG ax, az;
    SLONG angle;

    SLONG z;

    //	LogText(" x1 %d z1 %d x2 %d z2 %d x3 %d z3 %d \n",x1,z1,x2,z2,x3,z3);

    vx = x2 - x1;
    vz = z2 - z1;

    wx = -(x3 - x2);
    wz = -(z3 - z2);

    z = vx * wz - vz * wx;
    //	LogText(" z= %d \n",z);

    dist = Root(SDIST2(vx, vz));
    //	LogText(" V (%d,%d) dist %d \n",vx,vz,dist);
    if (dist == 0)
        return;

    vx = (vx << 14) / dist;
    vz = (vz << 14) / dist;

    dist = Root(SDIST2(wx, wz));
    //	LogText(" W (%d,%d) dist %d \n",wx,wz,dist);
    if (dist == 0)
        return;

    wx = (wx << 14) / dist;
    wz = (wz << 14) / dist;

    ax = (vx + wx);
    az = (vz + wz);

    dist = Root(SDIST2(ax, az));
    //	LogText(" A (%d,%d) dist %d\n",ax,az,dist);
    if (dist == 0) {
        // u & V cancel each other out
        *res_x = ((vz * width) >> 14) + x2;
        *res_z = -((vx * width) >> 14) + z2;

        return;
    }

    ax = (ax << 14) / dist;
    az = (az << 14) / dist;

    //	LogText(" normalised vectors V(%d,%d) W(%d,%d) A(%d,%d)\n",vx,vz,wx,wz,ax,az);

    angle = (vx * ax + vz * az) >> 14;
    //	LogText(" cos angle = %d \n",angle);
    // we now have the cos of the angle
    angle = calc_sin_from_cos(angle); //<<2;
    // LogText(" sin angle(<<14) = %d \n",angle);

    // we now have the sin of the angle

    // sin@=o/h

    if (angle == 0)
        dist = width;
    else {
        dist = (width << 14) / angle;
    }
    //	LogText(" dist= %d \n",dist);

    if (z > 0) {
        dist = -dist;
    }

    *res_x = ((ax * dist) >> 14) + x2;
    *res_z = ((az * dist) >> 14) + z2;

    //	LogText("  resx %d resz %d \n",(ax*dist)>>14,(az*dist)>>14);
}

void build_ledge(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height)
{
    //	SLONG	sp[10];
    SLONG count = 0;
    SLONG index, c0;
    SLONG dx, dz;
    SLONG px, pz, rx, rz;

    storey = storey;
    start_point[0] = next_prim_point;
    add_point(x, y, z);
    index = wall;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        add_point(dx, y, dz);
        index = wall_list[index].Next;
        count++;
    }
    start_point[1] = next_prim_point;
    add_point(x, y, z);

    px = x;
    pz = z;
    index = wall;
    while (index) {
        SLONG next;
        //		LogText(" calc corner %d \n",index);
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        next = wall_list[index].Next;
        if (next) {
            calc_new_corner_point(px, pz, dx, dz, wall_list[next].DX, wall_list[next].DZ, BLOCK_SIZE, &rx, &rz);
            add_point(rx, y, rz);
        } else {
            calc_new_corner_point(px, pz, dx, dz, wall_list[wall].DX, wall_list[wall].DZ, BLOCK_SIZE, &rx, &rz);
            add_point(rx, y, rz);
            prim_points[start_point[1]].X = rx;
            prim_points[start_point[1]].Z = rz;
        }

        px = dx;
        pz = dz;
        index = wall_list[index].Next;
    }
    //	add_point(prim_points[start_point[1]].X,y,prim_points[start_point[1]].Z);

    y += height;
    start_point[2] = next_prim_point;
    for (c0 = start_point[1]; c0 < start_point[2]; c0++) {
        prim_points[next_prim_point].X = prim_points[c0].X;
        prim_points[next_prim_point].Y = y + 2;
        prim_points[next_prim_point].Z = prim_points[c0].Z;
        next_prim_point++;
    }

    start_point[3] = next_prim_point;
    add_point(x, y, z);
    index = wall;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        add_point(dx, y + 2, dz);
        index = wall_list[index].Next;
    }

    //	LogText("ledge start_point %d %d %d %d next p point %d count %d \n",start_point[0],start_point[1],start_point[2],start_point[3],next_prim_point,count);
    for (c0 = 0; c0 <= count; c0++) {
        create_a_quad(start_point[1] + c0, start_point[1] + c0 + 1, start_point[0] + c0, start_point[0] + c0 + 1, 0, 18);
        //		LogText(" create a face with points %d %d %d %d \n",start_point[1]+c0,start_point[1]+c0+1,start_point[0]+c0,start_point[0]+c0+1);
        create_a_quad(start_point[2] + c0, start_point[2] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, 0, 18);
        create_a_quad(start_point[3] + c0, start_point[3] + c0 + 1, start_point[2] + c0, start_point[2] + c0 + 1, 0, 18);
    }
}

SLONG create_strip_points(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG len, SLONG numb, SLONG end_flag)
{
    SLONG count;
    SLONG dist;
    SLONG dx, dy, dz;
    SLONG wwidth, wcount;

    dx = x2 - x1;
    dz = z2 - z1;

    dist = Root(dx * dx + dz * dz);

    if (dist == 0)
        dist = 1;

    if (numb)
        count = numb;
    else
        count = dist / len;

    wcount = count;

    wwidth = dist / (wcount);

    dx = (x2 - x1);
    dz = (z2 - z1);

    dx = (dx << 10) / dist;
    dz = (dz << 10) / dist;

    add_point(x1, y1, z1);
    count--;

    //	LogText(" create strip points y %d (%d,%d)->(%d,%d) count %d \n",y1,x1,z1,x2,z2,count+1);

    while (count) {

        x1 = x1 + ((dx * wwidth) >> 10);
        z1 = z1 + ((dz * wwidth) >> 10);
        add_point(x1, y1, z1);

        count--;
    }
    if (end_flag)
        add_point(x2, y1, z2); // make sure last point is spot on.
    return (wcount);
}

SLONG build_outline(SLONG* sx, SLONG* sz, SLONG storey, SLONG wall, SLONG y, SLONG out)
{
    SLONG offset;
    SLONG px;
    SLONG pz;
    SLONG index;
    SLONG dx, dz;
    SLONG rx, rz;

    offset = 1;

    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;

    index = wall;
    while (index) {
        SLONG next;
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        next = wall_list[index].Next;
        if (next) {
            calc_new_corner_point(px, pz, dx, dz, wall_list[next].DX, wall_list[next].DZ, out, &rx, &rz);
            sx[offset] = rx;
            sz[offset] = rz;
        } else {
            calc_new_corner_point(px, pz, dx, dz, wall_list[wall].DX, wall_list[wall].DZ, out, &rx, &rz);
            sx[0] = rx;
            sz[0] = rz;
            sx[offset] = rx;
            sz[offset] = rz;
        }
        //		LogText("build outline offset %d xz %d %d \n",offset,sx[offset],sz[offset]);

        px = dx;
        pz = dz;
        index = wall_list[index].Next;
        //		index=0;
        offset++;
    }
    return (offset - 1);
}

SLONG ladder_on_block(SLONG p0, SLONG p1, SLONG p2, SLONG p3)
{
    SLONG ax, az;
    SLONG flags;

    ax = prim_points[p0].X;
    ax += prim_points[p1].X;
    ax += prim_points[p2].X;
    ax += prim_points[p3].X;

    az = prim_points[p0].Z;
    az += prim_points[p1].Z;
    az += prim_points[p2].Z;
    az += prim_points[p3].Z;

    ax >>= 2;
    az >>= 2;

    flags = get_map_flags(ax >> PAP_SHIFT_HI, az >> PAP_SHIFT_HI);

    if (flags & FLOOR_LADDER)
        return (1);
    else
        return (0);
}

// p2		   p3
//
//
//   p0      p1
/*
void	build_ledge_around_ladder(SLONG	p0,SLONG p1,SLONG p2,SLONG p3)
{
        SLONG	x1,z1,x2,z2,dx,dz;

        x1=prim_points[p0].X;
        z1=prim_points[p0].Z;

        x2=prim_points[p1].X;
        z2=prim_points[p1].Z;

        dx=x2-x1;
        dz=z2-z1;


        x1+=dx/3;
        z1+=dz/3;

        x2-=dx/3;
        z2-=dz/3;

        x1+=dz>>3;
        x2+=dz>>3;

        z1-=dx>>3;
        z2-=dx>>3;





}
  */
SLONG sx_l2[30], sz_l2[30];
SLONG build_ledge2(SLONG y, SLONG storey, SLONG out, SLONG height, SLONG dip)
{
    // SLONG	sp[10];
    SLONG count = 0;
    SLONG index, c0;
    SLONG dx, dz;
    SLONG px, pz, rx, rz;
    SLONG x, z, wall;
    SLONG ox, oz;
    SLONG pcount;
    //	SLONG	numb[200];

    x = storey_list[storey].DX;
    z = storey_list[storey].DZ;
    wall = storey_list[storey].WallHead;

    storey = storey;

    //
    // Build points round top of building
    //
    start_point[0] = next_prim_point;
    ox = x;
    oz = z;
    index = wall;
    count = 0;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        numb[count] = create_strip_points(ox, y, oz, dx, y, dz, 256, 0, 1);
        count++;
        //		add_point(dx,y,dz);
        index = wall_list[index].Next;
        ox = dx;
        oz = dz;
    }

    //
    // Build points at top of building height that stick out
    //

    pcount = build_outline(&sx_l2[0], &sz_l2[0], storey, wall, y, out);

    start_point[1] = next_prim_point;
    count = 0;
    for (c0 = 0; c0 < pcount; c0++) {
        create_strip_points(sx_l2[c0], y, sz_l2[c0], sx_l2[c0 + 1], y, sz_l2[c0 + 1], 256, numb[count], 1);
        count++;
    }
    // create_strip_points(sx_l2[c0],y,sz_l2[c0],sx_l2[0],y,sz_l2[0],256);

    //
    // sticky out and up
    //
    start_point[2] = next_prim_point;

    pcount = build_outline(&sx_l2[0], &sz_l2[0], storey, wall, y + height, out);

    count = 0;
    for (c0 = 0; c0 < pcount; c0++) {
        create_strip_points(sx_l2[c0], y + height, sz_l2[c0], sx_l2[c0 + 1], y + height, sz_l2[c0 + 1], 256, numb[count], 1);
        count++;
    }
    //	create_strip_points(sx_l2[c0],y+height,sz_l2[c0],sx_l2[0],y+height,sz_l2[0],256);

    //
    // not sticky out but up
    //
    //	LogText(" build up & in \n");
    start_point[3] = next_prim_point;

    ox = x;
    oz = z;
    index = wall;
    count = 0;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        create_strip_points(ox, y + height, oz, dx, y + height, dz, 256, numb[count], 1);
        count++;
        index = wall_list[index].Next;
        ox = dx;
        oz = dz;
    }

    //	LogText(" build half up & in \n");
    start_point[4] = next_prim_point;

    ox = x;
    oz = z;
    index = wall;
    count = 0;
    while (index) {
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        create_strip_points(ox, y + height - dip, oz, dx, y + height - dip, dz, 256, numb[count], 1);
        count++;
        index = wall_list[index].Next;
        ox = dx;
        oz = dz;
    }

    count = start_point[1] - start_point[0];
    //	LogText("ledge start_point %d %d %d %d next p point %d count %d \n",start_point[0],start_point[1],start_point[2],start_point[3],next_prim_point,count);
    for (c0 = 0; c0 < count - 1; c0++) {
        SLONG p0, p1, p2, p3, p4;
        p0 = start_point[0] + c0;
        p1 = start_point[1] + c0;
        p2 = start_point[2] + c0;
        p3 = start_point[3] + c0;
        p4 = start_point[4] + c0;

        if (!ladder_on_block(p1, p1 + 1, p0, p0 + 1)) {
            create_a_quad(p1, p1 + 1, p0, p0 + 1, 0, 18);
            create_a_quad(p2, p2 + 1, p1, p1 + 1, 0, 18);
            create_a_quad(p3, p3 + 1, p2, p2 + 1, 0, 18);
            create_a_quad(p3 + 1, p3, p4 + 1, p4, 0, 18);
        } else {
            create_a_quad(p2, p3, p1, p0, 0, 18);
            create_a_quad(p3 + 1, p2 + 1, p0 + 1, p1 + 1, 0, 18);
            create_a_quad(p4, p4 + 1, p0, p0 + 1, 0, 18);
            //			build_ledge_around_ladder(p3,p3+1,p2,p2+1);
        }
    }
    return (y + height - dip);
}

#define RECESS_SIZE (32)
void append_recessed_wall_prim(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height)
{
    SLONG x2, y2, z2;
    SLONG dx, dz, len;
    //	SLONG	sp[10];
    SLONG texture, texture_style;
    SLONG texture_style2;
    struct PrimFace4* p_f4;
    SLONG c0;
    UBYTE* ptexture1;
    SLONG tcount1;
    UBYTE* ptexture2;
    SLONG tcount2;

    ptexture1 = wall_list[wall].Textures;
    tcount1 = wall_list[wall].Tcount;
    ptexture2 = wall_list[wall].Textures2;
    tcount2 = wall_list[wall].Tcount2;

    texture_style = wall_list[wall].TextureStyle;
    texture_style2 = wall_list[wall].TextureStyle2;
    if (texture_style == 0)
        texture_style = 1;
    if (texture_style2 == 0)
        texture_style2 = 1;

    x2 = wall_list[wall].DX;
    z2 = wall_list[wall].DZ;

    dx = x2 - x;
    dz = z2 - z;

    len = Root(dx * dx + dz * dz);
    if (len == 0)
        len = 1;

    dx = (dx * RECESS_SIZE) / len;
    dz = (dz * RECESS_SIZE) / len;

    start_point[0] = next_prim_point;
    add_point(x, y, z);
    add_point(x, y + height, z);

    add_point(x + dz, y, z - dx);
    add_point(x + dz, y + height, z - dx);

    start_point[1] = next_prim_point;
    add_point(x2, y, z2);
    add_point(x2, y + height, z2);

    add_point(x2 + dz, y, z2 - dx);
    add_point(x2 + dz, y + height, z2 - dx);

    texture = TEXTURE_PIECE_LEFT;

    texture_style = wall_list[wall].TextureStyle;

    if (texture_style == 0)
        texture_style = 1;

    p_f4 = create_a_quad(start_point[0] + 1, start_point[0] + 3, start_point[0] + 0, start_point[0] + 2, texture_style, texture); // left
    p_f4->ThingIndex = -wall;

    texture = TEXTURE_PIECE_RIGHT;
    p_f4 = create_a_quad(start_point[1] + 3, start_point[1] + 1, start_point[1] + 2, start_point[1] + 0, texture_style, texture); // right
    p_f4->ThingIndex = -wall;

    texture = TEXTURE_PIECE_MIDDLE2;
    p_f4 = create_a_quad(start_point[0], start_point[0] + 2, start_point[1] + 0, start_point[1] + 2, texture_style, texture); // floor
    p_f4->ThingIndex = -wall;

    texture = TEXTURE_PIECE_MIDDLE1;
    p_f4 = create_a_quad(start_point[0] + 3, start_point[0] + 1, start_point[1] + 3, start_point[1] + 1, texture_style, texture); // roof
    p_f4->ThingIndex = -wall;

    x += dz;
    z += -dx;

    x2 += dz;
    z2 += -dx;

    start_point[0] = build_row_wall_only_points_at_y(y + height + 2, x, z, x2, z2, wall);
    start_point[1] = build_row_wall_only_points_at_y(y, x, z, x2, z2, wall);
    //	for(c0=0;c0<wall_list[wall].WindowCount;c0++)
    for (c0 = 0; c0 < WindowCount; c0++) {
        texture = TEXTURE_PIECE_MIDDLE;
        if (c0 == 0) {
            texture = TEXTURE_PIECE_RIGHT;
        } else if (c0 == WindowCount - 1) {

            texture = TEXTURE_PIECE_LEFT;
        } else {
            texture = TEXTURE_PIECE_MIDDLE;
        }
        //		p_f4=create_a_quad(start_point[0]+c0,start_point[0]+c0+1,start_point[1]+c0,start_point[1]+c0+1,texture_style,texture);

        //		p_f4->ThingIndex=-wall;

        if (ptexture1 && (c0 < WindowCount) && ptexture1[c0]) {
            p_f4 = create_a_quad_tex(start_point[0] + c0, start_point[0] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, ptexture1[c0]);
            p_f4->ThingIndex = -wall;
        } else {
            //			if(ControlFlag)
            {
                p_f4 = create_a_quad(start_point[0] + c0, start_point[0] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, texture_style, texture);
                p_f4->ThingIndex = -wall;
            }
        }
        //			if(storey_list[storey].InsideIDIndex||storey_list[storey].InsideStorey||storey_list[storey].StoreyType==STOREY_TYPE_PARTITION)
        {
            if (ptexture2 && (c0 < tcount2) && ptexture2[c0]) {
                p_f4 = create_a_quad_tex(start_point[0] + c0 + 1, start_point[0] + c0, start_point[1] + c0 + 1, start_point[1] + c0, ptexture2[c0]);
                p_f4->ThingIndex = -wall;
            } else {
                p_f4 = create_a_quad(start_point[0] + c0 + 1, start_point[0] + c0, start_point[1] + c0 + 1, start_point[1] + c0, texture_style2, texture);
                p_f4->ThingIndex = -wall;
            }
            p_f4->FaceFlags |= FACE_FLAG_TEX2;
        }
    }
}

void append_foundation_wall(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height)
{
    SLONG c0;
    SLONG start_point[10];
    //	SLONG	sf4[10];
    SLONG texture, texture_style;
    struct PrimFace4* p_f4;
    UBYTE* ptexture;
    SLONG tcount, count;

    /*
            if(wall_list[wall].WallFlags&FLAG_WALL_RECESSED)
            {
                    append_recessed_wall_prim(x,y,z, wall,storey,height);
                    return;

            }
    */
    ptexture = wall_list[wall].Textures;
    tcount = wall_list[wall].Tcount;

    //	set_build_seed(x*z*storey*wall+x+z+y);
    set_build_seed(x * z + y);

    texture_style = wall_list[wall].TextureStyle;
    if (texture_style == 0)
        texture_style = 1;

    if (!(wall_list[wall].WallFlags & FLAG_WALL_AUTO_WINDOWS) && WindowCount == 0) {
        start_point[0] = build_row_wall_only_points_at_y(y + 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        start_point[1] = build_row_wall_only_points_at_floor_alt(0, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
        count = 0;
        for (c0 = 0; c0 < WindowCount; c0++) {
            texture = TEXTURE_PIECE_MIDDLE;
            if (c0 == 0) {
                //				if(storey_list[storey].Next==0)
                //					texture=TEXTURE_PIECE_TOP_RIGHT;
                //				else
                texture = TEXTURE_PIECE_RIGHT;
            } else if (c0 == WindowCount - 1) {

                //				if(storey_list[storey].Next==0)
                //					texture=TEXTURE_PIECE_TOP_LEFT;
                //				else
                texture = TEXTURE_PIECE_LEFT;
            } else {
                //				if(storey_list[storey].Next==0)
                //					texture=TEXTURE_PIECE_TOP_MIDDLE;
                //				else
                texture = TEXTURE_PIECE_MIDDLE;
            }
            if (ptexture && (count < tcount) && ptexture[count]) {
                p_f4 = create_a_quad_tex(start_point[0] + c0, start_point[0] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, ptexture[count]);
                p_f4->ThingIndex = -wall;
            } else {
                p_f4 = create_a_quad(start_point[0] + c0, start_point[0] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, texture_style, texture);
                p_f4->ThingIndex = -wall;
            }
            //
            // fix texture sizes
            //
            {
                SLONG dy;
                dy = prim_points[start_point[0] + c0 + 1].Y - prim_points[start_point[1] + c0 + 1].Y;
                if (dy > 256)
                    dy = 256;
                dy = (31 * dy) >> 8;
                p_f4->UV[2][1] = p_f4->UV[0][1] + dy;

                dy = prim_points[start_point[0] + c0].Y - prim_points[start_point[1] + c0].Y;
                if (dy > 256)
                    dy = 256;
                dy = (31 * dy) >> 8;
                p_f4->UV[3][1] = p_f4->UV[1][1] + dy;
            }
            count++;
        }
        WindowCount = 0;
    }
}

void append_wall_prim(SLONG x, SLONG y, SLONG z, SLONG wall, SLONG storey, SLONG height) //,UBYTE *ptexture,UWORD tcount)
{
    SLONG c0;
    SLONG start_point[10];
    //	SLONG	sf4[10];
    SLONG texture, texture_style, texture_style2;
    SLONG count;
    struct PrimFace4* p_f4;
    UBYTE* ptexture1;
    SLONG tcount1;
    UBYTE* ptexture2;
    SLONG tcount2;
    SLONG no_draw = 0;
    SLONG circular;

    circular = is_storey_circular(storey);

    if (next_prim_point > 50000)
        no_draw = 1;

    if (wall_list[wall].WallFlags & FLAG_WALL_RECESSED) {
        append_recessed_wall_prim(x, y, z, wall, storey, height);
        return;
    }
    WindowCount = 0;

    ptexture1 = wall_list[wall].Textures;
    tcount1 = wall_list[wall].Tcount;
    ptexture2 = wall_list[wall].Textures2;
    tcount2 = wall_list[wall].Tcount2;

    //	set_build_seed(x*z*storey*wall+x+z+y);
    set_build_seed(x * z + y);

    texture_style = wall_list[wall].TextureStyle;
    texture_style2 = wall_list[wall].TextureStyle2;
    if (texture_style == 0)
        texture_style = 1;
    if (texture_style2 == 0)
        texture_style2 = 1;

    if (!(wall_list[wall].WallFlags & FLAG_WALL_AUTO_WINDOWS) && WindowCount == 0) {
        //		if(next_prim_point>50000)
        if (!no_draw) {
            if (!circular) {
                start_point[0] = build_row_wall_only_points_at_floor_alt(y + height + 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
                start_point[1] = build_row_wall_only_points_at_floor_alt(y, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
            } else {
                start_point[0] = build_row_wall_only_points_at_y(y + height + 2, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
                start_point[1] = build_row_wall_only_points_at_y(y, x, z, wall_list[wall].DX, wall_list[wall].DZ, wall);
            }
        } else {
            start_point[0] = 0;
            start_point[1] = 0;
        }
        count = 0;
        for (c0 = 0; c0 < WindowCount; c0++) {
            texture = TEXTURE_PIECE_MIDDLE;
            if (c0 == 0) {
                //				if(storey_list[storey].Next==0)
                //					texture=TEXTURE_PIECE_TOP_RIGHT;
                //				else
                texture = TEXTURE_PIECE_RIGHT;
            } else if (c0 == WindowCount - 1) {

                //				if(storey_list[storey].Next==0)
                //					texture=TEXTURE_PIECE_TOP_LEFT;
                //				else
                texture = TEXTURE_PIECE_LEFT;
            } else {
                //				if(storey_list[storey].Next==0)
                //					texture=TEXTURE_PIECE_TOP_MIDDLE;
                //				else
                texture = TEXTURE_PIECE_MIDDLE;
            }

            //			if((edit_info.HideMap&1) && prim_points[start_point[0]+c0].X>=16384 || (edit_info.HideMap&2) && prim_points[start_point[0]+c0].X<=16384 ||(edit_info.HideMap&3)==0)
            {

                //			LogText(" face %d texture  %d \n",c0,texture);
                if (ptexture1 && (count < tcount1) && ptexture1[count]) {
                    p_f4 = create_a_quad_tex(start_point[0] + c0, start_point[0] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, ptexture1[count]);
                    if (p_f4)
                        p_f4->ThingIndex = -wall;
                } else {
                    p_f4 = create_a_quad(start_point[0] + c0, start_point[0] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, texture_style, texture);
                    if (p_f4)
                        p_f4->ThingIndex = -wall;
                }
                if (no_draw)
                    next_prim_face4--;

                if (!circular) {
                    p_f4->DrawFlags |= POLY_FLAG_DOUBLESIDED;
                }

                //
                // All warehouses have two-sided walls.
                //

                if (building_list[storey_list[storey].BuildingHead].BuildingType == BUILDING_TYPE_WAREHOUSE || storey_list[storey].InsideIDIndex || storey_list[storey].InsideStorey || storey_list[storey].StoreyType == STOREY_TYPE_PARTITION) {
                    if (ptexture2 && (count < tcount2) && ptexture2[count]) {
                        p_f4 = create_a_quad_tex(start_point[0] + c0 + 1, start_point[0] + c0, start_point[1] + c0 + 1, start_point[1] + c0, ptexture2[count]);
                        if (p_f4)
                            p_f4->ThingIndex = -wall;
                    } else {
                        p_f4 = create_a_quad(start_point[0] + c0 + 1, start_point[0] + c0, start_point[1] + c0 + 1, start_point[1] + c0, texture_style2, texture, 1);
                        if (p_f4)
                            p_f4->ThingIndex = -wall;
                    }
                    if (p_f4)
                        p_f4->FaceFlags |= FACE_FLAG_TEX2;
                    if (no_draw)
                        next_prim_face4--;
                }

                if (p_f4)
                    if (height < 256) {
                        if (height == 64) {
                            p_f4->UV[2][1] -= 24;
                            p_f4->UV[3][1] -= 24;
                        } else if (height == 128) {
                            p_f4->UV[2][1] -= 16;
                            p_f4->UV[3][1] -= 16;
                        } else if (height == 128 + 64) {
                            p_f4->UV[2][1] -= 8;
                            p_f4->UV[3][1] -= 8;
                        }
                    }
            }

            count++;
        }
        WindowCount = 0;
    } else {

        /*
                        start_point[0]=build_row_wall_points_at_y(y+height      ,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);
                        start_point[1]=build_row_wall_points_at_y(y+(height*2)/3,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);
                        start_point[2]=build_row_wall_points_at_y(y+(height*1)/3,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);
                        start_point[3]=build_row_wall_points_at_y(y             ,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);

                        start_point[4]=build_row_window_depth_points_at_y(y+(height*2)/3,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);
                        start_point[5]=build_row_window_depth_points_at_y(y+(height*1)/3,x,z,wall_list[wall].DX,wall_list[wall].DZ,wall);

                        for(c0=0;c0<WindowCount*2+1;c0++)
                        {
                                p_f4=create_a_quad(start_point[0]+c0,start_point[0]+c0+1,start_point[1]+c0,start_point[1]+c0+1,0,0);
                                if(p_f4)
                                        p_f4->ThingIndex=-wall;
                                p_f4=create_a_quad(start_point[2]+c0,start_point[2]+c0+1,start_point[3]+c0,start_point[3]+c0+1,0,0);
                                if(p_f4)
                                        p_f4->ThingIndex=-wall;
                                if(!(c0&1))
                                {
                                        p_f4=create_a_quad(start_point[1]+c0,start_point[1]+c0+1,start_point[2]+c0,start_point[2]+c0+1,0,0);
                                        p_f4->ThingIndex=-wall;
                                }

                        }
                        for(c0=0;c0<WindowCount;c0++)
                        {
                                p_f4=create_a_quad(start_point[1]+c0*2+1,start_point[1]+c0*2+2,start_point[4]+c0*2,start_point[4]+c0*2+1,0,0);  //lid
                                p_f4->ThingIndex=-wall;
                                p_f4=create_a_quad(start_point[1]+c0*2+1,start_point[4]+c0*2,start_point[2]+c0*2+1,start_point[5]+c0*2,0,0);  //side1
                                p_f4->ThingIndex=-wall;
                                p_f4=create_a_quad(start_point[4]+c0*2+1,start_point[1]+c0*2+2,start_point[5]+c0*2+1,start_point[2]+c0*2+2,0,0);  //side2
                                p_f4->ThingIndex=-wall;
                                p_f4=create_a_quad(start_point[5]+c0*2,start_point[5]+c0*2+1,start_point[2]+c0*2+1,start_point[2]+c0*2+2,0,0);  //base
                                p_f4->ThingIndex=-wall;
                        }
        */
    }
}

SLONG find_near_prim_point(SLONG x, SLONG z, SLONG sp, SLONG ep)
{
    SLONG best, best_dist = 0x7fffffff, dx, dz, dist, c0;

    for (c0 = sp; c0 < ep; c0++) {
        dx = (prim_points[c0].X - x);
        dz = (prim_points[c0].Z - z);
        dist = SDIST2(dx, dz);
        if (dist < best_dist) {
            best_dist = dist;
            best = c0;
        }
    }

    return (best);
}

void create_recessed_storey_points(SLONG y, SLONG storey, SLONG count, SLONG size)
{

    SLONG px, pz, index, dx, dz;
    SLONG rx, rz;
    SLONG wall;
    SLONG sp;

    count = count;
    sp = next_prim_point;

    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;
    add_point(px, y, pz); // this gets replaced later
    index = storey_list[storey].WallHead;
    wall = index;

    while (index) {
        SLONG next;
        //		LogText(" calc corner %d \n",index);
        dx = wall_list[index].DX;
        dz = wall_list[index].DZ;
        next = wall_list[index].Next;
        if (next) {
            calc_new_corner_point(px, pz, dx, dz, wall_list[next].DX, wall_list[next].DZ, size, &rx, &rz);
            add_point(rx, y, rz);
        } else {
            calc_new_corner_point(px, pz, dx, dz, wall_list[wall].DX, wall_list[wall].DZ, size, &rx, &rz);
            add_point(rx, y, rz);
            prim_points[sp].X = rx;
            prim_points[sp].Z = rz;
        }

        px = dx;
        pz = dz;
        index = wall_list[index].Next;
    }
}

void scan_triangle(SLONG x0, SLONG y0, SLONG z0, SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG flag)
{

    SLONG px, py, pz;
    SLONG face_x, face_y, face_z;
    SLONG c0;
    SLONG s, t, step_s, step_t;
    SLONG vx, vy, vz, wx, wy, wz;
    struct DepthStrip* me;
    SLONG quad;
    SLONG len;
    //	CBYTE	str[100];
    UBYTE info = 0;

    face_x = x0;
    face_y = y0;
    face_z = z0;

    vx = x1 - x0;
    vy = y1 - y0; // vector from point 0 to point 1
    vz = z1 - z0;

    wx = x2 - x0; // vector from point 1 to point 2
    wy = y2 - y0;
    wz = z2 - z0;

    len = (Root(vx * vx + vy * vy + vz * vz) >> 7);
    if (len < 2)
        len = 2;
    step_s = (1 << 7) / len;
    len = (Root(wx * wx + wy * wy + wz * wz) >> 7);
    if (len < 2)
        len = 2;
    step_t = (1 << 7) / len;

    if (step_s == 0)
        step_s = 256;
    if (step_t == 0)
        step_t = 256;

    for (s = 5; s < (245); s += step_s)
        for (t = 5; t < (245) && ((s + t) < (245)); t += step_t) {
            px = face_x + ((s * vx) >> 8) + ((t * wx) >> 8);
            py = face_y + ((s * vy) >> 8) + ((t * wy) >> 8);
            pz = face_z + ((s * vz) >> 8) + ((t * wz) >> 8);

            if (PAP_on_map_hi(px >> PAP_SHIFT_HI, pz >> PAP_SHIFT_HI)) {
                set_map_flag(
                    px >> PAP_SHIFT_HI,
                    pz >> PAP_SHIFT_HI,
                    PAP_FLAG_HIDDEN);
            }

            //		if((px>>8)>0&&(px>>8)<EDIT_MAP_WIDTH&&(pz>>8)>0&&(pz>>8)<EDIT_MAP_DEPTH)
            //		if(in_map_range((px>>ELE_SHIFT),(pz>>ELE_SHIFT)))
            //		{
            //			me=&edit_map[(px>>ELE_SHIFT)][(pz>>ELE_SHIFT)];
            //			me->Flags|=FLOOR_HIDDEN;
            //			set_map_flag(px>>ELE_SHIFT,(pz>>ELE_SHIFT),FLOOR_HIDDEN);
            // LogText(" dx %d dz %d  hidden \n",px>>ELE_SHIFT,pz>>ELE_SHIFT);
            //			me->Texture|=TS_TEXT_SORT;
            //		}
        }
}

void flag_floor_tiles_for_quad(SLONG p0, SLONG p1, SLONG p2, SLONG p3)
{
    SLONG x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;

    x0 = prim_points[p0].X;
    y0 = prim_points[p0].Y;
    z0 = prim_points[p0].Z;

    x1 = prim_points[p1].X;
    y1 = prim_points[p1].Y;
    z1 = prim_points[p1].Z;

    x2 = prim_points[p2].X;
    y2 = prim_points[p2].Y;
    z2 = prim_points[p2].Z;

    x3 = prim_points[p3].X;
    y3 = prim_points[p3].Y;
    z3 = prim_points[p3].Z;

    scan_triangle(x0, y0, z0, x1, y1, z1, x2, y2, z2, 0);
    scan_triangle(x1, y1, z1, x3, y3, z3, x2, y2, z2, 0);
}

void flag_floor_tiles_for_tri(SLONG p0, SLONG p1, SLONG p2)
{
    SLONG x0, y0, z0, x1, y1, z1, x2, y2, z2;

    x0 = prim_points[p0].X;
    y0 = prim_points[p0].Y;
    z0 = prim_points[p0].Z;

    x1 = prim_points[p1].X;
    y1 = prim_points[p1].Y;
    z1 = prim_points[p1].Z;

    x2 = prim_points[p2].X;
    y2 = prim_points[p2].Y;
    z2 = prim_points[p2].Z;

    scan_triangle(x0, y0, z0, x1, y1, z1, x2, y2, z2, 0);
}

SLONG build_roof(UWORD storey, SLONG y, SLONG flat_flag)
{
    //	SLONG	x1,z1,x2,z2,x3,z3;
    SLONG wall;
    //	,prev_wall,prev_prev_wall;
    //	SLONG	sp[10];
    SLONG roof;
    SLONG p0, p1, p2, p3;
    //	SLONG	rx,rz;
    SLONG count;
    SLONG roof_height = BLOCK_SIZE * 3;
    SLONG c0;
    SLONG overlap;

    SLONG roof_flags;
    SLONG roof_rim;
    SLONG overlap_height = BLOCK_SIZE >> 1;

    SLONG poox, pooz;

    // calc_new_corner_point(0,0,0,50,0,100,12,&poox,&pooz);
    overlap = BLOCK_SIZE >> 1;
    /*
            roof=storey_list[storey].Roof;
            roof_flags=storey_list[roof].StoreyFlags;

            if(roof_flags&FLAG_ROOF_WALLED)
                    overlap_height=BLOCK_SIZE;

            if(roof_flags&FLAG_ROOF_FLAT)
                    roof_height=0;

            overlap=BLOCK_SIZE>>1;

            if(roof_flags&FLAG_ROOF_OVERLAP_SMALL)
                    overlap=BLOCK_SIZE>>1;
            else
            if(roof_flags&FLAG_ROOF_OVERLAP_MEDIUM)
                    overlap=BLOCK_SIZE;
    */

    if ((storey_list[storey].StoreyFlags & FLAG_STOREY_ROOF_RIM) && 0) {

        if (storey_list[storey].WallHead) // && storey_list[storey].Roof)
        {

            //
            // Roof points arround top of wall
            //

            start_point[0] = next_prim_point;
            add_point(storey_list[storey].DX, y + 2, storey_list[storey].DZ);
            wall = storey_list[storey].WallHead;
            count = 0;
            while (wall) {

                add_point(wall_list[wall].DX, y + 2, wall_list[wall].DZ);
                wall = wall_list[wall].Next;
                count++;
            }

            //
            // Roof points jutting out at wall height
            //
            start_point[1] = next_prim_point;
            create_recessed_storey_points(y, storey, count, overlap);

            //
            // Roof point jutting out at raised height (overlap_height)
            //

            start_point[2] = next_prim_point;
            roof_rim = next_prim_point;
            for (c0 = 0; c0 < start_point[2] - start_point[1]; c0++) {
                SLONG x, z;
                x = prim_points[c0 + start_point[1]].X;
                z = prim_points[c0 + start_point[1]].Z;

                add_point(x, y + (overlap_height), z);
            }

            //	BUILD RAISED/CENTER POINTS FOR ANGLED/FLAT ROOF
            //  now unused

            /*
                            sp[3]=next_prim_point;
                            wall=storey_list[roof].WallHead;
                            add_point(storey_list[roof].DX,y+roof_height,storey_list[roof].DZ);
                            while(wall)
                            {
                                    add_point(wall_list[wall].DX,y+roof_height,wall_list[wall].DZ);
                                    wall=wall_list[wall].Next;
                            }
            */
            start_point[4] = next_prim_point;
            /*
                            for(c0=0;c0<=count;c0++)
                            {
                                    p0=start_point[0]+c0+1;
                                    p1=start_point[0]+c0;

                                    p2=find_near_prim_point(prim_points[p1].X,prim_points[p1].Z,start_point[3],start_point[4]);
                                    p3=find_near_prim_point(prim_points[p0].X,prim_points[p0].Z,start_point[3],start_point[4]);
                                    if(p2&&p3)
                                    {
                                            if(p2!=p3)
                                            {
                                                    flag_floor_tiles_for_quad(p0,p1,p3,p2);
                                            }
                                            else
                                            {
                                                    flag_floor_tiles_for_tri(p0,p1,p3);
                                            }
                                    }
                            }
            */

            // cancel walled
            /*
                            if(roof_flags&FLAG_ROOF_WALLED)
                            {
                                    //
                                    // raised and overlap inwards
                                    //

                                    create_recessed_storey_points(y+overlap_height,storey,count,-overlap);
                                    start_point[5]=next_prim_point;

                                    //
                                    // overlap inwards but back to wall height
                                    //
                                    roof_rim=next_prim_point;
                                    for(c0=0;c0<start_point[5]-start_point[4];c0++)
                                    {
                                            SLONG	x,z;
                                            x=prim_points[c0+start_point[4]].X;
                                            z=prim_points[c0+start_point[4]].Z;

                                            add_point(x,y,z);

                                    }
                            }

                            if( (roof_flags&(FLAG_ROOF_WALLED|FLAG_ROOF_FLAT)) ==(FLAG_ROOF_WALLED)) //ANGLED ROOF WITH WALL
                            {
                                    //
                                    // overlap inwards even further but back to wall height //only for non flat walled buildings
                                    //
                                    start_point[6]=next_prim_point;
                                    roof_rim=next_prim_point;
                                    create_recessed_storey_points(y,storey,count,-overlap*2);

                            }
            */

            //
            // Brind overlap back to storey position and a bit more
            //
            start_point[5] = next_prim_point;
            roof_rim = next_prim_point;
            //
            // overlap inwards slightly and raised
            //
            create_recessed_storey_points(y + overlap_height, storey, count, -3);

            //	if(storey_list[storey].StoreyFlags&FLAG_ROOF_WALLED)

            for (c0 = 0; c0 <= count; c0++) {
                create_a_quad(start_point[1] + c0, start_point[1] + c0 + 1, start_point[0] + c0, start_point[0] + c0 + 1, 0, 23);
                create_a_quad(start_point[2] + c0, start_point[2] + c0 + 1, start_point[1] + c0, start_point[1] + c0 + 1, 0, 23);
                create_a_quad(start_point[5] + c0, start_point[5] + c0 + 1, start_point[2] + c0, start_point[2] + c0 + 1, 0, 23);
            }

            /* //forget walled for now
                            if(roof_flags&FLAG_ROOF_WALLED)
                            {
                                    for(c0=0;c0<=count;c0++)
                                    {
                                            create_a_quad(start_point[4]+c0,start_point[4]+c0+1,start_point[2]+c0,start_point[2]+c0+1,0,23);
                                            create_a_quad(start_point[5]+c0,start_point[5]+c0+1,start_point[4]+c0,start_point[4]+c0+1,0,23);
                                    }

                            }
                            if( (roof_flags&(FLAG_ROOF_WALLED|FLAG_ROOF_FLAT)) ==(FLAG_ROOF_WALLED)) //ANGLED ROOF WITH WALL
                            {
                                    for(c0=0;c0<=count;c0++)
                                            create_a_quad(start_point[6]+c0,start_point[6]+c0+1,start_point[5]+c0,start_point[5]+c0+1,0,23);
                            }
            */

            /* //old filling to roof points raised or flat
                            wall=storey_list[storey].WallHead;
                            count=0;
                            while(wall)
                            {
                                    SLONG	q0,q1;

                                    p0=roof_rim+count+1;
                                    p1=roof_rim+count;

                                    q0=start_point[0]+count+1;
                                    q1=start_point[0]+count;

                                    p2=find_near_prim_point(prim_points[p1].X,prim_points[p1].Z,start_point[3],start_point[4]);
                                    p3=find_near_prim_point(prim_points[p0].X,prim_points[p0].Z,start_point[3],start_point[4]);

                                    if(p2!=p3)
                                    {
                                            create_a_quad(p0,p1,p3,p2,0,23); //p2,p3,p1,p0);
                                            //no more flag_floor_tiles_for_quad(q0,q1,p3,p2);

                                    }
                                    else
                                    {
                                            create_a_tri(p0,p1,p2,0,23);
                                            //no more flag_floor_tiles_for_tri(q0,q1,p3);
                                    }
                                    wall=wall_list[wall].Next;
                                    count++;
                            }
            */
        }
    } else
        overlap_height = 0;
    return (build_roof_grid(storey, y + overlap_height, flat_flag));
}

SLONG area_of_quad(SLONG p0, SLONG p1, SLONG p2, SLONG p3)
{

    SLONG dx, dz;
    dx = abs(prim_points[p0].X - prim_points[p1].X);
    dz = abs(prim_points[p0].Z - prim_points[p2].Z);
    return (dx * dz);
}

// p0         p01          p1

// p20        p03          p13

// p2        p32          p3

void create_split_quad_into_4(SLONG p0, SLONG p1, SLONG p2, SLONG p3, SLONG wall, SLONG y)
{

    SLONG p01, p13, p32, p20, p03;
    SLONG x, z;
    struct PrimFace4* p_f4;

    SWORD texture_style;
    texture_style = wall_list[wall].TextureStyle;

    p01 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p0].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p0].Z) >> 1;
    add_point(x, y, z);

    p13 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p3].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p3].Z) >> 1;
    add_point(x, y, z);

    p32 = next_prim_point;
    x = (prim_points[p3].X + prim_points[p2].X) >> 1;
    z = (prim_points[p3].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p20 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p2].X) >> 1;
    z = (prim_points[p0].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p03 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p1].X + prim_points[p2].X + prim_points[p3].X) >> 2;
    z = (prim_points[p0].Z + prim_points[p1].Z + prim_points[p2].Z + prim_points[p3].Z) >> 2;
    add_point(x, y, z);

    p_f4 = create_a_quad(p0, p01, p20, p03, texture_style, 0); // p2,p3,p1,p0);
    add_quad_to_walkable_list(next_prim_face4 - 1);
    p_f4->ThingIndex = -wall;

    p_f4 = create_a_quad(p20, p03, p2, p32, texture_style, 0); // p2,p3,p1,p0);
    add_quad_to_walkable_list(next_prim_face4 - 1);
    p_f4->ThingIndex = -wall;

    p_f4 = create_a_quad(p01, p1, p03, p13, texture_style, 0); // p2,p3,p1,p0);
    add_quad_to_walkable_list(next_prim_face4 - 1);
    p_f4->ThingIndex = -wall;

    p_f4 = create_a_quad(p03, p13, p32, p3, texture_style, 0); // p2,p3,p1,p0);
    add_quad_to_walkable_list(next_prim_face4 - 1);
    p_f4->ThingIndex = -wall;
}

void create_split_quad_into_16(SLONG p0, SLONG p1, SLONG p2, SLONG p3, SLONG wall, SLONG y)
{
    SLONG p01, p13, p32, p20, p03;
    SLONG x, z;
    struct PrimFace4* p_f4;

    p01 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p0].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p0].Z) >> 1;
    add_point(x, y, z);

    p13 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p3].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p3].Z) >> 1;
    add_point(x, y, z);

    p32 = next_prim_point;
    x = (prim_points[p3].X + prim_points[p2].X) >> 1;
    z = (prim_points[p3].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p20 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p2].X) >> 1;
    z = (prim_points[p0].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p03 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p1].X + prim_points[p2].X + prim_points[p3].X) >> 2;
    z = (prim_points[p0].Z + prim_points[p1].Z + prim_points[p2].Z + prim_points[p3].Z) >> 2;
    add_point(x, y, z);

    create_split_quad_into_4(p0, p01, p20, p03, wall, y); // p2,p3,p1,p0);
    create_split_quad_into_4(p20, p03, p2, p32, wall, y); // p2,p3,p1,p0);
    create_split_quad_into_4(p01, p1, p03, p13, wall, y); // p2,p3,p1,p0);
    create_split_quad_into_4(p03, p13, p32, p3, wall, y); // p2,p3,p1,p0);
}

void create_split_quad_into_48(SLONG p0, SLONG p1, SLONG p2, SLONG p3, SLONG wall, SLONG y)
{
    SLONG p01, p13, p32, p20, p03;
    SLONG x, z;
    struct PrimFace4* p_f4;

    p01 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p0].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p0].Z) >> 1;
    add_point(x, y, z);

    p13 = next_prim_point;
    x = (prim_points[p1].X + prim_points[p3].X) >> 1;
    z = (prim_points[p1].Z + prim_points[p3].Z) >> 1;
    add_point(x, y, z);

    p32 = next_prim_point;
    x = (prim_points[p3].X + prim_points[p2].X) >> 1;
    z = (prim_points[p3].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p20 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p2].X) >> 1;
    z = (prim_points[p0].Z + prim_points[p2].Z) >> 1;
    add_point(x, y, z);

    p03 = next_prim_point;
    x = (prim_points[p0].X + prim_points[p1].X + prim_points[p2].X + prim_points[p3].X) >> 2;
    z = (prim_points[p0].Z + prim_points[p1].Z + prim_points[p2].Z + prim_points[p3].Z) >> 2;
    add_point(x, y, z);

    create_split_quad_into_16(p0, p01, p20, p03, wall, y); // p2,p3,p1,p0);
    create_split_quad_into_16(p20, p03, p2, p32, wall, y); // p2,p3,p1,p0);
    create_split_quad_into_16(p01, p1, p03, p13, wall, y); // p2,p3,p1,p0);
    create_split_quad_into_16(p03, p13, p32, p3, wall, y); // p2,p3,p1,p0);
}

void build_roof_quad(UWORD storey, SLONG y)
{
    //	SLONG	x1,z1,x2,z2,x3,z3;
    SLONG wall;
    //	,prev_wall,prev_prev_wall;
    //	SLONG	start_point[10];
    SLONG roof;
    SLONG p0, p1, p2, p3;
    //	SLONG	rx,rz;
    SLONG count = 0;
    SLONG roof_height = 0; // BLOCK_SIZE*3;
    struct PrimFace4* p_f4;
    SWORD texture_style;

    // build_storey_lip(storey,y);

    if (storey_list[storey].WallHead && 0)
        ; // storey_list[storey].Roof)
    {
        SLONG area;
        SLONG npp;

        npp = next_prim_point;

        roof = 0; // storey_list[storey].Roof;
        start_point[0] = next_prim_point;
        wall = storey_list[roof].WallHead;
        if (wall_list[wall].WallFlags & FLAG_ROOF_FLAT)
            roof_height = 0;

        if (wall)
            add_point(storey_list[roof].DX, y - roof_height, storey_list[roof].DZ);
        while (wall && count < 3) {

            add_point(wall_list[wall].DX, y - roof_height, wall_list[wall].DZ);
            wall = wall_list[wall].Next;
            count++;
        }

        if (count < 3) {
            //
            // A quad roof has been found that has less than 4 walls
            //

            next_prim_point = npp;
            return;
        }

        wall = storey_list[roof].WallHead;
        texture_style = wall_list[wall].TextureStyle;
        p0 = start_point[0] + 3;
        p1 = start_point[0] + 2;
        p2 = start_point[0] + 0;
        p3 = start_point[0] + 1;
        area = area_of_quad(p0, p1, p2, p3);
        if (area > 8 * 256 * 8 * 256)
            create_split_quad_into_48(p0, p1, p2, p3, wall, y + roof_height);
        else if (area > 4 * 256 * 4 * 256)
            create_split_quad_into_16(p0, p1, p2, p3, wall, y + roof_height);
        else if (area > 2 * 256 * 2 * 256)
            create_split_quad_into_4(p0, p1, p2, p3, wall, y + roof_height);
        else {
            p_f4 = create_a_quad(p0, p1, p2, p3, texture_style, 0); // p2,p3,p1,p0);
            add_quad_to_walkable_list(next_prim_face4 - 1);
            // no more flag_floor_tiles_for_quad(p0,p1,p2,p3);
            p_f4->ThingIndex = -wall;
        }
    }
}

void center_object(SLONG sp, SLONG ep)
{
    SLONG c0;
    //	,count;
    SLONG az = 0, ax = 0;
    if (ep - sp < 0) {
        return;
    }
    if ((ep - sp) == 0) {
        return;
    }

    /*
     */
    for (c0 = sp; c0 < ep; c0++) {
        ax += prim_points[c0].X;
        az += prim_points[c0].Z;
    }

    ax /= (ep - sp);
    az /= (ep - sp);
    build_x = ax; //+(64<<ELE_SHIFT);
    build_z = az; //+(64<<ELE_SHIFT);
    build_y = 0;

    /*
            for(c0=sp;c0<ep;c0++)
            {
                    prim_points[c0].X-=ax;
                    prim_points[c0].Z-=az;
            }
    */
}

void center_object_about_xz(SLONG sp, SLONG ep, SLONG x, SLONG z)
{
    SLONG c0;
    //	,count;
    if (ep - sp < 0) {
        return;
    }
    if ((ep - sp) == 0) {
        return;
    }

    build_x = x; //+(64<<ELE_SHIFT);
    build_z = z; //+(64<<ELE_SHIFT);
    build_y = 0;
    /*
            for(c0=sp;c0<ep;c0++)
            {
                    prim_points[c0].X-=x;
                    prim_points[c0].Z-=z;
            }
    */
}

SLONG build_facet(SLONG sp, SLONG mp, SLONG sf3, SLONG sf4, SLONG mf4, SLONG prev_facet, UWORD flags, UWORD col_vect)
{
    struct BuildingFacet* p_obj;
    p_obj = &building_facets[next_building_facet];
    //	LogText(" add facet %d to building %d \n",next_building_facet,next_building_object);
    next_building_facet++;

    SLONG c0;
    for (c0 = sf4; c0 < next_prim_face4; c0++) {
        ASSERT(prim_faces4[c0].Points[0] < next_prim_point);
        ASSERT(prim_faces4[c0].Points[1] < next_prim_point);
        ASSERT(prim_faces4[c0].Points[2] < next_prim_point);
        ASSERT(prim_faces4[c0].Points[3] < next_prim_point);
    }

    p_obj->StartPoint = sp;
    p_obj->MidPoint = mp;
    p_obj->EndPoint = next_prim_point;

    ASSERT(sf4 >= 0);
    ASSERT(sf4 <= 65535);
    p_obj->StartFace4 = sf4;
    p_obj->MidFace4 = mf4;
    p_obj->EndFace4 = next_prim_face4;
    p_obj->StartFace3 = sf3;
    p_obj->EndFace3 = next_prim_face3;
    p_obj->NextFacet = prev_facet;
    p_obj->FacetFlags = flags;
    p_obj->ColVect = col_vect;

    ASSERT(p_obj->StartFace4 >= 0);
    ASSERT(p_obj->StartFace3 >= 0);
    ASSERT(p_obj->EndFace4 >= 0);
    ASSERT(p_obj->EndFace3 >= 0);

    //	center_object(p_obj);
    return (next_building_facet - 1);
}

SLONG build_building(SLONG sp, SLONG sf3, SLONG sf4, SLONG prev_facet)
{
    struct BuildingObject* p_bobj;
    p_bobj = &building_objects[next_building_object];
    next_building_object++;

    p_bobj->StartPoint = sp;
    p_bobj->EndPoint = next_prim_point;

    p_bobj->StartFace4 = sf4;
    p_bobj->EndFace4 = next_prim_face4;

    p_bobj->StartFace3 = sf3;
    p_bobj->EndFace3 = next_prim_face3;
    p_bobj->FacetHead = prev_facet;
    //	LogText(" center object %d->%d \n",p_bobj->StartPoint,p_bobj->EndPoint);
    center_object(p_bobj->StartPoint, p_bobj->EndPoint);

    //	center_object(p_bobj);
    return (next_building_object - 1);
}

SLONG build_building2(SLONG sp, SLONG sf3, SLONG sf4, SLONG prev_facet, SLONG cx, SLONG cz)
{
    struct BuildingObject* p_bobj;
    p_bobj = &building_objects[next_building_object];
    next_building_object++;

    p_bobj->StartPoint = sp;
    p_bobj->EndPoint = next_prim_point;

    p_bobj->StartFace4 = sf4;
    p_bobj->EndFace4 = next_prim_face4;

    p_bobj->StartFace3 = sf3;
    p_bobj->EndFace3 = next_prim_face3;
    p_bobj->FacetHead = prev_facet;
    //	LogText(" center object %d->%d \n",p_bobj->StartPoint,p_bobj->EndPoint);
    center_object_about_xz(p_bobj->StartPoint, p_bobj->EndPoint, cx, cz);

    //	center_object(p_bobj);
    return (next_building_object - 1);
}

SLONG build_prim_object(SLONG sp, SLONG sf3, SLONG sf4)
{
    struct PrimObject* p_obj;
    p_obj = &prim_objects[next_prim_object];
    next_prim_object++;

    p_obj->StartPoint = sp;
    p_obj->EndPoint = next_prim_point;

    p_obj->StartFace4 = sf4;
    p_obj->EndFace4 = next_prim_face4;

    p_obj->StartFace3 = sf3;
    p_obj->EndFace3 = next_prim_face3;
    center_object(p_obj->StartPoint, p_obj->EndPoint);
    return (next_prim_object - 1);
}

void find_next_last_coord(SWORD wall, SLONG* x, SLONG* z)
{
    SLONG next_wall;
    while (wall) {
        next_wall = wall_list[wall].Next;
        //		LogText(" wall %d ",wall);
        if (wall_list[next_wall].Next == 0) {
            //			LogText(" next to last wall is %d xz (%d,%d)",wall,wall_list[wall].DX,wall_list[wall].DZ);
            *x = wall_list[wall].DX;
            *z = wall_list[wall].DZ;
            return;
        }
        wall = next_wall;
    }
}

struct LedgeInfo {
    SWORD Storey, Wall;
    SWORD Y;
    SLONG X1, Z1, X2, Z2, X3, Z3, X4, Z4;
};

void build_single_ledge(struct LedgeInfo* p_ledge)
{

    SLONG sp[4], count = 0;
    SLONG rx, rz, rx2, rz2;

    SLONG y, height;
    struct PrimFace4* p4;

    //	LogText(" build ledge (%d,%d) (%d,%d) (%d,%d) (%d,%d)  storey %d wall %d \n",p_ledge->X1,p_ledge->Z1,p_ledge->X2,p_ledge->Z2,p_ledge->X3,p_ledge->Z3,p_ledge->X4,p_ledge->Z4,p_ledge->Storey,p_ledge->Wall);

    height = BLOCK_SIZE >> 1;
    y = p_ledge->Y;

    sp[0] = next_prim_point;
    add_point(p_ledge->X2, y + 2, p_ledge->Z2);
    add_point(p_ledge->X3, y + 2, p_ledge->Z3);
    calc_new_corner_point(p_ledge->X1, p_ledge->Z1, p_ledge->X2, p_ledge->Z2, p_ledge->X3, p_ledge->Z3, BLOCK_SIZE, &rx, &rz);
    calc_new_corner_point(p_ledge->X2, p_ledge->Z2, p_ledge->X3, p_ledge->Z3, p_ledge->X4, p_ledge->Z4, BLOCK_SIZE, &rx2, &rz2);
    sp[1] = next_prim_point;
    add_point(rx, y + 2, rz);
    add_point(rx2, y + 2, rz2);

    y += height;
    sp[2] = next_prim_point;
    add_point(rx, y + 2, rz);
    add_point(rx2, y + 2, rz2);

    sp[3] = next_prim_point;
    add_point(p_ledge->X2, y + 2, p_ledge->Z2);
    add_point(p_ledge->X3, y + 2, p_ledge->Z3);

    p4 = create_a_quad(sp[1], sp[1] + 1, sp[0], sp[0] + 1, 0, 18);
    OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_LONG_LEDGE);
    p4 = create_a_quad(sp[2], sp[2] + 1, sp[1], sp[1] + 1, 0, 18);
    OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_LONG_LEDGE);
    p4 = create_a_quad(sp[3], sp[3] + 1, sp[2], sp[2] + 1, 0, 18);
    OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_LONG_LEDGE);
}

SLONG dist_between_vertex_and_vector(SLONG x1, SLONG y1, SLONG x2, SLONG y2, SLONG px, SLONG py)
{
    SLONG dist, dx, dy;

    x1 = (x1 + x2) >> 1;
    y1 = (y1 + y2) >> 1;

    dx = abs(x1 - px);
    dy = abs(y1 - py);

    dist = QDIST2(dx, dy);
    return (dist);
}

SLONG find_wall_for_fe(SLONG fe_x, SLONG fe_y, SLONG storey)
{
    SLONG wall = 0;
    SLONG px, pz, x1, z1;
    SLONG best_wall = -1, best_dist = 0x7fffffff, dist;
    SLONG wall_count = 0;

    while (storey_list[storey].StoreyType == STOREY_TYPE_LADDER || storey_list[storey].StoreyType == STOREY_TYPE_FIRE_ESCAPE || storey_list[storey].StoreyType == STOREY_TYPE_STAIRCASE || storey_list[storey].StoreyType == STOREY_TYPE_FENCE_BRICK || storey_list[storey].StoreyType == STOREY_TYPE_FENCE_FLAT || storey_list[storey].StoreyType == STOREY_TYPE_OUTSIDE_DOOR || storey_list[storey].StoreyType == STOREY_TYPE_FENCE) {
        storey = storey_list[storey].Next;
    }
    wall = storey_list[storey].WallHead;
    px = storey_list[storey].DX;
    pz = storey_list[storey].DZ;
    while (wall) {
        x1 = wall_list[wall].DX;
        z1 = wall_list[wall].DZ;

        dist = dist_between_vertex_and_vector(px, pz, x1, z1, fe_x, fe_y);
        if (dist < best_dist) {
            best_wall = wall_count;
            best_dist = dist;
        }

        wall_count++;
        wall = wall_list[wall].Next;
        px = x1;
        pz = z1;
    }
    return (best_wall);
}

SLONG sp_stairs[300];
void build_staircase(SLONG storey)
{
    SLONG wall;
    SLONG wall_count = 0;
    SLONG count;
    SLONG step_count, step_size, step_height, len, step_y, step_pos, step_length;
    SLONG row = 0;
    SLONG c0, c1;
    SLONG y, start_y = 0;
    struct PrimFace4* p4;
    SLONG step_pos_old;
    SLONG last;
    SLONG x1, z1;

    struct StairVect {
        SLONG X1, Z1, X2, Z2;
    };

    struct StairVect s_vects[50];

    wall = storey_list[storey].WallHead;
    x1 = storey_list[storey].DX;
    z1 = storey_list[storey].DZ;
    while (wall) {
        SLONG x2, z2;
        wall_count++;
        x2 = wall_list[wall].DX;
        z2 = wall_list[wall].DZ;
        insert_collision_vect(x1, start_y, z1, x2, start_y, z2, STOREY_TYPE_STAIRCASE, 0, -wall);
        set_vect_floor_height(x1, z1, x2, z2, (start_y + build_min_y) >> ALT_SHIFT);
        wall = wall_list[wall].Next;
        x1 = x2;
        z1 = z2;
    }

    start_y += build_min_y;

    if ((wall_count < 4) || (wall_count & 1))
        return;

    count = 1;
    wall = storey_list[storey].WallHead;
    //	vects[0].X1=storey_list[storey].DX;
    //	vects[0].Z1=storey_list[storey].DZ;
    while (wall) {
        if (count <= (wall_count >> 1)) {
            s_vects[count - 1].X1 = wall_list[wall].DX;
            s_vects[count - 1].Z1 = wall_list[wall].DZ;

        } else {
            SLONG pos;
            pos = (wall_count) - (count) + 1;

            s_vects[pos - 1].X2 = wall_list[wall].DX;
            s_vects[pos - 1].Z2 = wall_list[wall].DZ;
        }

        count++;
        wall = wall_list[wall].Next;
    }

    /* We now have a load of vects to travel along */

    {
        SLONG dx, dz;

        dx = abs(s_vects[0].X1 - s_vects[0].X2);
        dz = abs(s_vects[0].Z1 - s_vects[0].Z2);
        len = QDIST2(dx, dz);
    }

    if (len == 0)
        return;

    step_height = BLOCK_SIZE >> 1;
    step_length = BLOCK_SIZE >> 1;
    step_count = len / step_length;
    if (step_count == 0)
        return;
    step_size = (len << 8) / step_count;
    step_y = storey_list[storey].DY;

    step_pos = len << 8;
    y = start_y;

    step_pos_old = step_pos;
    sp_stairs[99] = next_prim_point;
    while (step_pos >= 0) {
        SLONG x, z;
        if (storey_list[storey].Info1 && step_pos < storey_list[storey].Info1 * step_size) {
            step_pos = 0;
        }
        last = (wall_count >> 1) - 1;
        x = s_vects[0].X1 + ((s_vects[0].X2 - s_vects[0].X1) * step_pos) / (len << 8);
        z = s_vects[0].Z1 + ((s_vects[0].Z2 - s_vects[0].Z1) * step_pos) / (len << 8);
        add_point(x, start_y, z);
        x = s_vects[last].X1 + ((s_vects[last].X2 - s_vects[last].X1) * step_pos) / (len << 8);
        z = s_vects[last].Z1 + ((s_vects[last].Z2 - s_vects[last].Z1) * step_pos) / (len << 8);
        add_point(x, start_y, z);
        step_pos -= step_size;
    }
    step_pos = step_pos_old;

    while (step_pos >= 0) {

        if (storey_list[storey].Info1 && step_pos < storey_list[storey].Info1 * step_size) {
            step_pos = 0;
            /*
                                    sp_stairs[row]=next_prim_point;
                                    for(c0=0;c0<(wall_count>>1);c0++)
                                    {
                                            SLONG	x,z;

                                            x=s_vects[c0].X1;
                                            z=s_vects[c0].Z1;

                                            add_point(x,y,z);
                                    }
                                    row++;
                                    step_pos=-1;
            */
        }
        //		else
        {

            sp_stairs[row] = next_prim_point;
            for (c0 = 0; c0 < (wall_count >> 1); c0++) {
                SLONG x, z;
                x = s_vects[c0].X1 + ((s_vects[c0].X2 - s_vects[c0].X1) * step_pos) / (len << 8);
                z = s_vects[c0].Z1 + ((s_vects[c0].Z2 - s_vects[c0].Z1) * step_pos) / (len << 8);
                add_point(x, y, z);
            }
            sp_stairs[row + 1] = next_prim_point;
            step_pos -= step_size;
            if (step_pos >= 0) {
                // we dont need a riser on the last one
                for (c0 = sp_stairs[row]; c0 < sp_stairs[row] + (wall_count >> 1); c0++) {
                    add_point(prim_points[c0].X, y + step_height, prim_points[c0].Z);
                }

                y += step_height;
                row += 2;
            } else {

                y += step_height;
                row += 1;
            }
        }
    }

    wall = storey_list[storey].WallHead;

    for (c1 = 0; c1 < (row - 1); c1++) {
        for (c0 = 0; c0 < ((wall_count >> 1) - 1); c0++) {
            //
            // Do the risers and the steps
            //
            p4 = create_a_quad(sp_stairs[c1 + 1] + 1 + c0, sp_stairs[c1 + 1] + c0, sp_stairs[c1] + 1 + c0, sp_stairs[c1] + c0, 0, 18 + ((c1 & 1) ? 5 : 0));
            p4->ThingIndex = -wall;
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            //			if(c1&1) //only steps are walkable
            add_quad_to_walkable_list(next_prim_face4 - 1);
        }
        if (c1 & 1) {
            //
            // do the sides
            //

            p4 = create_a_quad(sp_stairs[c1], sp_stairs[c1 + 1], sp_stairs[99] + ((c1 - 1) & 0xfffe), sp_stairs[99] + 2 + ((c1 - 1) & 0xfffe), 0, 23);
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
            p4 = create_a_quad(sp_stairs[c1 + 1] + last, sp_stairs[c1] + last, sp_stairs[99] + 3 + ((c1 - 1) & 0xfffe), sp_stairs[99] + 1 + ((c1 - 1) & 0xfffe), 0, 23);
            OR_SORT_LEVEL(p4->FaceFlags, SORT_LEVEL_FIRE_ESCAPE);
        }
    }
}

void get_wall_start_and_end(
    SLONG want_wall,

    //
    // These are 16-bit map coordinates...
    //

    SLONG* x1, SLONG* z1,
    SLONG* x2, SLONG* z2)
{
    SLONG wall;

    FWall* p_wall;
    FStorey* p_storey;

    p_storey = &storey_list[wall_list[want_wall].StoreyHead];

    *x1 = p_storey->DX;
    *z1 = p_storey->DZ;

    wall = p_storey->WallHead;

    while (wall) {
        p_wall = &wall_list[wall];

        *x2 = p_wall->DX;
        *z2 = p_wall->DZ;

        if (wall == want_wall) {
            return;
        }

        *x1 = *x2;
        *z1 = *z2;

        wall = p_wall->Next;
    }

    //
    // The wall wasn't part of its storey!
    //

    ASSERT(0);
}

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
