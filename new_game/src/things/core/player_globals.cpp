#include "engine/platform/uc_common.h"
#include "things/core/player_globals.h"
#include "things/characters/roper_globals.h"

// Forward-declare darci_states (Darci.cpp not yet migrated).
// uc_orig: darci_states (fallen/Source/Darci.cpp)
extern StateFunction darci_states[];

// uc_orig: player_functions (fallen/Source/Player.cpp)
GenusFunctions player_functions[] = {
    { PLAYER_NONE, NULL },
    { PLAYER_DARCI, darci_states },
    { PLAYER_ROPER, roper_states }
};

// uc_orig: player_pos (fallen/Source/Player.cpp)
GameCoord player_pos;
