#include <platform.h>
#include "missions/game_types.h"
#include "engine/graphics/pipeline/aeng.h"  // AENG_world_line
#include "world/environment/prim.h"   // get_prim_info
#include "ai/mav.h"
#include "world/level_pools.h"
#include "world/map/supermap.h"
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"
#include "world/map/ob.h"
#include "world/map/ob_globals.h"
#include "missions/elev.h"
#include "world/environment/ware.h"
#include "world/environment/ware_globals.h"
#include "world/map/pap.h"

// uc_orig: WARE_OFFSET_TO_ROOFTEXTURES (fallen/Source/ware.cpp)
// Byte offset into the .map file where the rooftop texture table begins.
#define WARE_OFFSET_TO_ROOFTEXTURES (4 + 128 * 128 * 12)

// Returns the floor height (in world units) at world-space position (x, z) inside warehouse ware.
// Converts world coords to mapsquare, then looks up WARE_height[].
// uc_orig: WARE_calc_height_at (fallen/Source/ware.cpp)
SLONG WARE_calc_height_at(UBYTE ware, SLONG x, SLONG z)
{
    SLONG ans;
    SLONG index;

    WARE_Ware* ww;

    ASSERT(WITHIN(ware, 1, WARE_ware_upto - 1));

    ww = &WARE_ware[ware];

    x >>= 8;
    z >>= 8;

    if (WITHIN(x, ww->minx, ww->maxx) && WITHIN(z, ww->minz, ww->maxz)) {
        x -= ww->minx;
        z -= ww->minz;

        index = x * ww->nav_pitch + z;

        ASSERT(WITHIN(ww->height + index, 0, WARE_height_upto - 1));

        ans = WARE_height[ww->height + index] << 6;

        return ans;
    }

    return 0;
}

// Computes the axis-aligned bounding box of building dbuilding by scanning STOREY_TYPE_NORMAL facets.
// bx1/bz1 are the minimum coords, bx2/bz2 are the exclusive maximum coords.
// uc_orig: WARE_bounding_box (fallen/Source/ware.cpp)
static void WARE_bounding_box(SLONG dbuilding, SLONG* bx1, SLONG* bz1, SLONG* bx2, SLONG* bz2)
{
    SLONG i;
    SLONG x, z;
    SLONG x1, z1, x2, z2;

    DBuilding* db;
    DFacet* df;

    x1 = +UC_INFINITY;
    z1 = +UC_INFINITY;
    x2 = -UC_INFINITY;
    z2 = -UC_INFINITY;

    ASSERT(WITHIN(dbuilding, 1, next_dbuilding - 1));

    db = &dbuildings[dbuilding];

    for (i = db->StartFacet; i < db->EndFacet; i++) {
        ASSERT(WITHIN(i, 1, next_dfacet - 1));

        df = &dfacets[i];

        if (df->FacetType == STOREY_TYPE_NORMAL) {
            x = df->x[0]; z = df->z[0];
            if (x < x1) x1 = x;
            if (x > x2) x2 = x;
            if (z < z1) z1 = z;
            if (z > z2) z2 = z;

            x = df->x[1]; z = df->z[1];
            if (x < x1) x1 = x;
            if (x > x2) x2 = x;
            if (z < z1) z1 = z;
            if (z > z2) z2 = z;
        }
    }

    *bx1 = x1;
    *bz1 = z1;
    *bx2 = x2;
    *bz2 = z2;
}

// Initializes all warehouse data structures from the current level.
// Loads rooftop textures from the .map file, computes navigation and height arrays,
// and marks PAP_lo squares with PAP_LO_FLAG_WAREHOUSE.
// uc_orig: WARE_init (fallen/Source/ware.cpp)
void WARE_init()
{
    SLONG i, j;
    SLONG x, z;
    SLONG x1, z1, x2, z2;
    SLONG mx, mz;
    SLONG door_x, door_y, door_z, door_angle;
    SLONG dx, dy, dz;
    SLONG rx, rz;
    SLONG sx, sz;
    SLONG bx1, bz1, bx2, bz2;
    SLONG nav_memory;
    SLONG facet;
    SLONG index;
    SLONG walkable;

    DBuilding* db;
    DFacet* df;
    WARE_Ware* ww;
    OB_Info* oi;
    PrimInfo* pi;
    RoofFace4* p_f4;
    DWalkable* p_walk;

    // Load the rooftop texture table from the .map file (rename .iam → .map).
    {
        CBYTE* ch;

        for (ch = ELEV_last_map_loaded; *ch; ch++)
            ;

        ASSERT(ch[-3] == 'i' || ch[-3] == 'I');
        ASSERT(ch[-2] == 'a' || ch[-2] == 'A');
        ASSERT(ch[-1] == 'm' || ch[-1] == 'M');

        ch[-3] = 'm';
        ch[-2] = 'a';
        ch[-1] = 'p';

        FILE* handle = MF_Fopen(ELEV_last_map_loaded, "rb");

        if (!handle) {
            memset(WARE_roof_tex, 0, sizeof(UWORD) * 128 * 128);
        } else {
            fseek(handle, WARE_OFFSET_TO_ROOFTEXTURES, SEEK_SET);
            fread(WARE_roof_tex, sizeof(UWORD), 128 * 128, handle);
            MF_Fclose(handle);
        }
    }

    WARE_in = NULL;

    MAV_calc_height_array(UC_TRUE);

    memset(WARE_ware, 0, sizeof(WARE_Ware) * WARE_MAX_WARES);
    WARE_ware_upto = 1;

    memset(WARE_nav, 0, sizeof(UWORD) * WARE_MAX_NAVS);
    WARE_nav_upto = 0;

    memset(WARE_height, 0, sizeof(UBYTE) * WARE_MAX_HEIGHTS);
    WARE_height_upto = 0;

    memset(WARE_rooftex, 0, sizeof(UWORD) * WARE_MAX_ROOFTEXES);
    WARE_rooftex_upto = 0;

    // Clear all warehouse flags from the PAP lo-res grid.
    for (mx = 0; mx < PAP_SIZE_LO; mx++)
        for (mz = 0; mz < PAP_SIZE_LO; mz++)
            PAP_2LO(mx, mz).Flag &= ~PAP_LO_FLAG_WAREHOUSE;

    for (i = 1; i < next_dbuilding; i++) {
        db = &dbuildings[i];

        if (db->Type != BUILDING_TYPE_WAREHOUSE)
            continue;

        if (!WITHIN(WARE_ware_upto, 1, WARE_MAX_WARES - 1))
            return;

        ww = &WARE_ware[WARE_ware_upto];

        ww->building = i;
        db->Ware = WARE_ware_upto;

        WARE_bounding_box(i, &bx1, &bz1, &bx2, &bz2);

        ww->minx = bx1;
        ww->minz = bz1;
        ww->maxx = bx2 - 1; // bounding box is exclusive, convert to inclusive
        ww->maxz = bz2 - 1;

        // Mark PAP_lo squares that contain hidden (warehouse interior) hi-res squares.
        for (mx = bx1; mx < bx2; mx++)
            for (mz = bz1; mz < bz2; mz++)
                if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN)
                    PAP_2LO(mx >> 2, mz >> 2).Flag |= PAP_LO_FLAG_WAREHOUSE;

        // Collect up to WARE_MAX_DOORS entrance/exit door pairs from STOREY_TYPE_DOOR facets.
        ww->door_upto = 0;

        for (facet = db->StartFacet; facet < db->EndFacet; facet++) {
            df = &dfacets[facet];

            if (df->FacetType == STOREY_TYPE_DOOR) {
                x1 = df->x[0] << 8;
                z1 = df->z[0] << 8;
                x2 = df->x[1] << 8;
                z2 = df->z[1] << 8;

                dx = x2 - x1 >> 1;
                dz = z2 - z1 >> 1;

                if (!WITHIN(ww->door_upto, 0, WARE_MAX_DOORS - 1))
                    break;

                // Outside mapsquare is perpendicular-offset outward from door midpoint.
                // Inside mapsquare is perpendicular-offset inward.
                ww->door[ww->door_upto].out_x = x1 + dx + dz >> 8;
                ww->door[ww->door_upto].out_z = z1 + dz - dx >> 8;

                ww->door[ww->door_upto].in_x = x1 + dx - dz >> 8;
                ww->door[ww->door_upto].in_z = z1 + dz + dx >> 8;

                ww->door_upto += 1;
            }
        }

        ww->nav_pitch = bz2 - bz1;
        ww->nav = WARE_nav_upto;

        nav_memory = (bx2 - bx1) * (bz2 - bz1);

        ASSERT(WARE_nav_upto + nav_memory <= WARE_MAX_NAVS);
        WARE_nav_upto += nav_memory;

        MAV_precalculate_warehouse_nav(WARE_ware_upto);

        ASSERT(WARE_height_upto + nav_memory <= WARE_MAX_HEIGHTS);
        ww->height = WARE_height_upto;
        WARE_height_upto += nav_memory;

        for (x = ww->minx; x <= ww->maxx; x++)
            for (z = ww->minz; z <= ww->maxz; z++) {
                index = (x - ww->minx) * ww->nav_pitch + (z - ww->minz);

                ASSERT(WITHIN(ww->height + index, 0, WARE_height_upto - 1));

                WARE_height[ww->height + index] = MAVHEIGHT(x, z);
            }

        ww->rooftex = WARE_rooftex_upto;

        for (walkable = db->Walkable; walkable; walkable = p_walk->Next) {
            p_walk = &dwalkables[walkable];

            for (j = p_walk->StartFace4; j < p_walk->EndFace4; j++) {
                p_f4 = &roof_faces4[j];

                ASSERT(WITHIN(WARE_rooftex_upto, 0, WARE_MAX_ROOFTEXES - 1));
                ASSERT(WITHIN(p_f4->RX & 127, 0, 127));
                ASSERT(WITHIN(p_f4->RZ & 127, 0, 127));

                WARE_rooftex[WARE_rooftex_upto++] = WARE_roof_tex[p_f4->RX & 127][p_f4->RZ & 127];
            }
        }

        WARE_ware_upto++;
    }

    // Tag static objects (OBs) that are physically inside warehouse interiors.
    for (i = 1; i < OB_ob_upto; i++)
        OB_ob[i].flags &= ~OB_FLAG_WAREHOUSE;

    MAV_calc_height_array(UC_FALSE);

    for (i = 1; i < WARE_ware_upto; i++) {
        ww = &WARE_ware[i];

        x1 = ww->minx >> 2;
        z1 = ww->minz >> 2;
        x2 = ww->maxx >> 2;
        z2 = ww->maxz >> 2;

        x1 -= 1; z1 -= 1;
        x2 += 1; z2 += 1;

        SATURATE(x1, 0, PAP_SIZE_LO - 1);
        SATURATE(z1, 0, PAP_SIZE_LO - 1);
        SATURATE(x2, 0, PAP_SIZE_LO - 1);
        SATURATE(z2, 0, PAP_SIZE_LO - 1);

        for (mx = x1; mx <= x2; mx++)
            for (mz = z1; mz <= z2; mz++) {
                for (oi = OB_find(mx, mz); oi->prim; oi++) {
                    PrimInfo* pi = get_prim_info(oi->prim);

                    if (oi->prim == 23) {
                        // Prim 23 is never inside; Stuart knows why.
                    } else {
                        // Find which mapsquare the centre of this prim is over.
                        if (oi->prim == PRIM_OBJ_SWITCH_OFF) {
                            sx = oi->x + (SIN(oi->yaw) >> 10) >> 8;
                            sz = oi->z + (COS(oi->yaw) >> 10) >> 8;
                        } else {
                            dx = pi->minx + pi->maxx >> 1;
                            dz = pi->minz + pi->maxz >> 1;

                            SLONG matrix[4];
                            SLONG useangle = -oi->yaw & 2047;
                            SLONG sin_yaw = SIN(useangle);
                            SLONG cos_yaw = COS(useangle);

                            matrix[0] = cos_yaw;
                            matrix[1] = -sin_yaw;
                            matrix[2] = sin_yaw;
                            matrix[3] = cos_yaw;

                            rx = MUL64(dx, matrix[0]) + MUL64(dz, matrix[1]);
                            rz = MUL64(dx, matrix[2]) + MUL64(dz, matrix[3]);

                            sx = oi->x + rx;
                            sz = oi->z + rz;

                            sx >>= 8;
                            sz >>= 8;
                        }

                        ASSERT(WITHIN(sx, 0, PAP_SIZE_HI - 1));
                        ASSERT(WITHIN(sz, 0, PAP_SIZE_HI - 1));

                        if (oi->y + pi->maxy < (MAVHEIGHT(sx, sz) << 6) - 0x20)
                            OB_ob[oi->index].flags |= OB_FLAG_WAREHOUSE;
                    }
                }
            }
    }
}

// WARE_enter and WARE_exit are commented out in the pre-release source.
// The ambient light save/restore logic was removed; only WARE_in and cache invalidation remain.
// Implementations were behind /* */ in the original and incomplete.

// uc_orig: WARE_enter (fallen/Source/ware.cpp)
void WARE_enter(SLONG building)
{
    DBuilding* db;

    ASSERT(WITHIN(building, 1, next_dbuilding - 1));

    db = &dbuildings[building];

    ASSERT(db->Type == BUILDING_TYPE_WAREHOUSE);
    ASSERT(WITHIN(db->Ware, 1, WARE_ware_upto - 1));

    WARE_in = db->Ware;

    NIGHT_destroy_all_cached_info();
}

// uc_orig: WARE_exit (fallen/Source/ware.cpp)
void WARE_exit(void)
{
    WARE_in = NULL;

    NIGHT_destroy_all_cached_info();
}

// uc_orig: WARE_mav_enter (fallen/Source/ware.cpp)
MAV_Action WARE_mav_enter(Thing* p_person, UBYTE ware, UBYTE caps)
{
    UBYTE i;
    UBYTE best_door;
    SLONG best_dist = 0xffff;
    SLONG dx, dz, dist;

    MAV_Action ans;
    WARE_Ware* ww;

    ASSERT(WITHIN(ware, 1, WARE_ware_upto - 1));

    ww = &WARE_ware[ware];

    for (i = 0; i < ww->door_upto; i++) {
        dx = ww->door[i].out_x - (p_person->WorldPos.X >> 16);
        dz = ww->door[i].out_z - (p_person->WorldPos.Z >> 16);

        dist = abs(dx) + abs(dz);

        if (best_dist > dist) {
            best_dist = dist;
            best_door = i;
        }
    }

    if (best_dist == 0) {
        // Already at the entrance square; walk into the warehouse.
        ans.dest_x = ww->door[best_door].in_x;
        ans.dest_z = ww->door[best_door].in_z;
        ans.action = MAV_ACTION_GOTO;
        ans.dir = 0;
    } else {
        ans = MAV_do(
            p_person->WorldPos.X >> 16,
            p_person->WorldPos.Z >> 16,
            ww->door[best_door].out_x,
            ww->door[best_door].out_z,
            caps);
    }

    return ans;
}

// uc_orig: WARE_mav_inside (fallen/Source/ware.cpp)
MAV_Action WARE_mav_inside(Thing* p_person, UBYTE dest_x, UBYTE dest_z, UBYTE caps)
{
    UBYTE start_x, start_z;

    UWORD* old_mav;
    UWORD old_mav_pitch;

    MAV_Action ma;
    WARE_Ware* ww;

    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_WAREHOUSE);
    ASSERT(WITHIN(p_person->Genus.Person->Ware, 1, WARE_ware_upto - 1));

    ww = &WARE_ware[p_person->Genus.Person->Ware];

    ASSERT(WARE_in_floorplan(p_person->Genus.Person->Ware, dest_x, dest_z));

    // Temporarily swap MAV to use this warehouse's private nav grid.
    old_mav = MAV_nav;
    old_mav_pitch = MAV_nav_pitch;

    MAV_nav = &WARE_nav[ww->nav];
    MAV_nav_pitch = ww->nav_pitch;

    // Convert to warehouse-local coordinates.
    start_x = (p_person->WorldPos.X >> 16);
    start_z = (p_person->WorldPos.Z >> 16);

    ASSERT(WARE_in_floorplan(p_person->Genus.Person->Ware, start_x, start_z));

    start_x -= ww->minx;
    start_z -= ww->minz;

    dest_x -= ww->minx;
    dest_z -= ww->minz;

    ma = MAV_do(start_x, start_z, dest_x, dest_z, caps);

    // Convert result back to world mapsquare coordinates.
    ma.dest_x += ww->minx;
    ma.dest_z += ww->minz;

    MAV_nav = old_mav;
    MAV_nav_pitch = old_mav_pitch;

    return ma;
}

// uc_orig: WARE_mav_exit (fallen/Source/ware.cpp)
MAV_Action WARE_mav_exit(Thing* p_person, UBYTE caps)
{
    UBYTE i;
    UBYTE best_door = 0;
    UBYTE start_x, start_z;
    UBYTE dest_x, dest_z;
    SLONG best_dist = 0xffff;
    SLONG dx, dz, dist;

    UWORD* old_mav;
    UWORD old_mav_pitch;

    MAV_Action ans;
    WARE_Ware* ww;

    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_WAREHOUSE);
    ASSERT(WITHIN(p_person->Genus.Person->Ware, 1, WARE_ware_upto - 1));

    ww = &WARE_ware[p_person->Genus.Person->Ware];

    old_mav = MAV_nav;
    old_mav_pitch = MAV_nav_pitch;

    MAV_nav = &WARE_nav[ww->nav];
    MAV_nav_pitch = ww->nav_pitch;

    // Find the nearest exit door reachable at the same floor level.
    for (i = 0; i < ww->door_upto; i++) {
        dx = ww->door[i].in_x - (p_person->WorldPos.X >> 16);
        dz = ww->door[i].in_z - (p_person->WorldPos.Z >> 16);

        dist = abs(dx) + abs(dz);

        SLONG height_in = WARE_calc_height_at(
            p_person->Genus.Person->Ware,
            (ww->door[i].in_x << 8) + 0x80,
            (ww->door[i].in_z << 8) + 0x80);

        SLONG height_out = PAP_calc_map_height_at(
            (ww->door[i].out_x << 8) + 0x80,
            (ww->door[i].out_z << 8) + 0x80);

        // Only use this door if it's at the same floor level as the outside.
        if (abs(height_out - height_in) < 0x80) {
            if (best_dist > dist) {
                best_dist = dist;
                best_door = i;
            }
        }
    }

    if (best_dist == 0) {
        // Already at the exit square; walk outside.
        ans.dest_x = ww->door[best_door].out_x;
        ans.dest_z = ww->door[best_door].out_z;
        ans.action = MAV_ACTION_GOTO;
        ans.dir = 0;
    } else {
        start_x = (p_person->WorldPos.X >> 16);
        start_z = (p_person->WorldPos.Z >> 16);

        ASSERT(WARE_in_floorplan(p_person->Genus.Person->Ware, start_x, start_z));

        start_x -= ww->minx;
        start_z -= ww->minz;

        dest_x = ww->door[best_door].in_x - ww->minx;
        dest_z = ww->door[best_door].in_z - ww->minz;

        ans = MAV_do(start_x, start_z, dest_x, dest_z, caps);

        ans.dest_x += ww->minx;
        ans.dest_z += ww->minz;
    }

    MAV_nav = old_mav;
    MAV_nav_pitch = old_mav_pitch;

    return ans;
}

// uc_orig: WARE_in_floorplan (fallen/Source/ware.cpp)
SLONG WARE_in_floorplan(UBYTE ware, UBYTE x, UBYTE z)
{
    ASSERT(WITHIN(ware, 1, WARE_ware_upto - 1));

    WARE_Ware* ww = &WARE_ware[ware];

    if (WITHIN(x, ww->minx, ww->maxx) && WITHIN(z, ww->minz, ww->maxz)) {
        if (PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN)
            return UC_TRUE;
    }

    return UC_FALSE;
}

// uc_orig: WARE_debug (fallen/Source/ware.cpp)
void WARE_debug(void)
{
    SLONG i, j, k;
    SLONG x1, y1, z1, x2, y2, z2;
    WARE_Ware* ww;

    if (!Keys[KB_T])
        return;

    for (i = 1; i < WARE_ware_upto; i++) {
        ww = &WARE_ware[i];

        // Draw door connections (green line from outside to inside mapsquare).
        for (j = 0; j < ww->door_upto; j++) {
            x1 = (ww->door[j].in_x << 8) + 0x80;
            z1 = (ww->door[j].in_z << 8) + 0x80;
            x2 = (ww->door[j].out_x << 8) + 0x80;
            z2 = (ww->door[j].out_z << 8) + 0x80;

            y1 = PAP_calc_height_at(x1, z1);
            y2 = PAP_calc_height_at(x2, z2);

            AENG_world_line(x1, y1, z1, 00, 0xffffff, x2, y2, z2, 16, 0x00ff00, UC_TRUE);
        }

        // Draw MAV navigation arrows for each warehouse cell.
        {
            SLONG x, z;
            SLONG mx, mz;
            SLONG dx, dz;
            SLONG lx, lz;
            SLONG index;

            MAV_Opt* mo;

            struct { SLONG dx; SLONG dz; } order[4] = {
                { -1, 0 }, { +1, 0 }, { 0, -1 }, { 0, +1 }
            };

            ULONG colour[7] = {
                0x00ff0000, 0x0000ff00, 0x000000ff,
                0x00ffff00, 0x00ff00ff, 0x0000ffff, 0x00ffaa88
            };

            for (x = ww->minx; x <= ww->maxx; x++)
                for (z = ww->minz; z <= ww->maxz; z++) {
                    x1 = x + 0 << 8;
                    z1 = z + 0 << 8;
                    x2 = x + 1 << 8;
                    z2 = z + 1 << 8;

                    y1 = y2 = WARE_calc_height_at(i, (x << 8) + 0x80, (z << 8) + 0x80);

                    AENG_world_line(x1, y1, z1, 4, 0x00000077, x2, y2, z2, 4, 0x00000077, UC_TRUE);
                    AENG_world_line(x2, y1, z1, 4, 0x00000077, x1, y2, z2, 4, 0x00000077, UC_TRUE);

                    index = (x - ww->minx) * ww->nav_pitch + (z - ww->minz);

                    ASSERT(WITHIN(ww->nav + index, 0, WARE_nav_upto - 1));

                    mo = &MAV_opt[WARE_nav[ww->nav + index]];

                    mx = x1 + x2 >> 1;
                    mz = z1 + z2 >> 1;

                    for (j = 0; j < 4; j++) {
                        dx = order[j].dx;
                        dz = order[j].dz;

                        lx = mx + dx * 96;
                        lz = mz + dz * 96;

                        lx += dz * (16 * 3);
                        lz += -dx * (16 * 3);

                        for (k = 0; k < 8; k++) {
                            if (mo->opt[j] & (1 << k))
                                AENG_world_line(mx, y1, mz, 0, 0, lx, y2, lz, 9, colour[k], UC_TRUE);

                            lx += -dz * 16;
                            lz += +dx * 16;
                        }
                    }
                }
        }
    }
}

// uc_orig: WARE_inside (fallen/Source/ware.cpp)
SLONG WARE_inside(UBYTE ware, SLONG x, SLONG y, SLONG z)
{
    SLONG index;
    SLONG height;

    WARE_Ware* ww;

    ASSERT(WITHIN(ware, 1, WARE_ware_upto - 1));

    ww = &WARE_ware[ware];

    x >>= 8;
    z >>= 8;

    if (!WITHIN(x, ww->minx, ww->maxx) || !WITHIN(z, ww->minz, ww->maxz) || !(PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN))
        return UC_TRUE; // Outside the building floor plan.

    x -= ww->minx;
    z -= ww->minz;

    index = x * ww->nav_pitch + z;

    ASSERT(WITHIN(ww->height + index, 0, WARE_height_upto - 1));

    height = WARE_height[ww->height + index] << 6;

    return y < height;
}

// uc_orig: WARE_which_contains (fallen/Source/ware.cpp)
SLONG WARE_which_contains(UBYTE x, UBYTE z)
{
    SLONG i;

    for (i = 1; i < WARE_ware_upto; i++)
        if (WARE_in_floorplan(i, x, z))
            return i;

    return NULL;
}

// uc_orig: WARE_get_caps (fallen/Source/ware.cpp)
UBYTE WARE_get_caps(UBYTE ware, UBYTE x, UBYTE z, UBYTE dir)
{
    SLONG index;
    SLONG mo_index;

    WARE_Ware* ww;

    ASSERT(WITHIN(ware, 1, WARE_ware_upto - 1));

    ww = &WARE_ware[ware];

    if (!WITHIN(x, ww->minx, ww->maxx) || !WITHIN(z, ww->minz, ww->maxz))
        return 0;

    x -= ww->minx;
    z -= ww->minz;

    index = x * ww->nav_pitch + z;

    ASSERT(WITHIN(ww->nav + index, 0, WARE_nav_upto - 1));

    mo_index = WARE_nav[ww->nav + index];

    ASSERT(WITHIN(mo_index, 0, MAV_opt_upto - 1));

    return MAV_opt[mo_index].opt[dir];
}
