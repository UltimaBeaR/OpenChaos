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
#include "missions/memory_globals.h"    // Temporary: dfacets, DFacet, next_dfacet
#include "fallen/Headers/collide.h"     // Temporary: there_is_a_los_mav, can_a_see_b
#include "fallen/Headers/road.h"        // Temporary: ROAD_get_dest
#include "fallen/Headers/wand.h"        // Temporary: PAP_*, PAP_FLAG_*
#include "fallen/Headers/ware.h"        // Temporary: WARE_mav_enter etc
#include "fallen/Headers/ob.h"          // Temporary: person_has_special, THING_find_sphere
#include "fallen/Headers/statedef.h"    // Temporary: state/substate constants
#include "fallen/Headers/animate.h"     // Temporary: SUB_OBJECT_PELVIS, ANIM_SIT_DOWN, ANIM_SIT_IDLE
#include "fallen/Headers/Person.h"      // Temporary: set_person_idle, set_person_goto_xz, set_person_circle, etc.
#include "engine/audio/mfx.h"           // Temporary: MFX_play_thing
#include "engine/audio/sound.h"         // Temporary: SOUND_Range
#include "assets/sound_id.h"            // Temporary: S_COP_ARREST_START, S_COP_ARREST_END, sound IDs
#include "fallen/Headers/spark.h"       // Temporary: SPARK_create, SPARK_Pinfo
#include "fallen/Headers/balloon.h"     // Temporary: BALLOON_create
#include "fallen/Headers/cnet.h"        // Temporary: NET_PERSON
#include "fallen/Headers/overlay.h"     // Temporary: track_enemy
#include "fallen/Headers/Vehicle.h"     // Temporary: reinit_vehicle, Vehicle struct, ROAD_find
#include "fallen/DDEngine/Headers/aeng.h" // Temporary: AENG_world_line (debug view line)
#include "actors/core/interact.h"         // Temporary: calc_sub_objects_position

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
        return FALSE;
    }

    if (EWAY_stop_player_moving()) {
        // In a cutscene.
        return FALSE;
    }

    if (p_person->Genus.Person->PersonType == PERSON_COP) {
        // Cops always attack the thugs!
        return TRUE;
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
            return FALSE;
        }
    }

    return TRUE;
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
        return FALSE;
    }

    {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        if (p_special->Genus.Special->SpecialType == SPECIAL_SHOTGUN || p_special->Genus.Special->SpecialType == SPECIAL_AK47 || p_special->Genus.Special->SpecialType == SPECIAL_GRENADE) {
            return (p_special->Genus.Special->SpecialType);
        }
    }

    return FALSE;
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
// Returns FALSE if target is climbing/jumping/dangling, or too far, or in different warehouse zone.
// uc_orig: PCOM_should_i_try_to_los_mav_to_person (fallen/Source/pcom.cpp)
SLONG PCOM_should_i_try_to_los_mav_to_person(Thing* p_person, Thing* p_target)
{
    if (p_target->Class == CLASS_PERSON) {
        if (p_target->State == STATE_CLIMB_LADDER || p_target->State == STATE_CLIMBING || p_target->State == STATE_DANGLING || p_target->State == STATE_JUMPING) {
            return FALSE;
        }

        SLONG dx = abs(p_target->WorldPos.X - p_person->WorldPos.X);
        SLONG dy = abs(p_target->WorldPos.Y - p_person->WorldPos.Y);
        SLONG dz = abs(p_target->WorldPos.Z - p_person->WorldPos.Z);

        if (dx < 0x30000 && dz < 0x30000 && dy < 0x10000) {
            return TRUE;
        }
    }

    return FALSE;
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

// Teleports or verifies proximity to a seat prim. Returns FALSE if too far and dont_teleport set.
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
            return FALSE;
        }
    }

    newpos.Y = prim_y << 8;

    move_thing_on_map(p_person, &newpos);

    return TRUE;
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

// Returns TRUE if the person is carrying any gun (including MIBs built-in AK47).
// uc_orig: PCOM_person_has_any_sort_of_gun (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_any_sort_of_gun(Thing* p_person)
{
    if (p_person->Flags & FLAGS_HAS_GUN) {
        return TRUE;
    }

    if (person_has_special(p_person, SPECIAL_SHOTGUN) || person_has_special(p_person, SPECIAL_AK47) || person_has_special(p_person, SPECIAL_GRENADE)) {
        return TRUE;
    }

    if (p_person->Genus.Person->PersonType == PERSON_MIB1 || p_person->Genus.Person->PersonType == PERSON_MIB2 || p_person->Genus.Person->PersonType == PERSON_MIB3) {
        return TRUE;
    }

    return FALSE;
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
            return TRUE;
        }
    }

    if (p_person->Genus.Person->PersonType == PERSON_MIB1 || p_person->Genus.Person->PersonType == PERSON_MIB2 || p_person->Genus.Person->PersonType == PERSON_MIB3) {
        return TRUE;
    }

    if ((p_special = person_has_special(p_person, SPECIAL_SHOTGUN)) || (p_special = person_has_special(p_person, SPECIAL_AK47)) || (p_special = person_has_special(p_person, SPECIAL_GRENADE))) {
        if (p_special->Genus.Special->ammo) {
            return TRUE;
        }
    }

    return FALSE;
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
                return TRUE;
            }
        }
    }

    return FALSE;
}

// Returns TRUE if this NPC can be interrupted by new AI stimuli.
// Returns FALSE while dying, dead, being carried, climbing, jumping, in a vehicle action, or mid-arrest crouch.
// uc_orig: PCOM_person_doing_nothing_important (fallen/Source/pcom.cpp)
SLONG PCOM_person_doing_nothing_important(Thing* p_person)
{
    if (p_person->State == STATE_DYING || p_person->State == STATE_DEAD || p_person->State == STATE_CARRY)
        return (FALSE);

    if (p_person->State == STATE_MOVEING && (p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER || p_person->SubState == SUB_STATE_SIMPLE_ANIM)) {
        if (p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_DOWN || p_person->Draw.Tweened->CurrentAnim == ANIM_SIT_IDLE) {
            // If they're sitting down, they can be interrupted.
            return TRUE;
        }
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL)
        if (p_person->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER && (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER)
            return (TRUE);

    if (p_person->Genus.Person->Flags & (FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C))
        return (FALSE);

    switch (p_person->Genus.Person->pcom_move_state) {
    case PCOM_MOVE_STATE_GOTO_XZ:
    case PCOM_MOVE_STATE_GOTO_WAYPOINT:
    case PCOM_MOVE_STATE_GOTO_THING:

        if (p_person->Genus.Person->pcom_move_substate == PCOM_MOVE_SUBSTATE_ACTION) {
            // In the middle of a complicated move.
            return FALSE;
        }
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
        if (p_person->Genus.Person->pcom_ai == PCOM_AI_COP_DRIVER && (p_person->Genus.Person->Flags & FLAG_PERSON_DRIVING) && p_person->Genus.Person->pcom_move == PCOM_MOVE_WANDER)
            return TRUE;

        if (p_person->State == STATE_IDLE || p_person->State == STATE_GOTOING || p_person->State == STATE_NORMAL) {
            return TRUE;
        }

        if (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_AIM_GUN) {
            if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_NORMAL) {
                return TRUE;
            }
        }

        if (p_person->State == STATE_MOVEING) {
            if (p_person->SubState == SUB_STATE_SIMPLE_ANIM || p_person->SubState == SUB_STATE_SIMPLE_ANIM_OVER) {
                return TRUE;
            }
        }
    }

    if (p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_WARM_HANDS || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_HOMESICK || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_INVESTIGATING || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FOLLOWING) {
        return TRUE;
    }

    return FALSE;
}

// Returns TRUE if the person has any kind of gun ready to shoot.
// uc_orig: PCOM_person_has_gun_in_hand (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_gun_in_hand(Thing* p_person)
{
    if (p_person->Genus.Person->PersonType == PERSON_MIB1 || p_person->Genus.Person->PersonType == PERSON_MIB2 || p_person->Genus.Person->PersonType == PERSON_MIB3) {
        // MIB are ninja shooting machines with built-in AK47s!
        return TRUE;
    }

    if (p_person->State == STATE_GUN && p_person->SubState == SUB_STATE_DRAW_GUN) {
        // Still drawing a weapon.
        return FALSE;
    }

    if (p_person->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        return TRUE;
    }

    if (p_person->Genus.Person->SpecialUse) {
        Thing* p_special = TO_THING(p_person->Genus.Person->SpecialUse);

        if (p_special->Genus.Special->SpecialType == SPECIAL_SHOTGUN || p_special->Genus.Special->SpecialType == SPECIAL_AK47 || p_special->Genus.Special->SpecialType == SPECIAL_GRENADE) {
            return TRUE;
        }
    }

    return FALSE;
}

// Returns TRUE if the shooter has a gun aimed at p_person.
// uc_orig: PCOM_target_could_shoot_me (fallen/Source/pcom.cpp)
SLONG PCOM_target_could_shoot_me(Thing* p_person, Thing* p_shooter)
{
#define PCOM_SHOOTME_DANGLE 256

    if (can_a_see_b(p_shooter, p_person))
        if (PCOM_person_has_gun_in_hand(p_shooter)) {
            SLONG dangle = get_dangle(p_shooter, p_person);

            if (dangle < PCOM_SHOOTME_DANGLE || dangle > 2048 - PCOM_SHOOTME_DANGLE) {
                return TRUE;
            }
        }

    return FALSE;
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
// visible from the person's position. Returns TRUE if found, FALSE otherwise.
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

    SLONG ndx;
    SLONG ndz;
    SLONG ndist;

    SLONG mid_x = p_person->WorldPos.X >> 16;
    SLONG mid_z = p_person->WorldPos.Z >> 16;

    SLONG mx;
    SLONG mz;

    SLONG best_x = 0;
    SLONG best_z = 0;
    SLONG best_score = -INFINITY;

    SLONG x1 = (p_person->WorldPos.X >> 8);
    SLONG y1 = (p_person->WorldPos.Y >> 8) + 0x60;
    SLONG z1 = (p_person->WorldPos.Z >> 8);

    SLONG x2;
    SLONG y2;
    SLONG z2;

    MAV_Action ma;

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

    if (best_score > -INFINITY) {
        *hide_x = best_x;
        *hide_z = best_z;
    }

    return (best_score > -INFINITY);
}

// Returns TRUE if Darci is doing something that would get her arrested (fighting).
// uc_orig: PCOM_player_is_doing_something_naughty (fallen/Source/pcom.cpp)
SLONG PCOM_player_is_doing_something_naughty(Thing* darci)
{
    SLONG map_x;
    SLONG map_z;

    // She isn't allowed to be fighting anyone.
    if (darci->Genus.Person->Mode == PERSON_MODE_FIGHT) {
        return TRUE;
    }

    /*
    map_x = darci->WorldPos.X >> 16;
    map_z = darci->WorldPos.Z >> 16;

    if (PAP_2HI(map_x,map_z).Flags & PAP_FLAG_NAUGHTY)
    {
            return TRUE;
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
                    return TRUE;
            }
    }
    */

    return FALSE;
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
                SLONG dx = p_gang->WorldPos.X - p_target->WorldPos.X;
                SLONG dz = p_gang - p_target;

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

                        PCOM_set_person_ai_kill_person(p_gang, p_target, FALSE);
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
    SLONG start_y;
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
    UBYTE nav_outof_ware = FALSE;
    UBYTE nav_inside_ware = FALSE;

    if ((p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PLACE || p_person->Genus.Person->pcom_ai_state == PCOM_AI_STATE_FLEE_PERSON)) {
        if (p_person->Genus.Person->Ware) {
            nav_outof_ware = TRUE;
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
            nav_inside_ware = TRUE;
        } else {
            nav_outof_ware = TRUE;
            nav_into_ware = FALSE;
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

    UBYTE nav_outof_ware = FALSE;
    UBYTE nav_inside_ware = FALSE;
    SLONG nav_into_ware = NULL;

    UBYTE caps = (p_person->Genus.Person->pcom_bent & PCOM_BENT_RESTRICTED) ? MAV_CAPS_GOTO : MAV_CAPS_DARCI;

    p_person->Genus.Person->pcom_move_state = PCOM_MOVE_STATE_GOTO_THING;
    p_person->Genus.Person->pcom_move_substate = PCOM_MOVE_SUBSTATE_GOTO;
    p_person->Genus.Person->pcom_move_arg = THING_NUMBER(p_target);
    p_person->Genus.Person->pcom_move_counter = 0;

    if (p_person->Genus.Person->Ware) {
        if (p_target->Class != CLASS_PERSON) {
            nav_outof_ware = TRUE;
        } else if (p_target->Genus.Person->Ware != p_person->Genus.Person->Ware) {
            nav_outof_ware = TRUE;
        } else {
            nav_inside_ware = TRUE;
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
    UBYTE nav_outof_ware = FALSE;
    UBYTE nav_inside_ware = FALSE;
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
            nav_outof_ware = TRUE;
        } else {
            nav_inside_ware = TRUE;
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
    SLONG dest_y;
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

// Returns TRUE if the person has reached their navigation destination.
// Accounts for following mode (tighter threshold) and prolonged sliding.
// uc_orig: PCOM_finished_nav (fallen/Source/pcom.cpp)
SLONG PCOM_finished_nav(Thing* p_person)
{
    SLONG dest_x;
    SLONG dest_z;

    if (p_person->State == STATE_IDLE) {
        // Not moving at all — treat as finished.
        return TRUE;
    }

    if (p_person->State == STATE_DANGLING) {
        // Mid-manoeuvre, can't stop.
        return FALSE;
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
            return TRUE;
        } else {
            return FALSE;
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
    Thing* p_special;

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
    SLONG c0, count = 0;
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
    SLONG c0, count = 0, ret;

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
    SLONG c0, count = 0, ret;
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
    SLONG c0, perp;

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
    SLONG attack_count = 0;

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
                    attack_count++;
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
            return TRUE;
        }
    }

    return FALSE;
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
        SLONG best_dist = INFINITY;

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
    UWORD anim;

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

    SLONG best_score = INFINITY;
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
            TRUE);
    } else {
        waypoint = EWAY_find_waypoint_rand(
            p_person->Genus.Person->pcom_move_arg,
            p_person->Genus.Person->pcom_colour,
            p_person->Genus.Person->pcom_group,
            TRUE);
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
                        TRUE);
                } else {
                    waypoint = EWAY_find_waypoint_rand(
                        p_person->Genus.Person->pcom_move_arg,
                        p_person->Genus.Person->pcom_colour,
                        p_person->Genus.Person->pcom_group,
                        TRUE);
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
                        SLONG nx, nz;
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
    SLONG quick_kick = 0;

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
        AENG_world_line(x1, y1, z1, 20, 0xffffff, x1 + dx, y1 + dy, z1 + dz, 20, 0xffffff, TRUE);

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
