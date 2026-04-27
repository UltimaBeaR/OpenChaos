#include "game/network_state_globals.h"

// Stub network state: always single-player.

// uc_orig: CNET_network_game (fallen/Source/cnet.cpp)
UBYTE CNET_network_game;

// uc_orig: CNET_player_id (fallen/Source/cnet.cpp)
UBYTE CNET_player_id = 0;

// uc_orig: CNET_num_players (fallen/Source/cnet.cpp)
UBYTE CNET_num_players = 1;
