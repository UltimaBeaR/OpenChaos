#include "fallen/Headers/Game.h"
#include "world/map/supermap.h"
#include "world/map/supermap_globals.h"
#include "world/map/pap.h"
#include "world/map/pap_globals.h"
#include "world/navigation/inside2.h"
#include "world/navigation/inside2_globals.h"
#include "world/map/ob.h"
#include "world/map/ob_globals.h"
#include "world/environment/build2.h"
#include "engine/io/env.h"
// Temporary: memory.h — dbuildings, dfacets, dwalkables, roof_faces4 etc. not yet in new/
#include "fallen/Headers/memory.h"
// Temporary: mav.h — MAV_opt_upto (for make_all_clumps — build tool, dead in runtime)
#include "ai/mav.h"
// Temporary: texture.h — TEXTURE_load_needed
#include "..\ddengine\headers\texture.h"
#include "engine/physics/collide.h"
// Temporary: io.h — FileRead
#include "fallen/Headers/io.h"
#include <MFStdLib.h>
#include <string.h>

extern UBYTE roper_pickup;
extern SLONG TEXTURE_set;
extern int TEXTURE_create_clump;

// Forward declarations for functions defined in Game.cpp (not yet migrated).
// uc_orig: make_texture_clumps (fallen/Source/Game.cpp)
BOOL make_texture_clumps(CBYTE* mission_name);

// Forward declarations for functions defined in build2.cpp (not in build2.h).
// uc_orig: add_facet_to_map (fallen/Source/build2.cpp)
void add_facet_to_map(SLONG facet);

// ---- Globals defined here ----

// Supermap counter globals are in supermap_globals.cpp.
// next_dbuilding, next_dfacet, etc. are in memory.cpp (extern decls in world/map/supermap_globals.h).

// ---- Internal state ----

// Levels struct and levels[] table are defined in supermap_globals.cpp.

// ---- Functions ----

// uc_orig: calc_inside_for_xyz (fallen/Source/supermap.cpp)
UWORD calc_inside_for_xyz(SLONG x, SLONG y, SLONG z, UWORD* room)
{
    SLONG c0;
    SLONG mx = x >> 8;
    SLONG mz = z >> 8;

    for (c0 = 1; c0 < next_inside_storey; c0++) {
        if (mx >= inside_storeys[c0].MinX)
            if (mx < inside_storeys[c0].MaxX)
                if (mz >= inside_storeys[c0].MinZ)
                    if (mz < inside_storeys[c0].MaxZ) {
                        if (y > inside_storeys[c0].StoreyY &&
                            y < inside_storeys[c0].StoreyY + 256) {
                            SLONG r = find_inside_room(c0, mx, mz);
                            if (r) {
                                *room = r;
                                return c0;
                            }
                        }
                    }
    }

    *room = 0;
    return 0;
}

// Load walkable geometry (DWalkable array and roof face quads) from the level file.
// uc_orig: load_walkables (fallen/Source/supermap.cpp)
static void load_walkables(MFFileHandle handle, SLONG save_type)
{
    struct RoofFace4* p_roof;
    SLONG c0;

    FileRead(handle, &next_dwalkable, 2);
    FileRead(handle, &next_roof_face4, 2);
    FileRead(handle, &dwalkables[0], sizeof(struct DWalkable) * next_dwalkable);
    FileRead(handle, &roof_faces4[0], sizeof(struct RoofFace4) * next_roof_face4);

    first_walkable_prim_point = 0;
    number_of_walkable_prim_points = 0;
    first_walkable_prim_face4 = 0;
    number_of_walkable_prim_faces4 = 0;

    // Set slope flag in RZ field for non-flat roof quads.
    p_roof = &roof_faces4[0];
    for (c0 = 0; c0 < next_roof_face4; c0++) {
        if (p_roof->DY[0] || p_roof->DY[1] || p_roof->DY[2])
            p_roof->RZ |= (1 << 7);
        else
            p_roof->RZ &= 127;
        p_roof++;
    }
}

// uc_orig: load_super_map (fallen/Source/supermap.cpp)
void load_super_map(MFFileHandle handle, SLONG save_type)
{
    SLONG c0;

    next_dbuilding = 1;
    next_dfacet = 1;
    next_dstyle = 1;

    INDOORS_INDEX = 0;
    INDOORS_INDEX_NEXT = 0;

    FileRead(handle, &next_dbuilding, 2);
    FileRead(handle, &next_dfacet, 2);
    FileRead(handle, &next_dstyle, 2);

    if (save_type >= 17) {
        FileRead(handle, &next_paint_mem, 2);
        FileRead(handle, &next_dstorey, 2);
    }

    FileRead(handle, &dbuildings[0], sizeof(struct DBuilding) * next_dbuilding);
    FileRead(handle, &dfacets[0], sizeof(struct DFacet) * next_dfacet);
    FileRead(handle, &dstyles[0], sizeof(UWORD) * next_dstyle);

    if (save_type >= 17) {
        FileRead(handle, &paint_mem[0], sizeof(UBYTE) * next_paint_mem);
        FileRead(handle, &dstoreys[0], sizeof(struct DStorey) * next_dstorey);
    }

    // Reset lighting cache for all facets — will be rebuilt on first draw.
    for (c0 = 1; c0 < next_dfacet; c0++) {
        dfacets[c0].Dfcache = 0;
    }

    if (save_type >= 21) {
        FileRead(handle, &next_inside_storey, sizeof(next_inside_storey));
        FileRead(handle, &next_inside_stair, sizeof(next_inside_stair));
        FileRead(handle, &next_inside_block, sizeof(next_inside_block));
        FileRead(handle, &inside_storeys[0], sizeof(struct InsideStorey) * next_inside_storey);
        FileRead(handle, &inside_stairs[0], sizeof(struct Staircase) * next_inside_stair);
        FileRead(handle, &inside_block[0], sizeof(UBYTE) * next_inside_block);
    }

    load_walkables(handle, save_type);

    if (save_type >= 23) {
        FileRead(handle, (UBYTE*)&OB_ob_upto, sizeof(OB_ob_upto));
        FileRead(handle, (UBYTE*)&OB_ob[0], sizeof(OB_Ob) * OB_ob_upto);
        FileRead(handle, (UBYTE*)&OB_mapwho[0][0], sizeof(OB_Mapwho) * OB_SIZE * OB_SIZE);
    }
}

// uc_orig: add_sewer_ladder (fallen/Source/supermap.cpp)
void add_sewer_ladder(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG bottom, SLONG height,
    SLONG link)
{
    DFacet* df;

    if (!WITHIN(next_dfacet, 0, MAX_DFACETS - 1))
        return;

    if (next_dfacet == 0)
        next_dfacet = 1;

    df = &dfacets[next_dfacet];
    df->FacetType = STOREY_TYPE_LADDER;
    df->FacetFlags = FACET_FLAG_IN_SEWERS;
    df->x[0] = x1 >> 8;
    df->z[0] = z1 >> 8;
    df->x[1] = x2 >> 8;
    df->z[1] = z2 >> 8;
    df->Y[0] = bottom;
    df->Y[1] = bottom;
    df->Height = height;

    if (link)
        df->FacetFlags |= FACET_FLAG_LADDER_LINK;

    add_facet_to_map(next_dfacet);
    next_dfacet += 1;
}

// uc_orig: find_electric_fence_dbuilding (fallen/Source/supermap.cpp)
SLONG find_electric_fence_dbuilding(
    SLONG world_x, SLONG world_y, SLONG world_z, SLONG range)
{
    SLONG dist;
    SLONG best_dist = UC_INFINITY;
    SLONG best_facet = NULL;
    SLONG mx, mz;
    SLONG mx1, mx2, mz1, mz2;
    SLONG f_list;
    SLONG facet;
    SLONG exit;

    mx1 = (world_x - range) >> PAP_SHIFT_LO;
    mz1 = (world_z - range) >> PAP_SHIFT_LO;
    mx2 = (world_x + range) >> PAP_SHIFT_LO;
    mz2 = (world_z + range) >> PAP_SHIFT_LO;

    SATURATE(mx1, 0, PAP_SIZE_LO - 1);
    SATURATE(mz1, 0, PAP_SIZE_LO - 1);
    SATURATE(mx2, 0, PAP_SIZE_LO - 1);
    SATURATE(mz2, 0, PAP_SIZE_LO - 1);

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            f_list = PAP_2LO(mx, mz).ColVectHead;

            if (f_list) {
                exit = UC_FALSE;

                while (!exit) {
                    facet = facet_links[f_list];
                    ASSERT(facet);

                    if (facet < 0) {
                        // Negative index = last facet in this mapsquare's chain.
                        facet = -facet;
                        exit = UC_TRUE;
                    }

                    dist = distance_to_line(
                        dfacets[facet].x[0] << 8,
                        dfacets[facet].z[0] << 8,
                        dfacets[facet].x[1] << 8,
                        dfacets[facet].z[1] << 8,
                        world_x, world_z);

                    if (dist < best_dist) {
                        best_dist = dist;
                        best_facet = facet;
                    }

                    f_list++;
                }
            }
        }

    if (best_facet == NULL)
        return NULL;

    return dfacets[best_facet].Building;
}

// uc_orig: set_electric_fence_state (fallen/Source/supermap.cpp)
void set_electric_fence_state(SLONG dbuilding, SLONG onoroff)
{
    SLONG facet;

    for (facet = dbuildings[dbuilding].StartFacet;
         facet < dbuildings[dbuilding].EndFacet;
         facet++) {
        if (onoroff)
            dfacets[facet].FacetFlags |= FACET_FLAG_ELECTRIFIED;
        else
            dfacets[facet].FacetFlags &= ~FACET_FLAG_ELECTRIFIED;
    }
}

// uc_orig: SUPERMAP_counter_increase (fallen/Source/supermap.cpp)
void SUPERMAP_counter_increase(UBYTE which)
{
    SUPERMAP_counter[which] += 1;

    if (SUPERMAP_counter[which] == 0) {
        // Counter wrapped — reset all facet counters so generation stamps stay consistent.
        SLONG i;
        for (i = 1; i < next_dfacet; i++) {
            dfacets[i].Counter[which] = 0;
        }
        SUPERMAP_counter[which] = 1;
    }
}

// uc_orig: get_level_no (fallen/Source/supermap.cpp)
SLONG get_level_no(CBYTE* name)
{
    SLONG p0, p1, c0 = 0;

    roper_pickup = 0;

    while (levels[c0].level) {
        p0 = strlen(name) - 5;
        p1 = strlen(levels[c0].name) - 1;

        for (; p1 >= 0; p1--, p0--) {
            if (tolower(name[p0]) != tolower(levels[c0].name[p1]))
                break;
        }

        if (p1 < 0) {
            level_index = c0;
            DONT_load = levels[c0].dontload;
            return levels[c0].level;
        }

        c0++;
    }

    return 0;
}

// Dev tool: iterates all levels, generates texture clump files, then exits.
// Called from game_startup() when TEXTURE_create_clump mode is active.
// uc_orig: make_all_clumps (fallen/Source/supermap.cpp)
void make_all_clumps(void)
{
    SLONG c0 = 0;
    SLONG highest = 0;

    TEXTURE_create_clump = 1;

    TEXTURE_load_needed("levels\\frontend.ucm", 0, 256, 40);

    Levels* lptr = levels;

    while (lptr[c0].level) {
        CBYTE name[100];
        sprintf(name, "levels\\%s.ucm", lptr[c0].name);
        make_texture_clumps(name);
        if (MAV_opt_upto > highest) {
            highest = MAV_opt_upto;
        }
        c0++;
    }

    exit(0);
}
