// game.h must come first: MFStdLib.h declares extern DisplayWidth/DisplayHeight,
// and gd_display.h redefines them as #define 640/480. Reversed order = syntax error.
#include "engine/platform/platform.h"
#include "missions/game_types.h"
#include "engine/physics/collide.h"     // LOS_FLAG_* constants
#include "actors/characters/anim_ids.h"
#include "engine/core/fmatrix.h"
#include "world/navigation/walkable.h"
#include "world/navigation/walkable_globals.h"
#include "world/environment/ware.h"
#include "world/environment/ware_globals.h"
#include "actors/core/statedef.h"
#include "missions/eway.h"
#include "world/level_pools.h"
#include "ui/camera/fc.h"
#include "ui/camera/fc_globals.h"
#include "world/map/pap.h"

// CAM_MORE_IN: PC camera is 25% closer to the player than the PSX version.
// Applied to cam_dist and camera height offsets.
// uc_orig: CAM_MORE_IN (fallen/Source/fc.cpp)
#define CAM_MORE_IN (0.75F)

// uc_orig: CAM_AT_HEAD (fallen/Source/fc.cpp)
#define CAM_AT_HEAD 1
// uc_orig: CAM_AT_WORLD_POS (fallen/Source/fc.cpp)
#define CAM_AT_WORLD_POS 2
// uc_orig: CAM_AT_FEET (fallen/Source/fc.cpp)
#define CAM_AT_FEET 3

// uc_orig: FC_ROTATE_DIR_LEFT (fallen/Source/fc.cpp)
#define FC_ROTATE_DIR_LEFT 0
// uc_orig: FC_ROTATE_DIR_RIGHT (fallen/Source/fc.cpp)
#define FC_ROTATE_DIR_RIGHT 1

// uc_orig: FC_PUSH_XS (fallen/Source/fc.cpp)
#define FC_PUSH_XS (1 << 0)
// uc_orig: FC_PUSH_XL (fallen/Source/fc.cpp)
#define FC_PUSH_XL (1 << 1)
// uc_orig: FC_PUSH_ZS (fallen/Source/fc.cpp)
#define FC_PUSH_ZS (1 << 2)
// uc_orig: FC_PUSH_ZL (fallen/Source/fc.cpp)
#define FC_PUSH_ZL (1 << 3)

// uc_orig: FC_CSP_HEIGHT (fallen/Source/fc.cpp)
#define FC_CSP_HEIGHT 0xa0
// uc_orig: FC_CSP_RADIUS (fallen/Source/fc.cpp)
#define FC_CSP_RADIUS 0x30

// uc_orig: FC_ROTATE_NEAR (fallen/Source/fc.cpp)
#define FC_ROTATE_NEAR (0x40)

// uc_orig: FC_DIST_MIN (fallen/Source/fc.cpp)
#define FC_DIST_MIN ((fc->cam_dist * offset_dist))
// uc_orig: FC_DIST_MAX (fallen/Source/fc.cpp)
#define FC_DIST_MAX ((fc->cam_dist * offset_dist))

extern UBYTE GAME_cut_scene;
extern SLONG analogue;

// uc_orig: person_has_gun_out (fallen/Source/fc.cpp)
extern SLONG person_has_gun_out(Thing* p_person);

// uc_orig: calc_sub_objects_position (fallen/Source/fc.cpp)
// Not declared in any header — forward declaration needed.
extern void calc_sub_objects_position(
    Thing* p_mthing,
    SLONG tween,
    UWORD object,
    SLONG* x,
    SLONG* y,
    SLONG* z);

// uc_orig: EWAY_cam_jumped (fallen/Source/fc.cpp)
extern SLONG EWAY_cam_jumped;
// Note: EWAY_grab_camera declared in fallen/Headers/eway.h (returns SLONG)

// uc_orig: set_slow_motion (fallen/Source/fc.cpp)
extern void set_slow_motion(UWORD motion);

// Adjusts the camera height offset (dheight) and distance (ddist) based on the
// focus character's current state: gun-out, driving a vehicle, or idle walking.
// Gun-out: camera moves closer and lower (dheight=0, ddist=200).
// In a car: dheight=0, ddist=356. Otherwise: ddist=256 unless idle.
// uc_orig: FC_alter_for_pos (fallen/Source/fc.cpp)
SLONG FC_alter_for_pos(FC_Cam* fc, SLONG* dheight, SLONG* ddist)
{
    if (person_has_gun_out(fc->focus) && !(fc->focus->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING))) {
        *dheight = 0;
        *ddist = 200;
        return 0;
    }
    if (fc->focus->Genus.Person->InCar) {
        switch (TO_THING(fc->focus->Genus.Person->InCar)->Genus.Vehicle->Type) {
        case 0:
        case 4:
        case 5:
        case 6:
        case 8:
            *dheight = 0;
            *ddist = 356;
            break;
        default:
            *dheight = 0;
            *ddist = 356;
            break;
        }
        return (0);
    }

    if (fc->focus->State != STATE_IDLE) {
        *dheight = 0;
        *ddist = 256;
        return (0);
    }

    *dheight = 0;
    *ddist = 256;

    return (0);
}

// uc_orig: FC_init (fallen/Source/fc.cpp)
void FC_init(void)
{
    SLONG i;

    memset(FC_cam, 0, sizeof(FC_Cam) * FC_MAX_CAMS);

    for (i = 0; i < FC_MAX_CAMS; i++) {
        FC_cam[i].lens = 0x24000;
        FC_cam[i].cam_dist = 0x280 * CAM_MORE_IN;
        FC_cam[i].cam_height = 0x16000;
    }

    GAME_cut_scene = 0;
}

// Switch between 4 camera distance/height presets.
// Type 0 (default): dist=0x280*0.75, height=0x16000.
// Type 1: wider view. Type 2: even wider. Type 3: low cinematic angle.
// uc_orig: FC_change_camera_type (fallen/Source/fc.cpp)
void FC_change_camera_type(SLONG cam, SLONG cam_type)
{
    switch (cam_type) {
    case 0:
        FC_cam[cam].cam_dist = 0x280 * CAM_MORE_IN;
        FC_cam[cam].cam_height = 0x16000;
        break;
    case 1:
        FC_cam[cam].cam_dist = 0x280;
        FC_cam[cam].cam_height = 0x20000;
        break;
    case 2:
        FC_cam[cam].cam_dist = 0x380;
        FC_cam[cam].cam_height = 0x25000;
        break;
    case 3:
        FC_cam[cam].cam_dist = 0x300;
        FC_cam[cam].cam_height = 0x8000;
        break;
    default:
        ASSERT(0);
        break;
    }
}

// Returns UC_TRUE if the camera needs to move up and around to the front of the player.
// Always returns UC_FALSE in this codebase (stub for future use or PSX variant).
// uc_orig: FC_must_move_up_and_around (fallen/Source/fc.cpp)
SLONG FC_must_move_up_and_around(SLONG cam)
{
    return UC_FALSE;
}

// uc_orig: FC_look_at (fallen/Source/fc.cpp)
void FC_look_at(SLONG cam, UWORD thing_index)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    fc->focus = TO_THING(thing_index);
    GAME_cut_scene = 0;
}

// uc_orig: FC_move_to (fallen/Source/fc.cpp)
void FC_move_to(SLONG cam, SLONG world_x, SLONG world_y, SLONG world_z)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    fc->x = world_x << 8;
    fc->y = world_y << 8;
    fc->z = world_z << 8;
    fc->want_x = world_x << 8;
    fc->want_y = world_y << 8;
    fc->want_z = world_z << 8;
    fc->dx = 0;
    fc->dy = 0;
    fc->dz = 0;
}

// Returns the desired height of the camera above the focus point.
// Adjusts based on: gun-out (lower), vehicle type, warehouse (adds 50%), split-screen (subtracts).
// uc_orig: FC_focus_above (fallen/Source/fc.cpp)
SLONG FC_focus_above(FC_Cam* fc)
{
    SLONG above;
    SLONG lower = 0;

    if (fc->focus->Class == CLASS_PERSON && person_has_gun_out(fc->focus)) {
        lower = 0xa000;
    }

    if (fc->focus->Genus.Person->InCar) {
        switch (TO_THING(fc->focus->Genus.Person->InCar)->Genus.Vehicle->Type) {
        case 4:
            lower = -(0x3000) * CAM_MORE_IN;
            break;
        case 0:
        case 5:
        case 6:
        case 8:
            lower = -(0x1000) * CAM_MORE_IN;
            break;
        default:
            lower = 0xa000 * CAM_MORE_IN;
            break;
        }
    }

    if (fc->focus->Class == CLASS_PERSON && fc->focus->Genus.Person->InsideIndex) {
        above = fc->cam_height + (fc->cam_height >> 1);
    } else {
        above = fc->cam_height;

        if (FC_cam[1].focus) {
            // Split-screen: camera sits lower because the viewport is narrower.
            above -= 0x4000;
            if (above < 0x1000)
                above = 0x1000;
        }
        above -= lower;
    }

    return above;
}

// Returns which body part of a person the camera should target.
// Dangling/climbing → feet; entering vehicle/dead → head; default → world position.
// uc_orig: FC_get_person_body_part_target (fallen/Source/fc.cpp)
SLONG FC_get_person_body_part_target(Thing* p_thing)
{
    switch (p_thing->Genus.Person->Action) {
    case ACTION_DANGLING:
    case ACTION_PULL_UP:
    case ACTION_CLIMBING:
        return (CAM_AT_FEET);

    case ACTION_HOP_BACK:
    case ACTION_SIDE_STEP:
    case ACTION_FLIP_LEFT:
    case ACTION_FLIP_RIGHT:
    case ACTION_DEAD:
    case ACTION_DYING:
    case ACTION_ENTER_VEHICLE:
        return (CAM_AT_HEAD);

    default:
        switch (p_thing->SubState) {
        case SUB_STATE_RUNNING_VAULT:
        case SUB_STATE_RUNNING_HALF_BLOCK:
            return (CAM_AT_FEET);
            break;
        default:
            return (CAM_AT_WORLD_POS);
        }
        break;
    }
    return (CAM_AT_WORLD_POS);
}

// Calculates the world position (focus_x/y/z) and yaw (focus_yaw) that the camera looks at.
// For persons: adjusts yaw based on driving/entering vehicle/hugging wall.
// For vehicles: smoothly tracks vehicle yaw.
// focus_x/y/z is set from head, feet, or world position depending on action.
// uc_orig: FC_calc_focus (fallen/Source/fc.cpp)
void FC_calc_focus(FC_Cam* fc)
{
    SLONG body_part = 0;

    fc->focus_in_warehouse = NULL;

    switch (fc->focus->Class) {
    case CLASS_PERSON:

        if (fc->focus->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
            Thing* p_vehicle = TO_THING(fc->focus->Genus.Person->InCar);

            SLONG yaw_car;

            yaw_car = p_vehicle->Genus.Vehicle->Angle;
            yaw_car -= p_vehicle->Genus.Vehicle->WheelAngle * p_vehicle->Velocity >> 10;

            if (p_vehicle->Velocity < 0) {
                yaw_car += 1024;
            }

            fc->focus_yaw = yaw_car & 2047;
        } else {
            fc->focus_yaw = fc->focus->Draw.Tweened->Angle;

            if (fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE) {
                if (fc->focus->Draw.Tweened->CurrentAnim == ANIM_ENTER_TAXI) {
                    fc->focus_yaw -= 712;
                    fc->focus_yaw &= 2047;
                } else {
                    fc->focus_yaw += 712;
                    fc->focus_yaw &= 2047;
                }
            } else if (fc->focus->Genus.Person->Action == ACTION_SIDE_STEP || fc->focus->Genus.Person->Action == ACTION_SIT_BENCH || fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE || fc->focus->Genus.Person->Action == ACTION_HUG_WALL) {
                fc->focus_yaw += 1024;
                fc->focus_yaw &= 2047;
            }
        }

        if (fc->focus->Genus.Person->Flags & FLAG_PERSON_WAREHOUSE) {
            fc->focus_in_warehouse = fc->focus->Genus.Person->Ware;
        }

        if (GAME_FLAGS & GF_SIDE_ON_COMBAT) {
            fc->focus_yaw -= 512;
        }

        if (fc->focus->State == STATE_DANGLING) {
            if (fc->focus->SubState == SUB_STATE_DANGLING_CABLE_FORWARD || fc->focus->SubState == SUB_STATE_DANGLING_CABLE_BACKWARD || fc->focus->SubState == SUB_STATE_DEATH_SLIDE || fc->focus->SubState == SUB_STATE_TRAVERSE_LEFT || fc->focus->SubState == SUB_STATE_TRAVERSE_RIGHT || fc->focus->Genus.Person->Mode == PERSON_MODE_FIGHT || fc->focus->SubState == SUB_STATE_DANGLING_CABLE) {
                // Look from the side when dangling on cable.
                SLONG a1;
                SLONG a2;
                SLONG dangle;

                a1 = fc->focus_yaw;
                a2 = fc->yaw >> 8;

                dangle = a2 - a1;
                dangle &= 2047;

                if (dangle > 1024) {
                    dangle -= 2048;
                }

                if (dangle < 0) {
                    fc->focus_yaw -= 550;
                } else {
                    fc->focus_yaw += 550;
                }

                fc->focus_yaw &= 2047;
            }
        }

        break;

    case CLASS_VEHICLE:
    {
        SLONG dyaw;
        SLONG yaw_car;
        SLONG yaw_cam;

        yaw_car = fc->focus->Genus.Vehicle->Angle;
        yaw_cam = fc->yaw >> 8;

        dyaw = yaw_car - yaw_cam;

        if (dyaw > +1024) {
            dyaw -= 2048;
        }
        if (dyaw < -1024) {
            dyaw += 2048;
        }

        fc->focus_yaw = yaw_car + dyaw * 4;
        fc->focus_yaw &= 2047;
    }
    break;

    default:
        ASSERT(0);
        break;
    }

    switch (fc->focus->Class) {
    case CLASS_PERSON:

        body_part = FC_get_person_body_part_target(fc->focus);

        switch (body_part) {
        case CAM_AT_HEAD:
            calc_sub_objects_position(
                fc->focus,
                fc->focus->Draw.Tweened->AnimTween,
                SUB_OBJECT_HEAD,
                &fc->focus_x,
                &fc->focus_y,
                &fc->focus_z);

            fc->focus_x <<= 8;
            fc->focus_z <<= 8;

            fc->focus_x += fc->focus->WorldPos.X;
            fc->focus_y = fc->focus->WorldPos.Y;
            fc->focus_z += fc->focus->WorldPos.Z;
            break;

        case CAM_AT_FEET:
        {
            SLONG lfx, lfy, lfz;
            SLONG rfx, rfy, rfz;

            calc_sub_objects_position(
                fc->focus,
                fc->focus->Draw.Tweened->AnimTween,
                SUB_OBJECT_LEFT_FOOT,
                &lfx, &lfy, &lfz);

            calc_sub_objects_position(
                fc->focus,
                fc->focus->Draw.Tweened->AnimTween,
                SUB_OBJECT_RIGHT_FOOT,
                &rfx, &rfy, &rfz);

            fc->focus_x = lfx + rfx << 7;
            fc->focus_y = lfy + rfy << 7;
            fc->focus_z = lfz + rfz << 7;

            fc->focus_x += fc->focus->WorldPos.X;
            fc->focus_y += fc->focus->WorldPos.Y;
            fc->focus_z += fc->focus->WorldPos.Z;
        }
        break;

        case CAM_AT_WORLD_POS:
            fc->focus_x = fc->focus->WorldPos.X;
            fc->focus_y = fc->focus->WorldPos.Y;
            fc->focus_z = fc->focus->WorldPos.Z;
            break;
        }
        break;

    default:
        fc->focus_x = fc->focus->WorldPos.X;
        fc->focus_y = fc->focus->WorldPos.Y;
        fc->focus_z = fc->focus->WorldPos.Z;
        break;
    }

    // Peek left/right wall-hug states: shift focus_x/z in the look direction.
    if (fc->focus->SubState == SUB_STATE_HUG_WALL_LOOK_L) {
        SLONG angle, dx, dz;
        SLONG velocity;
        angle = fc->focus->Draw.Tweened->Angle - 512;
        velocity = fc->focus->Genus.Person->InsideRoom;
        angle &= 2047;
        dx = -(SIN(angle) * velocity) >> 4;
        dz = -(COS(angle) * velocity) >> 4;
        fc->focus_x += dx;
        fc->focus_z += dz;
    }
    if (fc->focus->SubState == SUB_STATE_HUG_WALL_LOOK_R) {
        SLONG angle, dx, dz;
        SLONG velocity;
        angle = fc->focus->Draw.Tweened->Angle + 512;
        velocity = fc->focus->Genus.Person->InsideRoom;
        angle &= 2047;
        dx = -(SIN(angle) * velocity) >> 4;
        dz = -(COS(angle) * velocity) >> 4;
        fc->focus_x += dx;
        fc->focus_z += dz;
    }
}

// Computes want_yaw/pitch/roll to aim the camera at the current focus point.
// In toonear mode: adjusts aim angles from the saved pre-collision position.
// When driving: adds vehicle roll based on wheel angle and velocity.
// uc_orig: FC_look_at_focus (fallen/Source/fc.cpp)
void FC_look_at_focus(FC_Cam* fc)
{
    SLONG dx = fc->focus_x - fc->want_x >> 8;
    SLONG dy = fc->focus_y + fc->lookabove - fc->want_y >> 8;
    SLONG dz = fc->focus_z - fc->want_z >> 8;

    if (fc->toonear && fc->toonear_dist != 0x90000) {
        SLONG dangle;
        SLONG angle;

        dangle = fc->focus_yaw - fc->toonear_focus_yaw;

        if (dangle > +1024)  { dangle -= 2048; }
        if (dangle < -1024)  { dangle += 2048; }
        if (dangle < -512)   { dangle = -512 + (-512 - dangle); }
        if (dangle > 512)    { dangle = 512 - (dangle - 512); }

        angle = fc->toonear_focus_yaw + dangle;
        angle &= 2047;

        dx -= SIN(angle) >> 9;
        dz -= COS(angle) >> 8;
    }

    SLONG dxz = QDIST2(abs(dx), abs(dz));

    fc->want_yaw = -Arctan(dx, dz);
    fc->want_pitch = Arctan(dy, dxz);
    fc->want_roll = 1024;

    fc->want_yaw &= 2047;
    fc->want_pitch &= 2047;

    fc->want_yaw <<= 8;
    fc->want_pitch <<= 8;
    fc->want_roll <<= 8;

    if (fc->focus->Class == CLASS_PERSON && (fc->focus->Genus.Person->Flags & FLAG_PERSON_DRIVING)) {
        Thing* p_vehicle = TO_THING(fc->focus->Genus.Person->InCar);

        fc->want_roll = 1024 - (p_vehicle->Genus.Vehicle->WheelAngle * p_vehicle->Velocity >> 15);
        fc->want_roll &= 2047;
        fc->want_roll <<= 8;
    }
}

// Immediately snaps want_x/y/z to behind the focus character, running collision avoidance.
// If the ideal behind-position is blocked, tries 4 rotated alternatives.
// Sets toonear=UC_TRUE with saved pre-collision state if all alternatives fail (emergency mode).
// uc_orig: FC_force_camera_behind (fallen/Source/fc.cpp)
void FC_force_camera_behind(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG gox;
    SLONG goy;
    SLONG goz;

    FC_calc_focus(fc);

    gox = fc->focus_x + (SIN(fc->focus_yaw) * fc->cam_dist >> 8);
    goz = fc->focus_z + (COS(fc->focus_yaw) * fc->cam_dist >> 8);
    goy = fc->focus_y + FC_focus_above(fc);

    fc->toonear = UC_FALSE;

    if (!MAV_height_los_slow(
            fc->focus_in_warehouse,
            fc->focus_x >> 8,
            fc->focus_y + 0x10000 >> 8,
            fc->focus_z >> 8,
            gox >> 8,
            goy >> 8,
            goz >> 8)) {
        SLONG abort_x = MAV_height_los_fail_x << 8;
        SLONG abort_y = MAV_height_los_fail_y << 8;
        SLONG abort_z = MAV_height_los_fail_z << 8;

        SLONG i;
        SLONG dangle;

        for (i = 0; i < 4; i++) {
            switch (i) {
            case 0: dangle = +100; break;
            case 1: dangle = -100; break;
            case 2: dangle = +200; break;
            case 3: dangle = -200; break;
            }

            gox = fc->focus_x + (SIN((fc->focus_yaw + dangle) & 2047) * fc->cam_dist >> 8);
            goz = fc->focus_z + (COS((fc->focus_yaw + dangle) & 2047) * fc->cam_dist >> 8);

            if (MAV_height_los_slow(
                    fc->focus_in_warehouse,
                    fc->focus_x >> 8,
                    fc->focus_y + 0x10000 >> 8,
                    fc->focus_z >> 8,
                    gox >> 8,
                    goy >> 8,
                    goz >> 8)) {
                goto found_place;
            }
        }

        // Emergency: use the last collision point, move slightly below it.
        gox = abort_x;
        goy = abort_y;
        goz = abort_z;
        goy -= 0x6000;

        SLONG dx;
        SLONG dz;

        dx = abs(gox - fc->focus_x);
        dz = abs(goz - fc->focus_z);

        fc->toonear = UC_TRUE;
        fc->toonear_dist = QDIST2(dx, dz);
        fc->toonear_x = fc->want_x;
        fc->toonear_y = fc->want_y;
        fc->toonear_z = fc->want_z;
        fc->toonear_yaw = fc->want_yaw;
        fc->toonear_pitch = fc->want_pitch;
        fc->toonear_roll = fc->want_roll;
        fc->toonear_focus_yaw = fc->focus_yaw;
    }

found_place:;

    fc->want_x = gox;
    fc->want_y = goy;
    fc->want_z = goz;
}

// uc_orig: FC_setup_initial_camera (fallen/Source/fc.cpp)
void FC_setup_initial_camera(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG dx;
    SLONG dz;

    FC_calc_focus(fc);

    dx = -SIN(fc->focus_yaw);
    dz = -COS(fc->focus_yaw);

    if (there_is_a_los(
            fc->focus_x >> 8,
            fc->focus_y + 0xa000 >> 8,
            fc->focus_z >> 8,
            fc->focus_x + dx + dx + dx >> 8,
            fc->focus_x + 0xa000 >> 8,
            fc->focus_z + dz + dz + dz >> 8,
            LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
        fc->x = fc->want_x = fc->focus_x + dx + dx + dx;
        fc->y = fc->want_y = fc->focus_y + 0xa000;
        fc->z = fc->want_z = fc->focus_z + dz + dz + dz;
    } else {
        FC_force_camera_behind(cam);
        fc->x = fc->want_x;
        fc->y = fc->want_y;
        fc->z = fc->want_z;
    }

    FC_look_at_focus(fc);

    fc->yaw = fc->want_yaw;
    fc->pitch = fc->want_pitch;
    fc->roll = fc->want_roll;

    fc->toonear = UC_FALSE;
}

// Returns UC_TRUE if the camera can rotate in the given direction without clipping into geometry.
// Checks 4 corners of the rotated camera position against MAV_inside (or warehouse bounds).
// uc_orig: FC_allowed_to_rotate (fallen/Source/fc.cpp)
SLONG FC_allowed_to_rotate(FC_Cam* fc, SLONG rotate_dir)
{
    SLONG i;
    SLONG dx, dz;
    SLONG mx, my, mz;
    SLONG cx, cy, cz;

    dx = fc->focus_x - fc->want_x >> 2;
    dz = fc->focus_z - fc->want_z >> 2;

    switch (rotate_dir) {
    case FC_ROTATE_DIR_LEFT:
        mx = fc->x - (-dz) >> 8;
        my = fc->y >> 8;
        mz = fc->z - (+dx) >> 8;
        break;
    case FC_ROTATE_DIR_RIGHT:
        mx = fc->x + (-dz) >> 8;
        my = fc->y >> 8;
        mz = fc->z + (+dx) >> 8;
        break;
    default:
        ASSERT(0);
        break;
    }

    for (i = 0; i < 4; i++) {
        dx = 0;
        dz = 0;

        switch (i) {
        case 0: dx = +FC_ROTATE_NEAR; break;
        case 1: dx = -FC_ROTATE_NEAR; break;
        case 2: dz = +FC_ROTATE_NEAR; break;
        case 3: dz = -FC_ROTATE_NEAR; break;
        }

        cx = mx + dx;
        cy = my;
        cz = mz + dz;

        if (fc->focus_in_warehouse) {
            if (!MAV_inside(cx, cy, cz)) {
                return UC_FALSE;
            }
        } else {
            if (MAV_inside(cx, cy, cz)) {
                return UC_FALSE;
            }
        }
    }

    return UC_TRUE;
}

// uc_orig: FC_rotate_left (fallen/Source/fc.cpp)
void FC_rotate_left(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    if (FC_allowed_to_rotate(fc, FC_ROTATE_DIR_LEFT)) {
        fc->rotate = -0x600;
    }
}

// uc_orig: FC_rotate_right (fallen/Source/fc.cpp)
void FC_rotate_right(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    if (FC_allowed_to_rotate(fc, FC_ROTATE_DIR_RIGHT)) {
        fc->rotate = +0x600;
    }
}

// Positions the camera just inside the warehouse, in front of the door closest to the player.
// Called when the player first enters a warehouse to give a good initial inside view.
// uc_orig: FC_setup_camera_for_warehouse (fallen/Source/fc.cpp)
void FC_setup_camera_for_warehouse(SLONG cam)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG i;
    SLONG dx, dz;
    SLONG dist;
    SLONG best_dist = UC_INFINITY;
    SLONG best_door = -1;

    WARE_Ware* ww;

    ASSERT(fc->focus->Class == CLASS_PERSON);
    ASSERT(WITHIN(fc->focus->Genus.Person->Ware, 1, WARE_ware_upto - 1));

    ww = &WARE_ware[fc->focus->Genus.Person->Ware];

    for (i = 0; i < ww->door_upto; i++) {
        dx = ww->door[i].in_x - (fc->focus->WorldPos.X >> 16);
        dz = ww->door[i].in_z - (fc->focus->WorldPos.Z >> 16);
        dist = abs(dx) + abs(dz);
        if (best_dist > dist) {
            best_dist = dist;
            best_door = i;
        }
    }

    ASSERT(WITHIN(best_door, 0, ww->door_upto - 1));

    dx = ww->door[best_door].in_x - ww->door[best_door].out_x;
    dz = ww->door[best_door].in_z - ww->door[best_door].out_z;

    dx = SIGN(dx);
    dz = SIGN(dz);

    dx <<= 18;
    dz <<= 18;

    fc->want_x = fc->x = fc->focus->WorldPos.X + dx;
    fc->want_z = fc->z = fc->focus->WorldPos.Z + dz;
    fc->want_y = fc->y = fc->focus->WorldPos.Y + 0xa000;

    FC_look_at_focus(fc);

    fc->yaw = fc->want_yaw;
    fc->pitch = fc->want_pitch;
    fc->roll = fc->want_roll;

    fc->toonear = UC_FALSE;
}

// Main camera update called once per frame. Processes each active camera in FC_cam[].
// Steps per camera:
//   1. Skip if focus == NULL (inactive).
//   2. FC_alter_for_pos: get height/distance offsets from character state.
//   3. FC_calc_focus: compute the target look-at position.
//   4. Warehouse transition: snap camera on enter/exit.
//   5. lookabove: 0xa000 normally, decays on death.
//   6. Rotate: L1/R1 camera orbit; nobehind suppresses get-behind logic.
//   7. toonear cancel: exit first-person mode if angle diverges > 200.
//   8. Collision: 8-step raycast pushing camera away from walls/fences/roof.
//   9. Get-behind: push camera behind Darci with configurable strength.
//   10. Y position: track FC_focus_above with clamped delta.
//   11. Distance clamp: keep camera between FC_DIST_MIN and FC_DIST_MAX.
//   12. Position smoothing: want → actual (x/z >>2, y >>3).
//   13. Angle smoothing: want_yaw/pitch/roll → actual >>2.
//   14. Lens: fixed FOV = 0x28000 * CAM_MORE_IN.
//   15. Shake: random offset decays each frame.
// uc_orig: FC_process (fallen/Source/fc.cpp)
void FC_process()
{
    SLONG i;
    SLONG x, y, z;
    SLONG dx, dy, dz;
    SLONG cam;
    SLONG dist;
    SLONG ddist;
    SLONG xforce, yforce, zforce;
    SLONG behind_x, behind_z;

    UBYTE used_to_be_in_warehouse;
    SLONG offset_dist, offset_height;

    FC_Cam* fc;

    for (cam = 0; cam < FC_MAX_CAMS; cam++) {
        fc = &FC_cam[cam];

        if (fc->focus == NULL) {
            continue;
        }

        FC_alter_for_pos(fc, &offset_height, &offset_dist);

        used_to_be_in_warehouse = fc->focus_in_warehouse;

        FC_calc_focus(fc);

        // Warehouse transition: snap camera on entry/exit.
        if (used_to_be_in_warehouse != fc->focus_in_warehouse) {
            EWAY_cam_jumped = 10;
            if (fc->focus_in_warehouse) {
                FC_setup_camera_for_warehouse(cam);
            } else {
                FC_setup_initial_camera(cam);
            }
        }

        // lookabove: height the camera looks above the focus.
        // Slowly lowers after the focus person dies.
        if (fc->focus->Class == CLASS_PERSON && (fc->focus->State == STATE_DEAD || fc->focus->State == STATE_DYING)) {
            if (fc->lookabove > 0) {
                fc->lookabove -= 0x80 * TICK_RATIO >> TICK_SHIFT;
            }
        } else {
            fc->lookabove = 0xa000;
        }

        if (!fc->toonear) {
            if (fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE) {
                fc->rotate = 0;
                fc->nobehind = 0;
            }

            if (fc->rotate != 0) {
                if (fc->rotate > 0) {
                    fc->rotate -= 0x80 * TICK_RATIO >> TICK_SHIFT;
                }
                if (fc->rotate < 0) {
                    fc->rotate += 0x80 * TICK_RATIO >> TICK_SHIFT;
                }

                if (abs(fc->rotate) < 0x100) {
                    fc->rotate = 0;
                } else {
                    dx = fc->focus_x - fc->want_x >> 3;
                    dz = fc->focus_z - fc->want_z >> 3;

                    fc->want_x -= dz * (fc->rotate >> 4) * TICK_RATIO >> (TICK_SHIFT + 6);
                    fc->want_z += dx * (fc->rotate >> 4) * TICK_RATIO >> (TICK_SHIFT + 6);
                }

                fc->nobehind = 0x2000;
            }
        } else {
            fc->rotate = 0;
        }

        // Cancel toonear (first-person mode) if the camera angle diverges enough.
        if (fc->toonear) {
            dx = abs(fc->want_x - fc->focus_x);
            dz = abs(fc->want_z - fc->focus_z);
            dist = QDIST2(dx, dz);

            if (dist > fc->toonear_dist + 0x4000) {
                SLONG cdx = fc->focus_x - fc->want_x >> 8;
                SLONG cdz = fc->focus_z - fc->want_z >> 8;
                SLONG cangle = -Arctan(cdx, cdz) & 2047;
                SLONG dangle;

                dangle = fc->toonear_focus_yaw - cangle;

                if (dangle < -1024) { dangle += 2048; }
                if (dangle > +1024) { dangle -= 2048; }

                if (abs(dangle) > 200) {
                    fc->toonear = UC_FALSE;
                    fc->x = fc->want_x = fc->toonear_x;
                    fc->y = fc->want_y = fc->toonear_y;
                    fc->z = fc->want_z = fc->toonear_z;
                    fc->yaw = fc->want_yaw = fc->toonear_yaw;
                    fc->pitch = fc->want_pitch = fc->toonear_pitch;
                    fc->roll = fc->want_roll = fc->toonear_roll;
                    fc->smooth_transition = UC_TRUE;
                }
            }
        }

        if (!fc->toonear) {
            // 8-step raycast collision: push camera away from walls, fences, and roof.
            x = fc->want_x >> 8;
            y = fc->want_y >> 8;
            z = fc->want_z >> 8;

            y += 0x50;

            xforce = 0;
            yforce = 0;
            zforce = 0;

            dx = fc->focus_x - fc->want_x >> 11;
            dy = fc->focus_y - fc->want_y >> 11;
            dz = fc->focus_z - fc->want_z >> 11;

            if (fc->focus_in_warehouse) {
                for (i = 0; i < 8; i++) {
                    if (WARE_inside(fc->focus_in_warehouse, x, y, z)) {
                        // Inside warehouse bounds — no push.
                    } else {
                        // Push away from warehouse walls.
                        if (!(WARE_get_caps(fc->focus_in_warehouse, x >> 8, z >> 8, MAV_DIR_XS) & MAV_CAPS_GOTO)) {
                            xforce += 0x100 - (x & 0xff);
                        }
                        if (!(WARE_get_caps(fc->focus_in_warehouse, x >> 8, z >> 8, MAV_DIR_XL) & MAV_CAPS_GOTO)) {
                            xforce -= (x & 0xff);
                        }
                        if (!(WARE_get_caps(fc->focus_in_warehouse, x >> 8, z >> 8, MAV_DIR_ZS) & MAV_CAPS_GOTO)) {
                            zforce += 0x100 - (z & 0xff);
                        }
                        if (!(WARE_get_caps(fc->focus_in_warehouse, x >> 8, z >> 8, MAV_DIR_ZL) & MAV_CAPS_GOTO)) {
                            zforce -= (z & 0xff);
                        }

                        if (i < 4) {
                            if (WARE_inside(fc->focus_in_warehouse, x, y - 0x100, z)) {
                                yforce += (y & 0xff);
                            }

                            // Don't go above the roof.
                            {
                                SLONG roof = MAVHEIGHT(x >> 8, z >> 8) << 6;
                                if (y > roof - 0x100) {
                                    yforce -= y & 0xff;
                                }
                            }
                        }
                    }

                    x += dx;
                    y += dy;
                    z += dz;
                }
            } else {
                for (i = 0; i < 8; i++) {
                    if (i < 4) {
                        if (!MAV_inside(x, y, z)) {
                            if (MAV_inside(x, y - 0x100, z)) {
                                yforce += 256 - (y & 0xff);
                            }
                        }
                    }

                    // Push away from fences.
                    UBYTE push;

                    push = 0;

                    if (MAV_inside(x - 0x100, y, z)) { push |= FC_PUSH_XS; }
                    if (MAV_inside(x + 0x100, y, z)) { push |= FC_PUSH_XL; }
                    if (MAV_inside(x, y, z - 0x100)) { push |= FC_PUSH_ZS; }
                    if (MAV_inside(x, y, z + 0x100)) { push |= FC_PUSH_ZL; }

                    if (!MAV_inside(x, y, z)) {
                        SLONG ground = PAP_calc_map_height_at(x, z);

                        if (y <= ground + 0x240) {
                            // Fence detection: same height but can't walk through → push.
                            if (!(push & FC_PUSH_XS)) {
                                if (abs(PAP_calc_map_height_at(x - 0x100, z) - ground) <= 0x40) {
                                    if (!(MAV_get_caps(x >> 8, z >> 8, MAV_DIR_XS) & MAV_CAPS_GOTO)) {
                                        if (!there_is_a_los((x & 0xffffff00) + 128, y, z, (x & 0xffffff00) - 128, ground + 200, z, LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_UNDERGROUND_CHECK | LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG))
                                            push |= FC_PUSH_XS;
                                    }
                                }
                            }
                            if (!(push & FC_PUSH_XL)) {
                                if (abs(PAP_calc_map_height_at(x + 0x100, z) - ground) <= 0x40) {
                                    if (!(MAV_get_caps(x >> 8, z >> 8, MAV_DIR_XL) & MAV_CAPS_GOTO)) {
                                        if (!there_is_a_los((x & 0xffffff00) + 128, y, z, (x & 0xffffff00) + 256 + 128, ground + 200, z, LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_UNDERGROUND_CHECK | LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG))
                                            push |= FC_PUSH_XL;
                                    }
                                }
                            }
                            if (!(push & FC_PUSH_ZS)) {
                                if (abs(PAP_calc_map_height_at(x, z - 0x100) - ground) <= 0x40) {
                                    if (!(MAV_get_caps(x >> 8, z >> 8, MAV_DIR_ZS) & MAV_CAPS_GOTO)) {
                                        if (!there_is_a_los(x, y, (z & 0xffffff00) + 128, x, ground + 200, (z & 0xffffff00) - 128, LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_UNDERGROUND_CHECK | LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG))
                                            push |= FC_PUSH_ZS;
                                    }
                                }
                            }
                            if (!(push & FC_PUSH_ZL)) {
                                if (abs(PAP_calc_map_height_at(x, z + 0x100) - ground) <= 0x40) {
                                    if (!(MAV_get_caps(x >> 8, z >> 8, MAV_DIR_ZL) & MAV_CAPS_GOTO)) {
                                        if (!there_is_a_los(x, y, (z & 0xffffff00) + 128, x, ground + 200, (z & 0xffffff00) + 256 + 128, LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_UNDERGROUND_CHECK | LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG))
                                            push |= FC_PUSH_ZL;
                                    }
                                }
                            }
                        }
                    }

                    if (push & FC_PUSH_XS) { xforce += 0x100 - (x & 0xff); }
                    if (push & FC_PUSH_XL) { xforce -= (x & 0xff); }
                    if (push & FC_PUSH_ZS) { zforce += 0x100 - (z & 0xff); }
                    if (push & FC_PUSH_ZL) { zforce -= (z & 0xff); }

                    x += dx;
                    y += dy;
                    z += dz;
                }
            }

            fc->want_x += xforce << 4;
            fc->want_y += yforce << 4;
            fc->want_z += zforce << 4;
        }

        if (!fc->toonear) {
            if (fc->nobehind) {
                fc->nobehind -= 0x80 * TICK_RATIO >> TICK_SHIFT;
                if (fc->nobehind < 0) {
                    fc->nobehind = 0;
                }
            } else if (fc->focus->Class == CLASS_PERSON && fc->focus->Genus.Person->Mode == PERSON_MODE_FIGHT) {
                // Don't get behind fighting people.
            } else {
                // Get-behind: gently push camera to behind Darci.
                behind_x = fc->focus_x + (SIN(fc->focus_yaw) * ((fc->cam_dist * offset_dist) >> 8) >> 8);
                behind_z = fc->focus_z + (COS(fc->focus_yaw) * ((fc->cam_dist * offset_dist) >> 8) >> 8);

                dx = behind_x - fc->want_x;
                dz = behind_z - fc->want_z;

                if (fc->focus->SubState == SUB_STATE_ENTERING_VEHICLE) {
                    fc->want_x += dx >> 3;
                    fc->want_z += dz >> 3;
                } else if (fc->focus->Class == CLASS_PERSON && fc->focus->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                    fc->want_x += dx >> 5;
                    fc->want_z += dz >> 5;
                }

                if (xforce || zforce) {
                    // Stronger get-behind when near walls.
                    fc->want_x += dx >> 6;
                    fc->want_z += dz >> 6;
                    fc->want_x += dx >> 5;
                    fc->want_z += dz >> 5;

                    if (person_has_gun_out(fc->focus) && !(fc->focus->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING))) {
                        fc->want_x += dx >> 4;
                        fc->want_z += dz >> 4;
                    }
                } else {
                    fc->want_x += dx >> 3;
                    fc->want_z += dz >> 3;

                    if (person_has_gun_out(fc->focus) && !(fc->focus->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING))) {
                        fc->want_x += dx >> 4;
                        fc->want_z += dz >> 4;
                    }
                }
            }
        }

        if (!fc->toonear) {
            // Track the desired Y height above the focus character.
            SLONG goto_y = fc->focus_y + FC_focus_above(fc) + offset_height;

            if (GAME_FLAGS & GF_NO_FLOOR) {
                if (goto_y < 0x28000) {
                    goto_y = 0x28000;
                }
            }

            dy = goto_y - fc->want_y;

            if (dy > 0x10000) {
                // Well below target: move faster.
                dy >>= 3;
                if (dy > 0x2000) { dy = 0x2000; }
            } else {
                dy >>= 3;
                if (dy >  0x0d00) { dy =  0x0d00; }
                if (dy < -0x0c00) { dy = -0x0c00; }
            }

            if (dy > 0) {
                if (fc->focus->Class == CLASS_PERSON) {
                    Thing* p_person = fc->focus;

                    // Moving platform: follow the platform much faster.
                    if (p_person->OnFace > 0) {
                        ASSERT(WITHIN(p_person->OnFace, 1, next_prim_face4 - 1));

                        PrimFace4* f4 = &prim_faces4[p_person->OnFace];

                        ASSERT(f4->FaceFlags & FACE_FLAG_WALKABLE);

                        if (f4->FaceFlags & FACE_FLAG_WMOVE) {
                            dy <<= 5;
                        }
                    }
                }
            }

            fc->want_y += dy;
        }

        // Distance clamp: keep camera within [FC_DIST_MIN, FC_DIST_MAX] from focus XZ.
        dx = fc->focus_x - fc->want_x;
        dz = fc->focus_z - fc->want_z;

        dist = QDIST2(abs(dx), abs(dz));

        {
            // QDIST2 lacks accuracy; use integer sqrt instead.
            dist = (dx >> 8) * (dx >> 8) + (dz >> 8) * (dz >> 8);
            dist = Root(dist);
            dist <<= 8;
        }

        if (dist < FC_DIST_MIN) {
            if (!fc->toonear) {
                ddist = FC_DIST_MIN - dist;

                dx >>= 6;
                dy >>= 6;
                dz >>= 6;

                dist >>= 6;
                dist += 1;

                fc->want_x -= ddist * dx / dist;
                fc->want_z -= ddist * dz / dist;
            }
        } else if (dist > FC_DIST_MAX) {
            ddist = dist - FC_DIST_MAX;

            dx >>= 8;
            dz >>= 8;

            dist >>= 8;
            dist += 1;

            fc->want_x += ((ddist >> 4) * dx / dist) << 4;
            fc->want_z += ((ddist >> 4) * dz / dist) << 4;

            fc->toonear = UC_FALSE;
        }

        // Smooth position: want → actual.
        dx = fc->want_x - fc->x;
        dy = fc->want_y - fc->y;
        dz = fc->want_z - fc->z;

        if (QDIST3(abs(dx), abs(dy), abs(dz)) > 0x800) {
            fc->x += dx >> 2;
            fc->y += dy >> 3;
            fc->z += dz >> 2;
        } else {
            fc->want_x = fc->x;
            fc->want_y = fc->y;
            fc->want_z = fc->z;
            fc->smooth_transition = UC_FALSE;
        }

        FC_look_at_focus(fc);

        // Smooth angles: want → actual >>2.
        SLONG dyaw   = fc->want_yaw   - fc->yaw;
        SLONG dpitch = fc->want_pitch - fc->pitch;
        SLONG droll  = fc->want_roll  - fc->roll;

        dyaw   &= (2048 << 8) - 1;
        dpitch &= (2048 << 8) - 1;
        droll  &= (2048 << 8) - 1;

        if (dyaw   > (1024) << 8) { dyaw   -= (2048 << 8); }
        if (dpitch > (1024) << 8) { dpitch -= (2048 << 8); }
        if (droll  > (1024) << 8) { droll  -= (2048 << 8); }

        if (QDIST3(abs(dyaw), abs(dpitch), abs(droll)) > 0x180) {
            fc->yaw   += dyaw   >> 2;
            fc->pitch += dpitch >> 2;
            fc->roll  += droll  >> 2;
        } else {
            fc->want_yaw   = fc->yaw;
            fc->want_pitch = fc->pitch;
            fc->want_roll  = fc->roll;
        }

        // Fixed lens (FOV). Fight zoom is commented out in original.
        fc->lens = 0x28000 * CAM_MORE_IN;

        // Camera shake from explosions.
        if (fc->shake) {
            SLONG shake_x = (rand() % fc->shake) - (fc->shake >> 1);
            SLONG shake_y = (rand() % fc->shake) - (fc->shake >> 1);
            SLONG shake_z = (rand() % fc->shake) - (fc->shake >> 1);

            fc->x += shake_x << 7;
            fc->y += shake_y << 7;
            fc->z += shake_z << 7;

            fc->shake -= 1;
            fc->shake -= fc->shake >> 3;
        }
    }
}

// Fast LOS reject: traces from camera to a point through 5 steps.
// Returns UC_TRUE if the path is clear (none of the steps are inside geometry).
// uc_orig: FC_can_see_point (fallen/Source/fc.cpp)
SLONG FC_can_see_point(
    SLONG cam,
    SLONG x,
    SLONG y,
    SLONG z)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG i;

    SLONG dx = (fc->x >> 8) - x >> 3;
    SLONG dy = (fc->y >> 8) - y >> 3;
    SLONG dz = (fc->z >> 8) - z >> 3;

    for (i = 0; i < 5; i++) {
        x += dx;
        y += dy;
        z += dz;

        if (MAV_inside(x, y, z)) {
            return UC_FALSE;
        }
    }

    return UC_TRUE;
}

// Returns UC_TRUE if the camera can see the given person.
// Handles cut-scene camera position. Always returns UC_TRUE for climbers/danglers.
// Checks head, torso, and 4 shoulder points; UC_FALSE only if all 6 are obscured.
// uc_orig: FC_can_see_person (fallen/Source/fc.cpp)
SLONG FC_can_see_person(SLONG cam, Thing* p_person)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];

    SLONG x, y, z;
    SLONG old_fc_x = fc->x;
    SLONG old_fc_y = fc->y;
    SLONG old_fc_z = fc->z;
    SLONG junk;

    if (p_person->State == STATE_CLIMB_LADDER || p_person->State == STATE_DANGLING || p_person->State == STATE_CLIMBING) {
        return UC_TRUE;
    }

    // Get cut-scene camera position if active.
    EWAY_grab_camera(
        &fc->x, &fc->y, &fc->z,
        &junk, &junk, &junk, &junk);

    // Always visible if close enough.
    SLONG dx = abs(p_person->WorldPos.X - fc->x);
    SLONG dy = abs(p_person->WorldPos.Y - fc->y);
    SLONG dz = abs(p_person->WorldPos.Z - fc->z);
    SLONG dist = abs(dx) + abs(dy) + abs(dz);

    if (dist < 0x58000) {
        fc->x = old_fc_x;
        fc->y = old_fc_y;
        fc->z = old_fc_z;
        return UC_TRUE;
    }

    x = p_person->WorldPos.X >> 8;
    y = p_person->WorldPos.Y >> 8;
    z = p_person->WorldPos.Z >> 8;

    if (!FC_can_see_point(cam, x, y + FC_CSP_HEIGHT, z) && !FC_can_see_point(cam, x, y, z)) {
        if (!FC_can_see_point(cam, x + FC_CSP_RADIUS, y + FC_CSP_HEIGHT / 2, z) && !FC_can_see_point(cam, x - FC_CSP_RADIUS, y + FC_CSP_HEIGHT / 2, z) && !FC_can_see_point(cam, x, y + FC_CSP_HEIGHT / 2, z + FC_CSP_RADIUS) && !FC_can_see_point(cam, x, y + FC_CSP_HEIGHT / 2, z - FC_CSP_RADIUS)) {
            fc->x = old_fc_x;
            fc->y = old_fc_y;
            fc->z = old_fc_z;
            return UC_FALSE;
        }
    }

    fc->x = old_fc_x;
    fc->y = old_fc_y;
    fc->z = old_fc_z;

    return UC_TRUE;
}

// Sets up an over-the-shoulder view at the given pitch, entering toonear/first-person mode.
// toonear_dist=0x90000 is the sentinel value for this mode.
// uc_orig: FC_position_for_lookaround (fallen/Source/fc.cpp)
void FC_position_for_lookaround(SLONG cam, SLONG pitch)
{
    ASSERT(WITHIN(cam, 0, FC_MAX_CAMS - 1));

    FC_Cam* fc = &FC_cam[cam];
    SLONG vector[3];

    FMATRIX_vector(vector, fc->focus_yaw, pitch);

    fc->want_x = fc->focus_x + (vector[0] * 3 >> 2);
    fc->want_y = fc->focus_y + 0xb000 + (vector[1] * 3 >> 2);
    fc->want_z = fc->focus_z + (vector[2] * 3 >> 2);

    fc->toonear = UC_TRUE;
    fc->toonear_dist = 0x90000;
}

// Shakes each active camera proportionally to an explosion's proximity and force.
// Shake amount decays each frame in FC_process.
// uc_orig: FC_explosion (fallen/Source/fc.cpp)
void FC_explosion(SLONG x, SLONG y, SLONG z, SLONG force)
{
    SLONG i;
    SLONG dx, dy, dz;
    SLONG dist;
    SLONG shake;

    FC_Cam* fc;

    for (i = 0; i < FC_MAX_CAMS; i++) {
        fc = &FC_cam[i];

        dx = abs((fc->x >> 8) - x);
        dy = abs((fc->y >> 8) - y);
        dz = abs((fc->z >> 8) - z);

        dist = QDIST3(dx, dy, dz);

        if (dist < force * 16) {
            shake = 256 * ((force * 16) - dist) / (force * 16);
            SATURATE(shake, 0, 255);
            fc->shake = shake;
        }
    }
}

// Freezes camera behind the player and activates cut-scene mode (death camera).
// uc_orig: FC_kill_player_cam (fallen/Source/fc.cpp)
void FC_kill_player_cam(Thing* p_thing)
{
    FC_cam[0].focus = p_thing;
    FC_force_camera_behind(0);
    GAME_cut_scene = 1;
}
