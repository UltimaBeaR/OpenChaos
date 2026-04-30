#ifndef MISSIONS_EWAY_GLOBALS_H
#define MISSIONS_EWAY_GLOBALS_H

#include "engine/core/types.h"

struct Thing;

// ====================================================
// EWAY type definitions (moved from fallen/Headers/eway.h)
// ====================================================

// uc_orig: EWAY_MAX_CONDS (fallen/Headers/eway.h)
#define EWAY_MAX_CONDS 128
// uc_orig: EWAY_MAX_WAYS (fallen/Headers/eway.h)
#define EWAY_MAX_WAYS 512
// uc_orig: EWAY_MAX_EDEFS (fallen/Headers/eway.h)
#define EWAY_MAX_EDEFS 150
// uc_orig: EWAY_MAX_TIMERS (fallen/Headers/eway.h)
#define EWAY_MAX_TIMERS 32
// uc_orig: EWAY_MESS_BUFFER_SIZE (fallen/Headers/eway.h)
#define EWAY_MESS_BUFFER_SIZE 16384
// uc_orig: EWAY_MAX_MESSES (fallen/Headers/eway.h)
#define EWAY_MAX_MESSES 128
// uc_orig: EWAY_MAX_COUNTERS (fallen/Headers/eway.h)
#define EWAY_MAX_COUNTERS 10

// Waypoint runtime state flags.
// uc_orig: EWAY_FLAG_ACTIVE (fallen/Headers/eway.h)
#define EWAY_FLAG_ACTIVE (1 << 0)
// uc_orig: EWAY_FLAG_COUNTDOWN (fallen/Headers/eway.h)
#define EWAY_FLAG_COUNTDOWN (1 << 1)
// uc_orig: EWAY_FLAG_DEAD (fallen/Headers/eway.h)
#define EWAY_FLAG_DEAD (1 << 2)
// uc_orig: EWAY_FLAG_TRIGGERED (fallen/Headers/eway.h)
#define EWAY_FLAG_TRIGGERED (1 << 3)
// uc_orig: EWAY_FLAG_GOTITEM (fallen/Headers/eway.h)
#define EWAY_FLAG_GOTITEM (1 << 4)
// uc_orig: EWAY_FLAG_FINISHED (fallen/Headers/eway.h)
#define EWAY_FLAG_FINISHED (1 << 5)
// uc_orig: EWAY_FLAG_CLEARED (fallen/Headers/eway.h)
#define EWAY_FLAG_CLEARED (1 << 6)
// uc_orig: EWAY_FLAG_WHY_LOST (fallen/Headers/eway.h)
#define EWAY_FLAG_WHY_LOST (1 << 7)

// Condition types (EWAY_COND_*). 42 types total (0-41).
// uc_orig: EWAY_COND_FALSE (fallen/Headers/eway.h)
#define EWAY_COND_FALSE 0
// uc_orig: EWAY_COND_TRUE (fallen/Headers/eway.h)
#define EWAY_COND_TRUE 1
// uc_orig: EWAY_COND_PROXIMITY (fallen/Headers/eway.h)
#define EWAY_COND_PROXIMITY 2
// uc_orig: EWAY_COND_TRIPWIRE (fallen/Headers/eway.h)
#define EWAY_COND_TRIPWIRE 3
// uc_orig: EWAY_COND_PRESSURE (fallen/Headers/eway.h)
#define EWAY_COND_PRESSURE 4
// uc_orig: EWAY_COND_CAMERA (fallen/Headers/eway.h)
#define EWAY_COND_CAMERA 5
// uc_orig: EWAY_COND_SWITCH (fallen/Headers/eway.h)
#define EWAY_COND_SWITCH 6
// uc_orig: EWAY_COND_TIME (fallen/Headers/eway.h)
#define EWAY_COND_TIME 7
// uc_orig: EWAY_COND_DEPENDENT (fallen/Headers/eway.h)
#define EWAY_COND_DEPENDENT 8
// uc_orig: EWAY_COND_BOOL_AND (fallen/Headers/eway.h)
#define EWAY_COND_BOOL_AND 9
// uc_orig: EWAY_COND_BOOL_OR (fallen/Headers/eway.h)
#define EWAY_COND_BOOL_OR 10
// uc_orig: EWAY_COND_COUNTDOWN (fallen/Headers/eway.h)
#define EWAY_COND_COUNTDOWN 11
// uc_orig: EWAY_COND_COUNTDOWN_SEE (fallen/Headers/eway.h)
#define EWAY_COND_COUNTDOWN_SEE 12
// uc_orig: EWAY_COND_PERSON_DEAD (fallen/Headers/eway.h)
#define EWAY_COND_PERSON_DEAD 13
// uc_orig: EWAY_COND_PERSON_NEAR (fallen/Headers/eway.h)
#define EWAY_COND_PERSON_NEAR 14
// uc_orig: EWAY_COND_CAMERA_AT (fallen/Headers/eway.h)
#define EWAY_COND_CAMERA_AT 15
// uc_orig: EWAY_COND_PLAYER_CUBE (fallen/Headers/eway.h)
#define EWAY_COND_PLAYER_CUBE 16
// uc_orig: EWAY_COND_A_SEE_B (fallen/Headers/eway.h)
#define EWAY_COND_A_SEE_B 17
// uc_orig: EWAY_COND_GROUP_DEAD (fallen/Headers/eway.h)
#define EWAY_COND_GROUP_DEAD 18
// uc_orig: EWAY_COND_HALF_DEAD (fallen/Headers/eway.h)
#define EWAY_COND_HALF_DEAD 19
// uc_orig: EWAY_COND_PERSON_USED (fallen/Headers/eway.h)
#define EWAY_COND_PERSON_USED 20
// uc_orig: EWAY_COND_ITEM_HELD (fallen/Headers/eway.h)
#define EWAY_COND_ITEM_HELD 21
// uc_orig: EWAY_COND_RADIUS_USED (fallen/Headers/eway.h)
#define EWAY_COND_RADIUS_USED 22
// uc_orig: EWAY_COND_PRIM_DAMAGED (fallen/Headers/eway.h)
#define EWAY_COND_PRIM_DAMAGED 23
// uc_orig: EWAY_COND_CONVERSE_END (fallen/Headers/eway.h)
#define EWAY_COND_CONVERSE_END 24
// uc_orig: EWAY_COND_COUNTER_GTEQ (fallen/Headers/eway.h)
#define EWAY_COND_COUNTER_GTEQ 25
// uc_orig: EWAY_COND_PERSON_ARRESTED (fallen/Headers/eway.h)
#define EWAY_COND_PERSON_ARRESTED 26
// uc_orig: EWAY_COND_PLAYER_CUBOID (fallen/Headers/eway.h)
#define EWAY_COND_PLAYER_CUBOID 27
// uc_orig: EWAY_COND_KILLED_NOT_ARRESTED (fallen/Headers/eway.h)
#define EWAY_COND_KILLED_NOT_ARRESTED 28
// uc_orig: EWAY_COND_CRIME_RATE_GTEQ (fallen/Headers/eway.h)
#define EWAY_COND_CRIME_RATE_GTEQ 29
// uc_orig: EWAY_COND_CRIME_RATE_LTEQ (fallen/Headers/eway.h)
#define EWAY_COND_CRIME_RATE_LTEQ 30
// uc_orig: EWAY_COND_IS_MURDERER (fallen/Headers/eway.h)
#define EWAY_COND_IS_MURDERER 31
// uc_orig: EWAY_COND_PERSON_IN_VEHICLE (fallen/Headers/eway.h)
#define EWAY_COND_PERSON_IN_VEHICLE 32
// uc_orig: EWAY_COND_THING_RADIUS_DIR (fallen/Headers/eway.h)
#define EWAY_COND_THING_RADIUS_DIR 33
// uc_orig: EWAY_COND_SPECIFIC_ITEM_HELD (fallen/Headers/eway.h)
#define EWAY_COND_SPECIFIC_ITEM_HELD 34
// uc_orig: EWAY_COND_RANDOM (fallen/Headers/eway.h)
#define EWAY_COND_RANDOM 35
// uc_orig: EWAY_COND_PLAYER_FIRED_GUN (fallen/Headers/eway.h)
#define EWAY_COND_PLAYER_FIRED_GUN 36
// uc_orig: EWAY_COND_PRIM_ACTIVATED (fallen/Headers/eway.h)
#define EWAY_COND_PRIM_ACTIVATED 37
// uc_orig: EWAY_COND_DARCI_GRABBED (fallen/Headers/eway.h)
#define EWAY_COND_DARCI_GRABBED 38
// uc_orig: EWAY_COND_PUNCHED_AND_KICKED (fallen/Headers/eway.h)
#define EWAY_COND_PUNCHED_AND_KICKED 39
// uc_orig: EWAY_COND_MOVE_RADIUS_DIR (fallen/Headers/eway.h)
#define EWAY_COND_MOVE_RADIUS_DIR 40
// uc_orig: EWAY_COND_AFTER_FIRST_GAMETURN (fallen/Headers/eway.h)
#define EWAY_COND_AFTER_FIRST_GAMETURN 41

// Extended condition definition (from .ucm file data), used during level load only.
// uc_orig: EWAY_Conddef (fallen/Headers/eway.h)
typedef struct eway_conddef {
    UBYTE type;
    UBYTE negate;
    UWORD arg1;
    UWORD arg2;
    struct eway_conddef* bool_arg1;
    struct eway_conddef* bool_arg2;
} EWAY_Conddef;

// Runtime stay (deactivation) condition.
// uc_orig: EWAY_Stay (fallen/Headers/eway.h)
typedef struct {
    UWORD type;
    UWORD arg;
} EWAY_Stay;

// Runtime activation condition (compact form stored in EWAY_Way).
// uc_orig: EWAY_Cond (fallen/Headers/eway.h)
typedef struct {
    UBYTE type;
    UBYTE negate;
    UWORD arg1;
    UWORD arg2;
} EWAY_Cond;

// Runtime action definition stored in EWAY_Way.
// uc_orig: EWAY_Do (fallen/Headers/eway.h)
typedef struct {
    UBYTE type;
    UBYTE subtype;
    UWORD arg1;
    UWORD arg2;
} EWAY_Do;

// One mission waypoint: position, condition, action, and stay rule.
// uc_orig: EWAY_Way (fallen/Headers/eway.h)
typedef struct {
    UWORD id;
    UBYTE colour;
    UBYTE group;
    UBYTE flag;
    UBYTE yaw;
    UWORD timer;
    UWORD x;
    SWORD y;
    UWORD z;
    UBYTE index;
    UBYTE ware;
    EWAY_Cond ec;
    EWAY_Stay es;
    EWAY_Do ed;
} EWAY_Way;

// Stay mode constants.
// uc_orig: EWAY_STAY_ALWAYS (fallen/Headers/eway.h)
#define EWAY_STAY_ALWAYS 1
// uc_orig: EWAY_STAY_WHILE (fallen/Headers/eway.h)
#define EWAY_STAY_WHILE 2
// uc_orig: EWAY_STAY_WHILE_TIME (fallen/Headers/eway.h)
#define EWAY_STAY_WHILE_TIME 3
// uc_orig: EWAY_STAY_TIME (fallen/Headers/eway.h)
#define EWAY_STAY_TIME 4
// uc_orig: EWAY_STAY_AFTER_TIME (fallen/Headers/eway.h)
#define EWAY_STAY_AFTER_TIME 5
// uc_orig: EWAY_STAY_DIE (fallen/Headers/eway.h)
#define EWAY_STAY_DIE 6

// Action type constants (EWAY_DO_*). 52 types total.
// uc_orig: EWAY_DO_NOTHING (fallen/Headers/eway.h)
#define EWAY_DO_NOTHING 0
// uc_orig: EWAY_DO_CREATE_PLAYER (fallen/Headers/eway.h)
#define EWAY_DO_CREATE_PLAYER 1
// uc_orig: EWAY_DO_CREATE_ANIMAL (fallen/Headers/eway.h)
#define EWAY_DO_CREATE_ANIMAL 2
// uc_orig: EWAY_DO_CREATE_ENEMY (fallen/Headers/eway.h)
#define EWAY_DO_CREATE_ENEMY 3
// uc_orig: EWAY_DO_CREATE_ITEM (fallen/Headers/eway.h)
#define EWAY_DO_CREATE_ITEM 4
// uc_orig: EWAY_DO_CREATE_VEHICLE (fallen/Headers/eway.h)
#define EWAY_DO_CREATE_VEHICLE 5
// uc_orig: EWAY_DO_SOUND_ALARM (fallen/Headers/eway.h)
#define EWAY_DO_SOUND_ALARM 6
// uc_orig: EWAY_DO_CONTROL_DOOR (fallen/Headers/eway.h)
#define EWAY_DO_CONTROL_DOOR 7
// uc_orig: EWAY_DO_EXPLODE (fallen/Headers/eway.h)
#define EWAY_DO_EXPLODE 8
// uc_orig: EWAY_DO_MESSAGE (fallen/Headers/eway.h)
#define EWAY_DO_MESSAGE 9
// uc_orig: EWAY_DO_ELECTRIFY_FENCE (fallen/Headers/eway.h)
#define EWAY_DO_ELECTRIFY_FENCE 10
// uc_orig: EWAY_DO_CAMERA_CREATE (fallen/Headers/eway.h)
#define EWAY_DO_CAMERA_CREATE 11
// uc_orig: EWAY_DO_CAMERA_WAYPOINT (fallen/Headers/eway.h)
#define EWAY_DO_CAMERA_WAYPOINT 12
// uc_orig: EWAY_DO_CAMERA_TARGET (fallen/Headers/eway.h)
#define EWAY_DO_CAMERA_TARGET 13
// uc_orig: EWAY_DO_MISSION_FAIL (fallen/Headers/eway.h)
#define EWAY_DO_MISSION_FAIL 14
// uc_orig: EWAY_DO_MISSION_COMPLETE (fallen/Headers/eway.h)
#define EWAY_DO_MISSION_COMPLETE 15
// uc_orig: EWAY_DO_CHANGE_ENEMY (fallen/Headers/eway.h)
#define EWAY_DO_CHANGE_ENEMY 16
// uc_orig: EWAY_DO_CREATE_PLATFORM (fallen/Headers/eway.h)
#define EWAY_DO_CREATE_PLATFORM 17
// uc_orig: EWAY_DO_CREATE_BOMB (fallen/Headers/eway.h)
#define EWAY_DO_CREATE_BOMB 18
// uc_orig: EWAY_DO_ACTIVATE_PRIM (fallen/Headers/eway.h)
#define EWAY_DO_ACTIVATE_PRIM 19
// uc_orig: EWAY_DO_SOUND_EFFECT (fallen/Headers/eway.h)
#define EWAY_DO_SOUND_EFFECT 20
// uc_orig: EWAY_DO_NAV_BEACON (fallen/Headers/eway.h)
#define EWAY_DO_NAV_BEACON 21
// uc_orig: EWAY_DO_SPOT_FX (fallen/Headers/eway.h)
#define EWAY_DO_SPOT_FX 22
// uc_orig: EWAY_DO_WATER_SPOUT (fallen/Headers/eway.h)
#define EWAY_DO_WATER_SPOUT 23
// uc_orig: EWAY_DO_KILL_WAYPOINT (fallen/Headers/eway.h)
#define EWAY_DO_KILL_WAYPOINT 24
// uc_orig: EWAY_DO_OBJECTIVE (fallen/Headers/eway.h)
#define EWAY_DO_OBJECTIVE 25
// uc_orig: EWAY_DO_GROUP_LIFE (fallen/Headers/eway.h)
#define EWAY_DO_GROUP_LIFE 26
// uc_orig: EWAY_DO_GROUP_DEATH (fallen/Headers/eway.h)
#define EWAY_DO_GROUP_DEATH 27
// uc_orig: EWAY_DO_CONVERSATION (fallen/Headers/eway.h)
#define EWAY_DO_CONVERSATION 28
// uc_orig: EWAY_DO_INCREASE_COUNTER (fallen/Headers/eway.h)
#define EWAY_DO_INCREASE_COUNTER 29
// uc_orig: EWAY_DO_EMIT_STEAM (fallen/Headers/eway.h)
#define EWAY_DO_EMIT_STEAM 30
// uc_orig: EWAY_DO_TRANSFER_PLAYER (fallen/Headers/eway.h)
#define EWAY_DO_TRANSFER_PLAYER 31
// uc_orig: EWAY_DO_AUTOSAVE (fallen/Headers/eway.h)
#define EWAY_DO_AUTOSAVE 32
// uc_orig: EWAY_DO_CREATE_BARREL (fallen/Headers/eway.h)
#define EWAY_DO_CREATE_BARREL 33
// uc_orig: EWAY_DO_LOCK_VEHICLE (fallen/Headers/eway.h)
#define EWAY_DO_LOCK_VEHICLE 34
// uc_orig: EWAY_DO_GROUP_RESET (fallen/Headers/eway.h)
#define EWAY_DO_GROUP_RESET 36
// uc_orig: EWAY_DO_VISIBLE_COUNT_UP (fallen/Headers/eway.h)
#define EWAY_DO_VISIBLE_COUNT_UP 37
// uc_orig: EWAY_DO_RESET_COUNTER (fallen/Headers/eway.h)
#define EWAY_DO_RESET_COUNTER 38
// uc_orig: EWAY_DO_CUTSCENE (fallen/Headers/eway.h)
#define EWAY_DO_CUTSCENE 39
// uc_orig: EWAY_DO_CREATE_MIST (fallen/Headers/eway.h)
#define EWAY_DO_CREATE_MIST 40
// uc_orig: EWAY_DO_CHANGE_ENEMY_FLG (fallen/Headers/eway.h)
#define EWAY_DO_CHANGE_ENEMY_FLG 41
// uc_orig: EWAY_DO_STALL_CAR (fallen/Headers/eway.h)
#define EWAY_DO_STALL_CAR 42
// uc_orig: EWAY_DO_EXTEND_COUNTDOWN (fallen/Headers/eway.h)
#define EWAY_DO_EXTEND_COUNTDOWN 43
// uc_orig: EWAY_DO_MOVE_THING (fallen/Headers/eway.h)
#define EWAY_DO_MOVE_THING 44
// uc_orig: EWAY_DO_MAKE_PERSON_PEE (fallen/Headers/eway.h)
#define EWAY_DO_MAKE_PERSON_PEE 45
// uc_orig: EWAY_DO_CONE_PENALTIES (fallen/Headers/eway.h)
#define EWAY_DO_CONE_PENALTIES 46
// uc_orig: EWAY_DO_SIGN (fallen/Headers/eway.h)
#define EWAY_DO_SIGN 47
// uc_orig: EWAY_DO_WAREFX (fallen/Headers/eway.h)
#define EWAY_DO_WAREFX 48
// uc_orig: EWAY_DO_NO_FLOOR (fallen/Headers/eway.h)
#define EWAY_DO_NO_FLOOR 49
// uc_orig: EWAY_DO_SHAKE_CAMERA (fallen/Headers/eway.h)
#define EWAY_DO_SHAKE_CAMERA 50
// uc_orig: EWAY_DO_AMBIENT_CONV (fallen/Headers/eway.h)
#define EWAY_DO_AMBIENT_CONV 51
// uc_orig: EWAY_DO_END_OF_WORLD (fallen/Headers/eway.h)
#define EWAY_DO_END_OF_WORLD 52

// Animal subtypes for CREATE_ANIMAL.
// uc_orig: EWAY_SUBTYPE_ANIMAL_BAT (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_ANIMAL_BAT 1
// uc_orig: EWAY_SUBTYPE_ANIMAL_GARGOYLE (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_ANIMAL_GARGOYLE 2
// uc_orig: EWAY_SUBTYPE_ANIMAL_BALROG (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_ANIMAL_BALROG 3
// uc_orig: EWAY_SUBTYPE_ANIMAL_BANE (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_ANIMAL_BANE 4

// Vehicle subtypes for CREATE_VEHICLE.
// uc_orig: EWAY_SUBTYPE_VEHICLE_VAN (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_VAN 1
// uc_orig: EWAY_SUBTYPE_VEHICLE_HELICOPTER (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_HELICOPTER 2
// uc_orig: EWAY_SUBTYPE_VEHICLE_BIKE (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_BIKE 3
// uc_orig: EWAY_SUBTYPE_VEHICLE_CAR (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_CAR 4
// uc_orig: EWAY_SUBTYPE_VEHICLE_TAXI (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_TAXI 5
// uc_orig: EWAY_SUBTYPE_VEHICLE_POLICE (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_POLICE 6
// uc_orig: EWAY_SUBTYPE_VEHICLE_AMBULANCE (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_AMBULANCE 7
// uc_orig: EWAY_SUBTYPE_VEHICLE_MEATWAGON (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_MEATWAGON 8
// uc_orig: EWAY_SUBTYPE_VEHICLE_SEDAN (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_SEDAN 9
// uc_orig: EWAY_SUBTYPE_VEHICLE_JEEP (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_JEEP 10
// uc_orig: EWAY_SUBTYPE_VEHICLE_WILDCATVAN (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_WILDCATVAN 11

// Camera subtypes.
// uc_orig: EWAY_SUBTYPE_CAMERA_TARGET_PLACE (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_CAMERA_TARGET_PLACE 1
// uc_orig: EWAY_SUBTYPE_CAMERA_TARGET_THING (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_CAMERA_TARGET_THING 2
// uc_orig: EWAY_SUBTYPE_CAMERA_TARGET_NEAR (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_CAMERA_TARGET_NEAR 3
// uc_orig: EWAY_SUBTYPE_CAMERA_LOCK_PLAYER (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_CAMERA_LOCK_PLAYER (1 << 0)
// uc_orig: EWAY_SUBTYPE_CAMERA_LOCK_DIRECTION (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_CAMERA_LOCK_DIRECTION (1 << 1)
// uc_orig: EWAY_SUBTYPE_CAMERA_CANT_INTERRUPT (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_CAMERA_CANT_INTERRUPT (1 << 2)

// Objective subtypes.
// uc_orig: EWAY_SUBTYPE_OBJECTIVE_MAIN (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_OBJECTIVE_MAIN 1
// uc_orig: EWAY_SUBTYPE_OBJECTIVE_SUB (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_OBJECTIVE_SUB 2
// uc_orig: EWAY_SUBTYPE_OBJECTIVE_ADDITIONAL (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_OBJECTIVE_ADDITIONAL 3

// Steam emission subtypes.
// uc_orig: EWAY_SUBTYPE_STEAM_FORWARD (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_STEAM_FORWARD 0
// uc_orig: EWAY_SUBTYPE_STEAM_UP (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_STEAM_UP 1
// uc_orig: EWAY_SUBTYPE_STEAM_DOWN (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_STEAM_DOWN 2

// Vehicle lock subtypes.
// uc_orig: EWAY_SUBTYPE_VEHICLE_LOCK (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_LOCK 0
// uc_orig: EWAY_SUBTYPE_VEHICLE_UNLOCK (fallen/Headers/eway.h)
#define EWAY_SUBTYPE_VEHICLE_UNLOCK 1

// Item creation arg flags.
// uc_orig: EWAY_ARG_ITEM_FOLLOW_PERSON (fallen/Headers/eway.h)
#define EWAY_ARG_ITEM_FOLLOW_PERSON 1
// uc_orig: EWAY_ARG_ITEM_STASHED_IN_PRIM (fallen/Headers/eway.h)
#define EWAY_ARG_ITEM_STASHED_IN_PRIM 2

// Message speaker constants.
// uc_orig: EWAY_MESSAGE_WHO_RADIO (fallen/Headers/eway.h)
#define EWAY_MESSAGE_WHO_RADIO NULL
// uc_orig: EWAY_MESSAGE_WHO_STREETNAME (fallen/Headers/eway.h)
#define EWAY_MESSAGE_WHO_STREETNAME 0xffff
// uc_orig: EWAY_MESSAGE_WHO_TUTORIAL (fallen/Headers/eway.h)
#define EWAY_MESSAGE_WHO_TUTORIAL 0xfffe

// Enemy definition: extra NPC parameters for CREATE_ENEMY / CHANGE_ENEMY waypoints.
// uc_orig: EWAY_Edef (fallen/Headers/eway.h)
typedef struct {
    UBYTE pcom_ai;
    UBYTE pcom_move;
    UBYTE pcom_has;
    UBYTE drop;
    UBYTE pcom_bent;
    UBYTE ai_skill;
    UBYTE zone;
    UBYTE padding;
    UWORD follow;
    UWORD ai_other;
} EWAY_Edef;

// Fake wander message type constants.
// uc_orig: EWAY_FAKE_MESSAGE_NORMAL (fallen/Headers/eway.h)
#define EWAY_FAKE_MESSAGE_NORMAL 0
// uc_orig: EWAY_FAKE_MESSAGE_ANNOYED (fallen/Headers/eway.h)
#define EWAY_FAKE_MESSAGE_ANNOYED 1
// uc_orig: EWAY_FAKE_MESSAGE_GUILTY (fallen/Headers/eway.h)
#define EWAY_FAKE_MESSAGE_GUILTY 2

// Darci move flags.
// uc_orig: EWAY_DARCI_MOVE_PUNCH (fallen/Headers/eway.h)
#define EWAY_DARCI_MOVE_PUNCH (1 << 0)
// uc_orig: EWAY_DARCI_MOVE_KICK (fallen/Headers/eway.h)
#define EWAY_DARCI_MOVE_KICK (1 << 1)

// Waypoint find constants.
// uc_orig: EWAY_DONT_CARE (fallen/Headers/eway.h)
#define EWAY_DONT_CARE (-1)
// uc_orig: EWAY_NO_MATCH (fallen/Headers/eway.h)
#define EWAY_NO_MATCH (-1)
// uc_orig: EWAY_FIND_FIRST (fallen/Headers/eway.h)
#define EWAY_FIND_FIRST (0xffff)

// ====================================================
// Global variable declarations
// ====================================================

// uc_orig: EWAY_cond_upto (fallen/Source/eway.cpp)
extern SLONG EWAY_cond_upto;

// uc_orig: EWAY_cond (fallen/Source/eway.cpp)
extern EWAY_Cond* EWAY_cond;

// uc_orig: EWAY_way (fallen/Source/eway.cpp)
extern EWAY_Way* EWAY_way;

// uc_orig: EWAY_edef (fallen/Source/eway.cpp)
extern EWAY_Edef* EWAY_edef;

// uc_orig: EWAY_mess (fallen/Source/eway.cpp)
extern CBYTE** EWAY_mess;

// uc_orig: EWAY_mess_buffer (fallen/Source/eway.cpp)
extern CBYTE* EWAY_mess_buffer;

// uc_orig: EWAY_way_upto (fallen/Source/eway.cpp)
extern SLONG EWAY_way_upto;

// uc_orig: EWAY_edef_upto (fallen/Source/eway.cpp)
extern SLONG EWAY_edef_upto;

// uc_orig: EWAY_magic_radius_flag (fallen/Source/eway.cpp)
extern EWAY_Way* EWAY_magic_radius_flag;

// uc_orig: EWAY_mess_buffer_upto (fallen/Source/eway.cpp)
extern SLONG EWAY_mess_buffer_upto;

// uc_orig: EWAY_mess_upto (fallen/Source/eway.cpp)
extern SLONG EWAY_mess_upto;

// uc_orig: EWAY_time_accurate (fallen/Source/eway.cpp)
extern SLONG EWAY_time_accurate;

// uc_orig: EWAY_time (fallen/Source/eway.cpp)
extern SLONG EWAY_time;

// uc_orig: EWAY_tick (fallen/Source/eway.cpp)
extern SLONG EWAY_tick;

// uc_orig: EWAY_count_up (fallen/Source/eway.cpp)
extern SLONG EWAY_count_up;

// uc_orig: EWAY_count_up_visible (fallen/Source/eway.cpp)
extern UBYTE EWAY_count_up_visible;

// Countdown timer value for the render path. -1 = inactive. Set per physics tick.
extern SLONG EWAY_hud_countdown_value;

// uc_orig: EWAY_count_up_add_penalties (fallen/Source/eway.cpp)
extern UBYTE EWAY_count_up_add_penalties;

// uc_orig: EWAY_count_up_num_penalties (fallen/Source/eway.cpp)
extern SWORD EWAY_count_up_num_penalties;

// uc_orig: EWAY_count_up_penalty_timer (fallen/Source/eway.cpp)
extern UWORD EWAY_count_up_penalty_timer;

// uc_orig: EWAY_cam_jumped (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_jumped;

// uc_orig: EWAY_timer (fallen/Source/eway.cpp)
extern UWORD* EWAY_timer;

// uc_orig: EWAY_counter (fallen/Source/eway.cpp)
extern UBYTE* EWAY_counter;

// uc_orig: EWAY_fake_wander_text_normal_index (fallen/Source/eway.cpp)
extern UWORD EWAY_fake_wander_text_normal_index;

// uc_orig: EWAY_fake_wander_text_normal_number (fallen/Source/eway.cpp)
extern UWORD EWAY_fake_wander_text_normal_number;

// uc_orig: EWAY_fake_wander_text_guilty_index (fallen/Source/eway.cpp)
extern UWORD EWAY_fake_wander_text_guilty_index;

// uc_orig: EWAY_fake_wander_text_guilty_number (fallen/Source/eway.cpp)
extern UWORD EWAY_fake_wander_text_guilty_number;

// uc_orig: EWAY_fake_wander_text_annoyed_index (fallen/Source/eway.cpp)
extern UWORD EWAY_fake_wander_text_annoyed_index;

// uc_orig: EWAY_fake_wander_text_annoyed_number (fallen/Source/eway.cpp)
extern UWORD EWAY_fake_wander_text_annoyed_number;

// uc_orig: EWAY_tutorial_string (fallen/Source/eway.cpp)
extern CBYTE* EWAY_tutorial_string;

// uc_orig: EWAY_tutorial_counter (fallen/Source/eway.cpp)
extern SLONG EWAY_tutorial_counter;

// uc_orig: EWAY_darci_move (fallen/Source/eway.cpp)
extern UBYTE EWAY_darci_move;

// uc_orig: EWAY_used_thing (fallen/Source/eway.cpp)
extern UWORD EWAY_used_thing;

// uc_orig: EWAY_cam_active (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_active;

// uc_orig: EWAY_cam_goinactive (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_goinactive;

// uc_orig: EWAY_cam_x (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_x;

// uc_orig: EWAY_cam_y (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_y;

// uc_orig: EWAY_cam_z (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_z;

// uc_orig: EWAY_cam_dx (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_dx;

// uc_orig: EWAY_cam_dy (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_dy;

// uc_orig: EWAY_cam_dz (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_dz;

// uc_orig: EWAY_cam_yaw (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_yaw;

// uc_orig: EWAY_cam_pitch (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_pitch;

// uc_orig: EWAY_cam_want_yaw (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_want_yaw;

// uc_orig: EWAY_cam_want_pitch (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_want_pitch;

// uc_orig: EWAY_cam_waypoint (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_waypoint;

// uc_orig: EWAY_cam_target (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_target;

// uc_orig: EWAY_cam_delay (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_delay;

// uc_orig: EWAY_cam_speed (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_speed;

// uc_orig: EWAY_cam_freeze (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_freeze;

// uc_orig: EWAY_cam_cant_interrupt (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_cant_interrupt;

// uc_orig: EWAY_cam_thing (fallen/Source/eway.cpp)
extern UWORD EWAY_cam_thing;

// uc_orig: EWAY_cam_targx (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_targx;

// uc_orig: EWAY_cam_targy (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_targy;

// uc_orig: EWAY_cam_targz (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_targz;

// uc_orig: EWAY_cam_lens (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_lens;

// uc_orig: EWAY_cam_warehouse (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_warehouse;

// uc_orig: EWAY_cam_lock (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_lock;

// uc_orig: EWAY_cam_last_yaw (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_last_yaw;

// uc_orig: EWAY_cam_last_x (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_last_x;

// uc_orig: EWAY_cam_last_y (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_last_y;

// uc_orig: EWAY_cam_last_z (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_last_z;

// uc_orig: EWAY_cam_skip (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_skip;

// uc_orig: EWAY_cam_last_dyaw (fallen/Source/eway.cpp)
extern SLONG EWAY_cam_last_dyaw;

// uc_orig: GAME_cut_scene (fallen/Source/eway.cpp)
extern UBYTE GAME_cut_scene;

// uc_orig: EWAY_message (fallen/Source/eway.cpp)
extern CBYTE EWAY_message[128];

// uc_orig: EWAY_conv_active (fallen/Source/eway.cpp)
extern UBYTE EWAY_conv_active;

// uc_orig: EWAY_conv_waypoint (fallen/Source/eway.cpp)
extern UWORD EWAY_conv_waypoint;

// uc_orig: EWAY_conv_person_a (fallen/Source/eway.cpp)
extern THING_INDEX EWAY_conv_person_a;

// uc_orig: EWAY_conv_person_b (fallen/Source/eway.cpp)
extern THING_INDEX EWAY_conv_person_b;

// uc_orig: EWAY_conv_str (fallen/Source/eway.cpp)
extern UWORD EWAY_conv_str;

// uc_orig: EWAY_conv_str_count (fallen/Source/eway.cpp)
extern UWORD EWAY_conv_str_count;

// uc_orig: EWAY_conv_timer (fallen/Source/eway.cpp)
extern SLONG EWAY_conv_timer;

// uc_orig: EWAY_conv_skip (fallen/Source/eway.cpp)
extern SLONG EWAY_conv_skip;

// uc_orig: EWAY_conv_ambient (fallen/Source/eway.cpp)
extern SLONG EWAY_conv_ambient;

// uc_orig: EWAY_conv_talk (fallen/Source/eway.cpp)
extern SLONG EWAY_conv_talk;

// uc_orig: talk_thing (fallen/Source/eway.cpp)
extern Thing* talk_thing;

// uc_orig: crap_levels (fallen/Source/eway.cpp)
extern CBYTE* crap_levels[];

// uc_orig: ob_x (fallen/Source/eway.cpp)
extern SLONG ob_x;

// uc_orig: ob_y (fallen/Source/eway.cpp)
extern SLONG ob_y;

// uc_orig: ob_z (fallen/Source/eway.cpp)
extern SLONG ob_z;

// uc_orig: ob_yaw (fallen/Source/eway.cpp)
extern SLONG ob_yaw;

// uc_orig: ob_prim (fallen/Source/eway.cpp)
extern SLONG ob_prim;

// uc_orig: ob_index (fallen/Source/eway.cpp)
extern SLONG ob_index;

#endif // MISSIONS_EWAY_GLOBALS_H
