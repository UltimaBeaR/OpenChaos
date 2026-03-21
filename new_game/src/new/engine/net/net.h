#ifndef ENGINE_NET_NET_H
#define ENGINE_NET_NET_H

// Networking stub — all multiplayer functionality was removed before ship.
// All functions return empty/zero/false values. The game runs as single-player only.

#include "core/types.h"

// uc_orig: NET_PLAYER_NONE (fallen/DDLibrary/Headers/net.h)
#define NET_PLAYER_NONE 255
// uc_orig: NET_PLAYER_ALL (fallen/DDLibrary/Headers/net.h)
#define NET_PLAYER_ALL 254
// uc_orig: NET_PLAYER_SYSTEM (fallen/DDLibrary/Headers/net.h)
#define NET_PLAYER_SYSTEM 253

// Maximum length of a player or session name.
// uc_orig: NET_NAME_LENGTH (fallen/DDLibrary/Headers/net.h)
#define NET_NAME_LENGTH 64

// Maximum size of a network message in bytes.
// uc_orig: NET_MESSAGE_LENGTH (fallen/DDLibrary/Headers/net.h)
#define NET_MESSAGE_LENGTH (16 * 1024)

// System message type codes.
// uc_orig: NET_SYSMESS_NOP (fallen/DDLibrary/Headers/net.h)
#define NET_SYSMESS_NOP 0
// uc_orig: NET_SYSMESS_START_GAME (fallen/DDLibrary/Headers/net.h)
#define NET_SYSMESS_START_GAME 1
// uc_orig: NET_SYSMESS_LOST_CONNECTION (fallen/DDLibrary/Headers/net.h)
#define NET_SYSMESS_LOST_CONNECTION 2
// uc_orig: NET_SYSMESS_HOST_QUIT_OUT (fallen/DDLibrary/Headers/net.h)
#define NET_SYSMESS_HOST_QUIT_OUT 3

// Session info returned by NET_get_session_info.
// uc_orig: NET_Sinfo (fallen/DDLibrary/Headers/net.h)
typedef struct {
    char name[NET_NAME_LENGTH];
} NET_Sinfo;

// Received message from a player or the system.
// uc_orig: NET_Message (fallen/DDLibrary/Headers/net.h)
typedef struct {
    UBYTE player_id;
    UBYTE shit1;
    UWORD shit2;
    union {
        struct {
            UBYTE sysmess;
            UBYTE player_id;
            UBYTE shite;
        } system;
        struct {
            UWORD num_bytes;
            UWORD more_shit;
            void* data;
        } player;
    };
} NET_Message;

// uc_orig: NET_init (fallen/DDLibrary/Source/net.cpp)
void NET_init(void);
// uc_orig: NET_kill (fallen/DDLibrary/Source/net.cpp)
void NET_kill(void);

// uc_orig: NET_get_connection_number (fallen/DDLibrary/Source/net.cpp)
SLONG NET_get_connection_number(void);
// uc_orig: NET_get_connection_name (fallen/DDLibrary/Source/net.cpp)
CBYTE* NET_get_connection_name(SLONG connection);
// uc_orig: NET_connection_make (fallen/DDLibrary/Source/net.cpp)
SLONG NET_connection_make(SLONG connection);

// uc_orig: NET_create_session (fallen/DDLibrary/Source/net.cpp)
SLONG NET_create_session(CBYTE* name, SLONG max_players, CBYTE* my_player_name);
// uc_orig: NET_get_session_number (fallen/DDLibrary/Source/net.cpp)
SLONG NET_get_session_number(void);
// uc_orig: NET_get_session_info (fallen/DDLibrary/Source/net.cpp)
NET_Sinfo NET_get_session_info(SLONG session);
// uc_orig: NET_join_session (fallen/DDLibrary/Source/net.cpp)
SLONG NET_join_session(SLONG session, CBYTE* my_player_name);
// uc_orig: NET_leave_session (fallen/DDLibrary/Source/net.cpp)
void NET_leave_session(void);

// uc_orig: NET_start_game (fallen/DDLibrary/Source/net.cpp)
UBYTE NET_start_game(void);
// uc_orig: NET_get_num_players (fallen/DDLibrary/Source/net.cpp)
SLONG NET_get_num_players(void);
// uc_orig: NET_get_player_name (fallen/DDLibrary/Source/net.cpp)
CBYTE* NET_get_player_name(SLONG player);

// uc_orig: NET_message_send (fallen/DDLibrary/Source/net.cpp)
void NET_message_send(UBYTE player_id, void* data, UWORD num_bytes);
// uc_orig: NET_message_waiting (fallen/DDLibrary/Source/net.cpp)
SLONG NET_message_waiting(void);
// uc_orig: NET_message_get (fallen/DDLibrary/Source/net.cpp)
void NET_message_get(NET_Message* answer);

#endif // ENGINE_NET_NET_H
