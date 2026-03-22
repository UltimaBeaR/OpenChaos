#ifndef ACTORS_CHARACTERS_DARCI_GLOBALS_H
#define ACTORS_CHARACTERS_DARCI_GLOBALS_H

#include "core/types.h"
#include "actors/core/state.h"

// THING_INDEX is defined in Game.h; replicate guard pattern for standalone use.
#ifndef THING_INDEX
#define THING_INDEX UWORD
#endif

// uc_orig: darci_states (fallen/Source/Darci.cpp)
// State dispatch table for Darci (the player-controlled character).
extern StateFunction darci_states[];

// uc_orig: air_walking (fallen/Source/Darci.cpp)
// Debug/legacy flag — was used to track whether Darci is walking on air. Unused in current code.
extern SLONG air_walking;

// uc_orig: history_thing (fallen/Source/Darci.cpp)
// Ring buffer of recently touched thing indices — legacy debug data, no longer used.
extern THING_INDEX history_thing[100];

// uc_orig: history (fallen/Source/Darci.cpp)
// Current index into history_thing ring buffer.
extern SWORD history;

// uc_orig: just_started_falling_off_backwards (fallen/Source/Darci.cpp)
// Set to UC_TRUE for one tick when Darci begins a backwards fall-off animation.
// Read by slide_along during projectile_move_thing.
extern UBYTE just_started_falling_off_backwards;

#endif // ACTORS_CHARACTERS_DARCI_GLOBALS_H
