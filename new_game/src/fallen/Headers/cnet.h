//
// For doing network game stuff...
//

#ifndef FALLEN_HEADERS_CNET_H
#define FALLEN_HEADERS_CNET_H

//
// The configuring loop.
//

extern UBYTE CNET_network_game;
extern UBYTE CNET_i_am_host;
extern UBYTE CNET_num_players;
extern UBYTE CNET_player_id;

SLONG CNET_configure(void);

#endif // FALLEN_HEADERS_CNET_H
