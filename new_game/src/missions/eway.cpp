#include <ctype.h>
#include "feature_flags.h"
#include "missions/eway.h"
#include "engine/input/input_frame.h"

#include "things/characters/person.h" // can_a_see_b, set_person_idle and other person functions used in this file
#include "map/ob.h"
#include "engine/core/fmatrix.h"
#include "world_objects/tripwire.h"
#include "world_objects/dirt.h"
#include "things/core/thing.h"
#include "engine/audio/music.h"
#include "engine/graphics/text/font2d.h"
#include "engine/console/console.h"
#include "ui/hud/eng_map.h"
#include "things/vehicles/chopper.h"
#include "things/animals/animal.h"
#include "ai/pcom.h"
#include "map/supermap.h"
#include "engine/audio/sound.h"
#include "things/core/statedef.h"
#include "navigation/wmove.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/render_interp.h"
#include "ui/hud/panel.h"
#include "ui/hud/panel_globals.h"
#include "engine/audio/mfx.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "buildings/ware.h"
#include "buildings/ware_globals.h"
#include "world_objects/door.h"
#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "engine/effects/psystem.h"
#include "engine/graphics/pipeline/poly.h"
#include "missions/playcuts.h"
#include "missions/playcuts_globals.h"
#include "assets/xlat_str.h"
#include "effects/weather/mist.h"
#include "ui/menus/gamemenu.h"
#include "buildings/prim_types.h" // PrimInfo
#include "buildings/prim.h" // find_anim_prim
#include "things/vehicles/vehicle.h"
#include "engine/physics/collide.h"
#include "engine/physics/collide_globals.h"
#include "effects/combat/pyro.h"
#include "things/items/barrel.h"
#include "world_objects/plat.h"

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

    while (ELEV_fname_level[c0] != '\\' && ELEV_fname_level[c0] != '/' && c0 < 101) {
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

// Returns UC_TRUE if the current level is one of the combat tutorial levels.
// uc_orig: playing_combat_tutorial (fallen/Source/eway.cpp)
SLONG playing_combat_tutorial(void)
{

    SLONG c0 = 0;

    while (ELEV_fname_level[c0] != '\\' && ELEV_fname_level[c0] != '/' && c0 < 101) {
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

// Returns UC_TRUE if the currently loaded level filename matches 'name'.
// uc_orig: playing_level (fallen/Source/eway.cpp)
SLONG playing_level(const CBYTE* name)
{
    SLONG c0 = 0;

    while (ELEV_fname_level[c0] != '\\' && ELEV_fname_level[c0] != '/' && c0 < 101) {
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

// Returns UC_TRUE if the current level is a real scored mission (not tutorial/fight/drive).
// uc_orig: playing_real_mission (fallen/Source/eway.cpp)
SLONG playing_real_mission(void)
{

    SLONG c0 = 0, c1 = 0;

    while (ELEV_fname_level[c0] != '\\' && ELEV_fname_level[c0] != '/' && c0 < 101) {
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
    sprintf(str, "./talk2/%s.ucm%d.wav", level, waypoint);

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
    sprintf(str, "./talk2/%s.ucm%d%c.wav", level, waypoint, 65 + conversation);

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

// Wall-clock timestamp (sdl3_get_ticks ms) of the most recent physics tick where
// MFX_QUICK_still_playing() was true. Updated once per EWAY_process tick. Used by
// the cutscene MESSAGE-deferral logic to add a fixed silent tail after a voice
// line before the next MESSAGE may activate. Sentinel 0 = never seen.
static uint64_t s_eway_voice_last_seen_ms = 0;

void EWAY_reset_cutscene_voice_tail(void)
{
    s_eway_voice_last_seen_ms = 0;

    // For a voiced conversation line, drop the read-time minimum timer to zero
    // on resume. The text was already on screen long enough during the pause
    // (wall-clock didn't stop), so what gates the advance now is purely the
    // voice line: still playing → wait for it to finish + the silence tail;
    // already finished → advance on the next tick (tail timestamp was cleared
    // above, so the post-voice-tail window doesn't fire stale either). Leaves
    // silent lines (no wav, EWAY_conv_talk == 0) alone — those still need
    // their timer to expire normally to give the player time to read.
    if (EWAY_conv_active && EWAY_conv_talk != 0) {
        EWAY_conv_timer = 0;
    }
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

    EWAY_cam_active = UC_FALSE;
    EWAY_conv_active = UC_FALSE;

    // Defensive: clear the cutscene voice-tail tracker so a stale timestamp
    // from the previous level can't shave time off the next level's first
    // cutscene tick. EWAY_conv_active was just set to FALSE so the timer
    // branch inside this helper is a no-op here.
    EWAY_reset_cutscene_voice_tail();

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
    UWORD p_index = 0;

    switch (subtype) {
    case EWAY_SUBTYPE_ANIMAL_BAT:
        if (g_feature_flags.cut_bats) {
            p_index = BAT_create(
                BAT_TYPE_BAT,
                world_x,
                world_z,
                yaw << 3);
        }
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
        p_index = 0;
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
            oi = OB_find_index(world_x, world_y, world_z, 2048, UC_FALSE);
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
    if (ew->ed.type == EWAY_DO_CREATE_PLAYER || ew->ed.type == EWAY_DO_CREATE_ANIMAL || ew->ed.type == EWAY_DO_CREATE_ENEMY || ew->ed.type == EWAY_DO_CAMERA_TARGET) {
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
// Returns UC_TRUE on success; UC_FALSE if out-of-range or buffer full.
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
// Returns UC_TRUE if the file was found.
// uc_orig: EWAY_load_message_file (fallen/Source/eway.cpp)
SLONG EWAY_load_message_file(CBYTE* fname, UWORD* index, UWORD* number)
{
    FILE* handle = MF_Fopen(fname, "rb");

    // Log missing text files — e.g. guilty.txt doesn't exist in game resources,
    // gang mission stubs may reference non-existent files. Helps diagnose loading issues.
    if (!handle) {
        fprintf(stderr, "WARNING: EWAY_load_message_file: cannot open '%s'\n", fname);
        return UC_FALSE;
    }

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

    return UC_TRUE;
}

// Loads ambient NPC dialogue pools from text files. Handles localisation.
// Called after all EWAY_set_message() calls are done.
// uc_orig: EWAY_load_fake_wander_text (fallen/Source/eway.cpp)
void EWAY_load_fake_wander_text(CBYTE* fname)
{
    CBYTE name_buffer[_MAX_PATH];

    EWAY_fake_wander_text_normal_index = NULL;
    EWAY_fake_wander_text_normal_number = 0;
    EWAY_fake_wander_text_guilty_index = NULL;
    EWAY_fake_wander_text_guilty_number = 0;
    EWAY_fake_wander_text_annoyed_index = NULL;
    EWAY_fake_wander_text_annoyed_number = 0;

    XLAT_str(X_THIS_LANGUAGE_IS, name_buffer);

    if (!oc_stricmp(name_buffer, "Italian")) {
        fname = "text/citsez_ita.txt";
    }
    if (!oc_stricmp(name_buffer, "French")) {
        fname = "text/citsez_french.txt";
    }
    if (!oc_stricmp(name_buffer, "Deutsch")) {
        fname = "text/citsez_german.txt";
    }
    if (!oc_stricmp(name_buffer, "Spanish")) {
        fname = "text/citsez_spa.txt";
    }

    if (fname == NULL) {
        fname = "text/citsez.txt";
    }

    if (!EWAY_load_message_file(
            fname,
            &EWAY_fake_wander_text_normal_index,
            &EWAY_fake_wander_text_normal_number)) {
        EWAY_load_message_file(
            "text/citsez.txt",
            &EWAY_fake_wander_text_normal_index,
            &EWAY_fake_wander_text_normal_number);
    }

    EWAY_load_message_file(
        "text/guilty.txt",
        &EWAY_fake_wander_text_guilty_index,
        &EWAY_fake_wander_text_guilty_number);

    EWAY_load_message_file(
        "text/annoyed.txt",
        &EWAY_fake_wander_text_annoyed_index,
        &EWAY_fake_wander_text_annoyed_number);

    ASSERT(EWAY_fake_wander_text_annoyed_index + EWAY_fake_wander_text_annoyed_number <= EWAY_mess_upto);
    ASSERT(EWAY_fake_wander_text_guilty_index + EWAY_fake_wander_text_guilty_number <= EWAY_mess_upto);
    ASSERT(EWAY_fake_wander_text_normal_index + EWAY_fake_wander_text_normal_number <= EWAY_mess_upto);

    // normal_number == 0 is expected on gang missions (e.g. Rat Catch) where
    // the wander text file is a stub with no dialogue lines. Original ASSERT
    // was empty in release — game continued without NPC ambient dialogue.
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
        this_is_english = !oc_stricmp(XLAT_str(X_THIS_LANGUAGE_IS), "English");
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
        ASSERT(UC_FALSE);
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
    EWAY_hud_countdown_value = -1;

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
// Returns UC_TRUE if the condition is satisfied. ec->negate inverts the result.
// EWAY_sub_condition_of_a_boolean=UC_TRUE skips post-evaluation guards (caller is a BOOL_AND/OR).
// uc_orig: EWAY_evaluate_condition (fallen/Source/eway.cpp)
SLONG EWAY_evaluate_condition(EWAY_Way* ew, EWAY_Cond* ec, SLONG EWAY_sub_condition_of_a_boolean)
{
    SLONG ans = UC_FALSE;

    switch (ec->type) {
    case EWAY_COND_FALSE:
        ans = UC_FALSE;
        break;

    case EWAY_COND_TRUE:
        ans = UC_TRUE;
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
            ans = UC_FALSE;
        }
    }

    break;

    case EWAY_COND_TRIPWIRE:
        ans = TRIP_activated(ec->arg1);
        break;

    case EWAY_COND_PRESSURE:
        ans = UC_FALSE; // Pressure plates never fire in this codebase (always UC_FALSE).
        break;

    case EWAY_COND_CAMERA:
        ans = UC_FALSE; // Always UC_FALSE here; camera-at condition is satisfied externally
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
            ans = UC_FALSE;

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

        ans = EWAY_evaluate_condition(ew, ec1, UC_TRUE) && EWAY_evaluate_condition(ew, ec2, UC_TRUE);
    }

    break;

    case EWAY_COND_BOOL_OR:

    {
        EWAY_Cond* ec1 = &EWAY_cond[ec->arg1];
        EWAY_Cond* ec2 = &EWAY_cond[ec->arg2];

        ans = EWAY_evaluate_condition(ew, ec1, UC_TRUE) || EWAY_evaluate_condition(ew, ec2, UC_TRUE);
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

        ASSERT(depend == 0 || WITHIN(depend, 1, EWAY_way_upto - 1));

        if (!depend || (EWAY_way[depend].flag & EWAY_FLAG_ACTIVE)) {
            if (EWAY_conv_active) {
                // Timers stop during conversations.
            } else {
                // Tick down the timer.
                if (ec->arg2 <= EWAY_tick) {
                    ec->arg2 = 0;
                    ans = UC_TRUE;
                } else {
                    ec->arg2 -= EWAY_tick;
                    ans = UC_FALSE;
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
                // Persist value for the render path. -1 when done so the HUD clears.
                EWAY_hud_countdown_value = ans ? -1 : ec->arg2;
            }
        } else {
            // Not counting down.
            ans = UC_FALSE;
        }
    }

    break;

    case EWAY_COND_PERSON_DEAD:

        ans = UC_FALSE; // By default.

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
                                ans = UC_FALSE;
                            }
                        }
                    }

                    break;

                case EWAY_DO_CREATE_ITEM:
                    ans = (ew_dead->flag & EWAY_FLAG_GOTITEM);
                    break;

                case EWAY_DO_CREATE_BARREL:

                {
                    ans = UC_FALSE;

                    if (ew_dead->flag & EWAY_FLAG_ACTIVE) {
                        if (ew_dead->ed.arg1 == NULL) {
                            // The barrel has been created and destroyed.
                            ans = UC_TRUE;
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
                ans = UC_FALSE; // Person hasn't been created yet.
            }
        }
    }

    break;

    case EWAY_COND_CAMERA_AT:

        // This condition is perversely set in EWAY_process_camera().
        ans = UC_FALSE;

        break;

    case EWAY_COND_PLAYER_CUBE:

    {
        Thing* darci = NET_PERSON(0);

        ans = UC_FALSE; // By default...

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
                    ans = UC_TRUE;
                }
            }
        }
    }

    break;

    case EWAY_COND_A_SEE_B:

        ans = UC_FALSE; // by default...

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
        ans = UC_FALSE;

        {
            SLONG i;
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
                                ans = UC_FALSE;

                                break;
                            } else {
                                // Found at least one dead person -- make the default UC_TRUE.
                                ans = UC_TRUE;
                            }
                        }
                    }
                }
                ew2++;
            }
        }

        break;

    case EWAY_COND_HALF_DEAD:

        ans = UC_FALSE;

        {
            SLONG i_person = EWAY_get_person(ec->arg1);

            if (i_person) {
                Thing* p_person = TO_THING(i_person);

                // Is this person half dead?
                if (WITHIN(p_person->Genus.Person->Health, 10, 100)) {
                    ans = UC_TRUE;
                }
            }
        }

        break;

    case EWAY_COND_ITEM_HELD:

        ans = UC_FALSE;

        // Is the player carrying our item?
        if (ec->arg1 == SPECIAL_GUN) {
            if (NET_PERSON(0)->Flags & FLAGS_HAS_GUN) {
                ans = UC_TRUE;
            }
        } else {
            if (person_has_special(
                    NET_PERSON(0),
                    ec->arg1)) {
                ans = UC_TRUE;
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

        ans = UC_FALSE;

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

        // Always return UC_FALSE; this waypoint will be set active in
        // do_an_action() in interfac.cpp.
        ans = UC_FALSE;
    }

    break;

    case EWAY_COND_PRIM_DAMAGED:

        ans = UC_FALSE;

        if (ec->arg1) {
            ASSERT(WITHIN(ec->arg1, 1, OB_ob_upto - 1));

            if (OB_ob[ec->arg1].flags & OB_FLAG_DAMAGED) {
                ans = UC_TRUE;
            }
        }

        break;

    case EWAY_COND_CONVERSE_END:

        ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

        ans = UC_FALSE;

        if (EWAY_way[ec->arg1].flag & EWAY_FLAG_FINISHED) {
            ans = UC_TRUE;
        }

        break;

    case EWAY_COND_COUNTER_GTEQ:

        ASSERT(WITHIN(ec->arg1, 0, EWAY_MAX_COUNTERS - 1));

        ans = UC_FALSE;

        if (EWAY_counter[ec->arg1] >= ec->arg2) {
            ans = UC_TRUE;
        }

        break;

    // True when the NPC has been handcuffed by Darci (player presses arrest button while
    // grappling). Requires FLAG_PERSON_ARRESTED AND SubState == SUB_STATE_DEAD_ARRESTED.
    case EWAY_COND_PERSON_ARRESTED:

        ans = UC_FALSE; // By default.

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
            ans = UC_FALSE;
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
                    ans = UC_TRUE;
                } else {
                    ans = UC_FALSE;
                }
            }
        }

        break;

    case EWAY_COND_KILLED_NOT_ARRESTED:

        ans = UC_FALSE; // By default.

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
                                ans = UC_TRUE;
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
        ans = UC_FALSE;

        UWORD person = EWAY_get_person(ec->arg1);

        if (person) {
            Thing* p_person = TO_THING(person);

            if (p_person->Genus.Person->Flags2 & FLAG2_PERSON_IS_MURDERER) {
                ans = UC_TRUE;
            }
        }
    }

    break;

    case EWAY_COND_PERSON_IN_VEHICLE:

        ans = UC_FALSE;

        {
            UWORD person;

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
                            ans = UC_TRUE;
                        }
                    } else {
                        ans = UC_TRUE;
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

        ans = UC_FALSE;

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
                            ans = UC_TRUE;
                        }
                    }
                }
            }
        }

        break;

    case EWAY_COND_SPECIFIC_ITEM_HELD:

        ans = UC_FALSE;

        {
            EWAY_Way* ew_other;

            ASSERT(WITHIN(ec->arg1, 1, EWAY_way_upto - 1));

            ew_other = &EWAY_way[ec->arg1];

            ASSERT(ew_other->ed.type == EWAY_DO_CREATE_ITEM);

            if (ew_other->ed.arg2) {
                // Has this special been picked up by Darci?
                if (ew_other->flag & EWAY_FLAG_GOTITEM) {
                    ans = UC_TRUE;
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

        ans = UC_FALSE;

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

        ans = UC_FALSE;

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
                        ans = UC_TRUE;
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
            ans = UC_FALSE;
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
                        ans = UC_FALSE;
                    }
                }
            } else if (EWAY_cam_active) {
                // In scripted cutscenes (EWAY-driven camera) chained MESSAGE
                // waypoints would otherwise cut off the previous voice line
                // through EWAY_talk's MFX_QUICK_wait/stop sequence. Defer the
                // next MESSAGE until the current voice finishes plus a tail
                // of silence so the line lands cleanly instead of jamming
                // into the next sentence. Music is left running — the
                // EWAY_used_thing branch above stops music via MFX_QUICK_stop
                // but that actually stops the QUICK voice slot (the very voice
                // we want to preserve), so we don't mirror that behaviour here.
                // s_eway_voice_last_seen_ms is updated once per EWAY_process
                // tick (independent of which waypoint type is being evaluated)
                // so the timestamp is fresh even on the first MESSAGE check.
                extern uint64_t sdl3_get_ticks();
                if (s_eway_voice_last_seen_ms != 0
                    && (sdl3_get_ticks() - s_eway_voice_last_seen_ms) < MFX_CUTSCENE_VOICE_TAIL_MS) {
                    ans = UC_FALSE;
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
                        ans = UC_FALSE;
                    }
                }
            }
        }
    }

    if (ew->ed.type == EWAY_DO_CAMERA_CREATE && ans) {
        if (EWAY_stop_player_moving() && !EWAY_cam_goinactive) {
            ans = UC_FALSE;
        }
    }

    if ((ew->ed.type == EWAY_DO_CONVERSATION || ew->ed.type == EWAY_DO_AMBIENT_CONV) && ans) {
        Thing* darci = NET_PERSON(0);

        // Make sure that Darci isn't jumping or falling through the air!
        if (!person_ok_for_conversation(darci)) {
            ans = UC_FALSE;
        }

        // Make sure each person in the conversation is okay...
        UWORD person1 = EWAY_get_person(ew->ed.arg1);
        UWORD person2 = EWAY_get_person(ew->ed.arg2);

        if (person1 == NULL || person2 == NULL) {
            ans = UC_FALSE;
        } else {
            Thing* p_person1 = TO_THING(person1);
            Thing* p_person2 = TO_THING(person2);

            if (!person_ok_for_conversation(p_person1) || !person_ok_for_conversation(p_person2)) {
                ans = UC_FALSE;
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
                    ans = UC_FALSE;
                }
            }
        }
    }

    return ans;
}

// Initialises the scripted cut-scene camera from a CAMERA_CREATE waypoint.
// Sets EWAY_cam_active=UC_TRUE and populates all EWAY_cam_* globals.
// The camera starts at the waypoint position and looks for the first CAMERA_TARGET waypoint
// with the same colour/group. Movement is processed each tick by EWAY_process_camera().
// uc_orig: EWAY_create_camera (fallen/Source/eway.cpp)
void EWAY_create_camera(SLONG waypoint)
{
    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    EWAY_Way* ew;

    ew = &EWAY_way[waypoint];

    EWAY_cam_active = UC_TRUE;
    EWAY_cam_goinactive = UC_FALSE;
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
    EWAY_cam_target = EWAY_find_waypoint(1, EWAY_DO_CAMERA_TARGET, ew->colour, ew->group, UC_FALSE);

    if (EWAY_cam_target == EWAY_NO_MATCH) {
        // There is nothing for the camera to look at!
        CONSOLE_text("No target for camera", 8000);

        EWAY_cam_active = UC_FALSE;
    }

    MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SEARCH_END);
    MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SLIDE_START);
}

// Advances the scripted cut-scene camera each tick.
// Reads EWAY_cam_* globals set by EWAY_create_camera() and moves the camera through
// CAMERA_WAYPOINT nodes (waypoints with the same colour/group as the CAMERA_CREATE waypoint).
// Camera looks at a CAMERA_TARGET waypoint - can be a fixed position or track a Thing.
// When EWAY_cam_goinactive counts down to 0, sets EWAY_cam_active=UC_FALSE (end of cut-scene).
// Sets EWAY_FLAG_ACTIVE on CAMERA_AT conditions when the camera reaches each node.
// EWAY_cam_freeze=UC_TRUE locks player movement while the camera is running.
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

    SLONG target_stationary = UC_FALSE;

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
            EWAY_cam_active = UC_FALSE;
            EWAY_cam_goinactive = UC_FALSE;
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
                target_stationary = UC_TRUE;
            }
            if (p_thing->State == STATE_MOVEING) {
                if (p_thing->SubState == SUB_STATE_SIMPLE_ANIM || p_thing->SubState == SUB_STATE_SIMPLE_ANIM_OVER)

                {
                    target_stationary = UC_TRUE;
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
            target_stationary = UC_TRUE;
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
                            target_stationary = UC_TRUE;
                        }
                        if (p_thing->State == STATE_MOVEING) {
                            if (p_thing->SubState == SUB_STATE_SIMPLE_ANIM || p_thing->SubState == SUB_STATE_SIMPLE_ANIM_OVER)

                            {
                                target_stationary = UC_TRUE;
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
                            target_stationary = UC_TRUE;
                        }
                        if (p_look->State == STATE_MOVEING) {
                            if (p_look->SubState == SUB_STATE_SIMPLE_ANIM || p_look->SubState == SUB_STATE_SIMPLE_ANIM_OVER)

                            {
                                target_stationary = UC_TRUE;
                            }
                        }
                    }
                } else {
                    // If there is nothing to look at nearby -- just look at the waypoint itself.
                    look_x = ew_look->x;
                    look_y = ew_look->y;
                    look_z = ew_look->z;
                    look_yaw = ew_look->yaw << 11;
                    target_stationary = UC_TRUE;
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
                        // Clear post-voice tail so the skip is instant rather
                        // than waiting out the silence window from the voice
                        // we just killed.
                        s_eway_voice_last_seen_ms = 0;
                        // Mark all waypoints that this camera _would_ go to as arrived at.
                        while (1) {
                            // Mark this waypoint as active... if it is a CAMERA_WAYPOINT.
                            if (ew_go->ec.type == EWAY_COND_CAMERA_AT) {
                                ew_go->flag |= EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD;
                            }

                            next = EWAY_find_waypoint(EWAY_cam_waypoint + 1, EWAY_DO_CAMERA_WAYPOINT, ew_go->colour, ew_go->group, UC_FALSE);

                            if (next == EWAY_NO_MATCH || next <= EWAY_cam_waypoint) {
                                // The search wrapped -- there are no more waypoints for
                                // the camera to use. Go inactive NEXT GAME TURN!
                                EWAY_cam_goinactive = 2; // UC_TRUE;

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

                // Hold the camera on the current waypoint while a voice line is
                // still playing (or inside MFX_CUTSCENE_VOICE_TAIL_MS of trailing
                // silence). Without this, cam_delay can expire and switch the
                // camera to the next scene before the line lands — pairs with
                // the matching MESSAGE deferral in EWAY_evaluate_condition so
                // the whole "scene = camera + line" stays glued together. Only
                // applies inside an EWAY-driven cutscene; ambient gameplay
                // cameras never reach here without cam_active set.
                {
                    extern uint64_t sdl3_get_ticks();
                    if (s_eway_voice_last_seen_ms != 0
                        && (sdl3_get_ticks() - s_eway_voice_last_seen_ms) < MFX_CUTSCENE_VOICE_TAIL_MS) {
                        if (EWAY_cam_delay < 0) {
                            EWAY_cam_delay = 0;
                        }
                    }
                }

                if (EWAY_cam_delay < 0) {
                    // Mark this waypoint as active... if it is a CAMERA_WAYPOINT.
                    if (ew_go->ec.type == EWAY_COND_CAMERA_AT) {
                        ew_go->flag |= EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD;
                    }

                    // Move onto the next waypoint.
                    next = EWAY_find_waypoint(EWAY_cam_waypoint + 1, EWAY_DO_CAMERA_WAYPOINT, ew_go->colour, ew_go->group, UC_FALSE);

                    if (next == EWAY_NO_MATCH || next <= EWAY_cam_waypoint) {
                        // The search wrapped -- there are no more waypoints for
                        // the camera to use. Go inactive NEXT GAME TURN!
                        EWAY_cam_goinactive = 2; // UC_TRUE;

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
        if (EWAY_cam_lock != UC_INFINITY) {
            EWAY_cam_pitch = EWAY_cam_want_pitch;

            EWAY_cam_lock = UC_INFINITY;
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

// Ends the current two-person scripted conversation: clears EWAY_conv_active, marks the
// waypoint FINISHED, and stops both participants from talking to each other.
// Also relinquishes camera control for non-ambient conversations.
// uc_orig: EWAY_finish_conversation (fallen/Source/eway.cpp)
void EWAY_finish_conversation(void)
{
    EWAY_conv_active = UC_FALSE;

    if (!EWAY_conv_ambient) {
        EWAY_cam_relinquish();
    }

    ASSERT(WITHIN(EWAY_conv_waypoint, 1, EWAY_way_upto - 1));

    EWAY_way[EWAY_conv_waypoint].flag |= EWAY_FLAG_FINISHED;

    PCOM_stop_people_talking_to_eachother(
        TO_THING(EWAY_conv_person_a),
        TO_THING(EWAY_conv_person_b));

    // No header for these; declared in original source.
    extern THING_INDEX PANEL_wide_top_person;
    extern THING_INDEX PANEL_wide_bot_person;

    PANEL_wide_top_person = NULL;
    PANEL_wide_bot_person = NULL;
}

// Advances the active two-person scripted conversation one tick.
// Handles player-skip input, timer countdown, voice playback, subtitle display,
// and conversation abort when either participant's state changes unexpectedly.
// uc_orig: EWAY_process_conversation (fallen/Source/eway.cpp)
void EWAY_process_conversation(void)
{
    CBYTE* ch;
    CBYTE* str;

    check_eway_talk(0);
    if (!EWAY_conv_active) {
        return;
    }
    timer_bored = 0;

    EWAY_conv_timer -= EWAY_tick;
    EWAY_conv_skip -= EWAY_tick;

    if (EWAY_conv_skip <= 0) {
        if (!EWAY_conv_ambient) {
            if (NET_PLAYER(0)->Genus.Player->Pressed & (INPUT_MASK_JUMP | INPUT_MASK_KICK | INPUT_MASK_ACTION | INPUT_MASK_PUNCH)) {
                // Cut off the voice-overs immediately.
                MFX_QUICK_stop();

                EWAY_conv_timer = 0;
                // Clear post-voice tail so the skip is instant.
                s_eway_voice_last_seen_ms = 0;
            }
        }
    } else {
        if (!EWAY_conv_ambient)
            if (NET_PLAYER(0)->Genus.Player->Pressed & (INPUT_MASK_JUMP | INPUT_MASK_KICK | INPUT_MASK_ACTION | INPUT_MASK_PUNCH)) {
                // Two presses in a row skips.
                EWAY_conv_skip = 0;
            }
    }

    // Advance to the next conversation line only when the read-time minimum
    // (EWAY_conv_timer derived from string length) has elapsed AND, if a wave
    // was actually started, it has finished playing AND the post-voice silence
    // tail has elapsed. Original used OR (timer expiry was an independent
    // trigger), so a long voice line with a short text-derived timer would be
    // cut off mid-word. EWAY_conv_talk == 0 covers silent lines (no wav, play
    // failed) — those still advance by timer alone.
    extern uint64_t sdl3_get_ticks();
    bool wave_quiet = (EWAY_conv_talk == 0) || (MFX_QUICK_still_playing() == 0);
    bool tail_passed = (s_eway_voice_last_seen_ms == 0)
                       || ((sdl3_get_ticks() - s_eway_voice_last_seen_ms) >= MFX_CUTSCENE_VOICE_TAIL_MS);
    if (EWAY_conv_timer <= 0 && wave_quiet && tail_passed) {
        str = &EWAY_mess_buffer[EWAY_conv_str];

        // Reached the end of the conversation?
        if (*str == 0) {
            EWAY_finish_conversation();

            return;
        }

        EWAY_talk_conv((EWAY_way[EWAY_conv_waypoint].yaw << 8) + (EWAY_way[EWAY_conv_waypoint].index), EWAY_conv_str_count);

        // Build a null-terminated substring from the pipe-delimited message buffer.
        // Each '|' separator marks the next speaker's line. Timer scales with string length.
        for (ch = str, EWAY_conv_timer = 200; *ch != '\000' && *ch != '|'; ch++, EWAY_conv_str += 1, EWAY_conv_timer += 4)
            ;

        if (ch[0] == '|') {
            EWAY_conv_str += 1;
            EWAY_conv_str_count++;

            ch[0] = '\000';
        }

        EWAY_conv_skip = 50;

        PANEL_new_text(
            TO_THING(EWAY_conv_person_a),
            EWAY_conv_timer * 10 - 500,
            str);

        // Make our two people face and gesture at each other.
        PCOM_make_people_talk_to_eachother(
            TO_THING(EWAY_conv_person_a),
            TO_THING(EWAY_conv_person_b),
            ch[-1] == '?' || ch[-2] == '?',
            UC_TRUE);

        if (!EWAY_conv_ambient) {
            // Cut-scene camera follows the active speaker.
            EWAY_cam_converse(TO_THING(EWAY_conv_person_a), TO_THING(EWAY_conv_person_b));
        }

        // Swap who's talking for next line.
        SWAP(EWAY_conv_person_a, EWAY_conv_person_b);
        EWAY_cam_jumped = 10;
        // (render-interp uses delta-based cut detection on the EWAY camera
        // snapshot — see render_interp_capture_eway_camera. No explicit
        // teleport-mark needed here.)
    }

    // If either person gets attacked, knocked over, or otherwise disrupted, abort.
    {
        Thing* p_person_a = TO_THING(EWAY_conv_person_a);
        Thing* p_person_b = TO_THING(EWAY_conv_person_b);

        if ((p_person_a->SubState != SUB_STATE_SIMPLE_ANIM && p_person_a->SubState != SUB_STATE_SIMPLE_ANIM_OVER && p_person_a->State != STATE_IDLE) || (p_person_b->SubState != SUB_STATE_SIMPLE_ANIM && p_person_b->SubState != SUB_STATE_SIMPLE_ANIM_OVER && p_person_b->State != STATE_IDLE)) {
            EWAY_finish_conversation();

            return;
        }
    }
}

// Emits steam particles from an active EWAY_DO_EMIT_STEAM waypoint each game tick.
// Direction (forward/up/down) is controlled by the waypoint subtype; speed, steps,
// and range are packed into ed.arg2. Choreography bitmask selects which steps fire.
// uc_orig: EWAY_process_emit_steam (fallen/Source/eway.cpp)
void EWAY_process_emit_steam(EWAY_Way* ew)
{
    SLONG tick;

    SLONG speed;
    SLONG steps;
    SLONG range;
    SLONG choreography;

    ASSERT(ew->flag & EWAY_FLAG_ACTIVE);
    ASSERT(ew->ed.type == EWAY_DO_EMIT_STEAM);

    speed = (ew->ed.arg2 >> 10) & 0x3f;
    steps = (ew->ed.arg2 >> 6) & 0x0f;
    range = (ew->ed.arg2 >> 0) & 0x3f;

    tick = EWAY_time * speed >> 7;
    tick %= steps;

    choreography = ew->ed.arg1;

    if (choreography & (1 << tick)) {
        SLONG dx;
        SLONG dy;
        SLONG dz;

        switch (ew->ed.subtype) {
        case EWAY_SUBTYPE_STEAM_FORWARD:
            dx = -SIN(ew->yaw << 3) * range >> 8;
            dy = 0;
            dz = -COS(ew->yaw << 3) * range >> 8;
            break;

        case EWAY_SUBTYPE_STEAM_UP:
            dx = 0;
            dy = range << 8;
            dz = 0;
            break;

        case EWAY_SUBTYPE_STEAM_DOWN:
            dx = 0;
            dy = -range << 8;
            dz = 0;
            break;

        default:
            ASSERT(0);
            break;
        }

        dx += (Random() & 0xff) - 0x7f;
        dy += (Random() & 0xff) - 0x7f;
        dz += (Random() & 0xff) - 0x7f;

        PARTICLE_Add(
            ew->x + (Random() & 0x7) - 0x3 << 8,
            ew->y + (Random() & 0x7) - 0x3 << 8,
            ew->z + (Random() & 0x7) - 0x3 << 8,
            dx,
            dy,
            dz,
            POLY_PAGE_SMOKECLOUD2,
            2 + ((Random() & 0x3) << 2),
            0x7fe8ffd0,
            PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE | PFLAG_HURTPEOPLE,
            100,
            40,
            1,
            1,
            1);
    }
}

// Fires a waypoint's EWAY_DO_* action when its condition becomes true.
// Sets EWAY_FLAG_ACTIVE, dispatches on ed.type for all mission script actions
// (entity spawning, doors, explosions, messages, nav beacons, group control, etc.),
// then configures the EWAY_STAY_* timer/lifetime behaviour.
// uc_orig: EWAY_set_active (fallen/Source/eway.cpp)
void EWAY_set_active(EWAY_Way* ew)
{
    SLONG has;

    ew->flag |= EWAY_FLAG_ACTIVE;

    switch (ew->ed.type) {
    case EWAY_DO_NOTHING:
        break;

    case EWAY_DO_CREATE_PLAYER:

        has = (ew->ed.subtype & 8) ? 0 : PCOM_HAS_GUN;

        ew->ed.arg1 = EWAY_create_player(
            ew->ed.subtype,
            ew->yaw,
            has,
            ew->x,
            ew->y,
            ew->z);
        break;

    case EWAY_DO_CREATE_ANIMAL:
        ew->ed.arg1 = EWAY_create_animal(
            ew->ed.subtype,
            ew->yaw,
            ew->x,
            ew->y,
            ew->z);
        break;

    case EWAY_DO_CREATE_ENEMY: {
        EWAY_Edef* ee;

        ASSERT(WITHIN(ew->index, 1, EWAY_edef_upto - 1));

        ee = &EWAY_edef[ew->index];

        ew->ed.arg1 = EWAY_create_enemy(
            ew->ed.subtype,
            ew->yaw,
            ew->colour,
            ew->group,
            ee->pcom_ai,
            ee->pcom_bent,
            ee->pcom_move,
            ee->drop,
            ee->zone,
            ee->ai_skill,
            ee->follow,
            ee->ai_other,
            ee->pcom_has,
            ew->x,
            ew->y,
            ew->z,
            ew - EWAY_way);
    } break;

    case EWAY_DO_CREATE_ITEM:

        // Not in any header; declared in original source.
        extern void find_nice_place_near_person(
            Thing * p_person,
            SLONG * nice_x, // 8-bits per mapsquare
            SLONG * nice_y,
            SLONG * nice_z);

        if ((ew->ed.arg1 & EWAY_ARG_ITEM_FOLLOW_PERSON) && ((ew->ec.type == EWAY_COND_KILLED_NOT_ARRESTED) || (ew->ec.type == EWAY_COND_PERSON_ARRESTED) || (ew->ec.type == EWAY_COND_PERSON_DEAD) || (ew->ec.type == EWAY_COND_HALF_DEAD) || (ew->ec.type = EWAY_COND_PERSON_USED))) {
            ASSERT(ew->ec.arg1);

            if (ew->ec.arg1) {
                SLONG person = EWAY_get_person(ew->ec.arg1);

                ASSERT(person);

                if (person) {
                    SLONG item_x;
                    SLONG item_y;
                    SLONG item_z;

                    Thing* p_bloke = TO_THING(person);

                    find_nice_place_near_person(
                        p_bloke,
                        &item_x,
                        &item_y,
                        &item_z);

                    ew->x = item_x;
                    ew->y = item_y;
                    ew->z = item_z;
                }
            }
        }

        ew->ed.arg2 = EWAY_create_item(
            ew->ed.subtype,
            ew->x,
            ew->y,
            ew->z,
            ew);

        break;

    case EWAY_DO_CREATE_VEHICLE:
        ew->ed.arg1 = EWAY_create_vehicle(
            ew->ed.subtype,
            ew->ed.arg1,
            ew->ed.arg2,
            ew->x,
            ew->y,
            ew->z,
            ((ew->yaw << 3) + 1024) & 2047);

        break;
    case EWAY_DO_SOUND_ALARM:

        MFX_play_xyz(0, S_KICK_CAN, MFX_OVERLAP, ew->x << 8, ew->y << 8, ew->z << 8);

        PCOM_oscillate_tympanum(
            PCOM_SOUND_ALARM,
            NULL,
            ew->x,
            ew->y,
            ew->z);

        break;

    case EWAY_DO_SOUND_EFFECT:
        play_glue_wave(ew->ed.subtype, ew->ed.arg1, ew->x << 8, ew->y << 8, ew->z << 8);
        break;

    case EWAY_DO_CONTROL_DOOR:

        DOOR_open(
            ew->x,
            ew->z);

        break;

    case EWAY_DO_EXPLODE:

    {
        GameCoord posn;

        posn.X = ew->x << 8;
        posn.Y = ew->y << 8;
        posn.Z = ew->z << 8;

        PYRO_construct(
            posn,
            ew->ed.subtype,
            ew->ed.arg1);
    }

        if (ew->ed.subtype = !16) {

            // 16 is fire on its own...

            create_shockwave(
                ew->x,
                ew->y,
                ew->z,
                0x400,
                100,
                NULL);
        }

        break;

    case EWAY_DO_SPOT_FX:

        // water and similar stuff
        GameCoord posn;
        Thing* pyro;

        posn.X = ew->x << 8;
        posn.Y = ew->y << 8;
        posn.Z = ew->z << 8;
        pyro = PYRO_create(posn, PYRO_IRONICWATERFALL);
        if (pyro) {
            IRONICWATERFALL_state_function[STATE_INIT].StateFn(pyro);
            pyro->Genus.Pyro->radius = ew->ed.subtype;
            pyro->Genus.Pyro->scale = ew->ed.arg1;
        }
        break;

    case EWAY_DO_MESSAGE:

    {
        if (!WITHIN(ew->ed.arg1, 0, EWAY_MAX_MESSES - 1)) {
            CONSOLE_text("Too many messages for the waypoint system! Tell Mark!", 8000);
        } else {
            SLONG time = ew->ed.subtype;

            if (time) {
                time *= 1000;
            } else {
                time = 8000;
            }

            if (EWAY_mess[ew->ed.arg1] == NULL) {
            } else {
                if (EWAY_used_thing) {

                    talk_thing = TO_THING(EWAY_used_thing);
                    EWAY_talk((ew->yaw << 8) + ew->index);

                    PANEL_new_text(
                        TO_THING(EWAY_used_thing),
                        time,
                        EWAY_mess[ew->ed.arg1]);

                    if (person_ok_for_conversation(TO_THING(EWAY_used_thing))) {
                        PCOM_make_people_talk_to_eachother(
                            TO_THING(EWAY_used_thing),
                            NET_PERSON(0),
                            UC_FALSE,
                            UC_FALSE);
                    }
                } else {
                    if (ew->ed.arg2 == EWAY_MESSAGE_WHO_STREETNAME) {
                        PANEL_new_help_message(EWAY_mess[ew->ed.arg1]);
                    } else if (ew->ed.arg2 == EWAY_MESSAGE_WHO_TUTORIAL) {
                        EWAY_tutorial_string = EWAY_mess[ew->ed.arg1];
                        EWAY_tutorial_counter = 0;

                        MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SEARCH_END);
                        MFX_stop(THING_NUMBER(NET_PERSON(0)), S_SLIDE_START);

                        input_last_key_consume();
                    } else {
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
                            talk_thing = who_says;
                            EWAY_talk((ew->yaw << 8) + ew->index);

                            PANEL_new_text(
                                who_says,
                                time,
                                EWAY_mess[ew->ed.arg1]);

                            if (who_says && who_says != NET_PERSON(0)) {
                                if (person_ok_for_conversation(who_says)) {
                                    PCOM_make_people_talk_to_eachother(
                                        who_says,
                                        NET_PERSON(0),
                                        UC_FALSE,
                                        UC_FALSE,
                                        UC_FALSE);
                                }
                            }
                        }
                    }
                }
                if (ew->flag & EWAY_FLAG_WHY_LOST) {
                    // This is a message that comes up when you've lost the level.
                    GAMEMENU_set_level_lost_reason(EWAY_mess[ew->ed.arg1]);
                }
            }
        }
    }

    break;

    case EWAY_DO_NAV_BEACON:

    {
        UWORD track_thing = NULL;

        if (!WITHIN(ew->ed.arg1, 0, EWAY_MAX_MESSES - 1)) {
            CONSOLE_text("Too many navbeacon messages for the waypoint system! Tell Mark!");
        }

        track_thing = EWAY_get_person(ew->ed.arg2);

        if (ew->ed.arg2 && track_thing == 0) {
            // Person not created yet — try again next frame.
            ew->flag &= ~EWAY_FLAG_ACTIVE;

            return;

        } else {

            ew->ed.subtype = MAP_beacon_create(ew->x, ew->z, ew->ed.arg1, track_thing);
            ASSERT(ew->ed.subtype);

            if (GAME_TURN < 16) {
                MFX_play_stereo(0, S_MENU_END, MFX_REPLACE);
            }
        }
    }

    break;

    case EWAY_DO_ELECTRIFY_FENCE:

        if (ew->ed.arg1) {
            set_electric_fence_state(ew->ed.arg1, UC_TRUE);
        }

        break;

    case EWAY_DO_CAMERA_CREATE:
        EWAY_create_camera(ew - EWAY_way);
        if (EWAY_cam_freeze)
            MFX_play_stereo(0, S_CUTSCENE_STING, 0);
        break;

    case EWAY_DO_CAMERA_WAYPOINT:
        break;

    case EWAY_DO_CAMERA_TARGET:
        break;

    case EWAY_DO_MISSION_FAIL:
        if (GAME_STATE != GS_LEVEL_WON) {
            GAME_STATE = GS_LEVEL_LOST;
        }
        break;

    case EWAY_DO_MISSION_COMPLETE:
        if (GAME_STATE != GS_LEVEL_LOST) {
            // Make sure Darci doesn't die before the win screen shows.
            NET_PERSON(0)->Genus.Person->Flags2 |= FLAG2_PERSON_INVULNERABLE;

            GAME_STATE = GS_LEVEL_WON;
            // Not in any header; declared in original source.
            extern void set_stats(void);
            set_stats();
        }
        break;

    case EWAY_DO_CHANGE_ENEMY:

        ASSERT(WITHIN(ew->index, 1, EWAY_edef_upto - 1));

        {
            EWAY_Edef* ee = &EWAY_edef[ew->index];

            SLONG change = EWAY_get_person(ew->ed.arg1);

            if (change == NULL) {
            } else {
                Thing* p_change = TO_THING(change);

                PCOM_change_person_attributes(
                    p_change,
                    ew->colour,
                    ew->group,
                    ee->pcom_ai,
                    ee->ai_other,
                    ee->pcom_move,
                    ee->follow,
                    ee->pcom_bent,
                    ew->yaw << 3);
            }
        }

        break;

    case EWAY_DO_CHANGE_ENEMY_FLG: {
        SLONG change = EWAY_get_person(ew->ed.arg1);

        if (change == NULL) {
        } else {
            Thing* p_change = TO_THING(change);

            p_change->Genus.Person->pcom_bent = ew->ed.arg2;
            p_change->Genus.Person->pcom_zone = ew->ed.arg2 >> 8;
            // Not in any header; declared in original source.
            extern void PCOM_set_person_ai_normal(Thing * p_person);
            PCOM_set_person_ai_normal(p_change);
        }
    } break;

    case EWAY_DO_CREATE_PLATFORM:
        ew->ed.arg1 = PLAT_create(
            ew->colour,
            ew->group,
            PLAT_MOVE_PATROL,
            ew->ed.arg2,
            ew->ed.arg1,
            ew->x,
            ew->y,
            ew->z);
        break;
    case EWAY_DO_CREATE_BOMB:

        // The 'special' bomb monitors the waypoint that created it and
        // explodes when the waypoint goes active. Nothing to do at activation time.

        break;

    case EWAY_DO_WATER_SPOUT:

        // This waypoint spouts out water while active.
        // Particle emission is handled inside EWAY_process() per tick.

        break;

    case EWAY_DO_KILL_WAYPOINT:

        if (!(WITHIN(ew->ed.arg1, 1, EWAY_way_upto - 1))) {
        } else {
            EWAY_Way* ewk = &EWAY_way[ew->ed.arg1];

            ewk->flag |= EWAY_FLAG_DEAD;

            if (ewk->ec.type == EWAY_COND_COUNTDOWN_SEE) {
                music_mode_override = 0;
            }

            if (ewk->ed.type == EWAY_DO_CREATE_ENEMY) {
                if (ewk->ed.arg1) {
                    Thing* p_person = TO_THING(ewk->ed.arg1);

                    ASSERT(p_person->Class == CLASS_PERSON);

                    // Teleport the person out of the world ("holiday in another dimension").
                    p_person->Genus.Person->pcom_ai = PCOM_AI_NONE;
                    p_person->Genus.Person->pcom_bent = PCOM_BENT_ROBOT;

                    remove_thing_from_map(p_person);

                    p_person->WorldPos.X = 0x8000;
                    p_person->WorldPos.Z = 0x8000;

                    p_person->WorldPos.Y = PAP_calc_map_height_at(0x80, 0x80);

                    // Tell render-interp not to lerp from the previous
                    // WorldPos to (0x8000, _, 0x8000) — the actor was just
                    // teleported off-stage; without this the next render
                    // frame draws the body sliding from its last on-stage
                    // position to the corner of the map.
                    render_interp_mark_teleport(p_person);

                    set_person_idle(p_person);

                    // As if this person has never been created.
                    ewk->ed.arg1 = NULL;
                }
            } else if (ewk->ed.type == EWAY_DO_CREATE_VEHICLE) {
                if (ewk->ed.arg1) {
                    Thing* p_vehicle = TO_THING(ewk->ed.arg1);

                    if (p_vehicle->Class == CLASS_VEHICLE) {
                        // Make the vehicle blow up.
                        // Not in any header; declared in original source.
                        extern void VEH_reduce_health(Thing * p_car, Thing * p_person, SLONG damage);

                        VEH_reduce_health(
                            p_vehicle,
                            NULL,
                            1050);
                        extern UBYTE hit_player;
                        hit_player = 1;
                        create_shockwave(
                            p_vehicle->WorldPos.X >> 8,
                            p_vehicle->WorldPos.Y >> 8,
                            p_vehicle->WorldPos.Z >> 8,
                            0x300,
                            350,
                            NULL, 1);
                        hit_player = 0;

                        // As if this vehicle has never been created.
                        ewk->ed.arg1 = NULL;
                    }
                }
            } else if (ewk->ed.type == EWAY_DO_NAV_BEACON) {
                if (ewk->ed.subtype) {
                    MAP_beacon_remove(ewk->ed.subtype);

                    ewk->ed.subtype = NULL;
                }
            } else if (ewk->ed.type == EWAY_DO_ELECTRIFY_FENCE) {
                // Turn off the electric fence.
                if (ew->ed.arg1) {
                    set_electric_fence_state(ew->ed.arg1, UC_FALSE);
                }
            } else if (ewk->ed.type == EWAY_DO_MAKE_PERSON_PEE) {
                SLONG person = EWAY_get_person(ewk->ed.arg1);

                if (person) {
                    Thing* p_person = TO_THING(person);

                    ASSERT(p_person->Class == CLASS_PERSON);

                    p_person->Genus.Person->Flags &= ~FLAG_PERSON_PEEING;
                }
            } else if (ewk->ec.type == EWAY_COND_TRIPWIRE) {
                if (ewk->ec.arg1) {
                    TRIP_deactivate(ewk->ec.arg1);
                }
            } else if (ewk->ed.type == EWAY_DO_CREATE_BARREL) {
                // Make the barrel disappear.
                if (ewk->ed.arg1) {
                    Thing* p_barrel = TO_THING(ewk->ed.arg1);

                    if (p_barrel->Class == CLASS_BARREL) {
                        // Not in any header; declared in original source.
                        void BARREL_dissapear(Thing * p_barrel);

                        BARREL_dissapear(p_barrel);
                    }

                    ewk->ed.arg1 = 0;
                }
            }
        }

        break;

    case EWAY_DO_OBJECTIVE:
        // CRIME_RATE reduction. arg2 = points/10 from editor.
        // NOTE: Never actually reached at runtime — WPT_BONUS_POINTS is translated
        // to EWAY_DO_MESSAGE via a dead if(0) block in elev.cpp. Legacy code.
        {
            SLONG percent = ew->ed.arg2 * CRIME_RATE_SCORE_MUL >> 8;

            CRIME_RATE -= percent;

            SATURATE(CRIME_RATE, 0, 100);
        }

        break;

    case EWAY_DO_GROUP_LIFE:
        // Clears EWAY_FLAG_DEAD on all waypoints sharing same colour AND group.
        // Revives previously killed script branches. GROUP_LIFE/GROUP_DEATH waypoints are immune.
        {
            SLONG i;

            for (i = 1; i < EWAY_way_upto; i++) {
                if (EWAY_way[i].group == ew->group && EWAY_way[i].colour == ew->colour) {
                    if (EWAY_way[i].ed.type == EWAY_DO_GROUP_LIFE || EWAY_way[i].ed.type == EWAY_DO_GROUP_DEATH) {
                        // These waypoints are immune.
                    } else {
                        EWAY_way[i].flag &= ~EWAY_FLAG_DEAD;
                    }
                }
            }
        }

        break;

    case EWAY_DO_GROUP_DEATH:
        // Sets EWAY_FLAG_DEAD on all waypoints sharing same colour AND group.
        // Permanently disables the branch until a GROUP_LIFE revives it. GROUP_LIFE/DEATH immune.
        {
            SLONG i;

            for (i = 1; i < EWAY_way_upto; i++) {
                if (EWAY_way[i].group == ew->group && EWAY_way[i].colour == ew->colour) {
                    if (EWAY_way[i].ed.type == EWAY_DO_GROUP_LIFE || EWAY_way[i].ed.type == EWAY_DO_GROUP_DEATH) {
                        // These waypoints are immune.
                    } else {
                        EWAY_way[i].flag |= EWAY_FLAG_DEAD;
                    }
                }
            }
        }

        break;

    case EWAY_DO_CONVERSATION:
    case EWAY_DO_AMBIENT_CONV:

    {
        UWORD person_a = EWAY_get_person(ew->ed.arg1);
        UWORD person_b = EWAY_get_person(ew->ed.arg2);

        if (person_a && person_b) {
            if (person_ok_for_conversation(TO_THING(person_a)) && person_ok_for_conversation(TO_THING(person_b))) {
                if (EWAY_conv_active) {
                    // End the current conversation before starting a new one.
                    EWAY_finish_conversation();
                }

                // Make sure the people aren't too close together before they start talking.
                {
                    Thing* p_person_a = TO_THING(person_a);
                    Thing* p_person_b = TO_THING(person_b);

                    // Not in any header; declared in original source.
                    extern void push_people_apart(Thing * p_person, Thing * p_avoid);

                    push_people_apart(
                        p_person_a,
                        p_person_b);
                }

                // Start the conversation.
                EWAY_conv_active = UC_TRUE;
                EWAY_conv_waypoint = ew - EWAY_way;
                EWAY_conv_person_a = person_a;
                EWAY_conv_person_b = person_b;
                EWAY_conv_timer = 0;
                EWAY_conv_str_count = 0;
                EWAY_conv_skip = 100;
                EWAY_conv_ambient = (ew->ed.type == EWAY_DO_AMBIENT_CONV);

                if (!WITHIN(ew->ed.subtype, 0, EWAY_MAX_MESSES - 1)) {
                    CONSOLE_text("Too many messages for the waypoint system! Tell Mark!", 8000);
                } else {
                    if (EWAY_mess[ew->ed.subtype] == NULL) {
                        EWAY_conv_str = 0;
                    } else {
                        EWAY_conv_str = EWAY_mess[ew->ed.subtype] - EWAY_mess_buffer;
                    }
                }
            }
        }
    }

    break;

    case EWAY_DO_INCREASE_COUNTER:
        // Increments EWAY_counter[subtype] by 1. EWAY_COND_COUNTER_GTEQ checks >=.
        ASSERT(WITHIN(ew->ed.subtype, 0, EWAY_MAX_COUNTERS - 1));

        EWAY_counter[ew->ed.subtype] += 1;

        break;

    case EWAY_DO_EMIT_STEAM:

        // Steam emission is handled per-tick in EWAY_process_emit_steam() while active.

        break;

    case EWAY_DO_TRANSFER_PLAYER:

    {
        THING_INDEX i_player = EWAY_get_person(ew->ed.arg1);

        if (i_player == NULL) {
        } else {
            Thing* p_person = TO_THING(i_player);

            // Stop the old player being a player.
            NET_PERSON(0)->Genus.Person->PlayerID = 0;
            NET_PERSON(0)->Genus.Person->pcom_ai = PCOM_AI_CIV;
            NET_PERSON(0)->Genus.Person->pcom_bent = PCOM_BENT_ROBOT | PCOM_BENT_FIGHT_BACK;
            NET_PERSON(0)->Genus.Person->Flags2 |= FLAG2_PERSON_INVULNERABLE;
            NET_PERSON(0)->Genus.Person->HomeX = NET_PERSON(0)->WorldPos.X >> 8;
            NET_PERSON(0)->Genus.Person->HomeZ = NET_PERSON(0)->WorldPos.Z >> 8;
            NET_PERSON(0)->Genus.Person->HomeYaw = NET_PERSON(0)->Draw.Tweened->Angle >> 3;

            SET_SKILL(NET_PERSON(0), 15);

            set_person_idle(NET_PERSON(0));

            // Make the new person become the player.
            NET_PERSON(0) = p_person;
            p_person->Genus.Person->PlayerID = 1;
            NET_PLAYER(0)->Genus.Player->PlayerPerson = p_person;
            NET_PERSON(0)->Genus.Person->pcom_ai = PCOM_AI_NONE;
            NET_PERSON(0)->Genus.Person->Flags2 &= ~FLAG2_PERSON_INVULNERABLE;
            NET_PERSON(0)->Genus.Person->pcom_bent &= ~PCOM_BENT_PLAYERKILL;

            SET_SKILL(NET_PERSON(0), 0);

            FC_look_at(0, i_player);
            FC_setup_initial_camera(0);

            if (NET_PERSON(0)->State == STATE_MOVEING) {
                if (NET_PERSON(0)->SubState == SUB_STATE_SIMPLE_ANIM || NET_PERSON(0)->SubState == SUB_STATE_SIMPLE_ANIM_OVER) {
                    set_person_idle(NET_PERSON(0));
                }
            }
        }
    } break;

    case EWAY_DO_AUTOSAVE: {

        // Autosave placeholder — not implemented in this codebase.

    } break;

    case EWAY_DO_CREATE_BARREL:

        ew->ed.arg1 = BARREL_alloc(
            ew->ed.subtype,
            ew->ed.arg2,
            ew->x,
            ew->z,
            ew - EWAY_way);

        break;

    case EWAY_DO_LOCK_VEHICLE:

    {
        if (!WITHIN(ew->ed.arg1, 1, EWAY_way_upto - 1)) {
        } else {
            EWAY_Way* ewv = &EWAY_way[ew->ed.arg1];

            if (ewv->ed.arg1) {
                Thing* p_vehicle = TO_THING(ewv->ed.arg1);

                if (p_vehicle->Class != CLASS_VEHICLE) {
                } else {
                    switch (ew->ed.subtype) {
                    case EWAY_SUBTYPE_VEHICLE_LOCK:
                        p_vehicle->Genus.Vehicle->key = SPECIAL_NUM_TYPES; // Non-existent special.
                        break;

                    case EWAY_SUBTYPE_VEHICLE_UNLOCK:
                        p_vehicle->Genus.Vehicle->key = SPECIAL_NONE;
                        break;

                    default:
                        ASSERT(0);
                        break;
                    }
                }
            }
        }
    }

    break;

    case EWAY_DO_CUTSCENE:
        PLAYCUTS_Play(PLAYCUTS_cutscenes + ew->ed.arg1);
        break;

    case EWAY_DO_GROUP_RESET:

    {
        SLONG i;

        EWAY_Way* ewr;

        for (i = 1; i < EWAY_way_upto; i++) {
            ewr = &EWAY_way[i];

            if (ewr->colour == ew->colour && ewr->group == ew->group) {
                if (ewr->ed.type == EWAY_DO_GROUP_RESET) {
                    // Immune!
                } else {
                    if (ewr->flag & EWAY_FLAG_ACTIVE) {
                        // Forward decl needed here — EWAY_set_inactive is defined below in the same TU.
                        void EWAY_set_inactive(EWAY_Way * ew);

                        EWAY_set_inactive(ewr);
                    }

                    ewr->flag &= ~(EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD | EWAY_FLAG_FINISHED);
                }
            }
        }
    }

    break;

    case EWAY_DO_VISIBLE_COUNT_UP:

        // Timer for a visible on-screen count-up. The increment is in EWAY_process() while active.
        EWAY_count_up = 0;
        EWAY_count_up_add_penalties = UC_FALSE;
        EWAY_count_up_num_penalties = 0;
        EWAY_count_up_penalty_timer = 0;
        EWAY_counter[3] = 0;

        break;

    case EWAY_DO_RESET_COUNTER:

        if (ew->ed.subtype == 0) {
            UBYTE i;

            for (i = 0; i < EWAY_MAX_COUNTERS; i++) {
                EWAY_counter[i] = 0;
            }
        } else {
            ASSERT(WITHIN(ew->ed.subtype, 0, EWAY_MAX_COUNTERS - 1));

            EWAY_counter[ew->ed.subtype] = 0;
        }

        break;

    case EWAY_DO_CREATE_MIST: {
        static SLONG last_detail = 17;
        static SLONG last_height = 84;

#define MIST_SIZE 0x800

        SLONG x1 = (ew->x) - MIST_SIZE;
        SLONG z1 = (ew->z) - MIST_SIZE;
        SLONG x2 = (ew->x) + MIST_SIZE;
        SLONG z2 = (ew->z) + MIST_SIZE;

        MIST_create(
            last_detail,
            last_height,
            x1, z1,
            x2, z2);

        last_height += (last_height & 0x1) ? -11 : +11;
    } break;

    case EWAY_DO_STALL_CAR:

    {
        UWORD veh = EWAY_get_person(ew->ed.arg1);

        if (veh) {
            Thing* p_vehicle = TO_THING(veh);

            ASSERT(p_vehicle->Class == CLASS_VEHICLE);

            p_vehicle->Genus.Vehicle->Flags |= FLAG_VEH_STALLED;
        }
    }

    break;

    case EWAY_DO_EXTEND_COUNTDOWN:

        if (WITHIN(ew->ed.arg1, 1, EWAY_way_upto - 1)) {
            EWAY_Way* ew_other = &EWAY_way[ew->ed.arg1];

            if (ew_other->ec.type == EWAY_COND_COUNTDOWN_SEE || ew_other->ec.type == EWAY_COND_COUNTDOWN) {
                ew_other->ec.arg2 += ew->ed.arg2 * 100;
            }
        }

        break;

    case EWAY_DO_MOVE_THING:

    {
        UWORD i_thing = EWAY_get_person(ew->ed.arg1);

        if (i_thing) {
            Thing* p_thing = TO_THING(i_thing);

            if (p_thing->State == STATE_DEAD) {
                // Ignore — dead things don't move.
            } else {
                GameCoord newpos;

                newpos.X = ew->x << 8;
                newpos.Y = ew->y << 8;
                newpos.Z = ew->z << 8;

                if (p_thing->Flags & FLAGS_ON_MAPWHO) {
                    move_thing_on_map(p_thing, &newpos);
                } else {
                    p_thing->WorldPos = newpos;
                }

                // EWAY scene transitions teleport actors / vehicles between
                // scenes (entire-map jumps, ~millions of sub-units). Without
                // this hint the next render frame would lerp the model
                // smoothly across the whole map for one tick — visually a
                // huge "fly across the world" artifact each cinematic cut.
                // Covers both the on-map (move_thing_on_map) and off-map
                // (direct WorldPos assign) paths above. Person yaw is
                // overwritten below as well, so the same teleport mark
                // also collapses the angle interpolation discontinuity.
                render_interp_mark_teleport(p_thing);

                if (p_thing->Class == CLASS_PERSON) {
                    plant_feet(p_thing);
                    p_thing->Draw.Tweened->Angle = ew->yaw << 3;
                    p_thing->Genus.Person->HomeX = ew->x;
                    p_thing->Genus.Person->HomeZ = ew->z;
                    p_thing->Genus.Person->HomeYaw = ew->yaw;
                }
            }
        }
    }

    break;

    case EWAY_DO_MAKE_PERSON_PEE:

    {
        SLONG person = EWAY_get_person(ew->ed.arg1);

        if (person) {
            Thing* p_person = TO_THING(person);

            ASSERT(p_person->Class == CLASS_PERSON);

            p_person->Genus.Person->Flags |= FLAG_PERSON_PEEING;
        }
    }

    break;

    case EWAY_DO_CONE_PENALTIES:
        GAME_FLAGS |= GF_CONE_PENALTIES;
        break;

    case EWAY_DO_SIGN:
        PANEL_flash_sign(ew->ed.arg1, ew->ed.arg2);
        break;

    case EWAY_DO_WAREFX: {
        SLONG ware = WARE_which_contains(ew->x >> 8, ew->z >> 8);
        if (ware)
            WARE_ware[ware].ambience = ew->ed.arg1;
    } break;

    case EWAY_DO_NO_FLOOR:
        GAME_FLAGS |= GF_NO_FLOOR;
        break;

    case EWAY_DO_SHAKE_CAMERA:
        break;

    case EWAY_DO_END_OF_WORLD:
        PYRO_create(NET_PERSON(0)->WorldPos, PYRO_GAMEOVER);
        break;

    default:
        ASSERT(0);
        break;
    }

    switch (ew->es.type) {
    case EWAY_STAY_TIME:

        // Start the countdown to going inactive.
        {
            UBYTE i;

            for (i = 0; i < EWAY_MAX_TIMERS; i++) {
                if (EWAY_timer[i] == 0) {
                    EWAY_timer[i] = ew->es.arg * 10; // ew->es.arg is in tenths of a second.
                    ew->timer = i;
                    ew->flag |= EWAY_FLAG_COUNTDOWN;

                    break;
                }
            }
            if (i >= EWAY_MAX_TIMERS) {
                ASSERT(0);
            }
        }

        break;

    case EWAY_STAY_ALWAYS:

        // Waypoint fires once and never needs to be evaluated again.
        ew->flag |= EWAY_FLAG_DEAD;

        break;

    case EWAY_STAY_DIE:

        // Waypoint fires once, then goes inactive and is never re-evaluated.
        ew->flag |= EWAY_FLAG_DEAD;
        ew->flag &= ~EWAY_FLAG_ACTIVE;

        break;

    default:
        break;
    }
}

// Deactivates a waypoint: clears ACTIVE flag and runs any teardown actions
// (close door, turn off electric fence, remove nav beacon).
// uc_orig: EWAY_set_inactive (fallen/Source/eway.cpp)
void EWAY_set_inactive(EWAY_Way* ew)
{
    ew->flag &= ~EWAY_FLAG_ACTIVE;

    if (ew->ed.type == EWAY_DO_CONTROL_DOOR) {
        DOOR_shut(
            ew->x,
            ew->z);
    }

    if (ew->ed.type == EWAY_DO_ELECTRIFY_FENCE) {
        if (ew->ed.arg1) {
            set_electric_fence_state(ew->ed.arg1, UC_FALSE);
        }
    }

    if (ew->ed.type == EWAY_DO_NAV_BEACON) {
        MAP_beacon_remove(ew->ed.subtype);
    }
}

// Returns non-zero if the player's movement should be frozen
// (scripted camera is active and freezing, or a cut scene is playing).
// uc_orig: EWAY_stop_player_moving (fallen/Source/eway.cpp)
SLONG EWAY_stop_player_moving(void)
{
    return (EWAY_cam_active && EWAY_cam_freeze) || GAME_cut_scene;
}

// Animates the on-screen cone-penalty count-up after a driving mission timer goes inactive.
// Renders the accumulated time and increments it by one penalty per tick until all penalties added.
// uc_orig: EWAY_process_penalties (fallen/Source/eway.cpp)
void EWAY_process_penalties()
{
    if (!EWAY_count_up_add_penalties) {
        return;
    }

    EWAY_count_up_penalty_timer += EWAY_tick;

    if (EWAY_count_up_penalty_timer > 100) {
        CBYTE str[64];

        if (EWAY_count_up_num_penalties == 0) {
            strcpy(str, XLAT_str(X_NO_CONES_HIT));
        } else {
            sprintf(str, "%s = %d", XLAT_str(X_NUM_CONES_HIT), (EWAY_count_up_num_penalties > 0) ? EWAY_count_up_num_penalties : 0);
        }

        FONT2D_DrawStringCentred(
            str,
            320,
            100,
            0x00ff00);

        if (EWAY_count_up_penalty_timer >= 150) {
            if (EWAY_count_up_num_penalties == 0) {
                if (EWAY_count_up_penalty_timer >= 400) {
                    EWAY_count_up_add_penalties = UC_FALSE;
                }
            } else if (EWAY_count_up_num_penalties == -1) // -1 => We've finished adding up penalties
            {
                if (EWAY_count_up_penalty_timer >= 400) {
                    EWAY_count_up_add_penalties = UC_FALSE;
                }
            } else {
                while (EWAY_count_up_penalty_timer >= 200) {
                    EWAY_count_up_penalty_timer -= 10;
                    EWAY_count_up_num_penalties -= 1;
                    EWAY_count_up += 500; // 500 milliseconds per penalty.

                    if (EWAY_count_up_num_penalties == 0) {
                        EWAY_count_up_num_penalties = -1; // -1 => Finished adding up the penalties.
                    }
                }
            }
        }
    }
}

// Queues HUD mission timers for drawing. Called once per render frame (not per physics tick)
// so the display is stable even when render outruns physics (no flicker at high FPS).
void EWAY_draw_hud_timers()
{
    if (EWAY_count_up_visible || EWAY_count_up_add_penalties) {
        PANEL_draw_timer(EWAY_count_up / 10, 320, 50);
    }
    if (EWAY_hud_countdown_value >= 0) {
        PANEL_draw_timer(EWAY_hud_countdown_value, 320, 50);
    }
}

// extern forward declaration: defined in Person.cpp, used here to count NPC spawn types.
// uc_orig: people_types (fallen/Source/Person.cpp)
extern SWORD people_types[50];

// Main mission update tick. Evaluates every waypoint each frame:
// activates waypoints whose conditions are met, deactivates timed/conditional ones.
// Also ticks the active conversation, driving penalty display, and scripted camera.
// uc_orig: EWAY_process (fallen/Source/eway.cpp)
void EWAY_process()
{
    SLONG i;
    SLONG on;
    SLONG timer;

    EWAY_Way* ew;
    SLONG offset, step;
    SLONG eway_max = EWAY_way_upto;

    step = 1;
    offset = 0;

    EWAY_count_up_visible = UC_FALSE;

    EWAY_time_accurate += 80 * TICK_RATIO >> TICK_SHIFT;
    EWAY_time = EWAY_time_accurate >> 4;
    EWAY_tick = 5 * TICK_RATIO >> TICK_SHIFT;

    // Refresh the wall-clock timestamp of the most recent voice playback every
    // physics tick so the cutscene MESSAGE-deferral check (in
    // EWAY_evaluate_condition) sees an accurate "voice still going / just
    // ended" reading regardless of which waypoint type is being evaluated.
    if (MFX_QUICK_still_playing()) {
        extern uint64_t sdl3_get_ticks();
        s_eway_voice_last_seen_ms = sdl3_get_ticks();
    }

    if (GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON)) {
        // Don't process waypoints after the game is lost or won.
        // Clear the on-screen countdown value too: in the original it was
        // queued inline from EWAY_evaluate_condition each tick into a global
        // (slPANEL_draw_timer_time) that auto-cleared each draw. Our refactor
        // persists the value across render frames (to avoid flicker when
        // render > physics), so we must explicitly clear it here — otherwise
        // the timer lingers frozen at its last value after level complete.
        EWAY_hud_countdown_value = -1;
    } else {
        for (i = 1 + (offset); i < eway_max; i += step) {

            ew = &EWAY_way[i];

            if (ew->flag & EWAY_FLAG_DEAD) {
                continue;
            }

            if (ew->flag & EWAY_FLAG_ACTIVE) {
                if (ew->ed.type == EWAY_DO_EMIT_STEAM) {
                    EWAY_process_emit_steam(ew);
                } else if (ew->ed.type == EWAY_DO_SHAKE_CAMERA) {
                    if (FC_cam[0].shake < 100) {
                        FC_cam[0].shake += 3;
                    }
                } else if (ew->ed.type == EWAY_DO_VISIBLE_COUNT_UP) {
                    EWAY_count_up += (50 * TICK_RATIO >> TICK_SHIFT) * step; // 50 * 20 ticks/sec = 1000 units/sec

                    EWAY_count_up_visible = UC_TRUE;

                    {
                        SLONG secs = EWAY_count_up / 1000;

                        if (secs > 255) {
                            secs = 255;
                        }

                        EWAY_counter[3] = secs;
                    }
                }

                if (ew->flag & EWAY_FLAG_COUNTDOWN) {
                    timer = EWAY_timer[ew->timer];
                    timer -= EWAY_tick * step;

                    if (timer <= 0) {
                        timer = 0;

                        EWAY_set_inactive(ew);
                    }

                    EWAY_timer[ew->timer] = timer;
                } else {
                    switch (ew->es.type) {
                    case EWAY_STAY_ALWAYS:

                        ew->flag |= EWAY_FLAG_DEAD;

                        break;

                    case EWAY_STAY_WHILE:

                        on = EWAY_evaluate_condition(ew, &ew->ec);

                        if (!on) {
                            EWAY_set_inactive(ew);
                        }

                        break;

                    case EWAY_STAY_WHILE_TIME:

                        on = EWAY_evaluate_condition(ew, &ew->ec);

                        if (!on) {
                            // Start the countdown timer before going inactive.
                            ew->timer = ew->es.arg;
                            ew->flag |= EWAY_FLAG_COUNTDOWN;
                        }

                        break;

                        // There is no case for EWAY_STAY_TIME because ew->timer
                        // should always be set in that case.

                    default:
                        ASSERT(0);
                        break;
                    }
                }

                if (ew->ed.type == EWAY_DO_WATER_SPOUT) {
                    DIRT_new_water(ew->x + 2, ew->y, ew->z, -1, 28, 0);
                    DIRT_new_water(ew->x, ew->y, ew->z + 2, 0, 29, -1);
                    DIRT_new_water(ew->x, ew->y, ew->z - 2, 0, 28, +1);
                    DIRT_new_water(ew->x - 2, ew->y, ew->z, +1, 29, 0);
                }
            } else {
                on = EWAY_evaluate_condition(ew, &ew->ec);

                if (on) {
                    EWAY_set_active(ew);
                }
            }
        }
    }

    EWAY_process_conversation();

    EWAY_process_penalties();

    EWAY_process_camera();
}

// Returns world-space position of a waypoint by index.
// uc_orig: EWAY_get_position (fallen/Source/eway.cpp)
void EWAY_get_position(
    SLONG waypoint,
    SLONG* world_x,
    SLONG* world_y,
    SLONG* world_z)
{
    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    *world_x = EWAY_way[waypoint].x;
    *world_y = EWAY_way[waypoint].y;
    *world_z = EWAY_way[waypoint].z;
}

// Returns the facing angle (yaw << 3) of a waypoint.
// uc_orig: EWAY_get_angle (fallen/Source/eway.cpp)
UWORD EWAY_get_angle(SLONG waypoint)
{
    UWORD ans;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ans = EWAY_way[waypoint].yaw << 3;

    return ans;
}

// Returns the Thing index of the entity spawned by this waypoint (CREATE_ENEMY/PLAYER/VEHICLE/ANIMAL).
// Returns NULL if the waypoint hasn't fired yet or doesn't spawn a Thing.
// uc_orig: EWAY_get_person (fallen/Source/eway.cpp)
UWORD EWAY_get_person(SLONG waypoint)
{
    UWORD ans;

    if (waypoint == NULL) {
        return NULL;
    }

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    EWAY_Way* ew;

    ew = &EWAY_way[waypoint];

    if (ew->ed.type == EWAY_DO_CREATE_ENEMY || ew->ed.type == EWAY_DO_CREATE_PLAYER || ew->ed.type == EWAY_DO_CREATE_VEHICLE || ew->ed.type == EWAY_DO_CREATE_ANIMAL) {
        ans = ew->ed.arg1;
    } else {
        ans = NULL;
    }

    return ans;
}

// Searches EWAY_way[] circularly from 'index' for a waypoint matching type/colour/group.
// Pass EWAY_DONT_CARE for any field to match any value. Returns index or EWAY_NO_MATCH.
// uc_orig: EWAY_find_waypoint (fallen/Source/eway.cpp)
SLONG EWAY_find_waypoint(
    SLONG index,
    SLONG whatdo,
    SLONG colour,
    SLONG group,
    UBYTE only_active)
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        if (index >= EWAY_way_upto) {
            index = 1;
        }

        ASSERT(WITHIN(index, 0, EWAY_way_upto - 1));

        ew = &EWAY_way[index];

        if ((colour == EWAY_DONT_CARE || colour == ew->colour) && (group == EWAY_DONT_CARE || group == ew->group) && (whatdo == EWAY_DONT_CARE || whatdo == ew->ed.type)) {
            if (!only_active || (ew->flag & EWAY_FLAG_ACTIVE)) {
                return index;
            }
        }

        index += 1;
    }

    return EWAY_NO_MATCH;
}

// Picks a random waypoint matching colour/group, excluding 'not_this_index'.
// Samples up to EWAY_FIND_RAND_MAX candidates. Returns index or EWAY_NO_MATCH.
// uc_orig: EWAY_find_waypoint_rand (fallen/Source/eway.cpp)
SLONG EWAY_find_waypoint_rand(
    SLONG not_this_index,
    SLONG colour,
    SLONG group,
    UBYTE only_active)
{
    SLONG i;
    SLONG ans;

    EWAY_Way* ew;

// uc_orig: EWAY_FIND_RAND_MAX (fallen/Source/eway.cpp)
#define EWAY_FIND_RAND_MAX 32

    UWORD choice[EWAY_FIND_RAND_MAX];
    SLONG choice_upto = 0;

    for (i = 1; i < EWAY_way_upto; i++) {
        if (i == not_this_index) {
            continue;
        }

        ew = &EWAY_way[i];

        if ((colour == EWAY_DONT_CARE || colour == ew->colour) && (group == EWAY_DONT_CARE || group == ew->group)) {
            ASSERT(WITHIN(choice_upto, 0, EWAY_FIND_RAND_MAX - 1));

            if (!only_active || (ew->flag & EWAY_FLAG_ACTIVE)) {
                choice[choice_upto++] = i;

                if (choice_upto >= EWAY_FIND_RAND_MAX) {
                    break;
                }
            }
        }
    }

    if (choice_upto == 0) {
        ans = EWAY_NO_MATCH;
    } else {
        ans = choice[Random() % choice_upto];
    }

    return ans;
}

// Returns the index of the nearest active waypoint matching colour/group to the given world position.
// uc_orig: EWAY_find_nearest_waypoint (fallen/Source/eway.cpp)
SLONG EWAY_find_nearest_waypoint(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG colour,
    SLONG group)
{
    SLONG i;

    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;

    SLONG best_dist = UC_INFINITY;
    SLONG best_waypoint = EWAY_NO_MATCH;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];

        if ((colour == EWAY_DONT_CARE || colour == ew->colour) && (group == EWAY_DONT_CARE || group == ew->group)) {
            if (ew->flag & EWAY_FLAG_ACTIVE) {
                dx = abs(ew->x - x);
                dy = abs(ew->y - y);
                dz = abs(ew->z - z);

                dist = QDIST3(dx, dy, dz);

                if (dist < best_dist) {
                    best_dist = dist;
                    best_waypoint = i;
                }
            }
        }
    }

    return best_waypoint;
}

// Returns the current scripted camera's position/orientation for the renderer.
// If analogue controls are active and the player is still moving, camera grab is suppressed.
// uc_orig: EWAY_grab_camera (fallen/Source/eway.cpp)
SLONG EWAY_grab_camera(
    SLONG* cam_x,
    SLONG* cam_y,
    SLONG* cam_z,
    SLONG* cam_yaw,
    SLONG* cam_pitch,
    SLONG* cam_roll,
    SLONG* cam_lens)
{
    if (EWAY_cam_active) {
        // If there is analogue control and the player is still moving
        // then we can't grab the camera because it'll confuse the player control.
        extern SLONG analogue;

        if (analogue && !EWAY_stop_player_moving()) {
            return UC_FALSE;
        } else {
            *cam_x = EWAY_cam_x;
            *cam_y = EWAY_cam_y;
            *cam_z = EWAY_cam_z;
            *cam_yaw = EWAY_cam_yaw;
            *cam_pitch = EWAY_cam_pitch;
            *cam_roll = 1024 << 8;
            *cam_lens = EWAY_cam_lens;
        }
    }

    return EWAY_cam_active;
}

// Returns the warehouse byte for the active scripted camera.
// During a conversation, returns the speaker's warehouse instead.
// uc_orig: EWAY_camera_warehouse (fallen/Source/eway.cpp)
UBYTE EWAY_camera_warehouse()
{
    if (EWAY_cam_active) {
        if (EWAY_conv_active) {
            if (EWAY_conv_person_a) {
                ASSERT(TO_THING(EWAY_conv_person_a)->Class == CLASS_PERSON);

                return TO_THING(EWAY_conv_person_a)->Genus.Person->Ware;
            }
        } else {
            return EWAY_cam_warehouse;
        }
    }

    return NULL;
}

// Flags a CREATE_ITEM waypoint as collected (sets EWAY_FLAG_GOTITEM).
// Called when the player picks up the item this waypoint spawned.
// uc_orig: EWAY_item_pickedup (fallen/Source/eway.cpp)
void EWAY_item_pickedup(SLONG waypoint)
{
    EWAY_Way* ew;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ew = &EWAY_way[waypoint];

    ASSERT(ew->ed.type == EWAY_DO_CREATE_ITEM);

    ew->flag |= EWAY_FLAG_GOTITEM;
}

// Returns the delay (in ms) defined by a EWAY_DO_NOTHING waypoint, or default_delay if not applicable.
// uc_orig: EWAY_get_delay (fallen/Source/eway.cpp)
SLONG EWAY_get_delay(SLONG waypoint, SLONG default_delay)
{
    EWAY_Way* ew;

    if (waypoint == 0) {
        return default_delay;
    }

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ew = &EWAY_way[waypoint];

    switch (ew->ed.type) {
    case EWAY_DO_NOTHING:
        return ew->ed.arg1 * 100;
        break;

    default:
        return default_delay;
        break;
    }
}

// Returns UC_TRUE if the given waypoint is currently active.
// uc_orig: EWAY_is_active (fallen/Source/eway.cpp)
SLONG EWAY_is_active(SLONG waypoint)
{
    EWAY_Way* ew;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ew = &EWAY_way[waypoint];

    if (ew->flag & EWAY_FLAG_ACTIVE) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// Searches for a COND_PERSON_USED waypoint referencing the given Thing,
// fires it if found, and clears FLAG_PERSON_USEABLE on that Thing.
// Returns UC_TRUE if a MESSAGE waypoint was triggered.
// uc_orig: EWAY_used_person (fallen/Source/eway.cpp)
SLONG EWAY_used_person(UWORD t_index)
{
    UWORD i;
    SLONG ans = UC_FALSE;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];

        if (ew->flag & EWAY_FLAG_DEAD) {
            continue;
        }

        if (ew->ec.type == EWAY_COND_PERSON_USED) {
            ASSERT(WITHIN(ew->ec.arg1, 1, EWAY_way_upto - 1));

            if (EWAY_way[ew->ec.arg1].ed.type == EWAY_DO_CREATE_ENEMY) {
                EWAY_Way* ew2 = &EWAY_way[ew->ec.arg1];

                if (ew2->ed.arg1 == t_index) {
                    EWAY_used_thing = t_index;

                    EWAY_set_active(ew);

                    EWAY_used_thing = NULL;

                    ASSERT(TO_THING(t_index)->Class == CLASS_PERSON);

                    TO_THING(t_index)->Genus.Person->Flags &= ~FLAG_PERSON_USEABLE;

                    if (ew->ed.type == EWAY_DO_MESSAGE) {
                        ans = UC_TRUE;
                    }
                }
            }
        }
    }

    return ans;
}

// Returns the warehouse byte for a waypoint (set by EWAY_work_out_which_ones_are_in_warehouses).
// uc_orig: EWAY_get_warehouse (fallen/Source/eway.cpp)
UBYTE EWAY_get_warehouse(SLONG waypoint)
{
    UBYTE ans;

    ASSERT(WITHIN(waypoint, 1, EWAY_way_upto - 1));

    ans = EWAY_way[waypoint].ware;

    return ans;
}

// Scans all waypoints and tags those located inside hidden buildings
// (warehouses) with the containing warehouse index (ew->ware).
// uc_orig: EWAY_work_out_which_ones_are_in_warehouses (fallen/Source/eway.cpp)
void EWAY_work_out_which_ones_are_in_warehouses()
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];

        if (PAP_2HI(ew->x >> 8, ew->z >> 8).Flags & PAP_FLAG_HIDDEN) {
            SLONG ware_top;

            ware_top = PAP_calc_map_height_at(
                ew->x,
                ew->z);

            if (ew->y < ware_top - 0x80) {
                ew->ware = WARE_which_contains(
                    ew->x >> 8,
                    ew->z >> 8);
            }
        }
    }
}

// Number of discrete viewing angles sampled when placing the EWAY cinematic camera.
// Must be a power of 2.
// uc_orig: EWAY_CAM_VIEW_ANGLES (fallen/Source/eway.cpp)
#define EWAY_CAM_VIEW_ANGLES 16

// Number of angles sampled when finding the best conversation camera position.
// Must be a power of 2.
// uc_orig: EWAY_CONVERSE_ANGLES (fallen/Source/eway.cpp)
#define EWAY_CONVERSE_ANGLES 16

// Positions the scripted camera for a two-person conversation.
// Scores candidate positions based on LOS to both speakers, angle separation, and prim collision.
// uc_orig: EWAY_cam_converse (fallen/Source/eway.cpp)
void EWAY_cam_converse(Thing* p_thing, Thing* p_listener)
{
    SLONG i;
    SLONG j;

    SLONG x;
    SLONG y;
    SLONG z;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG cam_dist;

    SLONG best_angle;
    SLONG best_score;

    SLONG side1;
    SLONG side2;

    SLONG angle1;
    SLONG angle2;
    SLONG dangle;

    UBYTE score[EWAY_CONVERSE_ANGLES];

    EWAY_cam_thing = THING_NUMBER(p_thing);
    EWAY_cam_waypoint = NULL;
    EWAY_cam_lens = 0x28000;
    EWAY_cam_freeze = UC_TRUE;
    EWAY_cam_lock = UC_FALSE;
    EWAY_cam_goinactive = UC_FALSE;

    // If the current camera can see both speakers, don't move it needlessly.
    if (EWAY_cam_active) {
        x = EWAY_cam_x;
        y = EWAY_cam_y;
        z = EWAY_cam_z;
    } else {
        x = FC_cam[0].x;
        y = FC_cam[0].y;
        z = FC_cam[0].z;
    }

    dx = abs(p_thing->WorldPos.X - x);
    dy = abs(p_thing->WorldPos.Y - y);
    dz = abs(p_thing->WorldPos.Z - z);

    // Distance between speakers determines how far back the camera sits.
    dx = p_listener->WorldPos.X - p_thing->WorldPos.X >> 8;
    dz = p_listener->WorldPos.Z - p_thing->WorldPos.Z >> 8;

    cam_dist = QDIST2(abs(dx), abs(dz));

    SATURATE(cam_dist, 0x180, 0x300);

    for (i = 0; i < EWAY_CONVERSE_ANGLES; i++) {
        score[i] = 0;

        dx = SIN(i * (2048 / EWAY_CONVERSE_ANGLES)) * cam_dist >> 16;
        dz = COS(i * (2048 / EWAY_CONVERSE_ANGLES)) * cam_dist >> 16;

        x = (p_thing->WorldPos.X >> 8) + dx;
        y = (p_thing->WorldPos.Y >> 8) + 0x100;
        z = (p_thing->WorldPos.Z >> 8) + dz;

        if (!there_is_a_los(
                x, y, z,
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Y + 0x6000 >> 8,
                p_thing->WorldPos.Z >> 8,
                LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
            // Camera can't see the speaker — skip this angle.
            continue;
        }

        score[i] += 40;

        if (!there_is_a_los(
                x, y, z,
                p_thing->WorldPos.X >> 8,
                p_thing->WorldPos.Y + 0x6000 >> 8,
                p_thing->WorldPos.Z >> 8,
                LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG | LOS_FLAG_IGNORE_UNDERGROUND_CHECK)) {
            // Camera can also see the listener — good.
            score[i] += 25;
        }

        // Penalise angles where listener occludes speaker.
        dx = (p_thing->WorldPos.X >> 8) - x;
        dz = (p_thing->WorldPos.Z >> 8) - z;

        angle1 = calc_angle(dx, dz);

        dx = (p_listener->WorldPos.X >> 8) - x;
        dz = (p_listener->WorldPos.Z >> 8) - z;

        angle2 = calc_angle(dx, dz);

        dangle = angle2 - angle1;

        if (dangle < -1024) {
            dangle += 2048;
        }
        if (dangle > +1024) {
            dangle -= 2048;
        }

        if (abs(dangle) > 100) {
            // Speakers are visually separated — good.
            score[i] += 15;
        }

        // Bonus for seeing the speaker's face.
        dangle = angle1 - p_thing->Draw.Tweened->Angle;

        if (dangle < -1024) {
            dangle += 2048;
        }
        if (dangle > +1024) {
            dangle -= 2048;
        }

        if (abs(dangle) < 400) {
            score[i] += 10;
        }

        // Penalise positions that intersect primitives along the camera path.
        dx = (p_thing->WorldPos.X >> 8) - x >> 3;
        dy = (p_thing->WorldPos.Y >> 8) - y >> 3;
        dz = (p_thing->WorldPos.Z >> 8) - z >> 3;

        for (j = 0; j < 8; j++) {
            if (OB_inside_prim(x, y, z)) {
                score[i] -= 5;
            }

            x += dx;
            y += dy;
            z += dz;
        }
    }

    // Halve score for angles adjacent to 0 (avoids camera near walls).
    for (i = 0; i < EWAY_CONVERSE_ANGLES; i++) {
        side1 = (i - 1) & (EWAY_CONVERSE_ANGLES - 1);
        side2 = (i + 1) & (EWAY_CONVERSE_ANGLES - 1);

        if (side1 == 0 || side2 == 0) {
            score[i] >>= 1;
        }
    }

    best_angle = -1;
    best_score = 0;

    for (i = 0; i < EWAY_CONVERSE_ANGLES; i++) {
        if (score[i] > best_score) {
            best_score = score[i];
            best_angle = i;
        }
    }

    if (best_score == 0) {
        // Emergency fallback: place camera midway between speakers.
        EWAY_cam_x = p_thing->WorldPos.X + p_listener->WorldPos.X >> 1;
        EWAY_cam_y = p_thing->WorldPos.Y + p_listener->WorldPos.Y >> 1;
        EWAY_cam_z = p_thing->WorldPos.Z + p_listener->WorldPos.Z >> 1;

        EWAY_cam_y += 0x30000;
    } else {
        SLONG division = 2048 / EWAY_CONVERSE_ANGLES;
        SLONG angle = best_angle * division;

        // Modulate angle randomly within the winning sector.
        angle += (Random() & (division - 1)) - (division / 2);
        angle &= 2047;

        dx = SIN(angle) * cam_dist >> 8;
        dz = COS(angle) * cam_dist >> 8;

        // Interpolate target point 0–50% from speaker toward listener.
        SLONG rnd = Random() & 127;

        x = ((p_listener->WorldPos.X >> 8) - (p_thing->WorldPos.X >> 8)) * rnd;
        y = ((p_listener->WorldPos.Y >> 8) - (p_thing->WorldPos.Y >> 8)) * rnd;
        z = ((p_listener->WorldPos.Z >> 8) - (p_thing->WorldPos.Z >> 8)) * rnd;

        x += p_thing->WorldPos.X;
        y += p_thing->WorldPos.Y;
        z += p_thing->WorldPos.Z;

        EWAY_cam_targx = x;
        EWAY_cam_targy = y + 0xA000;
        EWAY_cam_targz = z;

        EWAY_cam_x = x + dx;
        EWAY_cam_y = y + 0x10000;
        EWAY_cam_z = z + dz;

        EWAY_cam_target = 0;
        EWAY_cam_thing = 0;
    }

    EWAY_cam_active = UC_TRUE;
}

// Begins the fade-out transition back from the scripted camera to the player camera.
// Sets goinactive=2 so the camera fades over two ticks before releasing.
// uc_orig: EWAY_cam_relinquish (fallen/Source/eway.cpp)
void EWAY_cam_relinquish()
{
    EWAY_cam_goinactive = 2;
}

// Finds the waypoint that spawned a given person, or creates a dummy waypoint for it.
// Used to associate pre-existing Things (e.g. the player) with the EWAY system.
// uc_orig: EWAY_find_or_create_waypoint_that_created_person (fallen/Source/eway.cpp)
SLONG EWAY_find_or_create_waypoint_that_created_person(Thing* p_person)
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];

        if (EWAY_get_person(i) == THING_NUMBER(p_person)) {
            return i;
        }
    }

    ASSERT(WITHIN(EWAY_way_upto, 1, EWAY_MAX_WAYS - 1));

    ew = &EWAY_way[EWAY_way_upto];

    ew->flag = EWAY_FLAG_ACTIVE | EWAY_FLAG_DEAD;
    ew->ed.type = EWAY_DO_CREATE_ENEMY;
    ew->ed.arg1 = THING_NUMBER(p_person);

    return EWAY_way_upto++;
}

// Returns UC_TRUE if a scripted conversation is currently playing,
// and fills person_a/person_b with the Thing indices of the two speakers.
// uc_orig: EWAY_conversation_happening (fallen/Source/eway.cpp)
SLONG EWAY_conversation_happening(
    THING_INDEX* person_a,
    THING_INDEX* person_b)
{
    if (EWAY_conv_active) {
        *person_a = EWAY_conv_person_a;
        *person_b = EWAY_conv_person_b;

        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// Called when a prim (interactive object) is activated by the player.
// Sets EWAY_FLAG_TRIGGERED on any waypoint waiting for that prim's activation.
// uc_orig: EWAY_prim_activated (fallen/Source/eway.cpp)
void EWAY_prim_activated(SLONG ob_index)
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];

        if (ew->ec.type == EWAY_COND_PRIM_ACTIVATED && ew->ec.arg1 == ob_index) {
            ew->flag |= EWAY_FLAG_TRIGGERED;

            return;
        }
    }
}
