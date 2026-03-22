//
// frontend.h
//
// matthew rosenfeld 8 july 99
//
// this is our new front end thingy to replace the hideous startscr.cpp
//

#ifndef FALLEN_HEADERS_FRONTEND_H
#define FALLEN_HEADERS_FRONTEND_H

#include "MFStdLib.h"

void FRONTEND_init(bool bGoToTitleScreen = UC_FALSE);
SBYTE FRONTEND_loop();
void FRONTEND_level_won();
void FRONTEND_level_lost();

extern UBYTE IsEnglish;

#endif // FALLEN_HEADERS_FRONTEND_H
