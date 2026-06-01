#ifndef GAME_ACTION_MAP_ACT_MODIFIERS_H
#define GAME_ACTION_MAP_ACT_MODIFIERS_H

// =============================================================================
// ACT_MOD_* — modifier-key bindings. These are GENERAL-PURPOSE modifiers: each
// one is held alongside other input to alter behaviour, and is used with that
// generic "modifier" role in many different places — so they are named by the
// modifier itself, not by any single action (per the action-map naming rule:
// a universal modifier is named for its role, not for one specific effect).
//
// Mechanism: keyboard.cpp::update_modifier_flags() reads these scancodes via
// input_key_event_held() into the level-state flags ControlFlag / ShiftFlag,
// which call sites then test as side channels (`if (ShiftFlag) ...`). The flag
// is the abstraction; the binding (which physical key feeds it) lives here.
// There is no per-call-site ACT constant — the flag read takes no argument, so
// adding one would be a dead constant. What each modifier DOES is enumerated
// in its comment below so this file alone tells you why the key matters.
//
// Naming rules → see new_game_devlog/input_system/action_map/rules.md
//
// Device codes (KKEY_*) live in
//   new_game/src/game/action_map/input_codes.h
// =============================================================================

#include "game/action_map/input_codes.h"

// Control (left only — right Ctrl is intentionally not read; the
// debug-overlay-lock toggle uses Left Ctrl separately). Drives `ControlFlag`.
// Roles (all behind allow_debug_keys unless noted): toggles on-screen debug
// draws across many subsystems (collision, springs, AI nav, facets, barrels,
// platforms, vehicles, lighting) — Ctrl held = show overlay; Ctrl+Q quits from
// attract / main menu; Ctrl+Shift + key = frontend point cheats; also forced on
// by debug_overlay_locked_on.
constexpr int ACT_MOD_CTRL_L_KKEY = KKEY_LEFT_CONTROL;

// Shift (left + right). Drives `ShiftFlag`. Roles: selects the upper-case
// translation in text-field widgets (InkeyToAsciiShift); reverses Form focus
// cycle (Shift+Tab); selects the alternate variant of several bangunsnotgames
// debug hotkeys; and is the Ctrl+Shift cheat co-modifier in the frontend.
constexpr int ACT_MOD_SHIFT_L_KKEY = KKEY_LEFT_SHIFT;
constexpr int ACT_MOD_SHIFT_R_KKEY = KKEY_RIGHT_SHIFT;

#endif // GAME_ACTION_MAP_ACT_MODIFIERS_H
