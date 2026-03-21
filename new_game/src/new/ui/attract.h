#ifndef UI_ATTRACT_H
#define UI_ATTRACT_H

#include "core/types.h"
#include "ui/attract_globals.h"

// uc_orig: MAX_PLAYBACKS (fallen/Source/Attract.cpp)
#define MAX_PLAYBACKS 3

// uc_orig: game_attract_mode (fallen/Source/Attract.cpp)
void game_attract_mode(void);

// level_won / level_lost are dead code in the pre-release (#if 0), kept as ABI stubs.
// uc_orig: level_won (fallen/Headers/attract.h)
void level_won(void);

// uc_orig: level_lost (fallen/Headers/attract.h)
void level_lost(void);

// uc_orig: ScoresDraw (fallen/Source/Attract.cpp)
void ScoresDraw(void);

// uc_orig: ATTRACT_loadscreen_init (fallen/Source/Attract.cpp)
void ATTRACT_loadscreen_init(void);

// uc_orig: ATTRACT_loadscreen_draw (fallen/Source/Attract.cpp)
void ATTRACT_loadscreen_draw(SLONG completion); // completion is 8-bit fixed point 0-256

// uc_orig: any_button_pressed (fallen/Source/Attract.cpp)
SLONG any_button_pressed(void);

#endif // UI_ATTRACT_H
