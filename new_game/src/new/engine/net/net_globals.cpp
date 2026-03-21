#include "engine/net/net_globals.h"

// uc_orig: NET_player (fallen/DDLibrary/Source/net.cpp)
NET_Player NET_player[NET_MAX_PLAYERS];

// uc_orig: NET_i_am_host (fallen/DDLibrary/Source/net.cpp)
UBYTE NET_i_am_host;

// uc_orig: NET_local_player (fallen/DDLibrary/Source/net.cpp)
UBYTE NET_local_player;

// uc_orig: NET_buffer (fallen/DDLibrary/Source/net.cpp)
CBYTE NET_buffer[NET_MESSAGE_LENGTH];

// uc_orig: net_init_done (fallen/DDLibrary/Source/net.cpp)
SLONG net_init_done = 0;
