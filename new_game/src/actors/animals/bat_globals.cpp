#include "actors/animals/bat.h"
#include "actors/animals/bat_globals.h"

// State name strings for debug/assert output. Indexed by BAT_STATE_*.
// uc_orig: BAT_state_name (fallen/Source/bat.cpp)
const char* BAT_state_name[] = {
    "Idle",
    "Goto",
    "Circle",
    "Attack",
    "Dying",
    "Dead",
    "Ground",
    "Recoil",
    "Balrog wander",
    "Balrog roar",
    "Balrog follow",
    "Balrog charge",
    "Balrog swipe",
    "Balrog stomp",
    "Balrog fireball",
    "Balrog idle",
    "Bane idle",
    "Bane attack",
    "Bane start"
};

// THING_INDEX array of bodies being levitated by the Bane boss.
// uc_orig: BAT_summon (fallen/Source/bat.cpp)
UWORD BAT_summon[BAT_SUMMON_NUM_BODIES];
