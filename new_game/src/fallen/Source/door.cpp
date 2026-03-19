// claude-ai: Door system for Urban Chaos.
// claude-ai: Doors are DFacet objects with STOREY_TYPE_OUTSIDE_DOOR and/or FACET_FLAG_OPEN.
// claude-ai: The system manages up to DOOR_MAX_DOORS simultaneously animated doors.
// claude-ai: Opening/closing: df->Open (0=closed, 128/255=fully open) animated in steps of 4 per frame.
// claude-ai: When a door opens/closes, MAV (navigation) movement is enabled/disabled across the door span.
// claude-ai: PORT NOTE: In new game, keep the MAV navmesh enable/disable logic tied to door state changes.
//
// Doors
//

#include "game.h"
#include "door.h"
#include "mav.h"
#include "supermap.h"
#include "memory.h"

//
// Doors in the process of opening or closing.
//

// claude-ai: Global array of all currently animating doors (opening or closing).
// claude-ai: Slot is freed (facet=NULL) when animation completes.
DOOR_Door* DOOR_door;

// claude-ai: DOOR_find: Spatial search for the nearest STOREY_TYPE_OUTSIDE_DOOR facet to a world position.
// claude-ai: Searches a 0x200-unit radius in the low-res PAP grid (PAP_2LO).
// claude-ai: Returns the index into dfacets[], or NULL if no door found nearby.
// claude-ai: Used by DOOR_open/DOOR_shut when a script triggers a door by world position.
//
// Finds a door facet.
//

UWORD DOOR_find(
    SLONG world_x,
    SLONG world_z)
{
    SLONG fx;
    SLONG fz;
    SLONG dx;
    SLONG dz;
    SLONG score;

    SLONG mx1 = world_x - 0x200 >> PAP_SHIFT_LO;
    SLONG mz1 = world_z - 0x200 >> PAP_SHIFT_LO;
    SLONG mx2 = world_x + 0x200 >> PAP_SHIFT_LO;
    SLONG mz2 = world_z + 0x200 >> PAP_SHIFT_LO;

    SLONG mx;
    SLONG mz;

    SLONG f_list;
    SLONG exit;
    SLONG i_facet;

    DFacet* df;

    SATURATE(mx1, 0, PAP_SIZE_LO - 1);
    SATURATE(mz1, 0, PAP_SIZE_LO - 1);
    SATURATE(mx2, 0, PAP_SIZE_LO - 1);
    SATURATE(mz2, 0, PAP_SIZE_LO - 1);

    SLONG best_score = INFINITY;
    SLONG best_facet = NULL;

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            f_list = PAP_2LO(mx, mz).ColVectHead;

            if (f_list) {
                exit = FALSE;

                while (1) {
                    i_facet = facet_links[f_list];

                    if (i_facet < 0) {
                        i_facet = -i_facet;
                        exit = TRUE;
                    }

                    df = &dfacets[i_facet];

                    if (df->FacetType == STOREY_TYPE_OUTSIDE_DOOR) {
                        fx = df->x[0] + df->x[1] << 7;
                        fz = df->z[0] + df->z[1] << 7;

                        dx = abs(fx - world_x);
                        dz = abs(fz - world_z);

                        score = QDIST2(dx, dz);

                        if (best_score > score) {
                            best_score = score;
                            best_facet = i_facet;
                        }
                    }

                    if (exit) {
                        break;
                    }

                    f_list += 1;
                }
            }
        }

    return best_facet;
}

// claude-ai: DOOR_open: Triggers a door to begin opening.
// claude-ai: Sets FACET_FLAG_OPEN on the facet, registers it in DOOR_door[] for animation.
// claude-ai: Also walks the door span and enables MAV movement links on both sides (pedestrian + vehicle).
// claude-ai: The door direction (dx/dz) determines which MAV_DIR_* sides to enable (left/right of door wall).
void DOOR_open(SLONG world_x, SLONG world_z)
{
    UWORD facet;

    facet = DOOR_find(world_x, world_z);

    if (facet) {
        SLONG i;

        SLONG x;
        SLONG z;

        SLONG dx;
        SLONG dz;

        SLONG mx;
        SLONG mz;

        SLONG left;
        SLONG right;

        DFacet* df = &dfacets[facet];

        //
        // Start it opening.
        //

        // claude-ai: Set the OPEN flag so DOOR_process() will animate df->Open toward max.
        df->FacetFlags |= FACET_FLAG_OPEN;

        for (i = 0; i < DOOR_MAX_DOORS; i++) {
            if (DOOR_door[i].facet == facet) {
                goto already_being_processed;
            }
        }

        for (i = 0; i < DOOR_MAX_DOORS; i++) {
            if (DOOR_door[i].facet == NULL) {
                DOOR_door[i].facet = facet;
            }
        }

    already_being_processed:;

        //
        // Make sure nobody can navigate through this facet.
        //

        dx = SIGN(df->x[1] - df->x[0]);
        dz = SIGN(df->z[1] - df->z[0]);

        x = df->x[0];
        z = df->z[0];

        if (dx) {
            if (dx == 1) {
                left = MAV_DIR_ZS;
                right = MAV_DIR_ZL;
            } else {
                left = MAV_DIR_ZL;
                right = MAV_DIR_ZS;
            }
        } else {
            if (dz == 1) {
                left = MAV_DIR_XL;
                right = MAV_DIR_XS;
            } else {
                left = MAV_DIR_XS;
                right = MAV_DIR_XL;
            }
        }

        while (1) {
            mx = (x << 8) + (dx << 7);
            mz = (z << 8) + (dz << 7);

            mx += -dz << 7;
            mz += +dx << 7;

            mx >>= 8;
            mz >>= 8;

            MAV_turn_movement_on(mx, mz, left);
            MAV_turn_car_movement_on(mx, mz, left);

            mx = (x << 8) + (dx << 7);
            mz = (z << 8) + (dz << 7);

            mx -= -dz << 7;
            mz -= +dx << 7;

            mx >>= 8;
            mz >>= 8;

            MAV_turn_movement_on(mx, mz, right);
            MAV_turn_car_movement_on(mx, mz, right);

            x += dx;
            z += dz;

            if (x == df->x[1] && z == df->z[1]) {
                break;
            }
        }
    }
}

// claude-ai: DOOR_shut: Triggers a door to begin closing.
// claude-ai: Clears FACET_FLAG_OPEN so DOOR_process() will animate df->Open back toward 0.
// claude-ai: Also disables MAV navigation movement through the door span (blocks pathfinding).
void DOOR_shut(SLONG world_x, SLONG world_z)
{
    UWORD facet;

    facet = DOOR_find(world_x, world_z);

    if (facet) {
        SLONG i;

        SLONG x;
        SLONG z;

        SLONG dx;
        SLONG dz;

        SLONG mx;
        SLONG mz;

        SLONG left;
        SLONG right;

        DFacet* df = &dfacets[facet];

        //
        // Start it shutting.
        //

        df->FacetFlags &= ~FACET_FLAG_OPEN;

        for (i = 0; i < DOOR_MAX_DOORS; i++) {
            if (DOOR_door[i].facet == facet) {
                goto already_being_processed;
            }
        }

        for (i = 0; i < DOOR_MAX_DOORS; i++) {
            if (DOOR_door[i].facet == NULL) {
                DOOR_door[i].facet = facet;
            }
        }

    already_being_processed:;

        //
        // Make sure nobody can navigate through this facet.
        //

        dx = SIGN(df->x[1] - df->x[0]);
        dz = SIGN(df->z[1] - df->z[0]);

        x = df->x[0];
        z = df->z[0];

        if (dx) {
            //
            // This is right...
            //

            if (dx == 1) {
                left = MAV_DIR_ZS;
                right = MAV_DIR_ZL;
            } else {
                left = MAV_DIR_ZL;
                right = MAV_DIR_ZS;
            }
        } else {
            if (dz == 1) {
                left = MAV_DIR_XL;
                right = MAV_DIR_XS;
            } else {
                left = MAV_DIR_XS;
                right = MAV_DIR_XL;
            }
        }

        while (1) {
            mx = (x << 8) + (dx << 7);
            mz = (z << 8) + (dz << 7);

            mx += -dz << 7;
            mz += +dx << 7;

            mx >>= 8;
            mz >>= 8;

            MAV_turn_movement_off(mx, mz, left);
            MAV_turn_car_movement_off(mx, mz, left);

            mx = (x << 8) + (dx << 7);
            mz = (z << 8) + (dz << 7);

            mx -= -dz << 7;
            mz -= +dx << 7;

            mx >>= 8;
            mz >>= 8;

            MAV_turn_movement_off(mx, mz, right);
            MAV_turn_car_movement_off(mx, mz, right);

            x += dx;
            z += dz;

            if (x == df->x[1] && z == df->z[1]) {
                break;
            }
        }
    }
}

// claude-ai: DOOR_knock_over_people: Stub — was intended to knock characters over when a door swings open.
// claude-ai: Body is empty ("there are more important things to do!"). Not implemented in this codebase.
// claude-ai: PORT NOTE: In new game, could add ragdoll/pushback here when door strikes a character.
//
// Make the door knock people over as it opens...
//

void DOOR_knock_over_people(DFacet* df, SLONG side)
{
    SLONG dx;
    SLONG dz;

    SLONG x1;
    SLONG z1;

    SLONG x2;
    SLONG z2;

    dx = df->x[1] - df->x[0];
    dz = df->z[1] - df->z[0];

    //
    // Well.. there are more important things to do!
    //
}

// claude-ai: DOOR_process: Per-frame update for all animating doors.
// claude-ai: Called every game tick to increment/decrement df->Open by 4 steps.
// claude-ai: df->Open range: 0 (fully closed) to 128 or 255 (fully open), minus DOOR_AJAR=15 slop.
// claude-ai: FACET_FLAG_90DEGREE doors rotate only 90 degrees (max=128), others rotate 180 (max=255).
// claude-ai: When animation completes, the slot in DOOR_door[] is freed (facet = NULL).
// claude-ai: PORT NOTE: This drives the door animation angle. In new game, use df->Open as 0..1 blend value.
void DOOR_process()
{
    SLONG i;
    SLONG open;
    SLONG max;

    DFacet* df;

    for (i = 0; i < DOOR_MAX_DOORS; i++) {
        if (DOOR_door[i].facet) {
            df = &dfacets[DOOR_door[i].facet];

            open = df->Open;

            if (df->FacetFlags & FACET_FLAG_OPEN) {
                // claude-ai: Door is opening: step Open up by 4 per frame toward max.
                open += 4;

#define DOOR_AJAR 15
                // claude-ai: DOOR_AJAR=15: leaves door slightly ajar (not fully 90/180 degrees) for gameplay reasons.

                if (df->FacetFlags & FACET_FLAG_90DEGREE) {
                    // claude-ai: 90-degree doors (e.g. car doors, some interior doors) stop at 128-15=113.
                    max = 128 - DOOR_AJAR;
                } else {
                    // claude-ai: Standard doors rotate a full 180 degrees, stopping at 255-15=240.
                    max = 255 - DOOR_AJAR;
                }

                if (open > max) {
                    open = max;

                    DOOR_door[i].facet = NULL;
                }
            } else {
                open -= 4;

                if (open < 0) {
                    open = 0;

                    DOOR_door[i].facet = NULL;
                }
            }

            df->Open = open;
        }
    }
}
