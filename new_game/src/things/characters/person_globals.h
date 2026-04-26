#ifndef THINGS_CHARACTERS_PERSON_GLOBALS_H
#define THINGS_CHARACTERS_PERSON_GLOBALS_H

// Global variable declarations for the person subsystem.

#include <stdint.h>
#include "engine/core/types.h"
#include "things/core/state.h"  // GenusFunctions, StateFunction

// Dispatch table: PERSON_TYPE -> state function table (darci_states / cop_states).
// uc_orig: people_functions (fallen/Source/Person.cpp)
extern GenusFunctions people_functions[];

// Generic state table shared by all character types.
// uc_orig: generic_people_functions (fallen/Source/Person.cpp)
extern StateFunction generic_people_functions[];

// Per-type initial health values.
// uc_orig: health (fallen/Source/Person.cpp)
extern SWORD health[];

// Per-type animation type table (ANIM_TYPE_DARCI / ANIM_TYPE_CIV / ...).
// uc_orig: anim_type (fallen/Source/Person.cpp)
extern UBYTE anim_type[];

// Per-type mesh ID table (which mesh model to use for each person type).
// uc_orig: mesh_type (fallen/Source/Person.cpp)
extern UBYTE mesh_type[];

// Counters for end-of-mission statistics.
// uc_orig: stat_killed_thug (fallen/Source/Person.cpp)
extern SLONG stat_killed_thug;
// uc_orig: stat_killed_innocent (fallen/Source/Person.cpp)
extern SLONG stat_killed_innocent;
// uc_orig: stat_arrested_thug (fallen/Source/Person.cpp)
extern SLONG stat_arrested_thug;
// uc_orig: stat_arrested_innocent (fallen/Source/Person.cpp)
extern SLONG stat_arrested_innocent;
// uc_orig: stat_count_bonus (fallen/Source/Person.cpp)
extern SLONG stat_count_bonus;
// uc_orig: stat_start_time (fallen/Source/Person.cpp)
// BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
extern uint64_t stat_start_time;
// uc_orig: stat_game_time (fallen/Source/Person.cpp)
// BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
extern uint64_t stat_game_time;

// Counter incremented each time create_person() is called.
// uc_orig: global_person (fallen/Source/Person.cpp)
extern UBYTE global_person;

// Dynamic light ID assigned to the player character (0 = none).
// uc_orig: player_dlight (fallen/Source/Person.cpp)
extern UWORD player_dlight;

// Boredom / context timer for general_process_player().
// uc_orig: timer_bored (fallen/Source/Person.cpp)
extern ULONG timer_bored;

// Dummy DrawTween used to reset dying persons on respawn.
// uc_orig: dead_tween (fallen/Source/Person.cpp)
extern DrawTween dead_tween;

// Tracks the last combo step direction for the fight-combo display (0=none, 1=punch, 2=kick).
// Read by pcom.cpp for the HUD combo indicator.
// uc_orig: combo_display (fallen/Source/Person.cpp)
extern UBYTE combo_display;

// The facet index found by check_near_facet() — shared with can_i_hug_wall() and fn_person_hug_wall().
// Set as a side effect of check_near_facet().
// uc_orig: near_facet (fallen/Source/Person.cpp)
extern UWORD near_facet;

// Alternating punch/kick selector for fn_person_circle() AI combat decisions.
// Incremented each time the circle AI decides to attack; odd=punch, even=kick.
// uc_orig: kick_or_punch (fallen/Source/Person.cpp)
extern SLONG kick_or_punch;


#endif // THINGS_CHARACTERS_PERSON_GLOBALS_H
