#ifndef UI_FRONTEND_ATTRACT_H
#define UI_FRONTEND_ATTRACT_H

#include "engine/core/types.h"
#include "ui/frontend/attract_globals.h"

// uc_orig: MAX_PLAYBACKS (fallen/Source/Attract.cpp)
#define MAX_PLAYBACKS 3

// uc_orig: game_attract_mode (fallen/Source/Attract.cpp)
void game_attract_mode(void);

// uc_orig: ScoresDraw (fallen/Source/Attract.cpp)
void ScoresDraw(void);

// uc_orig: ATTRACT_loadscreen_init (fallen/Source/Attract.cpp)
void ATTRACT_loadscreen_init(void);

// uc_orig: ATTRACT_loadscreen_draw (fallen/Source/Attract.cpp)
void ATTRACT_loadscreen_draw(SLONG completion); // completion is 8-bit fixed point 0-256

#endif // UI_FRONTEND_ATTRACT_H
