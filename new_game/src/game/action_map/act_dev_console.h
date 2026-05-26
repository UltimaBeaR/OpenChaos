#ifndef GAME_ACTION_MAP_ACT_DEV_CONSOLE_H
#define GAME_ACTION_MAP_ACT_DEV_CONSOLE_H

// =============================================================================
// ACT_CONS_* — action constants for the dev console once it is open: ENTER
// submits the typed command, BACKSPACE deletes the last char, ESC closes the
// console, letter keys type characters. This is text-input mode — gamepad is
// not involved.
//
// The hotkey that OPENS the console (F9) is not here — it belongs to the
// gameplay context (ACT_FOOT_* / ACT_CAR_*) because the console is opened from
// gameplay, same as ESC opens the pause menu.
//
// Naming rules, prefix table, suffix table → see
//   new_game_devlog/input_system/action_map/rules.md
// Step plan (this header is populated in step 3c.2) → see
//   new_game_devlog/input_system/action_map/plan.md
//
// Device codes used as values (KKEY_*) live in
//   new_game/src/game/action_map/input_codes.h
// =============================================================================

#include "game/action_map/input_codes.h"

// (Constants will be filled here in step 3c.2 — dev console.)

#endif // GAME_ACTION_MAP_ACT_DEV_CONSOLE_H
