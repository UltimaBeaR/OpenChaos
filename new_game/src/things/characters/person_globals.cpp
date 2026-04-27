// uc_orig: Person.cpp (fallen/Source/Person.cpp)
// Global variable definitions for the person subsystem.

#include "things/characters/person.h"
#include "things/characters/cop_globals.h"
#include "things/characters/darci_globals.h"
#include "things/characters/roper_globals.h"
#include "things/core/statedef.h"

// State functions still in old/fallen/Source/Person.cpp (not yet migrated).
// All functions from chunks 1-12 are now defined in actors/characters/person.cpp
// and declared in actors/characters/person.h (included above via person.h -> person_globals.h chain).
// person.h is included at the top of this file.
extern void fn_person_search(Thing* p_person);
extern void fn_person_carry(Thing* p_person);

// uc_orig: anim_type (fallen/Source/Person.cpp)
UBYTE anim_type[] = {
    ANIM_TYPE_DARCI, // PERSON_DARCI
    ANIM_TYPE_ROPER, // PERSON_ROPER
    ANIM_TYPE_CIV, // PERSON_COP
    ANIM_TYPE_CIV, // PERSON_CIV
    ANIM_TYPE_CIV, // PERSON_THUG_RASTA
    ANIM_TYPE_CIV, // PERSON_THUG_GREY
    ANIM_TYPE_CIV, // PERSON_THUG_RED
    ANIM_TYPE_DARCI, // PERSON_SLAG_TART
    ANIM_TYPE_DARCI, // PERSON_SLAG_FATUGLY
    ANIM_TYPE_DARCI, // PERSON_HOSTAGE
    ANIM_TYPE_CIV, // PERSON_MECHANIC
    ANIM_TYPE_CIV, // PERSON_TRAMP
    ANIM_TYPE_CIV, // PERSON_MIB1
    ANIM_TYPE_CIV, // PERSON_MIB2
    ANIM_TYPE_CIV, // PERSON_MIB3
};

// uc_orig: mesh_type (fallen/Source/Person.cpp)
UBYTE mesh_type[] = {
    0, // PERSON_DARCI
    0, // PERSON_ROPER
    4, // PERSON_COP
    7, // PERSON_CIV
    0, // PERSON_THUG_RASTA
    1, // PERSON_THUG_GREY
    2, // PERSON_THUG_RED
    1, // PERSON_SLAG_TART
    2, // PERSON_SLAG_FATUGLY
    3, // PERSON_HOSTAGE
    3, // PERSON_MECHANIC (Ed Miller)
    6, // PERSON_TRAMP
    5, // PERSON_MIB1
    5, // PERSON_MIB2
    5, // PERSON_MIB3
};

// Dispatch table: PERSON_TYPE -> state function table.
// All NPC types except Darci and Roper use cop_states.
// uc_orig: people_functions (fallen/Source/Person.cpp)
GenusFunctions people_functions[] = {
    { PERSON_DARCI, darci_states },
    { PERSON_ROPER, roper_states },
    { PERSON_COP, cop_states },
    { PERSON_CIV, cop_states },
    { PERSON_THUG_RASTA, cop_states },
    { PERSON_THUG_GREY, cop_states },
    { PERSON_THUG_RED, cop_states },
    { PERSON_SLAG_TART, cop_states },
    { PERSON_SLAG_FATUGLY, cop_states },
    { PERSON_HOSTAGE, cop_states },
    { PERSON_MECHANIC, cop_states },
    { PERSON_TRAMP, cop_states },
    { PERSON_MIB1, cop_states },
    { PERSON_MIB2, cop_states },
    { PERSON_MIB3, cop_states },
};

// Generic state table shared by all character types.
// Character-specific tables (darci_states, cop_states) override individual entries.
// uc_orig: generic_people_functions (fallen/Source/Person.cpp)
StateFunction generic_people_functions[] = {
    { STATE_INIT, NULL },
    { STATE_NORMAL, NULL },
    { STATE_HIT, NULL },
    { STATE_ABOUT_TO_REMOVE, NULL },
    { STATE_REMOVE_ME, NULL },

    { STATE_MOVEING, fn_person_moveing },
    { STATE_IDLE, fn_person_idle },
    { STATE_LANDING, NULL },
    { STATE_JUMPING, fn_person_jumping },
    { STATE_FIGHTING, fn_person_fighting },
    { STATE_FALLING, NULL },
    { STATE_USE_SCENERY, NULL },
    { STATE_DOWN, NULL },
    { STATE_HIT, NULL },
    { STATE_CHANGE_LOCATION, NULL },
    { STATE_DRIVING, NULL },
    { STATE_DYING, fn_person_dying },
    { STATE_DEAD, fn_person_dead },
    { STATE_DANGLING, fn_person_dangling },
    { STATE_CLIMB_LADDER, fn_person_laddering },
    { STATE_HIT_RECOIL, fn_person_recoil },
    { STATE_CLIMBING, fn_person_climbing },
    { STATE_GUN, fn_person_gun },
    { 0, NULL },
    { 0, NULL },
    { 0, NULL }, // STATE_NAVIGATING removed (dead MAV stub)
    { STATE_WAIT, fn_person_wait },
    { STATE_FIGHT, fn_person_fight },
    { 0, NULL }, // stand up (unused slot)
    { 0, NULL }, // STATE_MAVIGATING removed (dead MAV stub)
    { STATE_GRAPPLING, fn_person_grapple },
    { STATE_GOTOING, fn_person_goto },
    { STATE_CANNING, fn_person_can },
    { STATE_CIRCLING, fn_person_circle },
    { STATE_HUG_WALL, fn_person_hug_wall },
    { STATE_SEARCH, fn_person_search },
    { STATE_CARRY, fn_person_carry },
    { STATE_FLOAT, fn_person_float },
    { 0, NULL }
};

// Per-type initial health values.
// uc_orig: health (fallen/Source/Person.cpp)
SWORD health[PERSON_NUM_TYPES] = {
    200, // PERSON_DARCI
    400, // PERSON_ROPER
    200, // PERSON_COP
    130, // PERSON_CIV
    200, // PERSON_THUG_RASTA
    200, // PERSON_THUG_GREY
    200, // PERSON_THUG_RED
    130, // PERSON_SLAG_TART
    130, // PERSON_SLAG_FATUGLY
    200, // PERSON_HOSTAGE
    200, // PERSON_MECHANIC
    200, // PERSON_TRAMP
    700, // PERSON_MIB1
    700, // PERSON_MIB2
    700, // PERSON_MIB3
};

// uc_orig: stat_killed_thug (fallen/Source/Person.cpp)
SLONG stat_killed_thug;
// uc_orig: stat_killed_innocent (fallen/Source/Person.cpp)
SLONG stat_killed_innocent;
// uc_orig: stat_arrested_thug (fallen/Source/Person.cpp)
SLONG stat_arrested_thug;
// uc_orig: stat_arrested_innocent (fallen/Source/Person.cpp)
SLONG stat_arrested_innocent;
// uc_orig: stat_count_bonus (fallen/Source/Person.cpp)
SLONG stat_count_bonus;
// uc_orig: stat_start_time (fallen/Source/Person.cpp)
// BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
uint64_t stat_start_time;
// uc_orig: stat_game_time (fallen/Source/Person.cpp)
// BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
uint64_t stat_game_time;

// uc_orig: global_person (fallen/Source/Person.cpp)
UBYTE global_person = 0;

// uc_orig: player_dlight (fallen/Source/Person.cpp)
UWORD player_dlight = 0;

// uc_orig: timer_bored (fallen/Source/Person.cpp)
ULONG timer_bored = 0;

// uc_orig: dead_tween (fallen/Source/Person.cpp)
DrawTween dead_tween;

// uc_orig: combo_display (fallen/Source/Person.cpp)
UBYTE combo_display = 0;

// uc_orig: near_facet (fallen/Source/Person.cpp)
UWORD near_facet = 0;

// uc_orig: kick_or_punch (fallen/Source/Person.cpp)
SLONG kick_or_punch = 0;
