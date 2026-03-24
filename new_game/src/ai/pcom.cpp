// NPC AI system — Person COMportment (PCOM).
// First chunk: globals, timing utilities, helper queries, navigation helpers,
// gang management, alert propagation.
// See pcom.h for high-level architecture notes.

#include "ai/pcom.h"
#include "ai/pcom_globals.h"
#include "ai/mav.h"
#include "ai/combat.h"
#include "ai/combat_globals.h"
#include "missions/eway.h"
#include "world/level_pools.h"
#include "world/environment/prim.h"    // get_prim_info
#include "engine/physics/collide.h"
#include "world/map/road.h"
#include "world/map/road_globals.h"
#include "world/navigation/wand.h"
#include "world/environment/ware.h"
#include "world/environment/ware_globals.h"
#include "world/map/ob.h"
#include "world/map/ob_globals.h"
#include "actors/characters/anim_ids.h"
#include "actors/core/statedef.h"
#include "actors/characters/person.h"   // set_person_idle, set_person_goto_xz, set_person_circle, etc.
#include "engine/audio/mfx.h"
#include "engine/audio/sound.h"
#include "assets/sound_id.h"
#include "effects/spark.h"
#include "actors/items/balloon.h"
#include "actors/items/balloon_globals.h"
#include "ui/menus/cnet.h"
#include "ui/menus/cnet_globals.h"
#include "ui/hud/overlay.h"
#include "actors/vehicles/vehicle.h"
#include "actors/vehicles/vehicle_globals.h"
#include "engine/graphics/pipeline/aeng.h"
#include "actors/core/interact.h"
#include "actors/core/thing.h"
#include "actors/items/special.h"

// --- Internal movement state constants (file-local) ---

// uc_orig: PCOM_MOVE_STATE_PLAYER (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_PLAYER 0
// uc_orig: PCOM_MOVE_STATE_STILL (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_STILL 1
// uc_orig: PCOM_MOVE_STATE_GOTO_XZ (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_GOTO_XZ 2
// uc_orig: PCOM_MOVE_STATE_GOTO_WAYPOINT (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_GOTO_WAYPOINT 3
// uc_orig: PCOM_MOVE_STATE_GOTO_THING (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_GOTO_THING 4
// uc_orig: PCOM_MOVE_STATE_PAUSE (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_PAUSE 5
// uc_orig: PCOM_MOVE_STATE_ANIMATION (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_ANIMATION 6
// uc_orig: PCOM_MOVE_STATE_CIRCLE (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_CIRCLE 7
// uc_orig: PCOM_MOVE_STATE_DRIVETO (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_DRIVETO 8
// uc_orig: PCOM_MOVE_STATE_FOLLOW (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_FOLLOW 9
// uc_orig: PCOM_MOVE_STATE_PARK_CAR (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_PARK_CAR 10
// uc_orig: PCOM_MOVE_STATE_DRIVE_DOWN (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_DRIVE_DOWN 11
// uc_orig: PCOM_MOVE_STATE_BIKETO (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_BIKETO 12
// uc_orig: PCOM_MOVE_STATE_PARK_BIKE (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_PARK_BIKE 13
// uc_orig: PCOM_MOVE_STATE_BIKE_DOWN (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_BIKE_DOWN 14
// uc_orig: PCOM_MOVE_STATE_GRAPPLE (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_GRAPPLE 15
// uc_orig: PCOM_MOVE_STATE_GOTO_THING_SLIDE (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_GOTO_THING_SLIDE 16
// uc_orig: PCOM_MOVE_STATE_WAIT_CIRCLE (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_WAIT_CIRCLE 17
// uc_orig: PCOM_MOVE_STATE_PARK_CAR_ON_ROAD (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_PARK_CAR_ON_ROAD 18
// uc_orig: PCOM_MOVE_STATE_NUMBER (fallen/Source/pcom.cpp)
#define PCOM_MOVE_STATE_NUMBER 19

// Speed aliases — map to Person speed constants.
// uc_orig: PCOM_MOVE_SPEED_WALK (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SPEED_WALK PERSON_SPEED_WALK
// uc_orig: PCOM_MOVE_SPEED_RUN (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SPEED_RUN PERSON_SPEED_RUN
// uc_orig: PCOM_MOVE_SPEED_SNEAK (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SPEED_SNEAK PERSON_SPEED_SNEAK
// uc_orig: PCOM_MOVE_SPEED_YOMP (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SPEED_YOMP PERSON_SPEED_YOMP
// uc_orig: PCOM_MOVE_SPEED_SPRINT (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SPEED_SPRINT PERSON_SPEED_SPRINT

// Move sub-states — finer granularity within a move_state.
// uc_orig: PCOM_MOVE_SUBSTATE_NONE (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_NONE 0
// uc_orig: PCOM_MOVE_SUBSTATE_GOTO (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_GOTO 1
// uc_orig: PCOM_MOVE_SUBSTATE_ACTION (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_ACTION 2
// uc_orig: PCOM_MOVE_SUBSTATE_GUNAWAY (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_GUNAWAY 3
// uc_orig: PCOM_MOVE_SUBSTATE_GUNOUT (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_GUNOUT 4
// uc_orig: PCOM_MOVE_SUBSTATE_PUNCH (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_PUNCH 5
// uc_orig: PCOM_MOVE_SUBSTATE_KICK (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_KICK 6
// uc_orig: PCOM_MOVE_SUBSTATE_SHOOT (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_SHOOT 7
// uc_orig: PCOM_MOVE_SUBSTATE_ANIM (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_ANIM 8
// uc_orig: PCOM_MOVE_SUBSTATE_GETINCAR (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_GETINCAR 9
// uc_orig: PCOM_MOVE_SUBSTATE_3PTURN (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_3PTURN 10
// uc_orig: PCOM_MOVE_SUBSTATE_WAIT (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_WAIT 11
// uc_orig: PCOM_MOVE_SUBSTATE_LEAVECAR (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_LEAVECAR 12
// uc_orig: PCOM_MOVE_SUBSTATE_ARREST (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_ARREST 13
// uc_orig: PCOM_MOVE_SUBSTATE_LOSMAV (fallen/Source/pcom.cpp)
#define PCOM_MOVE_SUBSTATE_LOSMAV 14

// Avoidance flags packed in move_arg high bits.
// uc_orig: PCOM_MOVE_FLAG_AVOID_LEFT (fallen/Source/pcom.cpp)
#define PCOM_MOVE_FLAG_AVOID_LEFT (1 << 0)
// uc_orig: PCOM_MOVE_FLAG_AVOID_RIGHT (fallen/Source/pcom.cpp)
#define PCOM_MOVE_FLAG_AVOID_RIGHT (1 << 1)

// What to do when leaving a car (excar state).
// uc_orig: PCOM_EXCAR_NORMAL (fallen/Source/pcom.cpp)
#define PCOM_EXCAR_NORMAL 0
// uc_orig: PCOM_EXCAR_FLEE_PERSON (fallen/Source/pcom.cpp)
#define PCOM_EXCAR_FLEE_PERSON 1
// uc_orig: PCOM_EXCAR_ARREST_PERSON (fallen/Source/pcom.cpp)
#define PCOM_EXCAR_ARREST_PERSON 2
// uc_orig: PCOM_EXCAR_NAVTOKILL (fallen/Source/pcom.cpp)
#define PCOM_EXCAR_NAVTOKILL 3

// Arrival threshold: distance in world units to count as "reached".
// uc_orig: PCOM_ARRIVE_DIST (fallen/Source/pcom.cpp)
#define PCOM_ARRIVE_DIST (0x40)

// --- Forward declarations (internal) ---

// uc_orig: PCOM_set_person_ai_flee_person (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_flee_person(Thing* p_person, Thing* p_scary);

// uc_orig: PCOM_set_person_ai_kill_person (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_kill_person(Thing* p_person, Thing* p_target, SLONG alert_gang);

// uc_orig: PCOM_set_person_ai_homesick (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_homesick(Thing* p_person);

// uc_orig: person_holding_2handed (fallen/Source/pcom.cpp)
SLONG person_holding_2handed(Thing* p_person);

// uc_orig: am_i_a_thug (fallen/Source/pcom.cpp)  [defined in Person.cpp]
extern SLONG am_i_a_thug(Thing* p_person);

// uc_orig: is_person_dead (fallen/Source/pcom.cpp)
extern SLONG is_person_dead(Thing* p_person);

// uc_orig: is_person_ko (fallen/Source/pcom.cpp)
extern SLONG is_person_ko(Thing* p_person);

// uc_orig: stealth_debug (fallen/Source/pcom.cpp)
extern UBYTE stealth_debug;

// uc_orig: combo_display (fallen/Source/pcom.cpp)
extern UBYTE combo_display;

// uc_orig: allow_debug_keys (fallen/Source/pcom.cpp)
extern BOOL allow_debug_keys;

// uc_orig: vehicle_random (fallen/Source/pcom.cpp)
extern UBYTE vehicle_random[];

// uc_orig: there_is_a_los_mav (fallen/Source/pcom.cpp)  [from collide.cpp]
extern SLONG there_is_a_los_mav(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);

// uc_orig: people_allowed_to_hit_each_other (fallen/Source/pcom.cpp)
extern SLONG people_allowed_to_hit_each_other(Thing* p_victim, Thing* p_agressor);

// uc_orig: person_normal_animate (fallen/Source/pcom.cpp)
extern SLONG person_normal_animate(Thing* p_person);

// uc_orig: dist_to_target_pelvis (fallen/Source/pcom.cpp)
extern SLONG dist_to_target_pelvis(Thing* p_person_a, Thing* p_person_b);

// uc_orig: set_person_recircle (fallen/Source/pcom.cpp)
extern void set_person_recircle(Thing* p_person);

// uc_orig: FC_kill_player_cam (fallen/Source/pcom.cpp)
extern void FC_kill_player_cam(Thing* p_thing);

// uc_orig: person_has_gun_out (fallen/Source/pcom.cpp)
extern SLONG person_has_gun_out(Thing* p_person);

// uc_orig: is_person_guilty (fallen/Source/pcom.cpp)
extern SLONG is_person_guilty(Thing* p_person);

// uc_orig: GAME_cut_scene (fallen/Source/pcom.cpp)
extern UBYTE GAME_cut_scene;

// uc_orig: timer_bored (fallen/Source/pcom.cpp)
extern ULONG timer_bored;

// uc_orig: IsEnglish (fallen/Source/frontend.cpp)
extern UBYTE IsEnglish;

// Used by PCOM_process_findcar and PCOM_process_hitch — defined in Vehicle.cpp.
extern SLONG in_right_place_for_car(Thing* p_person, Thing* p_vehicle, SLONG* door);

// uc_orig: push_into_attack_group_at_angle (fallen/Source/pcom.cpp)
void push_into_attack_group_at_angle(Thing* p_person, SLONG gang, SLONG reqd_angle);

// uc_orig: remove_from_gang_attack (fallen/Source/pcom.cpp)
SLONG remove_from_gang_attack(Thing* p_person, Thing* p_target);

// uc_orig: DriveCar (fallen/Source/pcom.cpp)
void DriveCar(Thing* p_person);
// uc_orig: ParkCar (fallen/Source/pcom.cpp)
void ParkCar(Thing* p_person);
// uc_orig: DriveBike (fallen/Source/pcom.cpp)
void DriveBike(Thing* p_person);
// uc_orig: ParkBike (fallen/Source/pcom.cpp)
void ParkBike(Thing* p_person);

// ============================================================
// Initialisation
// ============================================================

// Reset gang tracking structures at level load.
// uc_orig: PCOM_init (fallen/Source/pcom.cpp)
void PCOM_init(void)
{
    memset(PCOM_gang, 0, sizeof(PCOM_Gang) * PCOM_MAX_GANGS);
    PCOM_gang_person_upto = 0;
}

// Register an NPC into a gang slot. Maintains the sorted gang_person list.
// Civilians cannot be gang members (asserted).
// uc_orig: PCOM_add_gang_member (fallen/Source/pcom.cpp)
void PCOM_add_gang_member(THING_INDEX person, UBYTE group)
{
    SLONG i;
    PCOM_Gang* pg;

    ASSERT(WITHIN(group, 0, PCOM_MAX_GANGS - 1));
    ASSERT(WITHIN(PCOM_gang_person_upto, 0, PCOM_MAX_GANG_PEOPLE - 1));
    if (PCOM_gang_person_upto >= PCOM_MAX_GANG_PEOPLE) {
        return;
    }

    // Mike added this assert, and it shouldn't be here, apparently.
    // if(TO_THING(person)->Genus.Person->PersonType==PERSON_COP)
    //     ASSERT(0);

    if (TO_THING(person)->Genus.Person->PersonType == PERSON_CIV)
        ASSERT(0);

    pg = &PCOM_gang[group];

    if (pg->index + pg->number == PCOM_gang_person_upto) {
        // The gang is at the end of the list — no shifting needed.
        PCOM_gang_person[PCOM_gang_person_upto] = person;
        pg->number += 1;
        PCOM_gang_person_upto += 1;
    } else {
        // Shift all entries after this gang's slot to make room.
        for (i = PCOM_gang_person_upto - 1; i >= pg->index + pg->number; i--) {
            PCOM_gang_person[i + 1] = PCOM_gang_person[i];
        }

        // Fix up index fields for all gangs that sit after the insertion point.
        for (i = 0; i < PCOM_MAX_GANGS; i++) {
            if (i == group) {
                continue;
            }
            if (PCOM_gang[i].index >= pg->index + pg->number) {
                PCOM_gang[i].index += 1;
            }
        }

        PCOM_gang_person[pg->index + pg->number] = person;
        pg->number += 1;
        PCOM_gang_person_upto += 1;
    }
}

// Decide if a regen'd fake wanderer should turn into a gang attacker targeting Darci.
// Checks: not in warehouse, no cutscene, has line of sight, no other thugs nearby.
// uc_orig: PCOM_should_fake_person_attack_darci (fallen/Source/pcom.cpp)
SLONG PCOM_should_fake_person_attack_darci(Thing* p_person)
{
    Thing* darci = NET_PERSON(0);

    if (darci->Genus.Person->Ware) {
        // Don't attack Darci when she's in a warehouse.
        return UC_FALSE;
    }

    if (EWAY_stop_player_moving()) {
        // In a cutscene.
        return UC_FALSE;
    }

    if (p_person->Genus.Person->PersonType == PERSON_COP) {
        // Cops always attack the thugs!
        return UC_TRUE;
    }

    PCOM_found_num = THING_find_sphere(
        darci->WorldPos.X >> 8,
        darci->WorldPos.Y >> 8,
        darci->WorldPos.Z >> 8,
        0x600,
        PCOM_found,
        PCOM_MAX_FIND,
        1 << CLASS_PERSON);

    SLONG i;
    Thing* p_found;

    for (i = 0; i < PCOM_found_num; i++) {
        p_found = TO_THING(PCOM_found[i]);

        if (p_found->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER) {
            continue;
        }

        switch (p_found->Genus.Person->PersonType) {
        case PERSON_THUG_RASTA:
        case PERSON_THUG_GREY:
        case PERSON_THUG_RED:
        case PERSON_MIB1:
        case PERSON_MIB2:
        case PERSON_MIB3:
            return UC_FALSE;
        }
    }

    return UC_TRUE;
}

// Returns ticks between shots for this person.
// MIBs have a built-in AK47 rate=20. Others depend on equipped weapon type.
// uc_orig: get_rate_of_fire (fallen/Source/pcom.cpp)
SLONG get_rate_of_fire(Thing* p_person)
{
    Thing* p_special;

    if (p_person->Genus.Person->PersonType == PERSON_MIB1 || p_person->Genus.Person->PersonType == PERSON_MIB2 || p_person->Genus.Person->PersonType == PERSON_MIB3) {
        // MIB are ninja shooting machines with built-in AK47s.
        return 20;
    }

    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        return 20;
    } else if (p_person->Genus.Person->SpecialUse) {
        p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_SHOTGUN:
            return 30;
        case SPECIAL_AK47:
            return 25;
        case SPECIAL_GRENADE:
            return 20;

        default:
            // This isn't a weapon you shoot!
            return 0;
        }
    } else {
        return 0; // not a gun
    }
}

// Returns SPECIAL_GUN/SPECIAL_GRENADE if weapon is drawn, 0 otherwise.
// uc_orig: person_has_gun_or_grenade_out (fallen/Source/pcom.cpp)
SLONG person_has_gun_or_grenade_out(Thing* p_person)
{
    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        return (SPECIAL_GUN);
    }

    if (!p_person->Genus.Person->SpecialUse) {
        return UC_FALSE;
    }

    {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        if (p_special->Genus.Special->SpecialType == SPECIAL_SHOTGUN || p_special->Genus.Special->SpecialType == SPECIAL_AK47 || p_special->Genus.Special->SpecialType == SPECIAL_GRENADE) {
            return (p_special->Genus.Special->SpecialType);
        }
    }

    return UC_FALSE;
}

// Returns a random tick-count in [min, max] (tenths of a second), converted to ticks.
// uc_orig: PCOM_get_random_duration (fallen/Source/pcom.cpp)
SLONG PCOM_get_random_duration(SLONG min, SLONG max)
{
    SLONG ans;

    if (max <= min) {
        return min;
    }

    min = min * (PCOM_TICKS_PER_SEC / 10);
    max = max * (PCOM_TICKS_PER_SEC / 10);

    ans = min + Random() % (max - min);

    return ans;
}

// Returns the compass angle for a MAV_DIR value (0..3 = N/S/E/W).
// uc_orig: PCOM_get_angle_for_dir (fallen/Source/pcom.cpp)
UWORD PCOM_get_angle_for_dir(SLONG dir)
{
    static UWORD dir_to_angle[4] = {
        512,
        512 + 1024,
        0,
        1024
    };

    SLONG angle;

    ASSERT(WITHIN(dir, 0, 3));

    angle = dir_to_angle[dir];

    return angle;
}

// Returns a unit vector for the given MAV_DIR.
// uc_orig: PCOM_get_dx_dz_for_dir (fallen/Source/pcom.cpp)
void PCOM_get_dx_dz_for_dir(SLONG dir, SLONG* dx, SLONG* dz)
{
    SLONG ans_x;
    SLONG ans_z;

    SLONG angle = PCOM_get_angle_for_dir(dir);

    ans_x = COS(angle) >> 11;
    ans_z = SIN(angle) >> 11;

    *dx = ans_x;
    *dz = ans_z;
}

// Pelvis-to-pelvis 3D distance with a vertical penalty.
// If vertical gap > 0x80, add 4x Y component — this prevents NPCs from targeting
// enemies through floors in multi-storey buildings.
// uc_orig: PCOM_get_dist_between (fallen/Source/pcom.cpp)
SLONG PCOM_get_dist_between(Thing* p_person_a, Thing* p_person_b)
{
    SLONG ax;
    SLONG ay;
    SLONG az;

    if (p_person_a->Class == CLASS_PERSON) {
        calc_sub_objects_position(
            p_person_a,
            p_person_a->Draw.Tweened->AnimTween,
            SUB_OBJECT_PELVIS,
            &ax,
            &ay,
            &az);
    } else {
        ax = 0;
        ay = 0;
        az = 0;
    }

    ax += p_person_a->WorldPos.X >> 8;
    ay += p_person_a->WorldPos.Y >> 8;
    az += p_person_a->WorldPos.Z >> 8;

    SLONG bx;
    SLONG by;
    SLONG bz;

    if (p_person_b->Class == CLASS_PERSON) {
        calc_sub_objects_position(
            p_person_b,
            p_person_b->Draw.Tweened->AnimTween,
            SUB_OBJECT_PELVIS,
            &bx,
            &by,
            &bz);
    } else {
        bx = 0;
        by = 0;
        bz = 0;
    }

    bx += p_person_b->WorldPos.X >> 8;
    by += p_person_b->WorldPos.Y >> 8;
    bz += p_person_b->WorldPos.Z >> 8;

    SLONG dx = abs(ax - bx);
    SLONG dy = abs(ay - by);
    SLONG dz = abs(az - bz);

    SLONG dist = QDIST2(dx, dz);

    if (dy >= 0x80) {
        // Must be on a different level — counts as very far away.
        dist += dy << 2;
    }

    return dist;
}

// Returns distance from person's current position to their home square.
// uc_orig: PCOM_get_dist_from_home (fallen/Source/pcom.cpp)
SLONG PCOM_get_dist_from_home(Thing* p_person)
{
    SLONG home_x = (p_person->Genus.Person->HomeX << 0);
    SLONG home_z = (p_person->Genus.Person->HomeZ << 0);

    SLONG dx = abs((p_person->WorldPos.X >> 8) - home_x);
    SLONG dz = abs((p_person->WorldPos.Z >> 8) - home_z);

    SLONG dist = QDIST2(dx, dz);

    return dist;
}

// Decide whether LOS-MAV (direct tracking) is better than full pathfinding.
// Returns UC_FALSE if target is climbing/jumping/dangling, or too far, or in different warehouse zone.
// uc_orig: PCOM_should_i_try_to_los_mav_to_person (fallen/Source/pcom.cpp)
SLONG PCOM_should_i_try_to_los_mav_to_person(Thing* p_person, Thing* p_target)
{
    if (p_target->Class == CLASS_PERSON) {
        if (p_target->State == STATE_CLIMB_LADDER || p_target->State == STATE_CLIMBING || p_target->State == STATE_DANGLING || p_target->State == STATE_JUMPING) {
            return UC_FALSE;
        }

        SLONG dx = abs(p_target->WorldPos.X - p_person->WorldPos.X);
        SLONG dy = abs(p_target->WorldPos.Y - p_person->WorldPos.Y);
        SLONG dz = abs(p_target->WorldPos.Z - p_person->WorldPos.Z);

        if (dx < 0x30000 && dz < 0x30000 && dy < 0x10000) {
            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// Returns the map-square nav target for reaching a person.
// Adjusts for knocked-out/climbing states so the pathfinder aims at the correct square.
// uc_orig: PCOM_get_person_navsquare (fallen/Source/pcom.cpp)
void PCOM_get_person_navsquare(
    Thing* p_person,
    SLONG* map_dest_x,
    SLONG* map_dest_z)
{
    ASSERT(p_person->Class == CLASS_PERSON);

    SLONG ans_x = p_person->WorldPos.X;
    SLONG ans_z = p_person->WorldPos.Z;

    if (p_person->State == STATE_CLIMB_LADDER || p_person->State == STATE_CLIMBING || p_person->State == STATE_DANGLING || p_person->State == STATE_JUMPING) {
        if (p_person->State == STATE_CLIMBING) {
            ASSERT(WITHIN(p_person->Genus.Person->OnFacet, 1, next_dfacet - 1));

            if (dfacets[p_person->Genus.Person->OnFacet].FacetType == STOREY_TYPE_FENCE_BRICK) {
                // This person won't be able to climb over the fence.
                goto dont_mav_in_front_of_the_person;
            }
        }

        // Navigate to the square in front of the person.
        SLONG dx = SIN(p_person->Draw.Tweened->Angle);
        SLONG dz = COS(p_person->Draw.Tweened->Angle);

        ans_x -= dx;
        ans_z -= dz;

        // Two squares ahead for jumping.
        if (p_person->State == STATE_JUMPING) {
            ans_x -= dx;
            ans_z -= dz;
        }
    } else if (is_person_ko(p_person)) {
        SLONG pelvis_x;
        SLONG pelvis_y;
        SLONG pelvis_z;

        // Go to the pelvis position.
        calc_sub_objects_position(
            p_person,
            p_person->Draw.Tweened->AnimTween,
            SUB_OBJECT_PELVIS,
            &pelvis_x,
            &pelvis_y,
            &pelvis_z);

        pelvis_x <<= 8;
        pelvis_z <<= 8;

        pelvis_x += p_person->WorldPos.X;
        pelvis_z += p_person->WorldPos.Z;

        ans_x = pelvis_x;
        ans_z = pelvis_z;
    }

dont_mav_in_front_of_the_person:;

    *map_dest_x = ans_x >> 16;
    *map_dest_z = ans_z >> 16;
}

// Returns the nav target near the car door closest to the approaching person.
// uc_orig: PCOM_get_vehicle_navsquare (fallen/Source/pcom.cpp)
void PCOM_get_vehicle_navsquare(
    Thing* p_vehicle,
    SLONG* map_dest_x,
    SLONG* map_dest_z,
    SLONG i_am_a_passenger,
    Thing* p_person)
{
    SLONG dx;
    SLONG dz;

    SLONG ix1;
    SLONG iz1;
    SLONG dist1;

    SLONG ix2;
    SLONG iz2;
    SLONG dist2;

    // uc_orig: get_car_enter_xz (fallen/Source/pcom.cpp)
    extern void get_car_enter_xz(Thing * p_vehicle, SLONG door, SLONG * cx, SLONG * cz);

    get_car_enter_xz(p_vehicle, 0, &ix1, &iz1);

    dx = abs((p_person->WorldPos.X >> 8) - ix1);
    dz = abs((p_person->WorldPos.Z >> 8) - iz1);

    dist1 = QDIST2(dx, dz);

    get_car_enter_xz(p_vehicle, 1, &ix2, &iz2);

    dx = abs((p_person->WorldPos.X >> 8) - ix2);
    dz = abs((p_person->WorldPos.Z >> 8) - iz2);

    dist2 = QDIST2(dx, dz);

    if (dist1 < dist2) {
        *map_dest_x = ix1 >> 8;
        *map_dest_z = iz1 >> 8;
    } else {
        *map_dest_x = ix2 >> 8;
        *map_dest_z = iz2 >> 8;
    }
}

// Teleports or verifies proximity to a seat prim. Returns UC_FALSE if too far and dont_teleport set.
// uc_orig: PCOM_position_person_to_sit_on_prim (fallen/Source/pcom.cpp)
SLONG PCOM_position_person_to_sit_on_prim(
    Thing* p_person,
    SLONG prim,
    SLONG prim_x,
    SLONG prim_y,
    SLONG prim_z,
    SLONG prim_yaw,
    SLONG dont_teleport)
{
    ASSERT(p_person->Class == CLASS_PERSON);

    SLONG dx;
    SLONG dz;
    SLONG away;

    PrimInfo* pi = get_prim_info(prim);

    GameCoord newpos;

    dx = SIN(prim_yaw & 2047) >> 8;
    dz = COS(prim_yaw & 2047) >> 8;

    p_person->Draw.Tweened->Angle = prim_yaw & 2047;

    away = pi->minz;
    away -= 0x20;

    newpos.X = (prim_x << 8) + dx * away;
    newpos.Z = (prim_z << 8) + dz * away;

    if (dont_teleport) {
        dx = abs(newpos.X - p_person->WorldPos.X);
        dz = abs(newpos.Z - p_person->WorldPos.Z);

        if (dx + dz > 0x5000) {
            return UC_FALSE;
        }
    }

    newpos.Y = prim_y << 8;

    move_thing_on_map(p_person, &newpos);

    return UC_TRUE;
}

// Returns the world position to flee away from.
// If fleeing a person it returns that person's live position, else stored flee-place coords.
// uc_orig: PCOM_get_flee_from_pos (fallen/Source/pcom.cpp)
void PCOM_get_flee_from_pos(
    Thing* p_person,
    SLONG* from_x,
    SLONG* from_z)
{
    SLONG ans_x;
    SLONG ans_z;

    switch (p_person->Genus.Person->pcom_ai_state) {
    case PCOM_AI_STATE_FLEE_PLACE:
        ans_x = (((p_person->Genus.Person->pcom_ai_arg >> 8) & 0xff) << 8) + 0x80;
        ans_z = (((p_person->Genus.Person->pcom_ai_arg >> 0) & 0xff) << 8) + 0x80;
        break;

    case PCOM_AI_STATE_FLEE_PERSON:
        ans_x = TO_THING(p_person->Genus.Person->pcom_ai_arg)->WorldPos.X >> 8;
        ans_z = TO_THING(p_person->Genus.Person->pcom_ai_arg)->WorldPos.Z >> 8;
        break;

    default:
        ASSERT(0);
        break;
    }

    *from_x = ans_x;
    *from_z = ans_z;
}

// Resolves the current navigation destination depending on move_state.
// uc_orig: PCOM_get_person_dest (fallen/Source/pcom.cpp)
void PCOM_get_person_dest(
    Thing* p_person,
    SLONG* dest_x,
    SLONG* dest_z)
{
    SLONG ans_x;
    SLONG ans_y;
    SLONG ans_z;

    Thing* p_thing;

    ans_x = p_person->WorldPos.X >> 8;
    ans_z = p_person->WorldPos.Z >> 8;

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:
        break;

    case PCOM_MOVE_STATE_GOTO_XZ:

        ans_x = (p_person->Genus.Person->pcom_move_arg >> 8) & 0xff;
        ans_z = (p_person->Genus.Person->pcom_move_arg >> 0) & 0xff;

        ans_x <<= 8;
        ans_z <<= 8;

        ans_x += 0x80;
        ans_z += 0x80;

        break;

    case PCOM_MOVE_STATE_GOTO_WAYPOINT:

        EWAY_get_position(
            p_person->Genus.Person->pcom_move_arg,
            &ans_x,
            &ans_y,
            &ans_z);

        break;

    case PCOM_MOVE_STATE_GOTO_THING:

        p_thing = TO_THING(p_person->Genus.Person->pcom_move_arg);

        switch (p_thing->Class) {
        case CLASS_PERSON:

            PCOM_get_person_navsquare(
                p_thing,
                &ans_x,
                &ans_z);

            break;

        case CLASS_VEHICLE:

            PCOM_get_vehicle_navsquare(
                p_thing,
                &ans_x,
                &ans_z,
                p_thing->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HITCH,
                p_person);

            break;

        default:

            ans_x = p_thing->WorldPos.X >> 16;
            ans_z = p_thing->WorldPos.Z >> 16;

            break;
        }

        ans_x <<= 8;
        ans_z <<= 8;

        ans_x += 0x80;
        ans_z += 0x80;

        break;

    case PCOM_MOVE_STATE_PAUSE:
        break;

    case PCOM_MOVE_STATE_DRIVETO:
    case PCOM_MOVE_STATE_BIKETO:

        EWAY_get_position(
            p_person->Genus.Person->pcom_move_arg,
            &ans_x,
            &ans_y,
            &ans_z);

        break;

    case PCOM_MOVE_STATE_DRIVE_DOWN:
    case PCOM_MOVE_STATE_BIKE_DOWN:
    case PCOM_MOVE_STATE_PARK_CAR_ON_ROAD:

        ROAD_get_dest(
            p_person->Genus.Person->pcom_move_arg >> 8,
            p_person->Genus.Person->pcom_move_arg & 0xff,
            &ans_x,
            &ans_z);

        break;

    case PCOM_MOVE_STATE_PARK_CAR:
    case PCOM_MOVE_STATE_PARK_BIKE:
        // Trying to stop.
        break;

    default:
        ASSERT(0);
        break;
    }

    *dest_x = ans_x;
    *dest_z = ans_z;

    return;
}

// Returns the world position corresponding to the current MAV_Action target.
// uc_orig: PCOM_get_mav_action_pos (fallen/Source/pcom.cpp)
void PCOM_get_mav_action_pos(
    Thing* p_person,
    SLONG* dest_x,
    SLONG* dest_z)
{
    ASSERT(p_person->Genus.Person->pcom_move_substate != PCOM_MOVE_SUBSTATE_LOSMAV);

    SLONG dx;
    SLONG dz;

    SLONG ans_x = (p_person->Genus.Person->pcom_move_ma.dest_x << 8) + 0x80;
    SLONG ans_z = (p_person->Genus.Person->pcom_move_ma.dest_z << 8) + 0x80;

    if (p_person->Genus.Person->pcom_move_ma.action != MAV_ACTION_GOTO) {
        // Take the direction of the MAV_Action into account.
        PCOM_get_dx_dz_for_dir(
            p_person->Genus.Person->MA.dir,
            &dx,
            &dz);

        ans_x += dx;
        ans_z += dz;
    }

    *dest_x = ans_x;
    *dest_z = ans_z;
}

// Returns UC_TRUE if the person is carrying any gun (including MIBs built-in AK47).
// uc_orig: PCOM_person_has_any_sort_of_gun (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_any_sort_of_gun(Thing* p_person)
{
    if (p_person->Flags & FLAGS_HAS_GUN) {
        return UC_TRUE;
    }

    if (person_has_special(p_person, SPECIAL_SHOTGUN) || person_has_special(p_person, SPECIAL_AK47) || person_has_special(p_person, SPECIAL_GRENADE)) {
        return UC_TRUE;
    }

    if (p_person->Genus.Person->PersonType == PERSON_MIB1 || p_person->Genus.Person->PersonType == PERSON_MIB2 || p_person->Genus.Person->PersonType == PERSON_MIB3) {
        return UC_TRUE;
    }

    return UC_FALSE;
}

// uc_orig: PCOM_person_has_any_sort_of_h2h (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_any_sort_of_h2h(Thing* p_person)
{
    if (person_has_special(p_person, SPECIAL_KNIFE)) {
        return (SPECIAL_KNIFE);
    } else if (person_has_special(p_person, SPECIAL_BASEBALLBAT)) {
        return (SPECIAL_BASEBALLBAT);
    } else {
        return (0);
    }
}

// uc_orig: PCOM_person_has_any_sort_of_gun_with_ammo (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_any_sort_of_gun_with_ammo(Thing* p_person)
{
    Thing* p_special;

    if (p_person->Flags & FLAGS_HAS_GUN) {
        if (p_person->Genus.Person->Ammo) {
            return UC_TRUE;
        }
    }

    if (p_person->Genus.Person->PersonType == PERSON_MIB1 || p_person->Genus.Person->PersonType == PERSON_MIB2 || p_person->Genus.Person->PersonType == PERSON_MIB3) {
        return UC_TRUE;
    }

    if ((p_special = person_has_special(p_person, SPECIAL_SHOTGUN)) || (p_special = person_has_special(p_person, SPECIAL_AK47)) || (p_special = person_has_special(p_person, SPECIAL_GRENADE))) {
        if (p_special->Genus.Special->ammo) {
            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// Scan nearby persons in FINDCAR state targeting this vehicle.
// Used to avoid moving the car while NPCs approach it.
// uc_orig: PCOM_are_there_people_who_want_to_enter (fallen/Source/pcom.cpp)
SLONG PCOM_are_there_people_who_want_to_enter(Thing* p_vehicle)
{
    SLONG i;
    SLONG num_found;
    Thing* p_found;

    num_found = THING_find_sphere(
        p_vehicle->WorldPos.X >> 8,
        p_vehicle->WorldPos.Y >> 8,
        p_vehicle->WorldPos.Z >> 8,
        0x400,
        THING_array,
        THING_ARRAY_SIZE,
        1 << CLASS_PERSON);

    for (i = 0; i < num_found; i++) {
        p_found = TO_THING(THING_array[i]);

        if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FINDCAR || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HITCH) {
            Thing* p_vehin = TO_THING(p_found->Genus.Person->pcom_ai_arg);

            if (p_vehin == p_vehicle) {
                return UC_TRUE;
            }
        }
    }

    return UC_FALSE;
}

// Returns UC_TRUE if this NPC can be interrupted by new AI stimuli.
// Returns UC_FALSE while dying, dead, being carried, climbing, jumping, in a vehicle action, or mid-arrest crouch.
// uc_orig: PCOM_person_doing_nothing_important (fallen/Source/pcom.cpp)
SLONG PCOM_person_doing_nothing_important(Thing* p_person)
{
    if (p_person->State == STATE_DYING || p_person->State == STATE_DEAD || p_person->State == STATE_CARRY)
        return (UC_FALSE);

    if (p_person->State == STATE_MOVEING && (p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER || p_person->SubState == SUB_STATE_SIMPLE_ANIM)) {
        if (p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_DOWN || p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_IDLE) {
            // If they're sitting down, they can be interrupted.
            return UC_TRUE;
        }
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL)
        if (p_person->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER && (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER)
            return (UC_TRUE);

    if (p_person->Genus.Person->Flags & (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C))
        return (UC_FALSE);

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_GOTO_XZ:
    case PCOM_MOVE_STATE_GOTO_WAYPOINT:
    case PCOM_MOVE_STATE_GOTO_THING:

        if (p_person->Genus.Person->pcom_move_substate == PCOM_MOVE_SUBSTATE_ACTION) {
            // In the middle of a complicated move.
            return UC_FALSE;
        }
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
        if (p_person->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER && (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER)
            return UC_TRUE;

        if (p_person->State == STATE_IDLE || p_person->State == STATE_GOTOING || p_person->State == STATE_NORMAL) {
            return UC_TRUE;
        }

        if (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN) {
            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
                return UC_TRUE;
            }
        }

        if (p_person->State == STATE_MOVEING) {
            if (p_person->SubState == SUB_STATE_SIMPLE_ANIM || p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER) {
                return UC_TRUE;
            }
        }
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING) {
        return UC_TRUE;
    }

    return UC_FALSE;
}

// Returns UC_TRUE if the person has any kind of gun ready to shoot.
// uc_orig: PCOM_person_has_gun_in_hand (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_gun_in_hand(Thing* p_person)
{
    if (p_person->Genus.Person->PersonType == PERSON_MIB1 || p_person->Genus.Person->PersonType == PERSON_MIB2 || p_person->Genus.Person->PersonType == PERSON_MIB3) {
        // MIB are ninja shooting machines with built-in AK47s!
        return UC_TRUE;
    }

    if (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_DRAW_GUN) {
        // Still drawing a weapon.
        return UC_FALSE;
    }

    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        return UC_TRUE;
    }

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        if (p_special->Genus.Special->SpecialType == SPECIAL_SHOTGUN || p_special->Genus.Special->SpecialType == SPECIAL_AK47 || p_special->Genus.Special->SpecialType == SPECIAL_GRENADE) {
            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// Returns UC_TRUE if the shooter has a gun aimed at p_person.
// uc_orig: PCOM_target_could_shoot_me (fallen/Source/pcom.cpp)
SLONG PCOM_target_could_shoot_me(Thing* p_person, Thing* p_shooter)
{
#define PCOM_SHOOTME_DANGLE 256

    if (can_a_see_b(p_shooter, p_person))
        if (PCOM_person_has_gun_in_hand(p_shooter)) {
            SLONG dangle = get_dangle(p_shooter, p_person);

            if (dangle < PCOM_SHOOTME_DANGLE || dangle > 2048 - PCOM_SHOOTME_DANGLE) {
                return UC_TRUE;
            }
        }

    return UC_FALSE;
}

// Returns the distance of the person from the world point (world_x, world_z).
// uc_orig: PCOM_person_dist_from (fallen/Source/pcom.cpp)
SLONG PCOM_person_dist_from(
    Thing* p_person,
    SLONG world_x,
    SLONG world_z)
{
    SLONG dx = abs((p_person->WorldPos.X >> 8) - world_x);
    SLONG dz = abs((p_person->WorldPos.Z >> 8) - world_z);

    SLONG dist = QDIST2(dx, dz);

    return dist;
}

// Finds a good hiding spot near p_person — a tile with PAP_FLAG_HIDDEN that is
// visible from the person's position. Returns UC_TRUE if found, UC_FALSE otherwise.
// uc_orig: PCOM_find_hiding_place (fallen/Source/pcom.cpp)
SLONG PCOM_find_hiding_place(
    Thing* p_person,
    SLONG* hide_x,
    SLONG* hide_z)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;
    SLONG score;

    SLONG mid_x = p_person->WorldPos.X >> 16;
    SLONG mid_z = p_person->WorldPos.Z >> 16;

    SLONG mx;
    SLONG mz;

    SLONG best_x = 0;
    SLONG best_z = 0;
    SLONG best_score = -UC_INFINITY;

    SLONG x1 = (p_person->WorldPos.X >> 8);
    SLONG y1 = (p_person->WorldPos.Y >> 8) + 0x60;
    SLONG z1 = (p_person->WorldPos.Z >> 8);

    SLONG x2;
    SLONG y2;
    SLONG z2;

    PAP_Hi* ph;

    for (dx = -5; dx <= 5; dx++)
        for (dz = -5; dz <= 5; dz++) {
            mx = mid_x + dx;
            mz = mid_z + dz;

            if (WITHIN(mx, 0, PAP_SIZE_HI - 1) && WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
                ph = &PAP_2HI(mx, mz);

                if (ph->Flags & PAP_FLAG_HIDDEN) {
                    // Inside a building.
                } else {
                    dist = abs(dx) + abs(dz);

                    if (dist >= 2) {
                        // Could we see someone standing on this square?
                        x2 = (mx << 8) + 0x80;
                        z2 = (mz << 8) + 0x80;

                        y2 = y1;

                        if (there_is_a_los_mav(x1, y1, z1, x2, y2, z2)) {
                            score = dist;

                            if (score > best_score) {
                                best_score = score;
                                best_x = x2;
                                best_z = z2;
                            }
                        }
                    }
                }
            }
        }

    if (best_score > -UC_INFINITY) {
        *hide_x = best_x;
        *hide_z = best_z;
    }

    return (best_score > -UC_INFINITY);
}

// Returns UC_TRUE if Darci is doing something that would get her arrested (fighting).
// uc_orig: PCOM_player_is_doing_something_naughty (fallen/Source/pcom.cpp)
SLONG PCOM_player_is_doing_something_naughty(Thing* darci)
{
    // She isn't allowed to be fighting anyone.
    if (darci->Genus.Person->Mode == PERSON_MODE_FIGHT) {
        return UC_TRUE;
    }

    /*
    map_x = darci->WorldPos.X >> 16;
    map_z = darci->WorldPos.Z >> 16;

    if (PAP_2HI(map_x,map_z).Flags & PAP_FLAG_NAUGHTY)
    {
            return UC_TRUE;
    }

    if (darci->Genus.Person->Action == ACTION_CLIMBING)
    {
            SLONG dx;
            SLONG dz;

            dx = -SIN(darci->Draw.Tweened->Angle);
            dz = -COS(darci->Draw.Tweened->Angle);

            map_x = darci->WorldPos.X + dx >> 16;
            map_z = darci->WorldPos.Z + dz >> 16;

            if (PAP_2HI(map_x,map_z).Flags & PAP_FLAG_NAUGHTY)
            {
                    return UC_TRUE;
            }
    }
    */

    return UC_FALSE;
}

// uc_orig: PCOM_set_person_ai_kill_person (fallen/Source/pcom.cpp)  [forward decl used by gang alert]
void PCOM_set_person_ai_kill_person(Thing* p_person, Thing* p_target, SLONG alert_gang);

// Notify nearby same-colour gang members to join fight against target.
// uc_orig: PCOM_alert_my_gang_to_a_fight (fallen/Source/pcom.cpp)
void PCOM_alert_my_gang_to_a_fight(Thing* p_person, Thing* p_target)
{
    SLONG i;

    PCOM_Gang* pg;
    Thing* p_gang;

    ASSERT(WITHIN(p_person->Genus.Person->pcom_colour, 0, PCOM_MAX_GANGS - 1));
    ASSERT(p_person->Genus.Person->pcom_bent & PCOM_BENT_GANG);

    pg = &PCOM_gang[p_person->Genus.Person->pcom_colour];

    for (i = 0; i < pg->number; i++) {
        p_gang = TO_THING(PCOM_gang_person[pg->index + i]);

        if (p_gang->Class == CLASS_PERSON)
            if (p_gang != p_person) {

                if (!(p_gang->Genus.Person->Flags & FLAG_PERSON_HELPLESS))
                    if (p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_SEARCHING || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_TAUNT || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING) {

                        if (p_target->Genus.Person->PersonType == PERSON_DARCI) {
                            if (p_gang->Genus.Person->PersonType == PERSON_CIV) {
                                ASSERT(0);
                            }
                            if (p_gang->Genus.Person->PersonType == PERSON_COP) {
                                ASSERT(0);
                            }
                        }

                        PCOM_set_person_ai_kill_person(p_gang, p_target, UC_FALSE);
                    }
            }
    }
}

// Signals all same-colour gang members to flee from p_target.
// uc_orig: PCOM_alert_my_gang_to_flee (fallen/Source/pcom.cpp)
void PCOM_alert_my_gang_to_flee(Thing* p_person, Thing* p_target)
{
    SLONG i;

    PCOM_Gang* pg;
    Thing* p_gang;

    ASSERT(WITHIN(p_person->Genus.Person->pcom_colour, 0, PCOM_MAX_GANGS - 1));
    ASSERT(p_person->Genus.Person->pcom_bent & PCOM_BENT_GANG);

    pg = &PCOM_gang[p_person->Genus.Person->pcom_colour];

    for (i = 0; i < pg->number; i++) {
        p_gang = TO_THING(PCOM_gang_person[pg->index + i]);

        if (p_gang->Class == CLASS_PERSON)
            if (!am_i_a_thug(p_person) || (!(p_gang->Genus.Person->pcom_bent & PCOM_BENT_FIGHT_BACK)))
                if (p_gang != p_person) {
                    if (!(p_gang->Genus.Person->Flags & FLAG_PERSON_HELPLESS))
                        if (p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_SEARCHING || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_TAUNT || p_gang->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING) {
                            PCOM_set_person_ai_flee_person(p_gang, p_target);
                        }
                }
    }
}

// ============================================================
// Movement state setters — second chunk
// These functions configure pcom_move_state and start locomotion.
// ============================================================

// Additional forward declarations needed by this chunk.

// uc_orig: find_arrestee (fallen/Source/pcom.cpp)  [defined in Person.cpp]
extern UWORD find_arrestee(Thing* p_person);

// uc_orig: set_person_arrest (fallen/Source/Person.cpp)
extern void set_person_arrest(Thing* p_person, SLONG who_to_arrest);

// uc_orig: dist_to_target (fallen/Source/Person.cpp)
extern SLONG dist_to_target(Thing* p_person_a, Thing* p_person_b);

// uc_orig: get_gangattack (fallen/Source/Combat.cpp)  [declared in ai/combat.h]
// already declared via #include "ai/combat.h"

// Halt the person in place and set move state to STILL.
// uc_orig: PCOM_set_person_move_still (fallen/Source/pcom.cpp)
void PCOM_set_person_move_still(Thing* p_person)
{
    set_person_idle(p_person);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Navigate to world-space (dest_x, dest_z) via MAV pathfinding.
// Handles warehouse routing: exits/enters/stays-inside warehouse as needed.
// Falls back to LOSMAV mode when in a wander-safe square as a civilian.
// uc_orig: PCOM_set_person_move_mav_to_xz (fallen/Source/pcom.cpp)
void PCOM_set_person_move_mav_to_xz(Thing* p_person, SLONG dest_x, SLONG dest_z, SLONG speed)
{
    SLONG caps;

    SLONG goal_x;
    SLONG goal_z;

    SLONG start_x;
    SLONG start_z;

    start_x = p_person->WorldPos.X >> 16;
    start_z = p_person->WorldPos.Z >> 16;

    dest_x >>= 8;
    dest_z >>= 8;

    p_person->Genus.Person->pcom_move_arg = (dest_x << 8) | (dest_z);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GOTO_XZ;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
    p_person->Genus.Person->pcom_move_counter = 0;

    caps = (p_person->Genus.Person->pcom_bent & PCOM_BENT_RESTRICTED) ? MAV_CAPS_GOTO : MAV_CAPS_DARCI;

    if (p_person->Genus.Person->pcom_ai == PCOM_AI_CIV) {
        if ((p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER) || (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PLACE || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PERSON)) {
            if (WAND_square_is_wander(p_person->WorldPos.X >> 16, p_person->WorldPos.Z >> 16)) {
                caps = MAV_CAPS_GOTO;
            }
        }
    }

    UWORD nav_into_ware = NULL;
    UBYTE nav_outof_ware = UC_FALSE;
    UBYTE nav_inside_ware = UC_FALSE;

    if ((p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PLACE || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PERSON)) {
        if (p_person->Genus.Person->Ware) {
            nav_outof_ware = UC_TRUE;
        }
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK) {
        if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_HOME_IN_WAREHOUSE) {
            nav_into_ware = WARE_which_contains(
                p_person->Genus.Person->HomeX >> 8,
                p_person->Genus.Person->HomeZ >> 8);
        }
    }

    if (nav_into_ware && p_person->Genus.Person->Ware) {
        if (nav_into_ware == p_person->Genus.Person->Ware) {
            nav_into_ware = NULL;
            nav_inside_ware = UC_TRUE;
        } else {
            nav_outof_ware = UC_TRUE;
            nav_into_ware = UC_FALSE;
        }
    }

    if (nav_outof_ware) {
        p_person->Genus.Person->pcom_move_ma = WARE_mav_exit(
            p_person,
            caps);
    } else if (nav_into_ware) {
        p_person->Genus.Person->pcom_move_ma = WARE_mav_enter(
            p_person,
            nav_into_ware,
            caps);
    } else if (nav_inside_ware) {
        p_person->Genus.Person->pcom_move_ma = WARE_mav_inside(
            p_person,
            dest_x,
            dest_z,
            caps);
    } else {
        p_person->Genus.Person->pcom_move_ma = MAV_do(
            start_x,
            start_z,
            dest_x,
            dest_z,
            caps);
    }

    PCOM_get_mav_action_pos(
        p_person,
        &goal_x,
        &goal_z);

    set_person_goto_xz(
        p_person,
        goal_x,
        goal_z,
        speed);
}

// Navigate to a Thing via MAV pathfinding.
// Handles warehouse routing relative to target's location.
// Uses LOSMAV (straight-line run) when LOS is blocked and conditions allow.
// uc_orig: PCOM_set_person_move_mav_to_thing (fallen/Source/pcom.cpp)
void PCOM_set_person_move_mav_to_thing(Thing* p_person, Thing* p_target, SLONG speed)
{
    SLONG goal_x;
    SLONG goal_z;

    SLONG start_x;
    SLONG start_y;
    SLONG start_z;

    calc_sub_objects_position(
        p_person,
        p_person->Draw.Tweened->AnimTween,
        SUB_OBJECT_LEFT_FOOT,
        &start_x,
        &start_y,
        &start_z);

    start_x += p_person->WorldPos.X >> 8;
    start_z += p_person->WorldPos.Z >> 8;

    start_x >>= 8;
    start_z >>= 8;

    SLONG dest_x;
    SLONG dest_z;

    UBYTE nav_outof_ware = UC_FALSE;
    UBYTE nav_inside_ware = UC_FALSE;
    SLONG nav_into_ware = NULL;

    UBYTE caps = (p_person->Genus.Person->pcom_bent & PCOM_BENT_RESTRICTED) ? MAV_CAPS_GOTO : MAV_CAPS_DARCI;

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GOTO_THING;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
    p_person->Genus.Person->pcom_move_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_move_counter = 0;

    if (p_person->Genus.Person->Ware) {
        if (p_target->Class != CLASS_PERSON) {
            nav_outof_ware = UC_TRUE;
        } else if (p_target->Genus.Person->Ware != p_person->Genus.Person->Ware) {
            nav_outof_ware = UC_TRUE;
        } else {
            nav_inside_ware = UC_TRUE;
        }
    } else if (p_target->Class == CLASS_PERSON && p_target->Genus.Person->Ware) {
        nav_into_ware = p_target->Genus.Person->Ware;
    }

    if (nav_into_ware) {
        p_person->Genus.Person->pcom_move_ma = WARE_mav_enter(
            p_person,
            nav_into_ware,
            caps);
    } else if (nav_outof_ware) {
        p_person->Genus.Person->pcom_move_ma = WARE_mav_exit(
            p_person,
            caps);
    } else {
        if (!nav_inside_ware && PCOM_should_i_try_to_los_mav_to_person(p_person, p_target)) {
            if (!there_is_a_los_mav(
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Y + 0x4000 >> 8,
                    p_person->WorldPos.Z >> 8,
                    p_target->WorldPos.X >> 8,
                    p_target->WorldPos.Y + 0x4000 >> 8,
                    p_target->WorldPos.Z >> 8)) {
                p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_LOSMAV;

                set_person_goto_xz(
                    p_person,
                    p_target->WorldPos.X >> 8,
                    p_target->WorldPos.Z >> 8,
                    speed);

                return;
            }
        }

        PCOM_get_person_dest(
            p_person,
            &dest_x,
            &dest_z);

        dest_x >>= 8;
        dest_z >>= 8;

        // Guard against off-map vehicles (MikeD).
        SATURATE(dest_x, 0, PAP_SIZE_HI - 1);
        SATURATE(dest_z, 0, PAP_SIZE_HI - 1);

        if (nav_inside_ware) {
            p_person->Genus.Person->pcom_move_ma = WARE_mav_inside(
                p_person,
                dest_x,
                dest_z,
                caps);
        } else {
            p_person->Genus.Person->pcom_move_ma = MAV_do(
                start_x,
                start_z,
                dest_x,
                dest_z,
                caps);
        }
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL) {
        if (p_person->Genus.Person->pcom_bent & PCOM_BENT_RESTRICTED) {
            if (!MAV_do_found_dest) {
                // Can't nav to target — go home instead.
                PCOM_set_person_ai_homesick(p_person);
                return;
            }
        }
    }

    PCOM_get_mav_action_pos(
        p_person,
        &goal_x,
        &goal_z);

    set_person_goto_xz(
        p_person,
        goal_x,
        goal_z,
        speed);
}

// Navigate to an EWAY waypoint via MAV.
// Handles warehouse routing based on waypoint's warehouse membership.
// uc_orig: PCOM_set_person_move_mav_to_waypoint (fallen/Source/pcom.cpp)
void PCOM_set_person_move_mav_to_waypoint(Thing* p_person, SLONG waypoint, SLONG speed)
{
    SLONG goal_x;
    SLONG goal_z;

    SLONG dest_x;
    SLONG dest_y;
    SLONG dest_z;

    SLONG start_x;
    SLONG start_y;
    SLONG start_z;

    calc_sub_objects_position(
        p_person,
        p_person->Draw.Tweened->AnimTween,
        SUB_OBJECT_LEFT_FOOT,
        &start_x,
        &start_y,
        &start_z);

    start_x += p_person->WorldPos.X >> 8;
    start_z += p_person->WorldPos.Z >> 8;

    start_x >>= 8;
    start_z >>= 8;

    UBYTE caps = (p_person->Genus.Person->pcom_bent & PCOM_BENT_RESTRICTED) ? MAV_CAPS_GOTO : MAV_CAPS_DARCI;
    UBYTE eware = EWAY_get_warehouse(waypoint);
    UBYTE nav_outof_ware = UC_FALSE;
    UBYTE nav_inside_ware = UC_FALSE;
    SLONG nav_into_ware = NULL;

    EWAY_get_position(
        waypoint,
        &dest_x,
        &dest_y,
        &dest_z);

    dest_x >>= 8;
    dest_z >>= 8;

    p_person->Genus.Person->pcom_move_arg = waypoint;

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GOTO_WAYPOINT;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
    p_person->Genus.Person->pcom_move_counter = 0;

    if (p_person->Genus.Person->Ware) {
        if (eware != p_person->Genus.Person->Ware) {
            nav_outof_ware = UC_TRUE;
        } else {
            nav_inside_ware = UC_TRUE;
        }
    } else {
        if (eware) {
            nav_into_ware = eware;
        }
    }

    if (nav_into_ware) {
        p_person->Genus.Person->pcom_move_ma = WARE_mav_enter(
            p_person,
            nav_into_ware,
            caps);
    } else if (nav_outof_ware) {
        p_person->Genus.Person->pcom_move_ma = WARE_mav_exit(
            p_person,
            caps);
    } else {
        if (nav_inside_ware) {
            p_person->Genus.Person->pcom_move_ma = WARE_mav_inside(
                p_person,
                dest_x,
                dest_z,
                caps);
        } else {
            p_person->Genus.Person->pcom_move_ma = MAV_do(
                start_x,
                start_z,
                dest_x,
                dest_z,
                caps);
        }
    }

    PCOM_get_mav_action_pos(
        p_person,
        &goal_x,
        &goal_z);

    set_person_goto_xz(
        p_person,
        goal_x,
        goal_z,
        speed);
}

// Pick a random direction away from (from_x, from_z) and nav there.
// Retries up to 3 times if the chosen path would run toward the threat.
// uc_orig: PCOM_set_person_move_runaway (fallen/Source/pcom.cpp)
void PCOM_set_person_move_runaway(
    Thing* p_person,
    SLONG from_x,
    SLONG from_z)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;

    SLONG goal_x;
    SLONG goal_z;

    SLONG dest_x;
    SLONG dest_z;

    SLONG dist_me;
    SLONG dist_from;

    SLONG tries = 0;

    while (1) {
        dx = (p_person->WorldPos.X >> 8) - from_x;
        dz = (p_person->WorldPos.Z >> 8) - from_z;

        dist = abs(dx) + abs(dz) + 1;

        dx = (dx << 13) / dist;
        dz = (dz << 13) / dist;

        dx += (Random() & 0x7ff);
        dz += (Random() & 0x7ff);

        dx -= 0x400;
        dz -= 0x400;

        goal_x = (p_person->WorldPos.X >> 8) + dx;
        goal_z = (p_person->WorldPos.Z >> 8) + dz;

        // Stay on map.
        if (goal_x < 0) {
            goal_x = -goal_x;
        }
        if (goal_z < 0) {
            goal_z = -goal_z;
        }

        if (goal_x > (PAP_SIZE_HI << PAP_SHIFT_HI)) {
            goal_x = 2 * (PAP_SIZE_HI << PAP_SHIFT_HI) - goal_x;
        }
        if (goal_z > (PAP_SIZE_HI << PAP_SHIFT_HI)) {
            goal_z = 2 * (PAP_SIZE_HI << PAP_SHIFT_HI) - goal_z;
        }

        PCOM_set_person_move_mav_to_xz(
            p_person,
            goal_x,
            goal_z,
            PCOM_MOVE_SPEED_RUN);

        PCOM_get_mav_action_pos(
            p_person,
            &dest_x,
            &dest_z);

        dx = abs((p_person->WorldPos.X >> 8) - dest_x);
        dz = abs((p_person->WorldPos.Z >> 8) - dest_z);

        dist_me = QDIST2(dx, dz);

        dx = abs(from_x - dest_x);
        dz = abs(from_z - dest_z);

        dist_from = QDIST2(dx, dz);

        if (dist_from < dist_me) {
            // Running toward the threat — retry.
            tries += 1;

            if (tries < 3) {
                // Have another go.
            } else {
                // Give up.
                break;
            }
        } else {
            // Running away correctly.
            break;
        }
    }
}

// Decide 3-point-turn vs. straight drive based on vehicle angle to destination.
// Used for DRIVETO state. Sets pcom_move_substate accordingly.
// uc_orig: PCOM_set_person_substate_goto_or_3pturn (fallen/Source/pcom.cpp)
void PCOM_set_person_substate_goto_or_3pturn(Thing* p_person)
{
    SLONG dx;
    SLONG dz;
    SLONG dest_x;
    SLONG dest_z;
    SLONG wangle;
    SLONG dangle;

    Thing* p_vehicle;

    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

    p_vehicle = TO_THING(p_person->Genus.Person->InCar);

    PCOM_get_person_dest(
        p_person,
        &dest_x,
        &dest_z);

    dx = dest_x - (p_vehicle->WorldPos.X >> 8);
    dz = dest_z - (p_vehicle->WorldPos.Z >> 8);

    wangle = calc_angle(dx, dz);
    wangle += 1024;
    wangle &= 2047;

    dangle = wangle - p_vehicle->Genus.Vehicle->Angle;

    if (dangle < -1024) {
        dangle += 2048;
    }
    if (dangle > +1024) {
        dangle -= 2048;
    }

    if (abs(dangle) > 750) {
        p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_3PTURN;
    } else {
        p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
    }
}

// Like PCOM_set_person_substate_goto_or_3pturn, but never 3-point-turns.
// When facing away from the next node, swaps n1/n2 road nodes so direction is forward.
// Used only for DRIVE_DOWN, not DRIVETO.
// uc_orig: PCOM_set_person_substate_goto (fallen/Source/pcom.cpp)
void PCOM_set_person_substate_goto(Thing* p_person)
{
    SLONG dx;
    SLONG dz;
    SLONG dest_x;
    SLONG dest_z;
    SLONG wangle;
    SLONG dangle;

    Thing* p_vehicle;

    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

    p_vehicle = TO_THING(p_person->Genus.Person->InCar);

    PCOM_get_person_dest(
        p_person,
        &dest_x,
        &dest_z);

    dx = dest_x - (p_vehicle->WorldPos.X >> 8);
    dz = dest_z - (p_vehicle->WorldPos.Z >> 8);

    wangle = calc_angle(dx, dz);
    wangle += 1024;
    wangle &= 2047;

    dangle = wangle - p_vehicle->Genus.Vehicle->Angle;

    if (dangle < -1024) {
        dangle += 2048;
    }
    if (dangle > +1024) {
        dangle -= 2048;
    }

    if (abs(dangle) > 750) {
        // swap the road nodes so we go in the forward direction
        p_person->Genus.Person->pcom_move_arg = (p_person->Genus.Person->pcom_move_arg << 8) | (p_person->Genus.Person->pcom_move_arg >> 8);
    }

    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
}

// Start driving toward a road waypoint; decide turn method based on vehicle angle.
// uc_orig: PCOM_set_person_move_driveto (fallen/Source/pcom.cpp)
void PCOM_set_person_move_driveto(Thing* p_person, SLONG waypoint)
{
    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_DRIVETO;
    p_person->Genus.Person->pcom_move_arg = waypoint;
    p_person->Genus.Person->pcom_move_counter = 0;

    PCOM_set_person_substate_goto_or_3pturn(p_person);
}

// Start parking at a road waypoint.
// uc_orig: PCOM_set_person_move_park_car (fallen/Source/pcom.cpp)
void PCOM_set_person_move_park_car(Thing* p_person, SLONG waypoint)
{
    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_PARK_CAR;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_NONE;
    p_person->Genus.Person->pcom_move_arg = waypoint;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Drive freely along a road segment between nodes n1 and n2.
// uc_orig: PCOM_set_person_move_drive_down (fallen/Source/pcom.cpp)
void PCOM_set_person_move_drive_down(Thing* p_person, SLONG n1, SLONG n2)
{
    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_DRIVE_DOWN;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
    p_person->Genus.Person->pcom_move_arg = (n1 << 8) | n2;
    p_person->Genus.Person->pcom_move_counter = 0;

    PCOM_set_person_substate_goto(p_person);
}

// If currently driving a road segment: switch to PARK_CAR_ON_ROAD (park at road edge).
// Otherwise: stop immediately in place (PARK_CAR with no waypoint).
// uc_orig: PCOM_set_person_move_park_car_on_road (fallen/Source/pcom.cpp)
void PCOM_set_person_move_park_car_on_road(Thing* p_person)
{
    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

    if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_DRIVE_DOWN) {
        // Stop near the edge of the road — pcom_move_arg unchanged (same road nodes).
        p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_PARK_CAR_ON_ROAD;
        p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
        p_person->Genus.Person->pcom_move_counter = 0;
    } else {
        // Stop in place.
        p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_PARK_CAR;
        p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_NONE;
        p_person->Genus.Person->pcom_move_arg = NULL;
        p_person->Genus.Person->pcom_move_counter = 0;
    }
}

// Start a sliding tackle toward p_target; notifies cop system if person is a thug.
// uc_orig: PCOM_set_person_move_goto_thing_slide (fallen/Source/pcom.cpp)
void PCOM_set_person_move_goto_thing_slide(Thing* p_person, Thing* p_target)
{
    if (am_i_a_thug(p_person)) {
        PCOM_call_cop_to_arrest_me(p_person, 1);
    }

    set_person_sliding_tackle(p_person, p_target);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GOTO_THING_SLIDE;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_NONE;
    p_person->Genus.Person->pcom_move_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Re-run pathfinding for the current move state (XZ/THING/WAYPOINT).
// Called when the nav result becomes stale (e.g. target moved far).
// uc_orig: PCOM_renav (fallen/Source/pcom.cpp)
void PCOM_renav(Thing* p_person)
{
    SLONG dest_x;
    SLONG dest_z;

    Thing* p_target;

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_GOTO_XZ:

        dest_x = (p_person->Genus.Person->pcom_move_arg >> 8) & 0xff;
        dest_z = (p_person->Genus.Person->pcom_move_arg >> 0) & 0xff;

        dest_x <<= 8;
        dest_z <<= 8;

        dest_x += 0x80;
        dest_z += 0x80;

        PCOM_set_person_move_mav_to_xz(
            p_person,
            dest_x,
            dest_z,
            p_person->Genus.Person->GotoSpeed);

        break;

    case PCOM_MOVE_STATE_GOTO_THING:

        p_target = TO_THING(p_person->Genus.Person->pcom_move_arg);

        PCOM_set_person_move_mav_to_thing(
            p_person,
            p_target,
            p_person->Genus.Person->GotoSpeed);

        break;

    case PCOM_MOVE_STATE_GOTO_WAYPOINT:

        PCOM_set_person_move_mav_to_waypoint(
            p_person,
            p_person->Genus.Person->pcom_move_arg,
            p_person->Genus.Person->GotoSpeed);

        break;

    default:
        ASSERT(0);
        break;
    }
}

// Returns UC_TRUE if the person has reached their navigation destination.
// Accounts for following mode (tighter threshold) and prolonged sliding.
// uc_orig: PCOM_finished_nav (fallen/Source/pcom.cpp)
SLONG PCOM_finished_nav(Thing* p_person)
{
    SLONG dest_x;
    SLONG dest_z;

    if (p_person->State == STATE_IDLE) {
        // Not moving at all — treat as finished.
        return UC_TRUE;
    }

    if (p_person->State == STATE_DANGLING) {
        // Mid-manoeuvre, can't stop.
        return UC_FALSE;
    }

    PCOM_get_person_dest(
        p_person,
        &dest_x,
        &dest_z);

    dest_x &= 0xffffff00;
    dest_z &= 0xffffff00;

    dest_x |= 0x80;
    dest_z |= 0x80;

    SLONG dist = PCOM_person_dist_from(
        p_person,
        dest_x,
        dest_z);

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING) {
        return (dist < 0x60);
    } else if (p_person->Genus.Person->SlideOdd > 20) {
        // Person has been sliding a long time; accept a wider threshold.
        if (dist < 0x100) {
            p_person->Genus.Person->SlideOdd = 1;
            return UC_TRUE;
        } else {
            return UC_FALSE;
        }
    } else {
        return (dist < PCOM_ARRIVE_DIST);
    }
}

// Halt person and enter PAUSE move state (idle until timeout).
// uc_orig: PCOM_set_person_move_pause (fallen/Source/pcom.cpp)
void PCOM_set_person_move_pause(Thing* p_person)
{
    set_person_idle(p_person);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_PAUSE;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Play a one-shot animation and wait for it to finish.
// uc_orig: PCOM_set_person_move_animation (fallen/Source/pcom.cpp)
void PCOM_set_person_move_animation(Thing* p_person, SLONG anim)
{
    set_person_do_a_simple_anim(p_person, anim);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_ANIM;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Execute a punch move; waits in WAIT_CIRCLE state until done.
// uc_orig: PCOM_set_person_move_punch (fallen/Source/pcom.cpp)
void PCOM_set_person_move_punch(Thing* p_person)
{
    turn_to_target_and_punch(p_person);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_WAIT_CIRCLE;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_PUNCH;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Execute a kick move; waits in WAIT_CIRCLE state until done.
// uc_orig: PCOM_set_person_move_kick (fallen/Source/pcom.cpp)
void PCOM_set_person_move_kick(Thing* p_person)
{
    turn_to_target_and_kick(p_person);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_WAIT_CIRCLE;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_KICK;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Bend down to pick up a Special item.
// uc_orig: PCOM_set_person_move_pickup_special (fallen/Source/pcom.cpp)
void PCOM_set_person_move_pickup_special(Thing* p_person, Thing* p_special)
{
    set_person_special_pickup(p_person);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_ANIM;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Find the best arrest target and initiate arrest animation.
// Falls back to STILL if no target found.
// uc_orig: PCOM_set_person_move_arrest (fallen/Source/pcom.cpp)
void PCOM_set_person_move_arrest(Thing* p_person)
{
    UWORD index;

    index = PCOM_person_wants_to_kill(p_person);

    if (index == NULL) {
        index = find_arrestee(p_person);
    }

    if (index) {
        set_person_arrest(p_person, index);

        p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_WAIT_CIRCLE;
        p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_ARREST;
        p_person->Genus.Person->pcom_move_counter = 0;
    } else {
        PCOM_set_person_move_still(p_person);
    }
}

// Draw best available ranged weapon: shotgun > AK47 > grenade > pistol.
// uc_orig: PCOM_set_person_move_draw_gun (fallen/Source/pcom.cpp)
void PCOM_set_person_move_draw_gun(Thing* p_person)
{
    Thing* p_special;

    if ((p_special = person_has_special(p_person, SPECIAL_SHOTGUN)) && p_special->Genus.Special->ammo) {
        set_person_draw_item(p_person, SPECIAL_SHOTGUN);
    } else if ((p_special = person_has_special(p_person, SPECIAL_AK47)) && p_special->Genus.Special->ammo) {
        set_person_draw_item(p_person, SPECIAL_AK47);
    } else if ((p_special = person_has_special(p_person, SPECIAL_GRENADE)) && p_special->Genus.Special->ammo) {
        set_person_draw_item(p_person, SPECIAL_GRENADE);
    } else {
        set_person_draw_gun(p_person);
    }

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GUNOUT;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Draw a hand-to-hand weapon of given special_type.
// uc_orig: PCOM_set_person_move_draw_h2h (fallen/Source/pcom.cpp)
void PCOM_set_person_move_draw_h2h(Thing* p_person, SLONG special)
{

    {
        set_person_draw_item(p_person, special);
    }

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GUNOUT;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Holster the current weapon; handles SpecialUse items separately from guns.
// uc_orig: PCOM_set_person_move_gun_away (fallen/Source/pcom.cpp)
void PCOM_set_person_move_gun_away(Thing* p_person)
{
    if (p_person->Genus.Person->SpecialUse) {
        p_person->Genus.Person->SpecialUse = NULL;
        p_person->Draw.Tweened->PersonID &= ~0xe0;

        set_person_idle(p_person);
    } else {
        set_person_gun_away(p_person);
    }

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GUNAWAY;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Fire the current gun.
// uc_orig: PCOM_set_person_move_shoot (fallen/Source/pcom.cpp)
void PCOM_set_person_move_shoot(Thing* p_person)
{
    set_person_shoot(p_person, 1);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_SHOOT;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// ============================================================
// Gang attack system — melee coordination ring
// ============================================================

// gang_angle_priority is declared in ai/pcom_globals.h (defined in pcom_globals.cpp)

// uc_orig: get_gangattack (fallen/Source/pcom.cpp) [external ref — defined in combat.cpp]
// already declared via ai/combat.h

// Validate all attacker slots around p_target; clear any that have left fight mode.
// uc_orig: check_players_gang (fallen/Source/pcom.cpp)
void check_players_gang(Thing* p_target)
{
    SLONG gang;
    SLONG c0;
    Thing* p_person;

    gang = p_target->Genus.Person->GangAttack;
    if (gang == 0)
        return;

    for (c0 = 0; c0 < 4; c0++) {
        if (gang_attacks[gang].Perp[c0]) {
            p_person = TO_THING(gang_attacks[gang].Perp[c0]);
            if (p_person->Genus.Person->PlayerID) {
                // Player — clear slot if they stopped fighting.
                if (p_person->Genus.Person->Mode != PERSON_MODE_FIGHT) {
                    remove_from_gang_attack(p_person, p_target);
                }
            }
        }
    }
}

// Count how many attacker slots are occupied around p_target.
// uc_orig: count_gang (fallen/Source/pcom.cpp)
UWORD count_gang(Thing* p_target)
{
    SLONG gang;
    SLONG c0, count = 0;

    gang = p_target->Genus.Person->GangAttack;
    if (gang == 0)
        return (0);

    for (c0 = 0; c0 < 4; c0++) {
        if (gang_attacks[gang].Perp[c0]) {
            count++;
        }
    }
    return (count);
}

// uc_orig: dist_to_target (fallen/Source/pcom.cpp) [external ref — defined in Person.cpp]
// already declared above

// Return the THING_NUMBER of any attacker already close to p_target (dist < 512).
// uc_orig: get_any_gang_member (fallen/Source/pcom.cpp)
UWORD get_any_gang_member(Thing* p_target)
{
    SLONG gang;
    SLONG c0, ret;

    gang = p_target->Genus.Person->GangAttack;
    if (gang == 0)
        return (0);

    for (c0 = 0; c0 < 4; c0++) {
        if (ret = gang_attacks[gang].Perp[c0]) {
            if (dist_to_target(p_target, TO_THING(ret)) < 512) {
                return (ret);
            }
        }
    }
    return (0);
}

// Return the THING_NUMBER of the closest non-KO'd attacker in p_target's gang ring.
// uc_orig: get_nearest_gang_member (fallen/Source/pcom.cpp)
UWORD get_nearest_gang_member(Thing* p_target)
{
    SLONG gang;
    SLONG c0, ret;
    SLONG bdist = 99999999, best_targ = 0, dist;

    gang = p_target->Genus.Person->GangAttack;
    if (gang == 0)
        return (0);

    for (c0 = 0; c0 < 4; c0++) {
        if (ret = gang_attacks[gang].Perp[c0]) {
            if (!is_person_ko(TO_THING(ret)))
                if ((dist = dist_to_target(p_target, TO_THING(ret))) < bdist) {
                    best_targ = ret;
                    bdist = dist;
                }
        }
    }
    return (best_targ);
}

// Return the THING_NUMBER of the first occupied slot (priority: 0,1,3,2).
// Used by the victim to know who their current attacker is.
// uc_orig: find_target_from_gang (fallen/Source/pcom.cpp)
UWORD find_target_from_gang(Thing* p_target)
{
    SLONG gang;
    SLONG perp;

    gang = p_target->Genus.Person->GangAttack;
    if (gang == 0)
        return (0);

    if (perp = gang_attacks[gang].Perp[0])
        return (perp);

    if (perp = gang_attacks[gang].Perp[1])
        return (perp);

    if (perp = gang_attacks[gang].Perp[3])
        return (perp);

    if (perp = gang_attacks[gang].Perp[2])
        return (perp);

    return (0);
}

// Remove p_person from p_target's gang attack slots. Returns 1 if found and removed.
// uc_orig: remove_from_gang_attack (fallen/Source/pcom.cpp)
SLONG remove_from_gang_attack(Thing* p_person, Thing* p_target)
{
    SLONG gang;
    SLONG c0;
    SLONG removed = 0;

    gang = p_target->Genus.Person->GangAttack;
    if (gang == 0)
        return (0);

    for (c0 = 0; c0 < 4; c0++) {
        if (gang_attacks[gang].Perp[c0] == THING_NUMBER(p_person)) {
            gang_attacks[gang].Perp[c0] = 0;
            removed = 1;
        }
    }
    return (removed);
}

// Set Agression=-55 on all attackers in p_target's gang slots (frighten them off).
// Called e.g. when the target fires a gun.
// uc_orig: scare_gang_attack (fallen/Source/pcom.cpp)
void scare_gang_attack(Thing* p_target)
{
    SLONG gang;
    SLONG c0;

    gang = p_target->Genus.Person->GangAttack;
    if (gang == 0)
        return;

    for (c0 = 0; c0 < 4; c0++) {
        if (gang_attacks[gang].Perp[c0]) {
            TO_THING(gang_attacks[gang].Perp[c0])->Genus.Person->Agression = -55;
        }
    }
}

// Re-slot all current attackers around p_target after positional changes.
// Clears all slots then re-inserts each attacker at their current angle.
// uc_orig: reset_gang_attack (fallen/Source/pcom.cpp)
void reset_gang_attack(Thing* p_target)
{
    UWORD perps[4];
    Thing* p_person;
    UWORD gang;
    SLONG c0;

    gang = p_target->Genus.Person->GangAttack;
    if (gang == 0)
        return;

    for (c0 = 0; c0 < 4; c0++) {
        perps[c0] = gang_attacks[gang].Perp[c0];
        gang_attacks[gang].Perp[c0] = 0;
    }

    for (c0 = 0; c0 < 4; c0++) {
        SLONG dx, dz, reqd_angle;

        if (perps[c0]) {
            p_person = TO_THING(perps[c0]);

            dx = p_target->WorldPos.X - p_person->WorldPos.X >> 8;
            dz = p_target->WorldPos.Z - p_person->WorldPos.Z >> 8;

            reqd_angle = calc_angle(dx, dz) + 256;
            reqd_angle &= 2047;
            reqd_angle >>= 9;

            push_into_attack_group_at_angle(p_person, (SLONG)gang, reqd_angle);
        }
    }
}

// Per-frame melee coordination: if p_person is attacking (SUB_STATE_CIRCLING_CIRCLE),
// back off other co-attackers (Agression=-100) to prevent simultaneous pile-in.
// uc_orig: process_gang_attack (fallen/Source/pcom.cpp)
void process_gang_attack(Thing* p_person, Thing* p_target)
{
    SLONG gang;
    SLONG c0;
    SLONG me;

    me = THING_NUMBER(p_person);

    gang = p_target->Genus.Person->GangAttack;

    if (p_person->SubState == SUB_STATE_CIRCLING_CIRCLE) {
        for (c0 = 0; c0 < 4; c0++) {
            SLONG perp;
            perp = gang_attacks[gang].Perp[c0];
            if (perp && perp != me) {
                switch (TO_THING(perp)->SubState) {
                case SUB_STATE_CIRCLING_CIRCLE:
                    TO_THING(perp)->Genus.Person->Agression = -100;
                    break;
                }
            }
        }
    }

    return;
    // Original rotation logic below was commented out in the original (left-in dead block).
    /*
        for(c0=0;c0<4;c0++)
        {
            if(gang_attacks[gang].Perp[c0]==me)
            {
                left=gang_attacks[gang].Perp[(c0-1)&3];
                right=gang_attacks[gang].Perp[(c0+1)&3];
                if(left==0 && right==0)
                    return;

                if(left==0&& right)
                {
                    lleft=gang_attacks[gang].Perp[(c0-2)&3];
                    if(lleft==0)
                    {
                        gang_attacks[gang].Perp[c0]=0;
                        p_person->Genus.Person->AttackAngle=(c0-1)&3;
                        gang_attacks[gang].Perp[(c0-1)&3]=me;
                        p_person->Genus.Person->Agression=-60-(c0<<2);
                    }
                }
                else
                if(left&&right==0)
                {
                    rright=gang_attacks[gang].Perp[(c0+2)&3];
                    if(rright==0)
                    {
                        gang_attacks[gang].Perp[c0]=0;
                        p_person->Genus.Person->AttackAngle=(c0+1)&3;
                        ASSERT(gang_attacks[gang].Perp[(c0+1)&3]==0);
                        gang_attacks[gang].Perp[(c0+1)&3]=me;
                        p_person->Genus.Person->Agression=-60-(c0<<2);
                    }
                }
                return;
            }
        }
    */
}

// Claim a cardinal slot in the gang ring for p_person, displacing others if needed.
// Rotates existing occupants to adjacent slots (positive or negative direction).
// uc_orig: push_into_attack_group_at_angle (fallen/Source/pcom.cpp)
void push_into_attack_group_at_angle(Thing* p_person, SLONG gang, SLONG reqd_angle)
{
    SLONG c0 = 4;
    Thing* p_copy;

    MSG_add("try push in  at %d    [%d %d %d %d %d %d %d %d] \n", reqd_angle, gang_attacks[gang].Perp[0], gang_attacks[gang].Perp[1], gang_attacks[gang].Perp[2], gang_attacks[gang].Perp[3], gang_attacks[gang].Perp[4], gang_attacks[gang].Perp[5], gang_attacks[gang].Perp[6], gang_attacks[gang].Perp[7]);

    if (gang_attacks[gang].Perp[(reqd_angle) & 3] != 0)
        for (c0 = 1; c0 <= 2; c0++) {
            ASSERT(gang_attacks[gang].Perp[(reqd_angle + c0) & 3] != THING_NUMBER(p_person));
            ASSERT(gang_attacks[gang].Perp[(reqd_angle - c0) & 3] != THING_NUMBER(p_person));

            if (gang_attacks[gang].Perp[(reqd_angle + c0) & 3] == 0) {
                // Rotate positive direction.
                MSG_add(" push in at position %d shoving %d peeps ", reqd_angle, c0);
                while (c0 > 0) {
                    gang_attacks[gang].Perp[(reqd_angle + c0) & 3] = gang_attacks[gang].Perp[(reqd_angle + c0 - 1) & 3];
                    p_copy = TO_THING(gang_attacks[gang].Perp[(reqd_angle + c0 - 1) & 3]);
                    p_copy->Genus.Person->AttackAngle = (reqd_angle + c0) & 3;
                    c0--;
                }
                break;
            } else if (gang_attacks[gang].Perp[(reqd_angle - c0 + 8) & 3] == 0) {
                // Rotate negative direction.
                MSG_add(" push in at position %d shoving NEG %d peeps ", reqd_angle, c0);
                while (c0 > 0) {
                    gang_attacks[gang].Perp[(reqd_angle - c0 + 8) & 3] = gang_attacks[gang].Perp[(reqd_angle - c0 + 1 + 8) & 3];
                    p_copy = TO_THING(gang_attacks[gang].Perp[(reqd_angle - c0 + 1 + 8) & 3]);
                    p_copy->Genus.Person->AttackAngle = (reqd_angle - c0 + 8) & 3;
                    c0--;
                }
                break;
            }
            if (c0 == 4)
                MSG_add("FAILED to push in\n");
        }

    // Assign p_person to the desired angle (may share if ring is full).
    gang_attacks[gang].Perp[reqd_angle & 3] = THING_NUMBER(p_person);
    p_person->Genus.Person->AttackAngle = reqd_angle;
}

// Enter gang combat against p_target: allocate a gang slot if needed,
// then claim a cardinal angle slot for p_person.
// uc_orig: PCOM_new_gang_attack (fallen/Source/pcom.cpp)
void PCOM_new_gang_attack(Thing* p_person, Thing* p_target)
{
    SLONG gang;
    SLONG c0;

    SLONG reqd_angle;
    SLONG dx;
    SLONG dz;

    dx = p_target->WorldPos.X - p_person->WorldPos.X >> 8;
    dz = p_target->WorldPos.Z - p_person->WorldPos.Z >> 8;

    reqd_angle = calc_angle(dx, dz);
    reqd_angle &= 2047;
    reqd_angle >>= 9;

    if (p_target->Genus.Person->GangAttack == 0) {
        // Allocate a new gang struct for this target.
        gang = get_gangattack(p_target);
    } else {
        gang = p_target->Genus.Person->GangAttack;
    }

    // Ensure p_person is not already in the ring.
    for (c0 = 0; c0 < 4; c0++) {
        if (gang_attacks[gang].Perp[c0] == THING_NUMBER(p_person)) {
            gang_attacks[gang].Perp[c0] = 0;
        }
    }

    if (gang_attacks[gang].Perp[reqd_angle] == 0) {
        // Preferred angle is free — claim it directly.
        gang_attacks[gang].Perp[reqd_angle] = THING_NUMBER(p_person);
        p_person->Genus.Person->AttackAngle = reqd_angle;
    } else {
        // Angle occupied — rotate others to make room.
        push_into_attack_group_at_angle(
            p_person,
            gang,
            reqd_angle);
    }
}

// Start circling a target (melee wind-up phase).
// Joins the gang attack ring for p_target.
// uc_orig: PCOM_set_person_move_circle (fallen/Source/pcom.cpp)
void PCOM_set_person_move_circle(Thing* p_person, Thing* p_target)
{
    set_person_circle(p_person, p_target);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_CIRCLE;
    p_person->Genus.Person->pcom_move_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_move_counter = 0;

    PCOM_new_gang_attack(p_person, p_target);
}

// ============================================================
// AI setter functions — high-level state transitions
// (Chunk 3: move_getincar through PCOM_create_person)
// ============================================================

// Command person to enter a vehicle as driver or passenger.
// Leaves any existing gang attack group first.
// uc_orig: PCOM_set_person_move_getincar (fallen/Source/pcom.cpp)
void PCOM_set_person_move_getincar(Thing* p_person, Thing* p_vehicle, SLONG am_i_a_passenger, SLONG door)
{
    ASSERT(door == 0 || door == 1);

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    if (am_i_a_passenger) {
        set_person_passenger_in_vehicle(p_person, p_vehicle, door);
    } else {
        set_person_enter_vehicle(p_person, p_vehicle, door);
    }

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GETINCAR;
    p_person->Genus.Person->pcom_move_arg = THING_NUMBER(p_vehicle);
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Command person to exit their current vehicle.
// Person must be driving or a passenger (asserted).
// uc_orig: PCOM_set_person_move_leavecar (fallen/Source/pcom.cpp)
void PCOM_set_person_move_leavecar(Thing* p_person)
{
    ASSERT(p_person->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_PASSENGER));

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    set_person_exit_vehicle(p_person);

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_LEAVECAR;
    p_person->Genus.Person->pcom_move_arg = 0;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Return person to idle state — cancel target, reset AI to NORMAL.
// uc_orig: PCOM_set_person_ai_normal (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_normal(Thing* p_person)
{
    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }
    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_NORMAL;
    p_person->Genus.Person->pcom_ai_substate = 0;
    p_person->Genus.Person->pcom_ai_counter = 0;

    PCOM_set_person_move_still(p_person);
}

// Put person into knocked-out recovery state.
// They wait for recovery, then resume normal AI.
// uc_orig: PCOM_set_person_ai_knocked_out (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_knocked_out(Thing* p_person)
{
    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_KNOCKEDOUT;
    p_person->Genus.Person->pcom_ai_substate = 0;
    p_person->Genus.Person->pcom_ai_counter = 0;

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_ANIM;
    p_person->Genus.Person->pcom_move_counter = 0;
}

// Start a cop arresting p_target — plays arrest-start sound, navigates to target.
// uc_orig: PCOM_set_person_ai_arrest (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_arrest(Thing* p_person, Thing* p_target)
{
    if (p_target->Genus.Person->PersonType == PERSON_DARCI)
        ASSERT(0);

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }
    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_ARREST;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_ai_substate = 0;
    p_person->Genus.Person->pcom_ai_counter = 0;

    set_face_thing(p_person, p_target);

    //	MFX_play_thing(THING_NUMBER(p_person),S_HELLOALLOALLO,MFX_REPLACE,p_person);
    MFX_play_thing(THING_NUMBER(p_person), SOUND_Range(S_COP_ARREST_START, S_COP_ARREST_END), MFX_REPLACE, p_person);

    PCOM_set_person_move_mav_to_thing(p_person, p_target, PCOM_MOVE_SPEED_RUN);
}

// Switch person to combat mode targeting p_target.
// Alerts the person's gang if alert_gang is set.
// uc_orig: PCOM_set_person_ai_kill_person (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_kill_person(Thing* p_person, Thing* p_target, SLONG alert_gang)
{
    if (p_person->Genus.Person->PersonType == PERSON_CIV) {
        ASSERT(0);
    }
    if (p_target->Genus.Person->PersonType == PERSON_DARCI) {
        if (p_person->Genus.Person->PersonType == PERSON_COP)
            ASSERT(0);
    }

    if ((p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS))
        return;

    if (am_i_a_thug(p_person)) {
        if (!am_i_a_thug(p_target))
            PCOM_call_cop_to_arrest_me(p_person, 1);
    }

    /*
            if(p_person->Genus.Person->pcom_ai_state    == PCOM_AI_STATE_KILLING)
                    return;
            if(p_person->Genus.Person->State    == STATE_CIRCLING)
                    return;
    */

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    set_face_thing(p_person, p_target);

    if (p_target->Genus.Person->PlayerID) {
        track_enemy(p_person);
    }

    PCOM_set_person_move_circle(p_person, p_target);

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_KILLING;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_target);

    if (alert_gang) {
        if (p_person->Genus.Person->pcom_bent & PCOM_BENT_GANG) {
            PCOM_alert_my_gang_to_a_fight(p_person, p_target);
        }
    }

    p_person->Genus.Person->Target = THING_NUMBER(p_target);
}

// Send person home. Clears target and starts navigation to HomeX/HomeZ.
// uc_orig: PCOM_set_person_ai_homesick (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_homesick(Thing* p_person)
{
    SLONG home_x = (p_person->Genus.Person->HomeX << 0);
    SLONG home_z = (p_person->Genus.Person->HomeZ << 0);

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));

        p_person->Genus.Person->Target = NULL;
    }

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_HOMESICK;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;

    PCOM_set_person_move_mav_to_xz(
        p_person,
        home_x,
        home_z,
        PCOM_MOVE_SPEED_WALK);
}

// Start exiting vehicle. Records the excar fields for what to do afterward.
// If the car is moving and person is driving, parks first; otherwise exits immediately.
// uc_orig: PCOM_set_person_ai_leavecar (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_leavecar(Thing* p_person, SLONG excar_state, SLONG excar_substate, SLONG excar_arg)
{
    Thing* p_vehicle;

    ASSERT(p_person->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_PASSENGER));

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    p_vehicle = TO_THING(p_person->Genus.Person->InCar);

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_LEAVECAR;
    p_person->Genus.Person->pcom_ai_arg = 0;
    p_person->Genus.Person->pcom_ai_substate = 0;
    p_person->Genus.Person->pcom_ai_counter = 0;

    p_person->Genus.Person->pcom_ai_excar_state = excar_state;
    p_person->Genus.Person->pcom_ai_excar_substate = excar_substate;
    p_person->Genus.Person->pcom_ai_excar_arg = excar_arg;

    if (p_vehicle->Velocity && (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING)) {
        PCOM_set_person_move_park_car_on_road(p_person);
    } else {
        PCOM_set_person_move_leavecar(p_person);
    }
}

// Make person investigate an unusual event at (odd_x, odd_z).
// Packs the grid coords into pcom_ai_arg. Helpless people are ignored.
// uc_orig: PCOM_set_person_ai_investigate (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_investigate(
    Thing* p_person,
    SLONG odd_x,
    SLONG odd_z)
{
    if ((p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS))
        return;
    //	FC_cam[1].focus=p_person;
    //	ASSERT(0);
    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    odd_x >>= 8;
    odd_z >>= 8;

    p_person->Genus.Person->pcom_ai_arg = (odd_x << 8) | (odd_z);
    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_INVESTIGATING;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_SUPRISED;
    p_person->Genus.Person->pcom_ai_counter = 0;

    set_face_pos(
        p_person,
        (odd_x << 8) + 0x80,
        (odd_z << 8) + 0x80);

    PCOM_set_person_move_still(p_person);
}

// Make person flee from a frightening position. If already fleeing that spot, just
// updates the packed pcom_ai_arg. State reuse avoids restart animation glitch.
// uc_orig: PCOM_set_person_ai_flee_place (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_flee_place(
    Thing* p_person,
    SLONG scary_x,
    SLONG scary_z)
{
    if ((p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS))
        return;
    /*
            play_quick_wave(
                    p_person,
                    S_ARGH,
                    WAVE_PLAY_INTERUPT);
    */
    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PLACE && p_person->Genus.Person->pcom_ai_substate == PCOM_AI_SUBSTATE_LEGIT) {
        scary_x >>= 8;
        scary_z >>= 8;

        p_person->Genus.Person->pcom_ai_arg = (scary_x << 8) | (scary_z);

        return;
    }

    scary_x >>= 8;
    scary_z >>= 8;

    p_person->Genus.Person->pcom_ai_arg = (scary_x << 8) | (scary_z);
    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_FLEE_PLACE;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_SUPRISED;
    p_person->Genus.Person->pcom_ai_counter = 0;

    set_face_pos(
        p_person,
        (scary_x << 8) + 0x80,
        (scary_z << 8) + 0x80);

    if (p_person->State == STATE_HIT_RECOIL) {
        // Wait to finish recoiling first.
    } else {
        PCOM_set_person_move_still(p_person);
    }
}

// Make person flee from p_scary.
// Civs remember who scared them (pcom_ai_memory). Thugs with FIGHT_BACK ignore this.
// If person is driving, leaves the car first via LEAVECAR state.
// uc_orig: PCOM_set_person_ai_flee_person (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_flee_person(Thing* p_person, Thing* p_scary)
{
    if (p_person->Genus.Person->pcom_ai != PCOM_AI_FLEE_PLAYER) {
        if (am_i_a_thug(p_person) && ((p_person->Genus.Person->pcom_bent & PCOM_BENT_FIGHT_BACK))) {
            return;
        }
    }

    if (p_person->Genus.Person->pcom_ai == PCOM_AI_CIV) {
        p_person->Genus.Person->pcom_ai_memory = THING_NUMBER(p_scary);

        if (p_person->Genus.Person->pcom_ai_state != PCOM_AI_STATE_FLEE_PERSON) {
            //			MFX_play_thing(THING_NUMBER(p_person),S_ARGH,MFX_REPLACE,p_person);
        }
    }

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
        PCOM_set_person_ai_leavecar(p_person, PCOM_EXCAR_FLEE_PERSON, 0, THING_NUMBER(p_scary));

        return;
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PERSON && p_person->Genus.Person->pcom_ai_substate == PCOM_AI_SUBSTATE_LEGIT) {
        p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_scary);

        return;
    }

    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_scary);
    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_FLEE_PERSON;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_SUPRISED;
    p_person->Genus.Person->pcom_ai_counter = 0;

    set_face_pos(
        p_person,
        p_scary->WorldPos.X >> 8,
        p_scary->WorldPos.Z >> 8);

    if (p_person->State == STATE_HIT_RECOIL) {
        // Wait to finish recoiling first.
    } else {
        PCOM_set_person_move_still(p_person);
    }
}

// Aimless drift — no target, no goal.
// uc_orig: PCOM_set_person_ai_aimless (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_aimless(Thing* p_person)
{
    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_AIMLESS;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
    p_person->Genus.Person->pcom_ai_arg = 0;
    p_person->Genus.Person->pcom_ai_counter = 0;

    PCOM_set_person_move_still(p_person);
}

// Switch into NAVTOKILL with gun already drawn (AIMING substate).
// Used when the target could shoot back and person has a gun.
// uc_orig: PCOM_set_person_ai_navtokill_shoot (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_navtokill_shoot(Thing* p_person, Thing* p_target)
{
    if (p_target->Genus.Person->PersonType == PERSON_DARCI) {
        if (p_person->Genus.Person->PersonType == PERSON_COP)
            ASSERT(0);
    }
    if (p_person->Genus.Person->PersonType == PERSON_CIV) {
        ASSERT(0);
    }
    PCOM_set_person_move_still(p_person);

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_NAVTOKILL;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_AIMING;
    p_person->Genus.Person->pcom_ai_counter = 0;
    p_person->Genus.Person->Target = THING_NUMBER(p_target);
}

// True if p_target is sprinting toward p_person within PCOM_SPRINTATME_DANGLE arc.
// uc_orig: PCOM_target_sprinting_towards_me (fallen/Source/pcom.cpp)
#define PCOM_SPRINTATME_DANGLE 512
SLONG PCOM_target_sprinting_towards_me(Thing* p_person, Thing* p_target)
{
    SLONG dangle = get_dangle(p_target, p_person);

    if (p_target->Genus.Person->Mode == PERSON_MODE_SPRINT) {
        if (dangle < PCOM_SPRINTATME_DANGLE || dangle > 2048 - PCOM_SPRINTATME_DANGLE) {
            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// Start navigating to and killing p_target.
// Switches to navtokill_shoot if target has a gun; flees if outgunned; checks zone restriction.
// uc_orig: PCOM_set_person_ai_navtokill (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_navtokill(Thing* p_person, Thing* p_target)
{
    if (p_target->Genus.Person->PersonType == PERSON_DARCI) {
        if (p_person->Genus.Person->PersonType == PERSON_COP)
            ASSERT(0);
    }
    if (p_person->Genus.Person->PersonType == PERSON_CIV) {
        //		ASSERT(0);
    }
    if ((p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS))
        return;

    if (am_i_a_thug(p_person)) {
        if (!am_i_a_thug(p_target))
            PCOM_call_cop_to_arrest_me(p_person, 1);
    }

    PCOM_set_person_move_mav_to_thing(p_person, p_target, PCOM_MOVE_SPEED_RUN);

    if (PCOM_target_could_shoot_me(p_person, p_target)) {
        if (PCOM_person_has_any_sort_of_gun(p_person) || (am_i_a_thug(p_person) && (p_person->Genus.Person->pcom_bent & PCOM_BENT_FIGHT_BACK))) {
            PCOM_set_person_ai_navtokill_shoot(p_person, p_target);

            return;
        } else {
            //			if(!am_i_a_thug(p_person)||(!(p_gang->Genus.Person->pcom_bent & PCOM_BENT_FIGHT_BACK)))
            if ((Random() & 0x4) && !(p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER) && (p_person->Genus.Person->PersonType != PERSON_COP)) {
                PCOM_set_person_ai_flee_person(p_person, p_target);

                return;
            }
        }
    } else if (PCOM_target_sprinting_towards_me(p_person, p_target)) {
        if (PCOM_person_has_any_sort_of_gun(p_person)) {
            PCOM_set_person_ai_navtokill_shoot(p_person, p_target);

            return;
        }
    }

    if (p_person->Genus.Person->pcom_bent & PCOM_BENT_RESTRICTED) {
        if (!MAV_do_found_dest) {
            PCOM_set_person_ai_normal(p_person);

            return;
        }
    }

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_NAVTOKILL;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_HUNTING;
    p_person->Genus.Person->pcom_ai_counter = 0;
    p_person->Genus.Person->Target = THING_NUMBER(p_target);
}

// Follow p_target. Checks whether player is reachable before committing.
// uc_orig: PCOM_set_person_ai_follow (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_follow(Thing* p_person, Thing* p_target)
{
    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    PCOM_set_person_move_mav_to_thing(p_person, p_target, PERSON_SPEED_RUN);

    if (p_target->Genus.Person->PlayerID && p_person->Genus.Person->pcom_move == PCOM_MOVE_FOLLOW) {
        if (p_person->Genus.Person->pcom_ai == PCOM_AI_BODYGUARD || p_person->Genus.Person->pcom_ai == PCOM_AI_CIV) {
            SLONG dx = p_person->WorldPos.X - p_target->WorldPos.X >> 8;
            SLONG dy = p_person->WorldPos.Y - p_target->WorldPos.Y >> 8;
            SLONG dz = p_person->WorldPos.Z - p_target->WorldPos.Z >> 8;

            SLONG dist = abs(dx) + abs(dz) + abs(dy + dy);

            if (dist > 0x600) {
                if (!MAV_do_found_dest) {
                    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_CANTFIND;

                    return;
                }
            }
        }
    }

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_FOLLOWING;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
    p_person->Genus.Person->pcom_ai_counter = 0;
}

// Locate a nearby unlocked, undamaged, unoccupied vehicle and navigate to it.
// If car == NULL, searches a sphere; if none found, switches to AIMLESS.
// uc_orig: PCOM_set_person_ai_findcar (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_findcar(Thing* p_person, UWORD car)
{
    SLONG speed;

    Thing* p_car;

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    if (car == NULL) {
        SLONG i;
        SLONG dx;
        SLONG dz;
        SLONG num;
        SLONG dist;
        SLONG best_dist = UC_INFINITY;

        num = THING_find_sphere(
            p_person->WorldPos.X >> 8,
            p_person->WorldPos.Y >> 8,
            p_person->WorldPos.Z >> 8,
            0xc00,
            THING_array,
            THING_ARRAY_SIZE,
            1 << CLASS_VEHICLE);

        for (i = 0; i < num; i++) {
            p_car = TO_THING(THING_array[i]);

            ASSERT(p_car->Class == CLASS_VEHICLE);

            if (p_car->Genus.Vehicle->Driver) {
                // Car already driven by someone.
            } else if (p_car->State == STATE_DEAD) {
                // Car is damaged.
            } else if (p_car->Genus.Vehicle->key != SPECIAL_NONE && !person_has_special(p_person, p_car->Genus.Vehicle->key)) {
                // This car is locked and we don't have the key.
            } else {
                dx = abs(p_car->WorldPos.X - p_person->WorldPos.X);
                dz = abs(p_car->WorldPos.Z - p_person->WorldPos.Z);

                dist = dx + dz;

                if (dist < best_dist) {
                    best_dist = dist;
                    car = THING_array[i];
                }
            }
        }
    }

    if (car == NULL) {
        PCOM_set_person_ai_aimless(p_person);
    } else {
        p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_FINDCAR;
        p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_GOTOCAR;
        p_person->Genus.Person->pcom_ai_arg = car;
        p_person->Genus.Person->pcom_ai_counter = 0;

        if (p_person->Genus.Person->pcom_bent & PCOM_BENT_LAZY) {
            speed = PERSON_SPEED_WALK;
        } else {
            speed = PERSON_SPEED_RUN;
        }

        PCOM_set_person_move_mav_to_thing(
            p_person,
            TO_THING(car),
            speed);
    }
}

// Navigate to p_bomb to defuse it.
// uc_orig: PCOM_set_person_ai_bdeactivate (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_bdeactivate(Thing* p_person, Thing* p_bomb)
{
    SLONG speed;

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_BDEACTIVATE;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_bomb);
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_GOTOBOMB;
    p_person->Genus.Person->pcom_ai_counter = 0;

    if (p_person->Genus.Person->pcom_bent & PCOM_BENT_LAZY) {
        speed = PERSON_SPEED_WALK;
    } else {
        speed = PERSON_SPEED_RUN;
    }

    PCOM_set_person_move_mav_to_thing(
        p_person,
        p_bomb,
        speed);
}

// Draw weapon and enter sniper mode on p_target.
// uc_orig: PCOM_set_person_ai_snipe (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_snipe(Thing* p_person, Thing* p_target)
{
    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_SNIPE;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LOOK;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_ai_counter = 0;

    //	PCOM_set_person_move_still(p_person);

    PCOM_set_person_move_draw_gun(p_person);
}

// Begin warm-hands loop — person seeks a fire to stand beside.
// uc_orig: PCOM_set_person_ai_warm_hands (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_warm_hands(Thing* p_person)
{
    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }
    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_WARM_HANDS;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_GOTOFIRE;
    p_person->Genus.Person->pcom_ai_arg = 0;
    p_person->Genus.Person->pcom_ai_counter = 0;

    PCOM_set_person_move_pause(p_person);
}

// Make person surrender (hands-up animation) facing the cop.
// Sets FLAG_PERSON_NO_RETURN_TO_NORMAL so normal AI won't resume.
// uc_orig: PCOM_set_person_ai_hands_up (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_hands_up(Thing* p_person, Thing* p_cop)
{
    set_face_thing(p_person, p_cop);

    PCOM_set_person_move_animation(p_person, ANIM_HANDS_UP);

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_HANDS_UP;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_cop);
    p_person->Genus.Person->pcom_ai_counter = 0;
    p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
}

// Start a conversation animation between two people.
// talk_substate selects ASK / TELL / LISTEN. Shotgun holders use idle anim.
// uc_orig: PCOM_set_person_ai_talk_to (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_talk_to(Thing* p_person, Thing* p_person_talked_at, UBYTE talk_substate, UBYTE stay_looking_at_eachother)
{
    UWORD anim;

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    set_face_thing(p_person, p_person_talked_at);

    switch (talk_substate) {
    case PCOM_AI_SUBSTATE_TALK_ASK:
        anim = ANIM_TALK_ASK;
        break;
    case PCOM_AI_SUBSTATE_TALK_TELL:
        anim = ANIM_TALK_TELL;
        break;
    case PCOM_AI_SUBSTATE_TALK_LISTEN:
        anim = ANIM_TALK_LISTEN;
        break;

    default:
        ASSERT(0);
        break;
    }
    if (person_holding_2handed(p_person) && p_person->Genus.Person->PersonType != PERSON_ROPER)
        anim = ANIM_SHOTGUN_IDLE;

    PCOM_set_person_move_animation(p_person, anim);

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_TALK;
    p_person->Genus.Person->pcom_ai_substate = talk_substate;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_person_talked_at);
    p_person->Genus.Person->pcom_ai_counter = 0;

    p_person->Genus.Person->Flags &= ~FLAG_PERSON_NO_RETURN_TO_NORMAL;

    if (stay_looking_at_eachother) {
        p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
    }
}

// Hitchhike — person navigates to vehicle door hoping for a ride.
// uc_orig: PCOM_set_person_ai_hitch (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_hitch(Thing* p_person, Thing* p_vehicle)
{
    SLONG speed;

    if (p_person->Genus.Person->Target) {
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_HITCH;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_GOTOCAR;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_vehicle);
    p_person->Genus.Person->pcom_ai_counter = 0;

    if (p_person->Genus.Person->pcom_bent & PCOM_BENT_LAZY) {
        speed = PERSON_SPEED_WALK;
    } else {
        speed = PERSON_SPEED_RUN;
    }

    PCOM_set_person_move_mav_to_thing(
        p_person,
        p_vehicle,
        speed);
}

// Start a taunt at p_target. Reacts to gun threat before committing to taunt animation.
// Cops are excluded. People currently grappled cannot taunt.
// uc_orig: PCOM_set_person_ai_taunt (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_taunt(Thing* p_person, Thing* p_target)
{
    if ((p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS))
        return;
    // cops shouldnt taunt, I've seen them do it with my own eyes MIKED
    if (p_person->Genus.Person->PersonType == PERSON_COP)
        return;

    if (p_person->SubState == SUB_STATE_GRAPPLEE || p_person->SubState == SUB_STATE_GRAPPLE_HELD) {
        return;
    }

    if (p_person->Genus.Person->Target) {
        // I disagree you should concentrate on the job MikeD.
        // return;
        remove_from_gang_attack(p_person, TO_THING(p_person->Genus.Person->Target));
    }

    if (PCOM_target_could_shoot_me(p_person, p_target)) {
        if (PCOM_person_has_any_sort_of_gun(p_person) || (Random() & 3) == 0) {
            PCOM_set_person_ai_navtokill_shoot(p_person, p_target);

            return;
        } else {
            if (Random() & 0x4) {
                PCOM_set_person_ai_flee_person(p_person, p_target);

                return;
            }
        }
    }

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_TAUNT;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_ai_substate = 0;
    p_person->Genus.Person->pcom_ai_counter = 0;

    set_face_thing(p_person, p_target);

    //	MFX_play_thing(THING_NUMBER(p_person),S_WANKER,MFX_REPLACE,p_person);

    PCOM_set_person_move_animation(p_person, ANIM_WANKER);
}

// Begin Bane's SUMMON sequence: locate up to 4 nearby corpses, then float up.
// uc_orig: PCOM_set_person_ai_summon (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_summon(Thing* p_person)
{
    SLONG i;
    SLONG num;
    SLONG bodies;

    num = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        0x800,
        THING_array,
        THING_ARRAY_SIZE,
        1 << CLASS_PERSON);

    memset(PCOM_summon, 0, sizeof(PCOM_summon));

    bodies = 0;

    for (i = 0; i < num; i++) {
        Thing* p_found = TO_THING(THING_array[i]);

        ASSERT(p_found->Class == CLASS_PERSON);

        if (p_found->Genus.Person->pcom_ai == PCOM_AI_NONE || p_found->Genus.Person->pcom_ai == PCOM_AI_SUICIDE) {
            if (p_found->Genus.Person->PlayerID == 0) {
                ASSERT(WITHIN(bodies, 0, PCOM_SUMMON_NUM_BODIES - 1));

                PCOM_summon[bodies] = THING_array[i];

                bodies += 1;

                if (bodies == PCOM_SUMMON_NUM_BODIES) {
                    break;
                }
            }
        }
    }

    set_person_float_up(p_person);

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_SUMMON;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_SUMMON_START;
}

// PersonIsMIB — forward declaration (defined in Person.cpp / combat.cpp).
// The commented-out body below is the original definition, later moved.
// uc_orig: PersonIsMIB (fallen/Source/pcom.cpp)
extern BOOL PersonIsMIB(Thing* p_person);
/*
BOOL PersonIsMIB(Thing* p_person)
{
        return (p_person->Genus.Person->PersonType == PERSON_MIB1 ||
                p_person->Genus.Person->PersonType == PERSON_MIB2 ||
                p_person->Genus.Person->PersonType == PERSON_MIB3);
};
*/

// Scan nearby for a weapon special that can be safely reached.
// MIBs and people already armed skip this check.
// Returns the item Thing* or NULL.
// uc_orig: PCOM_is_there_an_item_i_should_get (fallen/Source/pcom.cpp)
Thing* PCOM_is_there_an_item_i_should_get(Thing* p_person)
{
    UWORD ans;

    if (PersonIsMIB(p_person)) {
        // MIBs have an ak47 for an arm.
        return NULL;
    }

    if (p_person->Genus.Person->SpecialList || (p_person->Flags & FLAGS_HAS_GUN)) {
        // This person has a special or a gun, so he doesn't need to pick up anything.
        return NULL;
    }

    if (p_person->Genus.Person->Ware) {
        // People in warehouses don't pickup specials.
        return NULL;
    }

    ans = THING_find_nearest(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        0x300,
        1 << CLASS_SPECIAL);

    if (ans) {
        Thing* p_special = TO_THING(ans);

        switch (p_special->Genus.Special->SpecialType) {
        case SPECIAL_GUN:
        case SPECIAL_SHOTGUN:
        case SPECIAL_AK47:
        case SPECIAL_BASEBALLBAT:
        case SPECIAL_KNIFE:
            break;

        default:
            return NULL;
        }

        if (!there_is_a_los_mav(
                p_person->WorldPos.X >> 8,
                p_person->WorldPos.Y + 0x4000 >> 8,
                p_person->WorldPos.Z >> 8,
                p_special->WorldPos.X >> 8,
                p_special->WorldPos.Y + 0x4000 >> 8,
                p_special->WorldPos.Z >> 8)) {
            return p_special;
        }
    }

    return NULL;
}

// Start pick-up sequence for p_special. Stores excar state for after retrieval.
// uc_orig: PCOM_set_person_ai_getitem (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_getitem(Thing* p_person, Thing* p_special, SLONG move_speed, SLONG excar_state, SLONG excar_arg)
{
    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_GETITEM;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
    p_person->Genus.Person->pcom_ai_arg = THING_NUMBER(p_special);
    p_person->Genus.Person->pcom_ai_excar_state = excar_state;
    p_person->Genus.Person->pcom_ai_excar_substate = NULL;
    p_person->Genus.Person->pcom_ai_excar_arg = excar_arg;

    PCOM_set_person_move_mav_to_xz(
        p_person,
        p_special->WorldPos.X >> 8,
        p_special->WorldPos.Z >> 8,
        move_speed);
}

// Tick function for GETITEM state: wait for arrival, pick up, then resume prior state.
// uc_orig: PCOM_process_getitem (fallen/Source/pcom.cpp)
void PCOM_process_getitem(Thing* p_person)
{
    Thing* p_special = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_ANIMATION:
        break;

    case PCOM_MOVE_STATE_GOTO_XZ:

        if ((PTIME(p_person) & 0x3) == 0) {
            if (PCOM_finished_nav(p_person)) {
                if (p_special->Flags & FLAGS_ON_MAPWHO) {
                    PCOM_set_person_move_pickup_special(p_person, p_special);
                } else {
                    // Someone must have got the special first.
                    PCOM_set_person_move_pause(p_person);
                }
            }
        }

        break;

    case PCOM_MOVE_STATE_PAUSE:

        if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(10)) {
            // Fall through!
        } else {
            break;
        }

    case PCOM_MOVE_STATE_STILL:

        switch (p_person->Genus.Person->pcom_ai_excar_state) {
        case PCOM_EXCAR_NORMAL:
            PCOM_set_person_ai_normal(p_person);
            break;

        case PCOM_EXCAR_NAVTOKILL:
            PCOM_set_person_ai_navtokill(p_person, TO_THING(p_person->Genus.Person->pcom_ai_excar_arg));
            break;

        default:
            ASSERT(0);
            break;
        }

        break;

    default:

        ASSERT(0);

        PCOM_set_person_ai_normal(p_person);

        break;
    }
}

// Tick function for Bane's SUMMON state.
// SUMMON_START: wait for FLOAT_BOB anim, then levitate up to 4 bodies and create SPARK arcs.
// SUMMON_FLOAT: recreate sparks every PCOM_get_duration(50) ticks.
//   If Darci lingers within 0x60000 units for PCOM_get_duration(20) ticks (~2s): electrocute her.
//   pcom_move_counter halves if Darci moves (rewards movement).
// uc_orig: PCOM_process_summon (fallen/Source/pcom.cpp)
void PCOM_process_summon(Thing* p_person)
{
    SLONG i;

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_SUMMON_START:

        if (p_person->SubState == SUB_STATE_FLOAT_BOB) {
            for (i = 0; i < PCOM_SUMMON_NUM_BODIES; i++) {
                if (PCOM_summon[i]) {
                    Thing* p_summon = TO_THING(PCOM_summon[i]);

                    set_person_float_up(p_summon);

                    SPARK_Pinfo p1;
                    SPARK_Pinfo p2;

                    static UBYTE limb[4] = {
                        SUB_OBJECT_LEFT_HAND,
                        SUB_OBJECT_RIGHT_HAND,
                        SUB_OBJECT_LEFT_FOOT,
                        SUB_OBJECT_RIGHT_FOOT,
                    };

                    p1.type = SPARK_TYPE_LIMB;
                    p1.flag = 0;
                    p1.person = THING_NUMBER(p_person);
                    p1.limb = limb[i];

                    p2.type = SPARK_TYPE_LIMB;
                    p2.flag = 0;
                    p2.person = THING_NUMBER(p_summon);
                    p2.limb = SUB_OBJECT_PELVIS;

                    SPARK_create(
                        &p1,
                        &p2,
                        255);
                }
            }

            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_SUMMON_FLOAT;
            p_person->Genus.Person->pcom_ai_counter = 0;
        }

        break;

    case PCOM_AI_SUBSTATE_SUMMON_FLOAT:

        if (p_person->Genus.Person->pcom_ai_counter > PCOM_get_duration(50)) {
            for (i = 0; i < PCOM_SUMMON_NUM_BODIES; i++) {
                if (PCOM_summon[i]) {
                    Thing* p_summon = TO_THING(PCOM_summon[i]);

                    SPARK_Pinfo p1;
                    SPARK_Pinfo p2;

                    static UBYTE limb[4] = {
                        SUB_OBJECT_LEFT_HAND,
                        SUB_OBJECT_RIGHT_HAND,
                        SUB_OBJECT_LEFT_FOOT,
                        SUB_OBJECT_RIGHT_FOOT,
                    };

                    p1.type = SPARK_TYPE_LIMB;
                    p1.flag = 0;
                    p1.person = THING_NUMBER(p_person);
                    p1.limb = limb[i];

                    p2.type = SPARK_TYPE_LIMB;
                    p2.flag = 0;
                    p2.person = THING_NUMBER(p_summon);
                    p2.limb = SUB_OBJECT_PELVIS;

                    SPARK_create(
                        &p1,
                        &p2,
                        255);
                }
            }

            p_person->Genus.Person->pcom_ai_counter = 0;
        }

        p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        {
            Thing* darci = NET_PERSON(0);

            SLONG dx;
            SLONG dz;

            dx = abs(darci->WorldPos.X - p_person->WorldPos.X);
            dz = abs(darci->WorldPos.Z - p_person->WorldPos.Z);

            if (QDIST2(dx, dz) < 0x60000) {
                if ((darci->WorldPos.X >> 16) != (p_person->Genus.Person->pcom_ai_arg >> 8)) {
                    p_person->Genus.Person->pcom_move_counter >>= 1;
                }

                if ((darci->WorldPos.Z >> 16) != (p_person->Genus.Person->pcom_ai_arg & 0xff)) {
                    p_person->Genus.Person->pcom_move_counter >>= 1;
                }

                p_person->Genus.Person->pcom_ai_arg = darci->WorldPos.Z >> 16;
                p_person->Genus.Person->pcom_ai_arg |= (darci->WorldPos.X >> 16) & 0xff;

                p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
            } else {
                p_person->Genus.Person->pcom_move_counter = 0;
            }

            if (p_person->Genus.Person->pcom_move_counter >= PCOM_get_duration(20)) {
                if (darci->State == STATE_IDLE || darci->State == STATE_MOVEING || darci->State == STATE_GUN) {
                    set_person_recoil(darci, ANIM_HIT_FRONT_MID, 0);

                    darci->Genus.Person->Health -= 25;

                    SPARK_Pinfo p1;
                    SPARK_Pinfo p2;

                    p1.type = SPARK_TYPE_LIMB;
                    p1.flag = 0;
                    p1.person = THING_NUMBER(p_person);
                    p1.limb = SUB_OBJECT_PELVIS;

                    p2.type = SPARK_TYPE_LIMB;
                    p2.flag = 0;
                    p2.person = THING_NUMBER(darci);
                    p2.limb = SUB_OBJECT_PELVIS;

                    SPARK_create(
                        &p1,
                        &p2,
                        50);

                    p_person->Genus.Person->pcom_move_counter = 0;
                }
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// Allocate and configure a new NPC with full pcom attributes.
// Arms the person from the pcom_has bitmask. Registers gang membership if PCOM_BENT_GANG.
// Guard-in-zone → silently converts to PCOM_AI_GANG (zone AI).
// uc_orig: PCOM_create_person (fallen/Source/pcom.cpp)
THING_INDEX PCOM_create_person(
    SLONG type,
    SLONG colour,
    SLONG group,
    SLONG ai,
    SLONG ai_other,
    SLONG ai_skill,
    SLONG move,
    SLONG move_follow,
    SLONG bent,
    SLONG pcom_has,
    SLONG drop,
    SLONG pcom_zone,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG yaw,
    SLONG random,
    ULONG flag1,
    ULONG flag2)
{
    if (pcom_has & (PCOM_HAS_SHOTGUN | PCOM_HAS_KNIFE | PCOM_HAS_BASEBALLBAT)) {
        //		type = PERSON_DARCI;
    }

    if (pcom_has & PCOM_HAS_GUN) {
        if (drop == SPECIAL_GUN) {
            drop = 0;
        }
    }
    if (ai == PCOM_AI_ASSASIN) {
    }

    if (ai == PCOM_AI_ASSASIN && move == PCOM_MOVE_WANDER) {
        // Wandering assassins follow their target instead.
        move = PCOM_MOVE_FOLLOW;
        move_follow = ai_other;
    }

    //	ASSERT(type!=PERSON_CIV);

    THING_INDEX p_index = create_person(
        type,
        random,
        world_x,
        world_y,
        world_z);

    if (p_index == NULL) {
    } else {
        Thing* p_person = TO_THING(p_index);

        if (type == PERSON_COP) {
            if (ai == PCOM_AI_DRIVER)
                ai = PCOM_AI_COP_DRIVER;
        }

        p_person->Draw.Tweened->Angle = yaw;
        p_person->Genus.Person->HomeYaw = yaw >> 3;

        p_person->Genus.Person->pcom_colour = colour;
        p_person->Genus.Person->pcom_group = group;
        p_person->Genus.Person->pcom_ai = ai;
        p_person->Genus.Person->pcom_move = move;
        p_person->Genus.Person->pcom_bent = bent;
        p_person->Genus.Person->drop = drop;
        p_person->Genus.Person->pcom_zone = pcom_zone & 0xf;

        p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_NORMAL;
        p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
        p_person->Genus.Person->pcom_ai_arg = NULL;
        p_person->Genus.Person->pcom_ai_other = ai_other;

        p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
        p_person->Genus.Person->pcom_move_counter = 0;
        p_person->Genus.Person->pcom_move_arg = 0;
        p_person->Genus.Person->pcom_move_follow = move_follow;

        p_person->Genus.Person->FightRating = 0;
        p_person->Genus.Person->Flags |= flag1;
        p_person->Genus.Person->Flags2 |= flag2;

        // Guards in zones become gang members (zone AI).
        if (p_person->Genus.Person->pcom_zone && p_person->Genus.Person->pcom_ai == PCOM_AI_GUARD) {
            p_person->Genus.Person->pcom_ai = PCOM_AI_GANG;
        }

        SET_SKILL(p_person, ai_skill);

        if (pcom_has & (PCOM_HAS_SHOTGUN | PCOM_HAS_GUN)) {
            //			p_person->Genus.Person->pcom_bent |=PCOM_BENT_KILL_ON_SIGHT;
        }

        if (pcom_has & PCOM_HAS_GUN) {
            p_person->Flags |= FLAGS_HAS_GUN;
        }

        if (pcom_has & PCOM_HAS_SHOTGUN) {
            Thing* p_special = alloc_special(
                SPECIAL_SHOTGUN,
                SPECIAL_SUBSTATE_NONE,
                0, 0, 0, NULL);
            if (p_special) {
                if (should_person_get_item(p_person, p_special)) {
                    person_get_item(p_person, p_special);
                    set_person_draw_item(p_person, SPECIAL_SHOTGUN);
                }
            }
        }

        if (pcom_has & PCOM_HAS_AK47) {
            Thing* p_special = alloc_special(
                SPECIAL_AK47,
                SPECIAL_SUBSTATE_NONE,
                0, 0, 0, NULL);

            if (p_special) {
                if (should_person_get_item(p_person, p_special)) {
                    person_get_item(p_person, p_special);
                    set_person_draw_item(p_person, SPECIAL_AK47);
                }
            }
        }

        if (pcom_has & PCOM_HAS_KNIFE) {
            Thing* p_special = alloc_special(
                SPECIAL_KNIFE,
                SPECIAL_SUBSTATE_NONE,
                0, 0, 0, NULL);

            if (p_special) {
                if (should_person_get_item(p_person, p_special)) {
                    person_get_item(p_person, p_special);
                    set_person_draw_item(p_person, SPECIAL_KNIFE);
                }
            }
        }

        if (pcom_has & PCOM_HAS_BASEBALLBAT) {
            Thing* p_special = alloc_special(
                SPECIAL_BASEBALLBAT,
                SPECIAL_SUBSTATE_NONE,
                0, 0, 0, NULL);

            if (p_special) {
                if (should_person_get_item(p_person, p_special)) {
                    person_get_item(p_person, p_special);
                    set_person_draw_item(p_person, SPECIAL_BASEBALLBAT);
                }
            }
        }

        if (pcom_has & PCOM_HAS_GRENADE) {
            Thing* p_special = alloc_special(
                SPECIAL_GRENADE,
                SPECIAL_SUBSTATE_NONE,
                0, 0, 0, NULL);

            if (p_special) {
                if (should_person_get_item(p_person, p_special)) {
                    person_get_item(p_person, p_special);
                    set_person_draw_item(p_person, SPECIAL_GRENADE);
                }
            }
        }

        if (bent & PCOM_BENT_GANG) {
            PCOM_add_gang_member(p_index, colour);
        }
    }

    return p_index;
}

// uc_orig: PCOM_create_player (fallen/Source/pcom.cpp)
THING_INDEX PCOM_create_player(
    SLONG type,
    SLONG pcom_has,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG id,
    SLONG yaw)
{
    Thing* p_person = create_player(
        type,
        world_x,
        world_y,
        world_z,
        id);

    extern SLONG playing_level(const CBYTE* name);

    if (playing_level("skymiss2.ucm")) {
        pcom_has |= PCOM_HAS_SHOTGUN;
    }

    if (p_person) {
        p_person->Draw.Tweened->Angle = yaw;

        p_person->Genus.Person->pcom_ai = PCOM_AI_NONE;
        p_person->Genus.Person->pcom_move = PCOM_MOVE_STILL;
        p_person->Genus.Person->pcom_bent = 0;

        p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_PLAYER;
        p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
        p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_PLAYER;

        if (pcom_has & PCOM_HAS_GUN) {
            p_person->Flags |= FLAGS_HAS_GUN;
        }

        if (pcom_has & PCOM_HAS_SHOTGUN) {
            Thing* p_special = alloc_special(
                SPECIAL_SHOTGUN,
                SPECIAL_SUBSTATE_NONE,
                0, 0, 0, NULL);
            if (p_special) {
                if (should_person_get_item(p_person, p_special)) {
                    person_get_item(p_person, p_special);
                    set_person_draw_item(p_person, SPECIAL_SHOTGUN);
                }
            }
        }

        if (pcom_has & PCOM_HAS_BALLOON) {
            p_person->Genus.Person->Balloon = BALLOON_create(THING_NUMBER(p_person), BALLOON_TYPE_YELLOW);
        }

        return THING_NUMBER(p_person);
    }

    return NULL;
}

// uc_orig: PCOM_change_person_attributes (fallen/Source/pcom.cpp)
void PCOM_change_person_attributes(
    Thing* p_person,
    SLONG colour,
    SLONG group,
    SLONG ai,
    SLONG ai_other,
    SLONG move,
    SLONG move_follow,
    SLONG bent,
    SLONG yaw)
{
    if (is_person_dead(p_person)) {
        return;
    }

    p_person->Genus.Person->pcom_colour = colour;
    p_person->Genus.Person->pcom_group = group;
    p_person->Genus.Person->pcom_ai = ai;
    p_person->Genus.Person->pcom_move = move;
    p_person->Genus.Person->pcom_bent = bent;

    p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_NORMAL;
    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
    p_person->Genus.Person->pcom_ai_arg = NULL;
    p_person->Genus.Person->pcom_ai_other = ai_other;

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
    p_person->Genus.Person->pcom_move_counter = 0;
    p_person->Genus.Person->pcom_move_arg = 0;
    p_person->Genus.Person->pcom_move_follow = move_follow;

    if (p_person->Genus.Person->pcom_move == PCOM_MOVE_HANDS_UP) {
        void drop_current_gun(Thing * p_person, SLONG change_anim);

        drop_current_gun(p_person, 0);
    }

    if (p_person->Genus.Person->pcom_move == PCOM_MOVE_STILL || p_person->Genus.Person->pcom_move == PCOM_MOVE_DANCE || p_person->Genus.Person->pcom_move == PCOM_MOVE_HANDS_UP || p_person->Genus.Person->pcom_move == PCOM_MOVE_TIED_UP) {
        p_person->Genus.Person->HomeX = p_person->WorldPos.X >> 8;
        p_person->Genus.Person->HomeZ = p_person->WorldPos.Z >> 8;
        /*
        p_person->Genus.Person->HomeYaw = yaw >> 3;
        p_person->Draw.Tweened->Angle   = yaw;
        */
    }

    PCOM_set_person_ai_normal(p_person);
}

// Returns the zone bitmask (4-bit) for the PAP cell at the person's destination navsquare.
// NPCs with a non-zero pcom_zone field only perceive targets within their own zone.
// uc_orig: PCOM_get_zone_for_position (fallen/Source/pcom.cpp)
UBYTE PCOM_get_zone_for_position(Thing* p_person)
{
    UBYTE zone;

    SLONG dest_x;
    SLONG dest_z;

    PAP_Hi* ph;

    PCOM_get_person_navsquare(
        p_person,
        &dest_x,
        &dest_z);

    ph = &PAP_2HI(dest_x, dest_z);

    ASSERT(PAP_FLAG_ZONE1 == (1 << 10));

    zone = ph->Flags >> 10;

    return zone;
}

// Variant that takes a world position directly instead of a person pointer.
// uc_orig: PCOM_get_zone_for_position (fallen/Source/pcom.cpp)
UBYTE PCOM_get_zone_for_position(SLONG x, SLONG z)
{
    UBYTE zone;

    PAP_Hi* ph;

    ph = &PAP_2HI(x >> 8, z >> 8);

    ASSERT(PAP_FLAG_ZONE1 == (1 << 10));

    zone = ph->Flags >> 10;

    return zone;
}

// Returns the player if this NPC can see them and they share the same zone.
// uc_orig: PCOM_can_i_see_person_to_attack (fallen/Source/pcom.cpp)
Thing* PCOM_can_i_see_person_to_attack(Thing* p_person)
{
    Thing* p_target = NET_PERSON(0);

    if (p_target->State == STATE_DEAD || p_target->State == STATE_DYING || stealth_debug) {
        return NULL;
    }

    if (p_person->Genus.Person->pcom_zone) {
        if (PCOM_get_zone_for_position(p_target) & p_person->Genus.Person->pcom_zone) {
            // Target is in our zone.
        } else {
            return NULL;
        }
    }

    if (can_a_see_b(p_person, p_target)) {
        return p_target;
    }

    return NULL;
}

// Finds the nearest visible civ or player to intimidate.
// Only considers wandering civs and players (unless PCOM_BENT_ONLY_KILL_PLAYER).
// uc_orig: PCOM_can_i_see_person_to_bully (fallen/Source/pcom.cpp)
Thing* PCOM_can_i_see_person_to_bully(Thing* p_person)
{
    SLONG i;

    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG score;

    SLONG best_score = UC_INFINITY;
    Thing* best_thing = NULL;

    Thing* p_found;

    PCOM_found_num = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        0x600,
        PCOM_found,
        PCOM_MAX_FIND,
        1 << CLASS_PERSON);

    for (i = 0; i < PCOM_found_num; i++) {
        p_found = TO_THING(PCOM_found[i]);

        if (p_found == p_person) {
            continue;
        }

        if (p_found->State == STATE_DEAD || p_found->State == STATE_DYING) {
            continue;
        }

        if (p_person->Genus.Person->pcom_bent & PCOM_BENT_ONLY_KILL_PLAYER) {
            if (!p_found->Genus.Person->PlayerID) {
                continue;
            }
        } else {
            if (p_found->Genus.Person->pcom_ai == PCOM_AI_CIV && p_found->Genus.Person->pcom_move == PCOM_MOVE_WANDER) {
                // wandering civ — valid bully target
            } else if (p_found->Genus.Person->PlayerID) {
                // player — valid bully target
            } else {
                continue;
            }
        }

        if (can_a_see_b(p_person, p_found)) {
            dx = abs(p_person->WorldPos.X - p_found->WorldPos.X);
            dy = abs(p_person->WorldPos.Y - p_found->WorldPos.Y);
            dz = abs(p_person->WorldPos.Z - p_found->WorldPos.Z);

            score = dx + dy + dy + dz; // y-axis weighted double

            if (p_found->Genus.Person->PlayerID) {
                score >>= 1; // more likely to bully player
            }

            if (best_score > score) {
                best_score = score;
                best_thing = p_found;
            }
        }
    }

    return best_thing;
}

// Deferred arrest queue — persons queued via store_it=1 are processed at frame end by do_arrests().
// Using a queue avoids modifying NPC state mid-frame while iterating the person list.
// uc_orig: init_arrest (fallen/Source/pcom.cpp)
void init_arrest(void)
{
    next_arrest = 0;
}

// uc_orig: do_arrests (fallen/Source/pcom.cpp)
void do_arrests(void)
{
    SLONG c0;
    for (c0 = 0; c0 < next_arrest; c0++) {
        PCOM_call_cop_to_arrest_me(arrest_me[c0], 0);
    }

    next_arrest = 0;
}

// Finds a nearby cop NPC and instructs it to arrest p_person.
// If store_it is true, queues the request for end-of-frame processing via do_arrests().
// uc_orig: PCOM_call_cop_to_arrest_me (fallen/Source/pcom.cpp)
SLONG PCOM_call_cop_to_arrest_me(Thing* p_person, SLONG store_it)
{
    SLONG i;

    Thing* p_found;
    SLONG found_cop = 0;

    if (store_it) {
        if (next_arrest >= MAX_ARREST_ME) {
            ASSERT(0);
            return (0);
        }

        arrest_me[next_arrest] = p_person;
        next_arrest++;
        return (0);
    }

    if (p_person->Genus.Person->PersonType == PERSON_TRAMP)
        return (0);

    PCOM_found_num = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        0x800,
        PCOM_found,
        PCOM_MAX_FIND,
        (1 << CLASS_PERSON) | (1 << CLASS_VEHICLE));

    for (i = 0; i < PCOM_found_num; i++) {
        p_found = TO_THING(PCOM_found[i]);

        if (p_found == p_person) {
            continue;
        }

        if (p_found->State == STATE_DEAD || p_found->State == STATE_DYING) {
            continue;
        }

        if (p_found->Class == CLASS_VEHICLE) {
            if (p_found->Genus.Vehicle->Driver) {
                p_found = TO_THING(p_found->Genus.Vehicle->Driver);
            } else {
                continue;
            }
        }

        if (p_found->Genus.Person->pcom_ai == PCOM_AI_COP || p_found->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER) {
            if (PCOM_person_doing_nothing_important(p_found))
                switch (p_found->Genus.Person->pcom_ai_state) {
                case PCOM_AI_STATE_INVESTIGATING:
                case PCOM_AI_STATE_NORMAL:
                case PCOM_AI_STATE_WARM_HANDS:
                case PCOM_AI_STATE_HOMESICK:
                    if (can_a_see_b(p_person, p_found)) {
                        p_person->Genus.Person->Flags |= FLAG_PERSON_FELON;
                        found_cop = 1;

                        if (p_found->Genus.Person->pcom_ai == PCOM_AI_COP || p_found->Genus.Person->InCar == 0) {
                            PCOM_set_person_ai_arrest(p_found, p_person);
                        } else {
                            PCOM_set_person_ai_leavecar(
                                p_found,
                                PCOM_EXCAR_ARREST_PERSON,
                                0,
                                THING_NUMBER(p_person));
                        }
                    }
                    break;
                }
        }
    }

    return found_cop;
}

// PCOM_can_i_see_person_to_arrest — fully commented out in original source, dead code.

// Returns the player if this NPC has line-of-sight to taunt them.
// uc_orig: PCOM_can_i_see_person_to_taunt (fallen/Source/pcom.cpp)
Thing* PCOM_can_i_see_person_to_taunt(Thing* p_person)
{
    SLONG i;

    for (i = 0; i < NO_PLAYERS; i++) {
        Thing* p_target = NET_PERSON(0);

        if (p_target->State == STATE_DEAD || p_target->State == STATE_DYING || stealth_debug) {
            continue;
        }

        if (can_a_see_b(p_person, p_target)) {
            return p_target;
        }
    }

    return NULL;
}

// Returns the next patrol waypoint index for this person (wraps around or picks random).
// uc_orig: PCOM_get_next_patrol_waypoint (fallen/Source/pcom.cpp)
SLONG PCOM_get_next_patrol_waypoint(Thing* p_person)
{
    SLONG waypoint;

    if (p_person->Genus.Person->pcom_move == PCOM_MOVE_PATROL) {
        waypoint = EWAY_find_waypoint(
            p_person->Genus.Person->pcom_move_arg + 1,
            EWAY_DONT_CARE,
            p_person->Genus.Person->pcom_colour,
            p_person->Genus.Person->pcom_group,
            UC_TRUE);
    } else {
        waypoint = EWAY_find_waypoint_rand(
            p_person->Genus.Person->pcom_move_arg,
            p_person->Genus.Person->pcom_colour,
            p_person->Genus.Person->pcom_group,
            UC_TRUE);
    }

    return waypoint;
}

// AI tick for a person in a parked vehicle (PCOM_MOVE_STILL).
// Applies brakes until fully stopped.
// uc_orig: PCOM_process_driving_still (fallen/Source/pcom.cpp)
void PCOM_process_driving_still(Thing* p_person)
{
    ASSERT(p_person->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING));

    Thing* p_vehicle;

    p_vehicle = TO_THING(p_person->Genus.Person->InCar);

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:

        if (p_vehicle->Velocity != 0) {
            PCOM_set_person_move_park_car_on_road(p_person);
        }

        break;

    case PCOM_MOVE_STATE_PARK_CAR:
    case PCOM_MOVE_STATE_PARK_BIKE:

        if ((p_vehicle->Class == CLASS_VEHICLE && p_vehicle->Velocity == 0)

        ) {
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
            p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_NONE;
            p_person->Genus.Person->pcom_move_arg = 0;
            p_person->Genus.Person->pcom_move_counter = 0;
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// AI tick for a person driving between EWAY waypoints.
// Advances to the next waypoint on arrival; parks permanently at delay==10s waypoints.
// uc_orig: PCOM_process_driving_patrol (fallen/Source/pcom.cpp)
void PCOM_process_driving_patrol(Thing* p_person)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;
    SLONG waypoint;

    SLONG dest_x;
    SLONG dest_z;

    Thing* p_vehicle;

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:

        waypoint = EWAY_find_nearest_waypoint(
            p_person->WorldPos.X >> 8,
            p_person->WorldPos.Y >> 8,
            p_person->WorldPos.Z >> 8,
            p_person->Genus.Person->pcom_colour,
            p_person->Genus.Person->pcom_group);

        if (waypoint != EWAY_NO_MATCH) {
            if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                PCOM_set_person_move_driveto(p_person, waypoint);
            } else {
                ASSERT(0);
            }
        }

        break;

    case PCOM_MOVE_STATE_DRIVETO:
    case PCOM_MOVE_STATE_BIKETO:

        ASSERT(p_person->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING));

        PCOM_get_person_dest(
            p_person,
            &dest_x,
            &dest_z);

        p_vehicle = TO_THING(p_person->Genus.Person->InCar);

        dx = dest_x - (p_vehicle->WorldPos.X >> 8);
        dz = dest_z - (p_vehicle->WorldPos.Z >> 8);

        dist = QDIST2(abs(dx), abs(dz));

        if (dist < 0x300) {
            if (EWAY_get_delay(p_person->Genus.Person->pcom_move_arg, 0) == 10 * 1000) {
                // Maximum delay means driver should park permanently here.
                if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                    PCOM_set_person_move_park_car(p_person, p_person->Genus.Person->pcom_move_arg);
                } else {
                    ASSERT(0);
                }

                p_person->Genus.Person->pcom_move = PCOM_MOVE_STILL;
            } else {
                if (p_person->Genus.Person->pcom_move == PCOM_MOVE_PATROL) {
                    waypoint = EWAY_find_waypoint(
                        p_person->Genus.Person->pcom_move_arg + 1,
                        EWAY_DONT_CARE,
                        p_person->Genus.Person->pcom_colour,
                        p_person->Genus.Person->pcom_group,
                        UC_TRUE);
                } else {
                    waypoint = EWAY_find_waypoint_rand(
                        p_person->Genus.Person->pcom_move_arg,
                        p_person->Genus.Person->pcom_colour,
                        p_person->Genus.Person->pcom_group,
                        UC_TRUE);
                }

                if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                    PCOM_set_person_move_driveto(p_person, waypoint);
                } else {
                    ASSERT(0);
                }
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// AI tick for a person driving aimlessly along road segments.
// Follows road graph, picking next segment at each junction via ROAD_whereto_now().
// Off-map vehicles are teleported to a new road start; fake-wander vehicles regen near player.
// uc_orig: PCOM_process_driving_wander (fallen/Source/pcom.cpp)
void PCOM_process_driving_wander(Thing* p_person)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;

    SLONG dest_x;
    SLONG dest_z;

    SLONG n1;
    SLONG n2;

    SLONG wtn1;
    SLONG wtn2;

    Thing* p_vehicle;

    if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER) {
        p_vehicle = TO_THING(p_person->Genus.Person->InCar);

        if (p_vehicle->Flags & FLAGS_IN_VIEW) {
            p_person->Genus.Person->sewerbits = 0;
        } else {
            p_person->Genus.Person->sewerbits++;
            if (p_person->Genus.Person->sewerbits > 50) {
                SLONG dx, dz;
                Thing* p_darci = NET_PERSON(0);
                dx = abs((p_person->WorldPos.X - p_darci->WorldPos.X) >> 8);
                dz = abs((p_person->WorldPos.Z - p_darci->WorldPos.Z) >> 8);
                if (QDIST2(dx, dz) >= (DRAW_DIST << 8)) {
                    SLONG x, z, yaw;

                    extern SLONG WAND_find_good_start_point_for_car(SLONG * posx, SLONG * posz, SLONG * yaw, SLONG anywhere);

                    if (WAND_find_good_start_point_for_car(&x, &z, &yaw, 0)) {
                        GameCoord newpos;

                        newpos.X = x << 8;
                        newpos.Y = 0; // calculated properly in reinit_vehicle()
                        newpos.Z = z << 8;
                        move_thing_on_map(p_vehicle, &newpos);
                        p_person->Genus.Person->sewerbits = Random() & 15;
                        p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL; // re-init the wander

                        p_person->WorldPos = newpos;

                        Vehicle* veh = p_vehicle->Genus.Vehicle;
                        veh->Angle = yaw ^ 1024;

                        reinit_vehicle(p_vehicle);
                    }
                }
            }
        }
    }

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_DRIVE_DOWN:
    case PCOM_MOVE_STATE_BIKE_DOWN:

        p_vehicle = TO_THING(p_person->Genus.Person->InCar);

        if (ROAD_is_end_of_the_line(p_person->Genus.Person->pcom_move_arg & 0xff)) {
            if (!WITHIN(p_vehicle->WorldPos.X, 0, PAP_SIZE_HI << 16) || !WITHIN(p_vehicle->WorldPos.Z, 0, PAP_SIZE_HI << 16)) {
                SLONG world_x = p_vehicle->WorldPos.X >> 8;
                SLONG world_z = p_vehicle->WorldPos.Z >> 8;
                SLONG nrn1;
                SLONG nrn2;
                SLONG nyaw;

                ROAD_find_me_somewhere_to_appear(
                    &world_x,
                    &world_z,
                    &nrn1,
                    &nrn2,
                    &nyaw);

                p_vehicle->Genus.Vehicle->oldX[0] = p_vehicle->Genus.Vehicle->oldX[1] = p_vehicle->Genus.Vehicle->oldX[2] = p_vehicle->Genus.Vehicle->oldX[3] = 0;
                p_vehicle->Genus.Vehicle->oldZ[0] = p_vehicle->Genus.Vehicle->oldZ[1] = p_vehicle->Genus.Vehicle->oldZ[2] = p_vehicle->Genus.Vehicle->oldZ[3] = 0;
                p_vehicle->Genus.Vehicle->Smokin = 0;

                GameCoord newpos;

                newpos.X = world_x << 8;
                newpos.Z = world_z << 8;
                newpos.Y = PAP_calc_height_at(world_x, world_z) + 0x40 << 8;

                move_thing_on_map(p_vehicle, &newpos);

                if (p_vehicle->Class == CLASS_VEHICLE) {
                    p_vehicle->Genus.Vehicle->Angle = nyaw;
                } else {
                    ASSERT(0);
                }

                p_person->Genus.Person->pcom_move_arg = nrn1 << 8;
                p_person->Genus.Person->pcom_move_arg |= nrn2;
            }
        } else {
            PCOM_get_person_dest(
                p_person,
                &dest_x,
                &dest_z);

            dx = dest_x - (p_vehicle->WorldPos.X >> 8);
            dz = dest_z - (p_vehicle->WorldPos.Z >> 8);

            dist = QDIST2(abs(dx), abs(dz));

#define LEFT_TURN_DIST 0x380
#define RIGHT_TURN_DIST 0x280

            if (dist < LEFT_TURN_DIST) {
                if (!p_person->Genus.Person->InsideRoom) {
                    ROAD_whereto_now(
                        p_person->Genus.Person->pcom_move_arg >> 8,
                        p_person->Genus.Person->pcom_move_arg & 0xff,
                        &wtn1,
                        &wtn2);

                    p_person->Genus.Person->InsideRoom = wtn2;
                }

                if (ROAD_bend(p_person->Genus.Person->pcom_move_arg >> 8, p_person->Genus.Person->pcom_move_arg & 0xff, p_person->Genus.Person->InsideRoom) < 0) {
                    // left turn -- start turning later than right turn
                    dist += LEFT_TURN_DIST - RIGHT_TURN_DIST;
                }

                if (dist < LEFT_TURN_DIST) {
                    if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                        PCOM_set_person_move_drive_down(p_person, p_person->Genus.Person->pcom_move_arg & 0xff, p_person->Genus.Person->InsideRoom);
                    } else
                        ASSERT(0);
                    p_person->Genus.Person->InsideRoom = 0;
                }
            } else {
                p_person->Genus.Person->InsideRoom = 0;
            }
        }

        break;

    default:

        ROAD_find(
            p_person->WorldPos.X >> 8,
            p_person->WorldPos.Z >> 8,
            &n1,
            &n2);

        if (n1 && n2) {
            dist = ROAD_signed_dist(
                n1,
                n2,
                p_person->WorldPos.X >> 8,
                p_person->WorldPos.Z >> 8);

            if (dist < 0) {
                SWAP(n1, n2);
            }

            if (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                PCOM_set_person_move_drive_down(p_person, n1, n2);
            } else {
                ASSERT(0);
            }
        }

        p_person->Genus.Person->InsideRoom = 0; // InsideRoom = next road node to drive to
        break;
    }
}

// AI tick for a person walking between EWAY patrol waypoints.
// Handles sequential (PCOM_MOVE_PATROL) and random (PCOM_MOVE_PATROL_RAND) modes.
// uc_orig: PCOM_process_patrol (fallen/Source/pcom.cpp)
void PCOM_process_patrol(Thing* p_person)
{
    SLONG waittime;
    SLONG waypoint;

    switch (p_person->Genus.Person->pcom_move_state) {
    default:
    case PCOM_MOVE_STATE_STILL:

        waypoint = EWAY_find_nearest_waypoint(
            p_person->WorldPos.X >> 8,
            p_person->WorldPos.Y >> 8,
            p_person->WorldPos.Z >> 8,
            p_person->Genus.Person->pcom_colour,
            p_person->Genus.Person->pcom_group);

        if (waypoint == EWAY_NO_MATCH) {
            PCOM_set_person_move_pause(p_person);
        } else {
            SLONG way_x;
            SLONG way_y;
            SLONG way_z;

            EWAY_get_position(
                waypoint,
                &way_x,
                &way_y,
                &way_z);

            SLONG dx = abs(way_x - (p_person->WorldPos.X >> 8));
            SLONG dy = abs(way_y - (p_person->WorldPos.Y >> 8));
            SLONG dz = abs(way_z - (p_person->WorldPos.Z >> 8));

            SLONG dist = QDIST3(dx, dy, dz);

            if (dist < 0x100) {
                p_person->Genus.Person->pcom_move_arg = waypoint;
                waypoint = PCOM_get_next_patrol_waypoint(p_person);
            }

            PCOM_set_person_move_mav_to_waypoint(
                p_person,
                waypoint,
                (p_person->Genus.Person->pcom_bent & PCOM_BENT_DILIGENT) ? PCOM_MOVE_SPEED_RUN : PCOM_MOVE_SPEED_WALK);
        }

        break;

    case PCOM_MOVE_STATE_GOTO_WAYPOINT:

        if (PCOM_finished_nav(p_person)) {
            waypoint = p_person->Genus.Person->pcom_move_arg;

            if (EWAY_get_delay(waypoint, 0) == 10 * 1000) {
                SLONG way_x;
                SLONG way_y;
                SLONG way_z;

                EWAY_get_position(
                    waypoint,
                    &way_x,
                    &way_y,
                    &way_z);

                p_person->Genus.Person->pcom_move = PCOM_MOVE_STILL;
                p_person->Genus.Person->HomeX = way_x;
                p_person->Genus.Person->HomeZ = way_z;
                p_person->Genus.Person->HomeYaw = EWAY_get_angle(waypoint) >> 3;

                PCOM_set_person_move_still(p_person);
            } else {
                PCOM_set_person_move_pause(p_person);
                p_person->Draw.Tweened->Angle = EWAY_get_angle(p_person->Genus.Person->pcom_move_arg);
            }
        }

        break;

    case PCOM_MOVE_STATE_PAUSE:

        waittime = PCOM_get_duration(EWAY_get_delay(p_person->Genus.Person->pcom_move_arg, 0) / 100);

        if (p_person->Genus.Person->pcom_bent & PCOM_BENT_LAZY) {
            waittime += waittime;
        }

        if (p_person->Genus.Person->pcom_move_counter >= waittime) {
            waypoint = PCOM_get_next_patrol_waypoint(p_person);

            if (waypoint == EWAY_NO_MATCH || waypoint == p_person->Genus.Person->pcom_move_arg) {
                PCOM_set_person_move_pause(p_person);
            } else {
                PCOM_set_person_move_mav_to_waypoint(
                    p_person,
                    waypoint,
                    (p_person->Genus.Person->pcom_bent & PCOM_BENT_DILIGENT) ? PCOM_MOVE_SPEED_RUN : PCOM_MOVE_SPEED_WALK);
            }
        } else {
            p_person->Draw.Tweened->Angle = EWAY_get_angle(p_person->Genus.Person->pcom_move_arg);
        }

        break;
    }
}

// Returns true when a wandering NPC is far enough from the player and has been
// stationary long enough to warrant teleporting it to a new position.
// uc_orig: should_person_regen (fallen/Source/pcom.cpp)
SLONG should_person_regen(Thing* p_person)
{
    SLONG dx, dz;
    Thing* p_darci = NET_PERSON(0);
    dx = abs((p_person->WorldPos.X - p_darci->WorldPos.X) >> 8);
    dz = abs((p_person->WorldPos.Z - p_darci->WorldPos.Z) >> 8);
    if (QDIST2(dx, dz) < (DRAW_DIST << 8))
        return (0);

    if (p_person->Genus.Person->InsideRoom > 50)
        return (1);
    else
        return (0);
}

// Teleports an out-of-view wandering NPC to a new start point and optionally
// converts it to a gang/cop attacker when timer_bored threshold is reached.
// uc_orig: PCOM_do_regen (fallen/Source/pcom.cpp)
SLONG PCOM_do_regen(Thing* p_person)
{
    SLONG wand_x;
    SLONG wand_z;

    SLONG nx, nz;

    extern ULONG timer_bored;

    if (NET_PERSON(0)->Genus.Person->Ware || EWAY_stop_player_moving()) {
        return (0);
    }

    if (p_person->Genus.Person->Target) {
        Thing* p_target = TO_THING(p_person->Genus.Person->Target);

        remove_from_gang_attack(p_person, p_target);
    }

    extern SLONG WAND_find_good_start_point(SLONG * mapx, SLONG * mapz);
    if (WAND_find_good_start_point(&nx, &nz)) {
        GameCoord new_position;

        new_position.X = nx << 8;
        new_position.Y = PAP_calc_height_at(nx, nz) << 8;
        new_position.Z = nz << 8;
        move_thing_on_map(p_person, &new_position);
        p_person->Genus.Person->Flags &= ~(FLAG_PERSON_KO | FLAG_PERSON_HELPLESS | FLAG_PERSON_WAREHOUSE | FLAG_PERSON_ARRESTED | FLAG_PERSON_ARRESTED | FLAG_PERSON_SEARCHED);
        p_person->Genus.Person->Ware = 0;
        p_person->OnFace = 0;

        if ((signed)timer_bored > (BOREDOM_RATE * 5000) && BOREDOM_RATE != 255)
        {
            Thing* darci;
            darci = NET_PERSON(0);

            if (darci->Genus.Person->PersonType != PERSON_DARCI && darci->Genus.Person->PersonType != PERSON_ROPER) {
                p_person->Genus.Person->pcom_ai = PCOM_AI_COP;
                p_person->Genus.Person->pcom_bent = 0;
                p_person->Genus.Person->PersonType = PERSON_COP;
                p_person->Draw.Tweened->MeshID = 4;
            } else {
                p_person->Genus.Person->pcom_ai = PCOM_AI_GANG;
                p_person->Genus.Person->PersonType = PERSON_THUG_RASTA;
                p_person->Genus.Person->pcom_bent = PCOM_BENT_FIGHT_BACK;
                p_person->Draw.Tweened->MeshID = (Random() % 3);
            }

            p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_NORMAL;
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
            p_person->Genus.Person->pcom_ai_arg = NULL;
            p_person->Genus.Person->pcom_ai_other = NULL;

            p_person->Genus.Person->pcom_move = PCOM_MOVE_WANDER;
            p_person->Genus.Person->pcom_move_counter = 0;
            p_person->Genus.Person->pcom_move_arg = 0;

            p_person->Genus.Person->InsideRoom = Random() & 15;
            p_person->Draw.Tweened->PersonID = 0;
            p_person->Genus.Person->pcom_colour = 0;
            p_person->Genus.Person->pcom_group = 0;

            if (timer_bored & 16)
                p_person->Flags |= FLAGS_HAS_GUN;
            else
                p_person->Flags &= ~FLAGS_HAS_GUN;

            p_person->Genus.Person->Health = 200;

            PCOM_set_person_ai_navtokill(p_person, NET_PERSON(0));
            timer_bored = 0;
        } else {
            p_person->Flags &= ~FLAGS_HAS_GUN;
            p_person->Genus.Person->pcom_ai = PCOM_AI_CIV;
            p_person->Genus.Person->pcom_bent = 0;

            p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_NORMAL;
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NONE;
            p_person->Genus.Person->pcom_ai_arg = NULL;
            p_person->Genus.Person->pcom_ai_other = NULL;

            p_person->Genus.Person->pcom_move = PCOM_MOVE_WANDER;
            p_person->Genus.Person->pcom_move_counter = 0;
            p_person->Genus.Person->pcom_move_arg = 0;
            p_person->Genus.Person->InsideRoom = Random() & 15;

            p_person->Draw.Tweened->PersonID = 0;
            p_person->Draw.Tweened->MeshID = (Random() & 1) + 8;
            p_person->Genus.Person->pcom_colour = Random() & 3;
            p_person->Genus.Person->PersonType = PERSON_CIV;
            p_person->Genus.Person->Health = 130;
            p_person->Flags &= ~FLAGS_HAS_GUN;

            WAND_get_next_place(
                p_person,
                &wand_x,
                &wand_z);

            PCOM_set_person_move_mav_to_xz(
                p_person,
                wand_x,
                wand_z,
                PCOM_MOVE_SPEED_WALK);

            p_person->Genus.Person->pcom_ai_counter = 0;
        }
        return (1);
    }
    return (0);
}

// AI tick for a foot-wandering NPC.
// Picks a random accessible street position via WAND_get_next_place().
// Fake-wander NPCs (gang/cop spawns) check for player proximity and may attack.
// uc_orig: PCOM_process_wander (fallen/Source/pcom.cpp)
void PCOM_process_wander(Thing* p_person)
{
    SLONG wand_x;
    SLONG wand_z;

    if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER) {
        if (p_person->Genus.Person->PersonType == PERSON_THUG_RASTA || p_person->Genus.Person->PersonType == PERSON_COP) {
            if ((PTIME(p_person) & 0xf) == 0) {
                if (PCOM_should_fake_person_attack_darci(p_person)) {
                    PCOM_set_person_ai_navtokill(p_person, NET_PERSON(0));

                    return;
                } else {
                    if (PCOM_get_dist_between(p_person, NET_PERSON(0)) < 0x100 * 15) {
                        PCOM_set_person_ai_flee_person(p_person, NET_PERSON(0));

                        return;
                    }
                }
            }
        }
    }

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_GOTO_XZ:
        if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER)
            if (p_person->Genus.Person->pcom_ai == PCOM_AI_CIV) {
                if ((p_person->Flags & FLAGS_IN_VIEW)) {
                    p_person->Genus.Person->InsideRoom = 0;
                } else {
                    p_person->Genus.Person->InsideRoom++;

                    if (should_person_regen(p_person))
                    {
                        p_person->Genus.Person->InsideRoom = 240;

                        if (PCOM_do_regen(p_person))
                            return;
                        /*
                        extern	SLONG	WAND_find_good_start_point(SLONG *mapx,SLONG *mapz);
                                                                        if(WAND_find_good_start_point(&nx,&nz))
                                                                        {
                                                                                GameCoord new_position;

                                                                                new_position.X = nx<<8;
                                                                                new_position.Y = PAP_calc_height_at(nx,nz)<<8;
                                                                                new_position.Z = nz<<8;
                                                                                move_thing_on_map(p_person, &new_position);

                                                                                if(timer_bored>10000)
                                                                                {
                                                                                        p_person->Genus.Person->pcom_ai     = PCOM_AI_GANG;
                                                                                        p_person->Genus.Person->pcom_bent   = PCOM_BENT_FIGHT_BACK;

                                                                                        p_person->Genus.Person->pcom_ai_state     = PCOM_AI_STATE_NORMAL;
                                                                                        p_person->Genus.Person->pcom_ai_substate  = PCOM_AI_SUBSTATE_NONE;
                                                                                        p_person->Genus.Person->pcom_ai_arg       = NULL;
                                                                                        p_person->Genus.Person->pcom_ai_other     = NULL;

                                                                                        p_person->Genus.Person->pcom_move_counter = 0;
                                                                                        p_person->Genus.Person->pcom_move_arg     = 0;

                                                                                        p_person->Genus.Person->InsideRoom=Random()&15;
                                                                                        p_person->Draw.Tweened->PersonID = 0;
                                                                                        p_person->Draw.Tweened->MeshID=0;
                                                                                        p_person->Genus.Person->pcom_colour=Random()&3;
                                                                                        PCOM_set_person_ai_kill_person(p_person,NET_PERSON(0),0);
                                                                                }
                                                                                else
                                                                                {
                                                                                        p_person->Genus.Person->InsideRoom=Random()&15;
                                                                                        p_person->Draw.Tweened->PersonID = 0;
                                                                                        p_person->Draw.Tweened->MeshID=(Random()&1)+8;
                                                                                        p_person->Genus.Person->pcom_colour=Random()&3;

                                                                                        goto new_wander;
                                                                                }
                                                                        }
                        */
                    }
                }
            }

        if (!PCOM_finished_nav(p_person) && p_person->Genus.Person->pcom_ai_counter < PCOM_get_duration(50)) {
            p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

            return;
        }

        if (p_person->Genus.Person->pcom_ai == PCOM_AI_ASSASIN) {
            PCOM_set_person_ai_normal(p_person);

            return;
        }

        if (p_person->Genus.Person->pcom_ai == PCOM_AI_DRIVER || p_person->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER) {
            PCOM_set_person_ai_findcar(p_person, NULL);

            return;
        }

        // FALLTHROUGH to pick next wander destination.

    default:
    case PCOM_MOVE_STATE_STILL:

        WAND_get_next_place(
            p_person,
            &wand_x,
            &wand_z);

        PCOM_set_person_move_mav_to_xz(
            p_person,
            wand_x,
            wand_z,
            PCOM_MOVE_SPEED_WALK);

        p_person->Genus.Person->pcom_ai_counter = 0;

        break;
    }
}

// uc_orig: PCOM_process_killing (fallen/Source/pcom.cpp)
// Melee combat AI (AI_STATE_KILLING). Manages the close-range fighting loop:
// stays near target, transitions to NAVTOKILL if target escapes or is in a vehicle,
// handles GETITEM detour for weapon pickups, and enters CIRCLE/ATTACK moves.
void PCOM_process_killing(Thing* p_person)
{
    Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);
    if (p_person->State == STATE_JUMPING) {
        return;
    }

    if (p_person->State == STATE_DANGLING) {
        if (p_person->SubState == SUB_STATE_DROP_DOWN) {
            return;
        }
    }

    if ((PTIME(p_person) & 0x3) == 0) {
        if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER) {
            if (p_person->Genus.Person->PersonType == PERSON_THUG_RASTA || p_person->Genus.Person->PersonType == PERSON_COP) {
                if (!PCOM_should_fake_person_attack_darci(p_person)) {
                    PCOM_set_person_ai_flee_person(p_person, NET_PERSON(0));

                    return;
                }
            }
        }
    }

    if (p_target->State == STATE_DEAD) {
        // We might be arresting this person — wait if so.
        if ((p_target->Genus.Person->Flags & FLAG_PERSON_ARRESTED) && p_target->SubState != SUB_STATE_DEAD_ARRESTED) {
            // Arresting still in progress — wait.
        } else {
            PCOM_set_person_ai_normal(p_person);

            return;
        }
    }

    if (PTIME(p_person) & 0x1) {
        SLONG too_far;

        if (!is_person_ko(p_target)) {
            // Tighten pursuit distance if target is actively fleeing.
            if (p_target->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PERSON) {
                too_far = 0x150;
            } else {
                too_far = 0x250;
            }

            if (PCOM_get_dist_between(p_person, p_target) > too_far || !can_a_see_b(p_person, p_target) || !there_is_a_los_things(p_person, p_target, LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_PRIMS | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                if (p_person->State == STATE_CIRCLING) {
                    remove_from_gang_attack(p_person, p_target);
                }

                PCOM_set_person_ai_navtokill(p_person, p_target);

                return;
            }
        }
    }

    if (p_target->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_PASSENGER)) {
        // Target is in a vehicle — shoot at it if armed, otherwise taunt.
        if (PCOM_person_has_any_sort_of_gun_with_ammo(p_person)) {
            PCOM_set_person_ai_navtokill(p_person, p_target);

            return;
        } else {
            PCOM_set_person_ai_taunt(p_person, p_target);

            return;
        }
    }

    if ((PTIME(p_person) & 0x3f) == 0) {
        // Shout something aggressive if target is ignoring us.
        if (p_target->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
            if (IsEnglish)
                MFX_play_thing(THING_NUMBER(p_person), SOUND_Range(S_WTHUG1_ALERT_START, S_WTHUG1_ALERT_START + 1), MFX_REPLACE, p_person);
            PCOM_oscillate_tympanum(
                PCOM_SOUND_LOOKINGATME,
                p_person,
                p_person->WorldPos.X >> 8,
                p_person->WorldPos.Y >> 8,
                p_person->WorldPos.Z >> 8);
        }
    }

    // Detour to pick up a weapon if one is nearby.
    if ((PTIME(p_person) & 0xff) == 0) {
        Thing* p_special = PCOM_is_there_an_item_i_should_get(p_person);

        if (p_special) {
            PCOM_set_person_ai_getitem(
                p_person,
                p_special,
                PCOM_MOVE_SPEED_RUN,
                PCOM_EXCAR_NAVTOKILL,
                p_person->Genus.Person->pcom_ai_arg);

            return;
        }
    }

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:

        PCOM_set_person_move_circle(p_person, p_target);

        // 0.5–1.0 second gap between attacks.
        p_person->Genus.Person->pcom_ai_counter = PCOM_get_random_duration(15, 20);

        break;

    case PCOM_MOVE_STATE_PAUSE:
    case PCOM_MOVE_STATE_CIRCLE:
        // All attack AI is handled by fn_person_circle.
        break;

    case PCOM_MOVE_STATE_ANIMATION:
    case PCOM_MOVE_STATE_WAIT_CIRCLE:
    case PCOM_MOVE_STATE_GRAPPLE:
        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_fleeing (fallen/Source/pcom.cpp)
// Flee AI for AI_STATE_FLEE_PLACE and AI_STATE_FLEE_PERSON.
// Substates: SURPRISED (brief pause/scream) → LEGIT (run away).
// Returns to NORMAL once far enough from danger origin.
void PCOM_process_fleeing(Thing* p_person)
{
    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_SUPRISED:

        if (p_person->Genus.Person->pcom_ai_counter > PCOM_get_duration(10)) {
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LEGIT;
            p_person->Genus.Person->pcom_ai_counter = 0;

            {
                SLONG danger_x;
                SLONG danger_z;

                PCOM_get_flee_from_pos(
                    p_person,
                    &danger_x,
                    &danger_z);

                PCOM_set_person_move_runaway(
                    p_person,
                    danger_x,
                    danger_z);
            }
        } else {
            p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
        }

        break;

    case PCOM_AI_SUBSTATE_LEGIT:

        // Drop any balloon while fleeing.
        if (p_person->Genus.Person->Balloon && p_person->Genus.Person->pcom_ai_counter > PCOM_get_duration(5)) {
            BALLOON_release(p_person->Genus.Person->Balloon);
        }

        // Reached current runaway destination — pick a new one.
        if (PCOM_finished_nav(p_person) || p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            SLONG danger_x;
            SLONG danger_z;

            PCOM_get_flee_from_pos(
                p_person,
                &danger_x,
                &danger_z);

            PCOM_set_person_move_runaway(
                p_person,
                danger_x,
                danger_z);
        }

        p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        if ((PTIME(p_person) & 0xf) == 0) {
            SLONG danger_x;
            SLONG danger_z;

            PCOM_get_flee_from_pos(
                p_person,
                &danger_x,
                &danger_z);

            SLONG dx = danger_x - (p_person->WorldPos.X >> 8);
            SLONG dz = danger_z - (p_person->WorldPos.Z >> 8);

            SLONG dist = abs(dx) + abs(dz);

            SLONG want_dist = 0x100 * 15;

            // After 30 seconds of running, accept a shorter safe distance.
            if (p_person->Genus.Person->pcom_ai_counter >= PCOM_get_duration(300)) {
                want_dist = 0x100 * 3;
            }

            // Fake-wander NPCs (planted by missions) flee farther before calming down.
            if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER) {
                if (p_person->Genus.Person->PersonType == PERSON_THUG_RASTA || p_person->Genus.Person->PersonType == PERSON_COP) {
                    want_dist = 0x100 * 20;
                }
            }

            if (dist > want_dist) {
                PCOM_set_person_move_still(p_person);

                PCOM_set_person_ai_normal(p_person);
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_investigating (fallen/Source/pcom.cpp)
// Investigation AI (AI_STATE_INVESTIGATING). NPC heard something suspicious and walks
// toward the source position. On arrival, looks around (LOOK substate). If nothing found,
// may go home or check hiding spots. Transitions to NAVTOKILL if PCOM_process_normal spots the player.
void PCOM_process_investigating(Thing* p_person)
{
    SLONG dist;

    SLONG before;
    SLONG after;

    SLONG hide_x;
    SLONG hide_z;

    SLONG sound_x;
    SLONG sound_z;

    // Unpack the target position from pcom_ai_arg (8-bit tile coords).
    sound_x = (p_person->Genus.Person->pcom_ai_arg >> 8) & 0xff;
    sound_z = (p_person->Genus.Person->pcom_ai_arg >> 0) & 0xff;

    sound_x <<= 8;
    sound_z <<= 8;

    sound_x += 0x80;
    sound_z += 0x80;

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_SUPRISED:

        if (p_person->Genus.Person->pcom_ai_counter > PCOM_get_duration(10)) {
            SLONG sound_y = PAP_calc_map_height_at(sound_x, sound_z) + 0x60;

            // If we can already see where the sound came from, no need to walk over.
            if (there_is_a_los(
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Y + 0x6000 >> 8,
                    p_person->WorldPos.Z >> 8,
                    sound_x,
                    sound_y + 0x80,
                    sound_z,
                    LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                PCOM_set_person_ai_normal(p_person);
            } else {
                PCOM_set_person_move_mav_to_xz(
                    p_person,
                    sound_x,
                    sound_z,
                    PCOM_MOVE_SPEED_WALK);

                if (p_person->Genus.Person->pcom_bent & PCOM_BENT_RESTRICTED) {
                    // Restricted-movement NPC: give up if MAV couldn't find a path.
                    if (!MAV_do_found_dest) {
                        PCOM_set_person_ai_normal(p_person);

                        return;
                    }
                }

                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_WALKOVER;
            }
        } else {
            p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
        }

        break;

    case PCOM_AI_SUBSTATE_WALKOVER:

        dist = PCOM_person_dist_from(
            p_person,
            sound_x,
            sound_z);

        if (dist < PCOM_ARRIVE_DIST) {
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LOOK;
            p_person->Genus.Person->pcom_ai_counter = 0;

            PCOM_set_person_move_still(p_person);
        }

        if (p_person->Genus.Person->pcom_move_state != PCOM_MOVE_STATE_GOTO_XZ) {
            PCOM_set_person_move_mav_to_xz(
                p_person,
                sound_x,
                sound_z,
                PCOM_MOVE_SPEED_WALK);
        }

        p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        break;

    case PCOM_AI_SUBSTATE_LOOK:

        before = p_person->Genus.Person->pcom_ai_counter;
        after = p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        if (before < PCOM_get_duration(10) && after >= PCOM_get_duration(10)) {
            // Turn left to look around.
            p_person->Draw.Tweened->AngleTo -= 512;
            p_person->Draw.Tweened->Angle -= 512;

            p_person->Draw.Tweened->AngleTo &= 2047;
            p_person->Draw.Tweened->Angle &= 2047;
        }

        if (before < PCOM_get_duration(30) && after >= PCOM_get_duration(30)) {
            // Turn right.
            p_person->Draw.Tweened->AngleTo += 1024;
            p_person->Draw.Tweened->Angle += 1024;

            p_person->Draw.Tweened->AngleTo &= 2047;
            p_person->Draw.Tweened->Angle &= 2047;
        }

        if (after > PCOM_get_duration(50)) {
            // Finished looking — nothing found.
            if (Random() & 0x1) {
                PCOM_set_person_ai_homesick(p_person);
            } else {
                // Try checking a nearby hiding spot.
                if (PCOM_find_hiding_place(
                        p_person,
                        &hide_x,
                        &hide_z)) {
                    PCOM_set_person_move_mav_to_xz(
                        p_person,
                        hide_x,
                        hide_z,
                        PCOM_MOVE_SPEED_WALK);

                    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_WALKOVER;

                    hide_x >>= 8;
                    hide_z >>= 8;

                    p_person->Genus.Person->pcom_ai_arg = (hide_x << 8) | hide_z;
                } else {
                    PCOM_set_person_ai_homesick(p_person);
                }
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_follow_speed (fallen/Source/pcom.cpp)
// Returns the movement speed the following NPC should match to keep up with its target.
SLONG PCOM_follow_speed(Thing* p_person, Thing* p_target)
{
    SLONG wantspeed;

    if (p_target->Genus.Person->PlayerID) {
        switch (p_target->Genus.Person->Mode) {
        case PERSON_MODE_RUN:
            wantspeed = PERSON_SPEED_YOMP;
            break;
        case PERSON_MODE_WALK:
            wantspeed = PERSON_SPEED_WALK;
            break;
        case PERSON_MODE_SNEAK:
            wantspeed = PERSON_SPEED_SNEAK;
            break;
        case PERSON_MODE_FIGHT:
            wantspeed = PERSON_SPEED_RUN;
            break;
        case PERSON_MODE_SPRINT:
            wantspeed = PERSON_SPEED_RUN;
            break;

        default:
            ASSERT(0);
            break;
        }

        if (p_target->SubState == SUB_STATE_CRAWLING || p_target->SubState == SUB_STATE_IDLE_CROUTCH || p_target->SubState == SUB_STATE_IDLE_CROUTCHING) {
            wantspeed = PERSON_SPEED_CRAWL;
        }
    } else {
        wantspeed = p_target->Genus.Person->GotoSpeed;

        if (wantspeed != PERSON_SPEED_RUN && wantspeed != PERSON_SPEED_WALK && wantspeed != PERSON_SPEED_SNEAK && wantspeed != PERSON_SPEED_YOMP && wantspeed != PERSON_SPEED_CRAWL) {
            wantspeed = PERSON_SPEED_RUN;
        }
    }

    return wantspeed;
}

// uc_orig: PCOM_process_following (fallen/Source/pcom.cpp)
// Follow-target AI (AI_STATE_FOLLOWING). Maintains formation distance behind target,
// matching their speed. Gets in/out of vehicles to stay with target. Picks up weapons
// when idle and close enough.
void PCOM_process_following(Thing* p_person)
{
    SLONG dist;
    SLONG wantspeed;

    Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    if (p_target->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_PASSENGER)) {
        if (p_person->Genus.Person->Flags & FLAG_PERSON_PASSENGER) {
            // Both in same car — assume correct vehicle.
            ASSERT(p_person->Genus.Person->InCar == p_target->Genus.Person->InCar);

            return;
        } else {
            // Get into the same car as the follow target.
            PCOM_set_person_ai_hitch(p_person, TO_THING(p_target->Genus.Person->InCar));

            return;
        }
    } else {
        if (p_person->Genus.Person->Flags & FLAG_PERSON_PASSENGER) {
            // Target left the car — we should too.
            PCOM_set_person_ai_leavecar(p_person, PCOM_EXCAR_NORMAL, 0, 0);

            return;
        }
    }

    if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_PAUSE) {
        if ((PTIME(p_person) & 0x3) == 0) {
            SLONG wantdist = 0xa0;

            // Run to keep up if target is moving fast.
            if (p_target->SubState == SUB_STATE_RUNNING) {
                wantdist = 0x40;
            }

            if (PCOM_get_dist_between(
                    p_person,
                    p_target)
                > wantdist) {
                wantspeed = PCOM_follow_speed(p_person, p_target);

                PCOM_set_person_move_mav_to_thing(
                    p_person,
                    p_target,
                    wantspeed);

                return;
            } else {
                // Mirror target's crouch state.
                if (p_target->SubState == SUB_STATE_CRAWLING || p_target->SubState == SUB_STATE_IDLE_CROUTCH || p_target->SubState == SUB_STATE_IDLE_CROUTCHING) {
                    if (p_person->SubState == SUB_STATE_IDLE_CROUTCH || p_person->SubState == SUB_STATE_IDLE_CROUTCHING) {
                        // Already crouching like target.
                    } else {
                        set_person_croutch(p_person);
                    }
                } else if (p_person->SubState == SUB_STATE_IDLE_CROUTCH || p_person->SubState == SUB_STATE_IDLE_CROUTCHING) {
                    set_person_idle_uncroutch(p_person);
                } else {
                    // Opportunistically pick up a weapon when close to target.
                    if (p_person->Genus.Person->pcom_ai != PCOM_AI_CIV) {
                        if ((PTIME(p_person) & 0x7f) == 0) {
                            Thing* p_special = PCOM_is_there_an_item_i_should_get(p_person);

                            if (p_special) {
                                PCOM_set_person_ai_getitem(
                                    p_person,
                                    p_special,
                                    PCOM_MOVE_SPEED_RUN,
                                    PCOM_EXCAR_NORMAL,
                                    0);

                                return;
                            }
                        }
                    }
                }
            }
        }
    } else {
        {
            // Adjust speed based on separation: run if far, match target speed if close.
            SLONG dx = abs(p_target->WorldPos.X - p_person->WorldPos.X >> 8);
            SLONG dz = abs(p_target->WorldPos.Z - p_person->WorldPos.Z >> 8);

            dist = QDIST2(dx, dz);

            if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_GOTO_THING && (p_person->Genus.Person->pcom_move_substate == PCOM_MOVE_SUBSTATE_GOTO || p_person->Genus.Person->pcom_move_substate == PCOM_MOVE_SUBSTATE_LOSMAV)) {
                if (dist > 0x180) {
                    if (p_person->Genus.Person->GotoSpeed != PERSON_SPEED_RUN) {
                        PCOM_set_person_move_mav_to_thing(p_person, p_target, PERSON_SPEED_RUN);
                    }
                } else if (dist < 0xc0) {
                    wantspeed = PCOM_follow_speed(p_person, p_target);

                    if (p_person->Genus.Person->GotoSpeed != wantspeed) {
                        // Allow staying at run speed even when target walks — to catch up.
                        if (p_person->Genus.Person->GotoSpeed == PERSON_SPEED_RUN) {
                            if (wantspeed == PERSON_SPEED_WALK || wantspeed == PERSON_SPEED_SNEAK) {
                                PCOM_set_person_move_mav_to_thing(p_person, p_target, wantspeed);
                            }
                        } else {
                            PCOM_set_person_move_mav_to_thing(p_person, p_target, wantspeed);
                        }
                    }
                }

                // Extra boost if lagging behind.
                if (p_person->Velocity < p_target->Velocity) {
                    p_person->Velocity += 1;
                }
            }
        }

        if (p_person->State == STATE_GOTOING) {
            SLONG dx = abs(p_target->WorldPos.X - p_person->WorldPos.X >> 8);
            SLONG dz = abs(p_target->WorldPos.Z - p_person->WorldPos.Z >> 8);

            SLONG dist = QDIST2(dx, dz);

            SLONG wantdist;

            if (p_target->SubState == SUB_STATE_RUNNING) {
                wantdist = 0x40;
            } else {
                wantdist = 0x80;
            }

            if (dist < wantdist) {
                PCOM_set_person_move_pause(p_person);

                if (p_target->SubState == SUB_STATE_CRAWLING || p_target->SubState == SUB_STATE_IDLE_CROUTCH || p_target->SubState == SUB_STATE_IDLE_CROUTCHING) {
                    set_person_idle_croutch(p_person);
                }
            }
        }
    }
}

// uc_orig: PCOM_find_mib_appear_pos (fallen/Source/pcom.cpp)
// Stub — calculates a position for an MIB to teleport to. Body left empty in the
// pre-release source; presumably completed post-fork.
void PCOM_find_mib_appear_pos(
    Thing* p_mib,
    Thing* p_target,
    SLONG* appear_x,
    SLONG* appear_z)
{
}

// uc_orig: draw_view_line (fallen/Source/pcom.cpp)
// Debug visualisation: draws a line of world-space segments from shooter's head to target's head.
// Called from PCOM_process_navtokill only when shooting at the player.
void draw_view_line(Thing* p_person, Thing* p_target)
{
    SLONG x1, y1, z1, x2, y2, z2;
    SLONG dx, dy, dz;
    SLONG len, step, count;

    calc_sub_objects_position(p_person, p_person->Draw.Tweened->AnimTween, SUB_OBJECT_HEAD, &x1, &y1, &z1);

    x1 += p_person->WorldPos.X >> 8;
    y1 += p_person->WorldPos.Y >> 8;
    z1 += p_person->WorldPos.Z >> 8;

    calc_sub_objects_position(p_target, p_target->Draw.Tweened->AnimTween, SUB_OBJECT_HEAD, &x2, &y2, &z2);

    x2 += p_target->WorldPos.X >> 8;
    y2 += p_target->WorldPos.Y >> 8;
    z2 += p_target->WorldPos.Z >> 8;

    dx = x2 - x1;
    dy = y2 - y1;
    dz = z2 - z1;

    len = QDIST3(abs(dx), abs(dy), abs(dz));

    dx = (dx * 20) / len;
    dy = (dy * 20) / len;
    dz = (dz * 20) / len;

    count = (len / 20) >> 1;

    for (step = 0; step < count; step++) {
        AENG_world_line(x1, y1, z1, 20, 0xffffff, x1 + dx, y1 + dy, z1 + dz, 20, 0xffffff, UC_TRUE);

        x1 += dx << 1;
        y1 += dy << 1;
        z1 += dz << 1;
    }
}

// uc_orig: PCOM_process_navtokill (fallen/Source/pcom.cpp)
// Ranged/hunt AI (AI_STATE_NAVTOKILL). Three substates: HUNTING (path toward target),
// AIMING (draw weapon, wait, then shoot), NOMOREAMMO (holster gun and resume hunting).
// Transitions to KILLING when in melee range. Handles special MIB wait behaviour and
// FAKE_WANDER NPCs that should not attack during player cutscenes.
void PCOM_process_navtokill(Thing* p_person)
{
    SLONG dist;
    SLONG hit_distance;
    SLONG special;

    Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    if (p_target->State == STATE_DEAD && p_target->Genus.Person->PlayerID) {
        // Player is dead — investigate their last known position.
        PCOM_set_person_ai_investigate(p_person, p_target->WorldPos.X >> 8, p_target->WorldPos.Z >> 8);
        return;
    }

    if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER)
        timer_bored = 0;

    if (p_person->State == STATE_JUMPING)
        return;

    if ((PTIME(p_person) & 0x7) == 0) {
        if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER) {
            if (!PCOM_should_fake_person_attack_darci(p_person)) {
                PCOM_set_person_ai_flee_person(p_person, NET_PERSON(0));

                return;
            }
        }
    }

    hit_distance = 240;

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_WAITING:

        // Cutscene in progress — taunt periodically and wait.
        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            if (!EWAY_stop_player_moving()) {
                PCOM_set_person_move_mav_to_thing(
                    p_person,
                    p_target,
                    PCOM_MOVE_SPEED_RUN);

                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_HUNTING;
            } else if ((PTIME(p_person) & 0x3) == 0) {
                if ((Random() & 0x3) == 0) {
                    set_face_thing(
                        p_person,
                        p_target);

                    PCOM_set_person_move_animation(p_person, ANIM_WANKER);
                }
            }
        }

        break;

    case PCOM_AI_SUBSTATE_HUNTING_SLIDE:

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            PCOM_set_person_move_mav_to_thing(
                p_person,
                p_target,
                PCOM_MOVE_SPEED_RUN);

            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_HUNTING;
        }

        break;

    case PCOM_AI_SUBSTATE_HUNTING:

        if (p_target == NET_PERSON(0)) {
            if (EWAY_stop_player_moving()) {
                if (((PTIME(p_person)) & 0x1f) == 9) {
                    set_face_thing(
                        p_person,
                        p_target);

                    PCOM_set_person_move_animation(p_person, ANIM_WANKER);

                    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_WAITING;
                }

                return;
            }
        }

        if (p_person->State == STATE_IDLE || p_person->State == STATE_GUN || p_person->State == STATE_GOTOING) {
            if (p_person->Genus.Person->pcom_zone) {
                if (!(p_person->Genus.Person->pcom_zone & PCOM_get_zone_for_position(p_target))) {
                    if ((p_person->Genus.Person->pcom_zone & PCOM_get_zone_for_position(p_person->Genus.Person->HomeX, p_person->Genus.Person->HomeZ))) {
                        PCOM_set_person_ai_homesick(p_person);

                        return;

                    } else {
                        // Level design error: clear the zone flag so this NPC can still function.
                        p_person->Genus.Person->pcom_zone = 0;
                    }
                }
            }

            // Detour for weapon pickup.
            if (((PTIME(p_person)) & 0xff) == 0) {
                Thing* p_special = PCOM_is_there_an_item_i_should_get(p_person);

                if (p_special) {
                    PCOM_set_person_ai_getitem(
                        p_person,
                        p_special,
                        PCOM_MOVE_SPEED_RUN,
                        PCOM_EXCAR_NAVTOKILL,
                        p_person->Genus.Person->pcom_ai_arg);

                    return;
                }
            }

            if (PCOM_person_has_any_sort_of_gun_with_ammo(p_person) && !is_person_ko(p_target)) {
                SLONG check_look;

                // Gunner only enters melee at very close range.
                hit_distance >>= 1;

                {
                    check_look = PCOM_get_duration(5) - PCOM_get_duration100(GET_SKILL(p_person) << 1);
                }

                if (p_person->Genus.Person->pcom_ai_counter > check_look || p_person->Genus.Person->pcom_ai == PCOM_AI_SHOOT_DEAD) {
                    if (p_target->Genus.Person->Mode == PERSON_MODE_FIGHT) {
                        // Don't shoot when target is fighting someone else.
                    } else {
                        if (PCOM_get_dist_between(p_person, p_target) < 0x400 && can_a_see_b(p_person, p_target)) {
                            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_AIMING;
                            p_person->Genus.Person->pcom_ai_counter = 0;

                            PCOM_set_person_move_still(p_person);

                            break;
                        }
                    }

                    p_person->Genus.Person->pcom_ai_counter = 0;
                }
            }

            dist = PCOM_get_dist_between(p_person, p_target);

            // FAKE_WANDER NPCs respawn behind the player if they fall too far behind.
            if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER) {
                if (dist > 30 << 8 || EWAY_stop_player_moving()) {
                    if (!(p_person->Flags & FLAGS_IN_VIEW)) {
                        p_person->Genus.Person->InsideRoom++;

                        p_person->Genus.Person->InsideRoom = 240;

                        if (PCOM_do_regen(p_person))
                            return;
                    }
                }
            }

            if (p_person->SubState == SUB_STATE_RUNNING && p_target->SubState == SUB_STATE_RUNNING) {
                if (p_person->Velocity >= 20 && dist < 0xc0) {
                    if (p_person->Genus.Person->pcom_ai == PCOM_AI_FIGHT_TEST) {
                        // Fight-test dummies skip sliding tackles.
                    } else {
                        if ((PTIME(p_person) & 0x7) == 0 || !p_target->Genus.Person->PlayerID) {
                            if (can_a_see_b(p_person, p_target)) {
                                p_person->Velocity += 10;

                                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_HUNTING_SLIDE;

                                PCOM_set_person_move_goto_thing_slide(p_person, p_target);
                            }
                        }
                    }
                }
            } else if (dist < hit_distance) {
                if (abs(p_person->WorldPos.Y - p_target->WorldPos.Y) < 0x7000) {
                    SLONG x1;
                    SLONG y1;
                    SLONG z1;

                    SLONG x2;
                    SLONG y2;
                    SLONG z2;

                    x1 = p_person->WorldPos.X >> 8;
                    y1 = p_person->WorldPos.Y + 0x6000 >> 8;
                    z1 = p_person->WorldPos.Z >> 8;

                    x2 = p_target->WorldPos.X >> 8;
                    y2 = p_target->WorldPos.Y + 0x6000 >> 8;
                    z2 = p_target->WorldPos.Z >> 8;

                    if (there_is_a_los(
                            x1, y1, z1,
                            x2, y2, z2,
                            LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                        if ((p_target->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_PASSENGER)) && PCOM_person_has_any_sort_of_gun_with_ammo(p_person)) {
                            // Shoot targets in vehicles.
                            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_AIMING;
                            p_person->Genus.Person->pcom_ai_counter = 0;

                            PCOM_set_person_move_still(p_person);
                        } else {
                            // Draw an h2h weapon if we have one and haven't already.
                            special = PCOM_person_has_any_sort_of_h2h(p_person);

                            if (p_person->Genus.Person->SpecialUse && TO_THING(p_person->Genus.Person->SpecialUse)->Genus.Special->SpecialType == special) {
                                special = NULL;
                            }

                            if (special) {
                                PCOM_set_person_move_draw_h2h(p_person, special);

                                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_DRAW_H2H;
                            } else {
                                if (p_target->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                                    if (PCOM_person_has_any_sort_of_gun_with_ammo(p_person)) {
                                        p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_AIMING;
                                        p_person->Genus.Person->pcom_ai_counter = 0;

                                        PCOM_set_person_move_still(p_person);
                                    } else {
                                        PCOM_set_person_ai_taunt(p_person, p_target);
                                    }
                                } else {
                                    PCOM_set_person_ai_kill_person(p_person, p_target);
                                }
                            }
                        }
                    }
                }
            }
        }

        break;

    case PCOM_AI_SUBSTATE_AIMING:

        if (p_target == NET_PERSON(0)) {
            if (EWAY_stop_player_moving()) {
                if ((PTIME(p_person) & 0x1f) == 8) {
                    set_face_thing(
                        p_person,
                        p_target);

                    PCOM_set_person_move_animation(p_person, ANIM_WANKER);

                    p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_WAITING;
                }

                return;
            }
        }

        if (PCOM_person_has_any_sort_of_gun(p_person)) {
            if (!PCOM_person_has_gun_in_hand(p_person)) {
                if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
                    PCOM_set_person_move_draw_gun(p_person);
                }

                p_person->Genus.Person->pcom_ai_counter = 0;
            } else {
                if (p_target->Genus.Person->PlayerID && EWAY_stop_player_moving()) {
                    // Don't shoot while player is in a cutscene.
                    p_person->Genus.Person->pcom_ai_counter = 0;
                }

                SLONG shoot_time;

                shoot_time = PCOM_get_duration(get_rate_of_fire(p_person));

                // Reduce wait time slightly — allows firing more often for balance.
                shoot_time -= shoot_time >> 2;

                {
                    if (p_target->Genus.Person->Mode == PERSON_MODE_FIGHT) {
                        if (p_target->Genus.Person->Target == THING_NUMBER(p_person)) {
                            // Target is fighting us — don't hold back.
                        } else {
                            shoot_time <<= 1;
                        }
                    }
                }

                // Sprinting target — keep timer ticking to stop the player sprinting through levels.
                if (p_target->Class == CLASS_PERSON && p_target->Genus.Person->Mode == PERSON_MODE_SPRINT) {
                    p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
                }

                if (p_person->Genus.Person->pcom_ai == PCOM_AI_BODYGUARD && p_target->Genus.Person->PlayerID == 0) {
                    shoot_time >>= 2;
                }

                if (dist_to_target(p_person, p_target) < (10 << 8))
                    track_gun_sight(p_target, shoot_time - p_person->Genus.Person->pcom_ai_counter);

                if (p_target->Genus.Person->PlayerID) {
                    draw_view_line(p_person, p_target);
                }

                if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_ANIMATION && p_person->Genus.Person->pcom_move_substate == PCOM_MOVE_SUBSTATE_SHOOT) {
                    // Currently in the middle of a shooting animation.
                    p_person->Genus.Person->pcom_ai_counter = 0;
                } else {
                    if (p_person->Genus.Person->pcom_ai_counter > shoot_time || p_person->Genus.Person->pcom_ai == PCOM_AI_SHOOT_DEAD) {
                        if (PCOM_get_dist_between(p_person, p_target) < 0x600 && can_a_see_b(p_person, p_target)) {
                            if (!PCOM_person_has_any_sort_of_gun_with_ammo(p_person)) {
                                // No ammo — play click, holster gun, resume hunting.
                                set_person_shoot(p_person, 1);

                                PCOM_set_person_move_gun_away(p_person);

                                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NOMOREAMMO;
                            } else {
                                PCOM_set_person_move_shoot(p_person);
                            }
                        } else {
                            // Lost line of sight — chase again.
                            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_HUNTING;

                            PCOM_set_person_move_mav_to_thing(
                                p_person,
                                p_target,
                                PCOM_MOVE_SPEED_RUN);
                        }

                        p_person->Genus.Person->pcom_ai_counter = 0;
                    }
                }
            }

            // Always face target while aiming.
            set_face_thing(p_person, p_target);
        } else {
            // Gun was kicked out of hand — resume hunt.
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_HUNTING;

            PCOM_set_person_move_mav_to_thing(
                p_person,
                p_target,
                PCOM_MOVE_SPEED_RUN);
        }

        break;

    case PCOM_AI_SUBSTATE_NOMOREAMMO:

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_HUNTING;

            PCOM_set_person_move_mav_to_thing(
                p_person,
                p_target,
                PCOM_MOVE_SPEED_RUN);
        }

        break;

    case PCOM_AI_SUBSTATE_DRAW_H2H:

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            // Weapon drawn — decide whether to attack or close distance first.
            if (PCOM_get_dist_between(p_person, p_target) < 0x100 && can_a_see_b(p_person, p_target)) {
                PCOM_set_person_ai_kill_person(p_person, p_target);
            } else {
                PCOM_set_person_ai_navtokill(p_person, p_target);
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }

    if (p_target->State == STATE_DEAD) {
        PCOM_set_person_ai_normal(p_person);
    }

    p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
}

// ---------------------------------------------------------------------------
// Iteration 87: PCOM_process_findcar .. PCOM_find_bodyguard_victim
// ---------------------------------------------------------------------------

// uc_orig: PCOM_process_findcar (fallen/Source/pcom.cpp)
void PCOM_process_findcar(Thing* p_person)
{
    SLONG door;

    Thing* p_vehicle = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    ASSERT(p_vehicle->Class == CLASS_VEHICLE);

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_GOTOCAR:

        if (p_person->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER || p_person->Genus.Person->pcom_ai == PCOM_AI_DRIVER) {
            // Drivers never enter a car that is already occupied by another driver.
            if (p_vehicle->Genus.Vehicle->Driver) {
                PCOM_set_person_ai_findcar(p_person, NULL);
            }
        }

        {
            // Enter only if out of player view and close enough for 20 consecutive turns.
            if ((p_person->Flags & FLAGS_IN_VIEW) || PCOM_get_dist_between(p_person, p_vehicle) > 0x200) {
                p_person->Genus.Person->pcom_ai_counter = 0;
            } else {
                p_person->Genus.Person->pcom_ai_counter += 1;
            }

            door = 0;

            if (p_person->Genus.Person->pcom_ai_counter > 20 || in_right_place_for_car(p_person, p_vehicle, &door)) {
                ASSERT(door == 0 || door == 1);

                PCOM_set_person_move_getincar(p_person, p_vehicle, UC_FALSE, door);

                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_GETINCAR;
                p_person->Genus.Person->pcom_ai_counter = 0;
            }
        }

        break;

    case PCOM_AI_SUBSTATE_GETINCAR:

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            // Successfully inside. Wait until no more passengers are boarding.
            if (((GAME_TURN & 0xf) == (THING_NUMBER(p_person) & 0xf)) || PCOM_are_there_people_who_want_to_enter(p_vehicle)) {
                // Wait a while.
            } else {
                p_person->Genus.Person->pcom_ai_state = PCOM_AI_STATE_NORMAL;
                p_person->Genus.Person->pcom_ai_substate = 0;
                p_person->Genus.Person->pcom_ai_counter = 0;

                p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
                p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_NONE;
            }
        } else {
            if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(40)) {
                // Stuck getting in -- retry findcar.
                PCOM_set_person_ai_findcar(p_person, p_person->Genus.Person->pcom_ai_arg);
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_talk (fallen/Source/pcom.cpp)
void PCOM_process_talk(Thing* p_person)
{
    if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
        // Animation finished. If NO_RETURN_TO_NORMAL was set by mission script, stay put.
        if (p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER) {
            // Flagged as NO_RETURN_TO_NORMAL -- keep this state.
        } else {
            PCOM_set_person_ai_normal(p_person);
        }
    }
}

// uc_orig: PCOM_process_hands_up (fallen/Source/pcom.cpp)
void PCOM_process_hands_up(Thing* p_person)
{
    Thing* p_cop;
    p_cop = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

    if (p_person->Genus.Person->pcom_ai_counter > PCOM_get_duration(20)) {
        // After 20s check if cop is still aiming at us.
        if (p_cop->Genus.Person->Target != THING_NUMBER(p_person) || (p_cop->State != STATE_GUN && p_cop->SubState != SUB_STATE_RUNNING)) {
            // Cop moved on -- return to normal.
            PCOM_set_person_ai_normal(p_person);
        }
    }
}

// uc_orig: PCOM_process_hitch (fallen/Source/pcom.cpp)
void PCOM_process_hitch(Thing* p_person)
{
    SLONG door;

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_GOTOCAR:

    {
        Thing* p_vehicle = TO_THING(p_person->Genus.Person->pcom_ai_arg);

        ASSERT(p_vehicle->Class == CLASS_VEHICLE);

        if ((p_person->Flags & FLAGS_IN_VIEW) || PCOM_get_dist_between(p_person, p_vehicle) > 0x200) {
            p_person->Genus.Person->pcom_ai_counter = 0;
        } else {
            p_person->Genus.Person->pcom_ai_counter += 1;
        }

        if (p_person->Genus.Person->pcom_ai_counter > 20 || in_right_place_for_car(p_person, p_vehicle, &door)) {
            PCOM_set_person_move_getincar(p_person, p_vehicle, UC_TRUE, door);

            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_GETINCAR;
            p_person->Genus.Person->pcom_ai_counter = 0;
        }
    }

    break;

    case PCOM_AI_SUBSTATE_GETINCAR:

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_HITCHING;
        }

        break;

    case PCOM_AI_SUBSTATE_HITCHING:

        if (p_person->Genus.Person->pcom_move == PCOM_MOVE_FOLLOW) {
            SLONG i_target = EWAY_get_person(p_person->Genus.Person->pcom_move_follow);
            Thing* p_target = TO_THING(i_target);

            ASSERT(p_target->Class == CLASS_PERSON);

            if (!(p_target->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_PASSENGER))) {
                // Followed person left the vehicle -- exit too.
                PCOM_set_person_ai_leavecar(p_person, PCOM_EXCAR_NORMAL, 0, 0);

                return;
            }
        } else {
            ASSERT(0);
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_knockedout (fallen/Source/pcom.cpp)
void PCOM_process_knockedout(Thing* p_person)
{
    if (!(p_person->Genus.Person->Flags & FLAG_PERSON_KO)) {
        // Recovered from being knocked out.
        PCOM_set_person_ai_normal(p_person);
    }
}

// uc_orig: PCOM_process_taunt (fallen/Source/pcom.cpp)
void PCOM_process_taunt(Thing* p_person)
{
    Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    // Always face whoever is being taunted.
    set_face_thing(p_person, p_target);

    if ((PTIME(p_person) & 0x7) == 0) {
        if (!can_a_see_b(p_person, p_target)) {
            // Lost sight of target -- stop taunting.
            PCOM_set_person_ai_normal(p_person);

            return;
        }
    }

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:

        if (p_person->Genus.Person->pcom_zone && (PCOM_get_zone_for_position(p_person) & p_person->Genus.Person->pcom_zone)) {
            // Kill the target if they entered our zone.
            if (PCOM_get_zone_for_position(p_target) & p_person->Genus.Person->pcom_zone) {
                PCOM_set_person_ai_kill_person(p_person, p_target);

                return;
            }
        } else {
            // Random 50% chance to escalate to combat.
            if (Random() & 0x1) {
                PCOM_set_person_ai_kill_person(p_person, p_target);

                return;
            }
        }

        // Pause before next taunt animation.
        PCOM_set_person_move_pause(p_person);

        p_person->Genus.Person->pcom_ai_counter = PCOM_get_duration(Random() & 0xf);

        break;

    case PCOM_MOVE_STATE_PAUSE:

        if (p_person->Genus.Person->pcom_move_counter > p_person->Genus.Person->pcom_ai_counter) {
            if (p_person->Genus.Person->pcom_bent & PCOM_BENT_LAZY) {
                // Lazy people just stand, no taunt animation.
                PCOM_set_person_move_still(p_person);
            } else {
                ASSERT(p_person->SubState != SUB_STATE_GRAPPLEE);
                ASSERT(p_person->SubState != SUB_STATE_GRAPPLE_HELD);

                PCOM_set_person_move_animation(p_person, ANIM_WANKER);
            }
        }

        break;

    case PCOM_MOVE_STATE_ANIMATION:

        if (p_person->Genus.Person->pcom_zone) {
            // Zone check during animation -- kill if target entered zone.
            if (PCOM_get_zone_for_position(p_target) & p_person->Genus.Person->pcom_zone) {
                PCOM_set_person_ai_kill_person(p_person, p_target);

                return;
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_arrest (fallen/Source/pcom.cpp)
void PCOM_process_arrest(Thing* p_person)
{
    Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    if (p_target->State == STATE_DEAD) {
        PCOM_set_person_ai_normal(p_person);

        return;
    }

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:

        // Wait briefly, then escalate to combat if suspect does not comply.
        PCOM_set_person_move_pause(p_person);

        break;

    case PCOM_MOVE_STATE_PAUSE:

        if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(10)) {
            PCOM_set_person_ai_kill_person(
                p_person,
                p_target);

            return;
        }

        break;

    case PCOM_MOVE_STATE_GOTO_THING:

        if (PCOM_get_dist_between(p_person, p_target) < 0x200) {
            PCOM_set_person_move_still(p_person);
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_homesick (fallen/Source/pcom.cpp)
void PCOM_process_homesick(Thing* p_person)
{
    if (PCOM_finished_nav(p_person)) {
        // Arrived home -- return to normal behaviour.
        PCOM_set_person_ai_normal(p_person);
    }
}

// uc_orig: PCOM_process_bdeactivate (fallen/Source/pcom.cpp)
void PCOM_process_bdeactivate(Thing* p_person)
{
    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_GOTOBOMB:

        if (PCOM_finished_nav(p_person)) {
            // Reached the bomb -- begin defusing.
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_CUTWIRES;
            p_person->Genus.Person->pcom_ai_counter = 0;

            PCOM_set_person_move_still(p_person);
        }

        break;

    case PCOM_AI_SUBSTATE_CUTWIRES:

        // Spin in place to visually indicate activity.
        p_person->Draw.Tweened->Angle += 32;

        if (p_person->Genus.Person->pcom_ai_counter >= PCOM_get_duration(100)) {
            // Bomb defused.
            Thing* p_bomb = TO_THING(p_person->Genus.Person->pcom_ai_arg);

            p_bomb->SubState = SPECIAL_SUBSTATE_NONE;

            PCOM_set_person_ai_normal(p_person);
        } else {
            p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_leavecar (fallen/Source/pcom.cpp)
void PCOM_process_leavecar(Thing* p_person)
{
    Thing* p_vehicle;

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_STILL:

        // Finished exiting. Decide next AI state based on exit reason.
        switch (p_person->Genus.Person->pcom_ai_excar_state) {
        case PCOM_EXCAR_NORMAL:
            PCOM_set_person_ai_normal(p_person);
            break;

        case PCOM_EXCAR_FLEE_PERSON:
            PCOM_set_person_ai_flee_person(p_person, TO_THING(p_person->Genus.Person->pcom_ai_excar_arg));
            break;

        case PCOM_EXCAR_ARREST_PERSON:
            PCOM_set_person_ai_arrest(p_person, TO_THING(p_person->Genus.Person->pcom_ai_excar_arg));
            break;

        default:
            ASSERT(0);
            break;
        }

        break;

    case PCOM_MOVE_STATE_PARK_CAR:
    case PCOM_MOVE_STATE_PARK_CAR_ON_ROAD:

        ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

        p_vehicle = TO_THING(p_person->Genus.Person->InCar);

        if (p_vehicle->Velocity == 0) {
            // Car stopped -- play exit animation.
            PCOM_set_person_move_leavecar(p_person);
        }

        break;

    case PCOM_MOVE_STATE_ANIMATION:
        break;

    default:
        ASSERT(0);
        break;
    }
}

// Stationary sniper AI. Three substates:
//   LOOK: wait for line of sight to target.
//   AIMING: draw gun, then fire when counter exceeds shoot_time.
//   NOMOREAMMO: holster gun, return to LOOK.
// Max engagement range 0x600 units.
// PCOM_AI_SHOOT_DEAD variant fires immediately without waiting.
// uc_orig: PCOM_process_snipe (fallen/Source/pcom.cpp)
void PCOM_process_snipe(Thing* p_person)
{
    if (p_person->Genus.Person->pcom_ai_arg == NULL) {
        // No target -- reset to normal.
        p_person->Genus.Person->pcom_ai = PCOM_AI_NONE;

        PCOM_set_person_ai_normal(p_person);

        return;
    }

    Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_LOOK:

        p_person->Genus.Person->Target = NULL;

        if (!PCOM_person_has_any_sort_of_gun_with_ammo(p_person)) {
            // No ammo -- nothing to do.
        } else {
            if (can_a_see_b(p_person, p_target)) {
                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_AIMING;
                p_person->Genus.Person->pcom_ai_counter = 0;
            } else {
                p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
                p_person->Genus.Person->Target = NULL;
                // Note: "holster after 10s of no target" code is commented out in original.
                // Gun stays drawn indefinitely while scanning.
            }
        }

        break;

    case PCOM_AI_SUBSTATE_AIMING:

        p_person->Genus.Person->Target = THING_NUMBER(p_target);

        if (!person_has_gun_or_grenade_out(p_person)) {
            // Draw gun first.
            if (p_person->Genus.Person->pcom_move_state != PCOM_MOVE_STATE_ANIMATION) {
                PCOM_set_person_move_draw_gun(p_person);
            }
        } else {
            if (PCOM_get_dist_between(p_person, p_target) < 0x600 && can_a_see_b(p_person, p_target)) {
                SLONG shoot_time;

                shoot_time = PCOM_get_duration(get_rate_of_fire(p_person));
                shoot_time -= PCOM_get_duration100(GET_SKILL(p_person) << 2);

                if (p_person->Genus.Person->pcom_ai_counter > shoot_time || p_person->Genus.Person->pcom_ai == PCOM_AI_SHOOT_DEAD) {
                    if (p_person->Genus.Person->Ammo == 0) {
                        // Dry fire -- play click then holster.
                        set_person_shoot(p_person, 1);

                        PCOM_set_person_move_gun_away(p_person);

                        p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_NOMOREAMMO;
                    } else {
                        // Fire!
                        p_person->Genus.Person->Target = THING_NUMBER(p_target);
                        PCOM_set_person_move_shoot(p_person);

                        p_person->Genus.Person->pcom_ai_counter = 0;
                    }
                } else {
                    p_person->Genus.Person->pcom_ai_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
                }

                // Track target while aiming.
                set_face_thing(p_person, p_target);
            } else {
                // Lost sight or out of range -- scan again.
                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LOOK;
            }
        }

        break;

    case PCOM_AI_SUBSTATE_NOMOREAMMO:

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
            // Holster done -- resume scanning.
            p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LOOK;
            p_person->Genus.Person->pcom_ai_counter = 0;
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: PCOM_process_warm_hands (fallen/Source/pcom.cpp)
void PCOM_process_warm_hands(Thing* p_person)
{
    SLONG i_fire;
    Thing* p_fire;

    switch (p_person->Genus.Person->pcom_ai_substate) {
    case PCOM_AI_SUBSTATE_GOTOFIRE:

        switch (p_person->Genus.Person->pcom_move_state) {
        case PCOM_MOVE_STATE_PAUSE:

            if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(10)) {
                // Search for a nearby fire source (pyro/bonfire) within 0x300 units.
                i_fire = THING_find_nearest(
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Y >> 8,
                    p_person->WorldPos.Z >> 8,
                    0x300,
                    1 << CLASS_PYRO);

                if (i_fire) {
                    p_person->Genus.Person->pcom_ai_arg = i_fire;

                    PCOM_set_person_move_mav_to_thing(p_person, TO_THING(i_fire), PCOM_MOVE_SPEED_WALK);
                } else {
                    // No fire found -- wait and try again.
                    PCOM_set_person_move_pause(p_person);
                }
            }

            break;

        case PCOM_MOVE_STATE_GOTO_THING:

            p_fire = TO_THING(p_person->Genus.Person->pcom_ai_arg);

            if (PCOM_get_dist_between(p_person, p_fire) < 0xa0) {
                // Close enough -- start warming hands.
                p_person->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_WARMUP;
                set_face_thing(p_person, p_fire);

                PCOM_set_person_move_still(p_person);
            }

            break;
        }

        break;

    case PCOM_AI_SUBSTATE_WARMUP:

        p_fire = TO_THING(p_person->Genus.Person->pcom_ai_arg);

        if (p_fire->Class != CLASS_PYRO) {
            // Fire went out -- look for a new one.
            PCOM_set_person_ai_warm_hands(p_person);
        } else {
            // Warming hands -- animation handled by normal animation system.
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// Returns true if the person was rendered (in view) recently.
// Used to choose between teleporting home and walking home.
// uc_orig: person_drawn_recently (fallen/Source/pcom.cpp)
static SLONG person_drawn_recently(Thing* p_person)
{
    return p_person->Flags & FLAGS_IN_VIEW;
}

// Instantly moves a person to their HomeX/HomeZ without pathfinding.
// Used when the NPC is far from home and not visible to the player.
// uc_orig: PCOM_teleport_home (fallen/Source/pcom.cpp)
void PCOM_teleport_home(Thing* p_person)
{
    GameCoord pos;

    pos.X = p_person->Genus.Person->HomeX << 8;
    pos.Y = p_person->WorldPos.Y;
    pos.Z = p_person->Genus.Person->HomeZ << 8;

    move_thing_on_map(p_person, &pos);

    p_person->Draw.Tweened->Angle = p_person->Genus.Person->HomeYaw << 3;
}

// Search range for bench/sofa used by PCOM_BENT_LAZY in PCOM_MOVE_STILL mode.
// uc_orig: PCOM_MAX_BENCH_WALK (fallen/Source/pcom.cpp)
#define PCOM_MAX_BENCH_WALK 0x200

// Top-level idle/patrol dispatcher for AI_STATE_NORMAL.
// Routes based on pcom_move: STILL (sitting, dancing, guard gun-draw), PATROL, WANDER,
// FOLLOW, WARM_HANDS, FOLLOW_ON_SEE.
// uc_orig: PCOM_process_normal (fallen/Source/pcom.cpp)
void PCOM_process_normal(Thing* p_person)
{
    UWORD i_target;
    SLONG dist;

    switch (p_person->Genus.Person->pcom_move) {
    case PCOM_MOVE_STILL:
    case PCOM_MOVE_DANCE:
    case PCOM_MOVE_HANDS_UP:
    case PCOM_MOVE_TIED_UP:

        if (p_person->State == STATE_MOVEING && (p_person->SubState == SUB_STATE_SIMPLE_ANIM || p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER)) {
            // Person is animating (sitting, dancing).
            if (p_person->Genus.Person->pcom_move == PCOM_MOVE_STILL) {
                // STILL people should only animate when sitting or scratching.
                if (p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_DOWN || p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_IDLE || p_person->Draw.Tweened->CurrentAnim == ANIM_IDLE_SCRATCH2) {
                    // Keep sitting / scratching.
                } else {
                    if ((PTIME(p_person) & 0x3) == 0) {
                        PCOM_set_person_move_animation(p_person, ANIM_IDLE_SCRATCH2);
                    } else {
                        PCOM_set_person_move_still(p_person);
                    }
                }
            }
        } else {
            // Check distance from home position.
            dist = PCOM_get_dist_from_home(p_person);

            if (dist > 256) {
                // Too far -- walk back home.
                PCOM_set_person_ai_homesick(p_person);
            } else {
                // Face the home direction.
                p_person->Draw.Tweened->Angle = p_person->Genus.Person->HomeYaw << 3;

                if (dist > PCOM_ARRIVE_DIST) {
                    if (!person_drawn_recently(p_person)) {
                        PCOM_teleport_home(p_person);
                    }
                } else {
                    // At home. Start a special animation if needed.
                    if (p_person->Genus.Person->pcom_move == PCOM_MOVE_DANCE || p_person->Genus.Person->pcom_move == PCOM_MOVE_HANDS_UP || p_person->Genus.Person->pcom_move == PCOM_MOVE_TIED_UP) {
                        UWORD anim;

                        if (p_person->Genus.Person->pcom_move == PCOM_MOVE_DANCE) {
                            static UWORD dance_anim[4] = {
                                ANIM_DANCE_BOOGIE,
                                ANIM_DANCE_WOOGIE,
                                ANIM_DANCE_HEADBANG,
                                ANIM_DANCE_BOOGIE
                            };

                            anim = dance_anim[THING_NUMBER(p_person) & 0x3];
                        } else {
                            // HANDS_UP and TIED_UP both use the hands-up animation.
                            anim = ANIM_HANDS_UP;
                        }

                        PCOM_set_person_move_animation(p_person, anim);

                        // Loop the animation in SUB_STATE_SIMPLE_ANIM_OVER.
                        p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
                    } else if (p_person->Genus.Person->pcom_bent & PCOM_BENT_LAZY) {
                        if ((p_person->SubState == SUB_STATE_SIMPLE_ANIM) || (p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER)) {
                            // Already sitting -- stay.
                            break;
                        }

                        if ((PTIME(p_person) & 0x3f) == 0) {
                            // Look for a bench or sofa nearby to sit on.
                            SLONG mx;
                            SLONG mz;
                            SLONG mx1;
                            SLONG mz1;
                            SLONG mx2;
                            SLONG mz2;
                            SLONG dx;
                            SLONG dy;
                            SLONG dz;
                            SLONG dist;
                            SLONG best_x;
                            SLONG best_y;
                            SLONG best_z;
                            SLONG best_yaw;
                            SLONG best_prim = NULL;
                            SLONG best_dist = PCOM_MAX_BENCH_WALK;

                            OB_Info* oi;

                            mx1 = (p_person->WorldPos.X >> 8) - PCOM_MAX_BENCH_WALK >> PAP_SHIFT_LO;
                            mz1 = (p_person->WorldPos.Z >> 8) - PCOM_MAX_BENCH_WALK >> PAP_SHIFT_LO;

                            mx2 = (p_person->WorldPos.X >> 8) + PCOM_MAX_BENCH_WALK >> PAP_SHIFT_LO;
                            mz2 = (p_person->WorldPos.Z >> 8) + PCOM_MAX_BENCH_WALK >> PAP_SHIFT_LO;

                            for (mx = mx1; mx <= mx2; mx++)
                                for (mz = mz1; mz <= mz2; mz++) {
                                    for (oi = OB_find(mx, mz); oi->prim; oi++) {
                                        if (oi->prim == PRIM_OBJ_PARK_BENCH || oi->prim == PRIM_OBJ_SOFA || oi->prim == PRIM_OBJ_ARMCHAIR) {
                                            dx = oi->x - (p_person->WorldPos.X >> 8);
                                            dy = oi->y - (p_person->WorldPos.Y >> 8);
                                            dz = oi->z - (p_person->WorldPos.Z >> 8);

                                            dist = abs(dx) + abs(dz) + abs(dy + dy);

                                            if (best_dist > dist) {
                                                best_prim = oi->prim;
                                                best_dist = dist;
                                                best_x = oi->x;
                                                best_y = oi->y;
                                                best_z = oi->z;
                                                best_yaw = oi->yaw;
                                            }
                                        }
                                    }
                                }

                            if (best_prim) {
                                if (PCOM_position_person_to_sit_on_prim(
                                        p_person,
                                        best_prim,
                                        best_x,
                                        best_y,
                                        best_z,
                                        best_yaw,
                                        person_drawn_recently(p_person))) {
                                    PCOM_set_person_move_animation(p_person, ANIM_SIT_DOWN);

                                    p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
                                }
                            }
                        }
                    } else if (p_person->Genus.Person->pcom_ai == PCOM_AI_GUARD) {
                        // Guards draw their gun while idle (unless they carry a pistol).
                        if (PCOM_person_has_any_sort_of_gun(p_person) && !(p_person->Flags & FLAGS_HAS_GUN)) {
                            if (!PCOM_person_has_gun_in_hand(p_person)) {
                                if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_STILL) {
                                    PCOM_set_person_move_draw_gun(p_person);
                                }
                            }
                        }
                    }
                }
            }
        }

        break;

    case PCOM_MOVE_PATROL:
    case PCOM_MOVE_PATROL_RAND:
        PCOM_process_patrol(p_person);
        break;

    case PCOM_MOVE_WANDER:
        PCOM_process_wander(p_person);
        break;

    case PCOM_MOVE_FOLLOW:

        if (p_person->Genus.Person->pcom_ai_substate == PCOM_AI_SUBSTATE_CANTFIND) {
            if ((GAME_TURN & 0xf) == 0) {
                i_target = EWAY_get_person(p_person->Genus.Person->pcom_move_follow);

                if (i_target) {
                    if (can_a_see_b(p_person, TO_THING(i_target))) {
                        PCOM_set_person_ai_follow(p_person, TO_THING(i_target));
                    }
                }

            } else {
                PCOM_process_wander(p_person);
            }
        } else {
            i_target = EWAY_get_person(p_person->Genus.Person->pcom_move_follow);

            if (i_target) {
                PCOM_set_person_ai_follow(p_person, TO_THING(i_target));
            }
        }

        break;

    case PCOM_MOVE_WARM_HANDS:

        if (PCOM_get_dist_from_home(p_person) > 0x200) {
            // Go home first so the correct barrel is selected.
            PCOM_set_person_ai_homesick(p_person);
        } else {
            PCOM_set_person_ai_warm_hands(p_person);
        }

        break;

    case PCOM_MOVE_FOLLOW_ON_SEE:

        // If close enough and has line of sight, switch to active FOLLOW mode.
        i_target = EWAY_get_person(p_person->Genus.Person->pcom_move_follow);

        if (i_target) {
            if (PCOM_get_dist_between(p_person, TO_THING(i_target)) < 0x200) {
                if (can_a_see_b(p_person, TO_THING(i_target))) {
                    p_person->Genus.Person->pcom_move = PCOM_MOVE_FOLLOW;
                }
            }
        }

        break;

    default:
        ASSERT(0);
        break;
    }
}

// Scans for an active bomb (SPECIAL_BOMB + SPECIAL_SUBSTATE_ACTIVATED) within 0x300 units
// that p_person has line of sight to. Returns thing index of the nearest such bomb, or NULL.
// uc_orig: PCOM_find_bomb (fallen/Source/pcom.cpp)
UWORD PCOM_find_bomb(Thing* p_person)
{
    SLONG i;
    SLONG score;

    SLONG best_thing;
    SLONG best_score;

    Thing* p_found;

    PCOM_found_num = THING_find_sphere(
        p_person->WorldPos.X >> 8,
        p_person->WorldPos.Y >> 8,
        p_person->WorldPos.Z >> 8,
        0x300,
        PCOM_found,
        PCOM_MAX_FIND,
        1 << CLASS_SPECIAL);

    best_score = +UC_INFINITY;
    best_thing = NULL;

    for (i = 0; i < PCOM_found_num; i++) {
        p_found = TO_THING(PCOM_found[i]);

        if (p_found->Genus.Special->SpecialType == SPECIAL_BOMB && p_found->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
            if (can_a_see_b(p_person, p_found)) {
                score = THING_dist_between(p_person, p_found);

                if (score < best_score) {
                    best_score = score;
                    best_thing = PCOM_found[i];
                }
            }
        }
    }

    return best_thing;
}

// Routes to the correct PCOM_process_*() handler based on pcom_ai_state.
// Called from PCOM_process_state_change for most AI types.
// uc_orig: PCOM_process_default (fallen/Source/pcom.cpp)
void PCOM_process_default(Thing* p_person)
{
    switch (p_person->Genus.Person->pcom_ai_state) {
    case PCOM_AI_STATE_NORMAL:
        PCOM_process_normal(p_person);
        break;

    case PCOM_AI_STATE_INVESTIGATING:
        PCOM_process_investigating(p_person);
        break;

    case PCOM_AI_STATE_SEARCHING:
        break;

    case PCOM_AI_STATE_KILLING:
        PCOM_process_killing(p_person);
        break;

    case PCOM_AI_STATE_SLEEPING:
        break;

    case PCOM_AI_STATE_FLEE_PLACE:
    case PCOM_AI_STATE_FLEE_PERSON:
        PCOM_process_fleeing(p_person);
        break;

    case PCOM_AI_STATE_FOLLOWING:
        PCOM_process_following(p_person);
        break;

    case PCOM_AI_STATE_NAVTOKILL:
        PCOM_process_navtokill(p_person);
        break;

    case PCOM_AI_STATE_HOMESICK:
        PCOM_process_homesick(p_person);
        break;

    case PCOM_AI_STATE_LOOKAROUND:
        break;

    case PCOM_AI_STATE_FINDCAR:
        PCOM_process_findcar(p_person);
        break;

    case PCOM_AI_STATE_BDEACTIVATE:
        PCOM_process_bdeactivate(p_person);
        break;

    case PCOM_AI_STATE_LEAVECAR:
        PCOM_process_leavecar(p_person);
        break;

    case PCOM_AI_STATE_SNIPE:
        PCOM_process_snipe(p_person);
        break;

    case PCOM_AI_STATE_WARM_HANDS:
        PCOM_process_warm_hands(p_person);
        break;

    case PCOM_AI_STATE_KNOCKEDOUT:
        PCOM_process_knockedout(p_person);
        break;

    case PCOM_AI_STATE_TAUNT:
        PCOM_process_taunt(p_person);
        break;

    case PCOM_AI_STATE_ARREST:
        PCOM_process_arrest(p_person);
        break;

    case PCOM_AI_STATE_TALK:
        PCOM_process_talk(p_person);
        break;

    case PCOM_AI_STATE_HITCH:
        PCOM_process_hitch(p_person);
        break;

    case PCOM_AI_STATE_AIMLESS:
        PCOM_process_wander(p_person);
        break;

    case PCOM_AI_STATE_HANDS_UP:
        PCOM_process_hands_up(p_person);
        break;

    case PCOM_AI_STATE_GETITEM:
        PCOM_process_getitem(p_person);
        break;

    default:
        ASSERT(0);
        break;
    }
}

// When a MIB/GUARD/GANG/FIGHT_TEST NPC spots a threat, simultaneously aggros all
// nearby NPCs of the same type within radius 0x500.
// uc_orig: PCOM_alert_nearby_mib_to_attack (fallen/Source/pcom.cpp)
void PCOM_alert_nearby_mib_to_attack(Thing* p_person)
{
    {
        SLONG i;
        SLONG num_found;
        Thing* p_found;

        num_found = THING_find_sphere(
            p_person->WorldPos.X >> 8,
            p_person->WorldPos.Y >> 8,
            p_person->WorldPos.Z >> 8,
            0x500,
            THING_array,
            THING_ARRAY_SIZE,
            1 << CLASS_PERSON);

        for (i = 0; i < num_found; i++) {
            p_found = TO_THING(THING_array[i]);

            if (p_found->Genus.Person->pcom_ai == PCOM_AI_MIB || p_found->Genus.Person->pcom_ai == PCOM_AI_GUARD || p_found->Genus.Person->pcom_ai == PCOM_AI_GANG || p_found->Genus.Person->pcom_ai == PCOM_AI_FIGHT_TEST) {
                if (!(p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS)) {
                    if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING) {
                        PCOM_set_person_ai_navtokill(p_found, NET_PERSON(0));
                    }
                }
            }
        }
    }
}

// Scans nearby persons and returns the nearest hostile that is actively attacking
// p_client or p_bodyguard (pcom_ai_state KILLING or NAVTOKILL, target == client/guard).
// Knocked-out attackers and those already targeted by the client are deprioritised.
// Returns NULL if no such threat is found.
// uc_orig: PCOM_find_bodyguard_victim (fallen/Source/pcom.cpp)
Thing* PCOM_find_bodyguard_victim(Thing* p_bodyguard, Thing* p_client)
{
    SLONG i;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG num_found;
    SLONG best_score = UC_INFINITY;
    Thing* best_victim = NULL;
    Thing* p_found;

    num_found = THING_find_sphere(
        p_bodyguard->WorldPos.X >> 8,
        p_bodyguard->WorldPos.Y >> 8,
        p_bodyguard->WorldPos.Z >> 8,
        0x800,
        THING_array,
        THING_ARRAY_SIZE,
        1 << CLASS_PERSON);

    for (i = 0; i < num_found; i++) {
        p_found = TO_THING(THING_array[i]);

        if (is_person_dead(p_found)) {
            continue;
        }

        if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_KILLING || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL) {
            if (p_found->Genus.Person->pcom_ai_arg == THING_NUMBER(p_client) || p_found->Genus.Person->pcom_ai_arg == THING_NUMBER(p_bodyguard)) {
                // This person is attacking the client or bodyguard.
                dx = p_found->WorldPos.X - p_bodyguard->WorldPos.X;
                dy = p_found->WorldPos.Y - p_bodyguard->WorldPos.Y;
                dz = p_found->WorldPos.Z - p_bodyguard->WorldPos.Z;

                dist = abs(dx) + abs(dz) + abs(dy << 1);

                if (p_client->Genus.Person->Target == THING_array[i]) {
                    // Deprioritise targets the client is already engaging
                    // so bodyguard covers another attacker.
                    dist <<= 2;
                }

                if (is_person_ko(p_found)) {
                    // KO'd attackers pose less of a threat.
                    dist <<= 1;
                }

                if (dist < best_score) {
                    best_score = dist;
                    best_victim = p_found;
                }
            }
        }
    }

    return best_victim;
}

// Forward declarations for functions defined in Person.cpp / combat.cpp that have no header.
// uc_orig: set_person_fight_idle (fallen/Source/Person.cpp)
extern void set_person_fight_idle(Thing* p_person);
// uc_orig: people_allowed_to_hit_each_other (fallen/Source/Person.cpp)
extern SLONG people_allowed_to_hit_each_other(Thing* p_victim, Thing* p_agressor);


// uc_orig: PCOM_process_state_change (fallen/Source/pcom.cpp)
void PCOM_process_state_change(Thing* p_person)
{
    SLONG dist;
    SLONG home_x;
    SLONG home_z;
    SLONG bomb;
    Thing* p_target;
    SLONG i_target;

    // By default no NPC should have their gun drawn while idle.
    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
        if ((p_person->Flags & FLAGS_HAS_GUN) && (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)) {
            if (p_person->Genus.Person->pcom_move_state != PCOM_MOVE_STATE_ANIMATION || p_person->Genus.Person->pcom_move_substate != PCOM_MOVE_SUBSTATE_GUNAWAY) {
                PCOM_set_person_move_gun_away(p_person);
            }
            return;
        }
    }

    // Non-driver AIs that find themselves driving exit the vehicle.
    if ((p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) && p_person->Genus.Person->pcom_ai != PCOM_AI_DRIVER && p_person->Genus.Person->pcom_ai != PCOM_AI_COP_DRIVER && p_person->Genus.Person->pcom_ai_state != PCOM_AI_STATE_LEAVECAR) {
        PCOM_set_person_ai_leavecar(p_person, PCOM_EXCAR_NORMAL, 0, 0);
    }

    switch (p_person->Genus.Person->pcom_ai) {
    case PCOM_AI_NONE:
        break;

    case PCOM_AI_CIV:
        PCOM_process_default(p_person);
        /*
                if (((GAME_TURN + THING_NUMBER(p_person)) & 0xff) == 0)
                {
                        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL   ||
                                p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK ||
                                p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS)
                        {
                                if (p_person->Genus.Person->pcom_ai_memory && !EWAY_stop_player_moving())
                                {
                                        Thing *p_nasty = TO_THING(p_person->Genus.Person->pcom_ai_memory);
                                        if (PCOM_get_dist_between(p_person, p_nasty) < 0x120)
                                        {
                                                PCOM_set_person_ai_talk_to(p_person, p_nasty, PCOM_AI_SUBSTATE_TALK_TELL, UC_FALSE);
                                                if (p_nasty->Genus.Person->PlayerID && p_nasty->Genus.Person->PersonType == PERSON_DARCI)
                                                {
                                                        PANEL_new_text(p_person, 4000, EWAY_get_fake_wander_message(EWAY_FAKE_MESSAGE_ANNOYED));
                                                }
                                        }
                                }
                        }
                }
        */
        break;

    case PCOM_AI_GUARD:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NAVTOKILL:

        {
            Thing* p_target = TO_THING(p_person->Genus.Person->pcom_ai_arg);

            if (PCOM_get_dist_between(p_person, p_target) > 20 * 0x100) {
                // Target too far away -- give up.
                PCOM_set_person_ai_normal(p_person);
                return;
            }
        }

            if (p_person->Genus.Person->pcom_zone) {
                // Zone code will keep the guard in range; just pursue.
                PCOM_process_navtokill(p_person);
            } else if (p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER) {
                // Wandering guards don't check home distance.
                PCOM_process_navtokill(p_person);
            } else {
                // If the guard is far from his guard post, return home first.
                home_x = p_person->Genus.Person->HomeX; //<< 8;
                home_z = p_person->Genus.Person->HomeZ; //<< 8;

                dist = PCOM_person_dist_from(p_person, home_x, home_z);

                if (dist > 16 * 0x100) // If more than sixteen mapsquares away
                {
                    PCOM_set_person_ai_homesick(p_person);
                } else {
                    PCOM_process_navtokill(p_person);
                }
            }

            break;

        default:
            PCOM_process_default(p_person);
            break;
        }

        // Every 4 ticks check if the guard can see a threat.
        //			if ((GAME_TURN & 0x3) == 0)
        if ((PTIME(p_person) & 0x3) == 0) {
            SLONG look = UC_FALSE;

            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK) {
                look = UC_TRUE;
            }

            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING) {
                if (p_person->Genus.Person->pcom_ai_substate == PCOM_AI_SUBSTATE_SUPRISED) {
                    look = (p_person->Genus.Person->pcom_ai_counter >= PCOM_get_duration(4));
                } else {
                    look = UC_TRUE;
                }
            }

            if (look) {
                // Can you see the player?
                p_target = PCOM_can_i_see_person_to_attack(p_person);

                if (p_target) {
                    PCOM_alert_nearby_mib_to_attack(p_person);
                    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
                        // cant navigate to enemy so taunt
                        PCOM_set_person_ai_taunt(p_person, p_target);
                    }
                    //						else //play wav anyway MikeD
                    {
                        /*
                                                        if(((GAME_TURN+THING_NUMBER(p_person))&255)==0)
                                                                MFX_play_thing(THING_NUMBER(p_person),S_HEY_YOU,MFX_REPLACE,p_person);
                        */
                    }
                    PCOM_oscillate_tympanum(
                        PCOM_SOUND_HEY,
                        p_person,
                        p_person->WorldPos.X >> 8,
                        p_person->WorldPos.Y >> 8,
                        p_person->WorldPos.Z >> 8);

                    // track_enemy(p_person);
                }
            }
        }

        break;

    case PCOM_AI_ASSASIN:
    case PCOM_AI_SHOOT_DEAD:

        if (p_person->Genus.Person->pcom_ai_other == NULL) {
        } else {
            if (p_person->Genus.Person->pcom_ai_other == NULL) {
            } else {

                i_target = EWAY_get_person(p_person->Genus.Person->pcom_ai_other);

                if (i_target) {
                    p_target = TO_THING(i_target);

                    if (is_person_dead(p_target)) {
                        // Assassin changes to gang behaviour once the target is dead.
                        p_person->Genus.Person->pcom_ai = PCOM_AI_GANG;
                        p_person->Genus.Person->pcom_move = PCOM_MOVE_WANDER;
                        PCOM_set_person_ai_normal(p_person);
                        return;
                    }

                    switch (p_person->Genus.Person->pcom_ai_state) {
                    case PCOM_AI_STATE_NORMAL:

                        switch (p_person->Genus.Person->pcom_move) {
                        case PCOM_MOVE_STILL:
                            PCOM_set_person_ai_snipe(p_person, p_target);
                            break;
                        case PCOM_MOVE_PATROL:
                        case PCOM_MOVE_PATROL_RAND:
                        case PCOM_MOVE_WANDER:
                            p_person->Genus.Person->pcom_move = PCOM_MOVE_STILL;
                            break;
                        case PCOM_MOVE_FOLLOW:
                            PCOM_set_person_ai_navtokill(p_person, p_target);
                            break;
                        }

                        break;

                    default:
                        PCOM_process_default(p_person);
                        break;
                    }
                } else {
                    // This assasin has no target yet...
                    PCOM_process_default(p_person);
                }
            }
        }

        break;

    case PCOM_AI_BOSS:
        PCOM_process_default(p_person);
        break;

    case PCOM_AI_COP:
        PCOM_process_default(p_person);
        break;

    case PCOM_AI_GANG:

        PCOM_process_default(p_person);

        {
            SLONG look = UC_FALSE;

            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING) {
                if (p_person->Genus.Person->pcom_ai_substate == PCOM_AI_SUBSTATE_SUPRISED) {
                    look = (p_person->Genus.Person->pcom_ai_counter >= PCOM_get_duration(4));
                } else {
                    look = UC_TRUE;
                }
            }

            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK) {
                look = UC_TRUE;
            }

            if (look) {
                ASSERT(p_person->SubState != SUB_STATE_GRAPPLEE);
                ASSERT(p_person->SubState != SUB_STATE_GRAPPLE_HELD);

                if ((PTIME(p_person) & 0x7) == 0)
                //					if ((GAME_TURN & 0x7) == 0)
                {
                    // Can I see somebody worth taunting?
                    p_target = PCOM_can_i_see_person_to_taunt(p_person);

                    if (p_target) {
                        PCOM_set_person_ai_taunt(p_person, p_target);

                        PCOM_oscillate_tympanum(
                            PCOM_SOUND_WANKER,
                            p_person,
                            p_person->WorldPos.X >> 8,
                            p_person->WorldPos.Y >> 8,
                            p_person->WorldPos.Z >> 8);

                        return;
                    }
                }
            }
        }

        break;

    case PCOM_AI_DOORMAN:
        PCOM_process_default(p_person);
        break;

    case PCOM_AI_BODYGUARD:

    {
        UWORD i_client = EWAY_get_person(p_person->Genus.Person->pcom_ai_other);
        Thing* p_client = NULL;

        if (i_client) {
            p_client = TO_THING(i_client);
        }

        p_person->Genus.Person->Flags2 &= ~FLAG2_PERSON_INVULNERABLE;

        // Turn invulnerable when very far from player client.
        if (p_client && p_client->Genus.Person->PlayerID) {
            SLONG dx = abs(p_client->WorldPos.X - p_person->WorldPos.X);
            SLONG dy = abs(p_client->WorldPos.Y - p_person->WorldPos.Y);
            SLONG dz = abs(p_client->WorldPos.Z - p_person->WorldPos.Z);

            SLONG dist = QDIST3(dx, dy, dz);

            if (dist > 20 * 0x10000) {
                p_person->Genus.Person->Flags2 |= FLAG2_PERSON_INVULNERABLE;
            }
        }

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:
        case PCOM_AI_STATE_FOLLOWING:

        {
            // Is our client under attack?
            if (p_client) {
                Thing* p_target = PCOM_find_bodyguard_victim(p_person, p_client);

                if (p_target) {
                    //								MFX_play_thing(THING_NUMBER(p_person),S_OIGUVNOR,MFX_REPLACE,p_person);
                    PCOM_set_person_ai_navtokill(p_person, p_target);
                }
            }
        }

            // Fall-through to default processing.

        default:
            PCOM_process_default(p_person);
            break;
        }
    }

    break;

    case PCOM_AI_COP_DRIVER:

        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
            /*
                            if ((GAME_TURN & 0x7) == 0)
                            {
                                    p_target = PCOM_can_i_see_person_to_arrest(p_person);

                                    if (p_target)
                                    {
                                            p_target->Genus.Person->Flags |= FLAG_PERSON_FELON;

                                            PCOM_set_person_ai_leavecar(
                                                    p_person,
                                                    PCOM_EXCAR_ARREST_PERSON,
                                                    0,
                                                    THING_NUMBER(p_target));
                                    }
                            }
            */
        }

        // FALLTHROUGH to driving code.

    case PCOM_AI_DRIVER:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:

            if (!(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING)) {
                // Not in a car -- find one.
                PCOM_set_person_ai_findcar(p_person, NULL);
            } else {
                switch (p_person->Genus.Person->pcom_move) {
                case PCOM_MOVE_STILL:
                    PCOM_process_driving_still(p_person);
                    break;
                case PCOM_MOVE_PATROL:
                case PCOM_MOVE_PATROL_RAND:
                    PCOM_process_driving_patrol(p_person);
                    break;
                case PCOM_MOVE_FOLLOW:
                    break;
                case PCOM_MOVE_WANDER:
                    PCOM_process_driving_wander(p_person);
                    break;
                default:
                    ASSERT(0);
                    break;
                }
            }

            break;

        default:
            PCOM_process_default(p_person);
            break;
        }

        break;

    case PCOM_AI_BDISPOSER:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:

            PCOM_process_normal(p_person);

            bomb = PCOM_find_bomb(p_person);

            if (bomb) {
                PCOM_set_person_ai_bdeactivate(p_person, TO_THING(bomb));
            }

            break;

        case PCOM_AI_STATE_FOLLOWING:

            PCOM_process_following(p_person);

            bomb = PCOM_find_bomb(p_person);

            if (bomb) {
                PCOM_set_person_ai_bdeactivate(p_person, TO_THING(bomb));
            }

            break;

        default:
            PCOM_process_default(p_person);
            break;
        }

        break;

    case PCOM_AI_BIKER:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:

        {
            switch (p_person->Genus.Person->pcom_move) {
            case PCOM_MOVE_STILL:
                PCOM_process_driving_still(p_person);
                break;
            case PCOM_MOVE_PATROL:
            case PCOM_MOVE_PATROL_RAND:
                PCOM_process_driving_patrol(p_person);
                break;
            case PCOM_MOVE_FOLLOW:
                break;
            case PCOM_MOVE_WANDER:
                PCOM_process_driving_wander(p_person);
                break;
            default:
                ASSERT(0);
                break;
            }
        }

        break;

        default:
            PCOM_process_default(p_person);
            break;
        }

        break;

    case PCOM_AI_FIGHT_TEST:

        if (p_person->Genus.Person->pcom_ai_other & PCOM_COMBAT_COMBO_KKK) {
            combo_display = 2;
        } else if (p_person->Genus.Person->pcom_ai_other & PCOM_COMBAT_COMBO_PPP) {
            combo_display = 1;
        }

        PCOM_process_default(p_person);

        if (!(p_person->Genus.Person->pcom_bent & PCOM_BENT_ROBOT)) {
            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS) {
                if (PCOM_get_dist_between(p_person, NET_PERSON(0)) < 0x200) {
                    PCOM_alert_nearby_mib_to_attack(p_person); // Not just MIBs!
                }
            }
        }

        break;

    case PCOM_AI_BULLY:

        //			if(Keys[KB_B])
        //				FC_cam[0].focus	= p_person;

        PCOM_process_default(p_person);

        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS) {
            if ((PTIME(p_person) & 0x7) == 0)
            //				if ((GAME_TURN & 0x7) == 0)
            {
                p_target = PCOM_can_i_see_person_to_bully(p_person);

                if (p_target) {
                    PCOM_set_person_ai_kill_person(p_person, p_target);
                    return;
                }
            }
        }

        break;

    case PCOM_AI_SUICIDE:

        // Instant random death -- simplest possible AI.
        p_person->Genus.Person->Health = 0;

        set_person_dead(
            p_person,
            NULL,
            PERSON_DEATH_TYPE_SHOT_PISTOL, // was combat
            Random() & 0x1,
            Random() % 3);

        if (GAME_TURN < 32) {
            SLONG c0;
            for (c0 = 0; c0 < 100; c0++) {
                if (p_person->StateFn)
                    p_person->StateFn(p_person);
            }
        }

        //			while(person_normal_animate(p_person)==0);

        break;

    case PCOM_AI_FLEE_PLAYER:

        switch (p_person->Genus.Person->pcom_ai_state) {
        case PCOM_AI_STATE_NORMAL:
        case PCOM_AI_STATE_HOMESICK:

            // Is this person too near Darci?
            if (PCOM_get_dist_between(p_person, NET_PERSON(0)) < 0x600) {
                PCOM_set_person_ai_flee_person(p_person, NET_PERSON(0));
            }

            // FALL-THROUGH TO DEFAULT PROCESSING!

        default:
            PCOM_process_default(p_person);
            break;
        }

        break;

    case PCOM_AI_KILL_COLOUR:

        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
            // Find nearest person of the hated colour to kill.
            UWORD list;

            SLONG dist;
            SLONG best_dist = UC_INFINITY;
            Thing* best_target = NULL;
            Thing* p_found;

            list = thing_class_head[CLASS_PERSON];

            SLONG hate_colour;
            SLONG hate_example;

            hate_example = EWAY_get_person(p_person->Genus.Person->pcom_ai_other);

            if (hate_example) {
                Thing* p_example = TO_THING(hate_example);

                ASSERT(p_example->Class == CLASS_PERSON);

                hate_colour = p_example->Genus.Person->pcom_colour;

                while (list) {
                    p_found = TO_THING(list);

                    list = p_found->NextLink;

                    if (p_found->Genus.Person->pcom_colour == hate_colour && !is_person_dead(p_found) && !(p_found->Genus.Person->Flags2 & FLAG2_PERSON_FAKE_WANDER)) {
                        if (p_person->Genus.Person->pcom_zone) {
                            // Ignore people outside this guard's zone.
                            if (!(PCOM_get_zone_for_position(p_found->WorldPos.X >> 8, p_found->WorldPos.Z >> 8) & p_person->Genus.Person->pcom_zone)) {
                                continue;
                            }
                        }

                        dist = PCOM_get_dist_between(p_person, p_found);

                        if (best_dist > dist) {
                            best_dist = dist;
                            best_target = p_found;
                        }
                    }
                }

                if (best_target) {
                    PCOM_set_person_ai_kill_person(p_person, best_target);
                }

                /*
                else
                {
                        if (p_person->Genus.Person->PersonType == PERSON_COP)
                        {
                                p_person->Genus.Person->pcom_ai = PCOM_AI_COP;
                        }
                        else
                        {
                                p_person->Genus.Person->pcom_ai    = PCOM_AI_CIV;
                                p_person->Genus.Person->pcom_bent |= PCOM_BENT_FIGHT_BACK;
                        }
                }
                */
            }
        }

        PCOM_process_default(p_person);

        break;

    case PCOM_AI_MIB:

        if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
            // Kills the player on sight.
            if (can_a_see_b(p_person, NET_PERSON(0)) && !stealth_debug) {
                PCOM_alert_nearby_mib_to_attack(p_person);
            }
        }

        PCOM_process_default(p_person);

        break;

    case PCOM_AI_BANE:

        if (p_person->Genus.Person->pcom_ai_state != PCOM_AI_STATE_SUMMON) {
            PCOM_set_person_ai_summon(p_person);
        }

        PCOM_process_summon(p_person);

        break;

    case PCOM_AI_HYPOCHONDRIA:

        set_person_injured(p_person);

        break;

    default:
        ASSERT(0);
        break;
    }
}

// Bit flags returned by PCOM_find_runover_thing.
// uc_orig: PCOM_RUNOVER_STOP (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_STOP      (1 << 0)
// uc_orig: PCOM_RUNOVER_BEEP_HORN (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_BEEP_HORN (1 << 1)
// uc_orig: PCOM_RUNOVER_SHOUT_OUT (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_SHOUT_OUT (1 << 2)
// uc_orig: PCOM_RUNOVER_TURN_LEFT (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_TURN_LEFT (1 << 3)
// uc_orig: PCOM_RUNOVER_TURN_RIGHT (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_TURN_RIGHT (1 << 4)
// uc_orig: PCOM_RUNOVER_SLOW_DOWN (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_SLOW_DOWN (1 << 5)
// uc_orig: PCOM_RUNOVER_RUNAWAY (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_RUNAWAY   (1 << 6)
// uc_orig: PCOM_RUNOVER_REVERSE (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_REVERSE   (1 << 7)

// BLOCKED(C) is a debug-coloring marker compiled to nothing on PC.
// uc_orig: BLOCKED (fallen/Source/pcom.cpp)
#define BLOCKED(C)

// How many objects PCOM_find_runover_thing scans at once.
// uc_orig: PCOM_RUNOVER_FIND (fallen/Source/pcom.cpp)
#define PCOM_RUNOVER_FIND 8

// uc_orig: PCOM_find_runover_thing (fallen/Source/pcom.cpp)
SLONG PCOM_find_runover_thing(Thing* p_person, SLONG dangle)
{
    SLONG i;

    SLONG dx;
    SLONG dz;

    SLONG what;
    SLONG angle;

    UWORD found[PCOM_RUNOVER_FIND];
    SLONG num;
    SLONG velocity;

    Thing* p_vehicle;
    Vehicle* veh;
    Thing* p_found;
    Vehicle* v_found;

    ASSERT(p_person->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING));

    p_vehicle = TO_THING(p_person->Genus.Person->InCar);
    veh = p_vehicle->Genus.Vehicle;

    // Find all things ahead of the vehicle.
    num = VEH_find_runover_things(p_vehicle, found, PCOM_RUNOVER_FIND, dangle);

    // Vehicle movement vector and speed.
    switch (p_vehicle->Class) {
    case CLASS_VEHICLE:
        angle = p_vehicle->Genus.Vehicle->Angle & 2047;
        velocity = Root((veh->VelX >> 4) * (veh->VelX >> 4) + (veh->VelZ >> 4) * (veh->VelZ >> 4)) >> 4;
        break;

    case CLASS_BIKE:
        angle = p_vehicle->Draw.Mesh->Angle & 2047;
        velocity = 0;
        break;

    default:
        ASSERT(0);
        break;
    }

    dx = -SIN(angle) >> 8;
    dz = -COS(angle) >> 8;

    // get some info
    SLONG wx = p_vehicle->WorldPos.X >> 8;
    SLONG wz = p_vehicle->WorldPos.Z >> 8;

    SLONG onroad = ROAD_is_road(wx >> 8, wz >> 8);

    // find nearest road
    SLONG rn1;
    SLONG rn2;

    if ((p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_DRIVE_DOWN) || (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_PARK_CAR_ON_ROAD)) {
        rn1 = p_person->Genus.Person->pcom_move_arg & 0xFF;
        rn2 = (p_person->Genus.Person->pcom_move_arg >> 8) & 0xFF;
    } else {
        ROAD_find(wx, wz, &rn1, &rn2);
    }

    // get side
    SLONG rd = ROAD_signed_dist(rn1, rn2, wx, wz);

    // find nearest node
    SLONG nn;
    SLONG nnd;

    nn = ROAD_nearest_node(rn1, rn2, wx, wz, &nnd);

    // set action to none
    what = 0;

    // if we're near a junction, add objects
    SLONG orig_num = num;

    if (nnd < NEAR_JUNCTION) {
        SLONG jx, jz;

        ROAD_node_pos(nn, &jx, &jz);

        if ((jx - wx) * dx + (jz - wz) * dz > 0) {
            // approaching it
            num += THING_find_sphere(jx, p_person->WorldPos.Y >> 8, jz, JN_RADIUS_IN, found + num, PCOM_RUNOVER_FIND - num, (1 << CLASS_PERSON) | (1 << CLASS_VEHICLE));
            what = PCOM_RUNOVER_SLOW_DOWN;
        }
    }

    // remove self from list
    for (i = 0; i < num; i++) {
        if ((found[i] == THING_NUMBER(p_person)) || (found[i] == THING_NUMBER(p_vehicle))) {
            found[i] = found[--num];
        }
    }

    // check objects in list
    for (i = 0; i < num; i++) {
        p_found = TO_THING(found[i]);

        BLOCKED(0);

        if ((p_found->Class == CLASS_PERSON) && (p_found->State == STATE_DYING || p_found->State == STATE_DEAD)) {
            // Ignore dead or dying people
            continue;
        }

        // if it's on a junction, check dot product
        if (num >= orig_num) {
            SLONG vx = (p_found->WorldPos.X - p_person->WorldPos.X) >> 8;
            SLONG vz = (p_found->WorldPos.Z - p_person->WorldPos.Z) >> 8;

            if (vx * dx + vz * dz < 0)
                continue; // behind you!
        }

        BLOCKED(0x0000FF);

        // if we're on the road ...
        if (onroad) {
            // then ignore off-road things
            if (!ROAD_is_road(p_found->WorldPos.X >> 16, p_found->WorldPos.Z >> 16))
                continue;

            // find which road the thing is on
            SLONG trn1;
            SLONG trn2;

            ROAD_find(p_found->WorldPos.X >> 8, p_found->WorldPos.Z >> 8, &trn1, &trn2);

            // find nearest node
            SLONG tnn;
            SLONG tnnd;

            tnn = ROAD_nearest_node(trn1, trn2, p_found->WorldPos.X >> 8, p_found->WorldPos.Z >> 8, &tnnd);

            // handle junctions
            // #if 0
            if ((nnd < AT_JUNCTION) && (p_found->Class == CLASS_VEHICLE)) {
                // *on* the junction
                if ((tnnd < AT_JUNCTION) && (tnn == nn) && (ROAD_node_degree(nn) > 2)) {
                    // another car is on the junction
                    if (p_vehicle < p_found) {
                        // reverse back to make room
                        BLOCKED(0x888888);
                        return PCOM_RUNOVER_REVERSE;
                    }
                }
                // ignore other cars when we're *on* the junction
                continue;
            }
            // #endif

            if ((nnd < NEAR_JUNCTION) && (p_found->Class == CLASS_VEHICLE) && (tnn == nn) && (ROAD_node_degree(nn) > 2)) {
                // near junction, looking at a car on the same junction, not a bend
                if (tnnd < AT_JUNCTION) {
                    // stop if approaching a busy junction
                    BLOCKED(0x888888);
                    return PCOM_RUNOVER_STOP;
                } else if (tnnd < NEAR_JUNCTION) {
                    // stop if both approaching and other car is nearer
                    if (tnnd < nnd) {
                        BLOCKED(0x888888);
                        return PCOM_RUNOVER_STOP;
                    }
                }
            }

            if (nnd > AT_JUNCTION) // && (abs(dangle) < 32))
            {
                // look at the side of the road we're both on
                if (((rn1 == trn1) && (rn2 == trn2)) || ((rn1 == trn2) && (rn2 == trn1))) {
                    SLONG trd = ROAD_signed_dist(rn1, rn2, p_found->WorldPos.X >> 8, p_found->WorldPos.Z >> 8);

                    if (abs(trd - rd) > 0xC0)
                        continue;
                }
            }
        }

        switch (p_found->Class) {
        case CLASS_PERSON:
            if (p_found->OnFace)
                break; // ignore people on cars

            if ((veh->Type != VEH_TYPE_VAN) && (veh->Type != VEH_TYPE_AMBULANCE) && (veh->Type != VEH_TYPE_WILDCATVAN)) {
                // can't hijack vans or ambulances
                if ((p_found->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) || (p_found->Genus.Person->SpecialUse)) {
                    // person has a gun out
                    SLONG angle = calc_angle(dx, dz);
                    SLONG dangle = p_found->Draw.Tweened->Angle - angle;

                    if (abs(dangle) < 256) {
                        // pointing it in our direction
                        if (veh->Flags & FLAG_VEH_SHOT_AT) {
                            // and shooting it!
                            if (Random() & 0x08) {
                                // scared!
                                PCOM_runover_scary_person = p_found;
                                BLOCKED(0x888888);
                                return PCOM_RUNOVER_STOP | PCOM_RUNOVER_RUNAWAY;
                            } else {
                                // not scared
                                veh->Flags &= ~FLAG_VEH_SHOT_AT;
                            }
                        }
                    }
                }
            }

            if (ROAD_is_zebra(
                    p_found->WorldPos.X >> 16,
                    p_found->WorldPos.Z >> 16)) {
                // Always stop for people on zebra crossings...
                BLOCKED(0x888888);
                return PCOM_RUNOVER_STOP;
            } else {
                BLOCKED(0x888888);
                switch (((GAME_TURN) + THING_NUMBER(p_vehicle)) & 0x7f) {
                case 0:
                    return PCOM_RUNOVER_STOP | PCOM_RUNOVER_BEEP_HORN;
                case 25:
                    return PCOM_RUNOVER_STOP | PCOM_RUNOVER_SHOUT_OUT;
                default:
                    return PCOM_RUNOVER_STOP;
                }
            }

            break;

        case CLASS_VEHICLE: {
            v_found = p_found->Genus.Vehicle;

            SLONG vel = Root((v_found->VelX >> 4) * (v_found->VelX >> 4) + (v_found->VelZ >> 4) * (v_found->VelZ >> 4)) >> 4;

            // Do we stop for a car in front of us or do we try and drive around?
            SLONG avoid = UC_FALSE;

            // avoid parked (driverless) cars and dead cars
            if (!v_found->Driver)
                avoid = UC_TRUE;
            if ((p_found->State == STATE_DYING) || (p_found->State == STATE_DEAD))
                avoid = UC_TRUE;

            if (p_person->Genus.Person->pcom_bent & PCOM_BENT_DILIGENT) {
                // Diligent people are in a hurry!
                avoid = UC_TRUE;
            }

            if (avoid) {
                if (vel < (velocity >> 2)) {
                    // no, just stop
                    BLOCKED(0x888888);
                    return PCOM_RUNOVER_STOP;
                } else // if (vel < velocity)
                {
                    BLOCKED(0xFF0000);
                    return PCOM_RUNOVER_STOP;
                }
            } else {
                //						if (vel < velocity)
                {
                    BLOCKED(0x00FF00);
                    return PCOM_RUNOVER_STOP;
                }
            }
        } break;

        default:
            ASSERT(0);
            break;
        }
    }

    return what;
}

// uc_orig: PCOM_process_movement (fallen/Source/pcom.cpp)
void PCOM_process_movement(Thing* p_person)
{
    SLONG dist;
    SLONG goal_x;
    SLONG goal_z;
    SLONG ladder;

    SLONG renav = UC_FALSE;

    if (p_person->State == STATE_DYING || p_person->State == STATE_DEAD)
        return;

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_PLAYER:
        break;

    case PCOM_MOVE_STATE_STILL:

        if (p_person->SubState == SUB_STATE_GRAPPLE_HELD) {
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;
        }

        break;

    case PCOM_MOVE_STATE_GOTO_THING_SLIDE:

        // Finished sliding?
        if (p_person->State == STATE_IDLE || p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN) {
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
        } else {
            // Keep facing the slide target.
            ASSERT(p_person->Genus.Person->pcom_move_arg);
            turn_towards_thing(p_person, TO_THING(p_person->Genus.Person->pcom_move_arg));
        }

        break;

    case PCOM_MOVE_STATE_GOTO_XZ:
    case PCOM_MOVE_STATE_GOTO_WAYPOINT:
    case PCOM_MOVE_STATE_GOTO_THING:

        if (p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS) {
            break;
        }

        switch (p_person->Genus.Person->pcom_move_substate) {
        case PCOM_MOVE_SUBSTATE_GOTO:

            PCOM_get_mav_action_pos(p_person, &goal_x, &goal_z);
            if (ControlFlag && allow_debug_keys) {
                AENG_world_line(
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Y >> 8,
                    p_person->WorldPos.Z >> 8,
                    16,
                    0x00ffff00,
                    goal_x,
                    p_person->WorldPos.Y >> 8,
                    goal_z,
                    0,
                    0x000000ff,
                    UC_TRUE);
            }
            /*
                                    goal_x &= 0xffffff00;
                                    goal_z &= 0xffffff00;

                                    goal_x |= 0x80;
                                    goal_z |= 0x80;
            */
            dist = PCOM_person_dist_from(p_person, goal_x, goal_z);

            if (dist < PCOM_ARRIVE_DIST) {
                if (p_person->Genus.Person->pcom_move_ma.action == MAV_ACTION_GOTO) {
                    // Reached subgoal -- request renavigation to next.
                    renav = UC_TRUE;
                } else {
                    // Arrived but need to perform an action (jump, pullup, ladder, etc.).
                    p_person->Draw.Tweened->AngleTo = p_person->Draw.Tweened->Angle = PCOM_get_angle_for_dir(p_person->Genus.Person->pcom_move_ma.dir);

                    switch (p_person->Genus.Person->pcom_move_ma.action) {
                    case MAV_ACTION_JUMP:
                    case MAV_ACTION_JUMPPULL:
                    case MAV_ACTION_JUMPPULL2:
                        set_person_running_jump(p_person);
                        break;

                    case MAV_ACTION_PULLUP:
                        set_person_running_jump(p_person);
                        break;

                    case MAV_ACTION_CLIMB_OVER:
                        set_person_running_jump(p_person);
                        break;

                    case MAV_ACTION_FALL_OFF:
                        set_person_walking(p_person);
                        break;

                    case MAV_ACTION_LADDER_UP:

                        ladder = find_nearby_ladder_colvect(p_person);

                        if (ladder) {
                            set_person_climb_ladder(p_person, ladder);
                        } else {
                            ASSERT(0);
                        }

                        break;

                    default:
                        ASSERT(0);
                        break;
                    }

                    // Process the action.
                    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_ACTION;
                }
            } else {
                if (p_person->State == STATE_DANGLING && p_person->SubState == SUB_STATE_DANGLING) {
                    // Fell and ended up dangling -- pull up immediately.
                    set_person_pulling_up(p_person);
                } else if (p_person->Genus.Person->SlideOdd >= 50) {
                    if (p_person->Genus.Person->pcom_ai == PCOM_AI_CIV && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER && p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
                        // Wandering civs don't jump.
                    } else {
                        // Stuck sliding for 50 frames -- force a jump to escape.
                        set_person_running_jump(p_person);

                        p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_ACTION;
                        p_person->Genus.Person->SlideOdd = 0;
                    }
                } else if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(20)) {
                    // Timeout -- force renavigation if in a safe state.
                    if ((p_person->State == STATE_GOTOING) || (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN) || (p_person->State == STATE_IDLE)) {
                        renav = UC_TRUE;
                    } else {
                        // Don't interrupt a complex manoeuvre (ladder/jump).
                    }
                }
            }

            break;

        case PCOM_MOVE_SUBSTATE_ACTION:

            switch (p_person->State) {
            case STATE_MOVEING:

                if (p_person->SubState == SUB_STATE_RUNNING) {
                    // Running after completing a jump -- renav.
                    renav = UC_TRUE;
                }

                break;

            case STATE_IDLE:

                // Stopped -- time for a renavigation!
                renav = UC_TRUE;

                break;

            case STATE_LANDING:
            case STATE_JUMPING:
            case STATE_FIGHTING:
            case STATE_FALLING:
            case STATE_USE_SCENERY:
            case STATE_DOWN:
            case STATE_HIT:
            case STATE_CHANGE_LOCATION:
            case STATE_DYING:
            case STATE_DEAD:
                break;

            case STATE_DANGLING:

                if (p_person->SubState == SUB_STATE_DANGLING) {
                    switch (p_person->Genus.Person->pcom_move_ma.action) {
                    case MAV_ACTION_JUMPPULL:
                    case MAV_ACTION_JUMPPULL2:
                    case MAV_ACTION_PULLUP:
                        set_person_pulling_up(p_person);
                        break;

                    default:
                        set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);
                        break;
                    }
                } else if (p_person->SubState == SUB_STATE_DANGLING_CABLE) {
                    // Should never be on a cable!
                    set_person_drop_down(p_person, PERSON_DROP_DOWN_OFF_FACE);
                }

                break;

            case STATE_CLIMB_LADDER:

                if (p_person->SubState == SUB_STATE_ON_LADDER) {
                    p_person->SubState = SUB_STATE_CLIMB_UP_LADDER;
                }

                break;

            case STATE_HIT_RECOIL:
                break;

            case STATE_CLIMBING:

                if (p_person->SubState == SUB_STATE_STOPPING || p_person->SubState == SUB_STATE_CLIMB_AROUND_WALL) {
                    if (p_person->Genus.Person->pcom_move_ma.action == MAV_ACTION_CLIMB_OVER) {
                        // Climb over.
                        p_person->SubState = SUB_STATE_CLIMB_UP_WALL;
                    } else {
                        // Shouldn't be on a fence.
                        set_person_drop_down(p_person, 0);
                    }
                }

                break;

            case STATE_GUN:

                if (p_person->SubState == SUB_STATE_AIM_GUN) {
                    // Stopped aiming -- renav.
                    renav = UC_TRUE;
                }

                break;

            case STATE_SHOOT:
            case STATE_DRIVING:
            case STATE_NAVIGATING:
            case STATE_WAIT:
            case STATE_FIGHT:
            case STATE_STAND_UP:
            case STATE_MAVIGATING:
            case STATE_GRAPPLING:
                break;

            case STATE_GOTOING:
                break;

            default:
                ASSERT(0);
                break;
            }

            break;

        case PCOM_MOVE_SUBSTATE_LOSMAV:

        {
            ASSERT(p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_GOTO_THING);

            Thing* p_target = TO_THING(p_person->Genus.Person->pcom_move_arg);

            if (!PCOM_should_i_try_to_los_mav_to_person(p_person, p_target)) {
                // Switch to proper MAV navigation.
                renav = UC_TRUE;
            } else {
                // Track target position.
                p_person->Genus.Person->GotoX = p_target->WorldPos.X >> 8;
                p_person->Genus.Person->GotoZ = p_target->WorldPos.Z >> 8;
            }

            if (p_person->State == STATE_IDLE) {
                // Something happened to this bloke! Get him moving again.
                renav = UC_TRUE;
            }
        }

        break;

        default:
            ASSERT(0);
            break;
        }

        if (renav) {
            if (p_person->SubState == SUB_STATE_PULL_UP || p_person->SubState == SUB_STATE_CLIMB_OFF_LADDER_TOP) {
                // Don't renav while doing this...
            } else {
                // Time for a renavigation.
                PCOM_renav(p_person);
            }
        }

        p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        break;

    case PCOM_MOVE_STATE_PAUSE:
        p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
        break;

    case PCOM_MOVE_STATE_WAIT_CIRCLE:
        if ((p_person->State == STATE_IDLE && p_person->SubState != SUB_STATE_IDLE_CROUTCH_ARREST) || (p_person->StateFn == NULL)) {
            // The animation is over.
            set_person_recircle(p_person);
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_CIRCLE;
        } else if (p_person->State == STATE_FIGHTING && (p_person->SubState == SUB_STATE_GRAPPLE_HOLD || p_person->SubState == SUB_STATE_GRAPPLE_HELD)) {
            // The animation is over.
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;
        }
        break;

    case PCOM_MOVE_STATE_ANIMATION:

        p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        if ((p_person->State == STATE_IDLE && p_person->SubState != SUB_STATE_IDLE_CROUTCH_ARREST) || (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN) || (p_person->State == STATE_MOVEING && p_person->SubState == SUB_STATE_INSIDE_VEHICLE) || (p_person->State == STATE_MOVEING && p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER && p_person->Genus.Person->pcom_move != PCOM_MOVE_DANCE && p_person->Genus.Person->pcom_move != PCOM_MOVE_HANDS_UP && p_person->Genus.Person->pcom_move != PCOM_MOVE_TIED_UP) || (p_person->StateFn == NULL)) {
            // The animation is over.
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_STILL;
        } else if (p_person->State == STATE_FIGHTING && (p_person->SubState == SUB_STATE_GRAPPLE_HOLD || p_person->SubState == SUB_STATE_GRAPPLE_HELD)) {
            // The animation is over.
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;
        }

        break;

    case PCOM_MOVE_STATE_GRAPPLE:
        if (p_person->SubState == SUB_STATE_GRAPPLE_HOLD) {
            p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

            if (p_person->Genus.Person->pcom_move_counter++ > p_person->Genus.Person->pcom_ai_counter) {
                if (p_person->Genus.Person->pcom_ai == PCOM_AI_COP || p_person->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER) {
                    // Cops throw the grappled person down then arrest.
                    p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_PUNCH;
                } else {
                    p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_KICK;
                }

                p_person->Genus.Person->pcom_move_counter = 0;
                p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_ANIMATION;
            }
        }
        if (p_person->SubState == SUB_STATE_GRAPPLE_HELD) {
            //				if((GAME_TURN+THING_NUMBER(p_person))&1)
            if ((PTIME(p_person)) & 1) {
                p_person->Genus.Person->Flags |= FLAG_PERSON_REQUEST_BLOCK;
            }
        }
        if (p_person->State == STATE_IDLE) {
            set_person_recircle(p_person); //, TO_THING(p_person->Genus.Person->pcom_move_arg));
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_CIRCLE;
            p_person->Genus.Person->pcom_move_arg = p_person->Genus.Person->Target;
            p_person->Genus.Person->pcom_move_counter = 0;
        }

        break;
    case PCOM_MOVE_STATE_CIRCLE:

        p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

        if (p_person->State == STATE_IDLE || (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN)) {
            // This person has probably been punched and has just recovered.
            set_person_recircle(p_person); //, TO_THING(p_person->Genus.Person->pcom_move_arg));
        } else if ((p_person->SubState == SUB_STATE_GRAPPLE_HELD)) {
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;

        } else if ((p_person->SubState == SUB_STATE_GRAPPLE_HELD)) {
            p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GRAPPLE;
        }

        break;

    case PCOM_MOVE_STATE_PARK_CAR:

        ParkCar(p_person);

        break;

    case PCOM_MOVE_STATE_DRIVETO:
    case PCOM_MOVE_STATE_DRIVE_DOWN:
    case PCOM_MOVE_STATE_PARK_CAR_ON_ROAD:

        DriveCar(p_person);

        break;

    default:
        ASSERT(0);
        break;
    }
}

// ============================================================
// Top-level per-frame NPC entry point
// ============================================================

// Called every game tick for every person. Runs movement AI (StateFn + PCOM_process_movement),
// then state-change logic. Players only run StateFn (idle dance check) and return early.
// Wandering civilians are resurrected here after 200 frames out of view.
// uc_orig: PCOM_process_person (fallen/Source/pcom.cpp)
void PCOM_process_person(Thing* p_person)
{
    if (p_person->StateFn) {
        p_person->StateFn(p_person);
    }

    if (p_person->Genus.Person->PlayerID) {
        // Keep track of how long the player has been idle.
        if (p_person->State == STATE_IDLE) {
            p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;

            if (p_person->Genus.Person->pcom_move_counter > PCOM_get_duration(30)) {
                p_person->Genus.Person->pcom_move_counter = 0;

                UWORD pnear;

                // Is Darci near someone dancing?
                remove_thing_from_map(p_person);

                pnear = THING_find_nearest(
                    p_person->WorldPos.X >> 8,
                    p_person->WorldPos.Y >> 8,
                    p_person->WorldPos.Z >> 8,
                    0x200,
                    1 << CLASS_PERSON);

                add_thing_to_map(p_person);

                if (pnear) {
                    Thing* p_near;

                    // Is this person dancing?
                    p_near = TO_THING(pnear);

                    ASSERT(p_near->Class == CLASS_PERSON);

                    if (p_near->State == STATE_MOVEING && (p_near->SubState == SUB_STATE_SIMPLE_ANIM || p_near->SubState == SUB_STATE_SIMPLE_ANIM_OVER) && (p_near->Draw.Tweened->CurrentAnim == ANIM_DANCE_BOOGIE || p_near->Draw.Tweened->CurrentAnim == ANIM_DANCE_WOOGIE || p_near->Draw.Tweened->CurrentAnim == ANIM_DANCE_HEADBANG)) {
                        // This person is dancing... dance with him!
                        set_face_thing(p_person, p_near);

                        set_person_do_a_simple_anim(p_person, p_near->Draw.Tweened->CurrentAnim);

                        p_person->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
                        p_person->Genus.Person->Action = ACTION_SIT_BENCH;
                    }
                }
            }
        } else {
            p_person->Genus.Person->pcom_move_counter = 0;
        }

        // Players don't need to do the rest of this stuff.
        return;
    }

    PCOM_process_movement(p_person);

    if (p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS) {
        return;
    }

    if (p_person->State == STATE_DEAD || p_person->State == STATE_DYING) {
        // No AI after brain death. But maybe this person should be resurrected.
        if (p_person->Genus.Person->pcom_ai == PCOM_AI_CIV && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER) {
            // Wandering civilians come back to life when out of view after 200 frames.
            // NOTE: newpos is computed but never assigned to p_person->WorldPos before
            // plant_feet(); civ resurrects at current death location, not home position
            // (pre-release bug, preserved 1:1).
            if (!(p_person->Flags & FLAGS_IN_VIEW)) {
                p_person->Genus.Person->pcom_ai_counter += 1;

                if (p_person->Genus.Person->pcom_ai_counter > 200) {
                    // Bring this person back to life. Put him at home.
                    GameCoord newpos;

                    newpos.X = (p_person->Genus.Person->HomeX << 16) + 0x8000;
                    newpos.Z = (p_person->Genus.Person->HomeZ << 16) + 0x8000;
                    newpos.Y = PAP_calc_map_height_at(newpos.X >> 8, newpos.Z >> 8);

                    extern SLONG plant_feet(Thing * p_person); // in collide.cpp

                    plant_feet(p_person);

                    // Give him back his health.
                    p_person->Genus.Person->Health = health[p_person->Genus.Person->PersonType];

                    // Make him start doing what he normally does.
                    p_person->Flags &= ~FLAGS_BURNING;
                    p_person->Genus.Person->BurnIndex = 0;

                    PCOM_set_person_ai_normal(p_person);
                }
            }
        }
    } else {
        // Is it time to change what we are doing?
        PCOM_process_state_change(p_person);
    }
}

// ============================================================
// Noise propagation system
// ============================================================

// Reset the deferred noise queue. Called at the start of each frame.
// uc_orig: init_noises (fallen/Source/pcom.cpp)
void init_noises(void)
{
    noise_count = 0;
}

// Flush all deferred noises accumulated during physics / collision this frame.
// Each entry calls PCOM_oscillate_tympanum with store_it=0 to do actual propagation.
// uc_orig: process_noises (fallen/Source/pcom.cpp)
void process_noises(void)
{
    SLONG c0;
    for (c0 = 0; c0 < noise_count; c0++) {
        PCOM_oscillate_tympanum(
            noises[c0].Type,
            (noises[c0].Person) ? (TO_THING(noises[c0].Person)) : NULL,
            noises[c0].X,
            noises[c0].Y,
            noises[c0].Z, 0);
    }
    noise_count = 0;
}

// Propagate a sound event to all persons within hearing radius.
// If store_it is non-zero, the event is queued in noises[] for deferred processing.
// DRAW_GUN uses vision (can_a_see_b) instead of radius alone.
// Radii range from 0x180 (van near-miss) to 0xa00 (gunshot).
// uc_orig: PCOM_oscillate_tympanum (fallen/Source/pcom.cpp)
void PCOM_oscillate_tympanum(
    SLONG type,
    Thing* p_person,
    SLONG sound_x,
    SLONG sound_y,
    SLONG sound_z,
    UBYTE store_it)
{
    SLONG i;

    SLONG found_upto;
    SLONG radius;

    Thing* p_found;

    if (stealth_debug && (p_person == NET_PERSON(0)))
        return;

    if (store_it) {
        struct Noise* p_noise;

        if (noise_count >= MAX_NOISE) {
            ASSERT(0);
            return;
        }

        p_noise = &noises[noise_count];
        p_noise->Type = type;
        p_noise->Person = p_person ? (THING_NUMBER(p_person)) : 0;
        p_noise->X = (SWORD)sound_x;
        p_noise->Y = (SWORD)sound_y;
        p_noise->Z = (SWORD)sound_z;
        noise_count++;
        return;
    }

    // Hearing radius per sound type.
    switch (type) {
    case PCOM_SOUND_FOOTSTEP:
        radius = 0x280;
        break;
    case PCOM_SOUND_UNUSUAL:
        radius = 0x600;
        break;
    case PCOM_SOUND_HEY:
        radius = 0x600;
        break;
    case PCOM_SOUND_ALARM:
        radius = 0x800;
        break;
    case PCOM_SOUND_FIGHT:
        radius = 0x900;
        break;
    case PCOM_SOUND_GUNSHOT:
        radius = 0xa00;
        break;
    case PCOM_SOUND_DROP:
        radius = 0x200;
        break;
    case PCOM_SOUND_DROP_MED:
        radius = 0x400;
        break;
    case PCOM_SOUND_DROP_BIG:
        radius = 0x600;
        break;
    case PCOM_SOUND_VAN:
        radius = 0x180;
        break;
    case PCOM_SOUND_BANG:
        radius = 0x700;
        break;
    case PCOM_SOUND_MINE:
        radius = 0x300;
        break;
    case PCOM_SOUND_LOOKINGATME:
        radius = 0x400;
        break;
    case PCOM_SOUND_WANKER:
        radius = 0x400;
        break;
    case PCOM_SOUND_DRAW_GUN:
        radius = 0x600;
        break;
    case PCOM_SOUND_GRENADE_HIT:
        radius = 0x300;
        break;
    case PCOM_SOUND_GRENADE_FLY:
        radius = 0x300;
        break;

    default:
        ASSERT(0);
        break;
    }

    // Find all persons within hearing distance.
    found_upto = THING_find_sphere(
        sound_x,
        sound_y,
        sound_z,
        radius,
        THING_array,
        THING_ARRAY_SIZE,
        THING_FIND_PEOPLE);

    if (type == PCOM_SOUND_DRAW_GUN && p_person) {
        // DRAW_GUN is a visual event: remove persons who cannot see the source.
        for (i = 0; i < found_upto; i++) {
            p_found = TO_THING(THING_array[i]);

            if (!can_a_see_b(p_found, p_person)) {
                THING_array[i] = THING_array[found_upto - 1];

                i -= 1;
                found_upto -= 1;
            }
        }
    }

    for (i = 0; i < found_upto; i++) {
        p_found = TO_THING(THING_array[i]);

        if (p_found == p_person) {
            // Don't disturb yourself!
            continue;
        }

        if (p_person && p_person->Class == CLASS_PERSON) {
            if (p_person->Genus.Person->Ware != p_found->Genus.Person->Ware) {
                // Ignore sounds in a warehouse you're not in; if you're in
                // a warehouse, ignore sounds outside.
                continue;
            }
        }

        if (p_found->Genus.Person->Flags & FLAG_PERSON_HELPLESS) {
            continue;
        }

        if (p_found->State == STATE_DEAD || p_found->State == STATE_DYING) {
            continue;
        }

        if (p_found->Genus.Person->PlayerID) {
            continue;
        }

        if (p_found->Genus.Person->pcom_bent & PCOM_BENT_ROBOT) {
            // Robotic people ignore sounds.
            continue;
        }

        if (p_found->Genus.Person->pcom_zone) {
            // Ignore sounds that aren't in your zone.
            if (!(PCOM_get_zone_for_position(sound_x, sound_z) & p_found->Genus.Person->pcom_zone)) {
                continue;
            }
        }

        switch (p_found->Genus.Person->pcom_move_state) {
        case PCOM_MOVE_STATE_GOTO_XZ:
        case PCOM_MOVE_STATE_GOTO_WAYPOINT:
        case PCOM_MOVE_STATE_GOTO_THING:

            if (p_found->Genus.Person->pcom_move_substate == PCOM_MOVE_SUBSTATE_ACTION) {
                // In the middle of doing a complicated moving manoeuvre.
                continue;
            }
        }

        if (type == PCOM_SOUND_VAN) {
            // Only civilians are scared of cars...
            if (p_found->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING)) {
                // ...but not civs driving.
            } else {
                if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_CIV) {
                    if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PLACE && p_found->Genus.Person->pcom_ai_state == PCOM_AI_SUBSTATE_SUPRISED) {
                        // Don't scare them again or they'll never run away.
                    } else {
                        if (ROAD_is_zebra(
                                p_found->WorldPos.X >> 16,
                                p_found->WorldPos.Z >> 16)) {
                            // Civs on zebra crossings have a sense of security.
                        } else {
                            PCOM_set_person_ai_flee_place(
                                p_found,
                                sound_x,
                                sound_z);
                            p_found->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LEGIT;
                        }
                    }
                }
            }
        } else if (type == PCOM_SOUND_GRENADE_FLY || type == PCOM_SOUND_GRENADE_HIT) {
            // Ignore this unless you can see the grenade.
            if (!can_i_see_place(
                    p_found,
                    sound_x,
                    sound_y,
                    sound_z)) {
                if (type == PCOM_SOUND_GRENADE_HIT) {
                    // Some people will investigate this sound...
                    if (p_found->Genus.Person->pcom_ai == PCOM_AI_GANG || p_found->Genus.Person->pcom_ai == PCOM_AI_COP) {
                        if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS) {
                            PCOM_set_person_ai_investigate(
                                p_found,
                                sound_x,
                                sound_z);
                        }
                    }
                }
            } else {
                if (/* This grenade is going to land near me... */ 1) {
                    PCOM_set_person_ai_flee_place(
                        p_found,
                        sound_x,
                        sound_z);
                }
            }
        } else {
            switch (p_found->Genus.Person->pcom_ai) {
            case PCOM_AI_FIGHT_TEST:
            case PCOM_AI_NONE:
                break;

            case PCOM_AI_CIV:

                // Scared of gun-shots, explosions and fighting.
                if (type == PCOM_SOUND_FIGHT && (p_found->Genus.Person->pcom_bent & PCOM_BENT_FIGHT_BACK)) {
                    // Fight-back civs aren't scared of fighting!
                } else if (type == PCOM_SOUND_GUNSHOT || type == PCOM_SOUND_BANG || type == PCOM_SOUND_MINE || type == PCOM_SOUND_LOOKINGATME || type == PCOM_SOUND_DRAW_GUN) {
                    if (p_person) {
                        PCOM_set_person_ai_flee_person(
                            p_found,
                            p_person);
                    } else {
                        PCOM_set_person_ai_flee_place(
                            p_found,
                            sound_x,
                            sound_z);
                    }
                    if (!VIOLENCE) {
                        p_found->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LEGIT;
                    }
                }

                break;

            case PCOM_AI_GUARD:

                if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK) {
                    PCOM_set_person_ai_investigate(
                        p_found,
                        sound_x,
                        sound_z);
                    SOUND_Curious(p_found);
                }

                break;

            case PCOM_AI_ASSASIN:
                break;

            case PCOM_AI_GANG:

                // Thugs ain't scared of anything! But they are interested by things...

            case PCOM_AI_COP:

                // Ignore sounds made by Darci.
                if (p_person && p_person->Genus.Person->PersonType == PERSON_DARCI) {
                    if (type == PCOM_SOUND_FOOTSTEP) {
                        // They ignore Darci's footsteps!
                        continue;
                    }
                }

                if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS) {
                    PCOM_set_person_ai_investigate(
                        p_found,
                        sound_x,
                        sound_z);
                }

                break;

            case PCOM_AI_DOORMAN:
            case PCOM_AI_BODYGUARD:
            case PCOM_AI_DRIVER:
            case PCOM_AI_BDISPOSER:
            case PCOM_AI_BIKER:
            case PCOM_AI_BOSS:
            case PCOM_AI_BULLY:
            case PCOM_AI_COP_DRIVER:
            case PCOM_AI_SUICIDE:
            case PCOM_AI_FLEE_PLAYER:
            case PCOM_AI_KILL_COLOUR:
            case PCOM_AI_BANE:
            case PCOM_AI_SHOOT_DEAD:
                break;

            case PCOM_AI_MIB:

                if (p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL || p_found->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK) {
                    if (p_person && p_person->Genus.Person->PlayerID) {
                        PCOM_alert_nearby_mib_to_attack(p_person);
                    }
                }

                break;

            default:
                ASSERT(0);
                break;
            }
        }
    }
}

// ============================================================
// Combat callbacks
// ============================================================

// Called when p_victim is grappled by p_attacker. Currently no-op (stub).
// uc_orig: PCOM_youre_being_grappled (fallen/Source/pcom.cpp)
void PCOM_youre_being_grappled(
    Thing* p_victim,
    Thing* p_attacker)
{
}

// Returns non-zero if both persons are on the same side (cop/Darci/Roper).
// uc_orig: on_same_side (fallen/Source/pcom.cpp)
static SLONG on_same_side(Thing* p_victim, Thing* p_attacker)
{
    if (p_victim->Genus.Person->PersonType == PERSON_ROPER || p_victim->Genus.Person->PersonType == PERSON_DARCI || p_victim->Genus.Person->PersonType == PERSON_COP) {
        if (p_attacker->Genus.Person->PersonType == PERSON_ROPER || p_attacker->Genus.Person->PersonType == PERSON_DARCI || p_attacker->Genus.Person->PersonType == PERSON_COP) {
            return (1);
        }
    }
    return (0);
}

// Hit callback: called when p_victim is actually struck by p_attacker.
// Dispatches to fight (retaliate -> NAVTOKILL) or flee (CIVs -> FLEE_PERSON)
// based on AI type and pcom_bent flags.
// uc_orig: PCOM_attack_happened (fallen/Source/pcom.cpp)
void PCOM_attack_happened(
    Thing* p_victim,
    Thing* p_attacker)
{
    if (p_victim->Genus.Person->PlayerID) {
        return;
    }

    if (p_attacker == NET_PERSON(0) && stealth_debug)
        return;

    if (p_victim->Genus.Person->Flags & FLAG_PERSON_HELPLESS) {
        return;
    }
    if (!people_allowed_to_hit_each_other(p_victim, p_attacker)) {
        // Don't retaliate if you can't hurt them.
        return;
    }

    if (p_victim->Genus.Person->pcom_bent & PCOM_BENT_FIGHT_BACK)
        goto fight;
    if (p_victim->Genus.Person->pcom_bent & PCOM_BENT_ROBOT)
        return;

    if (p_victim->SubState == SUB_STATE_GRAPPLE_HOLD || p_victim->SubState == SUB_STATE_GRAPPLE_ATTACK) {
        // Take hit while grappling -- release the grappled person.
        set_person_fight_idle(TO_THING(p_victim->Genus.Person->Target));
    }

    if (p_victim->SubState == SUB_STATE_GRAPPLE_HELD) {
        // No sensible options here.
        return;
    }

    switch (p_victim->Genus.Person->pcom_ai) {
    case PCOM_AI_CIV:
        goto flee;
        break;

    case PCOM_AI_COP:
    case PCOM_AI_COP_DRIVER:

        /*
        if (p_attacker->Genus.Person->pcom_ai == PCOM_AI_COP ||
                p_attacker->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER)
        {
                // A cop hitting another cop? It must have been
                // an accident.
                return;
        }
        */

        // FALL-THROUGH

    case PCOM_AI_GANG:
    case PCOM_AI_GUARD:
    case PCOM_AI_ASSASIN:
        goto fight;
        break;

    case PCOM_AI_BODYGUARD:

        // Ignore hits from the person you're meant to be protecting.
        if (THING_NUMBER(p_attacker) != EWAY_get_person(p_victim->Genus.Person->pcom_ai_other)) {
            goto fight;
        }

        break;
    }

    return;

fight:

    // If you are already fighting your attacker, no need to re-trigger.
    if (p_victim->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL || p_victim->Genus.Person->pcom_ai_state == PCOM_AI_STATE_KILLING) {
        if (p_victim->Genus.Person->pcom_ai_arg == THING_NUMBER(p_attacker)) {
            return;
        }
    }

    // Don't retaliate against someone in your own gang.
    if ((p_victim->Genus.Person->pcom_bent & PCOM_BENT_GANG) && (p_attacker->Genus.Person->pcom_bent & PCOM_BENT_GANG) && (p_victim->Genus.Person->pcom_colour == p_attacker->Genus.Person->pcom_colour)) {
        return;
    }

    PCOM_set_person_ai_kill_person(p_victim, p_attacker);

    return;

flee:
    PCOM_set_person_ai_flee_person(p_victim, p_attacker);
    if (!VIOLENCE)
        p_victim->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LEGIT;

    return;
}

// Near-miss callback: same fight/flee dispatch as PCOM_attack_happened
// but called when an attack narrowly misses p_victim.
// uc_orig: PCOM_attack_happened_but_missed (fallen/Source/pcom.cpp)
void PCOM_attack_happened_but_missed(Thing* p_victim, Thing* p_attacker)
{
    if (p_victim->Genus.Person->PlayerID) {
        return;
    }

    if (p_attacker == NET_PERSON(0) && stealth_debug)
        return;

    if (p_victim->Genus.Person->Flags & FLAG_PERSON_HELPLESS) {
        return;
    }

    if (on_same_side(p_victim, p_attacker))
        return;

    if (p_victim->Genus.Person->pcom_bent & PCOM_BENT_ROBOT)
        return;

    if (!people_allowed_to_hit_each_other(p_victim, p_attacker))
        return;

    if (p_victim->Genus.Person->pcom_bent & PCOM_BENT_FIGHT_BACK)
        goto fight;

    switch (p_victim->Genus.Person->pcom_ai) {
    case PCOM_AI_CIV:
        goto flee;
        break;

    case PCOM_AI_COP:
    case PCOM_AI_COP_DRIVER:

        // FALL-THROUGH

    case PCOM_AI_GANG:
    case PCOM_AI_GUARD:
    case PCOM_AI_ASSASIN:
        goto fight;
        break;

    case PCOM_AI_BODYGUARD:

        // Ignore hits from the person you're meant to be protecting.
        if (THING_NUMBER(p_attacker) != EWAY_get_person(p_victim->Genus.Person->pcom_ai_other)) {
            goto fight;
        }

        break;
    }

    return;

fight:

    // If you are already fighting your attacker, no need to re-trigger.
    if (p_victim->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL || p_victim->Genus.Person->pcom_ai_state == PCOM_AI_STATE_KILLING) {
        if (p_victim->Genus.Person->pcom_ai_arg == THING_NUMBER(p_attacker)) {
            return;
        }
    }

    // Don't retaliate against someone in your own gang.
    if ((p_victim->Genus.Person->pcom_bent & PCOM_BENT_GANG) && (p_attacker->Genus.Person->pcom_bent & PCOM_BENT_GANG) && (p_victim->Genus.Person->pcom_colour == p_attacker->Genus.Person->pcom_colour)) {
        return;
    }

    PCOM_set_person_ai_kill_person(p_victim, p_attacker);

    return;

flee:
    PCOM_set_person_ai_flee_person(p_victim, p_attacker);
    if (!VIOLENCE)
        p_victim->Genus.Person->pcom_ai_substate = PCOM_AI_SUBSTATE_LEGIT;
    return;
}

// Returns UC_TRUE if the person's move state should keep them moving after initiating a jump.
// For difficult pull-up jumps (MAV_ACTION_JUMPPULL2) always moves. For near-destination
// cases (dist < 0x100), stops to avoid overshooting.
// uc_orig: PCOM_jumping_navigating_person_continue_moving (fallen/Source/pcom.cpp)
SLONG PCOM_jumping_navigating_person_continue_moving(Thing* p_person)
{
    if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_GOTO_XZ || p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_GOTO_WAYPOINT || p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_GOTO_THING) {
        // Unless you're doing a difficult jump that you'll really have to commit to...
        if (p_person->Genus.Person->pcom_move_ma.action != MAV_ACTION_JUMPPULL2) {
            SLONG goal_x;
            SLONG goal_z;

            PCOM_get_mav_action_pos(
                p_person,
                &goal_x,
                &goal_z);

            SLONG dx = goal_x - (p_person->WorldPos.X >> 8);
            SLONG dz = goal_z - (p_person->WorldPos.Z >> 8);

            SLONG dist = abs(dx) + abs(dz);

            if (dist < 0x100) {
                // Stop moving or you'll overshoot.
                return UC_FALSE;
            }
        }
    }

    return UC_TRUE;
}

// Knockdown callback: transitions AI to KNOCKEDOUT state to wait for recovery.
// Skips transition if already in active combat (KILLING/NAVTOKILL/CIRCLE) so
// fighters get back up fighting rather than wandering.
// uc_orig: PCOM_knockdown_happened (fallen/Source/pcom.cpp)
void PCOM_knockdown_happened(Thing* p_person)
{
    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_KILLING || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL || p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_CIRCLE || p_person->State == STATE_CIRCLING) {
        // If you do this to people who are attacking you, then when they get up
        // they just wander off rather than getting back into the fight.
    } else {
        PCOM_set_person_ai_knocked_out(p_person);
    }
}

// ============================================================
// Debug output
// ============================================================

// Build a human-readable debug string of all AI fields for on-screen overlay (PC only).
// Shows pcom_ai, pcom_ai_state, pcom_move, pcom_move_state, pcom_bent flags.
// uc_orig: PCOM_person_state_debug (fallen/Source/pcom.cpp)
CBYTE* PCOM_person_state_debug(Thing* p_person)
{
    SLONG i;
    CBYTE bent[256];

    if (p_person->Genus.Person->PlayerID) {
        sprintf(
            PCOM_debug_string,
            "Player %d\n"
            "Pos (0x%x,0x%x) height 0x%x\n"
            "Warehouse %d\n",
            p_person->Genus.Person->PlayerID,
            p_person->WorldPos.X >> 8,
            p_person->WorldPos.Z >> 8,
            p_person->WorldPos.Y >> 8,
            p_person->Genus.Person->Ware);

        return PCOM_debug_string;
    }

    bent[0] = '\000';

    for (i = 0; i < PCOM_BENT_NUMBER; i++) {
        if (p_person->Genus.Person->pcom_bent & (1 << i)) {
            strcat(bent, PCOM_bent_name[i]);
        }
    }

    if (p_person->Genus.Person->Flags & FLAG_PERSON_HELPLESS) {
        strcat(bent, "Helpless ");
    }

    sprintf(
        PCOM_debug_string,
        "%s%s%s,ptype %d,mesh %d personid %d \nState  %s : %s\nMove   %s : %s\nPerson 0x%p(%d) action %d anim %d Y %d st %d subs %d Agr %d",
        bent,
        PCOM_ai_name[p_person->Genus.Person->pcom_ai],
        (p_person->Genus.Person->Ware) ? " ware" : "",
        p_person->Genus.Person->PersonType,
        p_person->Draw.Tweened->MeshID,
        p_person->Draw.Tweened->PersonID,
        PCOM_ai_state_name[p_person->Genus.Person->pcom_ai_state],
        PCOM_ai_substate_name[p_person->Genus.Person->pcom_ai_substate],
        PCOM_move_name[p_person->Genus.Person->pcom_move],
        PCOM_move_state_name[p_person->Genus.Person->pcom_move_state],
        p_person, THING_NUMBER(p_person), p_person->Genus.Person->Action, p_person->Draw.Tweened->CurrentAnim,
        p_person->WorldPos.Y >> 8,
        p_person->State,
        p_person->SubState,
        p_person->Genus.Person->Agression);

    return PCOM_debug_string;
}

// ============================================================
// Cop interaction callbacks
// ============================================================

// Called when a cop aims a gun at p_person.
// Guilty persons retaliate (NAVTOKILL); innocent idle persons put hands up.
// Cops, Darci, and Roper are exempt from hands-up.
// uc_orig: PCOM_cop_aiming_at_you (fallen/Source/pcom.cpp)
SLONG PCOM_cop_aiming_at_you(Thing* p_person, Thing* p_cop)
{
    if (p_cop == NET_PERSON(0) && stealth_debug) {
        return 0;
    }

    if (p_person->Genus.Person->pcom_bent & PCOM_BENT_ROBOT) {
        return 0;
    }

    if (p_person->Genus.Person->pcom_ai == PCOM_AI_BODYGUARD) {
        // Bodyguards ignore being aimed at by the person they are guarding.
        if (THING_NUMBER(p_cop) == EWAY_get_person(p_person->Genus.Person->pcom_ai_other)) {
            return 0;
        }
    }

    if (is_person_guilty(p_person)) {

        if (p_person->State == STATE_DYING || p_person->State == STATE_DEAD)
            return (0);

        // Don't put hands up because you are guilty and need to do something else.
        if (p_person->Genus.Person->pcom_ai_state != PCOM_AI_STATE_NAVTOKILL && p_person->Genus.Person->pcom_ai_state != PCOM_AI_STATE_FLEE_PERSON) {
            PCOM_set_person_ai_navtokill(p_person, p_cop);
            return (1);
        } else {
            return (0);
        }
        /*
                if (PCOM_person_has_any_sort_of_gun(p_person))
                {
                        // you have a gun so kill the cop
                        PCOM_set_person_ai_navtokill_shoot(p_person, p_cop);
                        return(1);
                }
                else
                if (!(p_person->Genus.Person->pcom_bent & PCOM_BENT_FIGHT_BACK) && p_person->Genus.Person->pcom_ai != PCOM_AI_ASSASIN)
                {
                        // no gun so run away
                        PCOM_set_person_ai_flee_person(p_person, p_cop);
                        return(1);
                }
        */
    }
    if (p_person->Genus.Person->PersonType == PERSON_ROPER || p_person->Genus.Person->PersonType == PERSON_DARCI || p_person->Genus.Person->PersonType == PERSON_COP) {
        // Cops, Darci, and Roper don't put hands up.
        return (0);
    }

    /*
            if(p_person->Genus.Person->PersonType==PERSON_ROPER||p_person->Genus.Person->PersonType==PERSON_DARCI||p_person->Genus.Person->PersonType==PERSON_COP||p_person->Genus.Person->PersonType==PERSON_THUG_GREY || p_person->Genus.Person->PersonType==PERSON_THUG_RASTA|| p_person->Genus.Person->PersonType==PERSON_THUG_RED)
            {
                    // Thugs don't hands up
                    return(0);
            }
    */

    if (PCOM_person_doing_nothing_important(p_person)) {
        PCOM_set_person_ai_hands_up(p_person, p_cop);
        return (1);
    }
    return (0);
}

// ============================================================
// Conversation helpers
// ============================================================

// Start a conversation between two persons. p_person_a talks, p_person_b listens (if requested).
// Both are marked non-interruptible during the conversation.
// uc_orig: PCOM_make_people_talk_to_eachother (fallen/Source/pcom.cpp)
void PCOM_make_people_talk_to_eachother(
    Thing* p_person_a,
    Thing* p_person_b,
    UBYTE is_a_asking_a_question,
    UBYTE stay_looking_at_eachother,
    UBYTE make_the_person_talked_at_listen)
{
    SLONG substate;

    if (is_a_asking_a_question) {
        substate = PCOM_AI_SUBSTATE_TALK_ASK;
    } else {
        substate = PCOM_AI_SUBSTATE_TALK_TELL;
    }

    PCOM_set_person_ai_talk_to(p_person_a, p_person_b, substate, stay_looking_at_eachother);

    p_person_a->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C;

    if (make_the_person_talked_at_listen) {
        PCOM_set_person_ai_talk_to(p_person_b, p_person_a, PCOM_AI_SUBSTATE_TALK_LISTEN, stay_looking_at_eachother);
        p_person_b->Genus.Person->Flags |= FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C;
    }
}

// End a conversation between two persons. Clears no-return flag and resets to normal AI.
// uc_orig: PCOM_stop_people_talking_to_eachother (fallen/Source/pcom.cpp)
void PCOM_stop_people_talking_to_eachother(
    Thing* p_person_a,
    Thing* p_person_b)
{
    p_person_a->Genus.Person->Flags &= ~FLAG_PERSON_NO_RETURN_TO_NORMAL;
    p_person_b->Genus.Person->Flags &= ~FLAG_PERSON_NO_RETURN_TO_NORMAL;

    if (p_person_a->Genus.Person->pcom_ai_state == PCOM_AI_STATE_TALK) {
        PCOM_set_person_ai_normal(p_person_a);
    }
    if (p_person_b->Genus.Person->pcom_ai_state == PCOM_AI_STATE_TALK) {
        PCOM_set_person_ai_normal(p_person_b);
    }
}

// ============================================================
// Relationship queries
// ============================================================

// Returns UC_TRUE if person A is currently hostile toward person B.
// Considers same-gang colour (friends), player exemptions (bodyguards, followers,
// other players), and active KILLING/NAVTOKILL/ARREST/TAUNT target.
// uc_orig: PCOM_person_a_hates_b (fallen/Source/pcom.cpp)
SLONG PCOM_person_a_hates_b(Thing* p_person_a, Thing* p_person_b)
{
    ASSERT(p_person_a->Class == CLASS_PERSON);
    ASSERT(p_person_b->Class == CLASS_PERSON);

    if (p_person_a == p_person_b) {
        // Nobody hates themselves.
        return UC_FALSE;
    }

    // People in the same gang like each other.
    if (p_person_a->Genus.Person->pcom_bent & p_person_b->Genus.Person->pcom_bent & PCOM_BENT_GANG) {
        if (p_person_a->Genus.Person->pcom_colour == p_person_b->Genus.Person->pcom_colour) {
            return UC_FALSE;
        }
    }

    if (p_person_a->Genus.Person->PlayerID) {
        // Players like their bodyguards.
        if (p_person_b->Genus.Person->pcom_ai == PCOM_AI_BODYGUARD && EWAY_get_person(p_person_b->Genus.Person->pcom_ai_other) == THING_NUMBER(p_person_a)) {
            return UC_FALSE;
        }

        // Players like other players.
        if (p_person_b->Genus.Person->PersonType == PERSON_DARCI || p_person_b->Genus.Person->PersonType == PERSON_ROPER) {
            return UC_FALSE;
        }

        // Players like people who are following them.
        if (p_person_b->Genus.Person->pcom_move == PCOM_MOVE_FOLLOW && EWAY_get_person(p_person_b->Genus.Person->pcom_move_follow) == THING_NUMBER(p_person_a)) {
            return UC_FALSE;
        }

        // Players hate everybody else...
        return UC_TRUE;
    }

    if (p_person_a->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL || p_person_a->Genus.Person->pcom_ai_state == PCOM_AI_STATE_KILLING || p_person_a->Genus.Person->pcom_ai_state == PCOM_AI_STATE_TAUNT || p_person_a->Genus.Person->pcom_ai_state == PCOM_AI_STATE_ARREST) {
        // These people don't like the person they are dealing with.
        if (p_person_a->Genus.Person->pcom_ai_arg == THING_NUMBER(p_person_b)) {
            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// Returns the thing index of the person this NPC wants to kill, or NULL.
// uc_orig: PCOM_person_wants_to_kill (fallen/Source/pcom.cpp)
THING_INDEX PCOM_person_wants_to_kill(Thing* p_person)
{
    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NAVTOKILL || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_KILLING) {
        return p_person->Genus.Person->pcom_ai_arg;
    }

    return NULL;
}

// ============================================================
// Vehicle driving AI
// ============================================================

// Decelerate vehicle to a stop and steer to the desired parking angle.
// The target angle is retrieved via EWAY_get_angle(pcom_move_arg).
// uc_orig: ParkCar (fallen/Source/pcom.cpp)
void ParkCar(Thing* p_person)
{
    SLONG wangle;
    SLONG dangle;

    // Must be in a car to do this.
    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

    Thing* p_vehicle = TO_THING(p_person->Genus.Person->InCar);

    // What direction do we want the car to face?
    p_vehicle->Genus.Vehicle->IsAnalog = 0;
    p_vehicle->Genus.Vehicle->Steering = 0;

    if (p_person->Genus.Person->pcom_move_arg) {
        wangle = EWAY_get_angle(p_person->Genus.Person->pcom_move_arg);

        dangle = wangle - p_vehicle->Genus.Vehicle->Angle;

        if (dangle < -1024) {
            dangle += 2048;
        }
        if (dangle > +1024) {
            dangle -= 2048;
        }

        if (dangle < -8) {
            p_vehicle->Genus.Vehicle->Steering = -1;
        }
        if (dangle > +8) {
            p_vehicle->Genus.Vehicle->Steering = +1;
        }
    }

    // Decelerate.
    if (WITHIN(p_vehicle->Velocity, -10, +10)) {
        p_vehicle->Genus.Vehicle->DControl = 0;
        p_vehicle->Velocity = 0;
    } else if (p_vehicle->Velocity > 0) {
        p_vehicle->Genus.Vehicle->DControl = VEH_DECEL;
    } else {
        p_vehicle->Genus.Vehicle->DControl = VEH_ACCEL;
    }
}

// Core car driving AI. Follows the road network node by node; steers toward the next
// waypoint; applies acceleration/braking. Calls PCOM_find_runover_thing to react to
// obstacles. Handles lane keeping via ROAD_signed_dist. Three sub-states:
//   DRIVE_DOWN / PARK_CAR_ON_ROAD: road-following with lane correction
//   DRIVETO: free-drive to a specific coordinate
// uc_orig: DriveCar (fallen/Source/pcom.cpp)
void DriveCar(Thing* p_person)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;
    SLONG dest_x;
    SLONG dest_z;
    SLONG dangle;
    SLONG dlane;
    SLONG dspeed;
    SLONG wspeed;
    SLONG what;

    ASSERT(p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING);

    Thing* p_vehicle = TO_THING(p_person->Genus.Person->InCar);
    ASSERT(p_vehicle);

    if (p_person->Genus.Person->pcom_move_state != PCOM_MOVE_STATE_PARK_CAR_ON_ROAD) {
        // If the vehicle collided last time it moved, do something about it.
        if (p_vehicle->Flags & FLAGS_COLLIDED) {
            switch (p_person->Genus.Person->pcom_move_substate) {
            case PCOM_MOVE_SUBSTATE_GOTO:
                p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_3PTURN;
                p_person->Genus.Person->pcom_move_counter = 0;
                break;

            case PCOM_MOVE_SUBSTATE_3PTURN:
                p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
                p_person->Genus.Person->pcom_move_counter = 0;
                break;

            default:
                ASSERT(0);
                break;
            }
        }
    }

    // Where are we going?
    PCOM_get_person_dest(
        p_person,
        &dest_x,
        &dest_z);

    // If we are driving down a road, apply lane correction.
    if ((p_person->Genus.Person->pcom_move_state != PCOM_MOVE_STATE_DRIVETO) && (p_person->Genus.Person->pcom_move_substate != PCOM_MOVE_SUBSTATE_3PTURN)) {
        // How far are we from the middle of the road?
        dist = ROAD_signed_dist(
            p_person->Genus.Person->pcom_move_arg >> 8,
            p_person->Genus.Person->pcom_move_arg & 0xff,
            p_vehicle->WorldPos.X >> 8,
            p_vehicle->WorldPos.Z >> 8);

        if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_PARK_CAR_ON_ROAD) {
            dlane = 0x300 - dist;
        } else {
            dlane = 0x180 - dist;
        }

        if (abs(dlane) > 0x80 || p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_PARK_CAR_ON_ROAD) {
            // We are quite far from our lane. Aim for the middle of our lane.
            SLONG rx1;
            SLONG rz1;

            SLONG rx2;
            SLONG rz2;

            SLONG rdest_x;
            SLONG rdest_z;

            ROAD_node_pos(
                p_person->Genus.Person->pcom_move_arg >> 8,
                &rx1,
                &rz1);

            ROAD_node_pos(
                p_person->Genus.Person->pcom_move_arg & 0xff,
                &rx2,
                &rz2);

            nearest_point_on_line(
                rx1, rz1,
                rx2, rz2,
                p_vehicle->WorldPos.X >> 8,
                p_vehicle->WorldPos.Z >> 8,
                &rdest_x,
                &rdest_z);

            SLONG drx = SIGN(rx2 - rx1) << 8;
            SLONG drz = SIGN(rz2 - rz1) << 8;

            rdest_x += drx;
            rdest_z += drz;

            rdest_x += drx;
            rdest_z += drz;

            rdest_x -= drz;
            rdest_z += drx;

            if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_PARK_CAR_ON_ROAD) {
                rdest_x -= drz;
                rdest_z += drx;

                rdest_x -= drz;
                rdest_z += drx;
            }

            dest_x = rdest_x;
            dest_z = rdest_z;
        }
    }

    // What direction do we want the car to face?
    dx = dest_x - (p_vehicle->WorldPos.X >> 8);
    dz = dest_z - (p_vehicle->WorldPos.Z >> 8);

    dangle = (p_vehicle->Genus.Vehicle->Angle - (calc_angle(dx, dz) + 1024)) & 2047;

    if (dangle >= 1024)
        dangle -= 2048;

    // What speed do we want to be going at?
    if (p_person->Genus.Person->pcom_move_state == PCOM_MOVE_STATE_PARK_CAR_ON_ROAD) {
        if (abs(dangle) < 100 && abs(dlane) < 0x80) {
            wspeed = 0;
        } else {
            wspeed = VEH_SPEED_LIMIT >> 1;
        }
    } else {
        switch (p_person->Genus.Person->pcom_move_substate) {
        case PCOM_MOVE_SUBSTATE_GOTO:

            if (abs(dangle) > 100) {
                wspeed = VEH_SPEED_LIMIT - ((abs(dangle) - 100) >> 1);

                if (wspeed < 250) {
                    wspeed = 250;
                }
            } else {
                wspeed = VEH_SPEED_LIMIT;
            }

            if (p_person->Genus.Person->pcom_bent & PCOM_BENT_DILIGENT) {
                wspeed += wspeed;
            }

            break;

        case PCOM_MOVE_SUBSTATE_3PTURN:

            // Always reverse for a little while at least.
            if (p_person->Genus.Person->pcom_move_counter < PCOM_get_duration(15)) {
            } else {
                if (abs(dangle) < 500) {
                    // We can start driving normally.
                    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
                }
            }

            wspeed = -VEH_REVERSE_SPEED;
            dangle = -dangle;

            break;

        default:
            ASSERT(0);
            break;
        }
    }

    // Are we going to crash into anyone?
    what = PCOM_find_runover_thing(p_person, dangle);

    if (what & PCOM_RUNOVER_STOP) {
        if (wspeed > 0) {
            wspeed = 0;
        }
    }
    //	if (what & PCOM_RUNOVER_BEEP_HORN)  {MFX_play_thing(THING_NUMBER(p_person), S_BEEP,         MFX_REPLACE, p_person);}
    //	if (what & PCOM_RUNOVER_SHOUT_OUT)  {MFX_play_thing(THING_NUMBER(p_person), S_GETOUTTHEWAY, MFX_REPLACE, p_person);}
    if (what & (PCOM_RUNOVER_BEEP_HORN | PCOM_RUNOVER_SHOUT_OUT)) {
        MFX_play_thing(THING_NUMBER(p_person), SOUND_Range(S_CAR_HORN_START, S_CAR_HORN_END), MFX_REPLACE, p_person);
    }

    if (what & PCOM_RUNOVER_SLOW_DOWN) {
        if (wspeed > 0) {
            wspeed >>= 1;
        }
    }
    if (what & PCOM_RUNOVER_REVERSE) {
        wspeed = -(wspeed >> 2);
        dangle = 0;
    }

    if (what & PCOM_RUNOVER_TURN_LEFT) {
        p_person->Genus.Person->pcom_move_flag |= PCOM_MOVE_FLAG_AVOID_LEFT;
        p_person->Genus.Person->pcom_move_counter = 0;
    }
    if (what & PCOM_RUNOVER_TURN_RIGHT) {
        p_person->Genus.Person->pcom_move_flag |= PCOM_MOVE_FLAG_AVOID_RIGHT;
        p_person->Genus.Person->pcom_move_counter = 0;
    }

    if (what & PCOM_RUNOVER_RUNAWAY) {
        // The movement AI is updating high-level AI through complexity.
        PCOM_set_person_ai_flee_person(p_person, PCOM_runover_scary_person);
        return;
    }

    // Turn towards our destination.
    p_vehicle->Genus.Vehicle->IsAnalog = 0;
    p_vehicle->Genus.Vehicle->Steering = 0;

    if (dangle > +8) {
        p_vehicle->Genus.Vehicle->Steering = +1;
    }
    if (dangle < -8) {
        p_vehicle->Genus.Vehicle->Steering = -1;
    }

    // Accelerate / decelerate.
    p_vehicle->Genus.Vehicle->DControl = 0;

    dspeed = p_vehicle->Velocity - wspeed;

    if (dspeed < -10) {
        p_vehicle->Genus.Vehicle->DControl = VEH_ACCEL;
    }
    if (dspeed > +10) {
        p_vehicle->Genus.Vehicle->DControl = VEH_DECEL;
    }

    // shift up if PCOM_BENT_DILIGENT person (disabled in original, kept 1:1)
    if (p_person->Genus.Person->pcom_bent & PCOM_BENT_DILIGENT) {
        //		p_vehicle->Genus.Vehicle->DControl |= VEH_FASTER;
    }

    // Lateral avoidance: steer around obstacles for a brief window.
    if (p_person->Genus.Person->pcom_move_flag & PCOM_MOVE_FLAG_AVOID_LEFT) {
        p_vehicle->Genus.Vehicle->Steering = -1;
    }
    if (p_person->Genus.Person->pcom_move_flag & PCOM_MOVE_FLAG_AVOID_RIGHT) {
        p_vehicle->Genus.Vehicle->Steering = +1;
    }

    if (p_person->Genus.Person->pcom_move_flag & (PCOM_MOVE_FLAG_AVOID_LEFT | PCOM_MOVE_FLAG_AVOID_RIGHT)) {
        SLONG avoid_time;

        if (p_vehicle->Velocity > 700) {
            avoid_time = PCOM_get_duration(1);
        } else {
            avoid_time = PCOM_get_duration(2);
        }

        if (p_person->Genus.Person->pcom_move_counter > avoid_time) {
            p_person->Genus.Person->pcom_move_flag &= ~(PCOM_MOVE_FLAG_AVOID_LEFT | PCOM_MOVE_FLAG_AVOID_RIGHT);
        }
    }
    /*
            AENG_world_line_infinite(
                    p_vehicle->WorldPos.X >> 8,
                    p_vehicle->WorldPos.Y >> 8,
                    p_vehicle->WorldPos.Z >> 8,
                    32,
                    0xffffff,
                    dest_x,
                    0,
                    dest_z,
                    0,
                    0x000000,
                    UC_TRUE);
    */
    p_person->Genus.Person->pcom_move_counter += PCOM_TICKS_PER_TURN * TICK_RATIO >> TICK_SHIFT;
}

// Returns the jump speed an NPC should use when initiating a jump during navigation.
// NPCs navigating toward a goal use speed 24 (slower, controlled); all others use 40.
// uc_orig: PCOM_if_i_wanted_to_jump_how_fast_should_i_do_it (fallen/Source/pcom.cpp)
SLONG PCOM_if_i_wanted_to_jump_how_fast_should_i_do_it(Thing* p_person)
{
    if (!p_person->Genus.Person->PlayerID) {
        switch (p_person->Genus.Person->pcom_move_state) {
        case PCOM_MOVE_STATE_GOTO_XZ:
        case PCOM_MOVE_STATE_GOTO_WAYPOINT:
        case PCOM_MOVE_STATE_GOTO_THING:

            switch (p_person->Genus.Person->pcom_move_ma.action) {
            case MAV_ACTION_JUMP:
            case MAV_ACTION_PULLUP:
            case MAV_ACTION_CLIMB_OVER:
                return 24;
            }
        }
    }

    return 40;
}

// Make the driver flee from p_scary. If not driving, use FLEE_PERSON directly;
// if driving, transition to LEAVECAR state first then flee.
// uc_orig: PCOM_make_driver_run_away (fallen/Source/pcom.cpp)
void PCOM_make_driver_run_away(Thing* p_driver, Thing* p_scary)
{
    if (p_driver->Genus.Person->pcom_ai_state == PCOM_AI_STATE_LEAVECAR || p_driver->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PERSON) {
        return;
    }

    if (!(p_driver->Genus.Person->Flags & FLAG_PERSON_DRIVING)) {
        PCOM_set_person_ai_flee_person(p_driver, p_scary);
    } else {
        PCOM_set_person_ai_leavecar(
            p_driver,
            PCOM_EXCAR_FLEE_PERSON,
            0,
            THING_NUMBER(p_scary));
    }
}
