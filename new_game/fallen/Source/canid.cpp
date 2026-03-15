//
// Canids -- 4 legs, pointy teeth, tails, meat eaters, usually pack hunters,
//           non-retractable claws, and other things we've come to expect from
//           man's best friends and worst enemies: dogs, coyotes, wolves, that
//           sort of thing.
//

// claude-ai: ============================================================
// claude-ai: FILE OVERVIEW: canid.cpp
// claude-ai: ============================================================
// claude-ai: Implements the dog/canine AI for Urban Chaos. These are
// claude-ai: CLASS_ANIMAL entities of AnimalType ANIMAL_CANID. In the
// claude-ai: final game, canids appear primarily as security/guard dogs.
// claude-ai:
// claude-ai: STATUS NOTE: This file is heavily unfinished in this
// claude-ai: pre-release codebase. The main per-frame dispatch function
// claude-ai: CANID_fn_normal() has its entire switch statement commented
// claude-ai: out (lines ~973-985). Canids exist in the Thing list but
// claude-ai: do nothing each frame. A large "CODE GRAVEYARD" section
// claude-ai: (line ~992+) contains additional commented-out code for
// claude-ai: flee/fly/perch behaviors that were partially designed but
// claude-ai: never integrated. The final PS1/PC game likely finished
// claude-ai: this system after this codebase snapshot.
// claude-ai:
// claude-ai: ARCHITECTURE:
// claude-ai:   - Uses the ANIMAL subsystem: Animal struct holds all
// claude-ai:     canid-specific state (substate, target, home, dest, counter)
// claude-ai:   - ANIMAL_get_animal(thing) retrieves the Animal struct
// claude-ai:   - DrawTween used for rendering (canid->Draw.Tweened)
// claude-ai:   - Entirely excluded from Dreamcast build (#ifndef TARGET_DC)
// claude-ai:
// claude-ai: STATE MACHINE (via Animal::substate):
// claude-ai:   CANID_SUBSTATE_NONE  (0) — uninitialised / do nothing
// claude-ai:   CANID_SUBSTATE_SLEEP (1) — idle wait; counts down Animal::counter
// claude-ai:                              then calls CANID_start_doing_something()
// claude-ai:                              counter = Random()&0xff + 0x10
// claude-ai:   CANID_SUBSTATE_PROWL (2) — walks to a random offset from home
// claude-ai:                              (+/-4 map squares); uses MAV_do()
// claude-ai:                              pathfinding with MAV_CAPS_GOTO only
// claude-ai:                              (no jumping/falling). Calls 6sense+LOS
// claude-ai:                              each frame to look for targets.
// claude-ai:   CANID_SUBSTATE_CHASE (3) — chases target via MAV_do pathfinding;
// claude-ai:                              transitions to BARK when close (<64 units
// claude-ai:                              map-space, i.e. dx*dx+dz*dz < 4096).
// claude-ai:   CANID_SUBSTATE_FLEE  (4) — (defined but never implemented in normal)
// claude-ai:   CANID_SUBSTATE_BARK  (5) — face target and bark; transitions back
// claude-ai:                              to CHASE if target moves away (>14000)
// claude-ai:
// claude-ai: SENSING / DETECTION:
// claude-ai:   CANID_6sense() — proximity sense: THING_find_sphere radius 0xD0,
// claude-ai:                    CLASS_PERSON only; immediately initiates chase.
// claude-ai:   CANID_LOS()    — line-of-sight sense: larger radius (0xD0*5),
// claude-ai:                    uses there_is_a_los() + Arctan angle check
// claude-ai:                    (FOV cone ±256/2048 = ±45 degrees).
// claude-ai:   CANID_can_see()— helper: checks if target is in FOV cone and
// claude-ai:                    there_is_a_los() returns clear path.
// claude-ai:   Prowl calls both 6sense AND LOS each frame; Sleep only calls 6sense.
// claude-ai:
// claude-ai: MOVEMENT — CANID_Homing(canid, dest_x, dest_z, wibble):
// claude-ai:   Steers toward destination using Arctan + angular damping (>>3).
// claude-ai:   Speed: 4 (run) when dist^2 > 48000, 6 (walk) when closer.
// claude-ai:   Animation: anim 2 = run, anim 1 = walk (via ANIMAL_set_anim).
// claude-ai:   Y position: PAP_calc_height_at() * 256 + 14000 (ground snap).
// claude-ai:   Calls DIRT_gust() for footstep particle effects.
// claude-ai:   dest coords passed as >>16 WorldPos (mapsquare units) shifted
// claude-ai:   back to WorldPos by: (dest_x<<16)+(128<<8).
// claude-ai:
// claude-ai: INITIALISATION:
// claude-ai:   CANID_fn_init() — called at STATE_INIT; records home_x/home_z
// claude-ai:   from initial WorldPos>>16, then calls CANID_start_doing_something()
// claude-ai:   which randomly selects PROWL (75%) or SLEEP (25%).
// claude-ai:
// claude-ai: COORDINATE NOTES:
// claude-ai:   WorldPos is 32-bit fixed-point; >>16 = mapsquare, >>8 = fine unit.
// claude-ai:   CANID_SENSE_RADIUS = 0xD0 in fine units (same scale as >>8 coords).
// claude-ai: ============================================================


#include "game.h"
#include "canid.h"
#include "statedef.h"
#include "mav.h"
#include "pap.h"

// just for debug
#include "dirt.h"

//--------------------------------------------------------------------------
// Defines

//
// Canid substates.
//

#define CANID_SUBSTATE_NONE 0
#define CANID_SUBSTATE_SLEEP 1 // zzzz
#define CANID_SUBSTATE_PROWL 2 // sniffsniffsnufflewagwagsniffbarksniffsniff
#define CANID_SUBSTATE_CHASE 3 // Fetch! Kill!
#define CANID_SUBSTATE_FLEE 4 // Run for the hills
#define CANID_SUBSTATE_BARK 5 // Woof Woof
#define CANID_SUBSTATE_NUMBER 6

#define CANID_MAX_SENSE 32

//--------------------------------------------------------------------------
// Prototypes

//
// The functions called when a canid is in each State.
//

void CANID_fn_init(Thing*);
void CANID_fn_normal(Thing*);

//
// Substate initialisation
//

void CANID_init_sleep(Thing* canid);
void CANID_init_prowl(Thing* canid);
void CANID_init_bark(Thing* canid, Thing* victim);
void CANID_init_chase(Thing* canid, Thing* victim);
// void CANID_init_flee (Thing *canid, Thing *from);

//--------------------------------------------------------------------------
// Globals

StateFunction CANID_state_function[] = {
    { STATE_INIT, CANID_fn_init },
    { STATE_NORMAL, CANID_fn_normal }
};

//--------------------------------------------------------------------------
// Functions

//
//  Register canids with the animal system
//

// claude-ai: CANID_register() — stub; all ANIMAL_register calls are commented out.
// claude-ai: Dogs were apparently going to use a multi-part .all mesh system
// claude-ai: (doghead.all, dogfleg.all, etc.) but this was never completed.
void CANID_register()
{
    //	ANIMAL_register("data\\dog2.all",0,0,0);

    // ANIMAL_register("data\\dog2.all");//tumtetum

    /*	ANIMAL_register("data\\doghead.all", 0,0,0);
            ANIMAL_register("data\\dogfleg.all", 0,0,0);
            ANIMAL_register("data\\dogbelly.all", 0,0,0);
            ANIMAL_register("data\\doghleg.all", 0,0,0);
            ANIMAL_register("data\\dogtail.all", 0,0,0);
            */
}

//
//  straight-home towards a target
//

// claude-ai: CANID_Homing() — per-frame movement toward (dest_x, dest_z) in WorldPos units.
// claude-ai: Implements smooth turning: angular error >> 3 per frame.
// claude-ai: Speed selection: spd=4 (fast/run anim) if far, spd=6 (slow/walk) if near.
// claude-ai: Note: higher spd value = SLOWER (SIN(angle)>>spd divides velocity).
// claude-ai: wibble parameter is accepted but the wobble code is commented out.
void CANID_Homing(Thing* canid, SLONG dest_x, SLONG dest_z, int wibble)
{
    SLONG dx, dz;
    Animal* animal = ANIMAL_get_animal(canid);
    //	DrawTween *dt = ANIMAL_get_drawtween(animal);
    DrawTween* dt = canid->Draw.Tweened;
    GameCoord new_pos;
    SLONG angle, dangle, i;
    UBYTE spd;

    /*
                    SLONG px = dest_x>>8;
                    SLONG py = canid->WorldPos.Y >> 8;
                    py += 0x10;
                    SLONG pz = dest_z>>8;
        DIRT_new_water(px, py, pz,     -1, 28,  0);
    */

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

    //	if (wibble) angle+=(SIN((animal->counter+=100))/1024);
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

    /*	for (i=1;i<ANIMAL_body_size(animal);i++) {
                    dt->Angle+=dangle;
                    dt->Angle&=2047;
                    dt++;
            }*/

    /*

                    dangle >>= 3;

                    SATURATE(dangle, -10, +10);

                    chopper->ry=dangle;

                    if (chopper->dist>(512*512)) chopper->counter=128;

            }
            if (chopper->counter) chopper->counter--;

            dm->Angle += chopper->ry;
            dm->Angle &= 2047;
            chopper->ry*=0.8;
    */

    new_pos.X = canid->WorldPos.X - dx;
    //	new_pos.Y = canid->WorldPos.Y;
    //	new_pos.Y = (66+PAP_calc_height_at(canid->WorldPos.X>>8,canid->WorldPos.Z>>8))<<8;
    new_pos.Y = 14000 + (PAP_calc_height_at(canid->WorldPos.X >> 8, canid->WorldPos.Z >> 8) << 8);
    new_pos.Z = canid->WorldPos.Z - dz;

    DIRT_gust(canid,
        canid->WorldPos.X >> 8, canid->WorldPos.Z >> 8,
        new_pos.X >> 8, new_pos.Z >> 8);

    ANIMAL_animate(canid);

    move_thing_on_map(canid, &new_pos);
}

// claude-ai: CANID_6sense() — proximity "smell/hearing" sense, no LOS required.
// claude-ai: Scans THING_FIND_MOVING entities in radius 0xD0.
// claude-ai: Only reacts to CLASS_PERSON (initiates chase) and CLASS_ANIMAL.
// claude-ai: Returns 1 if a person was detected, 0 otherwise.
int CANID_6sense(Thing* canid)
{

#define CANID_SENSE_RADIUS (0xd0)

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
            // add reactions here ....?
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

// claude-ai: CANID_can_see() — checks if target is within forward FOV cone (±45 degrees)
// claude-ai: AND there is a clear line of sight (no buildings). Uses DrawTween->Angle
// claude-ai: as the canid's facing direction. Angle range: 0-2047 (full circle).
int CANID_can_see(Thing* canid, Thing* target)
{
    SLONG angle, dx, dz;
    //	DrawTween *dt = ANIMAL_get_drawtween(ANIMAL_get_animal(canid)); // returns head -- perfect
    DrawTween* dt = canid->Draw.Tweened;

    TRACE("ping... \n");
    dx = target->WorldPos.X - canid->WorldPos.X;
    dz = target->WorldPos.Z - canid->WorldPos.Z;

    angle = Arctan(-dx, dz);

    if (angle < 0)
        angle = 2048 + angle;
    else
        angle = angle & 2047;

    angle -= (dt->Angle);

    if (angle < 256 || angle > 2048 - 256) {
        //
        // angle is valid
        //
        if (there_is_a_los(canid->WorldPos.X >> 8, canid->WorldPos.Y >> 8, canid->WorldPos.Z >> 8, target->WorldPos.X >> 8, target->WorldPos.Y >> 8, target->WorldPos.Z >> 8, 0)) {
            //
            // no building in the way
            //
            TRACE("you're seen\n");
            return (1);
        } else
            TRACE("come out from behind that building ya coward\n");
    }
    TRACE("klang\n");
    return (0);
}

// claude-ai: CANID_LOS() — visual detection scan at radius 0xD0*5 (5x larger than 6sense).
// claude-ai: For each CLASS_PERSON found, calls CANID_can_see() before chasing.
// claude-ai: This represents the dog "spotting" something at longer range visually.
int CANID_LOS(Thing* canid)
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
            // add reactions here ....?
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

// ========================================================
//
// Initialising the various substates.
//
// ========================================================

void CANID_init_sleep(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);

    TRACE("canid: sleep mode\n");

    //
    // Use the counter for how long we should sleep.
    //

    animal->counter = Random() & 0xff;
    animal->counter += 0x10;
    animal->substate = CANID_SUBSTATE_SLEEP;
}

void CANID_init_bark(Thing* canid, Thing* victim)
{
    Animal* animal = ANIMAL_get_animal(canid);

    TRACE("canid: bark mode\n");

    animal->target = victim;
    animal->substate = CANID_SUBSTATE_BARK;
}

// claude-ai: CANID_init_prowl() — picks a random cardinal direction offset (+/-4 squares)
// claude-ai: from home, runs MAV_do() to get the next navigable step toward it.
// claude-ai: Stores result in animal->dest_x/dest_z. Resets counter=0.
void CANID_init_prowl(Thing* canid)
{
    int i, j;
    MAV_Action orders;
    Animal* animal = ANIMAL_get_animal(canid);

    TRACE("canid: prowl mode\n");

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

    //	orders=MAV_do(animal->map_x,animal->map_z,animal->dest_x,animal->dest_z,MAV_CAPS_GOTO|MAV_CAPS_JUMP|MAV_CAPS_FALL_OFF);
    orders = MAV_do(animal->map_x, animal->map_z, animal->dest_x, animal->dest_z, MAV_CAPS_GOTO);
    animal->dest_x = orders.dest_x;
    animal->dest_z = orders.dest_z;
    animal->counter = 0;
}

// claude-ai: CANID_init_chase() — sets target = victim, calculates MAV next step
// claude-ai: toward victim's mapsquare. Animal::target stores a raw Thing pointer
// claude-ai: (no refcounting — dangling pointer risk if victim dies!).
void CANID_init_chase(Thing* canid, Thing* victim)
{
    int i, j;
    MAV_Action orders;

    TRACE("canid: chase mode\n");

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

//
// Decides what the canid should do next.
//

void CANID_start_doing_something(Thing* canid)
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

// ========================================================
//
// The substate processing functions.
//
// ========================================================

void CANID_process_sleep(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);

    if (CANID_6sense(canid))
        return;
    if (animal->counter == 0) {
        //
        // Don't sleep any longer... do something else.
        //

        CANID_start_doing_something(canid);
    } else {
        animal->counter -= 1;
    }
}

void CANID_process_bark(Thing* canid)
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

void CANID_process_prowl(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);
    SLONG dx, dz;

    if (!CANID_6sense(canid))
        CANID_LOS(canid);

    animal->map_x = canid->WorldPos.X >> 16;
    animal->map_z = canid->WorldPos.Z >> 16;

    if ((animal->map_x == animal->dest_x) && (animal->map_z == animal->dest_z))
        CANID_start_doing_something(canid);

    ANIMAL_animate(canid);
    /*
            dx=animal->dest_x<<16-canid->WorldPos.X;
            dz=animal->dest_z<<16-canid->WorldPos.Z;

            dx>>=8;
            dz>>=8;

            if (dx*dx+dz*dz<=256*1*256*1) CANID_init_prowl(canid);
    */

    CANID_Homing(canid, (animal->dest_x << 16) + (128 << 8), (animal->dest_z << 16) + (128 << 8), 1);
}

void CANID_process_chase(Thing* canid)
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

/*
void CANID_process_peck(Thing *canid)
{
        Animal *animal = ANIMAL_get_animal(canid);

        animal->counter -= 1;

        //
        // Have a bit of standing around at the end of pecking.
        //

        #define CANID_PECK_SLEEP 8

        if (animal->counter >= CANID_PECK_SLEEP)
        {
                if (animal->counter & 0x1)
                {
                        canid->WorldPos.Y += 0x10<<8;
                }
                else
                {
                        canid->WorldPos.Y -= 0x10<<8;
                }
        }

        if (animal->counter == 0)
        {
                //
                // Enough pecking... do something else now.
                //

                CANID_start_doing_something(canid);
        }
}

void CANID_process_walk(Thing *canid)
{
        SLONG dx;
        SLONG dz;

        Animal   *animal = ANIMAL_get_animal  (canid);
        DrawMesh *dm     = ANIMAL_get_drawmesh(canid);

        //
        // Have we arrived yet?
        //

        animal->counter -= 1;

        if (animal->counter == 0)
        {
                //
                // We have arrived. What do we do now?
                //

                CANID_start_doing_something(canid);
        }
        else
        {
                //
                // Move the canid.
                //

                GameCoord new_pos = canid->WorldPos;

                dx = SIN(dm->Angle) >> 6;
                dz = COS(dm->Angle) >> 6;

                new_pos.X -= dx;
                new_pos.Z -= dz;

                dx  *= TICK_RATIO;
                dz  *= TICK_RATIO;

                dx >>= TICK_SHIFT;
                dz >>= TICK_SHIFT;

                move_thing_on_map(canid, &new_pos);
        }
}

void CANID_process_flee(Thing *canid)
{
        SLONG dx;
        SLONG dz;

        Animal   *animal = ANIMAL_get_animal  (canid);
        DrawMesh *dm     = ANIMAL_get_drawmesh(canid);

        //
        // Time to stop fleeing?
        //

        animal->counter -= 1;

        if (animal->counter == 0)
        {
                CANID_start_doing_something(canid);
        }
        else
        {
                //
                // Move the canid.
                //

                GameCoord new_pos = canid->WorldPos;

                if (animal->counter >= CANID_FLEE_TIME / 2)
                {
                        dx = SIN(dm->Angle) >> 4;
                        dz = COS(dm->Angle) >> 4;
                }
                else
                {
                        dx = SIN(dm->Angle) >> 5;
                        dz = COS(dm->Angle) >> 5;
                }

                dx  *= TICK_RATIO;
                dz  *= TICK_RATIO;

                dx >>= TICK_SHIFT;
                dz >>= TICK_SHIFT;

                new_pos.X -= dx;
                new_pos.Z -= dz;

                move_thing_on_map(canid, &new_pos);
        }
}

void CANID_process_fly(Thing *canid)
{
        SLONG dx;
        SLONG dy;
        SLONG dz;

        SLONG yangle;
        SLONG angle;
        SLONG dangle;
        SLONG speed;
        SLONG dist;

        SLONG dest_x;
        SLONG dest_y;
        SLONG dest_z;

        SLONG vel_x;
        SLONG vel_z;

        GameCoord new_pos;

        Animal *animal = ANIMAL_get_animal(canid);
        DrawMesh *dm   = ANIMAL_get_drawmesh(canid);

        //
        // Where are we flying to?
        //

        CANID_find_pos_along_vect(
                animal->other_index,
                animal->along,
           &dest_x,
           &dest_y,
           &dest_z);

        e_draw_3d_line(
                canid->WorldPos.X >> 8,
                canid->WorldPos.Y >> 8,
                canid->WorldPos.Z >> 8,
                dest_x,
                dest_y,
                dest_z);

        //
        // Turn the canid towards its destination.
        //

        dx = dest_x - (canid->WorldPos.X >> 8);
        dy = dest_y - (canid->WorldPos.Y >> 8);
        dz = dest_z - (canid->WorldPos.Z >> 8);

        dist = QDIST2(abs(dx),abs(dz));

        angle  = -Arctan(dx, dz);
        angle &=  2047;

        dangle = angle - dm->Angle;

        if (dangle >  1024) {dangle -= 2048;}
        if (dangle < -1024) {dangle += 2048;}

        dangle >>= 3;

        SATURATE(dangle, -100, +100);

        dm->Angle += dangle;
        dm->Angle &= 2047;

        //
        // How fast is the canid moving.
        //

        #define CANID_FLY_ACCEL_SPEED 15
        #define CANID_FLY_DECEL_SPEED 32

        #define CANID_FLY_ACCEL_TIME 8
        #define CANID_FLY_DECEL_TIME 16

        if (animal->counter < CANID_FLY_ACCEL_TIME)
        {
                speed = CANID_FLY_ACCEL_SPEED;
                animal->counter += 1;
        }
        else
        {
                speed = CANID_FLY_DECEL_SPEED;
        }

        //
        // Are we too close for our dangle?
        //

        if (abs(dangle) > 10)
        {
                if (dist < 256) {speed >>= 1;}
                if (dist < 128) {speed >>= 1;}
        }

        if (dist < 90) {speed >>= 1;}
        if (dist < 50) {speed >>= 1;}

        //
        // Find the new canid position in x,z
        //

        vel_x = SIN(dm->Angle) * speed >> 8;
        vel_z = COS(dm->Angle) * speed >> 8;

        vel_x  *= TICK_RATIO;
        vel_z  *= TICK_RATIO;

        vel_x >>= TICK_SHIFT;
        vel_z >>= TICK_SHIFT;

        new_pos.X = canid->WorldPos.X - vel_x;
        new_pos.Z = canid->WorldPos.Z - vel_z;

        //
        // Find the new canid position in y.
        //

        yangle = 512 - (dist * 512) / animal->dist;

        SATURATE(yangle, 0, 512);

        new_pos.Y   = animal->starty;
        new_pos.Y  += (dest_y - animal->starty) * SIN(yangle) >> 16;
        new_pos.Y <<= 8;

        //
        // Move the canid.
        //

        move_thing_on_map(canid, &new_pos);

        if (dist < CANID_FLY_DECEL_SPEED)
        {
                //
                // Start perching.
                //

                CANID_init_perch(canid);
        }
}

void CANID_process_perch(Thing *canid)
{
        UBYTE doing;
        UBYTE howlong;
        UBYTE oldalong;

        Animal *animal = ANIMAL_get_animal(canid);

        doing    = animal->counter >> 6;
        howlong  = animal->counter & 0x3f;

        if (howlong == 0)
        {
                switch(Random() & 0x3)
                {
                        case 0:
                                howlong = 32 + (Random() & 31);
                                doing   = CANID_PERCH_WAIT;
                                break;

                        case 1:
                                howlong = 32 + (Random() & 31);
                                doing   = CANID_PERCH_SHUFFLE_LEFT;
                                break;

                        case 2:
                                howlong = 32 + (Random() & 31);
                                doing   = CANID_PERCH_SHUFFLE_RIGHT;
                                break;

                        case 3:

                                //
                                // It is too scary to return to where we were
                                // (animal->map_x, animal->map_z)?
                                //

                                CANID_init_land(canid);
                                return;
                }
        }
        else
        {
                howlong -= 1;
        }

        switch(doing)
        {
                case CANID_PERCH_WAIT:
                        break;

                case CANID_PERCH_SHUFFLE_LEFT:

                        if (animal->along == 0)
                        {
                                doing = CANID_PERCH_SHUFFLE_RIGHT;
                        }
                        else
                        {
                                animal->along -= 1;
                        }

                        break;

                case CANID_PERCH_SHUFFLE_RIGHT:

                        if (animal->along == 255)
                        {
                                doing = CANID_PERCH_SHUFFLE_LEFT;
                        }
                        else
                        {
                                animal->along += 1;
                        }

                        break;

                default:
                        ASSERT(0);
                        break;
        }

        animal->counter  = howlong;
        animal->counter |= doing << 6;

        //
        // Move the canid.
        //

        SLONG dest_x;
        SLONG dest_y;
        SLONG dest_z;

        CANID_find_pos_along_vect(
                animal->other_index,
                animal->along,
           &dest_x,
           &dest_y,
           &dest_z);

        GameCoord new_pos;

        new_pos.X = dest_x << 8;
        new_pos.Y = dest_y << 8;
        new_pos.Z = dest_z << 8;

        move_thing_on_map(canid, &new_pos);
}

void CANID_process_land(Thing *canid)
{
        Animal *animal = ANIMAL_get_animal(canid);

}

  */

// ========================================================
//
// The state functions.
//
// ========================================================

// claude-ai: CANID_fn_init() — STATE_INIT handler. Records spawn position as home
// claude-ai: (home_x = WorldPos.X>>16, home_z = WorldPos.Z>>16), then transitions
// claude-ai: to STATE_NORMAL and begins the first behaviour via CANID_start_doing_something().
void CANID_fn_init(Thing* canid)
{
    Animal* animal = ANIMAL_get_animal(canid);

    animal->home_x = canid->WorldPos.X >> 16;
    animal->home_z = canid->WorldPos.Z >> 16;

    //
    // Start being a normal canid.
    //

    set_state_function(canid, STATE_NORMAL);

    //
    // Start doing something.
    //

    CANID_start_doing_something(canid);
}

// claude-ai: CANID_fn_normal() — per-frame STATE_NORMAL handler. CRITICAL NOTE:
// claude-ai: The entire substate dispatch switch is commented out here. This means
// claude-ai: canids are completely inert at runtime in this codebase version.
// claude-ai: The commented-out switch dispatches to: process_sleep, process_bark,
// claude-ai: process_prowl, process_chase based on Animal::substate.
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
                    case CANID_SUBSTATE_NONE:									break;
                    case CANID_SUBSTATE_SLEEP:	CANID_process_sleep(canid);		break;
                    case CANID_SUBSTATE_BARK :	CANID_process_bark (canid);		break;
                    case CANID_SUBSTATE_PROWL:	CANID_process_prowl(canid);		break;
                    case CANID_SUBSTATE_CHASE:	CANID_process_chase(canid);		break;

                    default:
                            ASSERT(0);
                            break;
            }
    */
}

/********************************************************************************
 *
 * CODE GRAVEYARD - HERE BE DRAGONS
 *
 */

//	Thing  *p_scary;
//	Animal *scary_animal;
/*   'scary' code marked out but kept -- useful when doing alert/flee stuff later

        //
        // Is the canid near anything scary?
        //

        #define CANID_MAX_SCARY 32

        THING_INDEX scary[CANID_MAX_SCARY];
        SLONG       scary_upto;

        //
        // How far to look for something scary.
        //

        #define CANID_SCARY_RADIUS (0xd0)

        scary_upto = THING_find_sphere(
                                        canid->WorldPos.X >> 8,
                                        canid->WorldPos.Y >> 8,
                                        canid->WorldPos.Z >> 8,
                                        CANID_SCARY_RADIUS,
                                        scary,
                                        CANID_MAX_SCARY,
                                        THING_FIND_MOVING);

        for (i = 0; i < scary_upto; i++)
        {
                p_scary = TO_THING(scary[i]);

                switch(p_scary->Class)
                {
                        case CLASS_ANIMAL:

                                scary_animal = ANIMAL_get_animal(p_scary);

                                //
                                // A canid is scared of any animal apart from a canid.
                                //

                                if (scary_animal->AnimalType != ANIMAL_CANID)
                                {
                                        CANID_init_fly(canid, p_scary);
                                }

                                break;

                        case CLASS_PERSON:
                        case CLASS_PROJECTILE:
                                CANID_init_fly(canid, p_scary);
                                break;

                        default:
                                ASSERT(0);
                                break;
                }
        }

        */

// debuuuuuuug

/*
                SLONG px = canid->WorldPos.X >> 8;
                SLONG py = canid->WorldPos.Y >> 8;
                py += 0x10;
                SLONG pz = canid->WorldPos.Z >> 8;

                        DIRT_new_water(px + 2, py, pz,     -1, 28,  0);
                        DIRT_new_water(px    , py, pz + 2,  0, 29, -1);
                        DIRT_new_water(px    , py, pz - 2,  0, 28, +1);
                        DIRT_new_water(px - 2, py, pz,     +1, 29,  0);
*/

/*
void CANID_find_pos_along_vect(SLONG vect, SLONG along, SLONG *x, SLONG *y, SLONG *z)
{
        CollisionVect *cv;

        ASSERT(WITHIN(vect, 1, next_col_vect - 1));

        cv = &col_vects[vect];

        *x  = cv->X[0] + (along * (cv->X[1] - cv->X[0]) >> 8);
        *y  = cv->Y[0] + (along * (cv->Y[1] - cv->Y[0]) >> 8);
        *z  = cv->Z[0] + (along * (cv->Z[1] - cv->Z[0]) >> 8);

        *y += cv->PrimExtra << 6;
}
*/

/*

//
// Find a colvect for the canid to perch on. Returns NULL
// if there is no suitable colvect nearby.
//

UWORD CANID_find_perch(Thing *canid, UWORD ignore_this_vect)
{
        SLONG x;
        SLONG z;

        SLONG mx;
        SLONG mz;

        SLONG dx;
        SLONG dy;
        SLONG dz;

        SLONG nx1;
        SLONG nz1;
        SLONG nx2;
        SLONG nz2;

        SLONG len;
        SLONG dist1;
        SLONG dist2;

        SLONG list;
        SLONG vect;

        SLONG score;

        SLONG best_vect;
        SLONG best_score;

        CollisionVectLink *cvl;
        CollisionVect     *cv;
        MapElement        *me;

        //
        // Find the box in which to look for colvects.
        //

        #define CANID_PERCH_SEARCH 4

        mx = canid->WorldPos.X >> 16;
        mz = canid->WorldPos.Z >> 16;

        nx1 = mx - CANID_PERCH_SEARCH;
        nz1 = mz - CANID_PERCH_SEARCH;
        nx2 = mx + CANID_PERCH_SEARCH;
        nz2 = mz + CANID_PERCH_SEARCH;

        SATURATE(nx1, 0, MAP_WIDTH - 1);
        SATURATE(nx2, 0, MAP_WIDTH - 1);

        SATURATE(nz1, 0, MAP_HEIGHT - 1);
        SATURATE(nz2, 0, MAP_HEIGHT - 1);

        //
        // Look for the best colvect in the bounding box.
        //

        best_score = INFINITY;	// Lower scores are better!

        for (x = nx1; x <= nx2; x++)
        for (z = nz1; z <= nz2; z++)
        {
                me = &MAP[MAP_INDEX(x,z)];

                //
                // Go through this mapsquare looking for collision vectors.
                //

                list = me->ColVectHead;

                while(list)
                {
                        ASSERT(WITHIN(list, 1, next_col_vect_link - 1));

                        cvl  = &col_vects_links[list];
                        vect =  cvl->VectIndex;

                        //
                        // The col vect hanging off this link.
                        //

                        ASSERT(WITHIN(vect, 1, next_col_vect - 1));

                        if (vect != ignore_this_vect)
                        {
                                cv = &col_vects[vect];

                                dx = abs(cv->X[1] - cv->X[0]);
                                dz = abs(cv->Z[1] - cv->Z[0]);

                                len = QDIST2(dx,dz);

                                if (len < 256)
                                {
                                        //
                                        // Ignore colvects that are too small.
                                        //
                                }
                                else
                                {
                                        //
                                        // How far is this colvect from the canid?
                                        //

                                        dx = cv->X[0] - (canid->WorldPos.X>>8);
                                        dz = cv->Z[0] - (canid->WorldPos.Z>>8);

                                        dist1 = dx*dx + dz*dz;

                                        dx = cv->X[1] - (canid->WorldPos.X>>8);
                                        dz = cv->Z[1] - (canid->WorldPos.Z>>8);

                                        dist2 = dx*dx + dz*dz;

                                        score = MIN(dist1, dist2);

                                        if (score < best_score)
                                        {
                                                best_vect  = vect;
                                                best_score = score;
                                        }
                                }
                        }

                        list = cvl->Next;
                }
        }

        if (best_score < INFINITY)
        {
                return best_vect;
        }
        else
        {
                //
                // No nearby colvects!
                //

                return NULL;
        }
}
*/
/*
void CANID_init_walk(Thing *canid)
{
        SLONG dx;
        SLONG dz;

        SLONG angle;

        Animal   *animal = ANIMAL_get_animal  (canid);
        DrawMesh *dm     = ANIMAL_get_drawmesh(canid);

        //
        // Pick a direction to walk in.
        //

        angle = rand() & 2047;

        //
        // Turn to face in this direction.
        //

        dm->Angle = angle;

        //
        // How long should we walk for?
        //

        animal->counter  = rand() & 0x1f;
        animal->counter += 0xf;

        //
        // Start walking!
        //

        animal->substate = CANID_SUBSTATE_WALK;
}

void CANID_init_peck(Thing *canid)
{
        Animal *animal = ANIMAL_get_animal(canid);

        //
        // Just make the canid jump up and down for now!
        // But for how long?
        //

        #define CANID_PECK_TIME 32

        animal->counter = CANID_PECK_TIME;

        //
        // Start pecking!
        //

        animal->substate = CANID_SUBSTATE_PECK;
}

void CANID_init_fly(Thing *canid, Thing *from)
{
        SLONG dx;
        SLONG dz;

        SLONG dist;
        SLONG dest_x;
        SLONG dest_y;
        SLONG dest_z;

        SLONG angle;
        UWORD ignore_vect;

        //
        // Find a perch.
        //

        UWORD vect;

        Animal   *animal = ANIMAL_get_animal  (canid);
        DrawMesh *dm     = ANIMAL_get_drawmesh(canid);

        //
        // Dont fly to the same colvect you are already on.
        //

        if (animal->substate == CANID_SUBSTATE_PERCH)
        {
                ignore_vect = animal->other_index;
        }
        else
        {
                ignore_vect = NULL;
        }

        vect = CANID_find_perch(canid, ignore_vect);

        if (vect)
        {
                //
                // Start flying to somewhere along the otp of this vect.
                //

                animal->substate    = CANID_SUBSTATE_FLY;
                animal->other_index = vect;
                animal->along       = Random();
                animal->counter     = 0;
                animal->starty      = canid->WorldPos.Y >> 8;
                animal->map_x       = canid->WorldPos.X >> 16;
                animal->map_z       = canid->WorldPos.Z >> 16;

                //
                // How far does the canid start from its destination?
                //

                CANID_find_pos_along_vect(
                        animal->other_index,
                        animal->along,
                   &dest_x,
                   &dest_y,
                   &dest_z);

                dx = dest_x - (canid->WorldPos.X >> 8);
                dz = dest_z - (canid->WorldPos.Z >> 8);

                dist = QDIST2(abs(dx),abs(dz));

                animal->dist = dist;
        }
        else
        {
                //
                // Nowhere to fly to, just flee.
                //

                CANID_init_flee(canid, from);
        }
}
*/

/*
void CANID_init_flee(Thing *canid, Thing *from)
{
        SLONG dx;
        SLONG dz;
        SLONG angle;

        Animal   *animal = ANIMAL_get_animal  (canid);
        DrawMesh *dm     = ANIMAL_get_drawmesh(canid);

        //
        // Where do we run from?
        //

        dx = (from->WorldPos.X - canid->WorldPos.X)>>8;
        dz = (from->WorldPos.Z - canid->WorldPos.Z)>>8;

        angle  = 1024 - Arctan(dx, dz);
        angle += rand() & 0x3f;
        angle -= 0x1f;
        angle &= 2047;

        //
        // How long to we run away for?
        //

        #define CANID_FLEE_TIME 0x20

        dm->Angle       = angle;
        animal->counter = CANID_FLEE_TIME;

        //
        // Start fleeing.
        //

        animal->substate = CANID_SUBSTATE_FLEE;

        MSG_add("Init flee");
}
*/

/*

//
// Only call this function when you've finished flying.
//

void CANID_init_perch(Thing *canid)
{
        SLONG dx;
        SLONG dz;

        SLONG angle;

        Animal   *animal = ANIMAL_get_animal  (canid);
        DrawMesh *dm     = ANIMAL_get_drawmesh(canid);

        ASSERT(animal->substate == CANID_SUBSTATE_FLY);

        CollisionVect *cv;

        ASSERT(WITHIN(animal->other_index, 1, next_col_vect - 1));

        cv = &col_vects[animal->other_index];

        //
        // Face pointing out of the colvect you have perched on.
        //

        dx = cv->X[1] - cv->X[0];
        dz = cv->Z[1] - cv->Z[0];

        angle  = 512 - Arctan(dx, dz);

        dm->Angle = angle;

        //
        // The top 2 bits of counter mean either, wait, shuffle left or shuffle right.
        //

        animal->counter  = Random() & 31;
        animal->counter += 32;
        animal->counter |= CANID_PERCH_WAIT << 6;		// Wait...

        animal->substate = CANID_SUBSTATE_PERCH;
}


void CANID_init_land(Thing *canid)
{
}
*/

