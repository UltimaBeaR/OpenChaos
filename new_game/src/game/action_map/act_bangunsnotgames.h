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

#endif // GAME_ACTION_MAP_ACT_BANGUNSNOTGAMES_H
