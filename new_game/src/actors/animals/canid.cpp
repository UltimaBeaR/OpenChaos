// Canid (dog/wolf/coyote) AI for Urban Chaos.
// Each canid is a CLASS_ANIMAL entity with AnimalType ANIMAL_CANID.
// Uses the Animal struct for all per-instance state.
//
// Pre-release NOTE: CANID_fn_normal() has its substate dispatch commented out,
// so canids are visually inert in this codebase. The final game completed this system.

#include "missions/game_types.h"
#include "actors/animals/canid.h"
#include "actors/core/statedef.h"
#include "ai/mav.h"
#include "world/map/pap_globals.h"
#include "effects/dirt.h"              // DIRT_gust for footstep particles
#include "engine/physics/collide.h"    // there_is_a_los

// uc_orig: CANID_SUBSTATE_NONE (fallen/Source/canid.cpp)
#define CANID_SUBSTATE_NONE   0
// uc_orig: CANID_SUBSTATE_SLEEP (fallen/Source/canid.cpp)
#define CANID_SUBSTATE_SLEEP  1
// uc_orig: CANID_SUBSTATE_PROWL (fallen/Source/canid.cpp)
#define CANID_SUBSTATE_PROWL  2
// uc_orig: CANID_SUBSTATE_CHASE (fallen/Source/canid.cpp)
#define CANID_SUBSTATE_CHASE  3
// uc_orig: CANID_SUBSTATE_FLEE (fallen/Source/canid.cpp)
#define CANID_SUBSTATE_FLEE   4
// uc_orig: CANID_SUBSTATE_BARK (fallen/Source/canid.cpp)
#define CANID_SUBSTATE_BARK   5
// uc_orig: CANID_SUBSTATE_NUMBER (fallen/Source/canid.cpp)
#define CANID_SUBSTATE_NUMBER 6

// uc_orig: CANID_MAX_SENSE (fallen/Source/canid.cpp)
#define CANID_MAX_SENSE 32

// uc_orig: CANID_SENSE_RADIUS (fallen/Source/canid.cpp)
// Proximity sense radius in fine units (>>8 space).
#define CANID_SENSE_RADIUS (0xd0)

// Forward declarations of substate helpers.
// uc_orig: CANID_init_sleep (fallen/Source/canid.cpp)
static void CANID_init_sleep(Thing* canid);
// uc_orig: CANID_init_prowl (fallen/Source/canid.cpp)
static void CANID_init_prowl(Thing* canid);
// uc_orig: CANID_init_bark (fallen/Source/canid.cpp)
static void CANID_init_bark(Thing* canid, Thing* victim);
// uc_orig: CANID_init_chase (fallen/Source/canid.cpp)
static void CANID_init_chase(Thing* canid, Thing* victim);
// uc_orig: CANID_start_doing_something (fallen/Source/canid.cpp)
static void CANID_start_doing_something(Thing* canid);
// uc_orig: CANID_6sense (fallen/Source/canid.cpp)
static int CANID_6sense(Thing* canid);
// uc_orig: CANID_can_see (fallen/Source/canid.cpp)
static int CANID_can_see(Thing* canid, Thing* target);
// uc_orig: CANID_LOS (fallen/Source/canid.cpp)
static int CANID_LOS(Thing* canid);
// uc_orig: CANID_Homing (fallen/Source/canid.cpp)
static void CANID_Homing(Thing* canid, SLONG dest_x, SLONG dest_z, int wibble);
// uc_orig: CANID_process_sleep (fallen/Source/canid.cpp)
static void CANID_process_sleep(Thing* canid);
// uc_orig: CANID_process_bark (fallen/Source/canid.cpp)
static void CANID_process_bark(Thing* canid);
// uc_orig: CANID_process_prowl (fallen/Source/canid.cpp)
static void CANID_process_prowl(Thing* canid);
// uc_orig: CANID_process_chase (fallen/Source/canid.cpp)
static void CANID_process_chase(Thing* canid);

// uc_orig: CANID_register (fallen/Source/canid.cpp)
// Stub — all ANIMAL_register calls are commented out in the pre-release version.
void CANID_register()
{
    // All registration calls were commented out in pre-release.
    // The dog mesh system (doghead.all, dogfleg.all etc.) was incomplete.
}

// uc_orig: CANID_Homing (fallen/Source/canid.cpp)
// Moves the canid one step toward (dest_x, dest_z) each frame.
// Smoothly turns using angular damping (>>3). Speed 4 = run, 6 = walk (higher = slower).
static void CANID_Homing(Thing* canid, SLONG dest_x, SLONG dest_z, int wibble)
{
    SLONG dx, dz;
    Animal* animal = ANIMAL_get_animal(canid);
    DrawTween* dt = canid->Draw.Tweened;
    GameCoord new_pos;
    SLONG angle, dangle, i;
    UBYTE spd;

    dx = dest_x - canid->WorldPos.X;
    dz = dest_z - canid->WorldPos.Z;

    dx >>= 8;
    dz >>= 8;

    if (dx * dx + dz * dz > 48000) {
        if (animal->extra != 4)
            ANIMAL_set_anim(canid, 2);
        animal->extra = spd = 4;
    } else {
        if (animal->extra != 6)
            ANIMAL_set_anim(canid, 1);
        animal->extra = spd = 6;
    }

    angle = Arctan(-dx, dz);
    angle &= 2047;

    dangle = angle - dt->Angle;

    if (dangle > 1024)
        dangle -= 2048;
    if (dangle < -1024)
        dangle += 2048;

    dangle >>= 3;

    dt->Angle += dangle;
    dt->Angle &= 2047;
    dx = SIN(dt->Angle) >> spd;
    dz = COS(dt->Angle) >> spd;

    new_pos.X = canid->WorldPos.X - dx;
    new_pos.Y = 14000 + (PAP_calc_height_at(canid->WorldPos.X >> 8, canid->WorldPos.Z >> 8) << 8);
    new_pos.Z = canid->WorldPos.Z - dz;

    DIRT_gust(canid,
        canid->WorldPos.X >> 8, canid->WorldPos.Z >> 8,
        new_pos.X >> 8, new_pos.Z >> 8);

    ANIMAL_animate(canid);

    move_thing_on_map(canid, &new_pos);
}

// uc_orig: CANID_6sense (fallen/Source/canid.cpp)
// Short-range omnidirectional sense — no LOS required.
// Returns 1 and initiates chase if a person is found within CANID_SENSE_RADIUS.
static int CANID_6sense(Thing* canid)
{
    THING_INDEX sense[CANID_MAX_SENSE];
    SLONG sense_upto;
    Thing* p_sense;
    Animal* sense_animal;
    int i;

    sense_upto = THING_find_sphere(
        canid->WorldPos.X >> 8,
        canid->WorldPos.Y >> 8,
        canid->WorldPos.Z >> 8,
        CANID_SENSE_RADIUS,
        sense,
        CANID_MAX_SENSE,
        THING_FIND_MOVING);

    for (i = 0; i < sense_upto; i++) {
        p_sense = TO_THING(sense[i]);

        switch (p_sense->Class) {
        case CLASS_ANIMAL:
            sense_animal = ANIMAL_get_animal(p_sense);
            break;

        case CLASS_PERSON:
            CANID_init_chase(canid, p_sense);
            return 1;
            break;

        default:
            ASSERT(0);
            break;
        }
    }
    return 0;
}

// uc_orig: CANID_can_see (fallen/Source/canid.cpp)
// Returns 1 if target is within ±45 degree forward FOV and line-of-sight is clear.
static int CANID_can_see(Thing* canid, Thing* target)
{
    SLONG angle, dx, dz;
    DrawTween* dt = canid->Draw.Tweened;

    dx = target->WorldPos.X - canid->WorldPos.X;
    dz = target->WorldPos.Z - canid->WorldPos.Z;

    angle = Arctan(-dx, dz);

    if (angle < 0)
        angle = 2048 + angle;
    else
        angle = angle & 2047;

    angle -= (dt->Angle);

    if (angle < 256 || angle > 2048 - 256) {
        if (there_is_a_los(canid->WorldPos.X >> 8, canid->WorldPos.Y >> 8, canid->WorldPos.Z >> 8, target->WorldPos.X >> 8, target->WorldPos.Y >> 8, target->WorldPos.Z >> 8, 0)) {
            return (1);
        }
    }
    return (0);
}

// uc_orig: CANID_LOS (fallen/Source/canid.cpp)
// Long-range visual detection at 5× sense radius. Uses CANID_can_see() for FOV+LOS check.
static int CANID_LOS(Thing* canid)
{
    THING_INDEX sense[CANID_MAX_SENSE];
    SLONG sense_upto;
    Thing* p_sense;
    Animal* sense_animal;
    int i;

    sense_upto = THING_find_sphere(
        canid->WorldPos.X >> 8,
        canid->WorldPos.Y >> 8,
        canid->WorldPos.Z >> 8,
        CANID_SENSE_RADIUS * 5,
        sense,
        CANID_MAX_SENSE,
        THING_FIND_MOVING);

    for (i = 0; i < sense_upto; i++) {
        p_sense = TO_THING(sense[i]);

        switch (p_sense->Class) {
        case CLASS_ANIMAL:
            sense_animal = ANIMAL_get_animal(p_sense);
            break;

        case CLASS_PERSON:
            if (CANID_can_see(canid, p_sense))
                CANID_init_chase(canid, p_sense);
            return 1;
            break;

        default:
            ASSERT(0);
            break;
        }
    }
    return 0;
}

// uc_orig: CANID_init_sleep (fallen/Source/canid.cpp)
static void CANID_init_sleep(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);

    animal->counter = Random() & 0xff;
    animal->counter += 0x10;
    animal->substate = CANID_SUBSTATE_SLEEP;
}

// uc_orig: CANID_init_bark (fallen/Source/canid.cpp)
static void CANID_init_bark(Thing* canid, Thing* victim)
{
    Animal* animal = ANIMAL_get_animal(canid);

    animal->target = victim;
    animal->substate = CANID_SUBSTATE_BARK;
}

// uc_orig: CANID_init_prowl (fallen/Source/canid.cpp)
// Picks a random cardinal offset (+/-4 squares) from home, uses MAV_do to find next nav step.
static void CANID_init_prowl(Thing* canid)
{
    int i, j;
    MAV_Action orders;
    Animal* animal = ANIMAL_get_animal(canid);

    switch (Random() & 0x3) {
    case 0:
        i = 0;
        j = 4;
        break;
    case 1:
        i = 4;
        j = 0;
        break;
    case 2:
        i = 0;
        j = -4;
        break;
    case 3:
        i = -4;
        j = 0;
        break;
    }

    animal->substate = CANID_SUBSTATE_PROWL;
    animal->dest_x = animal->home_x + i;
    animal->dest_z = animal->home_z + j;

    animal->map_x = canid->WorldPos.X >> 16;
    animal->map_z = canid->WorldPos.Z >> 16;

    orders = MAV_do(animal->map_x, animal->map_z, animal->dest_x, animal->dest_z, MAV_CAPS_GOTO);
    animal->dest_x = orders.dest_x;
    animal->dest_z = orders.dest_z;
    animal->counter = 0;
}

// uc_orig: CANID_init_chase (fallen/Source/canid.cpp)
// Sets target = victim, computes first MAV navigation step toward it.
static void CANID_init_chase(Thing* canid, Thing* victim)
{
    int i, j;
    MAV_Action orders;

    Animal* animal = ANIMAL_get_animal(canid);

    animal->substate = CANID_SUBSTATE_CHASE;
    animal->dest_x = victim->WorldPos.X >> 16;
    animal->dest_z = victim->WorldPos.Z >> 16;
    animal->target = victim;

    animal->map_x = canid->WorldPos.X >> 16;
    animal->map_z = canid->WorldPos.Z >> 16;

    orders = MAV_do(animal->map_x, animal->map_z, animal->dest_x, animal->dest_z, MAV_CAPS_GOTO);
    animal->dest_x = orders.dest_x;
    animal->dest_z = orders.dest_z;
}

// uc_orig: CANID_start_doing_something (fallen/Source/canid.cpp)
// Randomly picks the next behaviour: 75% prowl, 25% sleep.
static void CANID_start_doing_something(Thing* canid)
{
    switch (Random() & 0x3) {
    case 0:
    case 1:
    case 2:
        CANID_init_prowl(canid);
        break;
    case 3:
        CANID_init_sleep(canid);
        break;
    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: CANID_process_sleep (fallen/Source/canid.cpp)
static void CANID_process_sleep(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);

    if (CANID_6sense(canid))
        return;
    if (animal->counter == 0) {
        CANID_start_doing_something(canid);
    } else {
        animal->counter -= 1;
    }
}

// uc_orig: CANID_process_bark (fallen/Source/canid.cpp)
// If target moves far away (dx^2+dz^2 > 14000) resume chase.
static void CANID_process_bark(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);
    SLONG dx, dz;

    dx = animal->target->WorldPos.X - canid->WorldPos.X;
    dz = animal->target->WorldPos.Z - canid->WorldPos.Z;

    dx >>= 8;
    dz >>= 8;

    if (dx * dx + dz * dz > 14000) {
        CANID_init_chase(canid, animal->target);
    }
}

// uc_orig: CANID_process_prowl (fallen/Source/canid.cpp)
static void CANID_process_prowl(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);

    if (!CANID_6sense(canid))
        CANID_LOS(canid);

    animal->map_x = canid->WorldPos.X >> 16;
    animal->map_z = canid->WorldPos.Z >> 16;

    if ((animal->map_x == animal->dest_x) && (animal->map_z == animal->dest_z))
        CANID_start_doing_something(canid);

    ANIMAL_animate(canid);

    CANID_Homing(canid, (animal->dest_x << 16) + (128 << 8), (animal->dest_z << 16) + (128 << 8), 1);
}

// uc_orig: CANID_process_chase (fallen/Source/canid.cpp)
static void CANID_process_chase(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);
    SLONG dx, dz;

    if (!CANID_6sense(canid))
        CANID_LOS(canid);

    animal->map_x = canid->WorldPos.X >> 16;
    animal->map_z = canid->WorldPos.Z >> 16;

    dx = animal->target->WorldPos.X - canid->WorldPos.X;
    dz = animal->target->WorldPos.Z - canid->WorldPos.Z;

    dx >>= 8;
    dz >>= 8;

    if (dx * dx + dz * dz < 4096) {
        CANID_init_bark(canid, animal->target);
        return;
    }

    CANID_Homing(canid, (animal->dest_x << 16) + (128 << 8), (animal->dest_z << 16) + (128 << 8), 0);

    if ((animal->map_x == animal->dest_x) && (animal->map_z == animal->dest_z))
        CANID_init_prowl(canid);
}

// uc_orig: CANID_fn_init (fallen/Source/canid.cpp)
// STATE_INIT handler: records spawn position as home, starts first behaviour.
void CANID_fn_init(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);

    animal->home_x = canid->WorldPos.X >> 16;
    animal->home_z = canid->WorldPos.Z >> 16;

    set_state_function(canid, STATE_NORMAL);

    CANID_start_doing_something(canid);
}

// uc_orig: CANID_fn_normal (fallen/Source/canid.cpp)
// Per-frame STATE_NORMAL handler.
// NOTE: The substate dispatch switch is commented out in the pre-release version.
// Canids are completely inert at runtime in this codebase.
void CANID_fn_normal(Thing* canid)
{
    SLONG i;

    Animal* animal = ANIMAL_get_animal(canid);
    /*
            SLONG px = canid->WorldPos.X >> 8;
            SLONG py = canid->WorldPos.Y >> 8;
            py += 0x20;
            SLONG pz = canid->WorldPos.Z >> 8;
        DIRT_new_water(px, py, pz,     -1, 28,  0);
    /

            switch(animal->substate)
            {
                    case CANID_SUBSTATE_NONE:                                    break;
                    case CANID_SUBSTATE_SLEEP:  CANID_process_sleep(canid);     break;
                    case CANID_SUBSTATE_BARK :  CANID_process_bark (canid);     break;
                    case CANID_SUBSTATE_PROWL:  CANID_process_prowl(canid);     break;
                    case CANID_SUBSTATE_CHASE:  CANID_process_chase(canid);     break;

                    default:
                            ASSERT(0);
                            break;
            }
    */
}

// uc_orig: CANID_init (fallen/Headers/canid.h)
void CANID_init(Thing* canid)
{
    CANID_fn_init(canid);
}

// uc_orig: CANID_normal (fallen/Headers/canid.h)
void CANID_normal(Thing* canid)
{
    CANID_fn_normal(canid);
}
