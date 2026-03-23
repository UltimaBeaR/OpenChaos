// Temporary: game.h must be first — sets up all cross-module types
#include "fallen/Headers/game.h"
#include "actors/characters/anim_ids.h"
#include "fallen/Headers/mav.h"     // Temporary: MAV_inside
#include "actors/items/balloon.h"
#include "actors/items/balloon_globals.h"

// Desired distance between adjacent balloon string points.
// uc_orig: BALLOON_POINT_DIST (fallen/Source/balloon.cpp)
#define BALLOON_POINT_DIST (0x2000)

// Returns the world-space attachment point on a thing for a balloon string.
// Only CLASS_PERSON is supported.
// uc_orig: BALLOON_get_attached_point (fallen/Source/balloon.cpp)
static void BALLOON_get_attached_point(UWORD thing, SLONG* ax, SLONG* ay, SLONG* az)
{
    SLONG px;
    SLONG py;
    SLONG pz;

    Thing* p_thing = TO_THING(thing);

    switch (p_thing->Class) {
    case CLASS_PERSON:
        calc_sub_objects_position(
            p_thing,
            p_thing->Draw.Tweened->AnimTween,
            SUB_OBJECT_LEFT_HAND,
            &px,
            &py,
            &pz);

        px <<= 8;
        py <<= 8;
        pz <<= 8;

        px += p_thing->WorldPos.X;
        py += p_thing->WorldPos.Y;
        pz += p_thing->WorldPos.Z;

        break;

    default:
        ASSERT(0);
        break;
    }

    *ax = px;
    *ay = py;
    *az = pz;
}

// uc_orig: BALLOON_init (fallen/Source/balloon.cpp)
void BALLOON_init()
{
    SLONG i;

    memset(BALLOON_balloon, 0, sizeof(BALLOON_Balloon) * BALLOON_MAX_BALLOONS);

    BALLOON_balloon_upto = 1;
}

// uc_orig: BALLOON_create (fallen/Source/balloon.cpp)
UBYTE BALLOON_create(UWORD thing, UBYTE type)
{
    SLONG i;
    SLONG ax;
    SLONG ay;
    SLONG az;

    Thing* p_thing = TO_THING(thing);

    BALLOON_Balloon* bb;

    if (!WITHIN(BALLOON_balloon_upto, 1, BALLOON_MAX_BALLOONS - 1)) {
        return NULL;
    }

    ASSERT(WITHIN(type, 1, BALLOON_TYPE_NUMBER - 1));

    bb = &BALLOON_balloon[BALLOON_balloon_upto];

    bb->type = type;
    bb->next = NULL;
    bb->thing = thing;
    bb->yaw = 0;
    bb->pitch = 0;

    BALLOON_get_attached_point(thing, &ax, &ay, &az);

    for (i = 0; i < BALLOON_POINTS_PER_BALLOON; i++) {
        bb->bp[i].x = ax;
        bb->bp[i].y = ay;
        bb->bp[i].z = az;

        ay += BALLOON_POINT_DIST;
    }

    switch (p_thing->Class) {
    case CLASS_PERSON:
        bb->next = p_thing->Genus.Person->Balloon;
        p_thing->Genus.Person->Balloon = BALLOON_balloon_upto;
        break;

    default:
        break;
    }

    return BALLOON_balloon_upto++;
}

// uc_orig: BALLOON_process (fallen/Source/balloon.cpp)
void BALLOON_process()
{
    SLONG i;
    SLONG j;
    SLONG ax;
    SLONG ay;
    SLONG az;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG ddist;
    SLONG hyp;
    SLONG yaw;
    SLONG pitch;

    BALLOON_Balloon* bb;
    BALLOON_Point* bp;
    BALLOON_Point* bp1;
    BALLOON_Point* bp2;

// Collision radius for checking balloon tip against walls.
// uc_orig: BALLOON_RADIUS (fallen/Source/balloon.cpp)
#define BALLOON_RADIUS 0x30
// Impulse applied when balloon tip hits a wall.
// uc_orig: BALLOON_BOUNCE (fallen/Source/balloon.cpp)
#define BALLOON_BOUNCE 0x100

    for (i = 1; i < BALLOON_balloon_upto; i++) {
        bb = &BALLOON_balloon[i];

        if (bb->type == BALLOON_TYPE_UNUSED) {
            continue;
        }

        if (bb->thing) {
            // Point zero is fixed to the thing's left hand.
            BALLOON_get_attached_point(bb->thing, &ax, &ay, &az);

            bb->bp[0].x = ax;
            bb->bp[0].y = ay;
            bb->bp[0].z = az;

            for (j = 1; j < BALLOON_POINTS_PER_BALLOON; j++) {
                bp1 = &bb->bp[j - 1];
                bp2 = &bb->bp[j - 0];

                bp2->x += bp2->dx;
                bp2->y += bp2->dy;
                bp2->z += bp2->dz;

                dx = bp2->x - bp1->x;
                dy = bp2->y - bp1->y;
                dz = bp2->z - bp1->z;

                // Guard against overflows.
                if (dx >  0x7000) { dx =  0x7000; }
                if (dy >  0x7000) { dy =  0x7000; }
                if (dz >  0x7000) { dz =  0x7000; }
                if (dx < -0x7000) { dx = -0x7000; }
                if (dy < -0x7000) { dy = -0x7000; }
                if (dz < -0x7000) { dz = -0x7000; }

                dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

                if (dist > BALLOON_POINT_DIST) {
                    ddist = dist - BALLOON_POINT_DIST;

                    dx = (dx * ddist) / dist;
                    dy = (dy * ddist) / dist;
                    dz = (dz * ddist) / dist;

                    bp2->x -= dx;
                    bp2->y -= dy;
                    bp2->z -= dz;

                    bp2->dx -= dx / 0x10;
                    bp2->dy -= dy / 0x10;
                    bp2->dz -= dz / 0x10;
                }

                // Balloon rises.
                bp2->dy += 0x40;

                // Damping.
                bp2->dx -= bp2->dx / 0x10;
                bp2->dy -= bp2->dy / 0x10;
                bp2->dz -= bp2->dz / 0x10;
            }

            // Keep balloon tip out of walls.
            bp = &bb->bp[BALLOON_POINTS_PER_BALLOON - 1];

            SLONG px = bp->x >> 8;
            SLONG py = bp->y >> 8;
            SLONG pz = bp->z >> 8;

            if (MAV_inside(px + BALLOON_RADIUS, py, pz)) { bp->dx -= BALLOON_BOUNCE; }
            if (MAV_inside(px - BALLOON_RADIUS, py, pz)) { bp->dx += BALLOON_BOUNCE; }
            if (MAV_inside(px, py, pz + BALLOON_RADIUS)) { bp->dz -= BALLOON_BOUNCE; }
            if (MAV_inside(px, py, pz - BALLOON_RADIUS)) { bp->dz += BALLOON_BOUNCE; }
        } else {
            // Balloon is released: drift upward until too high, then destroy.

            bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dy += 0x40;

            bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dx -= bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dx / 0x10;
            bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dy -= bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dy / 0x10;
            bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dz -= bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dz / 0x10;

            bb->bp[BALLOON_POINTS_PER_BALLOON - 1].x += bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dx;
            bb->bp[BALLOON_POINTS_PER_BALLOON - 1].y += bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dy;
            bb->bp[BALLOON_POINTS_PER_BALLOON - 1].z += bb->bp[BALLOON_POINTS_PER_BALLOON - 1].dz;

            if (bb->bp[BALLOON_POINTS_PER_BALLOON - 1].y > 0x100000) {
                bb->type = BALLOON_TYPE_UNUSED;
            } else {
                // Pull the string after the balloon tip.
                for (j = BALLOON_POINTS_PER_BALLOON - 2; j >= 0; j--) {
                    bp1 = &bb->bp[j + 1];
                    bp2 = &bb->bp[j + 0];

                    bp2->x += bp2->dx;
                    bp2->y += bp2->dy;
                    bp2->z += bp2->dz;

                    dx = bp2->x - bp1->x;
                    dy = bp2->y - bp1->y;
                    dz = bp2->z - bp1->z;

                    if (dx >  0x4000) { dx =  0x4000; }
                    if (dy >  0x4000) { dy =  0x4000; }
                    if (dz >  0x4000) { dz =  0x4000; }
                    if (dx < -0x4000) { dx = -0x4000; }
                    if (dy < -0x4000) { dy = -0x4000; }
                    if (dz < -0x4000) { dz = -0x4000; }

                    dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

                    if (dist > BALLOON_POINT_DIST) {
                        ddist = dist - BALLOON_POINT_DIST;

                        dx = (dx * ddist) / dist;
                        dy = (dy * ddist) / dist;
                        dz = (dz * ddist) / dist;

                        bp2->x -= dx;
                        bp2->y -= dy;
                        bp2->z -= dz;

                        bp2->dx = bp1->dx;
                        bp2->dy = bp1->dy;
                        bp2->dz = bp1->dz;
                    }

                    // String falls.
                    bp2->dy -= 0x100;

                    bp2->dx -= bp2->dx / 0x10;
                    bp2->dy -= bp2->dy / 0x10;
                    bp2->dz -= bp2->dz / 0x10;
                }
            }
        }

        // Compute balloon orientation from the direction of the top two points.
        dx = bb->bp[BALLOON_POINTS_PER_BALLOON - 1].x - bb->bp[BALLOON_POINTS_PER_BALLOON - 2].x;
        dy = bb->bp[BALLOON_POINTS_PER_BALLOON - 1].y - bb->bp[BALLOON_POINTS_PER_BALLOON - 2].y;
        dz = bb->bp[BALLOON_POINTS_PER_BALLOON - 1].z - bb->bp[BALLOON_POINTS_PER_BALLOON - 2].z;

        hyp = QDIST2(abs(dx), abs(dz));

        yaw = calc_angle(dx, dz) + 1024;
        pitch = calc_angle(dy, hyp);
        pitch = -pitch;
        pitch += 512;
        pitch &= 2047;

        bb->yaw = yaw;
        bb->pitch = pitch;
    }
}

// uc_orig: BALLOON_release (fallen/Source/balloon.cpp)
void BALLOON_release(UBYTE balloon)
{
    ASSERT(WITHIN(balloon, 1, BALLOON_MAX_BALLOONS - 1));

    BALLOON_Balloon* bb;

    bb = &BALLOON_balloon[balloon];

    ASSERT(bb->type);

    Thing* p_thing = TO_THING(bb->thing);

    switch (p_thing->Class) {
    case CLASS_PERSON:
        p_thing->Genus.Person->Balloon = NULL;
        break;

    default:
        ASSERT(0);
        break;
    }

    while (balloon) {
        ASSERT(WITHIN(balloon, 1, BALLOON_balloon_upto - 1));

        bb = &BALLOON_balloon[balloon];

        bb->thing = NULL;
        balloon = bb->next;
    }
}

// uc_orig: BALLOON_find_grab (fallen/Source/balloon.cpp)
void BALLOON_find_grab(UWORD thing)
{
    SLONG i;
    SLONG ax;
    SLONG ay;
    SLONG az;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;

    Thing* p_thing = TO_THING(thing);

    BALLOON_Balloon* bb;

    if (BALLOON_balloon_upto == 1) {
        return;
    }

    // People only carry one balloon.
    if (p_thing->Genus.Person->Balloon) {
        return;
    }

    BALLOON_get_attached_point(thing, &ax, &ay, &az);

// Distance threshold for auto-grabbing a nearby released balloon.
// uc_orig: BALLOON_GRAB_DIST (fallen/Source/balloon.cpp)
#define BALLOON_GRAB_DIST (0x80)

    for (i = 1; i < BALLOON_balloon_upto; i++) {
        bb = &BALLOON_balloon[i];

        if (bb->type == BALLOON_TYPE_UNUSED || bb->thing != NULL) {
            continue;
        }

        dx = abs(bb->bp[0].x - ax);
        dy = abs(bb->bp[0].y - ay);
        dz = abs(bb->bp[0].z - az);

        dist = QDIST3(dx, dy, dz);

        if (dist < 0x10000) {
            bb->thing = thing;
            bb->next = p_thing->Genus.Person->Balloon;
            p_thing->Genus.Person->Balloon = i;

            return;
        }
    }
}
