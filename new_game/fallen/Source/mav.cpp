// claude-ai: MAV = A* pathfinding/navigation system on a 128x128 city grid (same resolution as PAP_Hi).
// claude-ai: Each grid cell stores movement options (MAV_Opt) for 4 cardinal directions.
// claude-ai: Each direction encodes a bitmask of allowed movement types (MAV_CAPS_* flags).
// claude-ai: The A* search (MAV_do) respects an agent's capability mask — e.g. MAV_CAPS_DARCI=0xff allows everything.
// claude-ai: Lookahead is capped at MAV_LOOKAHEAD=32 steps to bound search cost.
// claude-ai: Two separate nav grids exist: the main city grid (MAV_nav) and per-warehouse grids (WARE_nav).
// claude-ai: A separate car-navigation bitfield (MAV_CAR) tracks vehicle-passable edges independently.
//
// Another navigation system.
//

#include "game.h"
#include "mav.h"
#include "..\ddengine\headers\aeng.h"
#include "pap.h"
#include "supermap.h"
#include "walkable.h"
#include "memory.h"
#include "ware.h"
#include "ob.h"

//
// A prototype here never hurt anyone
//

//
// The height in stories of each square.
//

// MAV_height_workaround *MAV_height;

// claude-ai: MAV_opt is a shared pool of movement-option records. MAV_nav stores per-cell indices into this pool.
// claude-ai: Sharing identical option records saves memory: many cells have the same 4-directional passability.
//
// The navigation info. Each value in the nav array is an index
// into this array.
//

MAV_Opt* MAV_opt;
SLONG MAV_opt_upto;

// claude-ai: MAV_nav is a flat 128x128 UWORD array (pitch=128). Each UWORD packs:
// claude-ai:   bits [13:0] = index into MAV_opt pool (up to MAV_MAX_OPTS=1024 unique option records)
// claude-ai:   bits [15:14] = MAV_SPARE flags (bit0=water, bit1=unused)
// claude-ai: For warehouses MAV_nav is temporarily redirected to WARE_nav with a different pitch.
//
// How you can move out of each square.
//

UWORD* MAV_nav;
SLONG MAV_nav_pitch = 128;

// claude-ai: MAV_init pre-populates the first 16 MAV_opt entries (indices 0-15) with all combinations
// claude-ai: of 4 directions being open or blocked for plain walking (MAV_CAPS_GOTO only).
// claude-ai: This is required by INSIDE2_mav_nav_calc() which addresses these entries by fixed index.
// claude-ai: After init, MAV_opt_upto=16; all subsequent unique option sets are appended dynamically.
void MAV_init()
{
    SLONG i;

    MAV_nav_pitch = 128;
    MAV_opt_upto = 0;

    //
    // Create common MAV_opts first. We need these for the inside2 nav
    // so macke sure they are all here (so we won't have to dynamically
    // allocate them).
    //

    for (i = 0; i < 16; i++) {
        //
        // If you change this bit of code, make sure you change the corresponding
        // bit of code in INSIDE2_mav_nav_calc().
        //

        MAV_opt[i].opt[0] = (i & 1) ? 0 : MAV_CAPS_GOTO;
        MAV_opt[i].opt[1] = (i & 2) ? 0 : MAV_CAPS_GOTO;
        MAV_opt[i].opt[2] = (i & 4) ? 0 : MAV_CAPS_GOTO;
        MAV_opt[i].opt[3] = (i & 8) ? 0 : MAV_CAPS_GOTO;
    }

    MAV_opt_upto = 16;
}

// claude-ai: StoreMavOpts deduplicates MAV_Opt records: it first scans existing entries for an identical
// claude-ai: 4-byte option set. If found it reuses it (saves memory). Otherwise appends a new entry.
// claude-ai: MAV_MAX_OPTS=1024 is a hard cap; hitting it is a fatal error (ASSERT).
static void StoreMavOpts(SLONG x, SLONG z, UBYTE* opt)
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

// claude-ai: MAV_calc_height_array builds a per-cell height map used by MAV_precalculate to decide
// claude-ai: whether adjacent cells are walkable, require climbing, or allow jumping.
// claude-ai: Height is stored as SBYTE in units of 0x40 game-units (1 unit = 64 Y-coords).
// claude-ai: PAP_FLAG_HIDDEN marks cells inside buildings/warehouses; their height comes from roof faces.
// claude-ai: PAP_FLAG_NOGO is set for hidden cells with no roof data (inaccessible building interiors).
// claude-ai: NOT compiled on PSX (PC/DC editor-side precalculation only; PSX loads precomputed data).
//
// Calculates the MAV_height array.
//
void MAV_calc_height_array(SLONG ignore_warehouses)
{
    SLONG i;
    SLONG j;

    SLONG x;
    SLONG z;

    SLONG mx;
    SLONG mz;

    SLONG index;
    SLONG face;
    SLONG faceheight;

    SLONG height;
    SLONG walk;

    DBuilding* db;
    PrimFace4* p_f4;
    RoofFace4* rf;
    DWalkable* dw;
    PAP_Hi* ph;

    //
    // Work out the MAV_height array.
    //

    for (x = 0; x < PAP_SIZE_HI; x++)
        for (z = 0; z < PAP_SIZE_HI; z++) {
            //
            // The middle of this mapsquare.
            //

            mx = x << 8;
            mz = z << 8;

            mx += 0x80;
            mz += 0x80;

            if ((PAP_2HI(x, z).Flags & PAP_FLAG_ROOF_EXISTS)) {
            } else if ((PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN) && !ignore_warehouses) {
                MAVHEIGHT(x, z) = -127;
            } else {
                height = PAP_calc_height_at(mx, mz);

                //
                // Convert to the SBYTE coordinate system in the MAV_height array.
                //

                height /= 0x40;

                SATURATE(height, -127, +127);

                MAVHEIGHT(x, z) = height;
            }
        }

    //
    // Use the walkable faces to set the height of the hidden squares.
    //

    for (i = 1; i < next_dbuilding; i++) {
        db = &dbuildings[i];

        if (db->Type == BUILDING_TYPE_WAREHOUSE) {
            if (ignore_warehouses) {
                //
                // Ignore these roof faces.
                //

                continue;
            }
        }

        for (walk = db->Walkable; walk; walk = dw->Next) {
            dw = &dwalkables[walk];

            for (j = dw->StartFace4; j < dw->EndFace4; j++) {
                rf = &roof_faces4[j];

                rf->DrawFlags &= ~RFACE_FLAG_NODRAW; // This flag is set later... but shouldn't be set here.

                if (db->Type == BUILDING_TYPE_WAREHOUSE) {
                    //
                    // Mark this mapsquare as being inside a warehouse (top bit!)
                    //

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

    //
    // Nowadays there are buildings without roofs.
    //

    for (x = 0; x < PAP_SIZE_HI; x++)
        for (z = 0; z < PAP_SIZE_HI; z++) {
            if (MAVHEIGHT(x, z) <= -127) {
                if (PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN) {
                    MAVHEIGHT(x, z) = 127;

                    PAP_2HI(x, z).Flags |= PAP_FLAG_NOGO;
                }
            }
        }

    /*

    for (x = 0; x < PAP_SIZE_LO; x++)
    for (z = 0; z < PAP_SIZE_LO; z++)
    {
            face = PAP_2LO(x,z).Walkable;

            while(face)
            {
                    if (face > 0)
                    {
                            p_f4 = &prim_faces4[face];

                            //
                            // ASSUME FACES DONT SPAN MORE THAN ONE MAPSQUARE!
                            //

                            mx = prim_points[p_f4->Points[0]].X + prim_points[p_f4->Points[1]].X + prim_points[p_f4->Points[2]].X + prim_points[p_f4->Points[3]].X >> 10;
                            mz = prim_points[p_f4->Points[0]].Z + prim_points[p_f4->Points[1]].Z + prim_points[p_f4->Points[2]].Z + prim_points[p_f4->Points[3]].Z >> 10;

                            if (PAP_2HI(mx,mz).Flags & PAP_FLAG_HIDDEN)
                            {
                                    if(calc_height_on_face((mx << 8) + 0x80, (mz << 8) + 0x80, face,&height))
                                    {

                                            //
                                            // Convert to the SBYTE coordinate system in the MAV_height array.
                                            //

                                            height /= 0x40;

                                            SATURATE(height, -127, +127);

                                            if (MAV_height[mx][mz] < height)
                                            {
                                                    MAV_height[mx][mz] = height;
                                            }
                                    }

                            }

                            face = p_f4->WALKABLE;
                    }
                    else
                    {
                            rf = &roof_faces4[-face];

                            //
                            // This is a cunning roof face.
                            //

                            height = rf->Y / 0x40;

                            SATURATE(height, -127, +127);

                            if (MAV_height[rf->X][rf->Z] < height)
                            {
                                    MAV_height[rf->X][rf->Z] = height;
                            }

                            face = rf->Next;
                    }
            }
    }

    */
}

// claude-ai: MAV_turn_off_square removes only the plain-walk (MAV_CAPS_GOTO) edge INTO cell (x,z)
// claude-ai: from each of its 4 neighbours. Climbing/jumping edges from neighbours are preserved.
// claude-ai: Used to block off obstacle squares (e.g. sloped terrain, UCPD rotating sign).
//
// Makes sure that nobody mavigates into the given square by walking
// into it.
//
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

// claude-ai: MAV_turn_off_whole_square removes ALL movement capabilities (all bits) into cell (x,z)
// claude-ai: from each of its 4 neighbours — no walking, climbing, or jumping allowed into this cell.
// claude-ai: Stronger than MAV_turn_off_square; used for slopes, NOGO cells, etc.
//
// Makes sure nobody can go into this square in any way.
//

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

// claude-ai: MAV_turn_off_whole_square_car is the car-nav equivalent of MAV_turn_off_whole_square.
// claude-ai: Clears the corresponding direction bit in the MAV_CAR bitfield for the 4 neighbours.
// claude-ai: Applied after main nav precalculation for cells with PAP_FLAG_NOGO.
//
// Makes sure nobody can go into this square in any way.
//

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

    UBYTE opt[4];

    SLONG i;
    SLONG j;

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

// claude-ai: MAV_remove_facet_car cuts a wall/fence facet out of the car-navigation grid.
// claude-ai: A facet is a line segment (x1,z1)-(x2,z2) — either horizontal or vertical.
// claude-ai: Cells on each side of the facet have the crossing direction bit cleared in MAV_CAR.
//
// cut off a facet from the car mav
//

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

//
// Turns off movement in the given direction from the square.
//

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

//
// Turns on movement in the given direction from the square.
//

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

// claude-ai: MAV_precalculate is the main city-grid nav build pass (PC/editor only, not PSX or DC).
// claude-ai: Steps:
// claude-ai:   1. Build MAVHEIGHT array from PAP heights + walkable roof faces.
// claude-ai:   2. Adjust MAVHEIGHT under staircase prims (prim 41) using the prim's actual Y position.
// claude-ai:   3. For each cell, check all 4 cardinal neighbours and determine reachability:
// claude-ai:      - Same/close height + clear LOS => MAV_CAPS_GOTO (plain walk)
// claude-ai:      - Climbable fence in the way      => MAV_CAPS_CLIMB_OVER
// claude-ai:      - Height drop + clear LOS          => MAV_CAPS_FALL_OFF
// claude-ai:      - Height rise 2-4 + no fence       => MAV_CAPS_PULLUP
// claude-ai:      - Ladder present in direction      => MAV_CAPS_LADDER_UP
// claude-ai:      - 2-cell jump possible             => MAV_CAPS_JUMP / MAV_CAPS_JUMPPULL
// claude-ai:      - 3-cell jump possible             => MAV_CAPS_JUMPPULL2
// claude-ai:   4. Apply staircase hacks (turn off lateral movement on stair cells, enable stair direction).
// claude-ai:   5. Remove sloped/NOGO cells from nav.
// claude-ai:   6. Mark water texture cells with MAV_SPARE_FLAG_WATER.
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

    //
    // Calculates the MAV_height array including warehouses.
    //

    MAV_calc_height_array(FALSE);

    //
    // Make the staircase prims change the MAV_height array
    //

    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            for (oi = OB_find(x, z); oi->prim; oi++) {
                if (oi->prim == 41) {
                    //
                    // The step prim!
                    //

                    if (!(oi->flags & OB_FLAG_WAREHOUSE)) {
                        //
                        // Find which mapsquare the middle of this prim is over.
                        //

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

    //
    // Work out the nav for each square.
    //

    for (x = 0; x < PAP_SIZE_HI; x++)
        for (z = 0; z < PAP_SIZE_HI; z++) {
            //
            // Look for a nearby ladder.
            //

            ladder = find_nearby_ladder_colvect_radius(
                (x << 8) + 0x80,
                (z << 8) + 0x80,
                0x100);

            UBYTE caropts = 0; // car opts

            for (i = 0; i < 4; i++) {
                opt[i] = 0;

                dx = order[i].dx;
                dz = order[i].dz;

                tx = x + dx;
                tz = z + dz;

                if (WITHIN(tx, 0, PAP_SIZE_HI - 1) && WITHIN(tz, 0, PAP_SIZE_HI - 1)) {
                    //
                    // Is one of the squares on a building?
                    //

                    if (!(PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN) && !(PAP_2HI(tx, tz).Flags & PAP_FLAG_HIDDEN)) {
                        both_ground = TRUE;
                    } else {
                        both_ground = FALSE;
                    }

                    //
                    // Can we walk from (x,z) to (tx,tz)?
                    //

                    // claude-ai: Walkability threshold: on-ground cells allow dh up to ±2 MAVHEIGHT units;
                    // claude-ai: building-top cells (PAP_FLAG_HIDDEN on either side) are stricter: max ±1 unit.
                    // claude-ai: If exceeded, the code tries MAV_CAPS_FALL_OFF / CLIMB_OVER / PULLUP / LADDER_UP / JUMP.
                    dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                    if ((!both_ground && abs(dh) > 1) || (abs(dh) > 2)) {
                        //
                        // There are at different heights, so you cant
                        // just walk between the two squares.
                        //

                        if (dh < 0) {
                            //
                            // There might be a wall or fence in the way.
                            //

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
                                //
                                // We can always fall down becuase there is nothing in the way.
                                //

                                opt[i] |= MAV_CAPS_FALL_OFF;
                            } else {
                                //
                                // If there is a fence in the way, then we can scale the fence.
                                //

                                DFacet* df = &dfacets[los_failure_dfacet];

                                if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                                    if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                                        //
                                        // Unclimbable fence.
                                        //
                                    } else {
                                        //
                                        // We can scale this fence.
                                        //

                                        opt[i] |= MAV_CAPS_CLIMB_OVER;
                                    }
                                }
                            }

                            if (there_is_a_los(x1, y, z1, x2, y, z2,
                                    LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                                caropts |= 1 << i;
                            }
                        } else {
                            // claude-ai: Pull-up: neighbour is 2-4 MAVHEIGHT units higher and no fence along the edge => MAV_CAPS_PULLUP.
                            // claude-ai: Fence check uses a perpendicular line segment at the cell boundary.
                            if (WITHIN(dh, 2, 4)) {
                                //
                                // We can pull ourselves up as long as there isn't a fence in the way.
                                //

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
                                    //
                                    // You can't pull yourself up if there is a fence in the way.
                                    //
                                } else {
                                    opt[i] |= MAV_CAPS_PULLUP;
                                }
                            }

                            if (ladder) {
                                DFacet* df_ladder;

                                ASSERT(WITHIN(ladder, 1, next_dfacet - 1));

                                df_ladder = &dfacets[ladder];

                                ASSERT(df_ladder->FacetType == STOREY_TYPE_LADDER);

                                //
                                // There is a ladder- can we climb up it in this direction?
                                //

                                x1 = (x << 8) + 0x80;
                                z1 = (z << 8) + 0x80;
                                x2 = (tx << 8) + 0x80;
                                z2 = (tz << 8) + 0x80;

                                if (two4_line_intersection(
                                        x1, z1,
                                        x2, z2,
                                        df_ladder->x[0] << 8, df_ladder->z[0] << 8,
                                        df_ladder->x[1] << 8, df_ladder->z[1] << 8)) {
                                    //
                                    // Make sure the ladder reaches to the bottom.
                                    //

                                    if (abs(df_ladder->Y[0] - PAP_calc_map_height_at(x1, z1)) <= 0x50) {
                                        opt[i] |= MAV_CAPS_LADDER_UP;
                                    }
                                }
                            }
                        }
                    } else {
                        //
                        // There might be a wall or fence in the way.
                        //

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
                            //
                            // Nothing in the way...
                            //

                            opt[i] |= MAV_CAPS_GOTO;
                        } else {
                            //
                            // If there is a fence in the way, then we can scale the fence.
                            //

                            DFacet* df = &dfacets[los_failure_dfacet];

                            if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                                if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                                    //
                                    // Unclimbable fence.
                                    //
                                } else {
                                    //
                                    // We can scale this fence.
                                    //

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
                        //
                        // Now what about jumping one block?
                        //

                        tx += dx;
                        tz += dz;

                        if (WITHIN(tx, 0, MAP_WIDTH - 1) && WITHIN(tz, 0, MAP_HEIGHT - 1)) {
                            dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                            //
                            // Can we jump there?
                            //

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

                            //
                            // What about jumping two blocks?
                            //

                            tx += dx;
                            tz += dz;

                            if (WITHIN(tx, 0, MAP_WIDTH - 1) && WITHIN(tz, 0, MAP_HEIGHT - 1)) {
                                dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                                if (dh > 4) {
                                    //
                                    // Can't make this jump
                                    //
                                } else {
                                    if (dh > -6) {
                                        //
                                        // Can we jump there?
                                        //

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

            // claude-ai: After evaluating all 4 directions, commit the computed opt[4] bitmask array and car flags.
            // claude-ai: MAV_SPARE initialized to 3 here; overwritten later by the water-texture scan.
            StoreMavOpts(x, z, opt);
            SET_MAV_CAR(x, z, caropts);
            SET_MAV_SPARE(x, z, 3);
        }

    //
    // claude-ai: STAIRCASE / SKYLIGHT post-processing pass (prim 41 = step, prim 226/227 = skylight).
    // claude-ai: Staircase (prim 41): turns OFF lateral movement on the stair cell (you can't walk sideways),
    // claude-ai:   and turns ON movement in the stair direction to/from the adjacent cell.
    // claude-ai: Stair direction is derived from oi->yaw (angle in 2048-units) — 4 cardinal orientations.
    // claude-ai: Skylight (prim 226): removes 2 roof-face walkable entries (so AI doesn't walk on skylights).
    // claude-ai: Large skylight (prim 227): removes a 2x3 grid of roof-face walkable entries.
    // claude-ai: UCPD rotating sign (prim 133): blocks the entire grid square below it.
    // A hack for the staircase prim and the skylights.
    //

    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            for (oi = OB_find(x, z); oi->prim; oi++) {
                if (oi->prim == 41) {
                    //
                    // The step prim!
                    //

                    if (!(oi->flags & OB_FLAG_WAREHOUSE)) {
                        //
                        // Find which mapsquare the middle of this prim is over.
                        //

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

                        if (oi->yaw < 256 || oi->yaw > 1792 || WITHIN(oi->yaw, 768, 1280)) {
                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx - 1, rz)) {
                                //
                                // Walking left-right on a wide staircase.
                                //
                            } else {
                                MAV_turn_movement_off(rx, rz, MAV_DIR_XS);
                                MAV_turn_movement_off(rx - 1, rz, MAV_DIR_XL);
                            }

                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx + 1, rz)) {
                                //
                                // Walking left-right on a wide staircase.
                                //
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
                                //
                                // Walking across a wide staircase.
                                //
                            } else {
                                MAV_turn_movement_off(rx, rz, MAV_DIR_ZS);
                                MAV_turn_movement_off(rx, rz - 1, MAV_DIR_ZL);
                            }

                            if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx, rz + 1)) {
                                //
                                // Walking across a wide staircase.
                                //
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
                    //
                    // This is a skylight. Remove the two roof faces that
                    // are covered by the skylight.
                    //

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
                    //
                    // This is the large skylight. Remove the roof faces that
                    // are covered by it.
                    //

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
                    //
                    // This is the rotating UCPD sign.
                    //

                    MAV_turn_off_whole_square(oi->x >> 8, oi->z >> 8);
                }
            }
        }

    {
        extern SLONG PAP_on_slope(SLONG x, SLONG z, SLONG * angle);

        //
        // claude-ai: Slope rejection pass: cells where PAP_on_slope() > 100 at two corners are too steep to walk.
        // claude-ai: Only ground cells checked (PAP_FLAG_HIDDEN skipped — building roofs aren't sloped in this sense).
        // Take all slippy squares out of the mav.
        //

        SLONG mx;
        SLONG mz;

        SLONG angle;

        for (mx = 0; mx < PAP_SIZE_HI; mx++)
            for (mz = 0; mz < PAP_SIZE_HI; mz++) {
                if (PAP_2HI(mx, mz).Flags & PAP_FLAG_HIDDEN) {
                    //
                    // These squares aren't slippy.
                    //

                    continue;
                }

                if (PAP_on_slope((mx << 8) + 0x40, (mz << 8) + 0x40, &angle) > 100 ||
                    //				PAP_on_slope((mx << 8) + 0xc0, (mz << 8) + 0x40, &angle) > 50 ||
                    //				PAP_on_slope((mx << 8) + 0x40, (mz << 8) + 0xc0, &angle) > 50 ||
                    PAP_on_slope((mx << 8) + 0xc0, (mz << 8) + 0xc0, &angle) > 100) {
                    MAV_turn_off_whole_square(mx, mz);
                }
            }
    }

    // claude-ai: Final car-nav cleanup: cells marked PAP_FLAG_NOGO are removed from MAV_CAR as well.
    // remove NOGO sqaures from the car map
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

    /*

    //
    // Look for prims that block off certain squares
    //

    THING_INDEX t_index;

    Thing *p_thing;
    t_index = PRIMARY_USED;

    while(t_index)
    {
            p_thing = TO_THING(t_index);

            if (p_thing->Class == CLASS_FURNITURE)
            {
                    if (p_thing->Draw.Mesh->ObjectId  < 5               ||
                            p_thing->Draw.Mesh->ObjectId == 55              ||
                            p_thing->Draw.Mesh->ObjectId == 40              ||
                            p_thing->Draw.Mesh->ObjectId == 41              ||
                            p_thing->Draw.Mesh->ObjectId == PRIM_OBJ_CANOPY ||
                            p_thing->Draw.Mesh->ObjectId == PRIM_OBJ_SIGN)
                    {
                            //
                            // Ignore these objects.
                            //
                    }
                    else
                    {
                            //
                            // The rotation matrix of the prim.
                            //

                            useangle  = -p_thing->Draw.Mesh->Angle;
                            useangle &=  2047;

                            sin_yaw = SIN(useangle);
                            cos_yaw = COS(useangle);

                            matrix[0] =  cos_yaw;
                            matrix[1] =  sin_yaw;
                            matrix[2] = -sin_yaw;
                            matrix[3] =  cos_yaw;

                            //
                            // Take out all squares whose centre is in the
                            // bounding box of the prim.
                            //

                            PrimInfo *pi = get_prim_info(p_thing->Draw.Mesh->ObjectId);

                            x1 = (p_thing->WorldPos.X >> 8) - pi->radius >> 8;
                            z1 = (p_thing->WorldPos.Z >> 8) - pi->radius >> 8;
                            x2 = (p_thing->WorldPos.X >> 8) + pi->radius >> 8;
                            z2 = (p_thing->WorldPos.Z >> 8) + pi->radius >> 8;

                            SATURATE(x1, 0, MAP_WIDTH  - 1);
                            SATURATE(z1, 0, MAP_HEIGHT - 1);
                            SATURATE(x2, 0, MAP_WIDTH  - 1);
                            SATURATE(z2, 0, MAP_HEIGHT - 1);

                            for (x = x1; x <= x2; x++)
                            for (z = z1; z <= z2; z++)
                            {
                                    //
                                    // Rotate the centre of the square into the space of the prim.
                                    //

                                    tx  = ((x << 8) + 0x80) - (p_thing->WorldPos.X >> 8);
                                    tz  = ((z << 8) + 0x80) - (p_thing->WorldPos.Z >> 8);

                                    rx = MUL64(tx, matrix[0]) + MUL64(tz, matrix[1]);
                                    rz = MUL64(tx, matrix[2]) + MUL64(tz, matrix[3]);

                                    #define MAV_PUSH_OUT (0x10)

                                    if (WITHIN(rx, pi->minx - MAV_PUSH_OUT, pi->maxx + MAV_PUSH_OUT) &&
                                            WITHIN(rz, pi->minz - MAV_PUSH_OUT, pi->maxz + MAV_PUSH_OUT))
                                    {
                                            //
                                            // Turn off this square.
                                            //

                                            MAV_turn_off_square(x, z);
                                    }
                            }
                    }
            }

            t_index = p_thing->LinkChild;
    }

    */

    //
    // claude-ai: Water detection: texture pages 454, 99456, 99457 identify water tiles.
    // claude-ai: Cells with water get MAV_SPARE_FLAG_WATER set in the MAV_nav SPARE bits [15:14].
    // claude-ai: Used by NPC/gameplay logic to know which ground cells are water (e.g. drowning, avoidance).
    // Set a bit where there is a water texture on the ground.
    //

    {
        SLONG mx;
        SLONG mz;

        SLONG page;

        for (mx = 0; mx < PAP_SIZE_HI; mx++)
            for (mz = 0; mz < PAP_SIZE_HI; mz++) {
                page = PAP_2HI(mx, mz).Texture & 0x3ff;

                if (page == 454 || page == 99456 || page == 99457) {
                    //
                    // This is water texture.
                    //

                    SET_MAV_SPARE(mx, mz, MAV_SPARE_FLAG_WATER);
                } else {
                    SET_MAV_SPARE(mx, mz, 0);
                }
            }
    }
}

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
            //
            // Draw a blue cross at the height we think the square is at.
            //

            x1 = x + 0 << 8;
            z1 = z + 0 << 8;
            x2 = x + 1 << 8;
            z2 = z + 1 << 8;

            y1 = MAVHEIGHT(x, z) << 6;
            y2 = MAVHEIGHT(x, z) << 6;

            AENG_world_line(
                x1, y1, z1, 4, 0x00000077,
                x2, y2, z2, 4, 0x00000077,
                TRUE);

            AENG_world_line(
                x2, y1, z1, 4, 0x00000077,
                x1, y2, z2, 4, 0x00000077,
                TRUE);

            //
            // Draw the options for leaving this square.
            //

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
                                TRUE);
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
                            TRUE);
                    }
                }
            }
        }
}

//
// claude-ai: MAV_can_i_walk is the path-smoothing function: given a sequence of grid waypoints,
// claude-ai: it checks whether the agent can walk in a straight line between two grid cells
// claude-ai: by stepping through intermediate cells and checking MAV_CAPS_GOTO on each crossed edge.
// claude-ai: Used by MAV_get_first_action_from_nodelist to skip intermediate waypoints (path shortcutting).
// claude-ai: Only checks plain-walk (MAV_CAPS_GOTO), not jumps/climbs — those must be traversed cell-by-cell.
// Returns TRUE if you can walk from a to b. If it returns FALSE, then
// (MAV_last_x, MAV_last_z) is the last square it reached, and (MAV_dmx, MAV_dmz)
// is the direction it tried to leave the square in.
//

SLONG MAV_last_mx;
SLONG MAV_last_mz;
SLONG MAV_dmx;
SLONG MAV_dmz;

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
        return TRUE;
    } else {
        //
        // Normalise (dx,dz) to length 0x40 ish
        //

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

            //
            // Is there a wall in the way?
            //

            if (MAV_dmx == -1 && !(mo->opt[MAV_DIR_XS] & MAV_CAPS_GOTO)) {
                return FALSE;
            }
            if (MAV_dmx == +1 && !(mo->opt[MAV_DIR_XL] & MAV_CAPS_GOTO)) {
                return FALSE;
            }

            if (MAV_dmz == -1 && !(mo->opt[MAV_DIR_ZS] & MAV_CAPS_GOTO)) {
                return FALSE;
            }
            if (MAV_dmz == +1 && !(mo->opt[MAV_DIR_ZL] & MAV_CAPS_GOTO)) {
                return FALSE;
            }

            if (MAV_dmx && MAV_dmz) {
                //
                // We have to try the corner pieces as well because we are moving diagonally.
                //

                mo = &MAV_opt[MAV_NAV(mx, MAV_last_mz)];

                if (MAV_dmz == -1 && !(mo->opt[MAV_DIR_ZS] & MAV_CAPS_GOTO)) {
                    return FALSE;
                }
                if (MAV_dmz == +1 && !(mo->opt[MAV_DIR_ZL] & MAV_CAPS_GOTO)) {
                    return FALSE;
                }

                mo = &MAV_opt[MAV_NAV(MAV_last_mx, mz)];

                if (MAV_dmx == -1 && !(mo->opt[MAV_DIR_XS] & MAV_CAPS_GOTO)) {
                    return FALSE;
                }
                if (MAV_dmx == +1 && !(mo->opt[MAV_DIR_XL] & MAV_CAPS_GOTO)) {
                    return FALSE;
                }
            }

            if (mx == bx && mz == bz) {
                return TRUE;
            }

            MAV_last_mx = mx;
            MAV_last_mz = mz;
        }
    }
}

UBYTE MAV_start_x;
UBYTE MAV_start_z;

UBYTE MAV_dest_x;
UBYTE MAV_dest_z;

//
// claude-ai: MAV_LOOKAHEAD=32 — the A* search explores at most 32 steps from the agent's position.
// claude-ai: If the destination is farther away, the best partial path found so far is used (MAV_MAX_OVERFLOWS=8).
// claude-ai: MAV_node[] stores the recovered path (reversed: node[0]=furthest, node[node_upto-1]=first step).
// How much look-ahead we use in the navigation.  MAV_node[0] is the end of
// the looked-ahead route and (MAV_node_upto - 1) is the start(x,z)
//

// claude-ai: MAV_LOOKAHEAD=32: A* expands at most 32 grid cells from the agent before giving up on finding a direct path.
#define MAV_LOOKAHEAD 32

MAV_Action MAV_node[MAV_LOOKAHEAD];
SLONG MAV_node_upto;

//
// claude-ai: MAV_flag: packed A* open/closed set state. Each UBYTE holds 2 cells (4 bits per cell).
// claude-ai:   bits [2:0] = action taken to reach this cell (MAV_ACTION_* 0-7)
// claude-ai:   bit  [3]   = visited flag (1 = cell has been placed in the priority queue)
// claude-ai: MAV_dir: stores which direction (MAV_DIR_*) we came from (2 bits per cell, 4 cells per UBYTE).
// claude-ai: Both arrays are indexed [z][x/N] and cleared via MAV_clear_bbox before each A* run.
// Each UBYTE is four two squares. There is three bits for action, and one bit
// to say whether we have been here or not.
//

UBYTE MAV_flag[MAP_HEIGHT][MAP_WIDTH / 2];

//
// Each UBYTE is four squares-worth of the direction
// we have come from.
//

UBYTE MAV_dir[MAP_HEIGHT][MAP_WIDTH / 4];

//
// Functions to set bits...
//

inline void MAV_visited_set(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 1;
    SLONG bit = 8 << ((x & 0x1) << 2);

    MAV_flag[z][byte] |= bit;
}

inline SLONG MAV_visited_get(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 1;
    SLONG bit = 8 << ((x & 0x1) << 2);

    return (MAV_flag[z][byte] & bit);
}

//
// This function also sets the visited flag.
//

inline void MAV_action_set(SLONG x, SLONG z, SLONG dir)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 1;
    SLONG shift = (x & 0x1) << 2;

    dir |= 0x08; // Set the visited flag too.

    MAV_flag[z][byte] &= ~(0x7 << shift);
    MAV_flag[z][byte] |= (dir << shift);
}

inline SLONG MAV_action_get(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 1;
    SLONG shift = (x & 0x1) << 2;

    return ((MAV_flag[z][byte] >> shift) & 0x7);
}

inline void MAV_dir_set(SLONG x, SLONG z, SLONG dir)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 2;
    SLONG shift = (x & 0x3) << 1;

    MAV_dir[z][byte] &= ~(0x3 << shift);
    MAV_dir[z][byte] |= (dir << shift);
}

inline SLONG MAV_dir_get(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 2;
    SLONG shift = (x & 0x3) << 1;

    return ((MAV_dir[z][byte] >> shift) & 0x3);
}

//
// Clears the visited flags in given box. The
// other flags are undefined after this call.
//

void MAV_clear_bbox(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2)
{
    SLONG z;
    SLONG len;
    SLONG count;
    SLONG* zero;

    //
    // Round x1 down and x2 up to the nearest 4 byte boundary.
    //

    x2 += 0x7;
    x1 &= ~0x7;
    x2 &= ~0x7;

    SATURATE(x1, 0, MAP_WIDTH);
    SATURATE(x2, 0, MAP_WIDTH);
    SATURATE(z1, 0, MAP_HEIGHT);
    SATURATE(z2, 0, MAP_HEIGHT);

    //
    // Clear a SLONG at a time.
    //

    len = x2 - x1 >> 3;

    for (z = z1; z < z2; z++) {
        zero = (SLONG*)(&MAV_flag[z][x1 >> 1]);
        count = len;

        while (count--) {
            *zero++ = 0;
        }
    }
}

//
// claude-ai: MAV_get_first_action_from_nodelist implements PATH SMOOTHING for GOTO-only segments.
// claude-ai: It walks the nodelist from start backwards; for each GOTO waypoint it tries MAV_can_i_walk.
// claude-ai: The furthest waypoint reachable in a straight line becomes the immediate target.
// claude-ai: If a non-GOTO action (jump, climb, etc.) is encountered first, that action is returned immediately.
// claude-ai: Result: agents don't zigzag through grid cells — they walk directly toward the furthest visible point.
// Returns the first thing that should be done according to the current nodelist.
//

MAV_Action MAV_get_first_action_from_nodelist()
{
    SLONG i;

    UBYTE ax;
    UBYTE az;

    UBYTE bx;
    UBYTE bz;

    MAV_Action ans;

    //
    // Remember the last place we can walk to.
    //

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

//
// claude-ai: MAV_create_nodelist_from_pos: PATH RECONSTRUCTION after A* terminates.
// claude-ai: Traces back from end_x/end_z to MAV_start_x/z using stored MAV_dir and MAV_action values.
// claude-ai: Jump actions (JUMP, JUMPPULL) move 2 cells; JUMPPULL2 moves 3 cells — handled by extra back-steps.
// claude-ai: Result is stored in MAV_node[] array (reversed order: [0]=far end, [node_upto-1]=first step).
// Uses the MAV_flag and MAV_dir arrays to constuct a nodelist from
// from the given position back to (start_x,start_z).
//

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
            //
            // Moves you two squares.
            //

            x -= order[dir].dx;
            z -= order[dir].dz;
        }

        if (action == MAV_ACTION_JUMPPULL2) {
            //
            // Moves you three squares.
            //

            x -= order[dir].dx;
            z -= order[dir].dz;

            x -= order[dir].dx;
            z -= order[dir].dz;
        }
    }
}

//
// claude-ai: A* PRIORITY QUEUE setup. PQ_Type is the heap node: (x,z) position, score (heuristic), length (depth).
// claude-ai: PQ_HEAP_MAX_SIZE=256 — max nodes in the open set at any time.
// claude-ai: PQ_better compares by score (lower is better — pure heuristic, no g-cost, making this greedy best-first).
// claude-ai: Note: this is NOT classic A* (no accumulated path cost g). It's a greedy best-first / bounded BFS hybrid.
// The priority queue needs these definitions...
//

typedef struct
{
    UBYTE x;
    UBYTE z;
    UBYTE score; // The lower the score the better...
    UBYTE length;

} PQ_Type;

#define PQ_HEAP_MAX_SIZE 256

SLONG PQ_better(PQ_Type* a, PQ_Type* b)
{
    return a->score < b->score;
}

#include "pq.h"
#include "pq.cpp"

//
// claude-ai: MAV_score_pos: heuristic function — straight-line distance (QDIST2 = integer approximation of sqrt)
// claude-ai: from (x,z) to MAV_dest. Scaled x2 before QDIST2, capped at 255 for UBYTE storage.
// claude-ai: This is the only cost — no accumulated path cost — confirming greedy best-first search.
// Returns the score associated with the given position.
//

UBYTE MAV_score_pos(UBYTE x, UBYTE z)
{
    SLONG dx;
    SLONG dz;

    SLONG dist;

    //
    // Just return the distance to the destination.
    //

    dx = abs((MAV_dest_x - x) << 1);
    dz = abs((MAV_dest_z - z) << 1);

    dist = QDIST2(dx, dz);

    if (dist > 255) {
        dist = 255;
    }

    return dist;
}

UBYTE MAV_do_found_dest;

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

    //
    // Remember the destination.
    //

    MAV_start_x = me_x;
    MAV_start_z = me_z;

    MAV_dest_x = dest_x;
    MAV_dest_z = dest_z;

    //
    // Clear the flag by default.
    //

    // claude-ai: MAV_do is the main navigation function called by PCOM AI each tick for an NPC.
    // claude-ai: Parameters: agent grid position (me_x,me_z), target (dest_x,dest_z), capability mask (caps).
    // claude-ai: caps is ANDed with each cell's opt[dir] to filter moves the agent cannot perform.
    // claude-ai: Example: MAV_CAPS_DARCI=0xff allows all moves; regular cops have fewer bits set.
    // claude-ai: Returns a MAV_Action describing: action type, direction, and destination cell.
    // claude-ai: Sets MAV_do_found_dest=TRUE if the exact destination was reached during search.
    // claude-ai: PATH FAIL case: if PQ empties without reaching destination, returns best partial-path action.
    MAV_do_found_dest = FALSE;

    //
    // Clear the flags.
    //

    // claude-ai: Clear only the ±MAV_LOOKAHEAD box around the agent (not the whole 128x128 map).
    // claude-ai: This avoids resetting the entire flag array each tick — performance optimization.
    MAV_clear_bbox(
        me_x - MAV_LOOKAHEAD,
        me_z - MAV_LOOKAHEAD,
        me_x + MAV_LOOKAHEAD,
        me_z + MAV_LOOKAHEAD);

    // memset(MAV_flag, 0, sizeof(MAV_flag));

    //
    // Initialise the heap with our start square.
    //

    PQ_init();

    start.x = me_x;
    start.z = me_z;
    start.score = MAV_score_pos(me_x, me_z);
    start.length = 0;

    PQ_add(start);

    MAV_visited_set(me_x, me_z);

//
// Initialise the score and answer.
//

// claude-ai: MAV_MAX_OVERFLOWS=8: when the frontier reaches depth MAV_LOOKAHEAD, it counts as an "overflow".
// claude-ai: After 8 overflows the search stops and returns the best partial answer found so far (PATH_FAIL equivalent).
// claude-ai: This bounds the total work to O(LOOKAHEAD * MAX_OVERFLOWS) expansions per tick.
#define MAV_MAX_OVERFLOWS 8

    overflows = 0;
    best_score = INFINITY;
    ans.action = MAV_ACTION_GOTO;
    ans.dir = 0;
    ans.dest_x = me_x;
    ans.dest_z = me_z;

    while (1) {
        if (PQ_empty()) {
            break;
        }

        //
        // Get the best square so far and move it on a bit.
        //

        best = PQ_best();
        PQ_remove();

        if (best.length >= MAV_LOOKAHEAD) {
            if (best.score < best_score) {
                best_score = best.score;

                //
                // Work out the first action and put it in answer.
                //

                MAV_create_nodelist_from_pos(best.x, best.z);
                ans = MAV_get_first_action_from_nodelist();
            }

            overflows += 1;

            if (overflows >= MAV_MAX_OVERFLOWS) {
                //
                // Dont do any more calculation.
                //

                return ans;
            }

            continue;
        }

        if (best.x == MAV_dest_x && best.z == MAV_dest_z) {
            //
            // Found the destination.
            //

            MAV_do_found_dest = TRUE;

            //
            // Work out the first action and return it as the answer.
            //

            MAV_create_nodelist_from_pos(best.x, best.z);
            ans = MAV_get_first_action_from_nodelist();

            return ans;
        }

        ASSERT(WITHIN(best.x, 0, MAP_WIDTH - 1));
        ASSERT(WITHIN(best.z, 0, MAP_HEIGHT - 1));

        ASSERT(WITHIN(MAV_NAV(best.x, best.z), 0, MAV_opt_upto - 1));

        mo = &MAV_opt[MAV_NAV(best.x, best.z)];

        //
        // claude-ai: NEIGHBOUR EXPANSION — A* open-set growth:
        // claude-ai:   opt = cell's movement options ANDed with agent's caps bitmask.
        // claude-ai:   move_one:   MAV_CAPS_GOTO|PULLUP|CLIMB_OVER|FALL_OFF|LADDER_UP — move 1 cell.
        // claude-ai:   move_two:   MAV_CAPS_JUMP|JUMPPULL — move 2 cells (skip intermediate).
        // claude-ai:   move_three: MAV_CAPS_JUMPPULL2 — move 3 cells.
        // claude-ai: Non-GOTO actions add a score penalty (bias = Random()&3 + 3) to prefer normal walking.
        // claude-ai: FALL_OFF gets no extra penalty (falling is "free").
        // claude-ai: Only unvisited cells are added (MAV_visited_get check).
        // Add neighbouring squares to the priority queue.
        //

        for (i = 0; i < 4; i++) {
            opt = mo->opt[i] & caps;

            //
            // Moving one square.
            //

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

                    //
                    // What action did we use to get here?
                    //

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

                    //
                    // How we reached this square.
                    //

                    MAV_action_set( // Sets the visited flag aswell.
                        mx,
                        mz,
                        action);

                    MAV_dir_set(
                        mx,
                        mz,
                        i);

                    if (action != MAV_ACTION_GOTO) {
                        //
                        // Bias against using strange methods of travel!
                        //

                        next.score += Random() & 0x3;

                        if (action == MAV_ACTION_FALL_OFF) {
                            //
                            // Falling off isn't so bad...
                            //
                        } else {
                            next.score += 3;
                        }
                    }

                    //
                    // Add this square to the priority queue.
                    //

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

                    //
                    // What action did we use to get here?
                    //

                    if (opt & MAV_CAPS_JUMP) {
                        action = MAV_ACTION_JUMP;
                    } else if (opt & MAV_CAPS_JUMPPULL) {
                        action = MAV_ACTION_JUMPPULL;
                    } else
                        ASSERT(0);

                    //
                    // How we reached this square.
                    //

                    MAV_action_set( // Sets the visited flag aswell.
                        mx,
                        mz,
                        action);

                    MAV_dir_set(
                        mx,
                        mz,
                        i);

                    //
                    // Add this square to the priority queue.
                    //

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

                    //
                    // What action did we use to get here?
                    //

                    action = MAV_ACTION_JUMPPULL2;

                    //
                    // How we reached this square.
                    //

                    MAV_action_set( // Sets the visited flag aswell.
                        mx,
                        mz,
                        action);

                    MAV_dir_set(
                        mx,
                        mz,
                        i);

                    //
                    // Add this square to the priority queue.
                    //

                    PQ_add(next);
                }
            }
        }
    }

    return ans;
}

// claude-ai: MAV_inside: returns TRUE if world-space point (x,y,z) is below the MAVHEIGHT surface.
// claude-ai: Used by MAV_height_los_fast/slow to test whether a line-of-sight ray goes underground.
// claude-ai: x/z are in game units (>>8 to get grid cell), y is in game units (>>6 to get MAVHEIGHT units).
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
            return TRUE;
        }
        if (y > +127) {
            return FALSE;
        }

        if (y < MAVHEIGHT(x, z)) {
            return TRUE;
        }
    }

    return FALSE;
}

SLONG MAV_height_los_fail_x;
SLONG MAV_height_los_fail_y;
SLONG MAV_height_los_fail_z;

// claude-ai: MAV_height_los_fast: fast terrain LOS using the MAVHEIGHT grid (not full geometry).
// claude-ai: Steps along the ray in increments of ~1 cell (dist>>8 steps). Returns FALSE if any step is underground.
// claude-ai: Records the last valid point before failure in MAV_height_los_fail_*.
// claude-ai: Used by AI for visibility/sighting checks over the height map rather than full collision geometry.
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

            return FALSE;
        }

        x += dx;
        y += dy;
        z += dz;
    }

    return TRUE;
}

// claude-ai: MAV_height_los_slow: like MAV_height_los_fast but optionally uses WARE_inside() for warehouse interiors.
// claude-ai: When ware!=0, tests each step against the warehouse bounding volume instead of global MAVHEIGHT.
// claude-ai: Starts from (x1+dx, y1+dy, z1+dz) (skips the source point to avoid self-occlusion).
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

            return FALSE;
        }

        x += dx;
        y += dy;
        z += dz;
    }

    return TRUE;
}


// claude-ai: MAV_precalculate_warehouse_nav: builds a SEPARATE nav grid for a single warehouse interior.
// claude-ai: Warehouses are large indoor areas with PAP_FLAG_HIDDEN set on their cells.
// claude-ai: The function temporarily redirects MAV_nav to WARE_nav[ww->nav] with the warehouse's own pitch.
// claude-ai: Only cells inside the warehouse bounding box (ww->minx..maxx, ww->minz..maxz) are processed.
// claude-ai: Navigation rules are slightly different: no "both_ground" check; max walk dh=1 always.
// claude-ai: After precalculation, MAV_nav is restored to the global city grid pointer.
// claude-ai: Port note: warehouse nav grids must be stored separately from the main city grid.
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

    //
    // Remember the old MAV_nav array.
    //

    UWORD* old_mav_nav = MAV_nav;
    SLONG old_mav_nav_pitch = MAV_nav_pitch;

    //
    // Set the MAV_nav array to point to the warehouse's private array.
    //

    MAV_nav = &WARE_nav[ww->nav];
    MAV_nav_pitch = ww->nav_pitch;

    //
    // Make the staircase prims change the MAV_height array
    //

    OB_Info* oi;

    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            for (oi = OB_find(x, z); oi->prim; oi++) {
                if (oi->prim == 41) {
                    //
                    // The step prim!
                    //

                    //
                    // Find which mapsquare the middle of this prim is over.
                    //

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

                    if (WITHIN(rx, ww->minx, ww->maxx) && WITHIN(rz, ww->minz, ww->maxz)) {
                        MAVHEIGHT(rx, rz) = oi->y + 0x40 >> 6;
                    }
                }
            }
        }

    //
    // Work out the mav for each square in the bounding box of the warehouse.
    //

    for (x = ww->minx; x <= ww->maxx; x++)
        for (z = ww->minz; z <= ww->maxz; z++) {
            mx = x - ww->minx;
            mz = z - ww->minz;

            //
            // Look for a nearby ladder.
            //

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
                    //
                    // This square is outside the warehouse.
                    //

                    continue;
                }

                if (!WITHIN(tx, ww->minx, ww->maxx) || !WITHIN(tz, ww->minz, ww->maxz)) {
                    //
                    // Cannot navigate in this direction because it is
                    // outside the warehouse.
                    //

                    continue;
                }

                if (!(PAP_2HI(tx, tz).Flags & PAP_FLAG_HIDDEN)) {
                    //
                    // This square is outside the warehouse.
                    //

                    continue;
                }

                //
                // Can we walk from (x,z) to (tx,tz)?
                //

                dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                if (abs(dh) > 1) {
                    //
                    // There are at different heights, so you cant
                    // just walk between the two squares.
                    //

                    if (dh < 0) {
                        //
                        // There might be a wall or fence in the way.
                        //

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
                            //
                            // We can always fall down becuase there is nothing in the way.
                            //

                            opt[i] |= MAV_CAPS_FALL_OFF;
                        } else {
                            //
                            // If there is a fence in the way, then we can scale the fence.
                            //

                            DFacet* df = &dfacets[los_failure_dfacet];

                            if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                                if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                                    //
                                    // Unclimbable fence.
                                    //
                                } else {
                                    //
                                    // We can scale this fence.
                                    //

                                    opt[i] |= MAV_CAPS_CLIMB_OVER;
                                }
                            }
                        }
                    } else {
                        if (WITHIN(dh, 3, 5)) {
                            //
                            // We can pull ourselves up.
                            //

                            opt[i] |= MAV_CAPS_PULLUP;
                        }

                        if (ladder) {
                            DFacet* df_ladder;

                            ASSERT(WITHIN(ladder, 1, next_dfacet - 1));

                            df_ladder = &dfacets[ladder];

                            ASSERT(df_ladder->FacetType == STOREY_TYPE_LADDER);

                            //
                            // There is a ladder- can we climb up it in this direction?
                            //

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
                    //
                    // There might be a wall or fence in the way.
                    //

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
                        //
                        // Nothing in the way...
                        //

                        opt[i] |= MAV_CAPS_GOTO;
                    } else {
                        //
                        // If there is a fence in the way, then we can scale the fence.
                        //

                        DFacet* df = &dfacets[los_failure_dfacet];

                        if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                            if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                                //
                                // Unclimbable fence.
                                //
                            } else {
                                //
                                // We can scale this fence.
                                //

                                opt[i] |= MAV_CAPS_CLIMB_OVER;
                            }
                        }
                    }
                }

                // claude-ai: If no direct walk or fence-climb is possible, try jump moves.
                // claude-ai: Jump checks 2 cells ahead (tx+dx) and then 3 cells ahead (tx+dx+dx).
                // claude-ai: JUMP: 2 cells, dh<2. JUMPPULL: 2 cells, dh in [2,5]. JUMPPULL2: 3 cells, dh in (-6,4].
                // claude-ai: Each requires LOS at jump height: MAVHEIGHT<<6 + 0xa0 (approx head+shoulder height).
                if (!(opt[i] & MAV_CAPS_GOTO) && !(opt[i] & MAV_CAPS_CLIMB_OVER)) {
                    //
                    // Now what about jumping one block?
                    //

                    tx += dx;
                    tz += dz;

                    if (WITHIN(tx, ww->minx, ww->maxx) && WITHIN(tz, ww->minz, ww->maxz) && (PAP_2HI(tx, tz).Flags & PAP_FLAG_HIDDEN)) {
                        dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                        //
                        // Can we jump there?
                        //

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

                        //
                        // What about jumping two blocks?
                        //

                        tx += dx;
                        tz += dz;

                        if (WITHIN(tx, 0, MAP_WIDTH - 1) && WITHIN(tz, 0, MAP_HEIGHT - 1)) {
                            dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                            if (dh > 4) {
                                //
                                // Can't make this jump
                                //
                            } else {
                                if (dh > -6) {
                                    //
                                    // Can we jump there?
                                    //

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

    //
    // A hack for the staircase prim.
    //

    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            for (oi = OB_find(x, z); oi->prim; oi++) {
                if (oi->prim == 41) {
                    //
                    // The step prim!
                    //

                    {
                        //
                        // Find which mapsquare the middle of this prim is over.
                        //

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

                        if (WITHIN(rx, ww->minx, ww->maxx) && WITHIN(rz, ww->minz, ww->maxz)) {
                            mx = rx - ww->minx;
                            mz = rz - ww->minz;

                            if (oi->yaw < 256 || oi->yaw > 1792 || WITHIN(oi->yaw, 768, 1280)) {
                                if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx - 1, rz)) {
                                    //
                                    // Walking left-right on a wide staircase.
                                    //
                                } else {
                                    MAV_turn_movement_off(mx, mz, MAV_DIR_XS);
                                    MAV_turn_movement_off(mx - 1, mz, MAV_DIR_XL);
                                }

                                if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx + 1, rz)) {
                                    //
                                    // Walking left-right on a wide staircase.
                                    //
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
                                    //
                                    // Walking across a wide staircase.
                                    //
                                } else {
                                    MAV_turn_movement_off(mx, mz, MAV_DIR_ZS);
                                    MAV_turn_movement_off(mx, mz - 1, MAV_DIR_ZL);
                                }

                                if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx, rz + 1)) {
                                    //
                                    // Walking across a wide staircase.
                                    //
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

    //
    // Restore the old MAV_nav array.
    //

    MAV_nav = old_mav_nav;
    MAV_nav_pitch = old_mav_nav_pitch;
}

// claude-ai: MAV_get_caps: reads the raw capability bitmask for one direction from a given cell.
// claude-ai: Returns 0 if out of bounds. Used by external code to query what moves are possible from a cell.
// claude-ai: Works on whatever MAV_nav is currently active (city grid or warehouse grid).
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

// claude-ai: MAV_turn_car_movement_on/off: runtime helpers to toggle car-nav edges (e.g. when a car blocks a road).
// claude-ai: Operate on MAV_CAR bits in the packed MAV_nav UWORD array — separate from foot-nav MAV_opt data.
void MAV_turn_car_movement_on(UBYTE mx, UBYTE mz, UBYTE dir)
{
    UBYTE mav;

    ASSERT(WITHIN(mx, 0, PAP_SIZE_HI - 1));
    ASSERT(WITHIN(mz, 0, PAP_SIZE_HI - 1));

    mav = MAV_CAR(mx, mz);
    mav |= 1 << dir;

    SET_MAV_CAR(mx, mz, mav);
}

void MAV_turn_car_movement_off(UBYTE mx, UBYTE mz, UBYTE dir)
{
    UBYTE mav;

    ASSERT(WITHIN(mx, 0, PAP_SIZE_HI - 1));
    ASSERT(WITHIN(mz, 0, PAP_SIZE_HI - 1));

    mav = MAV_CAR(mx, mz);
    mav &= ~(1 << dir);

    SET_MAV_CAR(mx, mz, mav);
}
