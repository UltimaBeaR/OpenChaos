#ifndef GAME_ACTION_MAP_ACT_BANGUNSNOTGAMES_H
#define GAME_ACTION_MAP_ACT_BANGUNSNOTGAMES_H

// =============================================================================
// ACT_BANG_* — action constants for debug-key hotkeys active ONLY after the
// player types "bangunsnotgames" into the dev console (allow_debug_keys flag).
// Includes F-keys, Ctrl+X / Shift+X combos, and a couple of single-letter
// hotkeys for developer toggles. All gameplay-debug-mode bindings.
//
// Not "general debugging" — this is a specific MODE that the player explicitly
// enables. Constants here are KKEY-only (no gamepad bindings for these).
//
// Modifier flags (ControlFlag, ShiftFlag) are level-state side channels read
// separately at each call site — they are NOT folded into the action constant.
//
// Compile-time debug paths (`#if OC_DEBUG_PHYSICS_TIMING`) and playback-mode
// keys (`if (GS_PLAYBACK)`) are NOT here — they have their own gates, not
// allow_debug_keys, and live elsewhere.
//
// Naming rules, prefix table, suffix table → see
//   new_game_devlog/input_system/action_map/rules.md
// Step plan (this header is populated in step 3c.3) → see
//   new_game_devlog/input_system/action_map/plan.md
//
// Device codes used as values (KKEY_*) live in
//   new_game/src/game/action_map/input_codes.h
// =============================================================================

#include "game/action_map/input_codes.h"

// ---- F-key toggles ----------------------------------------------------------

// F1: show debug-hotkey legend overlay (5-second timer). Read in
// debug_help.cpp::debug_help_tick.
constexpr int ACT_BANG_SHOW_LEGEND_KKEY = KKEY_F1;

// F2: toggle CRT scanline post-process shader. Read in game.cpp::special_keys.
constexpr int ACT_BANG_TOGGLE_CRT_KKEY = KKEY_F2;

// F8: toggle single-step physics mode. Pair with KKEY_INS (step one tick).
// Read in game.cpp::special_keys.
constexpr int ACT_BANG_TOGGLE_SINGLE_STEP_KKEY = KKEY_F8;

// F10: toggle far-facet debug mode (skip normal level geometry pass + render
// far-facets with debug colours). Read in game.cpp::special_keys.
constexpr int ACT_BANG_TOGGLE_FARFACET_DEBUG_KKEY = KKEY_F10;

// F11: toggle modal input debug panel. Read in game.cpp::special_keys.
constexpr int ACT_BANG_TOGGLE_INPUT_DEBUG_PANEL_KKEY = KKEY_F11;

// Shift + F12: toggle cheat=2 mode (FPS / position overlay). ShiftFlag checked
// alongside this binding at the call site. Read in game_tick.cpp::process_controls.
constexpr int ACT_BANG_TOGGLE_CHEAT_OVERLAY_KKEY = KKEY_F12;

// ---- Step / pause ----------------------------------------------------------

// Insert: step one physics tick while single-step mode is active. Read in
// game.cpp::special_keys (gated by `single_step` flag).
constexpr int ACT_BANG_STEP_ONE_TICK_KKEY = KKEY_INS;

// LCtrl: rising-edge toggle of the debug-overlay lock (debug_overlay_locked_on).
// This is the LCtrl KEY itself, not a modifier read — ControlFlag is the
// separate held-state mirror used as modifier in other binds. Read in
// game.cpp::game_loop.
constexpr int ACT_BANG_TOGGLE_DEBUG_OVERLAY_LOCK_KKEY = KKEY_LCONTROL;

// ---- Modifier + key combos -------------------------------------------------
// Ctrl/Shift state checked separately via ControlFlag / ShiftFlag at the call
// site; constant below is the non-modifier key only.

// Ctrl + Q: quit game. Read in game.cpp::special_keys.
constexpr int ACT_BANG_QUIT_GAME_KKEY = KKEY_Q;

// Shift + Q (held): speed-boost player movement by 4x for testing locomotion.
// Read in person.cpp.
constexpr int ACT_BANG_SPEED_BOOST_KKEY = KKEY_Q;

// Ctrl + L: toggle "outside the city" camera mode. Read in
// aeng.cpp::AENG_draw_city.
constexpr int ACT_BANG_TOGGLE_OUTSIDE_CAM_KKEY = KKEY_L;

// ---- Screenshot / recording ------------------------------------------------

// S: dump a TGA screenshot. Shift + S also toggles continuous record mode.
// Read in aeng.cpp::AENG_screen_shot.
constexpr int ACT_BANG_SCREENSHOT_KKEY = KKEY_S;

// ---- Per-frame dev cycles --------------------------------------------------

// N: cycle the player model through PersonType / variant table. Read in
// game_tick.cpp::process_controls.
constexpr int ACT_BANG_CYCLE_PLAYER_MODEL_KKEY = KKEY_N;

// B: toggle per-bone debug skeleton overlay (lines + wireframe spheres on
// joints). Read in game_tick.cpp::process_controls.
constexpr int ACT_BANG_TOGGLE_SKELETON_OVERLAY_KKEY = KKEY_B;

// `]` (right brace): cycle the secondary camera through Thing slots, looking
// at the next non-civilian PERSON. Read in game_tick.cpp::process_controls.
constexpr int ACT_BANG_CYCLE_CAMERA_PERSON_KKEY = KKEY_RBRACE;

// `;` (semicolon — KKEY_COLON): toggle slow-motion. Read in thing.cpp.
constexpr int ACT_BANG_TOGGLE_SLOW_MO_KKEY = KKEY_COLON;

// V (held): show version-number overlay on the HUD. Read in panel.cpp.
constexpr int ACT_BANG_SHOW_VERSION_KKEY = KKEY_V;

// ---- Frontend menu theme switch --------------------------------------------
// In the frontend menu, debug mode unlocks 1..4 to switch the seasonal theme
// (leaves / rain / snow / blood). Read in frontend.cpp::FRONTEND_input.

constexpr int ACT_BANG_MENU_THEME_1_KKEY = KKEY_1;
constexpr int ACT_BANG_MENU_THEME_2_KKEY = KKEY_2;
constexpr int ACT_BANG_MENU_THEME_3_KKEY = KKEY_3;
constexpr int ACT_BANG_MENU_THEME_4_KKEY = KKEY_4;

// ---- Input debug panel (opened by F11) -------------------------------------
// Once the panel is open it owns keyboard input until ESC closes it. Read in
// input_debug.cpp::input_debug_tick.

// ESC: close the panel. Same scancode as ACT_MENU_CANCEL_KKEY but different
// semantic (panel-close vs. menu-back).
constexpr int ACT_BANG_PANEL_CLOSE_KKEY = KKEY_ESC;

// 1 / 2 / 3 — switch to keyboard / gamepad / DualSense page.
constexpr int ACT_BANG_PANEL_PAGE_KEYBOARD_KKEY  = KKEY_1;
constexpr int ACT_BANG_PANEL_PAGE_GAMEPAD_KKEY   = KKEY_2;
constexpr int ACT_BANG_PANEL_PAGE_DUALSENSE_KKEY = KKEY_3;

// TAB: cycle the current page through its sub-views (controller viz → tests
// → back). Only pages that define sub-views react.
constexpr int ACT_BANG_PANEL_CYCLE_SUBVIEW_KKEY = KKEY_TAB;

// Navigation within the panel re-uses menu-nav constants (ACT_MENU_NAV_*_KKEY
// + ACT_MENU_CONFIRM_KKEY_1) — the semantic is the same as in any other menu;
// no separate panel-nav constants needed.

// ---- WARE debug overlay (level-hold T) -------------------------------------
// T held draws the WARE (warehouse / interest-point) debug overlay each frame.
// NOTE: this read in ware.cpp is NOT gated by allow_debug_keys (it ships in
// every build). Effectively a developer-only feature anyway — T is unlikely
// to be pressed by a regular player, and the overlay is harmless if it is.
// Listed here for action-map completeness; if review wants the gate, add
// `if (allow_debug_keys)` to ware.cpp::WARE_debug.

constexpr int ACT_BANG_WARE_DEBUG_KKEY = KKEY_T;

// ---- On-screen message buffer scroll (debug-mode-only) ---------------------
// Inside MSG_draw (engine/console/message.cpp), gated by allow_debug_keys.
// Numpad +/-/Enter scroll the debug message history; Shift modifier multiplies
// scroll step by 20 (read separately via ShiftFlag).

constexpr int ACT_BANG_MSG_SCROLL_UP_KKEY    = KKEY_PPLUS;
constexpr int ACT_BANG_MSG_SCROLL_DOWN_KKEY  = KKEY_PMINUS;
constexpr int ACT_BANG_MSG_SCROLL_RESET_KKEY = KKEY_PENTER;

// ---- Misc gameplay-side dev hotkeys (gated by allow_debug_keys) ------------

// D: prints "There is room behind Darci" on the HUD if the behind-check
// passes — gameplay-area room test for AI debugging. game_tick.cpp.
constexpr int ACT_BANG_ROOM_BEHIND_CHECK_KKEY = KKEY_D;

// `[` (left brace): cycle secondary camera through Thing slots in REVERSE
// order. Symmetric to ACT_BANG_CYCLE_CAMERA_PERSON_KKEY (`]`).
constexpr int ACT_BANG_CYCLE_CAMERA_PERSON_REV_KKEY = KKEY_LBRACE;

// P: toggle the secondary camera's focus on / off (camera-target null vs
// current cycle slot). game_tick.cpp.
constexpr int ACT_BANG_TOGGLE_CAMERA_FOCUS_KKEY = KKEY_P;

// Combat-testing harness — `-` fewer enemies, `=` more, `\` cycle armament
// tier. update() keeps the wave topped up. game_tick.cpp.
constexpr int ACT_BANG_COMBAT_TEST_INC_KKEY              = KKEY_PLUS;
constexpr int ACT_BANG_COMBAT_TEST_DEC_KKEY              = KKEY_MINUS;
constexpr int ACT_BANG_COMBAT_TEST_CYCLE_ARMAMENT_KKEY   = KKEY_BACKSLASH;

// F3: save whole-game snapshot. Gated behind bangunsnotgames so regular
// players don't trigger debug-save by accident. game_tick.cpp.
constexpr int ACT_BANG_SAVE_GAME_KKEY = KKEY_F3;

#endif // GAME_ACTION_MAP_ACT_BANGUNSNOTGAMES_H
