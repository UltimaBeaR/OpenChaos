#ifndef ENGINE_NET_NET_GLOBALS_H
#define ENGINE_NET_NET_GLOBALS_H

#include "engine/net/net.h"

// uc_orig: NET_MAX_PLAYERS (fallen/DDLibrary/Source/net.cpp)
#define NET_MAX_PLAYERS 32

// Internal player slot — tracks connected player names and IDs.
// uc_orig: NET_Player (fallen/DDLibrary/Source/net.cpp)
typedef struct {
    UBYTE used;
    UBYTE player_id;
    UWORD shit;
    CBYTE name[NET_NAME_LENGTH];
} NET_Player;

// uc_orig: NET_player (fallen/DDLibrary/Source/net.cpp)
extern NET_Player NET_player[NET_MAX_PLAYERS];

// uc_orig: NET_i_am_host (fallen/DDLibrary/Source/net.cpp)
extern UBYTE NET_i_am_host;

// uc_orig: NET_local_player (fallen/DDLibrary/Source/net.cpp)
extern UBYTE NET_local_player;

// uc_orig: NET_buffer (fallen/DDLibrary/Source/net.cpp)
extern CBYTE NET_buffer[NET_MESSAGE_LENGTH];

// uc_orig: net_init_done (fallen/DDLibrary/Source/net.cpp)
extern SLONG net_init_done;

#endif // ENGINE_NET_NET_GLOBALS_H
