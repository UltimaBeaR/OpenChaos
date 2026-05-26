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

// Submit the typed command and close the console. Read in
// game_tick.cpp::process_controls (is_inputing branch).
constexpr int ACT_CONS_SUBMIT_KKEY = KKEY_ENTER;

// Cancel — close the console without running the typed command. Read in
// game_tick.cpp::process_controls (is_inputing branch).
constexpr int ACT_CONS_CANCEL_KKEY = KKEY_ESC;

// Text input (letter keys, digits, space, backspace) is read in the same
// branch via input_last_key() — a wildcard read of "any scancode" translated
// through InkeyToAscii[]. No ACT_* constant for this wildcard; the
// input_last_key() / input_last_key_consume() API stays as-is.
//
// Backspace handling lives inside that wildcard path (InkeyToAscii[KKEY_BS]
// returns 8); not bound to a dedicated constant.

#endif // GAME_ACTION_MAP_ACT_DEV_CONSOLE_H
