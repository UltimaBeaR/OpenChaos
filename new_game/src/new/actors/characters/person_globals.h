#ifndef ACTORS_CHARACTERS_PERSON_GLOBALS_H
#define ACTORS_CHARACTERS_PERSON_GLOBALS_H

// Global variable declarations for the person subsystem.

#include "core/types.h"
#include "actors/core/state.h"  // GenusFunctions, StateFunction

// Dispatch table: PERSON_TYPE -> state function table (darci_states / cop_states).
// uc_orig: people_functions (fallen/Source/Person.cpp)
extern GenusFunctions people_functions[];

// Generic state table shared by all character types.
// uc_orig: generic_people_functions (fallen/Source/Person.cpp)
extern StateFunction generic_people_functions[];

// Movement mode name strings, indexed by PERSON_MODE_* constants.
// uc_orig: PERSON_mode_name (fallen/Source/Person.cpp)
extern CBYTE* PERSON_mode_name[];

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
extern SLONG stat_start_time;
// uc_orig: stat_game_time (fallen/Source/Person.cpp)
extern SLONG stat_game_time;

// Tracks which map tiles the player has visited (for fog-of-war / minimap).
// 16 bytes wide * 128 entries = 16*128 bit array.
// uc_orig: player_visited (fallen/Source/Person.cpp)
extern UBYTE player_visited[16][128];

// Counter incremented each time create_person() is called.
// uc_orig: global_person (fallen/Source/Person.cpp)
extern UBYTE global_person;

// Dynamic light ID assigned to the player character (0 = none).
// uc_orig: player_dlight (fallen/Source/Person.cpp)
extern UWORD player_dlight;

// Boredom / context timer for general_process_player().
// uc_orig: timer_bored (fallen/Source/Person.cpp)
extern ULONG timer_bored;

#endif // ACTORS_CHARACTERS_PERSON_GLOBALS_H
