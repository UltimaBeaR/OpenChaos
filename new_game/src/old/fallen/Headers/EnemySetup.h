//	EnemySetup.h
//	Guy Simmons, 23rd August 1998.

#ifndef FALLEN_HEADERS_ENEMYSETUP_H
#define FALLEN_HEADERS_ENEMYSETUP_H

#include "Mission.h"

//---------------------------------------------------------------

void do_enemy_setup(EventPoint* the_ep, BOOL do_adjust = UC_FALSE);
CBYTE* get_enemy_message(EventPoint* ep, CBYTE* msg);

//---------------------------------------------------------------

#endif // FALLEN_HEADERS_ENEMYSETUP_H
