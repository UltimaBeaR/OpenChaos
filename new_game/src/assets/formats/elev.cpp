// Level loader: parses .ucm mission files and initialises game state.
// Despite the filename "elev" (historical: "ECTS level stuff"), this file
// contains the main level loading pipeline, not elevator mechanics.
// Elevator/platform logic lives in world/environment/plat.cpp and actors/core/interact.cpp.

#include "game/game.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"

#include "assets/formats/elev.h"
#include "assets/formats/elev_globals.h"

#include "missions/eway.h"
#include "missions/mission.h"
#include "missions/mission_globals.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "map/ob.h"
#include "map/ob_globals.h"
#include "world_objects/tripwire.h"
#include "world_objects/tripwire_globals.h"
#include "engine/audio/music.h"
#include "world_objects/dirt.h"
#include "effects/weather/fog.h"
#include "things/items/hook.h"
#include "things/items/hook_globals.h"
#include "effects/weather/mist.h"
#include "world_objects/puddle.h"
#include "world_objects/puddle_globals.h"
#include "effects/weather/drip.h"
#include "effects/combat/glitter.h"
#include "effects/combat/spark.h"
#include "assets/formats/level_loader.h"
#include "assets/formats/level_loader_globals.h"
#include "assets/formats/anim_loader.h"
#include "assets/formats/anim_loader_globals.h"
#include "effects/combat/pow.h"
#include "effects/combat/pow_globals.h"
#include "buildings/build2.h"
#include "map/sewers.h"
#include "map/road.h"
#include "map/road_globals.h"
#include "things/vehicles/chopper.h"
#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "assets/formats/anim_tmap.h"
#include "engine/graphics/lighting/shadow.h"
#include "engine/graphics/lighting/shadow_globals.h"
#include "ui/frontend/attract.h"
#include "camera/cam.h"
#include "engine/effects/psystem.h"
#include "engine/effects/psystem_globals.h"
#include "effects/environment/tracks.h"
#include "ai/pcom.h"
#include "navigation/wmove.h"
#include "navigation/wmove_globals.h"
#include "things/items/balloon.h"
#include "navigation/wand.h"
#include "effects/environment/ribbon.h"
#include "things/items/barrel.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "buildings/ware.h"
#include "missions/memory.h"
#include "missions/playcuts.h"
#include "missions/playcuts_globals.h"
#include "things/items/grenade.h"
#include "engine/io/env.h"
#include "ui/hud/panel.h"
#include "ui/hud/panel_globals.h"
#include "engine/audio/sound.h"

#include "things/animals/bat.h"
#include "map/supermap_globals.h"
#include "things/items/special.h"
#include "things/vehicles/vehicle.h"
#include "things/vehicles/vehicle_globals.h"
#include "world_objects/plat.h"
#include "buildings/prim_types.h" // PrimObject, PrimInfo, PRIM_OBJ_*
#include "buildings/prim.h"       // get_prim_info
#include "things/core/statedef.h"
#include "ai/mav.h"
#include "engine/physics/collide.h"
#include "ui/frontend/startscr_globals.h"    // STARTSCR_mission
#include "things/core/thing_globals.h"  // playback_file
#include "things/characters/person.h"  // init_persons
#include "engine/console/console.h" // CONSOLE_clear
#include "assets/texture.h"            // TEXTURE_fix_prim_textures
#include "engine/graphics/pipeline/aeng.h"  // AENG_create_dx_prim_points, TEXTURE_load_needed

// vehicle_random[]: shared table from Vehicle.cpp defining which vehicle prims
// appear in FAKE_CARS random spawns. Accessed here as an extern (not in any header).
// uc_orig: vehicle_random (fallen/Source/Vehicle.cpp)
extern UBYTE vehicle_random[];

// people_types[]: counters per person type, incremented as enemies are created from .ucm data.
// Declared in Person.cpp (not in Person.h); accessed here with an extern.
// uc_orig: people_types (fallen/Source/Person.cpp)
extern SWORD people_types[50];

// uc_orig: ELEV_load_level (fallen/Source/elev.cpp)
void ELEV_load_level(CBYTE* fname_level)
{
    SLONG i;
    SLONG j;
    SLONG x;
    SLONG z;
    SLONG tx;
    SLONG ty;
    SLONG tz;
    SLONG angle;
    SLONG version;
    SLONG load_ok = UC_FALSE;
    SLONG flag;

    SLONG ew_world_x;
    SLONG ew_world_y;
    SLONG ew_world_z;
    SLONG ew_colour;
    SLONG ew_group;
    SLONG ew_yaw;
    SLONG ew_speed;
    SLONG ew_delay;

    SLONG mess_count = 0;
    SLONG cutscene_count = 0;
    SLONG water_level = -0x80;

    MFFileHandle handle = NULL;

    SLONG enemy_type;
    SLONG follow;
    SLONG kludge_index;

    EWAY_Conddef ecd;
    EWAY_Conddef ecd1;
    EWAY_Conddef ecd2;
    EWAY_Do ed;
    EWAY_Stay es;
    EWAY_Edef ee;

    OB_Info* oi;
    UBYTE FAKE_CARS = 0;

    GAME_TURN = 0;
    SAVE_VALID = 0;

    EWAY_init();

    iamapsx = ENV_get_value_number("iamapsx", UC_FALSE);

    if (!CNET_network_game)
        NO_PLAYERS = 0;

    load_ok = UC_TRUE;

    if (fname_level != NULL) {
        handle = FileOpen(fname_level);

        if (handle == FILE_OPEN_ERROR) {
            load_ok = UC_FALSE;
        } else {
            if (FileRead(handle, &version, sizeof(SLONG)) == FILE_READ_ERROR)
                goto file_error; // Version
            if (FileRead(handle, &flag, sizeof(SLONG)) == FILE_READ_ERROR)
                goto file_error; // Used
            if (FileRead(handle, junk, _MAX_PATH) == FILE_READ_ERROR)
                goto file_error; // BriefName
            if (FileRead(handle, junk, _MAX_PATH) == FILE_READ_ERROR)
                goto file_error; // LightMapName
            if (FileRead(handle, junk, _MAX_PATH) == FILE_READ_ERROR)
                goto file_error; // MapName
            if (FileRead(handle, junk, _MAX_PATH) == FILE_READ_ERROR)
                goto file_error; // MissionName
            if (FileRead(handle, junk, _MAX_PATH) == FILE_READ_ERROR)
                goto file_error; // SewerMapName
            if (FileRead(handle, junk, sizeof(UWORD)) == FILE_READ_ERROR)
                goto file_error; // MapIndex
            if (FileRead(handle, junk, sizeof(UWORD)) == FILE_READ_ERROR)
                goto file_error; // Used
            if (FileRead(handle, junk, sizeof(UWORD)) == FILE_READ_ERROR)
                goto file_error; // Free

            GAME_FLAGS &= ~GF_SHOW_CRIMERATE;
            GAME_FLAGS &= ~GF_CARS_WITH_ROAD_PRIMS;

            if (flag & MISSION_FLAG_SHOW_CRIMERATE) {
                GAME_FLAGS |= GF_SHOW_CRIMERATE;
            }
            if (flag & MISSION_FLAG_CARS_WITH_ROAD_PRIMS) {
                GAME_FLAGS |= GF_CARS_WITH_ROAD_PRIMS;
            }

            // Read CRIME_RATE and FAKE_CIVS packed into a single UWORD.
            // junk[0] = crime rate percentage (0-100); junk[1] = fake civilian count.
            // If crime rate is 0 in the file it means unset — default to 50.
            if (FileRead(handle, junk, sizeof(UWORD)) == FILE_READ_ERROR)
                goto file_error;

            CRIME_RATE = junk[0];

            if (CRIME_RATE == 0) {
                CRIME_RATE = 50;
            }

            FAKE_CIVS = junk[1];

            for (i = 0; i < FAKE_CIVS; i++)

            {
                SLONG index;
                SLONG mx, mz;

                SLONG max = 0;

                while (max++ < 128 * 128) {
                    SLONG WAND_find_good_start_point_near(SLONG * mapx, SLONG * mapz);

                    mx = (Random() % 100) + 14;
                    mz = (Random() % 100) + 14;

                    if (WAND_find_good_start_point_near(&mx, &mz))
                        break;
                }

                index = PCOM_create_person(
                    PERSON_CIV,
                    0,
                    0,
                    PCOM_AI_CIV,
                    0,
                    0,
                    PCOM_MOVE_WANDER,
                    0,
                    0,
                    0,
                    0,
                    0,
                    mx << 8,
                    0,
                    mz << 8,
                    0,
                    0, 0, FLAG2_PERSON_FAKE_WANDER);

                TO_THING(index)->Genus.Person->InsideRoom = (Random() % 20) + 20;
            }

            // Parse EventPoint records and translate them into EWAY waypoints.
            // Each record is a 14-byte header followed by 60 bytes of Data[].
            // TT_* trigger types map to EWAY_COND_* conditions.
            // WPT_* waypoint types map to EWAY_DO_* actions.
            for (i = 0; i < MAX_EVENTPOINTS; i++) {
                if (FileRead(handle, &event_point, 14) == FILE_READ_ERROR)
                    goto file_error;
                if (FileRead(handle, &event_point.Data[0], 14 * 4 + 4) == FILE_READ_ERROR)
                    goto file_error;

                if (event_point.Used) {
                    ecd.type = EWAY_COND_TRUE;
                    ecd.negate = (event_point.Flags >> 1) & 1;
                    ecd.arg1 = 0;
                    ecd.arg2 = 0;
                    ecd.bool_arg1 = NULL;
                    ecd.bool_arg2 = NULL;

                    es.type = EWAY_STAY_ALWAYS;
                    es.arg = 0;

                    ed.type = EWAY_DO_NOTHING;
                    ed.subtype = 0;
                    ed.arg1 = 0;
                    ed.arg2 = 0;

                    ee.pcom_ai = 0;
                    ee.pcom_bent = 0;
                    ee.pcom_move = 0;
                    ee.pcom_has = 0;
                    ee.ai_other = 0;
                    ee.zone = 0;

                    kludge_index = 0;

                    ew_colour = event_point.Colour;
                    ew_group = event_point.Group;
                    ew_world_x = event_point.X;
                    if (event_point.Flags & WPT_FLAGS_INSIDE) {
                        ew_world_y = get_inside_alt(event_point.Y);
                    } else {
                        ew_world_y = event_point.Y;
                    }
                    ew_world_z = event_point.Z;
                    ew_yaw = ((128 + event_point.Direction) << 3) & 2047;

                    // Map TriggeredBy (TT_*) editor values to EWAY_COND_* runtime condition types.
                    switch (event_point.TriggeredBy) {
                    case TT_NONE:
                        break;

                    case TT_DEPENDENCY:
                        ecd.type = EWAY_COND_DEPENDENT;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_RADIUS:
                        ecd.type = EWAY_COND_PROXIMITY;
                        ecd.arg1 = event_point.Radius;
                        break;

                    case TT_DOOR:
                        break;

                    case TT_TRIPWIRE:
                        ecd.type = EWAY_COND_TRIPWIRE;
                        break;

                    case TT_PRESSURE_PAD:
                        ecd.type = EWAY_COND_PRESSURE;
                        break;

                    case TT_ELECTRIC_FENCE:
                        break;

                    case TT_WATER_LEVEL:
                        break;

                    case TT_SECURITY_CAMERA:
                        ecd.type = EWAY_COND_CAMERA;
                        break;

                    case TT_SWITCH:
                        ecd.type = EWAY_COND_SWITCH;
                        break;

                    case TT_ANIM_PRIM:
                        break;

                    case TT_TIMER:
                        ecd.type = EWAY_COND_TIME;
                        ecd.arg1 = event_point.Radius; // Radius field doubles as the time argument.
                        break;

                    case TT_SHOUT_ALL:
                        break;

                    case TT_BOOLEANAND:
                        // Boolean conditions always reference other waypoints (dependency-based).
                        ecd1.type = EWAY_COND_DEPENDENT;
                        ecd1.arg1 = event_point.EPRef;
                        ecd1.negate = UC_FALSE;

                        ecd2.type = EWAY_COND_DEPENDENT;
                        ecd2.arg1 = event_point.EPRefBool;
                        ecd2.negate = UC_FALSE;

                        ecd.type = EWAY_COND_BOOL_AND;
                        ecd.bool_arg1 = &ecd1;
                        ecd.bool_arg2 = &ecd2;

                        break;

                    case TT_BOOLEANOR:
                        ecd1.type = EWAY_COND_DEPENDENT;
                        ecd1.arg1 = event_point.EPRef;
                        ecd1.negate = UC_FALSE;

                        ecd2.type = EWAY_COND_DEPENDENT;
                        ecd2.arg1 = event_point.EPRefBool;
                        ecd2.negate = UC_FALSE;

                        ecd.type = EWAY_COND_BOOL_OR;
                        ecd.bool_arg1 = &ecd1;
                        ecd.bool_arg2 = &ecd2;

                        break;

                    case TT_ITEM_HELD:
                        ecd.type = EWAY_COND_ITEM_HELD;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_ITEM_SEEN:
                        break;

                    case TT_KILLED:
                        ecd.type = EWAY_COND_PERSON_DEAD;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_SHOUT_ANY:
                        break;

                    case TT_COUNTDOWN:
                        ecd.type = EWAY_COND_COUNTDOWN;
                        ecd.arg1 = event_point.EPRef;
                        ecd.arg2 = event_point.Radius;
                        break;

                    case TT_ENEMYRADIUS:
                        ecd.type = EWAY_COND_PERSON_NEAR;
                        ecd.arg1 = event_point.EPRef;
                        ecd.arg2 = event_point.Radius / 64;
                        break;

                    case TT_VISIBLECOUNTDOWN:
                        ecd.type = EWAY_COND_COUNTDOWN_SEE;
                        ecd.arg1 = event_point.EPRef;
                        ecd.arg2 = event_point.Radius;
                        break;

                    case TT_CUBOID:
                        ecd.type = EWAY_COND_PLAYER_CUBOID;
                        ecd.arg1 = event_point.Radius & 0xffff;
                        ecd.arg2 = event_point.Radius >> 16;
                        break;

                    case TT_HALFDEAD:
                        ecd.type = EWAY_COND_HALF_DEAD;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_PERSON_SEEN:
                        ecd.type = EWAY_COND_A_SEE_B;
                        ecd.arg1 = event_point.EPRef;
                        ecd.arg2 = event_point.EPRefBool;
                        break;

                    case TT_PERSON_USED:
                        ecd.type = EWAY_COND_PERSON_USED;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_PLAYER_USES_RADIUS:
                        ecd.type = EWAY_COND_RADIUS_USED;
                        ecd.arg1 = event_point.Radius;
                        break;

                    case TT_GROUPDEAD:
                        ecd.type = EWAY_COND_GROUP_DEAD;
                        break;

                    case TT_PRIM_DAMAGED:
                        ecd.type = EWAY_COND_PRIM_DAMAGED;
                        break;

                    case TT_PERSON_ARRESTED:
                        ecd.type = EWAY_COND_PERSON_ARRESTED;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_CONVERSATION_OVER:
                        ecd.type = EWAY_COND_CONVERSE_END;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_COUNTER:
                        ecd.type = EWAY_COND_COUNTER_GTEQ;
                        ecd.arg1 = event_point.EPRef;
                        ecd.arg2 = event_point.Radius;
                        break;

                    case TT_KILLED_NOT_ARRESTED:
                        ecd.type = EWAY_COND_KILLED_NOT_ARRESTED;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    // Radius field is repurposed as threshold percentage here; divide by 100
                    // to normalise to the same 0-100 scale as CRIME_RATE.
                    case TT_CRIME_RATE_ABOVE:
                        ecd.type = EWAY_COND_CRIME_RATE_GTEQ;
                        ecd.arg1 = event_point.Radius / 100;
                        break;

                    case TT_CRIME_RATE_BELOW:
                        ecd.type = EWAY_COND_CRIME_RATE_LTEQ;
                        ecd.arg1 = event_point.Radius / 100;
                        break;

                    case TT_PERSON_IS_MURDERER:
                        ecd.type = EWAY_COND_IS_MURDERER;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_PERSON_IN_VEHICLE:
                        ecd.type = EWAY_COND_PERSON_IN_VEHICLE;
                        ecd.arg1 = event_point.EPRef;
                        ecd.arg2 = event_point.EPRefBool;
                        break;

                    case TT_THING_RADIUS_DIR:
                        ecd.type = EWAY_COND_THING_RADIUS_DIR;
                        ecd.arg1 = event_point.EPRef;
                        ecd.arg2 = event_point.Radius / 64;
                        break;

                    case TT_SPECIFIC_ITEM_HELD:
                        ecd.type = EWAY_COND_SPECIFIC_ITEM_HELD;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_RANDOM:
                        ecd.type = EWAY_COND_RANDOM;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_PLAYER_FIRES_GUN:
                        ecd.type = EWAY_COND_PLAYER_FIRED_GUN;
                        break;

                    case TT_DARCI_GRABBED:
                        ecd.type = EWAY_COND_DARCI_GRABBED;
                        break;

                    case TT_PUNCHED_AND_KICKED:
                        ecd.type = EWAY_COND_PUNCHED_AND_KICKED;
                        ecd.arg1 = event_point.EPRef;
                        break;

                    case TT_MOVE_RADIUS_DIR:
                        ecd.type = EWAY_COND_MOVE_RADIUS_DIR;
                        ecd.arg1 = event_point.EPRef;
                        ecd.arg2 = event_point.Radius / 64;
                        break;

                    default:
                        ASSERT(0);
                        break;
                    }

                    // Map WaypointType (WPT_*) editor values to EWAY_DO_* runtime action types.
                    // WPT_BONUS_POINTS: the objective/scoring branch in if(0) is dead code —
                    // bonus points were replaced by messages before ship.
                    // WPT_GOTHERE_DOTHIS (if it appears): hits default ASSERT — not implemented.
                    switch (event_point.WaypointType) {
                    case WPT_NONE:
                        break;

                    case WPT_SIMPLE:
                        ed.type = EWAY_DO_NOTHING;
                        ed.arg1 = event_point.Data[0];
                        break;

                    case WPT_CREATE_PLAYER:

                        ed.type = EWAY_DO_CREATE_PLAYER;

                        switch (event_point.Data[0]) {
                        default:
                        case PT_DARCI:
                            ed.subtype = PLAYER_DARCI;
                            break;
                        case PT_ROPER:
                            ed.subtype = PLAYER_ROPER;
                            break;
                        case PT_COP:
                            ed.subtype = PLAYER_COP;
                            break;
                        case PT_GANG:
                            ed.subtype = PLAYER_THUG;
                            break;
                        }

                        ed.subtype |= (event_point.Data[1] ? 8 : 0);

                        break;

                    // Data[0] lo=enemy_type, hi=enemy_count (version>=3 packed format).
                    // Data[2] HIWORD = HAS_* weapon flags; Data[5] = pcom_ai / ai_skill.
                    // ee.drop from Data[8] bitmask — item NPC drops when killed.
                    case WPT_CREATE_ENEMIES:

                        ed.type = EWAY_DO_CREATE_ENEMY;

                        if (version >= 3) {
                            enemy_type = event_point.Data[0] & 0xffff;
                            follow = event_point.Data[1];
                        } else {
                            enemy_type = event_point.Data[0];
                            follow = NULL;
                        }

                        ee.pcom_has = 0;

                        if ((event_point.Data[2] >> 16) & HAS_PISTOL) {
                            ee.pcom_has |= PCOM_HAS_GUN;
                        }
                        if ((event_point.Data[2] >> 16) & HAS_SHOTGUN) {
                            ee.pcom_has |= PCOM_HAS_SHOTGUN;
                        }
                        if ((event_point.Data[2] >> 16) & HAS_AK47) {
                            ee.pcom_has |= PCOM_HAS_AK47;
                        }
                        if ((event_point.Data[2] >> 16) & HAS_GRENADE) {
                            ee.pcom_has |= PCOM_HAS_GRENADE;
                        }
                        if ((event_point.Data[2] >> 16) & HAS_BALLOON) {
                            ee.pcom_has |= PCOM_HAS_BALLOON;
                        }
                        if ((event_point.Data[2] >> 16) & HAS_KNIFE) {
                            ee.pcom_has |= PCOM_HAS_KNIFE;
                        }
                        if ((event_point.Data[2] >> 16) & HAS_BAT) {
                            ee.pcom_has |= PCOM_HAS_BASEBALLBAT;
                        }

                        switch (enemy_type) {
                        case ET_CIV:
                            ed.subtype = PERSON_CIV;
                            break;

                        case ET_CIV_BALLOON:
                            ed.subtype = PERSON_CIV;
                            ee.pcom_has |= PCOM_HAS_BALLOON;
                            break;

                        case ET_SLAG:
                            ed.subtype = PERSON_SLAG_TART;
                            break;

                        case ET_UGLY_FAT_SLAG:
                            ed.subtype = PERSON_SLAG_FATUGLY;
                            break;

                        case ET_WORKMAN:
                            ed.subtype = PERSON_MECHANIC;
                            break;

                        case ET_GANG_RASTA:
                            ed.subtype = PERSON_THUG_RASTA;
                            break;

                        case ET_GANG_RED:
                            ed.subtype = PERSON_THUG_RED;
                            break;

                        case ET_GANG_GREY:
                            ed.subtype = PERSON_THUG_GREY;
                            break;

                        case ET_GANG_RASTA_PISTOL:
                            ed.subtype = PERSON_THUG_RASTA;
                            ee.pcom_has |= PCOM_HAS_GUN;
                            break;

                        case ET_GANG_RED_SHOTGUN:
                            ed.subtype = PERSON_THUG_RED;
                            ee.pcom_has |= PCOM_HAS_SHOTGUN;
                            break;

                        case ET_GANG_GREY_AK47:
                            ed.subtype = PERSON_THUG_GREY;
                            ee.pcom_has |= PCOM_HAS_AK47;
                            break;

                        case ET_COP:
                            ed.subtype = PERSON_COP;
                            break;

                        case ET_COP_PISTOL:
                            ed.subtype = PERSON_COP;
                            ee.pcom_has |= PCOM_HAS_GUN;
                            break;

                        case ET_COP_SHOTGUN:
                            ed.subtype = PERSON_COP;
                            ee.pcom_has |= PCOM_HAS_SHOTGUN;
                            break;

                        case ET_COP_AK47:
                            ed.subtype = PERSON_COP;
                            ee.pcom_has |= PCOM_HAS_AK47;
                            break;

                        case ET_HOSTAGE:
                            ed.subtype = PERSON_HOSTAGE;
                            break;

                        case ET_WORKMAN_GRENADE:
                            ed.subtype = PERSON_MECHANIC;
                            ee.pcom_has |= PCOM_HAS_GRENADE;
                            break;
                        case ET_TRAMP:
                            ed.subtype = PERSON_TRAMP;
                            break;

                        case ET_MIB1:
                            ed.subtype = PERSON_MIB1;
                            break;
                        case ET_MIB2:
                            ed.subtype = PERSON_MIB2;
                            break;
                        case ET_MIB3:
                            ed.subtype = PERSON_MIB3;
                            break;

                        case ET_NONE:
                            ed.subtype = PERSON_CIV;
                            break;

                        case ET_DARCI:
                            ed.subtype = PERSON_DARCI;
                            break;

                        case ET_ROPER:
                            ed.subtype = PERSON_ROPER;
                            break;

                        default:
                            ASSERT(0);
                            break;
                        }

                        ee.pcom_ai = LOWORD(event_point.Data[5]);
                        ee.ai_skill = HIWORD(event_point.Data[5]);
                        ee.pcom_bent = event_point.Data[4];
                        ee.pcom_move = event_point.Data[3] + 1;
                        ee.ai_other = event_point.Data[7] & 0xffff;
                        ee.follow = follow;
                        ee.zone = event_point.Data[4] >> 8;

                        people_types[ed.subtype]++;

                        if (ed.subtype == PERSON_MIB1 || ed.subtype == PERSON_MIB2 || ed.subtype == PERSON_MIB3) {
                            // MIB are never low-skilled — enforce a minimum skill floor.
                            ee.ai_skill = 8 + (ee.ai_skill >> 1);
                        }

                        if (ee.pcom_ai == PCOM_AI_FIGHT_TEST) {
                            // For fight-test dummies: ai_other is the combat move that KOs them.
                            ee.ai_other = event_point.Data[7] >> 16;
                            // Fight test dummies are always invulnerable (zone bit 4).
                            ee.zone |= 1 << 4;
                        }

                        ee.drop = 0;

                        for (j = 0; j < 32; j++) {
                            if (event_point.Data[8] & (1 << j)) {
                                ee.drop = j + 1;
                            }
                        }

                        break;

                    case WPT_CREATE_VEHICLE:

                        ed.type = EWAY_DO_CREATE_VEHICLE;
                        ed.arg1 = 0;

                        switch (event_point.Data[3]) {
                        case VK_UNLOCKED:
                            ed.arg1 = SPECIAL_NONE;
                            break;
                        case VK_RED:
                            ed.arg1 = SPECIAL_KEY;
                            break;
                        case VK_BLUE:
                            ed.arg1 = SPECIAL_KEY;
                            break;
                        case VK_GREEN:
                            ed.arg1 = SPECIAL_KEY;
                            break;
                        case VK_BLACK:
                            ed.arg1 = SPECIAL_KEY;
                            break;
                        case VK_WHITE:
                            ed.arg1 = SPECIAL_KEY;
                            break;
                        case VK_LOCKED:
                            ed.arg1 = SPECIAL_NUM_TYPES;
                            break;

                        default:
                            ASSERT(0);
                            break;
                        }

                        switch (event_point.Data[0]) {
                        case VT_NONE:
                        case VT_CAR:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_CAR;
                            break;

                        case VT_VAN:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_VAN;
                            break;

                        case VT_TAXI:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_TAXI;
                            break;

                        case VT_HELICOPTER:

                            ed.subtype = EWAY_SUBTYPE_VEHICLE_HELICOPTER;

                            if (event_point.Data[1] == VMT_TRACK_TARGET) {
                                ed.arg2 = event_point.Data[2];
                            }

                            break;

                        case VT_BIKE:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_BIKE;
                            break;

                        case VT_POLICE:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_POLICE;
                            break;

                        case VT_AMBULANCE:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_AMBULANCE;
                            break;

                        case VT_JEEP:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_JEEP;
                            break;

                        case VT_MEATWAGON:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_MEATWAGON;
                            break;

                        case VT_SEDAN:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_SEDAN;
                            break;

                        case VT_WILDCATVAN:
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_WILDCATVAN;
                            break;

                        default:
                            ASSERT(0);
                            break;
                        }

                        break;

                    case WPT_CREATE_ITEM:

                        if (version < 4) {
                            // Old missions used a different item list; normalise to pistol.
                            event_point.Data[0] = IT_PISTOL;
                        }

                        ed.type = EWAY_DO_CREATE_ITEM;
                        ed.arg1 = event_point.Data[2]; // flags

                        switch (event_point.Data[0]) {
                        default:
                        case IT_NONE:

                        case IT_KEY:
                            ed.subtype = SPECIAL_KEY;
                            break;
                        case IT_PISTOL:
                            ed.subtype = SPECIAL_GUN;
                            break;
                        case IT_HEALTH:
                            ed.subtype = SPECIAL_HEALTH;
                            break;
                        case IT_SHOTGUN:
                            ed.subtype = SPECIAL_SHOTGUN;
                            break;
                        case IT_KNIFE:
                            ed.subtype = SPECIAL_KNIFE;
                            break;
                        case IT_AK47:
                            ed.subtype = SPECIAL_AK47;
                            break;
                        case IT_MINE:
                            ed.subtype = SPECIAL_MINE;
                            break;
                        case IT_BASEBALLBAT:
                            ed.subtype = SPECIAL_BASEBALLBAT;
                            break;
                        case IT_AMMO_SHOTGUN:
                            ed.subtype = SPECIAL_AMMO_SHOTGUN;
                            break;
                        case IT_AMMO_AK47:
                            ed.subtype = SPECIAL_AMMO_AK47;
                            break;
                        case IT_AMMO_PISTOL:
                            ed.subtype = SPECIAL_AMMO_PISTOL;
                            break;
                        case IT_KEYCARD:
                            ed.subtype = SPECIAL_KEYCARD;
                            break;
                        case IT_FILE:
                            ed.subtype = SPECIAL_FILE;
                            break;
                        case IT_FLOPPY_DISK:
                            ed.subtype = SPECIAL_FLOPPY_DISK;
                            break;
                        case IT_CROWBAR:
                            ed.subtype = SPECIAL_CROWBAR;
                            break;
                        case IT_VIDEO:
                            ed.subtype = SPECIAL_VIDEO;
                            break;
                        case IT_GLOVES:
                            ed.subtype = SPECIAL_GLOVES;
                            break;
                        case IT_WEEDAWAY:
                            ed.subtype = SPECIAL_WEEDAWAY;
                            break;
                        case IT_GRENADE:
                            ed.subtype = SPECIAL_GRENADE;
                            break;
                        case IT_EXPLOSIVES:
                            ed.subtype = SPECIAL_EXPLOSIVES;
                            break;
                        case IT_WIRE_CUTTER:
                            ed.type = EWAY_DO_END_OF_WORLD;
                            break;

                        case IT_BARREL:

                            // Barrels placed unconditionally — skip waypoint creation.
                            for (j = 0; j < event_point.Data[1]; j++) {

                                BARREL_alloc(
                                    BARREL_TYPE_NORMAL,
                                    PRIM_OBJ_BARREL,
                                    ew_world_x + (Random() & 0xf) - 0x7,
                                    ew_world_z + (Random() & 0xf) - 0x7,
                                    NULL);
                            }

                            goto dont_create_a_waypoint;
                        }

                        break;

                    case WPT_CREATE_CREATURE:
                        ed.type = EWAY_DO_CREATE_ANIMAL;
                        ed.subtype = event_point.Data[0];
                        break;

                    case WPT_CREATE_CAMERA:

                        ed.type = EWAY_DO_CAMERA_CREATE;

                        ew_speed = event_point.Data[2] >> 2;
                        ew_delay = event_point.Data[3];

                        SATURATE(ew_speed, 4, 255);

                        ed.arg1 = ew_speed;
                        ed.arg2 = ew_delay;

                        ed.subtype = 0;

                        if (event_point.Data[4]) {
                            ed.subtype |= EWAY_SUBTYPE_CAMERA_LOCK_PLAYER;
                        }
                        if (event_point.Data[5]) {
                            ed.subtype |= EWAY_SUBTYPE_CAMERA_LOCK_DIRECTION;
                        }
                        if (event_point.Data[6]) {
                            ed.subtype |= EWAY_SUBTYPE_CAMERA_CANT_INTERRUPT;
                        }

                        break;

                    case WPT_CREATE_TARGET:

                        ed.type = EWAY_DO_CAMERA_TARGET;

                        switch (event_point.Data[1]) {
                        case NULL:
                        case CT_NORMAL:
                            ed.subtype = EWAY_SUBTYPE_CAMERA_TARGET_PLACE;
                            break;

                        case CT_ATTACHED:
                            ed.subtype = EWAY_SUBTYPE_CAMERA_TARGET_THING;
                            break;

                        case CT_NEAREST_LIVING:
                            ed.subtype = EWAY_SUBTYPE_CAMERA_TARGET_NEAR;
                            break;

                        default:
                            ASSERT(0);
                            break;
                        }

                        break;

                    case WPT_CREATE_MAP_EXIT:
                        break;

                    case WPT_CAMERA_WAYPOINT:

                        ed.type = EWAY_DO_CAMERA_WAYPOINT;

                        ew_speed = event_point.Data[2] >> 2;
                        ew_delay = event_point.Data[3];

                        if (ew_speed < 4) {
                            ew_speed = 4;
                        }

                        ed.arg1 = ew_speed;
                        ed.arg2 = ew_delay;

                        // Camera waypoint waypoints must use the CAMERA_AT condition.
                        ecd.type = EWAY_COND_CAMERA_AT;
                        ecd.arg1 = NULL;
                        ecd.arg2 = NULL;
                        ecd.negate = UC_FALSE;

                        break;

                    case WPT_TARGET_WAYPOINT:
                        break;

                    case WPT_MESSAGE:
                        ed.type = EWAY_DO_MESSAGE;
                        ed.arg1 = mess_count++;
                        ed.arg2 = event_point.Data[2];
                        ed.subtype = event_point.Data[1];
                        break;

                    case WPT_NAV_BEACON:
                        ed.type = EWAY_DO_NAV_BEACON;
                        ed.arg1 = mess_count++;
                        ed.arg2 = event_point.Data[1];
                        break;

                    case WPT_SOUND_EFFECT:
                        ed.type = EWAY_DO_SOUND_EFFECT;
                        ed.subtype = event_point.Data[0];
                        ed.arg1 = event_point.Data[1];
                        break;

                    case WPT_VISUAL_EFFECT:
                        ed.type = EWAY_DO_EXPLODE;
                        ed.subtype = event_point.Data[0];
                        ed.arg1 = event_point.Data[1];
                        break;

                    case WPT_SPOT_EFFECT:
                        // Spot effects are auto-generated at load time; skip waypoint creation.
                        goto dont_create_a_waypoint;

                        /*
                        ed.type    = EWAY_DO_SPOT_FX;
                        ed.subtype = event_point.Data[0];
                        ed.arg1    = event_point.Data[1];
                        */

                        break;

                    case WPT_TELEPORT:
                        break;

                    case WPT_TELEPORT_TARGET:
                        break;

                    case WPT_END_GAME_LOSE:
                        ed.type = EWAY_DO_MISSION_FAIL;
                        break;

                    case WPT_END_GAME_WIN:
                        ed.type = EWAY_DO_MISSION_COMPLETE;
                        break;

                    case WPT_SHOUT:
                        break;

                    case WPT_ACTIVATE_PRIM:

                        switch (event_point.Data[0]) {
                        case AP_DOOR:
                            ed.type = EWAY_DO_CONTROL_DOOR;
                            break;

                        case AP_ELECTRIC_FENCE:
                            ed.type = EWAY_DO_ELECTRIFY_FENCE;
                            break;

                        case AP_SECURITY_CAMERA:
                            break;

                        default:
                            ASSERT(0);
                            break;
                        }

                        break;

                    case WPT_CREATE_TRAP:

                        ed.type = EWAY_DO_EMIT_STEAM;
                        ed.subtype = event_point.Data[4];
                        ed.arg1 = event_point.Data[3];
                        ed.arg2 = 0;
                        ed.arg2 |= (event_point.Data[1] & 0x3f) << 10;
                        ed.arg2 |= (event_point.Data[2] & 0x0f) << 6;
                        ed.arg2 |= (event_point.Data[5] & 0x3f) << 0;

                        break;

                    case WPT_ADJUST_ENEMY:

                        ed.type = EWAY_DO_CHANGE_ENEMY;
                        ed.arg1 = event_point.Data[6];
                        ee.pcom_ai = event_point.Data[5];
                        ee.pcom_bent = event_point.Data[4];
                        ee.pcom_move = event_point.Data[3] + 1;
                        ee.ai_other = event_point.Data[7];

                        if (ee.pcom_ai == PCOM_AI_FIGHT_TEST) {
                            ee.ai_other = event_point.Data[7] >> 16;
                            ee.zone |= 1 << 4;
                        }

                        if (version >= 3) {
                            ee.follow = event_point.Data[1];
                        } else {
                            ee.follow = NULL;
                        }

                        break;

                    case WPT_ENEMY_FLAGS:
                        ed.type = EWAY_DO_CHANGE_ENEMY_FLG;
                        ed.arg1 = event_point.Data[0];
                        ed.arg2 = event_point.Data[4];
                        break;

                    // ed.arg1 = speed (default 50 if Data[0]==0).
                    // ed.arg2 = PLAT_FLAG_* bits: LOCK_MOVE, LOCK_ROT, BODGE_ROCKET.
                    case WPT_LINK_PLATFORM:

                        ed.type = EWAY_DO_CREATE_PLATFORM;

                        if (event_point.Data[0] == 0) {
                            ed.arg1 = 50;
                        } else {
                            ed.arg1 = event_point.Data[0];
                        }

                        if (event_point.Data[1] & LP_LOCK_TO_AXIS) {
                            ed.arg2 = PLAT_FLAG_LOCK_MOVE;
                        }
                        if (event_point.Data[1] & LP_LOCK_ROTATION) {
                            ed.arg2 = PLAT_FLAG_LOCK_ROT;
                        }
                        if (event_point.Data[1] & LP_BODGE_ROCKET) {
                            ed.arg2 = PLAT_FLAG_BODGE_ROCKET;
                        }

                        break;

                    case WPT_CREATE_BOMB:
                        ed.type = EWAY_DO_CREATE_BOMB;
                        break;

                    case WPT_CREATE_BARREL:

                    {
                        UWORD barrel_type;
                        UWORD prim;

                        switch (event_point.Data[0]) {
                        case BT_OIL_DRUM:

                            extern SLONG playing_level(const CBYTE* name);

                            if (playing_level("Semtex.ucm")) {
                                barrel_type = BARREL_TYPE_NORMAL;
                                prim = 145;

                                break;
                            } else {
                                // Fallthrough!
                            }

                        case BT_BARREL:
                        case BT_LOX_DRUM:
                            barrel_type = BARREL_TYPE_NORMAL;
                            prim = PRIM_OBJ_BARREL;
                            break;

                        case BT_TRAFFIC_CONE:
                            barrel_type = BARREL_TYPE_CONE;
                            prim = PRIM_OBJ_TRAFFIC_CONE;
                            break;

                        case BT_BURNING_BARREL:
                            barrel_type = BARREL_TYPE_BURNING;
                            prim = PRIM_OBJ_BARREL;
                            break;

                        case BT_BURNING_BIN:
                        case BT_BIN:
                            barrel_type = BARREL_TYPE_BIN;
                            prim = PRIM_OBJ_BIN;
                            break;

                        default:
                            ASSERT(0);
                            break;
                        }

                        if (!(event_point.Flags & WPT_FLAGS_REFERENCED) && ecd.type == EWAY_COND_TRUE) {
                            BARREL_alloc(
                                barrel_type,
                                prim,
                                ew_world_x,
                                ew_world_z,
                                NULL);

                            goto dont_create_a_waypoint;
                        } else {
                            ed.type = EWAY_DO_CREATE_BARREL;
                            ed.subtype = barrel_type;
                            ed.arg2 = prim;
                        }
                    }

                    break;

                    case WPT_KILL_WAYPOINT:
                        ed.type = EWAY_DO_KILL_WAYPOINT;
                        ed.arg1 = event_point.Data[0];
                        break;

                    case WPT_CREATE_TREASURE:
                        ed.type = EWAY_DO_CREATE_ITEM;
                        ed.subtype = SPECIAL_TREASURE;
                        break;

                    case WPT_BONUS_POINTS:

                        if (0) {
                            // Dead code: bonus points were converted to plain messages before ship.
                            ed.type = EWAY_DO_OBJECTIVE;
                            ed.subtype = EWAY_SUBTYPE_OBJECTIVE_SUB;
                            ed.arg1 = mess_count++;
                            ed.arg2 = event_point.Data[1] / 10;
                        } else {
                            ed.type = EWAY_DO_MESSAGE;
                            ed.arg1 = mess_count++;
                            ed.arg2 = NULL;
                            ed.subtype = 0;
                        }

                        break;

                    case WPT_GROUP_LIFE:
                        ed.type = EWAY_DO_GROUP_LIFE;
                        break;

                    case WPT_GROUP_DEATH:
                        ed.type = EWAY_DO_GROUP_DEATH;
                        break;

                    case WPT_CONVERSATION:

                        if (event_point.Data[3]) {
                            ed.type = EWAY_DO_AMBIENT_CONV;
                        } else {
                            ed.type = EWAY_DO_CONVERSATION;
                        }

                        ed.subtype = mess_count++;
                        ed.arg1 = event_point.Data[1];
                        ed.arg2 = event_point.Data[2];
                        break;

                    case WPT_INCREMENT:
                        ed.type = EWAY_DO_INCREASE_COUNTER;
                        ed.subtype = event_point.Data[1];
                        break;

                    case WPT_TRANSFER_PLAYER:
                        ed.type = EWAY_DO_TRANSFER_PLAYER;
                        ed.arg1 = event_point.Data[0];
                        break;

                    case WPT_AUTOSAVE:
                        ed.type = EWAY_DO_AUTOSAVE;
                        break;

                    case WPT_MAKE_SEARCHABLE:

                        {
                            OB_Info* oi = OB_find_index(
                                ew_world_x,
                                ew_world_y,
                                ew_world_z,
                                0x100,
                                UC_FALSE);

                            if (oi) {
                                OB_ob[oi->index].flags |= OB_FLAG_SEARCHABLE;
                            }
                        }

                        goto dont_create_a_waypoint;

                    case WPT_LOCK_VEHICLE:
                        ed.type = EWAY_DO_LOCK_VEHICLE;
                        ed.arg1 = event_point.Data[0];

                        if (event_point.Data[1]) {
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_LOCK;
                        } else {
                            ed.subtype = EWAY_SUBTYPE_VEHICLE_UNLOCK;
                        }

                        break;

                    case WPT_CUT_SCENE: {
                        ed.type = EWAY_DO_CUTSCENE;
                        ed.arg1 = cutscene_count++;
                    } break;
                    case WPT_GROUP_RESET:
                        ed.type = EWAY_DO_GROUP_RESET;
                        break;

                    case WPT_COUNT_UP_TIMER:
                        ed.type = EWAY_DO_VISIBLE_COUNT_UP;
                        break;

                    case WPT_RESET_COUNTER:
                        ed.type = EWAY_DO_RESET_COUNTER;
                        ed.subtype = event_point.Data[0];
                        break;

                    case WPT_CREATE_MIST:
                        ed.type = EWAY_DO_CREATE_MIST;
                        break;

                    case WPT_STALL_CAR:
                        ed.type = EWAY_DO_STALL_CAR;
                        ed.arg1 = event_point.Data[0];
                        break;

                    case WPT_EXTEND:
                        ed.type = EWAY_DO_EXTEND_COUNTDOWN;
                        ed.arg1 = event_point.Data[0];
                        ed.arg2 = event_point.Data[1];
                        break;

                    case WPT_MOVE_THING:
                        ed.type = EWAY_DO_MOVE_THING;
                        ed.arg1 = event_point.Data[0];
                        break;

                    case WPT_MAKE_PERSON_PEE:
                        ed.type = EWAY_DO_MAKE_PERSON_PEE;
                        ed.arg1 = event_point.Data[0];
                        break;

                    case WPT_CONE_PENALTIES:
                        ed.type = EWAY_DO_CONE_PENALTIES;
                        break;

                    case WPT_SIGN:
                        ed.type = EWAY_DO_SIGN;
                        ed.arg1 = event_point.Data[0];
                        ed.arg2 = event_point.Data[1];
                        break;

                    case WPT_WAREFX:
                        ed.type = EWAY_DO_WAREFX;
                        ed.arg1 = event_point.Data[0];
                        break;

                    case WPT_NO_FLOOR:
                        ed.type = EWAY_DO_NO_FLOOR;
                        break;

                    case WPT_SHAKE_CAMERA:
                        ed.type = EWAY_DO_SHAKE_CAMERA;
                        break;

                    default:
                        ASSERT(0);
                        break;
                    }

                    // Map OnTrigger (OT_*) to EWAY_STAY_* stay rules.
                    switch (event_point.OnTrigger) {
                    case OT_NONE:
                    case OT_ACTIVE:
                        es.type = EWAY_STAY_ALWAYS;
                        break;

                    case OT_ACTIVE_WHILE:
                        es.type = EWAY_STAY_WHILE;
                        break;

                    case OT_ACTIVE_TIME:
                        es.type = EWAY_STAY_TIME;
                        es.arg = event_point.AfterTimer;
                        break;

                    case OT_ACTIVE_DIE:
                        es.type = EWAY_STAY_DIE;
                        break;

                    default:
                        ASSERT(0);
                        break;
                    }

                    // Unreferenced PERSON_USED+MESSAGE waypoints should re-trigger every time
                    // the player interacts, not just once.
                    if (ecd.type == EWAY_COND_PERSON_USED && ed.type == EWAY_DO_MESSAGE) {
                        if (!(event_point.Flags & WPT_FLAGS_REFERENCED)) {
                            es.type = EWAY_STAY_WHILE;
                        }
                    }

                    EWAY_create(
                        i,
                        ew_colour,
                        ew_group,
                        ew_world_x,
                        ew_world_y,
                        ew_world_z,
                        ew_yaw,
                        &ecd,
                        &ed,
                        &es,
                        &ee,
                        !(event_point.Flags & WPT_FLAGS_REFERENCED),
                        kludge_index,
                        event_point.Data[9]);

                // Skip EWAY waypoint creation for EventPoints that directly place objects
                // (barrels, spot effects, searchable obs).
                dont_create_a_waypoint:;

                    continue;
                } else {
                    continue;
                }
            }
        }

        // Skip over per-skill-level data blocks added in version 6+.
        if (version > 5)
            FileSeek(handle, SEEK_MODE_CURRENT, 254);
        FileRead(handle, &BOREDOM_RATE, 1);
        if (version < 10)
            BOREDOM_RATE = 4;

        if (BOREDOM_RATE < 4)
            BOREDOM_RATE = 4;

        if (version > 8) {
            FileRead(handle, &FAKE_CARS, 1);
            FileRead(handle, &MUSIC_WORLD, 1);
            if (MUSIC_WORLD < 1)
                MUSIC_WORLD = 1;
        }

        // Load message strings and cutscene data.
        // Versions before 8 store only messages; version 8+ interleaves cutscenes (type byte: 1=msg, 2=cutscene).
        if (version < 8) {

            for (i = 0; i < mess_count; i++) {
                SLONG l;

                memset(junk, 0, sizeof(junk));

                if (version > 4)
                    FileRead(handle, &l, 4);
                else
                    l = _MAX_PATH;
                if (FileRead(handle, junk, l) == FILE_READ_ERROR)
                    goto file_error;

                EWAY_set_message(i, junk);
            }

        } else {

            for (i = 0; i < mess_count + cutscene_count; i++) {
                SLONG l;
                UBYTE what;

                FileRead(handle, &what, 1);

                switch (what) {
                case 1: // message
                    memset(junk, 0, sizeof(junk));
                    FileRead(handle, &l, 4);
                    if (FileRead(handle, junk, l) == FILE_READ_ERROR)
                        goto file_error;
                    EWAY_set_message(i, junk);
                    break;
                case 2: // cutscene
                    PLAYCUTS_Read(handle);
                    break;
                }
            }
        }

        // Load zone flags (version >= 2): 128x128 grid of ZF_* bitmask bytes.
        // Each byte maps to PAP_FLAG_ZONE1-4, WANDER, and NOGO flags on the corresponding hi-res square.
        if (version >= 2) {

            for (x = 0; x < 128; x++) {
                if (FileRead(handle, junk, 128) == FILE_READ_ERROR)
                    goto file_error;

                for (z = 0; z < 128; z++) {
                    PAP_2HI(x, z).Flags &= ~PAP_FLAG_ZONE1;
                    PAP_2HI(x, z).Flags &= ~PAP_FLAG_ZONE2;
                    PAP_2HI(x, z).Flags &= ~PAP_FLAG_ZONE3;
                    PAP_2HI(x, z).Flags &= ~PAP_FLAG_ZONE4;

                    if (junk[z] & ZF_ZONE1) {
                        PAP_2HI(x, z).Flags |= PAP_FLAG_ZONE1;
                    }
                    if (junk[z] & ZF_ZONE2) {
                        PAP_2HI(x, z).Flags |= PAP_FLAG_ZONE2;
                    }
                    if (junk[z] & ZF_ZONE3) {
                        PAP_2HI(x, z).Flags |= PAP_FLAG_ZONE3;
                    }
                    if (junk[z] & ZF_ZONE4) {
                        PAP_2HI(x, z).Flags |= PAP_FLAG_ZONE4;
                    }

                    if (junk[z] & ZF_NO_WANDER) {
                        if (PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN) {
                            // For hidden squares the WANDER flag means something else; don't touch it.
                        } else {
                            PAP_2HI(x, z).Flags &= ~PAP_FLAG_WANDER;
                        }
                    }

                    if (junk[z] & ZF_NO_GO) {
                        void MAV_turn_off_whole_square(
                            SLONG x,
                            SLONG z);

                        void MAV_turn_off_whole_square_car(
                            SLONG x,
                            SLONG z);

                        MAV_turn_off_whole_square(x, z);
                        MAV_turn_off_whole_square_car(x, z);

                        PAP_2HI(x, z).Flags |= PAP_FLAG_NOGO;
                    } else {
                        PAP_2HI(x, z).Flags &= ~PAP_FLAG_NOGO;
                    }
                }
            }
        }

        FileClose(handle);

        // Override MUSIC_WORLD from an optional side-car text file (levels\mworlds.txt).
        // Format: one line per level — "levelname: N" where N is the world index.
        {
            FILE* handle = MF_Fopen("levels/mworlds.txt", "rb");

            if (handle) {
                CBYTE line[256];
                CBYTE levname[64];
                SLONG match;
                SLONG mworld;

                CBYTE* ch;
                CBYTE* blah;

                for (ch = fname_level; *ch && *ch != '\\' && *ch != '/'; ch++)
                    ;

                ch += 1;
                blah = levname;

                while (*ch != '.') {
                    *blah++ = *ch++;
                }

                *blah++ = ':';
                *blah++ = ' ';
                *blah++ = '%';
                *blah++ = 'd';
                *blah++ = '\000';

                oc_strlwr(levname);

                while (fgets(line, 256, handle)) {
                    oc_strlwr(line);

                    match = sscanf(line, levname, &mworld);

                    if (match == 1) {
                        MUSIC_WORLD = mworld;

                        break;
                    }
                }

                MF_Fclose(handle);
            }
        }

        EWAY_created_last_waypoint();
        EWAY_work_out_which_ones_are_in_warehouses();

        // Spawn fake background traffic cars (FAKE_CARS count from .ucm).
        // Half that count on software-render devices to save performance.
        extern SLONG WAND_find_good_start_point_for_car(SLONG * posx, SLONG * posz, SLONG * yaw, SLONG anywhere);

        if (FAKE_CARS)
            if (!ge_is_hardware()) {
                FAKE_CARS = (FAKE_CARS + 1) >> 1;
            }

        for (i = 0; i < FAKE_CARS; i++) {
            SLONG x, z, yaw;
            SLONG watchdog;

            watchdog = 16;
            while (!WAND_find_good_start_point_for_car(&x, &z, &yaw, 1)) {
                if (!--watchdog)
                    break;
            }
            if (!watchdog)
                continue;

            SLONG ix = PCOM_create_person(
                PERSON_CIV,
                0,
                0,
                PCOM_AI_DRIVER,
                0,
                0,
                PCOM_MOVE_WANDER,
                0,
                0,
                0,
                0,
                0,
                x << 8,
                0,
                z << 8,
                0,
                0,
                0,
                FLAG2_PERSON_FAKE_WANDER);

            TO_THING(ix)->Genus.Person->sewerbits = (Random() % 20) + 20;

            SLONG type = (Random() >> 4) & 15;

            SLONG p_index = VEH_create(
                x << 8,
                0,
                z << 8,
                yaw,
                0,
                0,
                vehicle_random[type],
                0,
                Random());

            WMOVE_create(TO_THING(p_index));
        }

        // Set water-related PAP flags.
        // lo-res squares near hi-res WATER squares get PAP_LO_NO_WATER cleared to the hard-coded water level.
        for (x = 0; x < PAP_SIZE_LO; x++)
            for (z = 0; z < PAP_SIZE_LO; z++) {
                PAP_2LO(x, z).water = PAP_LO_NO_WATER;

                for (x = 1; x < PAP_SIZE_HI - 1; x++)
                    for (z = 1; z < PAP_SIZE_HI - 1; z++) {
                        if (PAP_2HI(x, z).Flags & PAP_FLAG_WATER) {
                            PAP_2HI(x + 0, z + 0).Flags |= PAP_FLAG_SINK_POINT | PAP_FLAG_WATER;
                            PAP_2HI(x + 1, z + 0).Flags |= PAP_FLAG_SINK_POINT;
                            PAP_2HI(x + 0, z + 1).Flags |= PAP_FLAG_SINK_POINT;
                            PAP_2HI(x + 1, z + 1).Flags |= PAP_FLAG_SINK_POINT;

                            PAP_2LO(x >> 2, z >> 2).water = water_level >> 3;
                        }
                    }

                for (x = 0; x < PAP_SIZE_LO - 1; x++)
                    for (z = 0; z < PAP_SIZE_LO - 1; z++) {
                        for (oi = OB_find(x, z); oi->prim; oi += 1) {
                            if (PAP_2HI(oi->x >> 8, oi->z >> 8).Flags & PAP_FLAG_WATER) {
                                oi->y = water_level - get_prim_info(oi->prim)->miny;
                            }
                        }
                    }
            }
    } else {
    // Any failed FileRead() jumps here: set load_ok = UC_FALSE and close the file.
    file_error:;

        load_ok = UC_FALSE;

        if (handle) {
            FileClose(handle);
        }
    }

    // If level failed to load, place the player(s) at the centre of the map.
    if (!load_ok) {
        if (NO_PLAYERS == 0)
            NO_PLAYERS = 1;

        x = 64 << 8;
        z = 64 << 8;

        for (i = 0; i < NO_PLAYERS; i++) {
            angle = (2048 * i) / NO_PLAYERS;

            tx = x + (SIN(angle) >> 9);
            tz = z + (COS(angle) >> 9);

            ty = PAP_calc_height_at(tx, tz);

            NET_PERSON(i) = create_player(
                PLAYER_DARCI,
                tx, ty, tz,
                i);
        }
    } else {
        void load_level_anim_prims(void);
        load_level_anim_prims();
    }
}

// uc_orig: ELEV_game_init_common (fallen/Source/elev.cpp)
void ELEV_game_init_common(
    CBYTE* fname_map,
    CBYTE* fname_lighting,
    CBYTE* fname_citsez,
    CBYTE* fname_level)
{
    extern void SND_BeginAmbient();
    MFX_load_wave_list();
    SND_BeginAmbient();
}

// Full game world initialisation: loads map, lighting, entities, nav data.
// Called both from ELEV_load_name (via .ucm) and directly from ELEV_load_user
// (when individual map/lighting/citsez files are selected separately).
// Returns UC_TRUE on success.
// uc_orig: ELEV_game_init (fallen/Source/elev.cpp)
SLONG ELEV_game_init(
    CBYTE* fname_map,
    CBYTE* fname_lighting,
    CBYTE* fname_citsez,
    CBYTE* fname_level)
{
    SLONG i;

    /*

    extern UWORD darci_dlight;

    darci_dlight = 0;

    */

    ATTRACT_loadscreen_draw(10 * 256 / 100);

    strcpy(ELEV_last_map_loaded, fname_map);

    //	revert_to_prim_status();

    mark_prim_objects_as_unloaded();

    void global_load(void);
    global_load();

    ATTRACT_loadscreen_draw(15 * 256 / 100);

    extern SLONG kick_or_punch; // This is in person.cpp

    kick_or_punch = 0;

    void init_anim_prims(void);
    void init_overlay(void);

    init_overlay();
    WMOVE_init();
    PCOM_init();
    init_things();
    init_draw_meshes();
    //	init_draw_tweens(); // this is in global_load()

    init_anim_prims();
    init_persons();
    init_choppers();
    init_pyros();
    //	init_furniture();
    init_vehicles();
    init_projectiles();
    init_specials();
    BAT_init();
    void init_gangattack(void);
    init_gangattack();
    init_map();
    init_user_interface();

    ATTRACT_loadscreen_draw(20 * 256 / 100);

    SOUND_reset();
    PARTICLE_Reset();
    //	TRACKS_Reset();
    TRACKS_InitOnce();
    RIBBON_init();
    load_palette("data/tex01.pal");

    extern CBYTE PANEL_wide_text[256];
    extern THING_INDEX PANEL_wide_top_person;
    extern THING_INDEX PANEL_wide_bot_person;

    PANEL_wide_text[0] = 0;
    PANEL_wide_top_person = NULL;
    PANEL_wide_bot_person = NULL;

    load_animtmaps();

    ATTRACT_loadscreen_draw(25 * 256 / 100);

    FC_init();
    BARREL_init();
    BALLOON_init();
    NIGHT_init();
    OB_init();
    TRIP_init();
    FOG_init();
    MIST_init();
    //	WATER_init();
    PUDDLE_init();
    //	AZ_init();
    DRIP_init();
    //	BANG_init();
    GLITTER_init();
    POW_init();
    SPARK_init();
    CONSOLE_clear();
    PLAT_init();
    MAV_init();
    PANEL_new_text_init();

    ATTRACT_loadscreen_draw(35 * 256 / 100);

    void init_ambient(void);
    init_ambient();

    extern void MAP_beacon_init();
    extern void MAP_pulse_init();

    MAP_pulse_init();
    MAP_beacon_init();

    load_game_map(fname_map);

    calc_prim_info();

    ATTRACT_loadscreen_draw(50 * 256 / 100);

    build_quick_city();

    // tum te tum
    init_animals();

    // what? function call? i din... oh, _that_ function call

    DIRT_init(100, 1, 0, UC_INFINITY, UC_INFINITY, UC_INFINITY, UC_INFINITY);

    OB_convert_dustbins_to_barrels();
    ROAD_sink();
    ROAD_wander_calc();
    WAND_init();
    WARE_init();
    MAV_precalculate();
    extern void BUILD_car_facets();
    BUILD_car_facets();
    SHADOW_do();
    //	AZ_create_lines();
    COLLIDE_calc_fastnav_bits();
    COLLIDE_find_seethrough_fences();
    clear_all_wmove_flags();
    InitGrenades();

    // now the map's loaded we can precalc audio polys, if req'd
    SOUND_SewerPrecalc();

    if (fname_lighting == NULL || !NIGHT_load_ed_file(fname_lighting)) {
        NIGHT_ambient(255, 255, 255, 110, -148, -177);
    }

    NIGHT_generate_walkable_lighting();

    if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME)) {
        GAME_FLAGS |= GF_RAINING;
    }

    ATTRACT_loadscreen_draw(60 * 256 / 100);

    ELEV_load_level(fname_level);

    ATTRACT_loadscreen_draw(65 * 256 / 100);

    TEXTURE_fix_prim_textures();
    //	TEXTURE_fix_texture_styles();

    AENG_create_dx_prim_points();

    extern void envmap_specials(void);

    envmap_specials();

    calc_prim_info();
    calc_prim_normals();
    find_anim_prim_bboxes();

    loading_screen_active = UC_TRUE;

    if (!quick_load) {
        TEXTURE_load_needed(fname_level, 0, 256, 400);

        extern void PACK_do(void);

        // PACK_do();
    }

    loading_screen_active = UC_FALSE;

    EWAY_process(); // pre process map, stick it here Or we get stack overflow
    //	MUSIC_WORLD=(Random()%6)+1;

    ELEV_game_init_common(fname_map, fname_lighting, fname_citsez, fname_level);

    ATTRACT_loadscreen_draw(95 * 256 / 100);

    EWAY_load_fake_wander_text(fname_citsez);

    OB_make_all_the_switches_be_at_the_proper_height();

    OB_add_walkable_faces();

    calc_slide_edges();

    ATTRACT_loadscreen_draw(100 * 256 / 100);

    if (CNET_network_game) {
        FC_look_at(0, THING_NUMBER(NET_PERSON(PLAYER_ID)));
    } else {

        for (i = 0; i < NO_PLAYERS; i++) {
            if (NET_PERSON(i)) {
                FC_look_at(i, THING_NUMBER(NET_PERSON(i)));
                FC_setup_initial_camera(i);
            }
        }
    }

    OB_Mapwho* om;
    OB_Ob* oo;
    SLONG num;
    SLONG index;
    SLONG j;

    for (i = 0; i < OB_SIZE - 1; i++)
        for (j = 0; j < OB_SIZE - 1; j++) {
            om = &OB_mapwho[i][j];
            index = om->index;
            num = om->num;
            while (num--) {
                ASSERT(WITHIN(index, 1, OB_ob_upto - 1));
                oo = &OB_ob[index];
                if (oo->prim == 61)
                    MFX_play_xyz(0, S_ACIEEED, MFX_LOOPED | MFX_OVERLAP, ((i << 10) + (oo->x << 2)) << 8, oo->y << 8, ((j << 10) + (oo->z << 2)) << 8);
                index++;
            }
        }

    if (GAME_FLAGS & GF_RAINING) {
        PUDDLE_precalculate();
    }

    insert_collision_facets();

    Keys[KB_SPACE] = 0;
    Keys[KB_ENTER] = 0;
    Keys[KB_A] = 0;
    Keys[KB_Z] = 0;
    Keys[KB_X] = 0;
    Keys[KB_C] = 0;
    Keys[KB_V] = 0;
    Keys[KB_LEFT] = 0;
    Keys[KB_RIGHT] = 0;
    Keys[KB_UP] = 0;
    Keys[KB_DOWN] = 0;

    ATTRACT_loadscreen_draw(100 * 256 / 100);

    return UC_TRUE;
}

// Strips the directory prefix from src, copies the filename (without extension)
// into dest, and appends a dot followed by the given extension.
// Used to derive related filenames from a map path (e.g. "city.iam" → "city.lgt").
// uc_orig: ELEV_create_similar_name (fallen/Source/elev.cpp)
void ELEV_create_similar_name(
    CBYTE* dest,
    CBYTE* src,
    CBYTE* ext)
{
    CBYTE* ch;
    CBYTE* ci;

    for (ch = src; *ch; ch++)
        ;

    while (1) {
        if (ch == src) {
            break;
        }

        if (*ch == '\\' || *ch == '/') {
            ch++;

            break;
        }

        ch--;
    }

    ci = dest;

    while (1) {
        *ci++ = *ch++;

        if (*ch == '.' || *ch == '\000') {
            break;
        }
    }

    *ci++ = '.';

    strcpy(ci, ext);
}

// Reads map/lighting/citsez paths from the .ucm header, then calls ELEV_game_init.
// Also handles the Finale1 cutscene trigger before loading the final level.
// Returns UC_TRUE on success, UC_FALSE if file is missing or unreadable.
// uc_orig: ELEV_load_name (fallen/Source/elev.cpp)
SLONG ELEV_load_name(CBYTE* fname_level)
{
    SLONG ans;

    CBYTE* fname_map;
    CBYTE* fname_lighting;
    CBYTE* fname_citsez;

    MFFileHandle handle;

    // Play FMV now, when we have enough memory to do so!
    if (strstr(ELEV_fname_level, "Finale1.ucm")) {
        if (GAME_STATE & GS_REPLAY) {
            //
            // Don't play cutscenes on replay.
            //

        } else {
            stop_all_fx_and_music();
            { extern void video_play_cutscene(int); video_play_cutscene(2); }

            // Reshow the "loading" screen.
            ATTRACT_loadscreen_init();
        }
    }

    if (fname_level == NULL) {
        return UC_FALSE;
    }

    handle = FileOpen(fname_level);

    if (handle == FILE_OPEN_ERROR) {
        ASSERT(UC_FALSE);
        return UC_FALSE;
    }

    // ASan fix: callers (e.g. game_init on mission restart) pass ELEV_fname_level itself
    // as fname_level — strcpy of buffer into itself is undefined behavior.
    if (fname_level != ELEV_fname_level) {
        strcpy(ELEV_fname_level, fname_level);
    }

    char junk[1000];

    if (FileRead(handle, junk, sizeof(SLONG)) == FILE_READ_ERROR)
        goto file_error; // Version number
    if (FileRead(handle, junk, sizeof(SLONG)) == FILE_READ_ERROR)
        goto file_error; // Used
    if (FileRead(handle, junk, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // BriefName

    if (FileRead(handle, ELEV_fname_lighting, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // LightMapName
    if (FileRead(handle, ELEV_fname_map, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // MapName
    if (FileRead(handle, junk, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // MissionName
    if (FileRead(handle, ELEV_fname_citsez, _MAX_PATH) == FILE_READ_ERROR)
        goto file_error; // CitSezName

    FileClose(handle);

    fname_map = (ELEV_fname_map[0]) ? ELEV_fname_map : NULL;
    fname_lighting = (ELEV_fname_lighting[0]) ? ELEV_fname_lighting : NULL;
    fname_citsez = (ELEV_fname_citsez[0]) ? ELEV_fname_citsez : NULL;

    ans = ELEV_game_init(
        fname_map,
        fname_lighting,
        fname_citsez,
        fname_level);

    return ans;

file_error:;

    FileClose(handle);

    return UC_TRUE;
}

// Handles level selection from game startup: supports replay playback, mission name
// from STARTSCR_mission, or interactive Windows file dialogs for .ucm/.wad/.iam files.
// Returns 0 on failure, 4 for wad load, 5 for .ucm load success (see callers in Controls.cpp).
// uc_orig: ELEV_load_user (fallen/Source/elev.cpp)
SLONG ELEV_load_user(SLONG mission)
{
    CBYTE* fname_map;
    CBYTE* fname_lighting;
    CBYTE* fname_citsez;
    CBYTE* fname_level;
    CBYTE curr_directory[_MAX_PATH];

    MFX_QUICK_stop();
    MUSIC_mode(0);
    MUSIC_mode_process();

try_again:;

    /*
            if(mission<0)
            {
                    //
                    // bodge for publishing meeting
                    //
                            {
                                    SLONG	c0;
                                    strcpy(tab_map_name,my_mission_names[-mission]);
                                    for(c0=0;c0<strlen(tab_map_name);c0++)
                                    {
                                            if(tab_map_name[c0]=='.')
                                            {
                                                    tab_map_name[c0+1]='t';
                                                    tab_map_name[c0+2]='g';
                                                    tab_map_name[c0+3]='a';

                                                    break;
                                            }
                                    }
                            }
                    return ELEV_load_name(my_mission_names[-mission]);

            }
    */

    oc_getcwd(curr_directory, _MAX_PATH);

    if (GAME_STATE & GS_PLAYBACK) {
        UWORD c;

        // marker to indicate level name is included
        FileRead(playback_file, &c, 2);
        if (c == 1) {
            CBYTE temp[_MAX_PATH];
            // restore string
            FileRead(playback_file, &c, 2);
            FileRead(playback_file, temp, c);

            strcpy(ELEV_fname_level, curr_directory);
            if (ELEV_fname_level[strlen(ELEV_fname_level) - 1] != '\\' && ELEV_fname_level[strlen(ELEV_fname_level) - 1] != '/')
                strcat(ELEV_fname_level, "/");
            strcat(ELEV_fname_level, temp);
            return ELEV_load_name(ELEV_fname_level);

            /*
            strcpy(ELEV_fname_level,temp);
            return ELEV_load_name(&ELEV_fname_level[23]);  // The stupid thing saves the absolute address
            */
        }
    }

    // extern CBYTE* STARTSCR_mission;
    if (*STARTSCR_mission) {
        strcpy(ELEV_fname_level, STARTSCR_mission);

        if (GAME_STATE & GS_RECORD) {
            UWORD c = 1;
            CBYTE fname[_MAX_PATH], *cname;

            // marker to indicate level name is included
            FileWrite(playback_file, &c, 2);
            // store string
            strcpy(fname, ELEV_fname_level);
            cname = fname;
            if (oc_strnicmp(fname, curr_directory, strlen(curr_directory)) == 0) {
                cname += strlen(curr_directory);
                if (*cname = '\\')
                    cname++;
            }
            c = strlen(cname) + 1; // +1 is to include terminating zero
            FileWrite(playback_file, &c, 2);
            FileWrite(playback_file, cname, c);
        }

        //		MemFree(STARTSCR_mission);
        SLONG res = ELEV_load_name(STARTSCR_mission);
        *STARTSCR_mission = 0;
        return res;
    }

    // Editor file dialogs removed — game always loads by mission name.
    return UC_FALSE;
}

// Reloads the current level from the stored filename. Called when restarting a mission.
// uc_orig: reload_level (fallen/Source/elev.cpp)
void reload_level(void)
{
    ELEV_load_name(ELEV_fname_level);
}
