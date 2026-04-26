#ifndef GAME_NETWORK_STATE_GLOBALS_H
#define GAME_NETWORK_STATE_GLOBALS_H

// Stub network state: multiplayer was removed, these are always single-player defaults.
// Used by thing.cpp, player.cpp and other systems that check for network mode.

#include "engine/core/types.h"

// uc_orig: CNET_network_game (fallen/Source/cnet.cpp)
extern UBYTE CNET_network_game;

// uc_orig: CNET_player_id (fallen/Source/cnet.cpp)
extern UBYTE CNET_player_id;

// uc_orig: CNET_num_players (fallen/Source/cnet.cpp)
extern UBYTE CNET_num_players;


#endif // GAME_NETWORK_STATE_GLOBALS_H
