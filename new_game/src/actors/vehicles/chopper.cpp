// uc_orig: chopper.cpp (fallen/Source/chopper.cpp)
// Helicopter (CLASS_CHOPPER) — allocation, AI state machine, and flight physics.

#include "fallen/Headers/Game.h"
#include "actors/core/statedef.h"
#include "world/map/pap_globals.h"
#include "ai/mav.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "effects/dirt.h"
#include "missions/eway.h" // Temporary: actors → missions DAG violation (EWAY_grab_camera)
#include <stdio.h>

#include "actors/vehicles/chopper.h"
#include "actors/vehicles/chopper_globals.h"

// Chopper detection/avoidance radii (in world units).
// uc_orig: HARDWIRED_RADIUS (fallen/Source/chopper.cpp)
#define HARDWIRED_RADIUS 8192
// uc_orig: DETECTION_RADIUS (fallen/Source/chopper.cpp)
#define DETECTION_RADIUS 1024
// uc_orig: IGNORAMUS_RADIUS (fallen/Source/chopper.cpp)
#define IGNORAMUS_RADIUS 6144

// uc_orig: PRIM_OBJ_CHOPPER (fallen/Source/chopper.cpp)
#define PRIM_OBJ_CHOPPER 74
// uc_orig: PRIM_OBJ_CHOPPER_BLADES (fallen/Source/chopper.cpp)
#define PRIM_OBJ_CHOPPER_BLADES 75

// uc_orig: init_choppers (fallen/Source/chopper.cpp)
void init_choppers(void)
{
    memset((UBYTE*)CHOPPERS, 0, sizeof(Chopper) * MAX_CHOPPERS);
    CHOPPER_COUNT = 0;
}

// uc_orig: alloc_chopper (fallen/Source/chopper.cpp)
Thing* alloc_chopper(UBYTE type)
{
    SLONG i;

    Thing* p_thing;
    Chopper* p_chopper;
    DrawMesh* dm;

    THING_INDEX t_index;
    SLONG a_index;

    ASSERT(WITHIN(type, 1, CHOPPER_NUMB - 1));

    for (i = 1; i < MAX_CHOPPERS; i++) {
        if (CHOPPERS[i].ChopperType == CHOPPER_NONE) {
            a_index = i;
            goto found_chopper;
        }
    }

    return NULL;

found_chopper:
    dm = alloc_draw_mesh();

    if (dm == NULL) {
        ASSERT(0);
        TO_CHOPPER(a_index)->ChopperType = CHOPPER_NONE;
        return NULL;
    }

    t_index = alloc_primary_thing(CLASS_CHOPPER);

    if (t_index) {
        p_thing = TO_THING(t_index);
        p_chopper = TO_CHOPPER(a_index);

        p_thing->Class = CLASS_CHOPPER;
        p_thing->Genus.Chopper = p_chopper;
        p_chopper->ChopperType = type;
        p_chopper->thing = p_thing;
        p_chopper->speed = 2048;
        p_chopper->radius = HARDWIRED_RADIUS;

        p_chopper->substate = CHOPPER_substate_landed;

        p_thing->DrawType = DT_CHOPPER;
        p_thing->Draw.Mesh = dm;

        dm->Angle = 0;
        dm->Roll = 0;
        dm->Tilt = 0;
        dm->ObjectId = PRIM_OBJ_CHOPPER;
        p_chopper->rotorprim = PRIM_OBJ_CHOPPER_BLADES;

        return p_thing;
    } else {
        TO_CHOPPER(a_index)->ChopperType = CHOPPER_NONE;
        free_draw_mesh(dm);
    }
}

// uc_orig: free_chopper (fallen/Source/chopper.cpp)
void free_chopper(Thing* p_thing)
{
    Chopper* chopper = CHOPPER_get_chopper(p_thing);

    chopper->ChopperType = CHOPPER_NONE;
    CHOPPER_COUNT -= 1;

    free_thing(p_thing);
}

// uc_orig: CHOPPER_create (fallen/Source/chopper.cpp)
Thing* CHOPPER_create(GameCoord pos, UBYTE type)
{
    Thing* p_thing = alloc_chopper(type);

    if (p_thing != NULL) {
        p_thing->WorldPos = pos;

        add_thing_to_map(p_thing);

        set_state_function(p_thing, STATE_INIT);
    }

    return p_thing;
}

// uc_orig: CHOPPER_get_chopper (fallen/Source/chopper.cpp)
Chopper* CHOPPER_get_chopper(struct Thing* chopper_thing)
{
    Chopper* chopper;

    ASSERT(WITHIN(chopper_thing, TO_THING(1), TO_THING(MAX_THINGS)));
    ASSERT(chopper_thing->Class == CLASS_CHOPPER);

    chopper = chopper_thing->Genus.Chopper;

    ASSERT(WITHIN(chopper, TO_CHOPPER(1), TO_CHOPPER(MAX_CHOPPERS - 1)));

    return chopper;
}

// uc_orig: CHOPPER_get_drawmesh (fallen/Source/chopper.cpp)
DrawMesh* CHOPPER_get_drawmesh(struct Thing* chopper_thing)
{
    DrawMesh* dm;

    ASSERT(WITHIN(chopper_thing, TO_THING(1), TO_THING(MAX_THINGS)));
    ASSERT(chopper_thing->Class == CLASS_CHOPPER);

    dm = chopper_thing->Draw.Mesh;

    ASSERT(WITHIN(dm, &DRAW_MESHES[0], &DRAW_MESHES[MAX_DRAW_MESHES - 1]));

    return dm;
}

// uc_orig: CHOPPER_altitude (fallen/Source/chopper.cpp)
SLONG CHOPPER_altitude(Thing* thing)
{
    return (thing->WorldPos.Y - (250 << 8)) - (PAP_calc_map_height_at(thing->WorldPos.X >> 8, thing->WorldPos.Z >> 8));
}

// uc_orig: CHOPPER_home (fallen/Source/chopper.cpp)
void CHOPPER_home(Thing* thing, GameCoord new_pos)
{
    Chopper* chopper = CHOPPER_get_chopper(thing);
    DrawMesh* dm = CHOPPER_get_drawmesh(thing);
    SLONG dx, dz, angle, dangle;
    UBYTE accel;
    UWORD speed;

    dx = new_pos.X - thing->WorldPos.X;
    dz = new_pos.Z - thing->WorldPos.Z;

    chopper->dist = SDIST2((dx >> 8), (dz >> 8));

    accel = 0;

    if (chopper->dist > (128 * 128)) {
        if (chopper->dist > (512 * 512)) {
            accel = 96;
        } else {
            accel = 64;
        }
    }

    if (chopper->since_takeoff < 255) {
        accel = chopper->since_takeoff >> 2;
        chopper->since_takeoff += 5;
    }

    speed = chopper->dist / 64;
    if (speed < 2048)
        speed = 2048;
    if (speed > 12288)
        speed = 12288;
    chopper->speed = speed;
    if (accel) {
        if (thing->WorldPos.X > new_pos.X)
            chopper->dx -= accel;
        if (thing->WorldPos.X < new_pos.X)
            chopper->dx += accel;
        if (thing->WorldPos.Z > new_pos.Z)
            chopper->dz -= accel;
        if (thing->WorldPos.Z < new_pos.Z)
            chopper->dz += accel;
    }

    if ((chopper->dist > (512 * 512)) || chopper->counter) {
        angle = -Arctan(dx, dz);
        angle &= 2047;
        dangle = angle - dm->Angle;
        if (dangle > 1024) {
            dangle -= 2048;
        }
        if (dangle < -1024) {
            dangle += 2048;
        }

        dangle >>= 3;

        SATURATE(dangle, -10, +10);

        chopper->ry = dangle;

        if (chopper->dist > (512 * 512))
            chopper->counter = 128;
    }
    if (chopper->counter)
        chopper->counter--;

    dm->Angle += chopper->ry;
    dm->Angle &= 2047;
    chopper->ry = (chopper->ry * 204) >> 8;

    // Avoid buildings by testing MAV voxels in each direction.
    {
        SLONG dist;
        SLONG mid_x = thing->WorldPos.X >> 8;
        SLONG mid_y = thing->WorldPos.Y >> 8;
        SLONG mid_z = thing->WorldPos.Z >> 8;

        mid_y -= 0xff;

// uc_orig: CHOPPER_AVOID_SPEED (fallen/Source/chopper.cpp)
#define CHOPPER_AVOID_SPEED 64

        for (dist = 128; dist <= 0x400; dist += 128) {
            if (MAV_inside(mid_x + dist, mid_y, mid_z)) {
                chopper->dx -= CHOPPER_AVOID_SPEED;
            }
            if (MAV_inside(mid_x - dist, mid_y, mid_z)) {
                chopper->dx += CHOPPER_AVOID_SPEED;
            }
            if (MAV_inside(mid_x, mid_y, mid_z + dist)) {
                chopper->dz -= CHOPPER_AVOID_SPEED;
            }
            if (MAV_inside(mid_x, mid_y, mid_z - dist)) {
                chopper->dz += CHOPPER_AVOID_SPEED;
            }
        }
    }
}

// uc_orig: CHOPPER_limit (fallen/Source/chopper.cpp)
void CHOPPER_limit(Chopper* chopper)
{
    if (chopper->dx > chopper->speed)
        chopper->dx = (chopper->dx + chopper->dx + chopper->dx + chopper->speed) >> 2;
    if (chopper->dz > chopper->speed)
        chopper->dz = (chopper->dz + chopper->dz + chopper->dz + chopper->speed) >> 2;
    if (chopper->dx < -chopper->speed)
        chopper->dx = (chopper->dx + chopper->dx + chopper->dx - chopper->speed) >> 2;
    if (chopper->dz < -chopper->speed)
        chopper->dz = (chopper->dz + chopper->dz + chopper->dz - chopper->speed) >> 2;
}

// uc_orig: CHOPPER_damp (fallen/Source/chopper.cpp)
void CHOPPER_damp(Chopper* chopper, UBYTE factor)
{
    chopper->dx = (chopper->dx * 230) >> 8;
    chopper->dz = (chopper->dz * 230) >> 8;
}

// uc_orig: CHOPPER_radius_broken (fallen/Source/chopper.cpp)
UBYTE CHOPPER_radius_broken(GameCoord pnt, GameCoord ctr, SLONG radius)
{
    SLONG dist, dx, dz;

    dx = (pnt.X - ctr.X) >> 8;
    dz = (pnt.Z - ctr.Z) >> 8;
    return (((dx * dx) + (dz * dz)) > (radius * radius));
}

// uc_orig: CHOPPER_predict_altitude (fallen/Source/chopper.cpp)
void CHOPPER_predict_altitude(Thing* thing, Chopper* chopper)
{
    SLONG tx, tz, dx, dz, altitude, gnd;
    SLONG dist;

    if ((chopper->dx == 0) && (chopper->dz == 0))
        return;

    tx = thing->WorldPos.X >> 8;
    tz = thing->WorldPos.Z >> 8;

    dx = chopper->dx >> 3;
    dz = chopper->dz >> 3;

    tx += dx;
    tz += dz;

    gnd = PAP_calc_height_at(tx, tz) + 0x10;

    altitude = (675 << 8) + (PAP_calc_map_height_at(tx, tz) << 8);

    // Stay higher than target if applicable.
    if (chopper->target && (altitude < chopper->target->WorldPos.Y + (50 << 8)))
        altitude = chopper->target->WorldPos.Y + (50 << 8);

    altitude -= CHOPPER_altitude(thing);

    dist = Root((dx * dx) + (dz * dz));
    dist = dist ? dist : 1;
    altitude /= dist;

    altitude /= 32;

    if (chopper->since_takeoff != 255) {
        SATURATE(altitude, -6, +6);
    }

    chopper->dy += altitude;
    chopper->dy -= chopper->dy / 32;
}

// uc_orig: CHOPPER_fn_init (fallen/Source/chopper.cpp)
void CHOPPER_fn_init(Thing* thing)
{
    Chopper* chopper = CHOPPER_get_chopper(thing);
    DrawMesh* dm = CHOPPER_get_drawmesh(thing);

    chopper->home.X = thing->WorldPos.X;
    chopper->home.Y = thing->WorldPos.Y;
    chopper->home.Z = thing->WorldPos.Z;

    set_state_function(thing, STATE_NORMAL);
}

// uc_orig: CHOPPER_fn_normal (fallen/Source/chopper.cpp)
void CHOPPER_fn_normal(Thing* thing)
{
    GameCoord new_pos;
    SLONG mag, rpos, altitude;
    CBYTE msg[300];

    // Track player 0 (Darci) as default target.
    Thing* darci = NET_PERSON(0);

    Chopper* chopper = CHOPPER_get_chopper(thing);

    if (chopper->substate != CHOPPER_substate_landed) {
        MFX_play_thing(THING_NUMBER(thing), S_HELI, MFX_LOOPED | MFX_QUEUED | MFX_MOVING, thing);
    }

    // Apply accumulated motion vector.
    chopper->rotors += (chopper->rotorspeed >> 2);
    new_pos = thing->WorldPos;
    new_pos.X += chopper->dx;
    new_pos.Y += chopper->dy;
    new_pos.Z += chopper->dz;
    move_thing_on_map(thing, &new_pos);

    altitude = CHOPPER_altitude(thing);

    // Generate rotor-wash turbulence when close to the ground.
    if (chopper->rotorspeed > 0) {
        if (altitude < (1200 << 8)) {
            mag = chopper->rotorspeed - (altitude >> 10);
            if (mag > 0) {
                rpos = chopper->rotors;
                rpos &= 2047;
                new_pos.X = (mag * SIN(rpos)) >> 10;
                new_pos.Z = (mag * COS(rpos)) >> 10;

                new_pos.X += thing->WorldPos.X;
                new_pos.Z += thing->WorldPos.Z;

                DIRT_gust(
                    thing,
                    thing->WorldPos.X >> 8, thing->WorldPos.Z >> 8,
                    new_pos.X >> 8, new_pos.Z >> 8);

                rpos += 1024;
                rpos &= 2047;
                new_pos.X = (mag * SIN(rpos)) >> 10;
                new_pos.Z = (mag * COS(rpos)) >> 10;

                new_pos.X += thing->WorldPos.X;
                new_pos.Z += thing->WorldPos.Z;

                DIRT_gust(
                    thing,
                    thing->WorldPos.X >> 8, thing->WorldPos.Z >> 8,
                    new_pos.X >> 8, new_pos.Z >> 8);
            }
        }
    }

    switch (chopper->substate) {

    case CHOPPER_substate_idle:
        CHOPPER_damp(chopper, 1);
        if (chopper->victim)
            CHOPPER_init_state(thing, CHOPPER_substate_tracking);
        else if (!CHOPPER_radius_broken(darci->WorldPos, thing->WorldPos, DETECTION_RADIUS)) {
            if ((!chopper->target) && !chopper->victim)
                chopper->target = darci;
            CHOPPER_init_state(thing, CHOPPER_substate_tracking);
        }
        break;

    case CHOPPER_substate_takeoff:
        CHOPPER_damp(chopper, 1);
        if (chopper->rotorspeed < 500) {
            chopper->rotorspeed += 10;

            if (chopper->rotorspeed > 150) {
                chopper->dy += chopper->rotorspeed - 150 >> 6;
                chopper->dy += 2;
            }
        } else if (altitude < (170 << 8)) {
            chopper->dy += 10;

            if (chopper->light < 250) {
                chopper->light += 5;
            }
        } else {
            CHOPPER_init_state(thing, CHOPPER_substate_idle);
        }
        break;

    case CHOPPER_substate_landing:
        CHOPPER_damp(chopper, 4);
        if (altitude > 0) {
            chopper->dy = ((chopper->dy + chopper->dy + chopper->dy - (altitude >> 6)) * 0.25f) - 1.0f;
        } else {
            chopper->dy = 0;
            if (chopper->light > 3)
                chopper->light -= 3;
            if (chopper->light)
                chopper->light--;
            if (chopper->rotorspeed > 200)
                chopper->rotorspeed--;
            if (chopper->rotorspeed > 0)
                chopper->rotorspeed--;
            else
                CHOPPER_init_state(thing, CHOPPER_substate_landed);
        }
        break;

    case CHOPPER_substate_landed:
        if (chopper->victim || !CHOPPER_radius_broken(darci->WorldPos, thing->WorldPos, DETECTION_RADIUS)) {
            CHOPPER_init_state(thing, CHOPPER_substate_takeoff);
        }
        break;

    case CHOPPER_substate_tracking:

        if (chopper->victim) {
            chopper->victim = EWAY_get_person(chopper->victim);
            chopper->target = TO_THING(chopper->victim);
            chopper->victim = 0;
        }

        CHOPPER_predict_altitude(thing, chopper);
        CHOPPER_home(thing, chopper->target->WorldPos);
        CHOPPER_limit(chopper);

        if (chopper->target->Class == CLASS_PERSON) {
            if ((chopper->target->Genus.Person->PlayerID > 0) && (CHOPPER_radius_broken(thing->WorldPos, chopper->home, chopper->radius)))
                CHOPPER_init_state(thing, CHOPPER_substate_homing);
        }

        break;

    case CHOPPER_substate_patrolling:
        {
            GameCoord targ;
            SLONG rot;

            chopper->patrol++;
            rot = chopper->patrol;
            rot &= 2047;
            targ.X = chopper->home.X + (SIN(rot) * 5);
            targ.Z = chopper->home.Z + (COS(rot) * 5);
            targ.Y = thing->WorldPos.Y;
            CHOPPER_home(thing, targ);
        }
        break;

    case CHOPPER_substate_homing:
        CHOPPER_predict_altitude(thing, chopper);
        CHOPPER_home(thing, chopper->home);
        CHOPPER_limit(chopper);

        if (!CHOPPER_radius_broken(thing->WorldPos, chopper->home, IGNORAMUS_RADIUS)) {
            if (!CHOPPER_radius_broken(darci->WorldPos, thing->WorldPos, DETECTION_RADIUS)) {
                chopper->target = darci;
                CHOPPER_init_state(thing, CHOPPER_substate_tracking);
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: CHOPPER_init_state (fallen/Source/chopper.cpp)
void CHOPPER_init_state(Thing* chopper_thing, UBYTE new_state)
{
    Chopper* chopper = CHOPPER_get_chopper(chopper_thing);

    switch (new_state) {
    case CHOPPER_substate_idle:
        break;
    case CHOPPER_substate_takeoff:
        chopper->counter = 0;
        chopper->since_takeoff = 0;
        break;
    case CHOPPER_substate_landing:
        chopper->counter = 0;
        break;
    case CHOPPER_substate_landed:
        chopper->counter = 0;
        break;
    case CHOPPER_substate_tracking:
        if (chopper->victim) {
            chopper->victim = EWAY_get_person(chopper->victim);
            chopper->target = TO_THING(chopper->victim);
            chopper->victim = 0;
        }
        if (!(chopper->target)) {
            CHOPPER_init_state(chopper_thing, CHOPPER_substate_idle);
        }
        chopper->counter = 0;
        break;
    case CHOPPER_substate_homing:
        chopper->target = 0;
        chopper->counter = 0;
        break;
    case CHOPPER_substate_patrolling:
        chopper->target = 0;
        chopper->patrol = 0;
        break;
    }
    chopper->substate = new_state;
}

// Temporary: mesh.h not yet migrated — needed for MESH_draw_poly_inv_matrix
#include "fallen/DDEngine/Headers/mesh.h"
// Temporary: cone.h accessed via new/ but needs the full include chain
#include "engine/graphics/geometry/cone.h"
#include "actors/characters/anim_ids.h"

// uc_orig: CHOPPER_draw_chopper (fallen/DDEngine/Source/drawxtra.cpp)
// Draws the chopper body and rotor meshes, then projects a CONE spotlight from the
// underside if the searchlight is on (light != 0). The spotlight tracks Darci's pelvis.
void CHOPPER_draw_chopper(Thing* p_chopper)
{
    Chopper* chopper = CHOPPER_get_chopper(p_chopper);
    SLONG matrix[9], vector[3];

    vector[0] = (chopper->dx / 64) & 2047;
    vector[1] = (-chopper->dz / 64) & 2047;
    vector[2] = 0;
    p_chopper->Draw.Mesh->Roll = vector[0];
    p_chopper->Draw.Mesh->Tilt = vector[1];

    if (!(chopper->target)) {

        FMATRIX_calc(matrix, p_chopper->Draw.Mesh->Angle, p_chopper->Draw.Mesh->Tilt, p_chopper->Draw.Mesh->Roll);

        vector[0] = vector[2] = 0;
        vector[1] = -1;

        FMATRIX_MUL(matrix, vector[0], vector[1], vector[2]);

        chopper->spotx = p_chopper->WorldPos.X;
        chopper->spotz = p_chopper->WorldPos.Z;

        chopper->spotdx = chopper->spotdz = 0;

    } else {

        SLONG target_x;
        SLONG target_y;
        SLONG target_z;

        // Track Darci's pelvis as the spotlight aim point.
        calc_sub_objects_position(
            chopper->target,
            chopper->target->Draw.Tweened->AnimTween,
            SUB_OBJECT_PELVIS,
            &target_x,
            &target_y,
            &target_z);

        target_x <<= 8;
        target_y <<= 8;
        target_z <<= 8;

        target_x += chopper->target->WorldPos.X;
        target_y += chopper->target->WorldPos.Y;
        target_z += chopper->target->WorldPos.Z;

        SLONG dx, dz, dist;
        SLONG maxspd = chopper->speed << 6;

        if (chopper->spotx > target_x)
            chopper->spotdx -= (chopper->since_takeoff >> 1);
        if (chopper->spotz > target_z)
            chopper->spotdz -= (chopper->since_takeoff >> 1);

        if (chopper->spotx < target_x)
            chopper->spotdx += (chopper->since_takeoff >> 1);
        if (chopper->spotz < target_z)
            chopper->spotdz += (chopper->since_takeoff >> 1);

        if (chopper->spotdx > maxspd)
            chopper->spotdx = maxspd;
        if (chopper->spotdx < -maxspd)
            chopper->spotdx = -maxspd;
        if (chopper->spotdz > maxspd)
            chopper->spotdz = maxspd;
        if (chopper->spotdz < -maxspd)
            chopper->spotdz = -maxspd;

        chopper->spotx += chopper->spotdx + chopper->dx;
        chopper->spotz += chopper->spotdz + chopper->dz;

        chopper->spotx = ((chopper->spotx * 24.0f) + target_x) * 0.04f;
        chopper->spotz = ((chopper->spotz * 24.0f) + target_z) * 0.04f;

        vector[0] = chopper->spotx - p_chopper->WorldPos.X;

        // If target is above the chopper, don't aim upward.
        if (target_y > p_chopper->WorldPos.Y)
            vector[1] = 0;
        else
            vector[1] = target_y - p_chopper->WorldPos.Y;

        vector[2] = chopper->spotz - p_chopper->WorldPos.Z;
    }

    MESH_draw_poly_inv_matrix(
        p_chopper->Draw.Mesh->ObjectId,
        p_chopper->WorldPos.X >> 8,
        p_chopper->WorldPos.Y >> 8,
        p_chopper->WorldPos.Z >> 8,
        -p_chopper->Draw.Mesh->Angle,
        -p_chopper->Draw.Mesh->Tilt,
        -p_chopper->Draw.Mesh->Roll,
        NULL);
    MESH_draw_poly_inv_matrix(
        chopper->rotorprim,
        p_chopper->WorldPos.X >> 8,
        p_chopper->WorldPos.Y >> 8,
        p_chopper->WorldPos.Z >> 8,
        -(p_chopper->Draw.Mesh->Angle + chopper->rotors),
        -(p_chopper->Draw.Mesh->Tilt),
        -(p_chopper->Draw.Mesh->Roll),
        NULL);

    if (chopper->light) {
        SLONG colour;

        colour = (0x66 * chopper->light) / 255;
        colour += (colour << 8);
        colour <<= 8;

        CONE_create(
            p_chopper->WorldPos.X >> 8,
            p_chopper->WorldPos.Y >> 8,
            p_chopper->WorldPos.Z >> 8,
            vector[0],
            vector[1],
            vector[2],
            256.0F * 5.0F,
            256.0F,
            colour,
            0x00000000,
            50);

        CONE_intersect_with_map();

        CONE_draw();
    }
}
