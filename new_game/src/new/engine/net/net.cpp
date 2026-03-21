#include "engine/net/net.h"
#include "engine/net/net_globals.h"

// uc_orig: NET_init (fallen/DDLibrary/Source/net.cpp)
void NET_init(void)
{
}

// uc_orig: NET_kill (fallen/DDLibrary/Source/net.cpp)
void NET_kill(void)
{
}

// uc_orig: NET_get_connection_number (fallen/DDLibrary/Source/net.cpp)
SLONG NET_get_connection_number()
{
    return 0;
}

// uc_orig: NET_get_connection_name (fallen/DDLibrary/Source/net.cpp)
CBYTE* NET_get_connection_name(SLONG connection)
{
    return nullptr;
}

// uc_orig: NET_connection_make (fallen/DDLibrary/Source/net.cpp)
SLONG NET_connection_make(SLONG connection)
{
    return FALSE;
}

// uc_orig: NET_create_session (fallen/DDLibrary/Source/net.cpp)
SLONG NET_create_session(CBYTE* session_name, SLONG max_players, CBYTE* my_player_name)
{
    return FALSE;
}

// uc_orig: NET_get_session_number (fallen/DDLibrary/Source/net.cpp)
SLONG NET_get_session_number()
{
    return 0;
}

// uc_orig: NET_get_session_info (fallen/DDLibrary/Source/net.cpp)
NET_Sinfo NET_get_session_info(SLONG session)
{
    NET_Sinfo ans;
    return ans;
}

// uc_orig: NET_join_session (fallen/DDLibrary/Source/net.cpp)
SLONG NET_join_session(SLONG session, CBYTE* my_player_name)
{
    return FALSE;
}

// uc_orig: NET_start_game (fallen/DDLibrary/Source/net.cpp)
UBYTE NET_start_game()
{
    return 0;
}

// uc_orig: NET_leave_session (fallen/DDLibrary/Source/net.cpp)
void NET_leave_session()
{
}

// uc_orig: NET_get_num_players (fallen/DDLibrary/Source/net.cpp)
SLONG NET_get_num_players()
{
    return 1;
}

// uc_orig: NET_get_player_name (fallen/DDLibrary/Source/net.cpp)
CBYTE* NET_get_player_name(SLONG player)
{
    return "Unknown";
}

// uc_orig: NET_message_send (fallen/DDLibrary/Source/net.cpp)
void NET_message_send(
    UBYTE player_id,
    void* data,
    UWORD num_bytes)
{
}

// uc_orig: NET_message_waiting (fallen/DDLibrary/Source/net.cpp)
SLONG NET_message_waiting()
{
    return 0;
}

// uc_orig: NET_message_get (fallen/DDLibrary/Source/net.cpp)
void NET_message_get(NET_Message* ans)
{
    return;
}
