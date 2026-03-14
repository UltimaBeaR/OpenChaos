//
// Direct play stuff.
//

#include "DDLib.h"
#include "net.h"


//
// Error checking...
//

#ifndef VERIFY
#ifdef  NDEBUG
#define VERIFY(x) {x;}
#else
#define VERIFY(x) {ASSERT(x);}
#endif
#endif


//
// TRUE if this machine is the host.
// The local player number.
// 

UBYTE NET_i_am_host;
UBYTE NET_local_player;

//
// All the players.
//

typedef struct
{
	UBYTE used;
	UBYTE player_id;
	UWORD shit;
	//DPID  dpid;
	CBYTE name[NET_NAME_LENGTH];
	
} NET_Player;

#define NET_MAX_PLAYERS 32

NET_Player NET_player[NET_MAX_PLAYERS];

//
// The buffer into which messages are recieved.
//

CBYTE NET_buffer[NET_MESSAGE_LENGTH];

SLONG net_init_done=0;

void NET_init(void)
{
}


void NET_kill(void)
{
}



SLONG NET_get_connection_number()
{
	return 0;
}

CBYTE *NET_get_connection_name(SLONG connection)
{
	return nullptr;
}


SLONG NET_connection_make(SLONG connection)
{
	return FALSE;
}

SLONG NET_create_session(CBYTE *session_name, SLONG max_players, CBYTE *my_player_name)
{
	return FALSE;
}

SLONG NET_get_session_number()
{
	return 0;
}

NET_Sinfo NET_get_session_info(SLONG session)
{
	NET_Sinfo ans;

	return ans;
}

SLONG NET_join_session(SLONG session, CBYTE *my_player_name)
{
	return FALSE;
}


UBYTE NET_start_game()
{
	return 0;
}


void NET_leave_session()
{
}


SLONG NET_get_num_players()
{
	return 1;
}

CBYTE *NET_get_player_name(SLONG player)
{
	return "Unknown";
}

void NET_message_send(
		UBYTE  player_id,
		void  *data,
		UWORD  num_bytes)
{
}


SLONG NET_message_waiting()
{
	return 0;
}

void NET_message_get(NET_Message *ans)
{
		return;
}










