#ifndef MISSIONS_MISSION_H
#define MISSIONS_MISSION_H

// Mission system: EventPoint waypoints, Mission/GameMap structs,
// all waypoint/trigger/entity spawn type constants, and the mission management API.
// This is the core data model for the EWAY scripting engine.

#include "engine/platform/uc_common.h"  // BOOL, _MAX_PATH, base types

//-----------------------------------------------------------------------------
// EventPoint — a scripted game event placed in the level editor.
// Each EventPoint has a trigger condition (TT_*), an action (WPT_*),
// a dependency chain (EPRef/EPRefBool), and spatial/temporal parameters.

// uc_orig: WPT_FLAGS_SUCKS (fallen/Headers/Mission.h)
#define WPT_FLAGS_SUCKS      1
// uc_orig: WPT_FLAGS_INVERSE (fallen/Headers/Mission.h)
#define WPT_FLAGS_INVERSE    2
// uc_orig: WPT_FLAGS_INSIDE (fallen/Headers/Mission.h)
#define WPT_FLAGS_INSIDE     4
// uc_orig: WPT_FLAGS_WARE (fallen/Headers/Mission.h)
#define WPT_FLAGS_WARE       8
// uc_orig: WPT_FLAGS_REFERENCED (fallen/Headers/Mission.h)
#define WPT_FLAGS_REFERENCED 16  // Another event point references this one (set in export_mission).
// uc_orig: WPT_FLAGS_OPTIONAL (fallen/Headers/Mission.h)
#define WPT_FLAGS_OPTIONAL   32  // May be stripped on PSX if needed.

// uc_orig: MAX_EVENTPOINTS (fallen/Headers/Mission.h)
#define MAX_EVENTPOINTS 512

// uc_orig: EventPoint (fallen/Headers/Mission.h)
struct EventPoint {
    UBYTE Colour,       // Combined with Group to identify linked event sets (e.g. patrol waypoints).
          Group,        // Letter A-Z grouping; same Colour+Group = linked set.
          WaypointType, // WPT_* — what this event does when triggered.
          Used,         // Bool: whether this slot is in use.
          TriggeredBy,  // TT_* — what activates this event.
          OnTrigger,    // OT_* — how the trigger resets or continues after firing.
          Direction,    // Facing angle in degrees, scaled to byte range.
          Flags;        // WPT_FLAGS_* bitmask.

    UWORD EPRef,        // Dependency index for TT_DEPENDENCY and TT_BOOLEAN first input.
          EPRefBool,    // Dependency index for TT_BOOLEAN second input.
          AfterTimer;   // For OT_ACTIVE_TIME: delay before reset (ms).

    SLONG Data[10],     // Waypoint-type-specific payload data.
          Radius,       // For TT_RADIUS: trigger radius. Also used as time (TT_TIMER) or pointer (TT_SHOUT).
          X, Y, Z;      // World position of this event point.

    UWORD Next,         // Linked-list next index in mission free/used list.
          Prev;         // Linked-list prev index.
};

//-----------------------------------------------------------------------------
// Mission — a single level instance with its map reference, event points, and settings.

// uc_orig: MAX_MISSIONS (fallen/Headers/Mission.h)
#define MAX_MISSIONS 100

// uc_orig: MISSION_FLAG_USED (fallen/Headers/Mission.h)
#define MISSION_FLAG_USED                (1 << 0)
// uc_orig: MISSION_FLAG_SHOW_CRIMERATE (fallen/Headers/Mission.h)
#define MISSION_FLAG_SHOW_CRIMERATE      (1 << 1)
// uc_orig: MISSION_FLAG_CARS_WITH_ROAD_PRIMS (fallen/Headers/Mission.h)
#define MISSION_FLAG_CARS_WITH_ROAD_PRIMS (1 << 2)

// uc_orig: Mission (fallen/Headers/Mission.h)
struct Mission {
    CBYTE Flags;
    CBYTE BriefName[_MAX_PATH],
          LightMapName[_MAX_PATH],
          MapName[_MAX_PATH],
          MissionName[_MAX_PATH],
          CitSezMapName[_MAX_PATH]; // Was the sewer file path.
    UWORD MapIndex,
          FreeEPoints,
          UsedEPoints;
    UBYTE CrimeRate,
          CivsRate;
    EventPoint EventPoints[MAX_EVENTPOINTS];
    UBYTE SkillLevels[254]; // Up to 254 AI types supported.
    UBYTE BoredomRate;
    UBYTE CarsRate,
          MusicWorld;
};

// Older on-disk format B (without CBYTE Flags byte; used when loading old saves).
// uc_orig: OldMissionB (fallen/Headers/Mission.h)
struct OldMissionB {
    BOOL Used;
    CBYTE BriefName[_MAX_PATH],
          LightMapName[_MAX_PATH],
          MapName[_MAX_PATH],
          MissionName[_MAX_PATH],
          CitSezMapName[_MAX_PATH];
    UWORD MapIndex,
          FreeEPoints,
          UsedEPoints;
    UBYTE CrimeRate,
          CivsRate;
    EventPoint EventPoints[MAX_EVENTPOINTS];
    UBYTE SkillLevels[255]; // Up to 255 AI types supported.
};

// Oldest on-disk format (uses SewerMapName instead of CitSezMapName).
// uc_orig: OldMission (fallen/Headers/Mission.h)
struct OldMission {
    BOOL Used;
    CBYTE BriefName[_MAX_PATH],
          LightMapName[_MAX_PATH],
          MapName[_MAX_PATH],
          MissionName[_MAX_PATH],
          SewerMapName[_MAX_PATH];
    UWORD MapIndex,
          FreeEPoints,
          UsedEPoints,
          padding;
    EventPoint EventPoints[MAX_EVENTPOINTS];
};

//-----------------------------------------------------------------------------
// Zone flags — per-cell bitmask stored in MissionZones[mission][x][z].
// Controls audio reverb, NPC wandering restrictions, and indoor detection.

// uc_orig: ZF_NONE (fallen/Headers/Mission.h)
#define ZF_NONE      0
// uc_orig: ZF_INSIDE (fallen/Headers/Mission.h)
#define ZF_INSIDE    1
// uc_orig: ZF_REVERB (fallen/Headers/Mission.h)
#define ZF_REVERB    2
// uc_orig: ZF_NO_WANDER (fallen/Headers/Mission.h)
#define ZF_NO_WANDER 4
// uc_orig: ZF_ZONE1 (fallen/Headers/Mission.h)
#define ZF_ZONE1     8
// uc_orig: ZF_ZONE2 (fallen/Headers/Mission.h)
#define ZF_ZONE2     16
// uc_orig: ZF_ZONE3 (fallen/Headers/Mission.h)
#define ZF_ZONE3     32
// uc_orig: ZF_ZONE4 (fallen/Headers/Mission.h)
#define ZF_ZONE4     64
// uc_orig: ZF_NO_GO (fallen/Headers/Mission.h)
#define ZF_NO_GO     128
// uc_orig: ZF_NUM (fallen/Headers/Mission.h)
#define ZF_NUM       8

//-----------------------------------------------------------------------------
// GameMap — groups missions that share the same world map tile set.

// uc_orig: MAX_MAPS (fallen/Headers/Mission.h)
#define MAX_MAPS 20

// uc_orig: GameMap (fallen/Headers/Mission.h)
struct GameMap {
    BOOL Used;
    CBYTE MapName[_MAX_PATH];
    UWORD Missions[MAX_MISSIONS];
};

//-----------------------------------------------------------------------------
// Access macros — index-to-pointer helpers for mission/map/eventpoint pools.

// uc_orig: TO_MAP (fallen/Headers/Mission.h)
#define TO_MAP(m)              (&game_maps[m])
// uc_orig: TO_MISSION (fallen/Headers/Mission.h)
#define TO_MISSION(m)          (&mission_pool[m])
// uc_orig: TO_EVENTPOINT (fallen/Headers/Mission.h)
#define TO_EVENTPOINT(b, e)    (&(b)[e])
// uc_orig: MAP_NUMBER (fallen/Headers/Mission.h)
#define MAP_NUMBER(m)          ((m) - TO_MAP(0))
// uc_orig: MISSION_NUMBER (fallen/Headers/Mission.h)
#define MISSION_NUMBER(m)      ((m) - TO_MISSION(0))
// uc_orig: EVENTPOINT_NUMBER (fallen/Headers/Mission.h)
#define EVENTPOINT_NUMBER(b, e) ((e) - (b))

//-----------------------------------------------------------------------------
// Waypoint types (WPT_*) — what an EventPoint does when triggered.

// uc_orig: WPT_NONE (fallen/Headers/Mission.h)
#define WPT_NONE             0
// uc_orig: WPT_SIMPLE (fallen/Headers/Mission.h)
#define WPT_SIMPLE           1
// uc_orig: WPT_CREATE_PLAYER (fallen/Headers/Mission.h)
#define WPT_CREATE_PLAYER    2
// uc_orig: WPT_CREATE_ENEMIES (fallen/Headers/Mission.h)
#define WPT_CREATE_ENEMIES   3
// uc_orig: WPT_CREATE_VEHICLE (fallen/Headers/Mission.h)
#define WPT_CREATE_VEHICLE   4
// uc_orig: WPT_CREATE_ITEM (fallen/Headers/Mission.h)
#define WPT_CREATE_ITEM      5
// uc_orig: WPT_CREATE_CREATURE (fallen/Headers/Mission.h)
#define WPT_CREATE_CREATURE  6
// uc_orig: WPT_CREATE_CAMERA (fallen/Headers/Mission.h)
#define WPT_CREATE_CAMERA    7
// uc_orig: WPT_CREATE_TARGET (fallen/Headers/Mission.h)
#define WPT_CREATE_TARGET    8
// uc_orig: WPT_CREATE_MAP_EXIT (fallen/Headers/Mission.h)
#define WPT_CREATE_MAP_EXIT  9
// uc_orig: WPT_CAMERA_WAYPOINT (fallen/Headers/Mission.h)
#define WPT_CAMERA_WAYPOINT  10
// uc_orig: WPT_TARGET_WAYPOINT (fallen/Headers/Mission.h)
#define WPT_TARGET_WAYPOINT  11
// uc_orig: WPT_MESSAGE (fallen/Headers/Mission.h)
#define WPT_MESSAGE          12
// uc_orig: WPT_SOUND_EFFECT (fallen/Headers/Mission.h)
#define WPT_SOUND_EFFECT     13
// uc_orig: WPT_VISUAL_EFFECT (fallen/Headers/Mission.h)
#define WPT_VISUAL_EFFECT    14
// uc_orig: WPT_CUT_SCENE (fallen/Headers/Mission.h)
#define WPT_CUT_SCENE        15
// uc_orig: WPT_TELEPORT (fallen/Headers/Mission.h)
#define WPT_TELEPORT         16
// uc_orig: WPT_TELEPORT_TARGET (fallen/Headers/Mission.h)
#define WPT_TELEPORT_TARGET  17
// uc_orig: WPT_END_GAME_LOSE (fallen/Headers/Mission.h)
#define WPT_END_GAME_LOSE    18
// uc_orig: WPT_SHOUT (fallen/Headers/Mission.h)
#define WPT_SHOUT            19
// uc_orig: WPT_ACTIVATE_PRIM (fallen/Headers/Mission.h)
#define WPT_ACTIVATE_PRIM    20
// uc_orig: WPT_CREATE_TRAP (fallen/Headers/Mission.h)
#define WPT_CREATE_TRAP      21
// uc_orig: WPT_ADJUST_ENEMY (fallen/Headers/Mission.h)
#define WPT_ADJUST_ENEMY     22
// uc_orig: WPT_LINK_PLATFORM (fallen/Headers/Mission.h)
#define WPT_LINK_PLATFORM    23
// uc_orig: WPT_CREATE_BOMB (fallen/Headers/Mission.h)
#define WPT_CREATE_BOMB      24
// uc_orig: WPT_BURN_PRIM (fallen/Headers/Mission.h)
#define WPT_BURN_PRIM        25
// uc_orig: WPT_END_GAME_WIN (fallen/Headers/Mission.h)
#define WPT_END_GAME_WIN     26
// uc_orig: WPT_NAV_BEACON (fallen/Headers/Mission.h)
#define WPT_NAV_BEACON       27
// uc_orig: WPT_SPOT_EFFECT (fallen/Headers/Mission.h)
#define WPT_SPOT_EFFECT      28
// uc_orig: WPT_CREATE_BARREL (fallen/Headers/Mission.h)
#define WPT_CREATE_BARREL    29
// uc_orig: WPT_KILL_WAYPOINT (fallen/Headers/Mission.h)
#define WPT_KILL_WAYPOINT    30
// uc_orig: WPT_CREATE_TREASURE (fallen/Headers/Mission.h)
#define WPT_CREATE_TREASURE  31
// uc_orig: WPT_BONUS_POINTS (fallen/Headers/Mission.h)
#define WPT_BONUS_POINTS     32
// uc_orig: WPT_GROUP_LIFE (fallen/Headers/Mission.h)
#define WPT_GROUP_LIFE       33
// uc_orig: WPT_GROUP_DEATH (fallen/Headers/Mission.h)
#define WPT_GROUP_DEATH      34
// uc_orig: WPT_CONVERSATION (fallen/Headers/Mission.h)
#define WPT_CONVERSATION     35
// uc_orig: WPT_INTERESTING (fallen/Headers/Mission.h)
#define WPT_INTERESTING      36
// uc_orig: WPT_INCREMENT (fallen/Headers/Mission.h)
#define WPT_INCREMENT        37
// uc_orig: WPT_DYNAMIC_LIGHT (fallen/Headers/Mission.h)
#define WPT_DYNAMIC_LIGHT    38
// uc_orig: WPT_GOTHERE_DOTHIS (fallen/Headers/Mission.h)
#define WPT_GOTHERE_DOTHIS   39
// uc_orig: WPT_TRANSFER_PLAYER (fallen/Headers/Mission.h)
#define WPT_TRANSFER_PLAYER  40
// uc_orig: WPT_AUTOSAVE (fallen/Headers/Mission.h)
#define WPT_AUTOSAVE         41
// uc_orig: WPT_MAKE_SEARCHABLE (fallen/Headers/Mission.h)
#define WPT_MAKE_SEARCHABLE  42
// uc_orig: WPT_LOCK_VEHICLE (fallen/Headers/Mission.h)
#define WPT_LOCK_VEHICLE     43
// uc_orig: WPT_GROUP_RESET (fallen/Headers/Mission.h)
#define WPT_GROUP_RESET      44
// uc_orig: WPT_COUNT_UP_TIMER (fallen/Headers/Mission.h)
#define WPT_COUNT_UP_TIMER   45
// uc_orig: WPT_RESET_COUNTER (fallen/Headers/Mission.h)
#define WPT_RESET_COUNTER    46
// uc_orig: WPT_CREATE_MIST (fallen/Headers/Mission.h)
#define WPT_CREATE_MIST      47
// uc_orig: WPT_ENEMY_FLAGS (fallen/Headers/Mission.h)
#define WPT_ENEMY_FLAGS      48
// uc_orig: WPT_STALL_CAR (fallen/Headers/Mission.h)
#define WPT_STALL_CAR        49
// uc_orig: WPT_EXTEND (fallen/Headers/Mission.h)
#define WPT_EXTEND           50
// uc_orig: WPT_MOVE_THING (fallen/Headers/Mission.h)
#define WPT_MOVE_THING       51
// uc_orig: WPT_MAKE_PERSON_PEE (fallen/Headers/Mission.h)
#define WPT_MAKE_PERSON_PEE  52
// uc_orig: WPT_CONE_PENALTIES (fallen/Headers/Mission.h)
#define WPT_CONE_PENALTIES   53
// uc_orig: WPT_SIGN (fallen/Headers/Mission.h)
#define WPT_SIGN             54
// uc_orig: WPT_WAREFX (fallen/Headers/Mission.h)
#define WPT_WAREFX           55
// uc_orig: WPT_NO_FLOOR (fallen/Headers/Mission.h)
#define WPT_NO_FLOOR         56
// uc_orig: WPT_SHAKE_CAMERA (fallen/Headers/Mission.h)
#define WPT_SHAKE_CAMERA     57

// Legacy aliases kept for compatibility with elev.cpp.
// uc_orig: WPT_TRIGGER (fallen/Headers/Mission.h)
#define WPT_TRIGGER  -1
// uc_orig: WPT_END_GAME (fallen/Headers/Mission.h)
#define WPT_END_GAME WPT_END_GAME_LOSE

//-----------------------------------------------------------------------------
// Player type constants (PT_*) — which character the player controls.

// uc_orig: PT_NONE (fallen/Headers/Mission.h)
#define PT_NONE  0
// uc_orig: PT_DARCI (fallen/Headers/Mission.h)
#define PT_DARCI 1
// uc_orig: PT_ROPER (fallen/Headers/Mission.h)
#define PT_ROPER 2
// uc_orig: PT_COP (fallen/Headers/Mission.h)
#define PT_COP   3
// uc_orig: PT_GANG (fallen/Headers/Mission.h)
#define PT_GANG  4

//-----------------------------------------------------------------------------
// Enemy type constants (ET_*) — NPC spawn type for WPT_CREATE_ENEMIES.

// uc_orig: ET_NONE (fallen/Headers/Mission.h)
#define ET_NONE              0
// uc_orig: ET_CIV (fallen/Headers/Mission.h)
#define ET_CIV               1
// uc_orig: ET_CIV_BALLOON (fallen/Headers/Mission.h)
#define ET_CIV_BALLOON       2
// uc_orig: ET_SLAG (fallen/Headers/Mission.h)
#define ET_SLAG              3
// uc_orig: ET_UGLY_FAT_SLAG (fallen/Headers/Mission.h)
#define ET_UGLY_FAT_SLAG     4
// uc_orig: ET_WORKMAN (fallen/Headers/Mission.h)
#define ET_WORKMAN           5
// uc_orig: ET_GANG_RASTA (fallen/Headers/Mission.h)
#define ET_GANG_RASTA        6
// uc_orig: ET_GANG_RED (fallen/Headers/Mission.h)
#define ET_GANG_RED          7
// uc_orig: ET_GANG_GREY (fallen/Headers/Mission.h)
#define ET_GANG_GREY         8
// uc_orig: ET_GANG_RASTA_PISTOL (fallen/Headers/Mission.h)
#define ET_GANG_RASTA_PISTOL 9
// uc_orig: ET_GANG_RED_SHOTGUN (fallen/Headers/Mission.h)
#define ET_GANG_RED_SHOTGUN  10
// uc_orig: ET_GANG_GREY_AK47 (fallen/Headers/Mission.h)
#define ET_GANG_GREY_AK47    11
// uc_orig: ET_COP (fallen/Headers/Mission.h)
#define ET_COP               12
// uc_orig: ET_COP_PISTOL (fallen/Headers/Mission.h)
#define ET_COP_PISTOL        13
// uc_orig: ET_COP_SHOTGUN (fallen/Headers/Mission.h)
#define ET_COP_SHOTGUN       14
// uc_orig: ET_COP_AK47 (fallen/Headers/Mission.h)
#define ET_COP_AK47          15
// uc_orig: ET_HOSTAGE (fallen/Headers/Mission.h)
#define ET_HOSTAGE           16
// uc_orig: ET_WORKMAN_GRENADE (fallen/Headers/Mission.h)
#define ET_WORKMAN_GRENADE   17
// uc_orig: ET_TRAMP (fallen/Headers/Mission.h)
#define ET_TRAMP             18
// uc_orig: ET_MIB1 (fallen/Headers/Mission.h)
#define ET_MIB1              19
// uc_orig: ET_MIB2 (fallen/Headers/Mission.h)
#define ET_MIB2              20
// uc_orig: ET_MIB3 (fallen/Headers/Mission.h)
#define ET_MIB3              21
// uc_orig: ET_DARCI (fallen/Headers/Mission.h)
#define ET_DARCI             22
// uc_orig: ET_ROPER (fallen/Headers/Mission.h)
#define ET_ROPER             23

//-----------------------------------------------------------------------------
// Vehicle type constants (VT_*) — spawn type for WPT_CREATE_VEHICLE.

// uc_orig: VT_NONE (fallen/Headers/Mission.h)
#define VT_NONE       0
// uc_orig: VT_CAR (fallen/Headers/Mission.h)
#define VT_CAR        1
// uc_orig: VT_VAN (fallen/Headers/Mission.h)
#define VT_VAN        2
// uc_orig: VT_TAXI (fallen/Headers/Mission.h)
#define VT_TAXI       3
// uc_orig: VT_HELICOPTER (fallen/Headers/Mission.h)
#define VT_HELICOPTER 4
// uc_orig: VT_BIKE (fallen/Headers/Mission.h)
#define VT_BIKE       5
// uc_orig: VT_POLICE (fallen/Headers/Mission.h)
#define VT_POLICE     6
// uc_orig: VT_AMBULANCE (fallen/Headers/Mission.h)
#define VT_AMBULANCE  7
// uc_orig: VT_MEATWAGON (fallen/Headers/Mission.h)
#define VT_MEATWAGON  8
// uc_orig: VT_JEEP (fallen/Headers/Mission.h)
#define VT_JEEP       9
// uc_orig: VT_SEDAN (fallen/Headers/Mission.h)
#define VT_SEDAN      10
// uc_orig: VT_WILDCATVAN (fallen/Headers/Mission.h)
#define VT_WILDCATVAN 11

// Vehicle movement type (VMT_*) — AI driving behaviour.
// uc_orig: VMT_PLAYER_DRIVES (fallen/Headers/Mission.h)
#define VMT_PLAYER_DRIVES 0
// uc_orig: VMT_PATROL_WPTS (fallen/Headers/Mission.h)
#define VMT_PATROL_WPTS   1
// uc_orig: VMT_GUARD_AREA (fallen/Headers/Mission.h)
#define VMT_GUARD_AREA    2
// uc_orig: VMT_TRACK_TARGET (fallen/Headers/Mission.h)
#define VMT_TRACK_TARGET  3

// Vehicle key lock colour (VK_*).
// uc_orig: VK_UNLOCKED (fallen/Headers/Mission.h)
#define VK_UNLOCKED 0
// uc_orig: VK_RED (fallen/Headers/Mission.h)
#define VK_RED      1
// uc_orig: VK_BLUE (fallen/Headers/Mission.h)
#define VK_BLUE     2
// uc_orig: VK_GREEN (fallen/Headers/Mission.h)
#define VK_GREEN    3
// uc_orig: VK_BLACK (fallen/Headers/Mission.h)
#define VK_BLACK    4
// uc_orig: VK_WHITE (fallen/Headers/Mission.h)
#define VK_WHITE    5
// uc_orig: VK_LOCKED (fallen/Headers/Mission.h)
#define VK_LOCKED   6

//-----------------------------------------------------------------------------
// Item type constants (IT_*) — spawn type for WPT_CREATE_ITEM.

// uc_orig: IT_NONE (fallen/Headers/Mission.h)
#define IT_NONE         0
// uc_orig: IT_KEY (fallen/Headers/Mission.h)
#define IT_KEY          1
// uc_orig: IT_PISTOL (fallen/Headers/Mission.h)
#define IT_PISTOL       2
// uc_orig: IT_HEALTH (fallen/Headers/Mission.h)
#define IT_HEALTH       3
// uc_orig: IT_SHOTGUN (fallen/Headers/Mission.h)
#define IT_SHOTGUN      4
// uc_orig: IT_KNIFE (fallen/Headers/Mission.h)
#define IT_KNIFE        5
// uc_orig: IT_AK47 (fallen/Headers/Mission.h)
#define IT_AK47         6
// uc_orig: IT_MINE (fallen/Headers/Mission.h)
#define IT_MINE         7
// uc_orig: IT_BASEBALLBAT (fallen/Headers/Mission.h)
#define IT_BASEBALLBAT  8
// uc_orig: IT_BARREL (fallen/Headers/Mission.h)
#define IT_BARREL       9
// uc_orig: IT_AMMO_PISTOL (fallen/Headers/Mission.h)
#define IT_AMMO_PISTOL  10
// uc_orig: IT_AMMO_SHOTGUN (fallen/Headers/Mission.h)
#define IT_AMMO_SHOTGUN 11
// uc_orig: IT_AMMO_AK47 (fallen/Headers/Mission.h)
#define IT_AMMO_AK47    12
// uc_orig: IT_KEYCARD (fallen/Headers/Mission.h)
#define IT_KEYCARD      13
// uc_orig: IT_FILE (fallen/Headers/Mission.h)
#define IT_FILE         14
// uc_orig: IT_FLOPPY_DISK (fallen/Headers/Mission.h)
#define IT_FLOPPY_DISK  15
// uc_orig: IT_CROWBAR (fallen/Headers/Mission.h)
#define IT_CROWBAR      16
// uc_orig: IT_GASMASK (fallen/Headers/Mission.h)
#define IT_GASMASK      17
// uc_orig: IT_WRENCH (fallen/Headers/Mission.h)
#define IT_WRENCH       18
// uc_orig: IT_VIDEO (fallen/Headers/Mission.h)
#define IT_VIDEO        19
// uc_orig: IT_GLOVES (fallen/Headers/Mission.h)
#define IT_GLOVES       20
// uc_orig: IT_WEEDAWAY (fallen/Headers/Mission.h)
#define IT_WEEDAWAY     21
// uc_orig: IT_GRENADE (fallen/Headers/Mission.h)
#define IT_GRENADE      22
// uc_orig: IT_EXPLOSIVES (fallen/Headers/Mission.h)
#define IT_EXPLOSIVES   23
// uc_orig: IT_SILENCER (fallen/Headers/Mission.h)
#define IT_SILENCER     24
// uc_orig: IT_WIRE_CUTTER (fallen/Headers/Mission.h)
#define IT_WIRE_CUTTER  25

// Bitmask of items an enemy carries (HAS_*).
// uc_orig: HAS_PISTOL (fallen/Headers/Mission.h)
#define HAS_PISTOL  (1 << 0)
// uc_orig: HAS_SHOTGUN (fallen/Headers/Mission.h)
#define HAS_SHOTGUN (1 << 1)
// uc_orig: HAS_AK47 (fallen/Headers/Mission.h)
#define HAS_AK47    (1 << 2)
// uc_orig: HAS_GRENADE (fallen/Headers/Mission.h)
#define HAS_GRENADE (1 << 3)
// uc_orig: HAS_BALLOON (fallen/Headers/Mission.h)
#define HAS_BALLOON (1 << 4)
// uc_orig: HAS_KNIFE (fallen/Headers/Mission.h)
#define HAS_KNIFE   (1 << 5)
// uc_orig: HAS_BAT (fallen/Headers/Mission.h)
#define HAS_BAT     (1 << 6)

//-----------------------------------------------------------------------------
// Visual effect type (VFX_*) — for WPT_VISUAL_EFFECT.

// uc_orig: VFX_NONE (fallen/Headers/Mission.h)
#define VFX_NONE      0
// uc_orig: VFX_EXPLOSION (fallen/Headers/Mission.h)
#define VFX_EXPLOSION 1
// uc_orig: VFX_FIRE (fallen/Headers/Mission.h)
#define VFX_FIRE      2
// uc_orig: VFX_WATER_JET (fallen/Headers/Mission.h)
#define VFX_WATER_JET 3

//-----------------------------------------------------------------------------
// Creature type (CT_*) — for WPT_CREATE_CREATURE.

// uc_orig: CT_NONE (fallen/Headers/Mission.h)
#define CT_NONE   0
// uc_orig: CT_DOG (fallen/Headers/Mission.h)
#define CT_DOG    1
// uc_orig: CT_PIGEON (fallen/Headers/Mission.h)
#define CT_PIGEON 2

//-----------------------------------------------------------------------------
// Activate prim type (AP_*) — for WPT_ACTIVATE_PRIM.

// uc_orig: AP_DOOR (fallen/Headers/Mission.h)
#define AP_DOOR             0
// uc_orig: AP_ELECTRIC_FENCE (fallen/Headers/Mission.h)
#define AP_ELECTRIC_FENCE   1
// uc_orig: AP_SECURITY_CAMERA (fallen/Headers/Mission.h)
#define AP_SECURITY_CAMERA  2

//-----------------------------------------------------------------------------
// Trigger type (TT_*) — what activates an EventPoint.

// uc_orig: TT_NONE (fallen/Headers/Mission.h)
#define TT_NONE                0
// uc_orig: TT_DEPENDENCY (fallen/Headers/Mission.h)
#define TT_DEPENDENCY          1
// uc_orig: TT_RADIUS (fallen/Headers/Mission.h)
#define TT_RADIUS              2
// uc_orig: TT_DOOR (fallen/Headers/Mission.h)
#define TT_DOOR                3
// uc_orig: TT_TRIPWIRE (fallen/Headers/Mission.h)
#define TT_TRIPWIRE            4
// uc_orig: TT_PRESSURE_PAD (fallen/Headers/Mission.h)
#define TT_PRESSURE_PAD        5
// uc_orig: TT_ELECTRIC_FENCE (fallen/Headers/Mission.h)
#define TT_ELECTRIC_FENCE      6
// uc_orig: TT_WATER_LEVEL (fallen/Headers/Mission.h)
#define TT_WATER_LEVEL         7
// uc_orig: TT_SECURITY_CAMERA (fallen/Headers/Mission.h)
#define TT_SECURITY_CAMERA     8
// uc_orig: TT_SWITCH (fallen/Headers/Mission.h)
#define TT_SWITCH              9
// uc_orig: TT_ANIM_PRIM (fallen/Headers/Mission.h)
#define TT_ANIM_PRIM           10
// uc_orig: TT_TIMER (fallen/Headers/Mission.h)
#define TT_TIMER               11
// uc_orig: TT_SHOUT_ALL (fallen/Headers/Mission.h)
#define TT_SHOUT_ALL           12
// uc_orig: TT_BOOLEANAND (fallen/Headers/Mission.h)
#define TT_BOOLEANAND          13
// uc_orig: TT_BOOLEANOR (fallen/Headers/Mission.h)
#define TT_BOOLEANOR           14
// uc_orig: TT_ITEM_HELD (fallen/Headers/Mission.h)
#define TT_ITEM_HELD           15
// uc_orig: TT_ITEM_SEEN (fallen/Headers/Mission.h)
#define TT_ITEM_SEEN           16
// uc_orig: TT_KILLED (fallen/Headers/Mission.h)
#define TT_KILLED              17
// uc_orig: TT_SHOUT_ANY (fallen/Headers/Mission.h)
#define TT_SHOUT_ANY           18
// uc_orig: TT_COUNTDOWN (fallen/Headers/Mission.h)
#define TT_COUNTDOWN           19
// uc_orig: TT_ENEMYRADIUS (fallen/Headers/Mission.h)
#define TT_ENEMYRADIUS         20
// uc_orig: TT_VISIBLECOUNTDOWN (fallen/Headers/Mission.h)
#define TT_VISIBLECOUNTDOWN    21
// uc_orig: TT_CUBOID (fallen/Headers/Mission.h)
#define TT_CUBOID              22
// uc_orig: TT_HALFDEAD (fallen/Headers/Mission.h)
#define TT_HALFDEAD            23
// uc_orig: TT_GROUPDEAD (fallen/Headers/Mission.h)
#define TT_GROUPDEAD           24
// uc_orig: TT_PERSON_SEEN (fallen/Headers/Mission.h)
#define TT_PERSON_SEEN         25
// uc_orig: TT_PERSON_USED (fallen/Headers/Mission.h)
#define TT_PERSON_USED         26
// uc_orig: TT_PLAYER_USES_RADIUS (fallen/Headers/Mission.h)
#define TT_PLAYER_USES_RADIUS  27
// uc_orig: TT_PRIM_DAMAGED (fallen/Headers/Mission.h)
#define TT_PRIM_DAMAGED        28
// uc_orig: TT_PERSON_ARRESTED (fallen/Headers/Mission.h)
#define TT_PERSON_ARRESTED     29
// uc_orig: TT_CONVERSATION_OVER (fallen/Headers/Mission.h)
#define TT_CONVERSATION_OVER   30
// uc_orig: TT_COUNTER (fallen/Headers/Mission.h)
#define TT_COUNTER             31
// uc_orig: TT_KILLED_NOT_ARRESTED (fallen/Headers/Mission.h)
#define TT_KILLED_NOT_ARRESTED 32
// uc_orig: TT_CRIME_RATE_ABOVE (fallen/Headers/Mission.h)
#define TT_CRIME_RATE_ABOVE    33
// uc_orig: TT_CRIME_RATE_BELOW (fallen/Headers/Mission.h)
#define TT_CRIME_RATE_BELOW    34
// uc_orig: TT_PERSON_IS_MURDERER (fallen/Headers/Mission.h)
#define TT_PERSON_IS_MURDERER  35
// uc_orig: TT_PERSON_IN_VEHICLE (fallen/Headers/Mission.h)
#define TT_PERSON_IN_VEHICLE   36
// uc_orig: TT_THING_RADIUS_DIR (fallen/Headers/Mission.h)
#define TT_THING_RADIUS_DIR    37
// uc_orig: TT_PLAYER_CARRY_PERSON (fallen/Headers/Mission.h)
#define TT_PLAYER_CARRY_PERSON 38
// uc_orig: TT_SPECIFIC_ITEM_HELD (fallen/Headers/Mission.h)
#define TT_SPECIFIC_ITEM_HELD  39
// uc_orig: TT_RANDOM (fallen/Headers/Mission.h)
#define TT_RANDOM              40
// uc_orig: TT_PLAYER_FIRES_GUN (fallen/Headers/Mission.h)
#define TT_PLAYER_FIRES_GUN    41
// uc_orig: TT_DARCI_GRABBED (fallen/Headers/Mission.h)
#define TT_DARCI_GRABBED       42
// uc_orig: TT_PUNCHED_AND_KICKED (fallen/Headers/Mission.h)
#define TT_PUNCHED_AND_KICKED  43
// uc_orig: TT_MOVE_RADIUS_DIR (fallen/Headers/Mission.h)
#define TT_MOVE_RADIUS_DIR     44
// Total number of trigger types — used to size WaypointUses[].
// uc_orig: TT_NUMBER (fallen/Headers/Mission.h)
#define TT_NUMBER              45

//-----------------------------------------------------------------------------
// On-trigger behaviour (OT_*) — what happens after a trigger fires.

// uc_orig: OT_NONE (fallen/Headers/Mission.h)
#define OT_NONE         0
// uc_orig: OT_ACTIVE (fallen/Headers/Mission.h)
#define OT_ACTIVE       1
// uc_orig: OT_ACTIVE_WHILE (fallen/Headers/Mission.h)
#define OT_ACTIVE_WHILE 2
// uc_orig: OT_ACTIVE_TIME (fallen/Headers/Mission.h)
#define OT_ACTIVE_TIME  3
// uc_orig: OT_ACTIVE_DIE (fallen/Headers/Mission.h)
#define OT_ACTIVE_DIE   4

//-----------------------------------------------------------------------------
// Camera target type (CT_*) — for WPT_CREATE_TARGET/WPT_CREATE_CAMERA.
// Note: CT_NONE (0) is defined above in creature types; camera uses 1-3 only.

// uc_orig: CT_NORMAL (fallen/Headers/Mission.h)
#define CT_NORMAL        1
// uc_orig: CT_ATTACHED (fallen/Headers/Mission.h)
#define CT_ATTACHED      2
// uc_orig: CT_NEAREST_LIVING (fallen/Headers/Mission.h)
#define CT_NEAREST_LIVING 3

//-----------------------------------------------------------------------------
// Link platform flags (LP_*) — for WPT_LINK_PLATFORM.

// uc_orig: LP_LOCK_TO_AXIS (fallen/Headers/Mission.h)
#define LP_LOCK_TO_AXIS   (1 << 0)
// uc_orig: LP_LOCK_ROTATION (fallen/Headers/Mission.h)
#define LP_LOCK_ROTATION  (1 << 1)
// uc_orig: LP_BODGE_ROCKET (fallen/Headers/Mission.h)
#define LP_BODGE_ROCKET   (1 << 2)

//-----------------------------------------------------------------------------
// Barrel type constants (BT_*) — for WPT_CREATE_BARREL.

// uc_orig: BT_BARREL (fallen/Headers/Mission.h)
#define BT_BARREL        0
// uc_orig: BT_TRAFFIC_CONE (fallen/Headers/Mission.h)
#define BT_TRAFFIC_CONE  1
// uc_orig: BT_BIN (fallen/Headers/Mission.h)
#define BT_BIN           2
// uc_orig: BT_BURNING_BARREL (fallen/Headers/Mission.h)
#define BT_BURNING_BARREL 3
// uc_orig: BT_BURNING_BIN (fallen/Headers/Mission.h)
#define BT_BURNING_BIN   4
// uc_orig: BT_OIL_DRUM (fallen/Headers/Mission.h)
#define BT_OIL_DRUM      5
// uc_orig: BT_LOX_DRUM (fallen/Headers/Mission.h)
#define BT_LOX_DRUM      6

//-----------------------------------------------------------------------------
// WaypointUses bitmask — for each TT_* type, which property fields are in use.
// Sized by TT_NUMBER. Defined in elev.cpp.

// uc_orig: WPU_DEPEND (fallen/Headers/Mission.h)
#define WPU_DEPEND  1
// uc_orig: WPU_BOOLEAN (fallen/Headers/Mission.h)
#define WPU_BOOLEAN 2
// uc_orig: WPU_TIME (fallen/Headers/Mission.h)
#define WPU_TIME    4
// uc_orig: WPU_RADIUS (fallen/Headers/Mission.h)
#define WPU_RADIUS  8
// uc_orig: WPU_RADTEXT (fallen/Headers/Mission.h)
#define WPU_RADTEXT 16
// uc_orig: WPU_RADBOX (fallen/Headers/Mission.h)
#define WPU_RADBOX  32
// uc_orig: WPU_COUNTER (fallen/Headers/Mission.h)
#define WPU_COUNTER 64

//-----------------------------------------------------------------------------
// Mission management API.

// uc_orig: MISSION_init (fallen/Headers/Mission.h)
void MISSION_init(void);

// uc_orig: free_map (fallen/Headers/Mission.h)
void free_map(UWORD map);
// uc_orig: free_mission (fallen/Headers/Mission.h)
void free_mission(UWORD mission);
// uc_orig: init_mission (fallen/Headers/Mission.h)
void init_mission(UWORD mission_ref, CBYTE* mission_name);
// uc_orig: free_eventpoint (fallen/Headers/Mission.h)
void free_eventpoint(EventPoint* the_ep);
// uc_orig: write_event_extra (fallen/Headers/Mission.h)
void write_event_extra(FILE* file_handle, EventPoint* ep);
// uc_orig: read_event_extra (fallen/Headers/Mission.h)
void read_event_extra(FILE* file_handle, EventPoint* ep, EventPoint* base, SLONG ver = 0);
// uc_orig: import_mission (fallen/Headers/Mission.h)
void import_mission(void);
// uc_orig: ResetFreepoint (fallen/Headers/Mission.h)
void ResetFreepoint(Mission* mission);
// uc_orig: ResetUsedpoint (fallen/Headers/Mission.h)
void ResetUsedpoint(Mission* mission);
// uc_orig: ResetFreelist (fallen/Headers/Mission.h)
void ResetFreelist(Mission* mission);
// uc_orig: ResetUsedlist (fallen/Headers/Mission.h)
void ResetUsedlist(Mission* mission);
// uc_orig: SetEPTextID (fallen/Headers/Mission.h)
void SetEPTextID(EventPoint* ep, SLONG value = -1);
#endif // MISSIONS_MISSION_H
