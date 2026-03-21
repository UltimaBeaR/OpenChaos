#include "missions/eway.h"

#include "Game.h"
#include "cam.h"
#include "bang.h"
#include "ob.h"
#include "fmatrix.h"
#include "trip.h"
#include "dirt.h"
#include "thing.h"
#include "pow.h"
#include "fallen/Headers/music.h"
#include "font2d.h"
#include "fallen/DDEngine/Headers/console.h"
#include "fallen/DDEngine/Headers/map.h"
#include "fallen/Headers/chopper.h"    // Temporary: Chopper not migrated
#include "fallen/Headers/animal.h"     // Temporary: Animal/BAT not migrated
#include "engine/io/drive.h"           // drive.h already migrated to new/
#include "fallen/Headers/pcom.h"       // Temporary: PCOM not migrated
#include "fallen/Headers/supermap.h"   // Temporary: find_electric_fence_dbuilding
#include "fallen/Headers/sound.h"
#include "fallen/Headers/statedef.h"
#include "fallen/Headers/wmove.h"      // Temporary: WMOVE_create
#include "fallen/DDEngine/Headers/aeng.h"
#include "fallen/DDEngine/Headers/panel.h"
#include "missions/memory.h"
#include "fallen/Headers/cnet.h"
#include "mfx.h"
#include "fallen/Headers/fc.h"
#include "fallen/Headers/ware.h"
#include "fallen/Headers/door.h"       // Temporary: DOOR not migrated
#include "fallen/Headers/interfac.h"
#include "fallen/Headers/psystem.h"
#include "poly.h"
#include "fallen/Headers/playcuts.h"
#include "fallen/Headers/xlat_str.h"
#include "fallen/Headers/mist.h"
#include "fallen/Headers/gamemenu.h"
#include "fallen/Headers/env.h"
#include "fallen/Headers/prim.h"       // Temporary: find_anim_prim, PrimInfo
#include "fallen/Headers/vehicle.h"    // Temporary: VEH_create, Vehicle not migrated
#include "fallen/Headers/collide.h"    // Temporary: there_is_a_los

// Forward declaration not in any header.
// uc_orig: person_ok_for_conversation (fallen/Source/eway.cpp)
extern SLONG person_ok_for_conversation(Thing* p_person);

// Defined in Person.cpp; drives NPC boredom/respawn cycle.
// uc_orig: timer_bored (fallen/Source/Person.cpp)
extern ULONG timer_bored;

// Defined in elev_globals.cpp via missions/elev_globals.h.
// uc_orig: ELEV_fname_level (fallen/Source/elev.cpp)
extern CBYTE ELEV_fname_level[_MAX_PATH];

// Extracts the level name word from ELEV_fname_level (e.g. "Rescue1" from "path\Rescue1.ucm").
// uc_orig: get_level_word (fallen/Source/eway.cpp)
void get_level_word(CBYTE* str)
{
    SLONG c0 = 0, c1 = 0;

    while (ELEV_fname_level[c0] != '\\' && c0 < 101) {
        c0++;
    }
    ASSERT(c0 < 99);
    c0++;
    while (ELEV_fname_level[c0] != '.' && c0 < 151) {
        str[c1++] = ELEV_fname_level[c0];
        c0++;
    }
    str[c1] = 0;
}

// Returns TRUE if the current level is one of the combat tutorial levels.
// uc_orig: playing_combat_tutorial (fallen/Source/eway.cpp)
SLONG playing_combat_tutorial(void)
{

    SLONG c0 = 0, c1 = 0;

    while (ELEV_fname_level[c0] != '\\' && c0 < 101) {
        c0++;
    }
    if (c0 > 99)
        return (0);
    ASSERT(c0 < 99);
    c0++;

    if (!strcmp(&ELEV_fname_level[c0], "FTutor1.ucm")) {
        return (1);
    } else if (!strcmp(&ELEV_fname_level[c0], "fight1.ucm")) {
        return (1);
    } else if (!strcmp(&ELEV_fname_level[c0], "fight2.ucm")) {
        return (1);
    } else {
        return (0);
    }
}

// Returns TRUE if the currently loaded level filename matches 'name'.
// uc_orig: playing_level (fallen/Source/eway.cpp)
SLONG playing_level(const CBYTE* name)
{
    SLONG c0 = 0, c1 = 0;

    while (ELEV_fname_level[c0] != '\\' && c0 < 101) {
        c0++;
    }
    if (c0 > 99)
        return (0);
    ASSERT(c0 < 99);
    c0++;

    if (!strcmp(&ELEV_fname_level[c0], name)) {
        return (1);
    } else {
        return (0);
    }
}

// Returns TRUE if the current level is a real scored mission (not tutorial/fight/drive).
// uc_orig: playing_real_mission (fallen/Source/eway.cpp)
SLONG playing_real_mission(void)
{

    SLONG c0 = 0, c1 = 0;

    while (ELEV_fname_level[c0] != '\\' && c0 < 101) {
        c0++;
    }
    if (c0 > 99)
        return (0);
    ASSERT(c0 < 99);
    c0++;

    while (crap_levels[c1]) {
        if (!strcmp(&ELEV_fname_level[c0], crap_levels[c1])) {
            return (0);
        }
        c1++;
    }
    return (1);
}

// Plays the voiced speech WAV for the given waypoint number.
// Stops any music before playing. File: <SpeechPath>/talk2/<level>.ucm<waypoint>.wav
// uc_orig: EWAY_talk (fallen/Source/eway.cpp)
void EWAY_talk(ULONG waypoint)
{

    CBYTE str[100];
    CBYTE level[100];

    get_level_word(level);
    sprintf(str, "%stalk2\\%s.ucm%d.wav", GetSpeechPath(), level, waypoint);

    if (MUSIC_is_playing()) {
        MFX_QUICK_stop();
    }

    MFX_QUICK_wait();

    MFX_QUICK_play(str);
    EWAY_conv_talk = 0;
}

// Stops voiced speech if the speaker is dead, hit, or 'stop' is requested.
// uc_orig: check_eway_talk (fallen/Source/eway.cpp)
void check_eway_talk(SLONG stop)
{
    if (!MFX_QUICK_still_playing())
        talk_thing = 0;

    if (talk_thing) {
        if (stop || talk_thing->State == STATE_HIT_RECOIL || talk_thing->State == STATE_DEAD || talk_thing->State == STATE_DYING) {
            MFX_QUICK_stop();
            talk_thing = NULL;
        }
    }
}

// Plays a lettered conversation WAV (A/B/C... sub-parts within a waypoint dialogue).
// uc_orig: EWAY_talk_conv (fallen/Source/eway.cpp)
void EWAY_talk_conv(ULONG waypoint, SLONG conversation)
{

    CBYTE str[100];
    CBYTE level[100];

    talk_thing = NULL;
    get_level_word(level);
    sprintf(str, "%stalk2\\%s.ucm%d%c.wav", GetSpeechPath(), level, waypoint, 65 + conversation);

    if (MUSIC_is_playing()) {
        MFX_QUICK_stop();
    }

    MFX_QUICK_wait();

    Thing* speaker = TO_THING(EWAY_conv_person_a);
    EWAY_conv_talk = MFX_QUICK_play(str, speaker->WorldPos.X >> 8, speaker->WorldPos.Y >> 8, speaker->WorldPos.Z >> 8);
}

// Returns the message string at the given index (range-checked).
// uc_orig: EWAY_get_mess (fallen/Source/eway.cpp)
CBYTE* EWAY_get_mess(SLONG index)
{
    ASSERT(index < EWAY_mess_upto);
    ASSERT(EWAY_mess[index] >= &EWAY_mess_buffer[0] && EWAY_mess[index] < &EWAY_mess_buffer[EWAY_mess_buffer_upto]);
    ASSERT(EWAY_mess[index] != (CBYTE*)0x3c003c00);
    ASSERT(index >= 0);
    return (EWAY_mess[index]);
}

// Resets all EWAY state at the start of a level. Memory is pre-allocated; this zeroes it.
// Index 0 is intentionally skipped in all arrays.
// uc_orig: EWAY_init (fallen/Source/eway.cpp)
void EWAY_init()
{

    talk_thing = 0;

    memset((UBYTE*)EWAY_way, 0, sizeof(EWAY_Way) * EWAY_MAX_WAYS);

    EWAY_way_upto = 1;

    memset((UBYTE*)EWAY_edef, 0, sizeof(EWAY_Edef) * EWAY_MAX_EDEFS);

    EWAY_edef_upto = 1;

    EWAY_mess_buffer_upto = 0;
    EWAY_mess_upto = 0;

    memset((UBYTE*)EWAY_mess, 0, sizeof(CBYTE*) * EWAY_MAX_MESSES);

    memset((UBYTE*)EWAY_cond, 0, sizeof(EWAY_Cond) * EWAY_MAX_CONDS);

    EWAY_cond_upto = 1;

    memset((UBYTE*)EWAY_timer, 0, sizeof(UWORD) * EWAY_MAX_TIMERS);

    EWAY_cam_active = FALSE;
    EWAY_conv_active = FALSE;

    memset(EWAY_counter, 0, sizeof(UBYTE) * EWAY_MAX_COUNTERS);
}

// Spawns the player character (Darci, Roper, Cop, or Thug) at the given position.
// Applies player stats from the_game after creation.
// Note: Roper uses Darci stats in this pre-release build (Roper stat block is commented out).
// uc_orig: EWAY_create_player (fallen/Source/eway.cpp)
UWORD EWAY_create_player(
    UBYTE subtype,
    UBYTE yaw,
    SLONG has,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z)
{
    UWORD p_index;

    ASSERT(WITHIN(NO_PLAYERS, 0, 1));

    switch (subtype & 7) {
    case PLAYER_DARCI:

        p_index = PCOM_create_player(
            PLAYER_DARCI,
            has,
            world_x,
            world_y,
            world_z,
            NO_PLAYERS,
            yaw << 3);

        NET_PERSON(NO_PLAYERS) = TO_THING(p_index);

        NET_PLAYER(NO_PLAYERS)->Genus.Player->Strength = the_game.DarciStrength;
        NET_PLAYER(NO_PLAYERS)->Genus.Player->Constitution = the_game.DarciConstitution;
        NET_PLAYER(NO_PLAYERS)->Genus.Player->Stamina = the_game.DarciStamina;
        NET_PLAYER(NO_PLAYERS)->Genus.Player->Skill = the_game.DarciSkill;

        break;

    case PLAYER_ROPER:

        p_index = PCOM_create_player(
            PLAYER_ROPER,
            has,
            world_x,
            world_y,
            world_z,
            NO_PLAYERS,
            yaw << 3);

        NET_PERSON(NO_PLAYERS) = TO_THING(p_index);

        NET_PLAYER(NO_PLAYERS)->Genus.Player->Strength = the_game.DarciStrength;
        NET_PLAYER(NO_PLAYERS)->Genus.Player->Constitution = the_game.DarciConstitution;
        NET_PLAYER(NO_PLAYERS)->Genus.Player->Stamina = the_game.DarciStamina;
        NET_PLAYER(NO_PLAYERS)->Genus.Player->Skill = the_game.DarciSkill;
        /*
                NET_PLAYER(NO_PLAYERS)->Genus.Player->Strength	  =the_game.RoperStrength;
                NET_PLAYER(NO_PLAYERS)->Genus.Player->Constitution=the_game.RoperConstitution;
                NET_PLAYER(NO_PLAYERS)->Genus.Player->Stamina	  =the_game.RoperStamina;
                NET_PLAYER(NO_PLAYERS)->Genus.Player->Skill		  =the_game.RoperSkill;
        */

        break;

    case PLAYER_COP:

        p_index = PCOM_create_player(
            PLAYER_COP,
            has,
            world_x,
            world_y,
            world_z,
            NO_PLAYERS,
            yaw << 3);

        NET_PERSON(NO_PLAYERS) = TO_THING(p_index);

        break;

    case PLAYER_THUG:

        p_index = PCOM_create_player(
            PLAYER_THUG,
            has,
            world_x,
            world_y,
            world_z,
            NO_PLAYERS,
            yaw << 3);

        NET_PERSON(NO_PLAYERS) = TO_THING(p_index);

        break;

    default:
        ASSERT(0);
        break;
    }

    NO_PLAYERS += 1;

    return p_index;
}

// Spawns an NPC enemy. Zone encodes spawn zone (low 4 bits) + flags (bits 4-6):
//   bit 4 = FLAG2_PERSON_INVULNERABLE
//   bit 5 = FLAG2_PERSON_GUILTY
//   bit 6 = FLAG2_PERSON_FAKE_WANDER
// uc_orig: EWAY_create_enemy (fallen/Source/eway.cpp)
UWORD EWAY_create_enemy(
    UBYTE subtype,
    UBYTE yaw,
    UBYTE colour,
    UBYTE group,
    UBYTE pcom_ai,
    UBYTE pcom_bent,
    UBYTE pcom_move,
    UBYTE drop,
    UBYTE zone,
    SLONG skill,
    UWORD follow,
    UWORD other,
    SLONG has,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG random)
{
    THING_INDEX p_index;
    Thing* p_person;
    ULONG f1 = 0, f2 = 0;

    if (zone & (1 << 4)) {
        f2 |= FLAG2_PERSON_INVULNERABLE;
    }
    if (zone & (1 << 5)) {
        f2 |= FLAG2_PERSON_GUILTY;
    }
    if (zone & (1 << 6)) {
        f2 |= FLAG2_PERSON_FAKE_WANDER;
    }

    zone &= 0xf;

    p_index = PCOM_create_person(
        subtype,
        colour,
        group,
        pcom_ai,
        other,
        skill,
        pcom_move,
        follow,
        pcom_bent,
        has,
        drop,
        zone,
        world_x << 8,
        world_y << 8,
        world_z << 8,
        yaw << 3,
        random, f1, f2);

    p_person = TO_THING(p_index);

    return p_index;
}

// Creates an animal at the given position. Only gargoyle/balrog/bane are functional here.
// uc_orig: EWAY_create_animal (fallen/Source/eway.cpp)
UWORD EWAY_create_animal(
    UBYTE subtype,
    UBYTE yaw,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z)
{
    UWORD p_index;

    switch (subtype) {
    case EWAY_SUBTYPE_ANIMAL_BAT:
        break;

    case EWAY_SUBTYPE_ANIMAL_GARGOYLE:
        p_index = BAT_create(
            BAT_TYPE_GARGOYLE,
            world_x,
            world_z,
            yaw << 3);
        break;

    case EWAY_SUBTYPE_ANIMAL_BALROG:
        p_index = BAT_create(
            BAT_TYPE_BALROG,
            world_x,
            world_z,
            yaw << 3);
        break;

    case EWAY_SUBTYPE_ANIMAL_BANE:
        p_index = BAT_create(
            BAT_TYPE_BANE,
            world_x,
            world_z,
            yaw << 3);
        break;

    default:
        ASSERT(0);
        break;
    }

    return p_index;
}

// Creates a pickup special. If EWAY_ARG_ITEM_STASHED_IN_PRIM, hides it inside a nearby OB.
// uc_orig: EWAY_create_item (fallen/Source/eway.cpp)
UWORD EWAY_create_item(
    UBYTE subtype,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    EWAY_Way* ew)
{
    SLONG way_index;
    Thing* p_thing;

    if (ew == NULL) {
        way_index = NULL;
    } else {
        way_index = ew - EWAY_way;
    }

    if (subtype == SPECIAL_MINE) {
        world_y = PAP_calc_map_height_at(world_x, world_z);
    } else {
        SLONG ground = PAP_calc_map_height_at(world_x, world_z);

        if (world_y < ground + 0x40 && world_y > ground - 0xc0)

            if (WITHIN(world_y, ground - 0xc0, ground + 0x40)) {
                world_y = ground + 0x40;
            }
    }

    p_thing = alloc_special(
        subtype,
        SPECIAL_SUBSTATE_NONE,
        world_x,
        world_y + 0x10,
        world_z,
        way_index);

    ASSERT(p_thing);
    if (p_thing)
        if (ew->ed.arg1 & EWAY_ARG_ITEM_STASHED_IN_PRIM) {
            SLONG ob_index_local;
            OB_Info* oi;
            oi = OB_find_index(world_x, world_y, world_z, 2048, FALSE);
            if (oi) {
                ob_index_local = oi->index;

                OB_ob[ob_index_local].flags |= OB_FLAG_HIDDEN_ITEM | OB_FLAG_SEARCHABLE;
                p_thing->Genus.Special->counter = ob_index_local;

                remove_thing_from_map(p_thing);
                p_thing->Flags |= FLAG_SPECIAL_HIDDEN;
            }
        }

    return THING_NUMBER(p_thing);
}

// Creates a vehicle. Wheeled vehicles are placed one mapsquare above target Y to fall to ground.
// Also sets up vehicle traffic waypoint movement via WMOVE_create().
// uc_orig: EWAY_create_vehicle (fallen/Source/eway.cpp)
UWORD EWAY_create_vehicle(
    UBYTE subtype,
    UBYTE key,
    UWORD arg,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG yaw)
{
    Thing* p_thing = NULL;
    THING_INDEX p_index = NULL;

    switch (subtype) {
    case EWAY_SUBTYPE_VEHICLE_VAN:

        p_index = VEH_create(
            world_x << 8,
            world_y + 0x100 << 8,
            world_z << 8,
            yaw,
            0,
            0,
            VEH_TYPE_VAN,
            key,
            Random());

        WMOVE_create(TO_THING(p_index));

        break;

    case EWAY_SUBTYPE_VEHICLE_HELICOPTER:

    {
        GameCoord posn;

        posn.X = world_x << 8;
        posn.Z = world_z << 8;
        posn.Y = world_y + 265 << 8;

        p_thing = CHOPPER_create(
            posn,
            CHOPPER_CIVILIAN);

        if (arg) {
            p_thing->Genus.Chopper->victim = arg;
        }

        p_index = THING_NUMBER(p_thing);
    }

    break;

    case EWAY_SUBTYPE_VEHICLE_BIKE:

        break;

    case EWAY_SUBTYPE_VEHICLE_CAR:

        p_index = VEH_create(
            world_x << 8,
            world_y + 0x100 << 8,
            world_z << 8,
            yaw, 0, 0,
            VEH_TYPE_CAR,
            key,
            Random());
        WMOVE_create(TO_THING(p_index));
        break;

    case EWAY_SUBTYPE_VEHICLE_TAXI:

        p_index = VEH_create(
            world_x << 8,
            world_y + 0x100 << 8,
            world_z << 8,
            yaw, 0, 0,
            VEH_TYPE_TAXI,
            key,
            Random());
        WMOVE_create(TO_THING(p_index));
        break;

    case EWAY_SUBTYPE_VEHICLE_POLICE:

        p_index = VEH_create(
            world_x << 8,
            world_y + 0x100 << 8,
            world_z << 8,
            yaw, 0, 0,
            VEH_TYPE_POLICE,
            key,
            Random());
        WMOVE_create(TO_THING(p_index));
        break;

    case EWAY_SUBTYPE_VEHICLE_AMBULANCE:

        p_index = VEH_create(
            world_x << 8,
            world_y + 0x100 << 8,
            world_z << 8,
            yaw, 0, 0,
            VEH_TYPE_AMBULANCE,
            key,
            Random());

        WMOVE_create(TO_THING(p_index));

        break;

    case EWAY_SUBTYPE_VEHICLE_JEEP:

        p_index = VEH_create(
            world_x << 8,
            world_y + 0x100 << 8,
            world_z << 8,
            yaw, 0, 0,
            VEH_TYPE_JEEP,
            key,
            Random());
        WMOVE_create(TO_THING(p_index));
        break;

    case EWAY_SUBTYPE_VEHICLE_MEATWAGON:

        p_index = VEH_create(
            world_x << 8,
            world_y + 0x100 << 8,
            world_z << 8,
            yaw, 0, 0,
            VEH_TYPE_MEATWAGON,
            key,
            Random());
        WMOVE_create(TO_THING(p_index));
        break;

    case EWAY_SUBTYPE_VEHICLE_SEDAN:

        p_index = VEH_create(
            world_x << 8,
            world_y + 0x100 << 8,
            world_z << 8,
            yaw, 0, 0,
            VEH_TYPE_SEDAN,
            key,
            Random());

        WMOVE_create(TO_THING(p_index));

        break;

    case EWAY_SUBTYPE_VEHICLE_WILDCATVAN:

        p_index = VEH_create(
            world_x << 8,
            world_y + 0x100 << 8,
            world_z << 8,
            yaw,
            0,
            0,
            VEH_TYPE_WILDCATVAN,
            key,
            Random());

        WMOVE_create(TO_THING(p_index));

        break;

    default:
        ASSERT(0);
        break;
    }

    return p_index;
}

// Converts a raw condition definition (EWAY_Conddef from .ucm) into a runtime EWAY_Cond.
// Performs world-space lookups for tripwires, switches, OB prims, and boolean sub-conditions.
// uc_orig: EWAY_create_cond (fallen/Source/eway.cpp)
EWAY_Cond EWAY_create_cond(
    EWAY_Way* ew,
    EWAY_Conddef* ecd)
{
    EWAY_Cond ans;

    ans.type = ecd->type;
    ans.negate = ecd->negate;
    ans.arg1 = ecd->arg1;
    ans.arg2 = ecd->arg2;

    switch (ans.type) {
    case EWAY_COND_TRIPWIRE:

    {
        SLONG x1;
        SLONG y1;
        SLONG z1;

        SLONG x2;
        SLONG y2;
        SLONG z2;

        SLONG vector[3];

        PrimInfo* pi;

        if (OB_find_type(
                ew->x,
                ew->y,
                ew->z,
                0x200,
                FIND_OB_TRIPWIRE,
                &ob_x,
                &ob_y,
                &ob_z,
                &ob_yaw,
                &ob_prim,
                &ob_index)) {
            pi = get_prim_info(ob_prim);

            FMATRIX_vector(
                vector,
                ob_yaw,
                0);

            x1 = ob_x - MUL64(pi->radius, vector[0]);
            z1 = ob_z - MUL64(pi->radius, vector[2]);
            y1 = ob_y + (pi->miny + pi->maxy >> 1);

            x2 = x1 - MUL64(vector[0], 0x500);
            y2 = y1;
            z2 = z1 - MUL64(vector[2], 0x500);

            if (!there_is_a_los(
                    x1, y1, z1,
                    x2, y2, z2,
                    LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                x2 = los_failure_x;
                y2 = los_failure_y;
                z2 = los_failure_z;
            }

            ans.arg1 = TRIP_create(
                y1,
                x1, z1,
                x2, z2);
        } else {
            ans.arg1 = NULL;
        }
    }

    break;

    case EWAY_COND_SWITCH:

        ans.arg1 = find_anim_prim(
            ew->x,
            ew->y,
            ew->z,
            0x200,
            (1 << ANIM_PRIM_TYPE_SWITCH));

        break;

    case EWAY_COND_BOOL_AND:
    case EWAY_COND_BOOL_OR:

        ASSERT(EWAY_cond_upto + 2 <= EWAY_MAX_CONDS);

        ans.arg1 = EWAY_cond_upto++;
        ans.arg2 = EWAY_cond_upto++;

        EWAY_cond[ans.arg1] = EWAY_create_cond(ew, ecd->bool_arg1);
        EWAY_cond[ans.arg2] = EWAY_create_cond(ew, ecd->bool_arg2);

        break;

    case EWAY_COND_PRIM_DAMAGED:

    {
        SWORD best_dist = 0x7800;
        UWORD best_index = NULL;
        UWORD best_x;
        UWORD best_z;
        SWORD dx;
        SWORD dy;
        SWORD dz;
        SWORD dist;

        OB_Info* oi = OB_find(
            ew->x >> PAP_SHIFT_LO,
            ew->z >> PAP_SHIFT_LO);

        while (oi->prim) {
            dx = ew->x - oi->x;
            dy = ew->y - oi->y;
            dz = ew->z - oi->z;

            dist = abs(dx) + abs(dy) + abs(dz);

            if (best_dist > dist) {
                best_dist = dist;
                best_index = oi->index;
                best_x = oi->x;
                best_z = oi->z;
            }

            oi += 1;
        }

        if (best_index) {
            ew->x = best_x;
            ew->z = best_z;
            ans.arg1 = best_index;
        }
    }

    break;

    case EWAY_COND_RADIUS_USED:

        if (OB_find_type(
                ew->x,
                ew->y,
                ew->z,
                ecd->arg1 * 4,
                FIND_OB_SWITCH_OR_VALVE,
                &ob_x,
                &ob_y,
                &ob_z,
                &ob_yaw,
                &ob_prim,
                &ob_index)) {
            ans.type = EWAY_COND_PRIM_ACTIVATED;
            ans.arg1 = ob_index;
        }

        break;

    case EWAY_COND_PRIM_ACTIVATED:

        ans.arg1 = NULL;

        if (OB_find_type(
                ew->x,
                ew->y,
                ew->z,
                ecd->arg1,
                FIND_OB_SWITCH_OR_VALVE,
                &ob_x,
                &ob_y,
                &ob_z,
                &ob_yaw,
                &ob_prim,
                &ob_index)) {
            ans.arg1 = ob_index;
        }

        break;
    }

    return ans;
}

// Creates and registers a waypoint from .ucm data. Called during level load for each EventPoint.
// Handles special setup: edef storage, prim attachment, bomb creation, vehicle pre-fire optimisation.
// uc_orig: EWAY_create (fallen/Source/eway.cpp)
void EWAY_create(
    SLONG identifier,
    SLONG colour,
    SLONG group,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG yaw,
    EWAY_Conddef* ecd,
    EWAY_Do* ed,
    EWAY_Stay* es,
    EWAY_Edef* ee,
    SLONG unreferenced,
    SLONG kludge_index,
    UWORD magic_index)
{
    EWAY_Way* ew;

    ASSERT(WITHIN(EWAY_way_upto, 1, EWAY_MAX_WAYS - 1));

    ew = &EWAY_way[EWAY_way_upto];

    ew->id = identifier;
    ew->colour = colour;
    ew->group = group;
    ew->flag = 0;
    ew->timer = 0;
    ew->x = world_x;
    ew->y = world_y;
    ew->z = world_z;
    ew->yaw = yaw >> 3;
    ew->index = kludge_index;

    ew->ec = EWAY_create_cond(ew, ecd);
    ew->es = *es;
    ew->ed = *ed;

    if (ew->ed.type == EWAY_DO_MESSAGE || ew->ed.type == EWAY_DO_CONVERSATION || ew->ed.type == EWAY_DO_AMBIENT_CONV) {
        // Encode the magic sample number in the otherwise-unused yaw/index bytes.
        ew->yaw = magic_index >> 8;
        ew->index = magic_index & 0xff;
    }
    if (ew->ed.type == EWAY_DO_CREATE_PLAYER || ew->ed.type == EWAY_DO_CREATE_ANIMAL || ew->ed.type == EWAY_DO_CREATE_ENEMY ||
        ew->ed.type == EWAY_DO_CAMERA_TARGET) {
        // arg will hold the last Thing index created by this waypoint at runtime.
        ew->ed.arg1 = NULL;
        ew->ed.arg2 = NULL;
    }

    if (ew->ed.type == EWAY_DO_CREATE_ENEMY || ew->ed.type == EWAY_DO_CHANGE_ENEMY) {
        ASSERT(WITHIN(EWAY_edef_upto, 0, EWAY_MAX_EDEFS - 1));

        EWAY_edef[EWAY_edef_upto] = *ee;
        ew->index = EWAY_edef_upto;
        EWAY_edef_upto += 1;
    }

    if (ew->ed.type == EWAY_DO_CONTROL_DOOR) {
        // Handled by DOOR.CPP now — no EWAY-side setup needed.
        /*
        (old door attachment code removed, moved to door.cpp)
        */
    }

    if (ew->ed.type == EWAY_DO_ACTIVATE_PRIM) {
        ew->ed.arg1 = find_anim_prim(
            world_x,
            world_y,
            world_z,
            512,
            0xffff);

        if (ew->ed.arg1 == NULL) {
            sprintf(EWAY_message, "Waypoint %d could not find an anim-prim to activate", ew->id);

            CONSOLE_text(EWAY_message, 8000);
        }
    }

    if (ew->ed.type == EWAY_DO_ELECTRIFY_FENCE) {
        ew->ed.arg1 = find_electric_fence_dbuilding(
            world_x,
            world_y,
            world_z,
            256);
    }

    if (ew->ed.type == EWAY_DO_CREATE_BOMB) {
        alloc_special(
            SPECIAL_BOMB,
            SPECIAL_SUBSTATE_ACTIVATED,
            ew->x,
            ew->y + 0x60,
            ew->z,
            EWAY_way_upto);
    }

    if (ew->ec.type == EWAY_COND_TRUE) {
        if (unreferenced) {
            // Vehicles with COND_TRUE + unreferenced can be fired immediately and
            // their slot reused — no need to keep them in the polling loop.
            if (ew->ed.type == EWAY_DO_CREATE_VEHICLE && ew->ed.subtype != EWAY_SUBTYPE_VEHICLE_HELICOPTER) {
                void EWAY_set_active(EWAY_Way * ew);

                EWAY_set_active(ew);

                memset(ew, 0, sizeof(EWAY_Way));

                return;
            }
        }
    }

    EWAY_way_upto += 1;

    return;
}

// Stores a message string into the EWAY message buffer at index 'number'.
// Returns TRUE on success; FALSE if out-of-range or buffer full.
// uc_orig: EWAY_set_message (fallen/Source/eway.cpp)
SLONG EWAY_set_message(
    UBYTE number,
    CBYTE* message)
{
    SLONG len = strlen(message) + 1;

    if (!WITHIN(number, 0, EWAY_MAX_MESSES - 1)) {
        return (0);
    }

    if (EWAY_mess_buffer_upto + len > EWAY_MESS_BUFFER_SIZE) {
        return (0);
    }

    EWAY_mess[number] = &EWAY_mess_buffer[EWAY_mess_buffer_upto];
    if (number >= EWAY_mess_upto)
        EWAY_mess_upto = number + 1;

    strcpy(&EWAY_mess_buffer[EWAY_mess_buffer_upto], message);

    EWAY_mess_buffer_upto += len;
    return (1);
}

// Linear search for a waypoint by its designer id. Returns array index or NULL.
// Only used during load-time fixup; at runtime all ids are already resolved to indices.
// uc_orig: EWAY_find_id (fallen/Source/eway.cpp)
SLONG EWAY_find_id(SLONG id)
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];

        if (ew->id == id) {
            return i;
        }
    }

    return NULL;
}

// Resolves all designer-id references in a condition struct to array indices.
// Special case: ITEM_HELD resolves to the item's SPECIAL_* type for inventory checks.
// uc_orig: EWAY_fix_cond (fallen/Source/eway.cpp)
void EWAY_fix_cond(EWAY_Cond* ec)
{
    SLONG id;
    SLONG id1;
    SLONG id2;

    if (ec->type == EWAY_COND_DEPENDENT || ec->type == EWAY_COND_PERSON_DEAD || ec->type == EWAY_COND_HALF_DEAD || ec->type == EWAY_COND_PERSON_USED || ec->type == EWAY_COND_ITEM_HELD || ec->type == EWAY_COND_CONVERSE_END || ec->type == EWAY_COND_PERSON_ARRESTED || ec->type == EWAY_COND_COUNTDOWN || ec->type == EWAY_COND_COUNTDOWN_SEE || ec->type == EWAY_COND_PERSON_NEAR || ec->type == EWAY_COND_IS_MURDERER || ec->type == EWAY_COND_KILLED_NOT_ARRESTED || ec->type == EWAY_COND_THING_RADIUS_DIR || ec->type == EWAY_COND_SPECIFIC_ITEM_HELD || ec->type == EWAY_COND_MOVE_RADIUS_DIR || ec->type == EWAY_COND_RANDOM) {
        ec->arg1 = EWAY_find_id(ec->arg1);

        if (ec->type == EWAY_COND_ITEM_HELD) {
            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            if (EWAY_way[ec->arg1].ed.type == EWAY_DO_CREATE_ITEM) {
                ec->arg1 = EWAY_way[ec->arg1].ed.subtype;
            } else if (EWAY_way[ec->arg1].ed.type == EWAY_DO_CREATE_ENEMY) {
                EWAY_Edef* ee;
                EWAY_Way* ew = &EWAY_way[ec->arg1];

                ASSERT(WITHIN(ew->index, 1, EWAY_edef_upto - 1));

                ee = &EWAY_edef[ew->index];

                ec->arg1 = ee->drop;
            }
        }
    } else if (ec->type == EWAY_COND_A_SEE_B || ec->type == EWAY_COND_PERSON_IN_VEHICLE) {
        ec->arg1 = EWAY_find_id(ec->arg1);
        ec->arg2 = EWAY_find_id(ec->arg2);
    }
}

// Resolves designer-id references in an action struct to array indices.
// uc_orig: EWAY_fix_do (fallen/Source/eway.cpp)
void EWAY_fix_do(EWAY_Do* ed, EWAY_Way* ew)
{
    SLONG id;
    SLONG id1;
    SLONG id2;

    if (ed->type == EWAY_DO_CHANGE_ENEMY || ed->type == EWAY_DO_KILL_WAYPOINT || ed->type == EWAY_DO_LOCK_VEHICLE || ed->type == EWAY_DO_CHANGE_ENEMY_FLG || ed->type == EWAY_DO_STALL_CAR || ed->type == EWAY_DO_EXTEND_COUNTDOWN || ed->type == EWAY_DO_TRANSFER_PLAYER || ed->type == EWAY_DO_MAKE_PERSON_PEE || ed->type == EWAY_DO_MOVE_THING) {
        ed->arg1 = EWAY_find_id(ed->arg1);
    } else if (ed->type == EWAY_DO_CONVERSATION || ed->type == EWAY_DO_AMBIENT_CONV) {

        ed->arg1 = EWAY_find_id(ed->arg1);
        ed->arg2 = EWAY_find_id(ed->arg2);
    } else if (ed->type == EWAY_DO_NAV_BEACON) {
        if (ed->arg2) {
            ed->arg2 = EWAY_find_id(ed->arg2);

            if (EWAY_way[ed->arg2].ed.type == EWAY_DO_CREATE_PLAYER || EWAY_way[ed->arg2].ed.type == EWAY_DO_CREATE_ENEMY || EWAY_way[ed->arg2].ed.type == EWAY_DO_CREATE_ANIMAL || EWAY_way[ed->arg2].ed.type == EWAY_DO_CREATE_VEHICLE || EWAY_way[ed->arg2].ed.type == EWAY_DO_CREATE_ITEM) {
                // Okay to track these waypoints.
            } else {
                // Can't track these; it's a location, not a thing.
                ed->arg2 = NULL;
            }
        }
    } else if (ed->type == EWAY_DO_CREATE_VEHICLE) {
        if (ed->arg2) {
            ed->arg2 = EWAY_find_id(ed->arg2);
        }
    } else if (ed->type == EWAY_DO_MESSAGE) {
        if (ed->arg2 && ed->arg2 != EWAY_MESSAGE_WHO_STREETNAME && ed->arg2 != EWAY_MESSAGE_WHO_TUTORIAL) {
            ed->arg2 = EWAY_find_id(ed->arg2);
        }
    }
}

// Resolves designer-id references in an enemy definition (bodyguard target, follow target).
// uc_orig: EWAY_fix_edef (fallen/Source/eway.cpp)
void EWAY_fix_edef(EWAY_Edef* ee)
{
    if (ee->pcom_ai == PCOM_AI_BODYGUARD || ee->pcom_ai == PCOM_AI_ASSASIN || ee->pcom_ai == PCOM_AI_SHOOT_DEAD || ee->pcom_ai == PCOM_AI_KILL_COLOUR) {
        ee->ai_other = EWAY_find_id(ee->ai_other);
    }

    if (ee->pcom_move == PCOM_MOVE_FOLLOW) {
        ee->follow = EWAY_find_id(ee->follow);
    }
}

// Loads a numbered text file of mission messages into the EWAY message buffer.
// Format: "1. Message text" per line.
// Returns TRUE if the file was found.
// uc_orig: EWAY_load_message_file (fallen/Source/eway.cpp)
SLONG EWAY_load_message_file(CBYTE* fname, UWORD* index, UWORD* number)
{
    FILE* handle = MF_Fopen(fname, "rb");

    if (handle) {
        CBYTE line[512];
        CBYTE message[512];
        CBYTE* ch;
        CBYTE* start;

        SLONG match;
        SLONG upto;

        *index = EWAY_mess_upto;
        *number = 0;

        while (fgets(line, 512, handle)) {
            match = sscanf(line, "%d. %s", &upto, &message);

            if (match == 2) {
                for (ch = line; *ch != '.'; ch++)
                    ;
                while (isspace(*++ch))
                    ;
                start = ch;

                while (*ch)
                    ch++;
                for (; isspace(*--ch); *ch = '\000')
                    ;

                if (EWAY_set_message(*index + *number, start)) {
                    *number += 1;
                }
            }
        }

        MF_Fclose(handle);

        return TRUE;
    }

    return FALSE;
}

// Loads ambient NPC dialogue pools from text files. Handles localisation.
// Called after all EWAY_set_message() calls are done.
// uc_orig: EWAY_load_fake_wander_text (fallen/Source/eway.cpp)
void EWAY_load_fake_wander_text(CBYTE* fname)
{
    CBYTE name_buffer[_MAX_PATH];
    CBYTE str[100];

    EWAY_fake_wander_text_normal_index = NULL;
    EWAY_fake_wander_text_normal_number = 0;
    EWAY_fake_wander_text_guilty_index = NULL;
    EWAY_fake_wander_text_guilty_number = 0;
    EWAY_fake_wander_text_annoyed_index = NULL;
    EWAY_fake_wander_text_annoyed_number = 0;

    XLAT_str(X_THIS_LANGUAGE_IS, name_buffer);

    if (!stricmp(name_buffer, "Italian")) {
        fname = "text\\citsez_ita.txt";
    }
    if (!stricmp(name_buffer, "French")) {
        fname = "text\\citsez_french.txt";
    }
    if (!stricmp(name_buffer, "Deutsch")) {
        fname = "text\\citsez_german.txt";
    }
    if (!stricmp(name_buffer, "Spanish")) {
        fname = "text\\citsez_spa.txt";
    }

    if (fname == NULL) {
        fname = "text\\citsez.txt";
    }

    if (!EWAY_load_message_file(
            fname,
            &EWAY_fake_wander_text_normal_index,
            &EWAY_fake_wander_text_normal_number)) {
        EWAY_load_message_file(
            "text\\citsez.txt",
            &EWAY_fake_wander_text_normal_index,
            &EWAY_fake_wander_text_normal_number);
    }

    EWAY_load_message_file(
        "text\\guilty.txt",
        &EWAY_fake_wander_text_guilty_index,
        &EWAY_fake_wander_text_guilty_number);

    EWAY_load_message_file(
        "text\\annoyed.txt",
        &EWAY_fake_wander_text_annoyed_index,
        &EWAY_fake_wander_text_annoyed_number);

    ASSERT(EWAY_fake_wander_text_annoyed_index + EWAY_fake_wander_text_annoyed_number <= EWAY_mess_upto);
    ASSERT(EWAY_fake_wander_text_guilty_index + EWAY_fake_wander_text_guilty_number <= EWAY_mess_upto);
    ASSERT(EWAY_fake_wander_text_normal_index + EWAY_fake_wander_text_normal_number <= EWAY_mess_upto);

    // This assert fires if citsez.txt is missing.
    ASSERT(EWAY_fake_wander_text_normal_number > 0);
    return;
}

// Returns a random message from the NPC ambient dialogue pool for the given type.
// Non-English builds always return NORMAL type.
// uc_orig: EWAY_get_fake_wander_message (fallen/Source/eway.cpp)
CBYTE* EWAY_get_fake_wander_message(SLONG type)
{

    UWORD index;
    UWORD number;
    static SBYTE this_is_english = -1;

    if (this_is_english < 0) {
        this_is_english = !stricmp(XLAT_str(X_THIS_LANGUAGE_IS), "English");
    }

    if (!this_is_english)
        type = EWAY_FAKE_MESSAGE_NORMAL;
    switch (type) {
    case EWAY_FAKE_MESSAGE_NORMAL:
        index = EWAY_fake_wander_text_normal_index;
        number = EWAY_fake_wander_text_normal_number;
        break;

    case EWAY_FAKE_MESSAGE_ANNOYED:
        index = EWAY_fake_wander_text_annoyed_index;
        number = EWAY_fake_wander_text_annoyed_number;
        break;

    case EWAY_FAKE_MESSAGE_GUILTY:
        index = EWAY_fake_wander_text_guilty_index;
        number = EWAY_fake_wander_text_guilty_number;
        break;

    default:
        ASSERT(0);
        break;
    }

    ASSERT(index + number <= EWAY_mess_upto);

    if (number == 0) {
        ASSERT(FALSE);
        return 0;
    } else {
        SLONG which = Random() % number;

        ASSERT(WITHIN(which + index, 0, EWAY_mess_upto - 1));

        if (which + index >= EWAY_mess_upto) {
            which = 0;
            index = 0;
        }

        return EWAY_mess[which + index];
    }
}

// Post-load fixup pass. Called once after all EWAY_create() calls.
// 1. Resolves all designer-id references to array indices.
// 2. Calculates CRIME_RATE_SCORE_MUL from objective points and guilty NPCs.
// 3. Tags mission-fail messages with EWAY_FLAG_WHY_LOST.
// 4. Attaches CAMERA_TARGET_THING waypoints to the nearest CREATE_* waypoint.
// uc_orig: EWAY_created_last_waypoint (fallen/Source/eway.cpp)
void EWAY_created_last_waypoint()
{
    SLONG i;
    SLONG id;
    SLONG points;
    SLONG total_points;
    SLONG num_guilty_people;

    EWAY_Way* ew;
    EWAY_Cond* ec;
    EWAY_Do* ed;
    EWAY_Edef* ee;

    total_points = 0;
    num_guilty_people = 0;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];

        ec = &ew->ec;
        ed = &ew->ed;

        EWAY_fix_cond(ec);
        EWAY_fix_do(ed, ew);

        if (ed->type == EWAY_DO_CAMERA_TARGET && ed->subtype == EWAY_SUBTYPE_CAMERA_TARGET_THING) {
            // Find the nearest CREATE_* waypoint to attach the camera target to.
            SLONG j;
            SLONG dx;
            SLONG dy;
            SLONG dz;
            SLONG dist;
            SLONG best_dist = 0x300;
            SLONG best_way = NULL;

            EWAY_Way* ew_near;

            for (j = 1; j < EWAY_way_upto; j++) {
                ew_near = &EWAY_way[j];

                switch (ew_near->ed.type) {
                case EWAY_DO_CREATE_PLAYER:
                case EWAY_DO_CREATE_ANIMAL:
                case EWAY_DO_CREATE_ENEMY:
                case EWAY_DO_CREATE_ITEM:
                case EWAY_DO_CREATE_VEHICLE:
                case EWAY_DO_CREATE_PLATFORM:

                    dx = abs(ew_near->x - ew->x);
                    dy = abs(ew_near->y - ew->y);
                    dz = abs(ew_near->z - ew->z);

                    dist = QDIST3(dx, dy, dz);

                    if (dist < best_dist) {
                        best_dist = dist;
                        best_way = j;
                    }

                    break;

                default:
                    break;
                }
            }
            if (best_way == NULL) {
                sprintf(EWAY_message, "Camera target %d couldn't attach", i);

                CONSOLE_text(EWAY_message, 8000);
            }
            ew->ed.arg1 = best_way;
        } else if (ew->ed.type == EWAY_DO_OBJECTIVE) {
            points = ew->ed.arg2;

            switch (ew->ed.subtype) {
            case EWAY_SUBTYPE_OBJECTIVE_MAIN:
                // Main objective does not contribute to the crime rate.
                break;

            case EWAY_SUBTYPE_OBJECTIVE_SUB:
                total_points += points;
                break;

            case EWAY_SUBTYPE_OBJECTIVE_ADDITIONAL:
                total_points += points;
                break;
            }
        } else if (ew->ed.type == EWAY_DO_CREATE_ENEMY) {
            EWAY_Edef* ee;

            ASSERT(WITHIN(ew->index, 1, EWAY_edef_upto - 1));

            ee = &EWAY_edef[ew->index];

            if (ee->pcom_ai == PCOM_AI_GANG) {
                num_guilty_people += 1;
            }
        }
    }

    for (i = 1; i < EWAY_cond_upto; i++) {
        ec = &EWAY_cond[i];

        EWAY_fix_cond(ec);
    }

    for (i = 1; i < EWAY_edef_upto; i++) {
        ee = &EWAY_edef[i];

        EWAY_fix_edef(ee);
    }

    EWAY_time_accurate = 0;
    EWAY_time = 0;
    EWAY_count_up = 0;

    EWAY_count_up_add_penalties = 0;
    EWAY_count_up_num_penalties = 0;
    EWAY_count_up_penalty_timer = 0;

    EWAY_tutorial_string = NULL;

    // 4% of crime rate per guilty person, capped at 50% of total crime rate.
    if (total_points == 0) {
        CRIME_RATE_SCORE_MUL = 0;
    } else {
        SLONG for_guilty = num_guilty_people * 4;

        SATURATE(for_guilty, 0, CRIME_RATE >> 1);

        CRIME_RATE_SCORE_MUL = ((CRIME_RATE - for_guilty) << 8) / total_points;
    }

    // Tag mission-fail messages: find MISSION_FAIL waypoints and mark the message
    // they depend on with EWAY_FLAG_WHY_LOST so it can be shown on the loss screen.
    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];

        if (ew->ed.type == EWAY_DO_MISSION_FAIL) {
            if (ew->ec.type == EWAY_COND_DEPENDENT) {
                if (WITHIN(ew->ec.arg1, 1, EWAY_way_upto - 1)) {
                    EWAY_way[ew->ec.arg1].flag |= EWAY_FLAG_WHY_LOST;
                }
            }
        }

        if (ew->ed.type == EWAY_DO_MESSAGE) {
            if (ew->ec.type == EWAY_COND_DEPENDENT) {
                if (WITHIN(ew->ec.arg1, 1, EWAY_way_upto - 1)) {
                    if (EWAY_way[ew->ec.arg1].ed.type == EWAY_DO_MISSION_FAIL) {
                        ew->flag |= EWAY_FLAG_WHY_LOST;
                    }
                }
            }
        }
    }
}

// Evaluates a single condition struct against runtime game state.
// Returns TRUE if the condition is satisfied. ec->negate inverts the result.
// EWAY_sub_condition_of_a_boolean=TRUE skips post-evaluation guards (caller is a BOOL_AND/OR).
// uc_orig: EWAY_evaluate_condition (fallen/Source/eway.cpp)
SLONG EWAY_evaluate_condition(EWAY_Way* ew, EWAY_Cond* ec, SLONG EWAY_sub_condition_of_a_boolean)
{
    SLONG ans = FALSE;

    switch (ec->type) {
    case EWAY_COND_FALSE:
        ans = FALSE;
        break;

    case EWAY_COND_TRUE:
        ans = TRUE;
        break;

    case EWAY_COND_PROXIMITY:

    {
        Thing* darci = NET_PERSON(0);

        if (darci) {
            {
                SLONG dx = abs(ew->x - (darci->WorldPos.X >> 8));
                SLONG dy = abs(ew->y - (darci->WorldPos.Y >> 8));
                SLONG dz = abs(ew->z - (darci->WorldPos.Z >> 8));

                SLONG dist = QDIST3(dx, dy, dz);

                ans = (dist < ec->arg1);
            }
        } else {
            ans = FALSE;
        }
    }

    break;

    case EWAY_COND_TRIPWIRE:
        ans = TRIP_activated(ec->arg1);
        break;

    case EWAY_COND_PRESSURE:
        ans = FALSE; // Pressure plates never fire in this codebase (always FALSE).
        break;

    case EWAY_COND_CAMERA:
        ans = FALSE; // Always FALSE here; camera-at condition is satisfied externally
                     // via EWAY_FLAG_ACTIVE in EWAY_process_camera().
        break;

    case EWAY_COND_SWITCH:
        ans = (TO_THING(ec->arg1)->Flags & FLAGS_SWITCHED_ON);
        break;

    case EWAY_COND_TIME:
        ans = (EWAY_time >= ec->arg1);
        break;

    case EWAY_COND_DEPENDENT:

        if (ec->arg1 == NULL) {
            sprintf(EWAY_message, "Waypoint %d has a NULL dependency", ew->id);

            CONSOLE_text(EWAY_message, 8000);
            ans = FALSE;

            // Don't print the message again.
            ec->type = EWAY_COND_FALSE;
        } else {
            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            ans = (EWAY_way[ec->arg1].flag & EWAY_FLAG_ACTIVE);
        }

        break;

    case EWAY_COND_BOOL_AND:

    {
        EWAY_Cond* ec1 = &EWAY_cond[ec->arg1];
        EWAY_Cond* ec2 = &EWAY_cond[ec->arg2];

        ans = EWAY_evaluate_condition(ew, ec1, TRUE) && EWAY_evaluate_condition(ew, ec2, TRUE);
    }

    break;

    case EWAY_COND_BOOL_OR:

    {
        EWAY_Cond* ec1 = &EWAY_cond[ec->arg1];
        EWAY_Cond* ec2 = &EWAY_cond[ec->arg2];

        ans = EWAY_evaluate_condition(ew, ec1, TRUE) || EWAY_evaluate_condition(ew, ec2, TRUE);
    }

    break;

    // Fires after a countdown. ec->arg1 = dependency waypoint (0 = always counting).
    // ec->arg2 = remaining time in EWAY_tick units. Timers pause during conversations.
    case EWAY_COND_COUNTDOWN:
    // Like COUNTDOWN but also draws a visible HUD countdown timer and switches music
    // to MUSIC_MODE_TIMER when under 30 seconds.
    case EWAY_COND_COUNTDOWN_SEE:

        {
            SLONG depend = ec->arg1;
            SLONG timer = ec->arg2;

            ASSERT(depend == 0 || WITHIN(depend, 1, EWAY_way_upto - 1));

            if (!depend || (EWAY_way[depend].flag & EWAY_FLAG_ACTIVE)) {
                if (EWAY_conv_active) {
                    // Timers stop during conversations.
                } else {
                    // Tick down the timer.
                    if (ec->arg2 <= EWAY_tick) {
                        ec->arg2 = 0;
                        ans = TRUE;
                    } else {
                        ec->arg2 -= EWAY_tick;
                        ans = FALSE;
                    }
                }

                if (ec->type == EWAY_COND_COUNTDOWN_SEE) {
                    if (!(GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON))) {
                        if (ec->arg2 < 3000) // 30 seconds... theoretically
                            //								MUSIC_play(S_TUNE_TIMER1,MUSIC_FLAG_OVERRIDE);
                            music_mode_override = MUSIC_MODE_TIMER;
                        else // maybe we were below 30 seconds but time got added
                        //								if (MUSIC_wave()==S_TUNE_TIMER1) MUSIC_stop(1);
                        {
                            if (music_mode_override) {
                                // this is a little awkward but override must be cleared before the call or it fails
                                music_mode_override = 0;
                                MUSIC_mode(0);
                            } else
                                music_mode_override = 0;
                        }
                    }
                    PANEL_draw_timer(ec->arg2, 320, 50);
                }
            } else {
                // Not counting down.
                ans = FALSE;
            }
        }

        break;

    case EWAY_COND_PERSON_DEAD:

        ans = FALSE; // By default.

        {
            SLONG waypoint;
            SLONG index;

            Thing* p_thing;
            EWAY_Way* ew_dead;

            waypoint = ec->arg1;

            if (waypoint == 0) {
                CBYTE mess[128];

                sprintf(mess, "Waypoint %d: cond person dead has NULL dependency", ew->id);

                CONSOLE_text(mess, 8000);
                ew->flag |= EWAY_FLAG_DEAD;
            } else {
                ew_dead = &EWAY_way[waypoint];

                switch (ew_dead->ed.type) {
                case EWAY_DO_CREATE_PLAYER:
                case EWAY_DO_CREATE_VEHICLE:
                case EWAY_DO_CREATE_ANIMAL:
                case EWAY_DO_CREATE_ENEMY:

                    index = ew_dead->ed.arg1;

                    if (index) {
                        p_thing = TO_THING(index);

                        ans = (p_thing->State == STATE_DEAD);

                        // MIBs (Men in Black) should not count as dead if they are still
                        // on the map (they may be unconscious but not truly gone).
                        // uc_orig: PersonIsMIB (fallen/Source/Special.cpp)
                        extern BOOL PersonIsMIB(Thing * p_person);

                        if (p_thing->Class == CLASS_PERSON && PersonIsMIB(p_thing)) {
                            if (p_thing->Flags & FLAGS_ON_MAPWHO) {
                                ans = FALSE;
                            }
                        }
                    }

                    break;

                case EWAY_DO_CREATE_ITEM:
                    ans = (ew_dead->flag & EWAY_FLAG_GOTITEM);
                    break;

                case EWAY_DO_CREATE_BARREL:

                {
                    ans = FALSE;

                    if (ew_dead->flag & EWAY_FLAG_ACTIVE) {
                        if (ew_dead->ed.arg1 == NULL) {
                            // The barrel has been created and destroyed.
                            ans = TRUE;
                        }
                    }
                }

                break;

                default: {
                    CBYTE mess[128];

                    sprintf(mess, "Waypoint %d: cond person dead dependent on odd waypoint", ew->id);

                    CONSOLE_text(mess, 8000);
                    ew->flag |= EWAY_FLAG_DEAD;
                }

                break;
                }
            }
        }

        break;

    case EWAY_COND_PERSON_NEAR:

    {
        Thing* p_person;

        SLONG dx;
        SLONG dy;
        SLONG dz;
        SLONG dist;
        SLONG waypoint;
        SLONG index;
        SLONG radius;

        waypoint = ec->arg1;
        radius = ec->arg2 * 64; // Radius in quarter map-squares.

        if (waypoint == NULL) {
            // Just check for anybody being near.
            ans = THING_find_nearest(
                ew->x,
                ew->y,
                ew->z,
                radius,
                THING_FIND_PEOPLE);
        } else {
            index = EWAY_get_person(waypoint);

            if (index) {
                p_person = TO_THING(index);

                dx = abs((p_person->WorldPos.X >> 8) - ew->x);
                dy = abs((p_person->WorldPos.Y >> 8) - ew->y);
                dz = abs((p_person->WorldPos.Z >> 8) - ew->z);

                dist = QDIST3(dx, dy, dz);

                ans = dist < radius;
            } else {
                ans = FALSE; // Person hasn't been created yet.
            }
        }
    }

    break;

    case EWAY_COND_CAMERA_AT:

        // This condition is perversely set in EWAY_process_camera().
        ans = FALSE;

        break;

    case EWAY_COND_PLAYER_CUBE:

    {
        Thing* darci = NET_PERSON(0);

        ans = FALSE; // By default...

        if (darci) {
            SLONG dx = ec->arg1;
            SLONG dz = ec->arg2;

            SLONG minx = ew->x - dx;
            SLONG minz = ew->z - dz;
            SLONG maxx = ew->x + dx;
            SLONG maxz = ew->z + dz;
            SLONG maxy = ew->y;

            if (darci->WorldPos.Y < maxy) {
                if (WITHIN(darci->WorldPos.X >> 8, minx, maxx) && WITHIN(darci->WorldPos.Z >> 8, minz, maxz)) {
                    ans = TRUE;
                }
            }
        }
    }

    break;

    case EWAY_COND_A_SEE_B:

        ans = FALSE; // by default...

        {
            SLONG i_a = ec->arg1;
            SLONG i_b = ec->arg2;

            SLONG person_a = EWAY_get_person(i_a);
            SLONG person_b = EWAY_get_person(i_b);

            if (person_a && person_b) {
                if (TO_THING(person_a)->State != STATE_DEAD) {
                    ans = can_a_see_b(
                        TO_THING(person_a),
                        TO_THING(person_b));
                }
            }
        }

        break;

    case EWAY_COND_GROUP_DEAD:

        // Boy, this is slow!
        ans = FALSE;

        {
            SLONG i;
            SLONG found_at_least_one_person;
            SLONG eway_max = EWAY_way_upto;

            EWAY_Way* ew2;

            ew2 = &EWAY_way[0];
            for (i = 0; i < eway_max; i++) {
                // On Simons, request. This only checks for people in the
                // same colour. It ignores their group.
                if (ew2->colour == ew->colour) {
                    // This waypoint has our group and colour.
                    if (ew2->ed.type == EWAY_DO_CREATE_ENEMY) {
                        if (ew2->ed.arg1) {
                            // uc_orig: is_person_dead (fallen/Source/Person.cpp)
                            extern SLONG is_person_dead(Thing * p_person);

                            if (!is_person_dead(TO_THING(ew2->ed.arg1))) {
                                // Found someone alive.
                                ans = FALSE;

                                break;
                            } else {
                                // Found at least one dead person -- make the default TRUE.
                                ans = TRUE;
                            }
                        }
                    }
                }
                ew2++;
            }
        }

        break;

    case EWAY_COND_HALF_DEAD:

        ans = FALSE;

        {
            SLONG i_person = EWAY_get_person(ec->arg1);

            if (i_person) {
                Thing* p_person = TO_THING(i_person);

                // Is this person half dead?
                if (WITHIN(p_person->Genus.Person->Health, 10, 100)) {
                    ans = TRUE;
                }
            }
        }

        break;

    case EWAY_COND_ITEM_HELD:

        ans = FALSE;

        // Is the player carrying our item?
        if (ec->arg1 == SPECIAL_GUN) {
            if (NET_PERSON(0)->Flags & FLAGS_HAS_GUN) {
                ans = TRUE;
            }
        } else {
            if (person_has_special(
                    NET_PERSON(0),
                    ec->arg1)) {
                ans = TRUE;
            }
        }

        break;

    case EWAY_COND_PERSON_USED:

        if (ec->arg1) {
            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            if (EWAY_way[ec->arg1].ed.type == EWAY_DO_CREATE_ENEMY) {
                EWAY_Way* ew2 = &EWAY_way[ec->arg1];

                if (ew2->ed.arg1) {
                    // Set PERSON_FLAG_USEABLE so that when the player presses
                    // USE near this person the waypoint system will be alerted.
                    ASSERT(TO_THING(ew2->ed.arg1)->Class == CLASS_PERSON);

                    TO_THING(ew2->ed.arg1)->Genus.Person->Flags |= FLAG_PERSON_USEABLE;
                }
            }
        }

        ans = FALSE;

        if (ew->es.type == EWAY_STAY_ALWAYS) {
            ans = !!(ew->flag & EWAY_FLAG_ACTIVE);
        }

        break;

    case EWAY_COND_RADIUS_USED:

    {
        Thing* darci = NET_PERSON(0);

        if (darci) {
            SLONG dx = abs(ew->x - (darci->WorldPos.X >> 8));
            SLONG dy = abs(ew->y - (darci->WorldPos.Y >> 8));
            SLONG dz = abs(ew->z - (darci->WorldPos.Z >> 8));

            SLONG dist = QDIST3(dx, dy, dz);

            if (dist < ec->arg1)
                EWAY_magic_radius_flag = ew;
        }

        // Always return FALSE; this waypoint will be set active in
        // do_an_action() in interfac.cpp.
        ans = FALSE;
    }

    break;

    case EWAY_COND_PRIM_DAMAGED:

        ans = FALSE;

        if (ec->arg1) {
            ASSERT(WITHIN(ec->arg1, 1, OB_ob_upto - 1));

            if (OB_ob[ec->arg1].flags & OB_FLAG_DAMAGED) {
                ans = TRUE;
            }
        }

        break;

    case EWAY_COND_CONVERSE_END:

        ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

        ans = FALSE;

        if (EWAY_way[ec->arg1].flag & EWAY_FLAG_FINISHED) {
            ans = TRUE;
        }

        break;

    case EWAY_COND_COUNTER_GTEQ:

        ASSERT(WITHIN(ec->arg1, 0, EWAY_MAX_COUNTERS - 1));

        ans = FALSE;

        if (EWAY_counter[ec->arg1] >= ec->arg2) {
            ans = TRUE;
        }

        break;

    // True when the NPC has been handcuffed by Darci (player presses arrest button while
    // grappling). Requires FLAG_PERSON_ARRESTED AND SubState == SUB_STATE_DEAD_ARRESTED.
    case EWAY_COND_PERSON_ARRESTED:

        ans = FALSE; // By default.

        if (ec->arg1 == 0) {
            // Person waypoint not set.
            ew->flag |= EWAY_FLAG_DEAD;
        } else {
            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));
            ASSERT(
                EWAY_way[ec->arg1].ed.type == EWAY_DO_CREATE_ENEMY || EWAY_way[ec->arg1].ed.type == EWAY_DO_CREATE_PLAYER);

            UWORD index = EWAY_way[ec->arg1].ed.arg1;

            if (index) {
                Thing* p_person = TO_THING(index);

                ASSERT(p_person->Class == CLASS_PERSON);

                ans = !!(p_person->Genus.Person->Flags & FLAG_PERSON_ARRESTED) && (p_person->SubState == SUB_STATE_DEAD_ARRESTED);
            }
        }

        break;

    case EWAY_COND_PLAYER_CUBOID:
        // uc_orig: is_semtex (fallen/Source/frontend.cpp)
        // Level-specific hack: skip this cuboid check on the semtex mission (waypoint 124).
        extern UBYTE is_semtex;
        if ((ew - EWAY_way) == 124 && is_semtex) // miked remove wetback part for PC/Dreamcast
            ans = FALSE;
        else

        {
            SLONG x1;
            SLONG z1;

            SLONG x2;
            SLONG z2;

            SLONG dx = ec->arg1;
            SLONG dz = ec->arg2;

            Thing* darci = NET_PERSON(0);

            {
                x1 = ew->x - dx;
                x2 = ew->x + dx;

                z1 = ew->z - dz;
                z2 = ew->z + dz;

                if (WITHIN(darci->WorldPos.X >> 8, x1, x2) && WITHIN(darci->WorldPos.Z >> 8, z1, z2)) {
                    ans = TRUE;
                } else {
                    ans = FALSE;
                }
            }
        }

        break;

    case EWAY_COND_KILLED_NOT_ARRESTED:

        ans = FALSE; // By default.

        {
            SLONG waypoint;
            SLONG index;

            Thing* p_thing;
            EWAY_Way* ew_dead;

            waypoint = ec->arg1;

            if (waypoint == 0) {
                CBYTE mess[128];

                sprintf(mess, "Waypoint %d: cond person dead has NULL dependency", ew->id);

                CONSOLE_text(mess, 8000);
                ew->flag |= EWAY_FLAG_DEAD;
            } else {
                ew_dead = &EWAY_way[waypoint];

                switch (ew_dead->ed.type) {
                case EWAY_DO_CREATE_PLAYER:
                case EWAY_DO_CREATE_ENEMY:

                    index = ew_dead->ed.arg1;

                    if (index) {
                        p_thing = TO_THING(index);

                        ASSERT(p_thing->Class == CLASS_PERSON);

                        if (p_thing->State == STATE_DEAD) {
                            // Not if they're arrested.
                            if (!(p_thing->Genus.Person->Flags & FLAG_PERSON_ARRESTED)) {
                                ans = TRUE;
                            }
                        }
                    }

                    break;

                default:

                {
                    CBYTE mess[128];

                    sprintf(mess, "Waypoint %d: killed not arrested dependent on odd waypoint", ew->id);

                    CONSOLE_text(mess, 8000);

                    ew->flag |= EWAY_FLAG_DEAD;
                }

                break;
                }
            }
        }

        break;

    case EWAY_COND_CRIME_RATE_GTEQ:
        ans = (CRIME_RATE >= ec->arg1);
        break;
    case EWAY_COND_CRIME_RATE_LTEQ:
        ans = (CRIME_RATE <= ec->arg1);
        break;

    case EWAY_COND_IS_MURDERER:

    {
        ans = FALSE;

        UWORD person = EWAY_get_person(ec->arg1);

        if (person) {
            Thing* p_person = TO_THING(person);

            if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_IS_MURDERER) {
                ans = TRUE;
            }
        }
    }

    break;

    case EWAY_COND_PERSON_IN_VEHICLE:

        ans = FALSE;

        {
            UWORD person;
            UWORD vehicle;

            person = EWAY_get_person(ec->arg1);

            if (person) {
                Thing* p_person = TO_THING(person);

                ASSERT(p_person->Class == CLASS_PERSON);

                if (p_person->Genus.Person->Flags & (FLAG_PERSON_DRIVING | FLAG_PERSON_BIKING)) {
                    if (ec->arg2) {
                        // They must be driving a specific vehicle!
                        ASSERT(WITHIN(ec->arg2, 1, EWAY_way_upto - 1));
                        ASSERT(EWAY_way[ec->arg2].ed.type == EWAY_DO_CREATE_VEHICLE);

                        if (p_person->Genus.Person->InCar == EWAY_way[ec->arg2].ed.arg1) {
                            ans = TRUE;
                        }
                    } else {
                        ans = TRUE;
                    }
                }
            }
        }

        break;

    // THING_RADIUS_DIR: NPC within radius AND facing a specific direction.
    // MOVE_RADIUS_DIR: same condition logic (despite the name, uses facing angle not move direction).
    // Note: the `abs(speed < 16)` check is a bug in the original (always 0 or 1), preserved as-is.
    case EWAY_COND_THING_RADIUS_DIR:
    case EWAY_COND_MOVE_RADIUS_DIR:

        ans = FALSE;

        {
            UWORD thing = EWAY_get_person(ec->arg1);

            if (thing == NULL) {
            } else {
                Thing* p_thing = TO_THING(thing);
                SLONG radius = ec->arg2 * 64; // Radius in quarter map-squares.

                SLONG dx = abs((p_thing->WorldPos.X >> 8) - ew->x);
                SLONG dy = abs((p_thing->WorldPos.Y >> 8) - ew->y);
                SLONG dz = abs((p_thing->WorldPos.Z >> 8) - ew->z);

                SLONG dist = QDIST3(dx, dy, dz);

                if (dist < radius) {
                    SLONG dangle;
                    SLONG angle;
                    SLONG speed;

                    // Do the angles match up?
                    switch (p_thing->Class) {
                    case CLASS_PERSON:
                        angle = p_thing->Draw.Tweened->Angle;
                        speed = p_thing->Velocity;
                        break;

                    case CLASS_VEHICLE:
                        angle = p_thing->Genus.Vehicle->Angle;
                        speed = p_thing->Velocity;
                        break;
                    default:
                        ASSERT(0);
                        break;
                    }

                    if (ec->type == EWAY_COND_MOVE_RADIUS_DIR || abs(speed < 16)) {
                        dangle = angle - (ew->yaw << 3);

                        if (dangle > +1024) {
                            dangle -= 2048;
                        }
                        if (dangle < -1024) {
                            dangle += 2048;
                        }

                        if (WITHIN(dangle, -128, +128)) {
                            // Angle near enough too...
                            ans = TRUE;
                        }
                    }
                }
            }
        }

        break;

    case EWAY_COND_SPECIFIC_ITEM_HELD:

        ans = FALSE;

        {
            EWAY_Way* ew_other;

            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            ew_other = &EWAY_way[ec->arg1];

            ASSERT(ew_other->ed.type == EWAY_DO_CREATE_ITEM);

            if (ew_other->ed.arg2) {
                // Has this special been picked up by Darci?
                if (ew_other->flag & EWAY_FLAG_GOTITEM) {
                    ans = TRUE;
                }
            }
        }

        break;

    // Picks one random sibling COND_RANDOM waypoint (all sharing the same dependency)
    // and marks it active; all others are killed.
    case EWAY_COND_RANDOM:

    {
        ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

        if (EWAY_way[ec->arg1].flag & EWAY_FLAG_ACTIVE) {
            // Look for all other EWAY_COND_RANDOM waypoints that are dependent
            // on the same waypoint as us.
            // uc_orig: EWAY_MAX_SAME (fallen/Source/eway.cpp)
#define EWAY_MAX_SAME 8

            SLONG i;
            UWORD same[EWAY_MAX_SAME];
            SLONG same_upto = 0;
            SLONG which;

            for (i = 1; i < EWAY_way_upto; i++) {
                if (EWAY_way[i].ec.type == EWAY_COND_RANDOM && EWAY_way[i].ec.arg1 == ec->arg1) {
                    ASSERT(WITHIN(same_upto, 0, EWAY_MAX_SAME - 1));

                    same[same_upto++] = i;
                }
            }

            which = Random() % (same_upto + 1);

            if (which == same_upto) {
                // None of the Random waypoints go active.
            } else {
                // Set active the Random waypoint that we've chosen.
                // uc_orig: EWAY_set_active (fallen/Source/eway.cpp)
                void EWAY_set_active(EWAY_Way * ew);

                EWAY_set_active(&EWAY_way[same[which]]);
            }

            // Kill all the random waypoints.
            for (i = 0; i < same_upto; i++) {
                EWAY_way[same[i]].flag |= EWAY_FLAG_DEAD;
            }
        }
    }

        ans = FALSE;

        break;

    case EWAY_COND_PLAYER_FIRED_GUN:
        ans = !!(GAME_FLAGS & GF_PLAYER_FIRED_GUN);
        break;

    case EWAY_COND_PRIM_ACTIVATED:
        ans = !!(ew->flag & EWAY_FLAG_TRIGGERED);
        break;

    case EWAY_COND_DARCI_GRABBED:
        ans = (NET_PERSON(0)->State == STATE_FIGHTING && NET_PERSON(0)->SubState == SUB_STATE_GRAPPLE_HOLD);
        break;

    case EWAY_COND_PUNCHED_AND_KICKED:

        ans = FALSE;

        if (ec->arg1 == NULL) {
        } else {
            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            if (EWAY_way[ec->arg1].flag & EWAY_FLAG_ACTIVE) {
                if (ew->flag & EWAY_FLAG_CLEARED) {
                    if (EWAY_darci_move & EWAY_DARCI_MOVE_PUNCH) {
                        ec->arg2 += 1;

                        EWAY_darci_move &= ~EWAY_DARCI_MOVE_PUNCH;
                    }

                    if (EWAY_darci_move & EWAY_DARCI_MOVE_KICK) {
                        ec->arg2 += 0x100;

                        EWAY_darci_move &= ~EWAY_DARCI_MOVE_KICK;
                    }

                    if ((ec->arg2 & 0xff) >= 2 && (ec->arg2 >> 8) >= 2) {
                        ans = TRUE;
                    }
                } else {
                    // The first process where this waypoint has gone active.
                    EWAY_darci_move = 0;

                    ew->flag |= EWAY_FLAG_CLEARED;
                }
            }
        }

        break;

    case EWAY_COND_AFTER_FIRST_GAMETURN:
        ans = (GAME_TURN > 0);
        break;

    default:
        ASSERT(0);
        break;
    }

    if (ec->negate) {
        ans = !ans;
    }

    // Skip all this falsifying code if we don't have the final answer yet
    // (called recursively from BOOL_AND/OR).
    if (EWAY_sub_condition_of_a_boolean) {
        return ans;
    }

    if (ew->ed.type == EWAY_DO_CONVERSATION || ew->ed.type == EWAY_DO_AMBIENT_CONV || ew->ed.type == EWAY_DO_MESSAGE) {
        if (GAME_TURN < 1) {
            ans = FALSE;
        }
    }

    if (ew->ed.type == EWAY_DO_MESSAGE && ans && !(ew->flag & EWAY_FLAG_ACTIVE)) {
        // If there is a voice playing, then delay the message triggering.
        if (!WITHIN(ew->ed.arg1, 0, EWAY_MAX_MESSES - 1)) {
            // Bad message...
        } else {
            if (EWAY_used_thing) {
                if (MFX_QUICK_still_playing()) {
                    if (MUSIC_is_playing()) {
                        // Just stop the music...
                        MFX_QUICK_stop();
                        MFX_QUICK_wait();
                    } else {
                        ans = FALSE;
                    }
                }
            }
        }
    }

    if (ew->ed.type == EWAY_DO_CONVERSATION || ew->ed.type == EWAY_DO_AMBIENT_CONV) {
        if (ans && !(ew->flag & EWAY_FLAG_ACTIVE)) {
            Thing* who_says = NULL;

            if (ew->ed.arg2) {
                SLONG who = EWAY_get_person(ew->ed.arg2);

                if (who) {
                    who_says = TO_THING(who);
                }
            }

            if (who_says && who_says->State == STATE_DEAD) {
                // Dead people can't talk.
            } else {
                if (MFX_QUICK_still_playing()) {
                    if (MUSIC_is_playing()) {
                        // Just stop the music...
                        MFX_QUICK_stop();
                        MFX_QUICK_wait();
                    } else {
                        ans = FALSE;
                    }
                }
            }
        }
    }

    if (ew->ed.type == EWAY_DO_CAMERA_CREATE && ans) {
        if (EWAY_stop_player_moving() && !EWAY_cam_goinactive) {
            ans = FALSE;
        }
    }

    if ((ew->ed.type == EWAY_DO_CONVERSATION || ew->ed.type == EWAY_DO_AMBIENT_CONV) && ans) {
        Thing* darci = NET_PERSON(0);

        // Make sure that Darci isn't jumping or falling through the air!
        if (!person_ok_for_conversation(darci)) {
            ans = FALSE;
        }

        // Make sure each person in the conversation is okay...
        UWORD person1 = EWAY_get_person(ew->ed.arg1);
        UWORD person2 = EWAY_get_person(ew->ed.arg2);

        if (person1 == NULL || person2 == NULL) {
            ans = FALSE;
        } else {
            Thing* p_person1 = TO_THING(person1);
            Thing* p_person2 = TO_THING(person2);

            if (!person_ok_for_conversation(p_person1) || !person_ok_for_conversation(p_person2)) {
                ans = FALSE;
            } else {
                // If there isn't a line of sight between the two people...
                SLONG x1 = (p_person1->WorldPos.X >> 8);
                SLONG y1 = (p_person1->WorldPos.Y >> 8) + 0xa0;
                SLONG z1 = (p_person1->WorldPos.Z >> 8);

                SLONG x2 = (p_person2->WorldPos.X >> 8);
                SLONG y2 = (p_person2->WorldPos.Y >> 8) + 0xa0;
                SLONG z2 = (p_person2->WorldPos.Z >> 8);

                if (!there_is_a_los(
                        x1, y1, z1,
                        x2, y2, z2,
                        LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
                    ans = FALSE;
                }
            }
        }
    }

    return ans;
}

// Initialises the scripted cut-scene camera from a CAMERA_CREATE waypoint.
// Sets EWAY_cam_active=TRUE and populates all EWAY_cam_* globals.
// The camera starts at the waypoint position and looks for the first CAMERA_TARGET waypoint
// with the same colour/group. Movement is processed each tick by EWAY_process_camera().
// uc_orig: EWAY_create_camera (fallen/Source/eway.cpp)
void EWAY_create_camera(SLONG waypoint)
{
    SLONG i;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    EWAY_Way* ew;
    EWAY_Way* ew_target;

    ew = &EWAY_way[waypoint];

    EWAY_cam_active = TRUE;
    EWAY_cam_goinactive = FALSE;
    EWAY_cam_x = ew->x << 8;
    EWAY_cam_y = ew->y << 8;
    EWAY_cam_z = ew->z << 8;
    EWAY_cam_dx = 0;
    EWAY_cam_dy = 0;
    EWAY_cam_dz = 0;
    EWAY_cam_yaw = 0;
    EWAY_cam_pitch = 0;
    EWAY_cam_speed = ew->ed.arg1;
    EWAY_cam_delay = ew->ed.arg2 * 10;
    EWAY_cam_waypoint = waypoint;
    EWAY_cam_freeze = !!(ew->ed.subtype & EWAY_SUBTYPE_CAMERA_LOCK_PLAYER);
    EWAY_cam_thing = NULL;
    EWAY_cam_lens = 0x28000;
    EWAY_cam_lock = !!(ew->ed.subtype & EWAY_SUBTYPE_CAMERA_LOCK_DIRECTION);
    EWAY_cam_cant_interrupt = !!(ew->ed.subtype & EWAY_SUBTYPE_CAMERA_CANT_INTERRUPT);

    // This is benign
    ASSERT(EWAY_cam_cant_interrupt == 0);
    EWAY_cam_last_yaw = ew->yaw << 11;
    EWAY_cam_last_x = ew->x << 8;
    EWAY_cam_last_y = ew->y << 8;
    EWAY_cam_last_z = ew->z << 8;
    EWAY_cam_skip = 100; // Can't skip for 1 second.
    EWAY_cam_last_dyaw = 0;
    EWAY_conv_ambient = 0;

    // Find the lowest numbered camera-target waypoint.
    EWAY_cam_target = EWAY_find_waypoint(1, EWAY_DO_CAMERA_TARGET, ew->colour, ew->group, FALSE);

    if (EWAY_cam_target == EWAY_NO_MATCH) {
        // There is nothing for the camera to look at!
        CONSOLE_text("No target for camera", 8000);

        EWAY_cam_active = FALSE;
    }

    MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SEARCH_END);
    MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SLIDE_START);
}

// Advances the scripted cut-scene camera each tick.
// Reads EWAY_cam_* globals set by EWAY_create_camera() and moves the camera through
// CAMERA_WAYPOINT nodes (waypoints with the same colour/group as the CAMERA_CREATE waypoint).
// Camera looks at a CAMERA_TARGET waypoint - can be a fixed position or track a Thing.
// When EWAY_cam_goinactive counts down to 0, sets EWAY_cam_active=FALSE (end of cut-scene).
// Sets EWAY_FLAG_ACTIVE on CAMERA_AT conditions when the camera reaches each node.
// EWAY_cam_freeze=TRUE locks player movement while the camera is running.
// Outputs final camera position/angles to FC_cam[] for the rendering system.
// uc_orig: EWAY_process_camera (fallen/Source/eway.cpp)
void EWAY_process_camera(void)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;
    SLONG next;
    SLONG speed;
    SLONG wspeed;

    SLONG target_stationary = FALSE;

    SLONG look_x;
    SLONG look_y;
    SLONG look_z;
    SLONG look_yaw = 0;

    EWAY_Way* ew_go;
    EWAY_Way* ew_look;
    EWAY_Way* ew_next;

    EWAY_cam_warehouse = NULL;

    if (!EWAY_cam_active) {
        return;
    }

    if (EWAY_cam_goinactive) {
        EWAY_cam_goinactive--;
        if (EWAY_cam_goinactive == 0) {
            EWAY_cam_jumped = 10;
            EWAY_cam_active = FALSE;
            EWAY_cam_goinactive = FALSE;
        }
        return;
    }
    timer_bored = 0;

    EWAY_cam_skip -= EWAY_tick;

    // Work out what we should look at.
    if (EWAY_cam_thing) {
        Thing* p_thing = TO_THING(EWAY_cam_thing);

        look_x = (p_thing->WorldPos.X >> 8);
        look_y = (p_thing->WorldPos.Y >> 8) + 0xa0;
        look_z = (p_thing->WorldPos.Z >> 8);

        if (p_thing->Class == CLASS_PERSON) {
            EWAY_cam_warehouse = p_thing->Genus.Person->Ware;

            if (p_thing->State == STATE_IDLE) {
                target_stationary = TRUE;
            }
            if (p_thing->State == STATE_MOVEING) {
                if (p_thing->SubState == SUB_STATE_SIMPLE_ANIM || p_thing->SubState == SUB_STATE_SIMPLE_ANIM_OVER)

                {
                    target_stationary = TRUE;
                }
            }
        }
    } else if (!EWAY_cam_target) {
        // No waypoint _or_ thing -- so we must be forcing raw coordinates
        look_x = EWAY_cam_targx >> 8;
        look_y = EWAY_cam_targy >> 8;
        look_z = EWAY_cam_targz >> 8;
    } else {
        // The waypoint we are looking at.
        ASSERT(WITHIN(EWAY_cam_target, 1, EWAY_way_upto - 1));

        ew_look = &EWAY_way[EWAY_cam_target];

        // Just look at the waypoint by default...
        look_x = ew_look->x;
        look_y = ew_look->y;
        look_z = ew_look->z;

        EWAY_cam_warehouse = ew_look->ware;

        switch (ew_look->ed.subtype) {
        case EWAY_SUBTYPE_CAMERA_TARGET_PLACE:
            target_stationary = TRUE;
            break;

        case EWAY_SUBTYPE_CAMERA_TARGET_THING:

            if (ew_look->ed.arg1) {
                ASSERT(WITHIN(ew_look->ed.arg1, 1, EWAY_way_upto - 1));

                EWAY_Way* ew_thing;

                ew_thing = &EWAY_way[ew_look->ed.arg1];

                // Has this waypoint created a thing yet?
                if (ew_thing->ed.arg1) {
                    // Yes! Look at the thing.
                    Thing* p_thing = TO_THING(ew_thing->ed.arg1);

                    look_x = (p_thing->WorldPos.X >> 8);
                    look_y = (p_thing->WorldPos.Y >> 8) + 0xa0;
                    look_z = (p_thing->WorldPos.Z >> 8);

                    if (p_thing->Class == CLASS_PERSON) {
                        EWAY_cam_warehouse = p_thing->Genus.Person->Ware;

                        if (p_thing->State == STATE_IDLE) {
                            target_stationary = TRUE;
                        }
                        if (p_thing->State == STATE_MOVEING) {
                            if (p_thing->SubState == SUB_STATE_SIMPLE_ANIM || p_thing->SubState == SUB_STATE_SIMPLE_ANIM_OVER)

                            {
                                target_stationary = TRUE;
                            }
                        }
                    }
                }
            }

            break;

        case EWAY_SUBTYPE_CAMERA_TARGET_NEAR:

            // Search around for the nearest living thing.
            {
                UWORD look_index = THING_find_nearest(
                    ew_look->x,
                    ew_look->y,
                    ew_look->z,
                    0x500,
                    THING_FIND_LIVING);

                if (look_index) {
                    Thing* p_look = TO_THING(look_index);

                    look_x = p_look->WorldPos.X >> 8;
                    look_y = p_look->WorldPos.Y >> 8;
                    look_z = p_look->WorldPos.Z >> 8;

                    if (p_look->Class == CLASS_PERSON) {
                        EWAY_cam_warehouse = p_look->Genus.Person->Ware;

                        if (p_look->State == STATE_IDLE) {
                            target_stationary = TRUE;
                        }
                        if (p_look->State == STATE_MOVEING) {
                            if (p_look->SubState == SUB_STATE_SIMPLE_ANIM || p_look->SubState == SUB_STATE_SIMPLE_ANIM_OVER)

                            {
                                target_stationary = TRUE;
                            }
                        }
                    }
                } else {
                    // If there is nothing to look at nearby -- just look at the waypoint itself.
                    look_x = ew_look->x;
                    look_y = ew_look->y;
                    look_z = ew_look->z;
                    look_yaw = ew_look->yaw << 11;
                    target_stationary = TRUE;
                }
            }

            break;

        default:
            ASSERT(0);
            break;
        }
    }

    if (EWAY_cam_waypoint == NULL) {
        // No camera movement required.
    } else {
        ASSERT(WITHIN(EWAY_cam_waypoint, 1, EWAY_way_upto - 1));

        ew_go = &EWAY_way[EWAY_cam_waypoint];

        // The direction locked cameras look in.
        look_yaw = ew_go->yaw << 11;

        if (EWAY_stop_player_moving()) {
            if (!EWAY_cam_cant_interrupt && EWAY_cam_skip <= 0) {
                if (!EWAY_conv_ambient)
                    if (NET_PLAYER(0)->Genus.Player->Pressed & (INPUT_MASK_JUMP | INPUT_MASK_KICK | INPUT_MASK_ACTION | INPUT_MASK_PUNCH)) {
                        MFX_QUICK_stop();
                        // Mark all waypoints that this camera _would_ go to as arrived at.
                        while (1) {
                            // Mark this waypoint as active... if it is a CAMERA_WAYPOINT.
                            if (ew_go->ec.type == EWAY_COND_CAMERA_AT) {
                                ew_go->flag |= EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD;
                            }

                            next = EWAY_find_waypoint(EWAY_cam_waypoint + 1, EWAY_DO_CAMERA_WAYPOINT, ew_go->colour, ew_go->group, FALSE);

                            if (next == EWAY_NO_MATCH || next <= EWAY_cam_waypoint) {
                                // The search wrapped -- there are no more waypoints for
                                // the camera to use. Go inactive NEXT GAME TURN!
                                EWAY_cam_goinactive = 2; // TRUE;

                                return;
                            }

                            EWAY_cam_waypoint = next;

                            ASSERT(WITHIN(EWAY_cam_waypoint, 1, EWAY_way_upto - 1));

                            ew_go = &EWAY_way[EWAY_cam_waypoint];
                        }
                    }
            } else {
                // Pressing the key twice skips!
                if (!EWAY_conv_ambient)
                    if (NET_PLAYER(0)->Genus.Player->Pressed & (INPUT_MASK_JUMP | INPUT_MASK_KICK | INPUT_MASK_ACTION | INPUT_MASK_PUNCH)) {
                        EWAY_cam_skip = 0;
                    }
            }
        }

        // How far to our next waypoint?
        dx = ew_go->x - (EWAY_cam_x >> 8);
        dy = ew_go->y - (EWAY_cam_y >> 8);
        dz = ew_go->z - (EWAY_cam_z >> 8);

        dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

        if (dist < 0x80) {
            EWAY_cam_last_x = EWAY_cam_x;
            EWAY_cam_last_y = EWAY_cam_y;
            EWAY_cam_last_z = EWAY_cam_z;
            EWAY_cam_last_yaw = EWAY_cam_yaw;

            if (EWAY_cam_delay == 10 * 100) {
                // Maximum delay really means that it stays here while
                // the waypoint is active.
                if (ew_go->flag & EWAY_FLAG_ACTIVE) {
                    // Just stay here...
                } else {
                    // Move onto the next waypoint.
                    EWAY_cam_delay = 0;
                }
            } else {
                // Count down the delay.
                EWAY_cam_delay -= EWAY_tick;

                if (EWAY_cam_delay < 0) {
                    // Mark this waypoint as active... if it is a CAMERA_WAYPOINT.
                    if (ew_go->ec.type == EWAY_COND_CAMERA_AT) {
                        ew_go->flag |= EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD;
                    }

                    // Move onto the next waypoint.
                    next = EWAY_find_waypoint(EWAY_cam_waypoint + 1, EWAY_DO_CAMERA_WAYPOINT, ew_go->colour, ew_go->group, FALSE);

                    if (next == EWAY_NO_MATCH || next <= EWAY_cam_waypoint) {
                        // The search wrapped -- there are no more waypoints for
                        // the camera to use. Go inactive NEXT GAME TURN!
                        EWAY_cam_goinactive = 2; // TRUE;

                        // Make the camera look at Darci properly.
                        return;
                    }

                    ASSERT(WITHIN(next, 1, EWAY_way_upto - 1));

                    ew_next = &EWAY_way[next];

                    EWAY_cam_speed = ew_next->ed.arg1 << 8;
                    EWAY_cam_delay = ew_next->ed.arg2 * 10;

                    EWAY_cam_waypoint = next;
                }
            }
        }

        // Accelerate towards our next waypoint.
        // uc_orig: EWAY_CAM_ACCEL_SPEED (fallen/Source/eway.cpp)
#define EWAY_CAM_ACCEL_SPEED (EWAY_cam_speed >> 3)

        dx = (dx * EWAY_CAM_ACCEL_SPEED) / dist;
        dy = (dy * EWAY_CAM_ACCEL_SPEED) / dist;
        dz = (dz * EWAY_CAM_ACCEL_SPEED) / dist;

        EWAY_cam_dx += dx * TICK_RATIO >> TICK_SHIFT;
        EWAY_cam_dy += dy * TICK_RATIO >> TICK_SHIFT;
        EWAY_cam_dz += dz * TICK_RATIO >> TICK_SHIFT;

        // Keep our speed at what we want.
        speed = QDIST3(abs(EWAY_cam_dx), abs(EWAY_cam_dy), abs(EWAY_cam_dz)) + 1;

        if (dist >= 0x80) {
            wspeed = EWAY_cam_speed;
        } else {
            wspeed = (dist * EWAY_cam_speed) / 0x80;
        }

        if (speed > wspeed) {
            EWAY_cam_dx = (EWAY_cam_dx * wspeed) / speed;
            EWAY_cam_dy = (EWAY_cam_dy * wspeed) / speed;
            EWAY_cam_dz = (EWAY_cam_dz * wspeed) / speed;
        }

        // Move the camera.
        EWAY_cam_x += EWAY_cam_dx * TICK_RATIO >> TICK_SHIFT;
        EWAY_cam_y += EWAY_cam_dy * TICK_RATIO >> TICK_SHIFT;
        EWAY_cam_z += EWAY_cam_dz * TICK_RATIO >> TICK_SHIFT;
    }

    // Look at our target.
    dx = look_x - (EWAY_cam_x >> 8) >> 1;
    dy = look_y - (EWAY_cam_y >> 8) >> 1;
    dz = look_z - (EWAY_cam_z >> 8) >> 1;

    SLONG dxz = QDIST2(abs(dx), abs(dz));

    // Look at the right part of the thing.
    EWAY_cam_want_yaw = -Arctan(dx, dz);
    EWAY_cam_want_pitch = Arctan(dy, dxz);

    EWAY_cam_want_yaw &= 2047;
    EWAY_cam_want_pitch &= 2047;

    EWAY_cam_want_yaw <<= 8;
    EWAY_cam_want_pitch <<= 8;

    SLONG dyaw = EWAY_cam_want_yaw - EWAY_cam_yaw;
    SLONG dpitch = EWAY_cam_want_pitch - EWAY_cam_pitch;

    dyaw &= (2048 << 8) - 1;
    dpitch &= (2048 << 8) - 1;

    if (dyaw > (1024 << 8)) {
        dyaw -= (2048 << 8);
    }
    if (dpitch > (1024 << 8)) {
        dpitch -= (2048 << 8);
    }

    if (EWAY_cam_lock) {
        if (EWAY_cam_lock != INFINITY) {
            EWAY_cam_pitch = EWAY_cam_want_pitch;

            EWAY_cam_lock = INFINITY;
        }

        dyaw = look_yaw - EWAY_cam_yaw;
        dyaw &= (2048 << 8) - 1;

        if (dyaw > (1024 << 8)) {
            dyaw -= 2048 << 8;
        }

        dyaw = (EWAY_cam_last_dyaw * 3) + dyaw >> 2;

        EWAY_cam_yaw += dyaw >> 5;
        EWAY_cam_last_dyaw = dyaw;

    } else {
        SLONG max_dangle = 200 << 8;

        if (target_stationary) {
            max_dangle = 100 << 8;
        }

        if (abs(dyaw) > max_dangle || abs(dpitch) > max_dangle) {
            /*
                                    if(dyaw>0)
                                    {
                                            EWAY_cam_yaw   += MAX(dyaw   >> 2,dyaw-max_dangle);
                                    }
                                    else
                                    {
                                            EWAY_cam_yaw   -= MAX(-dyaw   >> 2,-dyaw-max_dangle);
                                    }
                                    if(dpitch>0)
                                    {
                                            EWAY_cam_pitch   += MAX(dpitch   >> 2,dpitch-max_dangle);
                                    }
                                    else
                                    {
                                            EWAY_cam_pitch   -= MAX(-dpitch   >> 2,-dpitch-max_dangle);
                                    }
            */

            EWAY_cam_yaw = EWAY_cam_want_yaw;
            EWAY_cam_pitch = EWAY_cam_want_pitch;
        }

        else

        {
            EWAY_cam_yaw += dyaw >> 1;
            EWAY_cam_pitch += dpitch >> 1;
        }
    }
}
