#ifndef GAME_ACTION_MAP_ACT_MODIFIERS_H
#define GAME_ACTION_MAP_ACT_MODIFIERS_H

// =============================================================================
// ACT_MOD_* — modifier-key bindings used to maintain the level-state flags
// `ShiftFlag`, `AltFlag`, `ControlFlag`. These flags are read as side channels
// at gameplay / menu / debug call sites; they're not actions in themselves.
// The sync code in keyboard.cpp::SetFlagsFromKeyArray() reads the modifier
// scancodes via `input_key_event_held()` to produce the live flag values.
//
// Naming rules → see new_game_devlog/input_system/action_map/rules.md
//
// Device codes (KKEY_*) live in
//   new_game/src/game/action_map/input_codes.h
// =============================================================================

#include "game/action_map/input_codes.h"

// Alt (left + right). Drives `AltFlag`.
constexpr int ACT_MOD_ALT_L_KKEY = KKEY_LEFT_ALT;
constexpr int ACT_MOD_ALT_R_KKEY = KKEY_RIGHT_ALT;

// Control (left only — right Ctrl is intentionally not read here in the
// current build; the debug-overlay-lock toggle uses KKEY_LEFT_CONTROL separately
// via ACT_BANG_TOGGLE_DEBUG_OVERLAY_LOCK_KKEY). Drives `ControlFlag`.
constexpr int ACT_MOD_CTRL_L_KKEY = KKEY_LEFT_CONTROL;

// Shift (left + right). Drives `ShiftFlag`.
constexpr int ACT_MOD_SHIFT_L_KKEY = KKEY_LEFT_SHIFT;
constexpr int ACT_MOD_SHIFT_R_KKEY = KKEY_RIGHT_SHIFT;

#endif // GAME_ACTION_MAP_ACT_MODIFIERS_H
