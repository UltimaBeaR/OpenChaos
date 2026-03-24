#ifndef UI_PAUSE_H
#define UI_PAUSE_H

#include "engine/core/types.h"

// Handles the pause menu: reads joystick/keyboard input, draws the menu overlay,
// and processes Resume / Restart / Exit choices.
// Returns TRUE if the game state was changed (restart or exit), FALSE otherwise.
// uc_orig: PAUSE_handler (fallen/Source/pause.cpp)
SLONG PAUSE_handler(void);

#endif // UI_PAUSE_H
