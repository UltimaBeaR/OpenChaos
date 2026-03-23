#include <platform.h>
#include "missions/game_types.h"
#include "actors/core/interact.h"      // calc_sub_objects_position
#include "actors/characters/anim_ids.h"
#include "world/environment/tripwire.h"
#include "world/environment/tripwire_globals.h"

// uc_orig: TRIP_NOT_COLLIDED (fallen/Source/trip.cpp)
#define TRIP_NOT_COLLIDED (-1)

// uc_orig: TRIP_init (fallen/Source/trip.cpp)
void TRIP_init()
{
    TRIP_wire_upto = 1;
}

// uc_orig: TRIP_create (fallen/Source/trip.cpp)
UBYTE TRIP_create(
    SLONG y,
    SLONG x1,
    SLONG z1,
    SLONG x2,
    SLONG z2)
{
    SLONG i;
    TRIP_Wire* tw;

    // Check if an identical wire already exists at this position.
    for (i = 1; i < TRIP_wire_upto; i++) {
        tw = &TRIP_wire[i];
        if (tw->y == y && tw->x1 == x1 && tw->z1 == z1 && tw->x2 == x2 && tw->z2 == z2) {
            return i;
        }
    }

    if (TRIP_wire_upto >= TRIP_MAX_WIRES) {
        return NULL;
    }

    tw = &TRIP_wire[TRIP_wire_upto++];
    tw->counter = rand();
    tw->y = y;
    tw->x1 = x1;
    tw->z1 = z1;
    tw->x2 = x2;
    tw->z2 = z2;
    tw->along = 255;
    tw->wait = 0;

    return TRIP_wire_upto - 1;
}

// uc_orig: TRIP_collide (fallen/Source/trip.cpp)
// Tests if point (x,z) lies along the segment (x1,z1)-(x2,z2).
// Returns the 8-bit fixed-point distance along the segment, or TRIP_NOT_COLLIDED.
static SLONG TRIP_collide(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG x,
    SLONG z)
{
    SLONG dx = x2 - x1;
    SLONG dz = z2 - z1;
    SLONG da = x - x1;
    SLONG db = z - z1;

    SLONG cprod = da * dz - db * dx;
    SLONG len = QDIST2(abs(dx), abs(dz)) + 1;
    SLONG dist = cprod / len;

    if (abs(dist) < 0x20) {
        SLONG dprod = da * dx + db * dz;
        SLONG along = dprod / len;

        // 8-bit fixed point
        along = (along * 256) / len;

        if (WITHIN(along, 0, 256)) {
            return along;
        }
    }

    return TRIP_NOT_COLLIDED;
}

// uc_orig: TRIP_process (fallen/Source/trip.cpp)
void TRIP_process()
{
    SLONG i;
    SLONG lfx, lfy, lfz;
    SLONG rfx, rfy, rfz;
    SLONG hx, hy, hz;
    SLONG tx, tz;
    SLONG dist;
    SLONG along;
    SLONG feet, head;

    TRIP_Wire* tw;
    Thing* darci = NET_PERSON(0);

    for (i = 1; i < TRIP_wire_upto; i++) {
        tw = &TRIP_wire[i];

        if (tw->x1 == TRIP_X1_DEACTIVATED) {
            continue;
        }

        tw->counter += 50;

        if (tw->wait) {
            ASSERT(tw->along != 255);
            tw->wait -= 1;
            continue;
        }

        tw->along = 255;

        SLONG dx = (darci->WorldPos.X >> 8) - tw->x1;
        SLONG dz = (darci->WorldPos.Z >> 8) - tw->z1;
        dist = abs(dx) + abs(dz);

        if (dist > 0x500) {
            continue;
        }

        // Sample Darci's foot and head positions.
        calc_sub_objects_position(darci, darci->Draw.Tweened->AnimTween, SUB_OBJECT_LEFT_FOOT, &lfx, &lfy, &lfz);
        calc_sub_objects_position(darci, darci->Draw.Tweened->AnimTween, SUB_OBJECT_RIGHT_FOOT, &rfx, &rfy, &rfz);

        feet = MIN(lfy, rfy);
        feet += darci->WorldPos.Y >> 8;

        calc_sub_objects_position(darci, darci->Draw.Tweened->AnimTween, SUB_OBJECT_HEAD, &hx, &hy, &hz);
        head = hy + (darci->WorldPos.Y >> 8);

        if (head < feet) {
            // Darci can do a somersault.
            SWAP(head, feet);
        }

        if (!WITHIN(tw->y, feet - 0x10, head + 0x10)) {
            continue;
        }

        along = TRIP_collide(
            tw->x1, tw->z1,
            tw->x2, tw->z2,
            darci->WorldPos.X >> 8,
            darci->WorldPos.Z >> 8);

        if (along != TRIP_NOT_COLLIDED) {
            if (along == 255) {
                along = 254;
            }
            tw->along = along;
            tw->wait = 4;
        }
    }
}

// uc_orig: TRIP_get_start (fallen/Source/trip.cpp)
void TRIP_get_start()
{
    TRIP_get_upto = 1;
}

// uc_orig: TRIP_get_next (fallen/Source/trip.cpp)
TRIP_Info* TRIP_get_next()
{
    TRIP_Wire* tw;

tail_recurse:;

    if (TRIP_get_upto >= TRIP_wire_upto) {
        return NULL;
    }

    ASSERT(WITHIN(TRIP_get_upto, 0, TRIP_wire_upto - 1));

    tw = &TRIP_wire[TRIP_get_upto++];

    if (tw->x1 == TRIP_X1_DEACTIVATED) {
        goto tail_recurse;
    }

    TRIP_get_info.y = tw->y;
    TRIP_get_info.x1 = tw->x1;
    TRIP_get_info.z1 = tw->z1;
    TRIP_get_info.x2 = tw->x2;
    TRIP_get_info.z2 = tw->z2;
    TRIP_get_info.counter = tw->counter;
    TRIP_get_info.along = tw->along;

    return &TRIP_get_info;
}

// uc_orig: TRIP_activated (fallen/Source/trip.cpp)
SLONG TRIP_activated(UBYTE tripwire)
{
    TRIP_Wire* tw;

    ASSERT(WITHIN(tripwire, 0, TRIP_wire_upto - 1));

    tw = &TRIP_wire[tripwire];
    return (tw->along != 255);
}

// uc_orig: TRIP_deactivate (fallen/Source/trip.cpp)
void TRIP_deactivate(UBYTE tripwire)
{
    TRIP_Wire* tw;

    ASSERT(WITHIN(tripwire, 0, TRIP_wire_upto - 1));

    tw = &TRIP_wire[tripwire];
    tw->x1 = TRIP_X1_DEACTIVATED;
}
