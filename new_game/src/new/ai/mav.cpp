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

#include "fallen/Headers/collide.h"       // Temporary: there_is_a_los, los_failure_dfacet, two4_line_intersection
#include "fallen/Headers/prim.h"          // Temporary: PrimInfo, get_prim_info, does_fence_lie_along_line, RFACE_FLAG_NODRAW, RoofFace4
#include "fallen/Headers/building.h"      // Temporary: BUILDING_TYPE_WAREHOUSE, DBuilding
#include "fallen/Headers/memory.h"        // Temporary: roof_faces4, next_dbuilding, next_dwalkable, dwalkables, next_dfacet

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

    MAV_calc_height_array(FALSE);

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
                        both_ground = TRUE;
                    } else {
                        both_ground = FALSE;
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
