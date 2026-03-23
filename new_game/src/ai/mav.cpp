// Game.h first: brings in MFStdLib.h which is needed by supermap.h (MFFileHandle),
// and anim.h (strcpy via string.h chain).
#include "fallen/Headers/Game.h"          // Temporary: PAP_FLAG_*, SIN/COS/MUL64, RoofFace4, etc.

#include "ai/mav.h"
#include "ai/mav_globals.h"

#include "world/map/pap.h"
#include "world/map/supermap.h"
#include "world/map/ob.h"
#include "world/navigation/walkable.h"
#include "world/environment/ware.h"
#include "world/environment/ware_globals.h"

#include "fallen/Headers/collide.h"       // Temporary: there_is_a_los, los_failure_dfacet, two4_line_intersection, find_nearby_ladder_colvect_radius
#include "fallen/Headers/prim.h"          // Temporary: PrimInfo, get_prim_info, does_fence_lie_along_line, RFACE_FLAG_NODRAW, RoofFace4
#include "fallen/Headers/building.h"      // Temporary: BUILDING_TYPE_WAREHOUSE, DBuilding, STOREY_TYPE_FENCE_FLAT, STOREY_TYPE_LADDER, FACET_FLAG_UNCLIMBABLE
#include "fallen/Headers/memory.h"        // Temporary: roof_faces4, next_dbuilding, next_dwalkable, dwalkables, next_dfacet

// TRACE was a no-op debug print macro in the original release build.
#ifndef TRACE
#define TRACE(...)
#endif

// uc_orig: MAV_init (fallen/Source/mav.cpp)
void MAV_init()
{
    SLONG i;

    MAV_nav_pitch = 128;
    MAV_opt_upto = 0;

    // Pre-populate entries 0-15 with all combinations of 4-direction plain-walk passability.
    // INSIDE2_mav_nav_calc() addresses these entries by fixed index — don't change this loop.
    for (i = 0; i < 16; i++) {
        MAV_opt[i].opt[0] = (i & 1) ? 0 : MAV_CAPS_GOTO;
        MAV_opt[i].opt[1] = (i & 2) ? 0 : MAV_CAPS_GOTO;
        MAV_opt[i].opt[2] = (i & 4) ? 0 : MAV_CAPS_GOTO;
        MAV_opt[i].opt[3] = (i & 8) ? 0 : MAV_CAPS_GOTO;
    }

    MAV_opt_upto = 16;
}

// Deduplicate MAV_Opt records: scan existing entries for an identical 4-byte set,
// reuse if found, otherwise append a new entry. Caps at MAV_MAX_OPTS=1024.
// uc_orig: StoreMavOpts (fallen/Source/mav.cpp)
void StoreMavOpts(SLONG x, SLONG z, UBYTE* opt)
{
    for (SLONG ii = 0; ii < MAV_opt_upto; ii++) {
        if ((MAV_opt[ii].opt[0] == opt[0]) && (MAV_opt[ii].opt[1] == opt[1]) && (MAV_opt[ii].opt[2] == opt[2]) && (MAV_opt[ii].opt[3] == opt[3])) {
            SET_MAV_NAV(x, z, ii);
            return;
        }
    }

    if (MAV_opt_upto == MAV_MAX_OPTS) {
        ASSERT(0);
    } else {
        MAV_opt[MAV_opt_upto].opt[0] = opt[0];
        MAV_opt[MAV_opt_upto].opt[1] = opt[1];
        MAV_opt[MAV_opt_upto].opt[2] = opt[2];
        MAV_opt[MAV_opt_upto].opt[3] = opt[3];

        SET_MAV_NAV(x, z, MAV_opt_upto);

        MAV_opt_upto++;
    }
}

// uc_orig: MAV_calc_height_array (fallen/Source/mav.cpp)
void MAV_calc_height_array(SLONG ignore_warehouses)
{
    SLONG i;
    SLONG j;

    SLONG x;
    SLONG z;

    SLONG mx;
    SLONG mz;

    SLONG height;
    SLONG walk;

    DBuilding* db;
    RoofFace4* rf;
    DWalkable* dw;
    PAP_Hi* ph;

    // First pass: ground cells get terrain height; hidden (building interior) cells get -127.
    for (x = 0; x < PAP_SIZE_HI; x++)
        for (z = 0; z < PAP_SIZE_HI; z++) {
            mx = x << 8;
            mz = z << 8;

            mx += 0x80;
            mz += 0x80;

            if ((PAP_2HI(x, z).Flags & PAP_FLAG_ROOF_EXISTS)) {
                // Roof-face height will be filled in the second pass.
            } else if ((PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN) && !ignore_warehouses) {
                MAVHEIGHT(x, z) = -127;
            } else {
                height = PAP_calc_height_at(mx, mz);

                // Convert to SBYTE coordinate system: 1 unit = 0x40 game-units.
                height /= 0x40;
                SATURATE(height, -127, +127);
                MAVHEIGHT(x, z) = height;
            }
        }

    // Second pass: use walkable roof-face data to populate height for building-top cells.
    for (i = 1; i < next_dbuilding; i++) {
        db = &dbuildings[i];

        if (db->Type == BUILDING_TYPE_WAREHOUSE) {
            if (ignore_warehouses) {
                continue;
            }
        }

        for (walk = db->Walkable; walk; walk = dw->Next) {
            dw = &dwalkables[walk];

            for (j = dw->StartFace4; j < dw->EndFace4; j++) {
                rf = &roof_faces4[j];

                // Clear any stale NODRAW flag — will be set properly later.
                rf->DrawFlags &= ~RFACE_FLAG_NODRAW;

                if (db->Type == BUILDING_TYPE_WAREHOUSE) {
                    // Mark this mapsquare as inside a warehouse (top bit of Texture).
                    ph = &PAP_2HI(rf->RX & 127, rf->RZ & 127);
                    ph->Texture |= 1 << 15;
                }

                height = rf->Y;
                height /= 0x40;
                SATURATE(height, -127, +127);

                if (MAVHEIGHT(rf->RX & 127, rf->RZ & 127) < height) {
                    MAVHEIGHT(rf->RX & 127, rf->RZ & 127) = height;
                }
            }
        }
    }

    // Final pass: hidden cells with no roof data (height still -127) have no walkable surface —
    // mark as NOGO and assign max height to avoid blocking above-ground movement.
    for (x = 0; x < PAP_SIZE_HI; x++)
        for (z = 0; z < PAP_SIZE_HI; z++) {
            if (MAVHEIGHT(x, z) <= -127) {
                if (PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN) {
                    MAVHEIGHT(x, z) = 127;
                    PAP_2HI(x, z).Flags |= PAP_FLAG_NOGO;
                }
            }
        }
}

// uc_orig: MAV_turn_off_square (fallen/Source/mav.cpp)
void MAV_turn_off_square(
    SLONG x,
    SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    UBYTE opt[4];

    SLONG i;
    SLONG j;

    SLONG mx;
    SLONG mz;

    for (i = 0; i < 4; i++) {
        mx = x - order[i].dx;
        mz = z - order[i].dz;

        if (WITHIN(mx, 0, MAP_WIDTH - 1) && WITHIN(mz, 0, MAP_HEIGHT - 1)) {
            ASSERT(WITHIN(MAV_NAV(mx, mz), 0, MAV_opt_upto - 1));

            opt[0] = MAV_opt[MAV_NAV(mx, mz)].opt[0];
            opt[1] = MAV_opt[MAV_NAV(mx, mz)].opt[1];
            opt[2] = MAV_opt[MAV_NAV(mx, mz)].opt[2];
            opt[3] = MAV_opt[MAV_NAV(mx, mz)].opt[3];

            opt[i] &= ~MAV_CAPS_GOTO;

            StoreMavOpts(mx, mz, opt);
        }
    }
}

// uc_orig: MAV_turn_off_whole_square (fallen/Source/mav.cpp)
void MAV_turn_off_whole_square(
    SLONG x,
    SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    UBYTE opt[4];

    SLONG i;
    SLONG j;

    SLONG mx;
    SLONG mz;

    for (i = 0; i < 4; i++) {
        mx = x - order[i].dx;
        mz = z - order[i].dz;

        if (WITHIN(mx, 0, MAP_WIDTH - 1) && WITHIN(mz, 0, MAP_HEIGHT - 1)) {
            ASSERT(WITHIN(MAV_NAV(mx, mz), 0, MAV_opt_upto - 1));

            opt[0] = MAV_opt[MAV_NAV(mx, mz)].opt[0];
            opt[1] = MAV_opt[MAV_NAV(mx, mz)].opt[1];
            opt[2] = MAV_opt[MAV_NAV(mx, mz)].opt[2];
            opt[3] = MAV_opt[MAV_NAV(mx, mz)].opt[3];

            opt[i] = 0;

            StoreMavOpts(mx, mz, opt);
        }
    }
}

// uc_orig: MAV_turn_off_whole_square_car (fallen/Source/mav.cpp)
void MAV_turn_off_whole_square_car(
    SLONG x,
    SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    SLONG i;

    SLONG mx;
    SLONG mz;

    for (i = 0; i < 4; i++) {
        mx = x - order[i].dx;
        mz = z - order[i].dz;

        if (WITHIN(mx, 0, MAP_WIDTH - 1) && WITHIN(mz, 0, MAP_HEIGHT - 1)) {
            UBYTE caropt = MAV_CAR(mx, mz);

            caropt &= ~(1 << i);

            SET_MAV_CAR(mx, mz, caropt);
        }
    }
}

// uc_orig: MAV_remove_facet_car (fallen/Source/mav.cpp)
void MAV_remove_facet_car(SLONG x1, SLONG z1, SLONG x2, SLONG z2)
{
    if (x1 == x2) {
        if (z1 > z2)
            SWAP(z1, z2);

        for (SLONG z = z1; z < z2; z++) {
            UBYTE caropt;

            if (x1 > 0) {
                caropt = MAV_CAR(x1 - 1, z);
                caropt &= ~(1 << MAV_DIR_XL);
                SET_MAV_CAR(x1 - 1, z, caropt);
            }
            if (x1 < MAP_WIDTH) {
                caropt = MAV_CAR(x1, z);
                caropt &= ~(1 << MAV_DIR_XS);
                SET_MAV_CAR(x1, z, caropt);
            }
        }
    } else {
        ASSERT(z1 == z2);
        if (x1 > x2)
            SWAP(x1, x2);

        for (SLONG x = x1; x < x2; x++) {
            UBYTE caropt;

            if (z1 > 0) {
                caropt = MAV_CAR(x, z1 - 1);
                caropt &= ~(1 << MAV_DIR_ZL);
                SET_MAV_CAR(x, z1 - 1, caropt);
            }
            if (z1 < MAP_HEIGHT) {
                caropt = MAV_CAR(x, z1);
                caropt &= ~(1 << MAV_DIR_ZS);
                SET_MAV_CAR(x, z1, caropt);
            }
        }
    }
}

// uc_orig: MAV_turn_movement_off (fallen/Source/mav.cpp)
void MAV_turn_movement_off(UBYTE mx, UBYTE mz, UBYTE dir)
{
    SLONG j;
    SLONG mo_index;
    MAV_Opt mo;

    mo_index = MAV_NAV(mx, mz);

    ASSERT(WITHIN(mo_index, 0, MAV_opt_upto - 1));

    mo = MAV_opt[mo_index];

    mo.opt[dir] &= ~MAV_CAPS_GOTO;

    StoreMavOpts(mx, mz, mo.opt);
}

// uc_orig: MAV_turn_movement_on (fallen/Source/mav.cpp)
void MAV_turn_movement_on(UBYTE mx, UBYTE mz, UBYTE dir)
{
    SLONG j;
    SLONG mo_index;
    MAV_Opt mo;

    mo_index = MAV_NAV(mx, mz);

    ASSERT(WITHIN(mo_index, 0, MAV_opt_upto - 1));

    mo = MAV_opt[mo_index];

    mo.opt[dir] = MAV_CAPS_GOTO;

    StoreMavOpts(mx, mz, mo.opt);
}

// uc_orig: MAV_precalculate (fallen/Source/mav.cpp)
void MAV_precalculate()
{
    SLONG i;

    SLONG x;
    SLONG y;
    SLONG z;

    SLONG x1;
    SLONG y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG dx;
    SLONG dz;

    SLONG tx;
    SLONG tz;

    SLONG rx;
    SLONG rz;

    SLONG dh;

    SLONG useangle;
    SLONG matrix[4];
    SLONG ladder;

    SLONG sin_yaw;
    SLONG cos_yaw;

    SLONG both_ground;

    OB_Info* oi;

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    UBYTE opt[4];

    MAV_calc_height_array(UC_FALSE);

    // Adjust MAVHEIGHT under staircase prims (prim 41) using the prim's actual Y position.
    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            for (oi = OB_find(x, z); oi->prim; oi++) {
                if (oi->prim == 41) {
                    if (!(oi->flags & OB_FLAG_WAREHOUSE)) {
                        PrimInfo* pi = get_prim_info(oi->prim);

                        SLONG mx = pi->minx + pi->maxx >> 1;
                        SLONG mz = pi->minz + pi->maxz >> 1;

                        SLONG matrix[4];
                        SLONG useangle;

                        SLONG sin_yaw;
                        SLONG cos_yaw;

                        SLONG rx;
                        SLONG rz;

                        SLONG sx;
                        SLONG sz;

                        useangle = -oi->yaw;
                        useangle &= 2047;

                        sin_yaw = SIN(useangle);
                        cos_yaw = COS(useangle);

                        matrix[0] = cos_yaw;
                        matrix[1] = -sin_yaw;
                        matrix[2] = sin_yaw;
                        matrix[3] = cos_yaw;

                        rx = MUL64(mx, matrix[0]) + MUL64(mz, matrix[1]);
                        rz = MUL64(mx, matrix[2]) + MUL64(mz, matrix[3]);

                        rx += oi->x;
                        rz += oi->z;

                        rx >>= 8;
                        rz >>= 8;

                        MAVHEIGHT(rx, rz) = oi->y + 0x40 >> 6;
                    }
                }
            }
        }

    // Evaluate all 4-directional edges per cell: walk, climb-over, fall-off,
    // pull-up, ladder, and jump capabilities.
    for (x = 0; x < PAP_SIZE_HI; x++)
        for (z = 0; z < PAP_SIZE_HI; z++) {
            ladder = find_nearby_ladder_colvect_radius(
                (x << 8) + 0x80,
                (z << 8) + 0x80,
                0x100);

            UBYTE caropts = 0;

            for (i = 0; i < 4; i++) {
                opt[i] = 0;

                dx = order[i].dx;
                dz = order[i].dz;

                tx = x + dx;
                tz = z + dz;

                if (WITHIN(tx, 0, PAP_SIZE_HI - 1) && WITHIN(tz, 0, PAP_SIZE_HI - 1)) {
                    if (!(PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN) && !(PAP_2HI(tx, tz).Flags & PAP_FLAG_HIDDEN)) {
                        both_ground = UC_TRUE;
                    } else {
                        both_ground = UC_FALSE;
                    }

                    // Height difference threshold: ground cells ±2, building-top cells ±1.
                    dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                    if ((!both_ground && abs(dh) > 1) || (abs(dh) > 2)) {
                        if (dh < 0) {
                            x1 = (x << 8) + 0x80;
                            z1 = (z << 8) + 0x80;
                            x2 = (tx << 8) + 0x80;
                            z2 = (tz << 8) + 0x80;

                            y1 = PAP_calc_map_height_at(x1, z1) + 0x50;
                            y2 = PAP_calc_map_height_at(x2, z2) + 0x50;

                            y = MAX(y1, y2);

                            if (there_is_a_los(
                                    x1, y, z1,
                                    x2, y, z2,
                                    LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                                opt[i] |= MAV_CAPS_FALL_OFF;
                            } else {
                                DFacet* df = &dfacets[los_failure_dfacet];

                                if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                                    if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                                        // Unclimbable fence.
                                    } else {
                                        opt[i] |= MAV_CAPS_CLIMB_OVER;
                                    }
                                }
                            }

                            if (there_is_a_los(x1, y, z1, x2, y, z2,
                                    LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                                caropts |= 1 << i;
                            }
                        } else {
                            // Pull-up: neighbour is 2-4 MAVHEIGHT units higher with no fence along the edge.
                            if (WITHIN(dh, 2, 4)) {
                                SLONG fx1;
                                SLONG fz1;
                                SLONG fx2;
                                SLONG fz2;

                                fx1 = (x << 8) + 0x80 + (dx << 7) - (dz << 7);
                                fz1 = (z << 8) + 0x80 + (dz << 7) + (dx << 7);

                                fx2 = (x << 8) + 0x80 + (dx << 7) + (dz << 7);
                                fz2 = (z << 8) + 0x80 + (dz << 7) - (dx << 7);

                                if (does_fence_lie_along_line(
                                        fx1, fz1,
                                        fx2, fz2)) {
                                    // Fence blocks pull-up.
                                } else {
                                    opt[i] |= MAV_CAPS_PULLUP;
                                }
                            }

                            if (ladder) {
                                DFacet* df_ladder;

                                ASSERT(WITHIN(ladder, 1, next_dfacet - 1));

                                df_ladder = &dfacets[ladder];

                                ASSERT(df_ladder->FacetType == STOREY_TYPE_LADDER);

                                x1 = (x << 8) + 0x80;
                                z1 = (z << 8) + 0x80;
                                x2 = (tx << 8) + 0x80;
                                z2 = (tz << 8) + 0x80;

                                if (two4_line_intersection(
                                        x1, z1,
                                        x2, z2,
                                        df_ladder->x[0] << 8, df_ladder->z[0] << 8,
                                        df_ladder->x[1] << 8, df_ladder->z[1] << 8)) {
                                    if (abs(df_ladder->Y[0] - PAP_calc_map_height_at(x1, z1)) <= 0x50) {
                                        opt[i] |= MAV_CAPS_LADDER_UP;
                                    }
                                }
                            }
                        }
                    } else {
                        x1 = (x << 8) + 0x80;
                        z1 = (z << 8) + 0x80;
                        x2 = (tx << 8) + 0x80;
                        z2 = (tz << 8) + 0x80;

                        y1 = PAP_calc_map_height_at(x1, z1) + 0x50;
                        y2 = PAP_calc_map_height_at(x2, z2) + 0x50;

                        y = MAX(y1, y2);

                        if (there_is_a_los(
                                x1, y, z1,
                                x2, y, z2,
                                LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                            opt[i] |= MAV_CAPS_GOTO;
                        } else {
                            DFacet* df = &dfacets[los_failure_dfacet];

                            if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                                if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                                    // Unclimbable fence.
                                } else {
                                    opt[i] |= MAV_CAPS_CLIMB_OVER;
                                }
                            }
                        }

                        if (there_is_a_los(x1, y, z1, x2, y, z2,
                                LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                            caropts |= 1 << i;
                        }
                    }

                    if (!(opt[i] & MAV_CAPS_GOTO) && !(opt[i] & MAV_CAPS_CLIMB_OVER)) {
                        // Check one-block jump.
                        tx += dx;
                        tz += dz;

                        if (WITHIN(tx, 0, MAP_WIDTH - 1) && WITHIN(tz, 0, MAP_HEIGHT - 1)) {
                            dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                            x1 = (x << 8) + 0x80;
                            z1 = (z << 8) + 0x80;
                            x2 = (tx << 8) + 0x80;
                            z2 = (tz << 8) + 0x80;

                            y1 = (MAVHEIGHT(x, z) << 6) + 0xa0;
                            y2 = (MAVHEIGHT(tx, tz) << 6) + 0xa0;

                            if (there_is_a_los(
                                    x1, y1, z1,
                                    x2, y2, z2,
                                    LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                                if (dh < 2) {
                                    opt[i] |= MAV_CAPS_JUMP;
                                } else {
                                    if (WITHIN(dh, 2, 5)) {
                                        opt[i] |= MAV_CAPS_JUMPPULL;
                                    }
                                }
                            }

                            // Check two-block jump.
                            tx += dx;
                            tz += dz;

                            if (WITHIN(tx, 0, MAP_WIDTH - 1) && WITHIN(tz, 0, MAP_HEIGHT - 1)) {
                                dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                                if (dh > 4) {
                                    // Can't make this jump.
                                } else {
                                    if (dh > -6) {
                                        x1 = (x << 8) + 0x80;
                                        z1 = (z << 8) + 0x80;
                                        x2 = (tx << 8) + 0x80;
                                        z2 = (tz << 8) + 0x80;

                                        y1 = (MAVHEIGHT(x, z) << 6) + 0xa0;
                                        y2 = (MAVHEIGHT(tx, tz) << 6) + 0xa0;

                                        if (there_is_a_los(
                                                x1, y1, z1,
                                                x2, y2, z2,
                                                LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                                            opt[i] |= MAV_CAPS_JUMPPULL2;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Commit the computed opt[4] bitmask array and car flags.
            // MAV_SPARE initialized to 3 here; overwritten later by the water-texture scan.
            StoreMavOpts(x, z, opt);
            SET_MAV_CAR(x, z, caropts);
            SET_MAV_SPARE(x, z, 3);
        }

    // Staircase (prim 41) / skylight (prim 226/227) / UCPD sign (prim 133) post-processing pass.
    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            for (oi = OB_find(x, z); oi->prim; oi++) {
                if (oi->prim == 41) {
                    if (!(oi->flags & OB_FLAG_WAREHOUSE)) {
                        PrimInfo* pi = get_prim_info(oi->prim);

                        SLONG mx = pi->minx + pi->maxx >> 1;
                        SLONG mz = pi->minz + pi->maxz >> 1;

                        SLONG matrix[4];
                        SLONG useangle;

                        SLONG sin_yaw;
                        SLONG cos_yaw;

                        SLONG rx;
                        SLONG rz;

                        SLONG sx;
                        SLONG sz;

                        useangle = -oi->yaw;
                        useangle &= 2047;

                        sin_yaw = SIN(useangle);
                        cos_yaw = COS(useangle);

                        matrix[0] = cos_yaw;
                        matrix[1] = -sin_yaw;
                        matrix[2] = sin_yaw;
                        matrix[3] = cos_yaw;

                        rx = MUL64(mx, matrix[0]) + MUL64(mz, matrix[1]);
                        rz = MUL64(mx, matrix[2]) + MUL64(mz, matrix[3]);

                        rx += oi->x;
                        rz += oi->z;

                        rx >>= 8;
                        rz >>= 8;

                        // Staircase direction derived from oi->yaw (4 cardinal orientations).
                        // Lateral movement blocked, stair direction enabled.
                        if (oi->yaw < 256 || oi->yaw > 1792 || WITHIN(oi->yaw, 768, 1280)) {
                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx - 1, rz)) {
                                // Wide staircase — lateral OK.
                            } else {
                                MAV_turn_movement_off(rx, rz, MAV_DIR_XS);
                                MAV_turn_movement_off(rx - 1, rz, MAV_DIR_XL);
                            }

                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx + 1, rz)) {
                                // Wide staircase.
                            } else {
                                MAV_turn_movement_off(rx, rz, MAV_DIR_XL);
                                MAV_turn_movement_off(rx + 1, rz, MAV_DIR_XS);
                            }

                            if (!WITHIN(oi->yaw, 768, 1280)) {
                                if (MAVHEIGHT(rx, rz + 1) <= MAVHEIGHT(rx, rz) + 3) {
                                    MAV_turn_movement_on(rx, rz, MAV_DIR_ZL);
                                    MAV_turn_movement_on(rx, rz + 1, MAV_DIR_ZS);
                                }
                            } else {
                                if (MAVHEIGHT(rx, rz - 1) <= MAVHEIGHT(rx, rz) + 3) {
                                    MAV_turn_movement_on(rx, rz, MAV_DIR_ZS);
                                    MAV_turn_movement_on(rx, rz - 1, MAV_DIR_ZL);
                                }
                            }
                        } else {
                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx, rz - 1)) {
                                // Wide staircase.
                            } else {
                                MAV_turn_movement_off(rx, rz, MAV_DIR_ZS);
                                MAV_turn_movement_off(rx, rz - 1, MAV_DIR_ZL);
                            }

                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx, rz + 1)) {
                                // Wide staircase.
                            } else {
                                MAV_turn_movement_off(rx, rz, MAV_DIR_ZL);
                                MAV_turn_movement_off(rx, rz + 1, MAV_DIR_ZS);
                            }

                            if (!WITHIN(oi->yaw, 256, 768)) {
                                if (MAVHEIGHT(rx - 1, rz) <= MAVHEIGHT(rx, rz) + 3) {
                                    MAV_turn_movement_on(rx, rz, MAV_DIR_XS);
                                    MAV_turn_movement_on(rx - 1, rz, MAV_DIR_XL);
                                }
                            } else {
                                if (MAVHEIGHT(rx + 1, rz) <= MAVHEIGHT(rx, rz) + 3) {
                                    MAV_turn_movement_on(rx, rz, MAV_DIR_XL);
                                    MAV_turn_movement_on(rx + 1, rz, MAV_DIR_XS);
                                }
                            }
                        }

                        WALKABLE_remove_rface(rx, rz);
                    }
                } else if (oi->prim == 226) {
                    // Skylight — remove the two roof faces covered by it.
                    SLONG useangle;

                    useangle = oi->yaw + 1024;
                    useangle &= 2047;

                    SLONG mx = oi->x >> 8;
                    SLONG mz = oi->z >> 8;

                    SLONG rx = mx + SIGN(SIN(useangle) >> 14);
                    SLONG rz = mz + SIGN(COS(useangle) >> 14);

                    WALKABLE_remove_rface(mx, mz);
                    WALKABLE_remove_rface(rx, rz);
                } else if (oi->prim == 227) {
                    // Large skylight — remove a 2x3 grid of roof faces.
                    SLONG i;
                    SLONG j;
                    SLONG useangle;

                    useangle = oi->yaw + 1024;
                    useangle &= 2047;

                    SLONG mx = oi->x;
                    SLONG mz = oi->z;

                    SLONG rx = SIN(useangle) >> 8;
                    SLONG rz = COS(useangle) >> 8;

                    SLONG sx;
                    SLONG sz;

                    for (i = -1; i <= 1; i += 1)
                        for (j = -1; j <= 1; j += 2) {
                            sx = mx + rx * i - (rz * j >> 1);
                            sz = mz + rz * i + (rx * j >> 1);

                            WALKABLE_remove_rface(
                                sx >> 8,
                                sz >> 8);
                        }
                } else if (oi->prim == 133) {
                    // UCPD rotating sign — block the grid square below it.
                    MAV_turn_off_whole_square(oi->x >> 8, oi->z >> 8);
                }
            }
        }

    // Slope rejection: cells where PAP_on_slope() > 100 at two corners are too steep to walk.
    {
        extern SLONG PAP_on_slope(SLONG x, SLONG z, SLONG * angle);

        SLONG mx;
        SLONG mz;

        SLONG angle;

        for (mx = 0; mx < PAP_SIZE_HI; mx++)
            for (mz = 0; mz < PAP_SIZE_HI; mz++) {
                if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN) {
                    continue;
                }

                if (PAP_on_slope((mx << 8) + 0x40, (mz << 8) + 0x40, &angle) > 100 ||
                    PAP_on_slope((mx << 8) + 0xc0, (mz << 8) + 0xc0, &angle) > 100) {
                    MAV_turn_off_whole_square(mx, mz);
                }
            }
    }

    // Remove NOGO squares from the car nav as well.
    {
        SLONG mx;
        SLONG mz;

        SLONG angle;

        for (mx = 0; mx < PAP_SIZE_HI; mx++)
            for (mz = 0; mz < PAP_SIZE_HI; mz++) {
                if (PAP_2HI(mx, mz).Flags & PAP_FLAG_NOGO) {
                    MAV_turn_off_whole_square_car(mx, mz);
                }
            }
    }

    // Water texture detection: texture pages 454, 99456, 99457 identify water tiles.
    // Sets MAV_SPARE_FLAG_WATER in the MAV_nav SPARE bits [15:14].
    {
        SLONG mx;
        SLONG mz;

        SLONG page;

        for (mx = 0; mx < PAP_SIZE_HI; mx++)
            for (mz = 0; mz < PAP_SIZE_HI; mz++) {
                page = PAP_2HI(mx, mz).Texture & 0x3ff;

                if (page == 454 || page == 99456 || page == 99457) {
                    SET_MAV_SPARE(mx, mz, MAV_SPARE_FLAG_WATER);
                } else {
                    SET_MAV_SPARE(mx, mz, 0);
                }
            }
    }
}

// Debug renderer: draws navigation options (foot-nav or car-nav) for each cell in the given
// map-square range. Uses AENG_world_line for world-space lines. Reads ControlFlag to switch
// between foot-nav and car-nav display modes.
// uc_orig: MAV_draw (fallen/Source/mav.cpp)
void MAV_draw(
    SLONG sx1, SLONG sz1,
    SLONG sx2, SLONG sz2)
{
    SLONG i;
    SLONG j;

    SLONG x;
    SLONG z;

    SLONG x1;
    SLONG y1;
    SLONG z1;
    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG mx;
    SLONG mz;

    SLONG dx;
    SLONG dz;

    SLONG lx;
    SLONG lz;

    MAV_Opt* mo;

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    ULONG colour[8] = {
        0x00ff0000,
        0x0000ff00,
        0x000000ff,
        0x00ffff00,
        0x00ff00ff,
        0x0000ffff,
        0x00ffaa88,
        0x00ffffff
    };

    SATURATE(sx1, 0, MAP_WIDTH - 1);
    SATURATE(sz1, 0, MAP_HEIGHT - 1);

    for (x = sx1; x < sx2; x++)
    for (z = sz1; z < sz2; z++) {
        // Draw a blue cross at the height we think the square is at.
        x1 = x + 0 << 8;
        z1 = z + 0 << 8;
        x2 = x + 1 << 8;
        z2 = z + 1 << 8;

        y1 = MAVHEIGHT(x, z) << 6;
        y2 = MAVHEIGHT(x, z) << 6;

        AENG_world_line(
            x1, y1, z1, 4, 0x00000077,
            x2, y2, z2, 4, 0x00000077,
            UC_TRUE);

        AENG_world_line(
            x2, y1, z1, 4, 0x00000077,
            x1, y2, z2, 4, 0x00000077,
            UC_TRUE);

        // Draw options for leaving this square.
        if (!ControlFlag) {
            ASSERT(WITHIN(MAV_NAV(x, z), 0, MAV_opt_upto - 1));

            mo = &MAV_opt[MAV_NAV(x, z)];

            mx = x1 + x2 >> 1;
            mz = z1 + z2 >> 1;

            for (i = 0; i < 4; i++) {
                dx = order[i].dx;
                dz = order[i].dz;

                lx = mx + dx * 96;
                lz = mz + dz * 96;

                lx += dz * (16 * 3);
                lz += -dx * (16 * 3);

                for (j = 0; j < 8; j++) {
                    if (mo->opt[i] & (1 << j)) {
                        AENG_world_line(
                            mx, y1, mz, 0, 0,
                            lx, y2, lz, 9, colour[j],
                            UC_TRUE);
                    }

                    lx += -dz * 16;
                    lz += +dx * 16;
                }
            }
        } else {
            mx = x1 + x2 >> 1;
            mz = z1 + z2 >> 1;

            for (i = 0; i < 4; i++) {
                dx = order[i].dx;
                dz = order[i].dz;

                lx = mx + dx * 96;
                lz = mz + dz * 96;

                if (MAV_CAR_GOTO(x, z, i)) {
                    AENG_world_line(
                        mx, y1, mz, 0, 0,
                        lx, y2, lz, 9, colour[0],
                        UC_TRUE);
                }
            }
        }
    }

    TRACE("MAV_opts_upto = %d\n", MAV_opt_upto);
}

// Returns UC_TRUE if you can walk in a straight line (plain-walk only) from (ax,az) to (bx,bz).
// Steps along the normalised direction vector, checking MAV_CAPS_GOTO on each cell boundary.
// On UC_FALSE: MAV_last_mx/mz = last reachable cell; MAV_dmx/dmz = blocked direction delta.
// uc_orig: MAV_can_i_walk (fallen/Source/mav.cpp)
SLONG MAV_can_i_walk(
    UBYTE ax, UBYTE az,
    UBYTE bx, UBYTE bz)
{
    SLONG x;
    SLONG z;

    SLONG dx;
    SLONG dz;

    SLONG dist;
    SLONG overdist;

    SLONG mx;
    SLONG mz;

    MAV_Opt* mo;

    ASSERT(WITHIN(ax, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(az, 0, MAP_HEIGHT - 1));

    ASSERT(WITHIN(bx, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(bz, 0, MAP_HEIGHT - 1));

    dx = bx - ax << 4;
    dz = bz - az << 4;

    dist = QDIST2(abs(dx), abs(dz));

    if (dist == 0) {
        return UC_TRUE;
    } else {
        // Normalise (dx,dz) to length 0x40 ish
        overdist = 0x4000 / dist;

        dx = dx * overdist >> 8;
        dz = dz * overdist >> 8;
    }

    MAV_last_mx = ax;
    MAV_last_mz = az;

    x = (ax << 8) + 0x80;
    z = (az << 8) + 0x80;

    while (1) {
        x += dx;
        z += dz;

        mx = x >> 8;
        mz = z >> 8;

        MAV_dmx = mx - MAV_last_mx;
        MAV_dmz = mz - MAV_last_mz;

        if (MAV_dmx | MAV_dmz) {
            ASSERT(WITHIN(MAV_last_mx, 0, MAP_WIDTH - 1));
            ASSERT(WITHIN(MAV_last_mz, 0, MAP_HEIGHT - 1));

            ASSERT(WITHIN(MAV_NAV(MAV_last_mx, MAV_last_mz), 0, MAV_opt_upto - 1));

            mo = &MAV_opt[MAV_NAV(MAV_last_mx, MAV_last_mz)];

            if (MAV_dmx == -1 && !(mo->opt[MAV_DIR_XS] & MAV_CAPS_GOTO)) {
                return UC_FALSE;
            }
            if (MAV_dmx == +1 && !(mo->opt[MAV_DIR_XL] & MAV_CAPS_GOTO)) {
                return UC_FALSE;
            }

            if (MAV_dmz == -1 && !(mo->opt[MAV_DIR_ZS] & MAV_CAPS_GOTO)) {
                return UC_FALSE;
            }
            if (MAV_dmz == +1 && !(mo->opt[MAV_DIR_ZL] & MAV_CAPS_GOTO)) {
                return UC_FALSE;
            }

            if (MAV_dmx && MAV_dmz) {
                // Diagonal step — also check corner cells.
                mo = &MAV_opt[MAV_NAV(mx, MAV_last_mz)];

                if (MAV_dmz == -1 && !(mo->opt[MAV_DIR_ZS] & MAV_CAPS_GOTO)) {
                    return UC_FALSE;
                }
                if (MAV_dmz == +1 && !(mo->opt[MAV_DIR_ZL] & MAV_CAPS_GOTO)) {
                    return UC_FALSE;
                }

                mo = &MAV_opt[MAV_NAV(MAV_last_mx, mz)];

                if (MAV_dmx == -1 && !(mo->opt[MAV_DIR_XS] & MAV_CAPS_GOTO)) {
                    return UC_FALSE;
                }
                if (MAV_dmx == +1 && !(mo->opt[MAV_DIR_XL] & MAV_CAPS_GOTO)) {
                    return UC_FALSE;
                }
            }

            if (mx == bx && mz == bz) {
                return UC_TRUE;
            }

            MAV_last_mx = mx;
            MAV_last_mz = mz;
        }
    }
}

// Clears the visited flags in the given bounding box of MAV_flag[].
// Rounds x1 down and x2 up to 4-byte (8-cell) boundaries for efficient SLONG clearing.
// uc_orig: MAV_clear_bbox (fallen/Source/mav.cpp)
void MAV_clear_bbox(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2)
{
    SLONG z;
    SLONG len;
    SLONG count;
    SLONG* zero;

    // Round x1 down and x2 up to the nearest 4 byte boundary.
    x2 += 0x7;
    x1 &= ~0x7;
    x2 &= ~0x7;

    SATURATE(x1, 0, MAP_WIDTH);
    SATURATE(x2, 0, MAP_WIDTH);
    SATURATE(z1, 0, MAP_HEIGHT);
    SATURATE(z2, 0, MAP_HEIGHT);

    len = x2 - x1 >> 3;

    for (z = z1; z < z2; z++) {
        zero = (SLONG*)(&MAV_flag[z][x1 >> 1]);
        count = len;

        while (count--) {
            *zero++ = 0;
        }
    }
}

// Path reconstruction: traces from (end_x,end_z) back to MAV_start using stored
// MAV_dir and MAV_action values. Fills MAV_node[] in reverse order (index 0 = far end).
// Jump/JumpPull actions move 2 cells; JumpPull2 moves 3 cells — stepped back accordingly.
// uc_orig: MAV_create_nodelist_from_pos (fallen/Source/mav.cpp)
void MAV_create_nodelist_from_pos(UBYTE end_x, UBYTE end_z)
{
    UBYTE x;
    UBYTE z;

    UBYTE action;
    UBYTE dir;

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    x = end_x;
    z = end_z;

    MAV_node_upto = 0;

    while (
        x != MAV_start_x || z != MAV_start_z) {
        ASSERT(WITHIN(MAV_node_upto, 0, MAV_LOOKAHEAD - 1));

        dir = MAV_dir_get(x, z);
        action = MAV_action_get(x, z);

        MAV_node[MAV_node_upto].dest_x = x;
        MAV_node[MAV_node_upto].dest_z = z;
        MAV_node[MAV_node_upto].dir = dir;
        MAV_node[MAV_node_upto].action = action;

        MAV_node_upto += 1;

        ASSERT(WITHIN(dir, 0, 3));

        x -= order[dir].dx;
        z -= order[dir].dz;

        if (action == MAV_ACTION_JUMP || action == MAV_ACTION_JUMPPULL) {
            // Moves you two squares.
            x -= order[dir].dx;
            z -= order[dir].dz;
        }

        if (action == MAV_ACTION_JUMPPULL2) {
            // Moves you three squares.
            x -= order[dir].dx;
            z -= order[dir].dz;

            x -= order[dir].dx;
            z -= order[dir].dz;
        }
    }
}

// Path smoothing: walks MAV_node[] from start backwards, trying MAV_can_i_walk for each
// GOTO waypoint. The furthest directly reachable cell becomes the immediate target.
// Non-GOTO actions (jump, climb, etc.) are returned immediately.
// uc_orig: MAV_get_first_action_from_nodelist (fallen/Source/mav.cpp)
MAV_Action MAV_get_first_action_from_nodelist()
{
    SLONG i;

    UBYTE ax;
    UBYTE az;

    UBYTE bx;
    UBYTE bz;

    MAV_Action ans;

    ax = MAV_start_x;
    az = MAV_start_z;

    ans.action = MAV_ACTION_GOTO;
    ans.dest_x = ax;
    ans.dest_z = az;

    for (i = MAV_node_upto - 1; i >= 0; i--) {
        if (MAV_node[i].action != MAV_ACTION_GOTO) {
            ans.action = MAV_node[i].action;
            ans.dir = MAV_node[i].dir;

            return ans;
        }

        bx = MAV_node[i].dest_x;
        bz = MAV_node[i].dest_z;

        if (MAV_can_i_walk(
                ax, az,
                bx, bz)) {
            ans.dest_x = bx;
            ans.dest_z = bz;
        } else {
            return ans;
        }
    }

    return ans;
}

// Priority queue node for MAV_do's A* search.
// uc_orig: PQ_Type (fallen/Source/mav.cpp)
typedef struct
{
    UBYTE x;
    UBYTE z;
    UBYTE score;  // Lower is better (heuristic distance to goal).
    UBYTE length; // Depth from start (capped at MAV_LOOKAHEAD).

} PQ_Type;

// Maximum open-set size for the A* heap.
// uc_orig: PQ_HEAP_MAX_SIZE (fallen/Source/mav.cpp)
#define PQ_HEAP_MAX_SIZE 256

// Comparison: returns UC_TRUE if node a is better (lower score) than b.
// uc_orig: PQ_better (fallen/Source/mav.cpp)
SLONG PQ_better(PQ_Type* a, PQ_Type* b)
{
    return a->score < b->score;
}

// Include the generic heap template. Requires PQ_Type, PQ_HEAP_MAX_SIZE, PQ_better above.
#include "core/pq.h"

// Heuristic: straight-line QDIST2 distance from (x,z) to MAV_dest, scaled x2, capped at 255.
// This is the sole cost — no accumulated path cost — making this greedy best-first search.
// uc_orig: MAV_score_pos (fallen/Source/mav.cpp)
UBYTE MAV_score_pos(UBYTE x, UBYTE z)
{
    SLONG dx;
    SLONG dz;

    SLONG dist;

    dx = abs((MAV_dest_x - x) << 1);
    dz = abs((MAV_dest_z - z) << 1);

    dist = QDIST2(dx, dz);

    if (dist > 255) {
        dist = 255;
    }

    return dist;
}

// Main navigation function. Runs greedy best-first search from (me_x,me_z) toward
// (dest_x,dest_z) with the given capability mask. Returns the first action to take.
// MAV_MAX_OVERFLOWS=8 bounds total work; partial-path is returned if dest unreachable.
// uc_orig: MAV_do (fallen/Source/mav.cpp)
MAV_Action MAV_do(
    SLONG me_x,
    SLONG me_z,
    SLONG dest_x,
    SLONG dest_z,
    UBYTE caps)
{
    SLONG i;
    SLONG j;

    UBYTE opt;
    UBYTE move_one;
    UBYTE move_two;
    UBYTE move_three;
    UBYTE action;

    SLONG overflows;
    SLONG best_score;
    MAV_Action ans;

    MAV_Opt* mo;

    PQ_Type start;
    PQ_Type best;
    PQ_Type next;

    SLONG mx;
    SLONG mz;

    SLONG dx;
    SLONG dz;

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    MAV_start_x = me_x;
    MAV_start_z = me_z;

    MAV_dest_x = dest_x;
    MAV_dest_z = dest_z;

    MAV_do_found_dest = UC_FALSE;

    // Clear only the ±MAV_LOOKAHEAD box around the agent (not the full 128x128 map).
    MAV_clear_bbox(
        me_x - MAV_LOOKAHEAD,
        me_z - MAV_LOOKAHEAD,
        me_x + MAV_LOOKAHEAD,
        me_z + MAV_LOOKAHEAD);

    PQ_init();

    start.x = me_x;
    start.z = me_z;
    start.score = MAV_score_pos(me_x, me_z);
    start.length = 0;

    PQ_add(start);

    MAV_visited_set(me_x, me_z);

// MAV_MAX_OVERFLOWS: when frontier depth hits MAV_LOOKAHEAD, count as overflow.
// After 8 overflows, stop and return the best partial answer found so far.
#define MAV_MAX_OVERFLOWS 8

    overflows = 0;
    best_score = UC_INFINITY;
    ans.action = MAV_ACTION_GOTO;
    ans.dir = 0;
    ans.dest_x = me_x;
    ans.dest_z = me_z;

    while (1) {
        if (PQ_empty()) {
            break;
        }

        best = PQ_best();
        PQ_remove();

        if (best.length >= MAV_LOOKAHEAD) {
            if (best.score < best_score) {
                best_score = best.score;

                MAV_create_nodelist_from_pos(best.x, best.z);
                ans = MAV_get_first_action_from_nodelist();
            }

            overflows += 1;

            if (overflows >= MAV_MAX_OVERFLOWS) {
                return ans;
            }

            continue;
        }

        if (best.x == MAV_dest_x && best.z == MAV_dest_z) {
            MAV_do_found_dest = UC_TRUE;

            MAV_create_nodelist_from_pos(best.x, best.z);
            ans = MAV_get_first_action_from_nodelist();

            return ans;
        }

        ASSERT(WITHIN(best.x, 0, MAP_WIDTH - 1));
        ASSERT(WITHIN(best.z, 0, MAP_HEIGHT - 1));

        ASSERT(WITHIN(MAV_NAV(best.x, best.z), 0, MAV_opt_upto - 1));

        mo = &MAV_opt[MAV_NAV(best.x, best.z)];

        // Expand neighbours: filter by caps bitmask, then add unvisited cells to the heap.
        for (i = 0; i < 4; i++) {
            opt = mo->opt[i] & caps;

            move_one = opt & (MAV_CAPS_GOTO | MAV_CAPS_PULLUP | MAV_CAPS_CLIMB_OVER | MAV_CAPS_FALL_OFF | MAV_CAPS_LADDER_UP);
            move_two = opt & (MAV_CAPS_JUMP | MAV_CAPS_JUMPPULL);
            move_three = opt & (MAV_CAPS_JUMPPULL2);

            dx = order[i].dx;
            dz = order[i].dz;

            if (move_one) {
                mx = best.x + dx;
                mz = best.z + dz;

                if (!MAV_visited_get(mx, mz)) {
                    next.x = mx;
                    next.z = mz;
                    next.length = best.length + 1;
                    next.score = MAV_score_pos(mx, mz);

                    if (opt & MAV_CAPS_GOTO) {
                        action = MAV_ACTION_GOTO;
                    } else if (opt & MAV_CAPS_PULLUP) {
                        action = MAV_ACTION_PULLUP;
                    } else if (opt & MAV_CAPS_CLIMB_OVER) {
                        action = MAV_ACTION_CLIMB_OVER;
                    } else if (opt & MAV_CAPS_FALL_OFF) {
                        action = MAV_ACTION_FALL_OFF;
                    } else if (opt & MAV_CAPS_LADDER_UP) {
                        action = MAV_ACTION_LADDER_UP;
                    } else
                        ASSERT(0);

                    MAV_action_set( // Sets the visited flag too.
                        mx,
                        mz,
                        action);

                    MAV_dir_set(
                        mx,
                        mz,
                        i);

                    if (action != MAV_ACTION_GOTO) {
                        // Bias against using non-standard movement types.
                        next.score += Random() & 0x3;

                        if (action == MAV_ACTION_FALL_OFF) {
                            // Falling off isn't so bad — no extra penalty.
                        } else {
                            next.score += 3;
                        }
                    }

                    PQ_add(next);
                }
            }

            if (move_two) {
                mx = best.x + dx + dx;
                mz = best.z + dz + dz;

                if (!MAV_visited_get(mx, mz)) {
                    next.x = mx;
                    next.z = mz;
                    next.length = best.length + 1;
                    next.score = MAV_score_pos(mx, mz);

                    if (opt & MAV_CAPS_JUMP) {
                        action = MAV_ACTION_JUMP;
                    } else if (opt & MAV_CAPS_JUMPPULL) {
                        action = MAV_ACTION_JUMPPULL;
                    } else
                        ASSERT(0);

                    MAV_action_set( // Sets the visited flag too.
                        mx,
                        mz,
                        action);

                    MAV_dir_set(
                        mx,
                        mz,
                        i);

                    PQ_add(next);
                }
            }

            if (move_three) {
                mx = best.x + dx + dx + dx;
                mz = best.z + dz + dz + dz;

                if (!MAV_visited_get(mx, mz)) {
                    next.x = mx;
                    next.z = mz;
                    next.length = best.length + 1;
                    next.score = MAV_score_pos(mx, mz);

                    action = MAV_ACTION_JUMPPULL2;

                    MAV_action_set( // Sets the visited flag too.
                        mx,
                        mz,
                        action);

                    MAV_dir_set(
                        mx,
                        mz,
                        i);

                    PQ_add(next);
                }
            }
        }
    }

    return ans;
}

// Returns UC_TRUE if world-space point (x,y,z) is below the MAVHEIGHT terrain surface.
// x/z in game units (>>8 → grid cell), y in game units (>>6 → MAVHEIGHT units).
// uc_orig: MAV_inside (fallen/Source/mav.cpp)
SLONG MAV_inside(
    SLONG x,
    SLONG y,
    SLONG z)
{
    x >>= 8;
    y >>= 6;
    z >>= 8;

    if (WITHIN(x, 0, MAP_WIDTH - 1) && WITHIN(z, 0, MAP_HEIGHT - 1)) {
        if (y < -127) {
            return UC_TRUE;
        }
        if (y > +127) {
            return UC_FALSE;
        }

        if (y < MAVHEIGHT(x, z)) {
            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// Fast terrain LOS using the MAVHEIGHT grid. Steps ~1 cell at a time.
// Returns UC_FALSE if any step is underground; failure point saved in MAV_height_los_fail_*.
// uc_orig: MAV_height_los_fast (fallen/Source/mav.cpp)
SLONG MAV_height_los_fast(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    SLONG dx = x2 - x1;
    SLONG dy = y2 - y1;
    SLONG dz = z2 - z1;

    SLONG dist = QDIST2(abs(dx), abs(dz));
    SLONG steps = (dist >> 8) + 1;

    SLONG x = x1;
    SLONG y = y1;
    SLONG z = z1;

    dx /= steps;
    dy /= steps;
    dz /= steps;

    while (steps-- >= 0) {
        if (MAV_inside(x, y, z)) {
            MAV_height_los_fail_x = x - (dx >> 0);
            MAV_height_los_fail_y = y - (dy >> 0);
            MAV_height_los_fail_z = z - (dz >> 0);

            return UC_FALSE;
        }

        x += dx;
        y += dy;
        z += dz;
    }

    return UC_TRUE;
}

// Slow terrain LOS — like MAV_height_los_fast but uses WARE_inside() when ware != 0.
// Starts from (x1+dx, y1+dy, z1+dz) to skip the source point and avoid self-occlusion.
// uc_orig: MAV_height_los_slow (fallen/Source/mav.cpp)
SLONG MAV_height_los_slow(
    SLONG ware,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    SLONG dx = x2 - x1;
    SLONG dy = y2 - y1;
    SLONG dz = z2 - z1;

    SLONG dist = QDIST2(abs(dx), abs(dz));
    SLONG steps = (dist >> 8) + 1;

    dx /= steps;
    dy /= steps;
    dz /= steps;

    SLONG x = x1 + dx;
    SLONG y = y1 + dy;
    SLONG z = z1 + dz;

    while (steps-- > 0) {
        SLONG inside;

        if (ware) {
            inside = WARE_inside(ware, x, y, z);
        } else {
            inside = MAV_inside(x, y, z);
        }

        if (inside) {
            MAV_height_los_fail_x = x;
            MAV_height_los_fail_y = y;
            MAV_height_los_fail_z = z;

            return UC_FALSE;
        }

        x += dx;
        y += dy;
        z += dz;
    }

    return UC_TRUE;
}

// Builds the navigation grid for a single warehouse interior.
// Temporarily redirects MAV_nav to WARE_nav[ww->nav] for the build pass,
// then restores the city grid pointer on exit.
// Uses LOS checks and ladder detection — more detailed than the outer city grid.
// uc_orig: MAV_precalculate_warehouse_nav (fallen/Source/mav.cpp)
void MAV_precalculate_warehouse_nav(UBYTE ware)
{
    SLONG i;

    SLONG x;
    SLONG y;
    SLONG z;

    SLONG x1;
    SLONG y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG dx;
    SLONG dz;

    SLONG mx;
    SLONG mz;

    SLONG tx;
    SLONG tz;

    SLONG rx;
    SLONG rz;

    SLONG dh;

    SLONG useangle;
    SLONG matrix[4];
    SLONG ladder;

    SLONG sin_yaw;
    SLONG cos_yaw;

    SLONG both_ground;

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    UBYTE opt[4];

    WARE_Ware* ww = &WARE_ware[ware];

    // Save city grid pointer.
    UWORD* old_mav_nav = MAV_nav;
    SLONG old_mav_nav_pitch = MAV_nav_pitch;

    // Redirect MAV_nav to the warehouse's private nav array.
    MAV_nav = &WARE_nav[ww->nav];
    MAV_nav_pitch = ww->nav_pitch;

    // First pass: update MAVHEIGHT for staircase prims (prim 41) inside this warehouse.
    OB_Info* oi;

    for (x = 0; x < PAP_SIZE_LO; x++)
    for (z = 0; z < PAP_SIZE_LO; z++) {
        for (oi = OB_find(x, z); oi->prim; oi++) {
            if (oi->prim == 41) {
                // The step prim — find which mapsquare its centre is over.
                PrimInfo* pi = get_prim_info(oi->prim);

                mx = pi->minx + pi->maxx >> 1;
                mz = pi->minz + pi->maxz >> 1;

                useangle = -oi->yaw;
                useangle &= 2047;

                sin_yaw = SIN(useangle);
                cos_yaw = COS(useangle);

                matrix[0] = cos_yaw;
                matrix[1] = -sin_yaw;
                matrix[2] = sin_yaw;
                matrix[3] = cos_yaw;

                rx = MUL64(mx, matrix[0]) + MUL64(mz, matrix[1]);
                rz = MUL64(mx, matrix[2]) + MUL64(mz, matrix[3]);

                rx += oi->x;
                rz += oi->z;

                rx >>= 8;
                rz >>= 8;

                if (WITHIN(rx, ww->minx, ww->maxx) &&
                    WITHIN(rz, ww->minz, ww->maxz)) {
                    MAVHEIGHT(rx, rz) = oi->y + 0x40 >> 6;
                }
            }
        }
    }

    // Main pass: compute navigation options for each cell in the warehouse bounding box.
    for (x = ww->minx; x <= ww->maxx; x++)
    for (z = ww->minz; z <= ww->maxz; z++) {
        mx = x - ww->minx;
        mz = z - ww->minz;

        // Look for a nearby ladder.
        ladder = find_nearby_ladder_colvect_radius(
                    (x << 8) + 0x80,
                    (z << 8) + 0x80,
                    0x100);

        for (i = 0; i < 4; i++) {
            opt[i] = 0;

            dx = order[i].dx;
            dz = order[i].dz;

            tx = x + dx;
            tz = z + dz;

            if (!(PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN)) {
                // This square is outside the warehouse.
                continue;
            }

            if (!WITHIN(tx, ww->minx, ww->maxx) ||
                !WITHIN(tz, ww->minz, ww->maxz)) {
                // Cannot navigate in this direction — outside the warehouse.
                continue;
            }

            if (!(PAP_2HI(tx, tz).Flags & PAP_FLAG_HIDDEN)) {
                // Target square is outside the warehouse.
                continue;
            }

            // Can we walk from (x,z) to (tx,tz)?
            dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

            if (abs(dh) > 1) {
                // Different heights — can't just walk.
                if (dh < 0) {
                    // Lower target — check for wall/fence in the way.
                    x1 = (x << 8) + 0x80;
                    z1 = (z << 8) + 0x80;
                    x2 = (tx << 8) + 0x80;
                    z2 = (tz << 8) + 0x80;

                    y1 = (MAVHEIGHT(x, z) << 6) + 0x50;
                    y2 = (MAVHEIGHT(tx, tz) << 6) + 0x50;

                    y = MAX(y1, y2);

                    if (there_is_a_los(
                            x1, y, z1,
                            x2, y, z2,
                            LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                        // Can always fall down — nothing in the way.
                        opt[i] |= MAV_CAPS_FALL_OFF;
                    } else {
                        // If there is a fence in the way, we might scale it.
                        DFacet* df = &dfacets[los_failure_dfacet];

                        if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                            if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                                // Unclimbable fence.
                            } else {
                                opt[i] |= MAV_CAPS_CLIMB_OVER;
                            }
                        }
                    }
                } else {
                    if (WITHIN(dh, 3, 5)) {
                        // Can pull up.
                        opt[i] |= MAV_CAPS_PULLUP;
                    }

                    if (ladder) {
                        DFacet* df_ladder;

                        ASSERT(WITHIN(ladder, 1, next_dfacet - 1));

                        df_ladder = &dfacets[ladder];

                        ASSERT(df_ladder->FacetType == STOREY_TYPE_LADDER);

                        // There is a ladder — can we climb up it in this direction?
                        x1 = (x << 8) + 0x80;
                        z1 = (z << 8) + 0x80;
                        x2 = (tx << 8) + 0x80;
                        z2 = (tz << 8) + 0x80;

                        if (two4_line_intersection(
                                x1, z1,
                                x2, z2,
                                df_ladder->x[0] << 8, df_ladder->z[0] << 8,
                                df_ladder->x[1] << 8, df_ladder->z[1] << 8)) {
                            opt[i] |= MAV_CAPS_LADDER_UP;
                        }
                    }
                }
            } else {
                // Same or close height — check for wall/fence in the way.
                x1 = (x << 8) + 0x80;
                z1 = (z << 8) + 0x80;
                x2 = (tx << 8) + 0x80;
                z2 = (tz << 8) + 0x80;

                y1 = (MAVHEIGHT(x, z) << 6) + 0x50;
                y2 = (MAVHEIGHT(tx, tz) << 6) + 0x50;

                y = MAX(y1, y2);

                if (there_is_a_los(
                        x1, y, z1,
                        x2, y, z2,
                        LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                    // Nothing in the way.
                    opt[i] |= MAV_CAPS_GOTO;
                } else {
                    // If there is a fence in the way, we might scale it.
                    DFacet* df = &dfacets[los_failure_dfacet];

                    if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                        if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                            // Unclimbable fence.
                        } else {
                            opt[i] |= MAV_CAPS_CLIMB_OVER;
                        }
                    }
                }
            }

            // If no direct walk or fence-climb, try jump moves.
            if (!(opt[i] & MAV_CAPS_GOTO) &&
                !(opt[i] & MAV_CAPS_CLIMB_OVER)) {
                // Try jumping one block.
                tx += dx;
                tz += dz;

                if (WITHIN(tx, ww->minx, ww->maxx) &&
                    WITHIN(tz, ww->minz, ww->maxz) &&
                    (PAP_2HI(tx, tz).Flags & PAP_FLAG_HIDDEN)) {
                    dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                    x1 = (x << 8) + 0x80;
                    z1 = (z << 8) + 0x80;
                    x2 = (tx << 8) + 0x80;
                    z2 = (tz << 8) + 0x80;

                    y1 = (MAVHEIGHT(x, z) << 6) + 0xa0;
                    y2 = (MAVHEIGHT(tx, tz) << 6) + 0xa0;

                    if (there_is_a_los(
                            x1, y1, z1,
                            x2, y2, z2,
                            LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                        if (dh < 2) {
                            opt[i] |= MAV_CAPS_JUMP;
                        } else {
                            if (WITHIN(dh, 2, 5)) {
                                opt[i] |= MAV_CAPS_JUMPPULL;
                            }
                        }
                    }

                    // Try jumping two blocks.
                    tx += dx;
                    tz += dz;

                    if (WITHIN(tx, 0, MAP_WIDTH - 1) &&
                        WITHIN(tz, 0, MAP_HEIGHT - 1)) {
                        dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                        if (dh > 4) {
                            // Can't make this jump.
                        } else {
                            if (dh > -6) {
                                x1 = (x << 8) + 0x80;
                                z1 = (z << 8) + 0x80;
                                x2 = (tx << 8) + 0x80;
                                z2 = (tz << 8) + 0x80;

                                y1 = (MAVHEIGHT(x, z) << 6) + 0xa0;
                                y2 = (MAVHEIGHT(tx, tz) << 6) + 0xa0;

                                if (there_is_a_los(
                                        x1, y1, z1,
                                        x2, y2, z2,
                                        LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                                    opt[i] |= MAV_CAPS_JUMPPULL2;
                                }
                            }
                        }
                    }
                }
            }
        }

        StoreMavOpts(mx, mz, opt);
    }

    // Second pass: staircase prim hack — fix up movement directions around step prims.
    for (x = 0; x < PAP_SIZE_LO; x++)
    for (z = 0; z < PAP_SIZE_LO; z++) {
        for (oi = OB_find(x, z); oi->prim; oi++) {
            if (oi->prim == 41) {
                // The step prim!
                {
                    // Find which mapsquare the middle of this prim is over.
                    PrimInfo* pi = get_prim_info(oi->prim);

                    mx = pi->minx + pi->maxx >> 1;
                    mz = pi->minz + pi->maxz >> 1;

                    useangle = -oi->yaw;
                    useangle &= 2047;

                    sin_yaw = SIN(useangle);
                    cos_yaw = COS(useangle);

                    matrix[0] = cos_yaw;
                    matrix[1] = -sin_yaw;
                    matrix[2] = sin_yaw;
                    matrix[3] = cos_yaw;

                    rx = MUL64(mx, matrix[0]) + MUL64(mz, matrix[1]);
                    rz = MUL64(mx, matrix[2]) + MUL64(mz, matrix[3]);

                    rx += oi->x;
                    rz += oi->z;

                    rx >>= 8;
                    rz >>= 8;

                    if (WITHIN(rx, ww->minx, ww->maxx) &&
                        WITHIN(rz, ww->minz, ww->maxz)) {
                        mx = rx - ww->minx;
                        mz = rz - ww->minz;

                        if (oi->yaw < 256 || oi->yaw > 1792 || WITHIN(oi->yaw, 768, 1280)) {
                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx - 1, rz)) {
                                // Walking left-right on a wide staircase.
                            } else {
                                MAV_turn_movement_off(mx, mz, MAV_DIR_XS);
                                MAV_turn_movement_off(mx - 1, mz, MAV_DIR_XL);
                            }

                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx + 1, rz)) {
                                // Walking left-right on a wide staircase.
                            } else {
                                MAV_turn_movement_off(mx, mz, MAV_DIR_XL);
                                MAV_turn_movement_off(mx + 1, mz, MAV_DIR_XS);
                            }

                            if (!WITHIN(oi->yaw, 768, 1280)) {
                                if (MAVHEIGHT(rx, rz + 1) <= MAVHEIGHT(rx, rz) + 3) {
                                    MAV_turn_movement_on(mx, mz, MAV_DIR_ZL);
                                    MAV_turn_movement_on(mx, mz + 1, MAV_DIR_ZS);
                                }
                            } else {
                                if (MAVHEIGHT(rx, rz - 1) <= MAVHEIGHT(rx, rz) + 3) {
                                    MAV_turn_movement_on(mx, mz, MAV_DIR_ZS);
                                    MAV_turn_movement_on(mx, mz - 1, MAV_DIR_ZL);
                                }
                            }
                        } else {
                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx, rz - 1)) {
                                // Walking across a wide staircase.
                            } else {
                                MAV_turn_movement_off(mx, mz, MAV_DIR_ZS);
                                MAV_turn_movement_off(mx, mz - 1, MAV_DIR_ZL);
                            }

                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx, rz + 1)) {
                                // Walking across a wide staircase.
                            } else {
                                MAV_turn_movement_off(mx, mz, MAV_DIR_ZL);
                                MAV_turn_movement_off(mx, mz + 1, MAV_DIR_ZS);
                            }

                            if (!WITHIN(oi->yaw, 256, 768)) {
                                if (MAVHEIGHT(rx - 1, rz) <= MAVHEIGHT(rx, rz) + 3) {
                                    MAV_turn_movement_on(mx, mz, MAV_DIR_XS);
                                    MAV_turn_movement_on(mx - 1, mz, MAV_DIR_XL);
                                }
                            } else {
                                if (MAVHEIGHT(rx + 1, rz) <= MAVHEIGHT(rx, rz) + 3) {
                                    MAV_turn_movement_on(mx, mz, MAV_DIR_XL);
                                    MAV_turn_movement_on(mx + 1, mz, MAV_DIR_XS);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Restore city grid pointer.
    MAV_nav = old_mav_nav;
    MAV_nav_pitch = old_mav_nav_pitch;
}

// Returns the raw capability bitmask for one direction from a given cell.
// Returns 0 if out of bounds. Works on the currently active MAV_nav (city or warehouse).
// uc_orig: MAV_get_caps (fallen/Source/mav.cpp)
UBYTE MAV_get_caps(
    UBYTE x,
    UBYTE z,
    UBYTE dir)
{
    UBYTE ans;

    MAV_Opt* mo;

    if (WITHIN(x, 0, MAV_nav_pitch - 1) && WITHIN(z, 0, MAV_nav_pitch - 1)) {
        mo = &MAV_opt[MAV_NAV(x, z)];

        ASSERT(WITHIN(dir, 0, 3));

        ans = mo->opt[dir];

        return ans;
    }

    return 0;
}

// Enables car movement through cell (mx,mz) in one direction at runtime.
// Sets the corresponding MAV_CAR bit in the packed MAV_nav UWORD.
// uc_orig: MAV_turn_car_movement_on (fallen/Source/mav.cpp)
void MAV_turn_car_movement_on(UBYTE mx, UBYTE mz, UBYTE dir)
{
    UBYTE mav;

    ASSERT(WITHIN(mx, 0, PAP_SIZE_HI - 1));
    ASSERT(WITHIN(mz, 0, PAP_SIZE_HI - 1));

    mav = MAV_CAR(mx, mz);
    mav |= 1 << dir;

    SET_MAV_CAR(mx, mz, mav);
}

// Disables car movement through cell (mx,mz) in one direction at runtime.
// Clears the corresponding MAV_CAR bit in the packed MAV_nav UWORD.
// uc_orig: MAV_turn_car_movement_off (fallen/Source/mav.cpp)
void MAV_turn_car_movement_off(UBYTE mx, UBYTE mz, UBYTE dir)
{
    UBYTE mav;

    ASSERT(WITHIN(mx, 0, PAP_SIZE_HI - 1));
    ASSERT(WITHIN(mz, 0, PAP_SIZE_HI - 1));

    mav = MAV_CAR(mx, mz);
    mav &= ~(1 << dir);

    SET_MAV_CAR(mx, mz, mav);
}
