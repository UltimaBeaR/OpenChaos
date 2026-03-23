#ifndef ACTORS_ANIMALS_BAT_GLOBALS_H
#define ACTORS_ANIMALS_BAT_GLOBALS_H

#include "actors/animals/bat.h"

// uc_orig: BAT_SUMMON_NUM_BODIES (fallen/Source/bat.cpp)
#define BAT_SUMMON_NUM_BODIES 4

// uc_orig: BAT_summon (fallen/Source/bat.cpp)
// THING_INDEX array of bodies being levitated by the Bane boss.
extern UWORD BAT_summon[BAT_SUMMON_NUM_BODIES];

// uc_orig: BAT_state_name (fallen/Source/bat.cpp)
// State name strings for debug/assert output. Indexed by BAT_STATE_*.
extern const char* BAT_state_name[];

#endif // ACTORS_ANIMALS_BAT_GLOBALS_H
