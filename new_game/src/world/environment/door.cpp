#include "world/environment/door.h"
#include "world/environment/door_globals.h"
#include "world/map/pap.h"
#include "world/map/pap_globals.h"

// Temporary includes — not yet migrated:
#include <MFStdLib.h>
#include "missions/game_types.h"
#include "world/map/supermap.h"
#include "world/environment/building.h"
#include "world/level_pools.h"
#include "ai/mav.h"

// Finds the nearest STOREY_TYPE_OUTSIDE_DOOR facet within 0x200 units of the given world position.
// Searches the lo-res PAP grid via the collision vector linked lists.
// Returns facet index, or 0 (NULL) if none found nearby.
// uc_orig: DOOR_find (fallen/Source/door.cpp)
static UWORD DOOR_find(
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

    SLONG best_score = UC_INFINITY;
    SLONG best_facet = NULL;

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            f_list = PAP_2LO(mx, mz).ColVectHead;

            if (f_list) {
                exit = UC_FALSE;

                while (1) {
                    i_facet = facet_links[f_list];

                    if (i_facet < 0) {
                        i_facet = -i_facet;
                        exit = UC_TRUE;
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

// Stub: was intended to apply forces to characters in the door's sweep arc.
// Not implemented in this codebase (body is empty).
// uc_orig: DOOR_knock_over_people (fallen/Source/door.cpp)
static void DOOR_knock_over_people(DFacet* df, SLONG side)
{
    SLONG dx;
    SLONG dz;

    SLONG x1;
    SLONG z1;

    SLONG x2;
    SLONG z2;

    dx = df->x[1] - df->x[0];
    dz = df->z[1] - df->z[0];

    // Not implemented.
    (void)dx; (void)dz; (void)x1; (void)z1; (void)x2; (void)z2; (void)side;
}

// Begins opening the door nearest to (world_x, world_z).
// Sets FACET_FLAG_OPEN, registers the facet in DOOR_door[] for per-frame animation,
// and enables MAV navigation links across the door span so pedestrians/vehicles can pass.
// uc_orig: DOOR_open (fallen/Source/door.cpp)
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

        // Determine which MAV direction pairs are "left" and "right" of this door wall.
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

// Begins closing the door nearest to (world_x, world_z).
// Clears FACET_FLAG_OPEN so the door animates back to closed,
// and disables MAV navigation links across the door span.
// uc_orig: DOOR_shut (fallen/Source/door.cpp)
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

// Per-frame update for all animating doors.
// Increments or decrements df->Open by 4 each tick toward the target state.
// FACET_FLAG_90DEGREE doors rotate 90 degrees (max 128 - DOOR_AJAR),
// standard doors rotate 180 degrees (max 255 - DOOR_AJAR).
// Frees the DOOR_door[] slot when animation completes.
// uc_orig: DOOR_process (fallen/Source/door.cpp)
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
                open += 4;

#define DOOR_AJAR 15

                if (df->FacetFlags & FACET_FLAG_90DEGREE) {
                    max = 128 - DOOR_AJAR;
                } else {
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
