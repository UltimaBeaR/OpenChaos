#ifndef GAME_ACTION_MAP_ACT_MENU_H
#define GAME_ACTION_MAP_ACT_MENU_H

// =============================================================================
// ACT_MENU_* — action constants for the UI menu context: frontend menu,
// pause menu, won / lost menus, attract mode wake-up keys, frontend debug
// cheats. Navigation, confirm/cancel, page-jump, pause toggle.
//
// Naming rules, prefix table, suffix table → see
//   new_game_devlog/input_system/action_map/rules.md
// Step plan (this header is populated in step 3c.4) → see
//   new_game_devlog/input_system/action_map/plan.md
//
// Device codes used as values (KKEY_*, GBTN_*, ...) live in
//   new_game/src/game/action_map/input_codes.h
//
// Note on sticks: gamepad stick discrete directions for menu navigation are
// read via the two-arg API `input_stick_held(GAXIS_LEFT, GDIR_UP)` —
// the GAXIS_* / GDIR_DIR_* values come from input_codes.h. Used directly at
// call sites inside menu-nav OR-chains alongside the ACT_MENU_NAV_*_GBTN /
// ACT_MENU_NAV_*_KKEY constants below.
// =============================================================================

#include "game/action_map/input_codes.h"

// ---- Navigation: up / down / left / right ----------------------------------
// Combined-source nav: keyboard arrow + D-pad button + left-stick direction.
// All three are OR'd together at the call site with a single auto-repeat
// throttle (see InputAutoRepeat::tick_combined). Each direction has both a
// KKEY and a GBTN constant so the binding for each source is explicit.

constexpr int ACT_MENU_NAV_UP_KKEY    = KKEY_UP;
constexpr int ACT_MENU_NAV_DOWN_KKEY  = KKEY_DOWN;
constexpr int ACT_MENU_NAV_LEFT_KKEY  = KKEY_LEFT;
constexpr int ACT_MENU_NAV_RIGHT_KKEY = KKEY_RIGHT;

constexpr int ACT_MENU_NAV_UP_GBTN    = GBTN_DPAD_UP;
constexpr int ACT_MENU_NAV_DOWN_GBTN  = GBTN_DPAD_DOWN;
constexpr int ACT_MENU_NAV_LEFT_GBTN  = GBTN_DPAD_LEFT;
constexpr int ACT_MENU_NAV_RIGHT_GBTN = GBTN_DPAD_RIGHT;

// ---- Confirm / submit ------------------------------------------------------
// Confirm-current-item in a menu / form. Multiple keyboard bindings — Enter,
// numpad Enter, and Space — all behave as confirm. Gamepad: Cross / A.

constexpr int ACT_MENU_CONFIRM_KKEY_1 = KKEY_ENTER;
constexpr int ACT_MENU_CONFIRM_KKEY_2 = KKEY_SPACE;
constexpr int ACT_MENU_CONFIRM_KKEY_3 = KKEY_NUMPAD_ENTER;
constexpr int ACT_MENU_CONFIRM_GBTN   = GBTN_SOUTH; // DS: Cross, Xbox: A

// ---- Cancel / back ---------------------------------------------------------
// Cancel-current-screen / back-out. Gamepad: Circle / B (modern PlayStation
// convention since 2020 — Cross confirms, Circle cancels). ESC on keyboard
// doubles as "open pause" from gameplay (see ACT_MENU_TOGGLE_PAUSE_*) — same
// key, different context.

constexpr int ACT_MENU_CANCEL_KKEY = KKEY_ESCAPE;
constexpr int ACT_MENU_CANCEL_GBTN = GBTN_EAST; // DS: Circle, Xbox: B

// ---- Pause toggle (from gameplay) ------------------------------------------
// Open or close the pause menu. Same key as menu cancel (ESC) — pause-toggle
// channel reads press_pending separately and consumes via input_key_force_release
// to avoid the press leaking into gameplay (JUMP / SELECT) on the same frame.
// GBTN_START is bound directly so the gamepad Start button opens / closes
// the pause menu; it's read in parallel with KKEY_ESCAPE at the call site.

constexpr int ACT_MENU_TOGGLE_PAUSE_KKEY = KKEY_ESCAPE;
constexpr int ACT_MENU_TOGGLE_PAUSE_GBTN = GBTN_START; // DS: Options, Xbox: Start

// ---- Page jump (frontend list nav) -----------------------------------------
// Jump to first / last item in a long list (mission select, etc.). KKEY-only
// — no gamepad equivalent in current layout.

constexpr int ACT_MENU_PAGE_FIRST_KKEY = KKEY_HOME;
constexpr int ACT_MENU_PAGE_LAST_KKEY  = KKEY_END;

// ---- Form widget: focus cycle ----------------------------------------------
// Tab inside a widget Form cycles focus between widget children (Shift+Tab
// reverses). Read inside widget.cpp::FORM_Process via FORM_KeyProc.

constexpr int ACT_MENU_FORM_CYCLE_FOCUS_KKEY = KKEY_TAB;

// ---- Attract-mode wake / quit ----------------------------------------------
// Any of these keys held wakes the attract screen up so the menu stays alive
// while the user is interacting. Re-uses the nav + confirm bindings (no new
// constants for the level-hold check — call site OR's existing constants).
//
// Ctrl + Q (modifier checked separately via ControlFlag) quits from the
// attract / main-menu state.

constexpr int ACT_MENU_ATTRACT_QUIT_KKEY = KKEY_Q;

// ---- Frontend cheat hotkeys (Ctrl+Shift + key) -----------------------------
// Two cheat shortcuts in the frontend: Numpad-Plus advances complete_point by
// one, Numpad-Asterisk maxes it out. ControlFlag + ShiftFlag are modifier
// side-channels at the call site. NOT gated by allow_debug_keys — these are
// frontend-only cheats with their own Ctrl+Shift guard.

// Moved off numpad (target hardware doesn't have one). Ctrl+Shift modifier
// guard at the call site already makes the combo unique enough that picking
// regular-row keys is safe — gameplay keys 1-8 are weapon hotkeys but in
// frontend (where these cheats live) those bindings are inert.
constexpr int ACT_MENU_FE_CHEAT_ADVANCE_POINT_KKEY = KKEY_EQUALS; // Ctrl+Shift+=
constexpr int ACT_MENU_FE_CHEAT_MAX_POINT_KKEY     = KKEY_8;      // Ctrl+Shift+8 (asterisk via Shift)

// ---- Modal dialog acknowledge ----------------------------------------------
// "Press anything sensible to continue" modal screens during gameplay (e.g.
// the dead-civilians warning in game.cpp). Same bindings as menu confirm /
// cancel — distinct ACT names so the modal context can be retargeted
// independently if needed. Read in game.cpp::game_loop deadcivs branch.

constexpr int ACT_MENU_MODAL_ACK_KKEY_1 = KKEY_ESCAPE;
constexpr int ACT_MENU_MODAL_ACK_KKEY_2 = KKEY_SPACE;
constexpr int ACT_MENU_MODAL_ACK_KKEY_3 = KKEY_ENTER;
constexpr int ACT_MENU_MODAL_ACK_KKEY_4 = KKEY_NUMPAD_ENTER;

// ---- "Any button" confirm probe (used in attract / "press start") ----------
// Frontend attract mode and similar "press start" screens want a single
// "any face button" trigger. Implemented as a Cross / A press at the call
// site (other gamepad buttons reachable through normal nav / cancel paths).
// Constant alias: same value as ACT_MENU_CONFIRM_GBTN; defined separately for
// search-by-semantic.

constexpr int ACT_MENU_ANY_BUTTON_GBTN = GBTN_SOUTH; // DS: Cross, Xbox: A

#endif // GAME_ACTION_MAP_ACT_MENU_H
