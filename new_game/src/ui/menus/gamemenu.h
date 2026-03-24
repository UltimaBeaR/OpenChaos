#ifndef UI_MENUS_GAMEMENU_H
#define UI_MENUS_GAMEMENU_H

#include "engine/core/types.h"

// In-game pause/won/lost menus (not the frontend).
// Menu types: NONE, PAUSE, WON, LOST, SURE (abandon confirmation).
// GAMEMENU_process() drives state transitions and input; returns one of the DO_* codes.
// GAMEMENU_draw() renders the dimmed overlay and menu text each frame when active.

// Return codes from GAMEMENU_process().
// uc_orig: GAMEMENU_DO_NOTHING (fallen/Headers/gamemenu.h)
#define GAMEMENU_DO_NOTHING 0
// uc_orig: GAMEMENU_DO_RESTART (fallen/Headers/gamemenu.h)
#define GAMEMENU_DO_RESTART 1
// uc_orig: GAMEMENU_DO_CHOOSE_NEW_MISSION (fallen/Headers/gamemenu.h)
#define GAMEMENU_DO_CHOOSE_NEW_MISSION 2
// uc_orig: GAMEMENU_DO_NEXT_LEVEL (fallen/Headers/gamemenu.h)
#define GAMEMENU_DO_NEXT_LEVEL 3

// uc_orig: GAMEMENU_init (fallen/Headers/gamemenu.h)
void GAMEMENU_init(void);

// uc_orig: GAMEMENU_process (fallen/Headers/gamemenu.h)
SLONG GAMEMENU_process(void);

// uc_orig: GAMEMENU_is_paused (fallen/Headers/gamemenu.h)
SLONG GAMEMENU_is_paused(void);

// Returns an 8-bit fixed-point slowdown multiplier. 0 = fully paused, 256 = normal speed.
// uc_orig: GAMEMENU_slowdown_mul (fallen/Headers/gamemenu.h)
SLONG GAMEMENU_slowdown_mul(void);

// uc_orig: GAMEMENU_set_level_lost_reason (fallen/Headers/gamemenu.h)
void GAMEMENU_set_level_lost_reason(CBYTE* reason);

// uc_orig: GAMEMENU_draw (fallen/Headers/gamemenu.h)
void GAMEMENU_draw(void);

#endif // UI_MENUS_GAMEMENU_H
