#include <platform.h>
#include "missions/game_types.h"
#include "ui/menus/cnet.h"
#include "ui/menus/cnet_globals.h"

// Networking was removed — CNET_configure is now a no-op stub.
// The entire DirectPlay session UI (connection/session/lobby) is dead code.
// uc_orig: CNET_configure (fallen/Source/cnet.cpp)
SLONG CNET_configure()
{
    CNET_network_game = UC_FALSE;
    CNET_i_am_host = UC_FALSE;
    CNET_num_players = 1;
    CNET_player_id = 0;
    return UC_FALSE;
}
