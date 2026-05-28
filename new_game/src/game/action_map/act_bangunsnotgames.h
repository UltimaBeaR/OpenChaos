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
//
// Note: under the F1-modifier scheme (see input_frame.h ::input_debug_modifier_active),
// nearly all debug-mode hotkeys fire only while F1 is HELD as a modifier.
// F1 itself doubles as the help-legend trigger — held alone (5-second
// timer) it shows the overlay; F1+key fires the debug action and the
// overlay auto-hides (debug_help.cpp).

// F1: hold as the global debug modifier; alone (no other key pressed)
// shows the debug-hotkey legend for 5 seconds. Read in
// debug_help.cpp::debug_help_tick.
constexpr int ACT_BANG_SHOW_LEGEND_KKEY = KKEY_F1;

// F2: toggle CRT scanline post-process shader. Read in game.cpp::special_keys
// (under F1-modifier).
constexpr int ACT_BANG_TOGGLE_CRT_KKEY = KKEY_F2;

// F8: toggle single-step physics mode. Pair with KKEY_INSERT (step one tick).
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
constexpr int ACT_BANG_STEP_ONE_TICK_KKEY = KKEY_INSERT;

// LCtrl: rising-edge toggle of the debug-overlay lock (debug_overlay_locked_on).
// This is the LCtrl KEY itself, not a modifier read — ControlFlag is the
// separate held-state mirror used as modifier in other binds. Read in
// game.cpp::game_loop.
constexpr int ACT_BANG_TOGGLE_DEBUG_OVERLAY_LOCK_KKEY = KKEY_LEFT_CONTROL;

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
constexpr int ACT_BANG_CYCLE_CAMERA_PERSON_KKEY = KKEY_RIGHT_BRACKET;

// `;` (semicolon — KKEY_SEMICOLON): toggle slow-motion. Read in thing.cpp.
constexpr int ACT_BANG_TOGGLE_SLOW_MO_KKEY = KKEY_SEMICOLON;

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
constexpr int ACT_BANG_PANEL_CLOSE_KKEY = KKEY_ESCAPE;

// 1 / 2 / 3 — switch to keyboard / gamepad / DualSense page.
constexpr int ACT_BANG_PANEL_PAGE_KEYBOARD_KKEY  = KKEY_1;
constexpr int ACT_BANG_PANEL_PAGE_GAMEPAD_KKEY   = KKEY_2;
constexpr int ACT_BANG_PANEL_PAGE_DUALSENSE_KKEY = KKEY_3;

// TAB: cycle the current page through its sub-views (controller viz → tests
// → back). Only pages that define sub-views react.
constexpr int ACT_BANG_PANEL_CYCLE_SUBVIEW_KKEY = KKEY_TAB;

// Navigation within the panel re-uses menu-nav constants (ACT_MENU_NAV_*_KKEY
// + ACT_MENU_CONFIRM_1_KKEY) — the semantic is the same as in any other menu;
// no separate panel-nav constants needed.

// ---- WARE debug overlay (level-hold T) -------------------------------------
// T held draws the WARE (warehouse / interest-point) debug overlay each frame.
// Gated by input_debug_modifier_active() at the ware.cpp read site —
// F1+T enables the overlay.

constexpr int ACT_BANG_WARE_DEBUG_KKEY = KKEY_T;

// ---- On-screen message buffer scroll (debug-mode-only) ---------------------
// Inside MSG_draw (engine/console/message.cpp). Numpad bindings moved off
// numpad onto the Home/End cluster — the perf-debug compile-flag now owns
// numpad, and the project target audience (Steam Deck etc.) doesn't have
// numpad anyway. Shift modifier multiplies scroll step by 20 (read
// separately via ShiftFlag).

constexpr int ACT_BANG_MSG_SCROLL_UP_KKEY    = KKEY_PAGE_UP;
constexpr int ACT_BANG_MSG_SCROLL_DOWN_KKEY  = KKEY_PAGE_DOWN;
constexpr int ACT_BANG_MSG_SCROLL_RESET_KKEY = KKEY_HOME;

// ---- Misc gameplay-side dev hotkeys (gated by allow_debug_keys) ------------

// D: prints "There is room behind Darci" on the HUD if the behind-check
// passes — gameplay-area room test for AI debugging. game_tick.cpp.
constexpr int ACT_BANG_ROOM_BEHIND_CHECK_KKEY = KKEY_D;

// `[` (left brace): cycle secondary camera through Thing slots in REVERSE
// order. Symmetric to ACT_BANG_CYCLE_CAMERA_PERSON_KKEY (`]`).
constexpr int ACT_BANG_CYCLE_CAMERA_PERSON_REV_KKEY = KKEY_LEFT_BRACKET;

// P: toggle the secondary camera's focus on / off (camera-target null vs
// current cycle slot). game_tick.cpp.
constexpr int ACT_BANG_TOGGLE_CAMERA_FOCUS_KKEY = KKEY_P;

// Combat-testing harness — `-` fewer enemies, `=` more, `\` cycle armament
// tier. update() keeps the wave topped up. game_tick.cpp.
constexpr int ACT_BANG_COMBAT_TEST_INC_KKEY              = KKEY_EQUALS;
constexpr int ACT_BANG_COMBAT_TEST_DEC_KKEY              = KKEY_MINUS;
constexpr int ACT_BANG_COMBAT_TEST_CYCLE_ARMAMENT_KKEY   = KKEY_BACKSLASH;

// F3: save whole-game snapshot. Gated behind bangunsnotgames so regular
// players don't trigger debug-save by accident. game_tick.cpp.
constexpr int ACT_BANG_SAVE_GAME_KKEY = KKEY_F3;

// ---- Misc gameplay dev hotkeys (game_tick.cpp, gated by allow_debug_keys) --
// Each ACT name describes a single semantic action. Several KKEYs (D, G, I, J,
// O, P, Q) are reused across multiple actions distinguished by ShiftFlag /
// ControlFlag / held-vs-edge — each variant gets its own ACT name per
// rules.md. Same physical scancode appears in multiple constants on purpose.

// F4: toggle cloud drawing in aeng.
constexpr int ACT_BANG_TOGGLE_CLOUDS_KKEY = KKEY_F4;

// `~` (tilde): toggle DETAIL_LEVEL between 0 and 0xffff.
constexpr int ACT_BANG_TOGGLE_DETAIL_LEVEL_KKEY = KKEY_GRAVE;

// P (no Shift): quick-save game to "save.me". Distinct from the dev console
// save (F3) and from ACT_BANG_TOGGLE_CAMERA_FOCUS_KKEY (also P) — all three
// fire on the same scancode when allow_debug_keys is on.
constexpr int ACT_BANG_QUICK_SAVE_GAME_KKEY = KKEY_P;

// J (no Shift, held): draw MAV navigation overlay around the player.
constexpr int ACT_BANG_DRAW_MAV_OVERLAY_KKEY = KKEY_J;

// I (no Shift, held): draw WAND debug overlay around the player.
constexpr int ACT_BANG_DRAW_WAND_OVERLAY_KKEY = KKEY_I;

// E: spawn a vehicle ahead of the player, cycling through vehicle types.
constexpr int ACT_BANG_SPAWN_VEHICLE_KKEY = KKEY_E;

// W (held): continuously spawn water particles around the player.
constexpr int ACT_BANG_SPAWN_WATER_KKEY = KKEY_W;

// End: cycle the pyro-test "which_pyro" selector (was Numpad 7).
constexpr int ACT_BANG_PYRO_CYCLE_TYPE_KKEY = KKEY_END;

// K: spawn the currently-selected pyro effect at the player (was Numpad 5).
// Moved off Insert because Insert is the single-step companion key (paired
// with F8). K is otherwise unused in act_bangunsnotgames — picked as a
// free letter near the home row.
constexpr int ACT_BANG_PYRO_SPAWN_KKEY = KKEY_K;

// L: toggle a directional debug light following the player.
constexpr int ACT_BANG_TOGGLE_DIRECTIONAL_LIGHT_KKEY = KKEY_L;

// Delete: immolate the test chopper (was Numpad 2).
constexpr int ACT_BANG_IMMOLATE_CHOPPER_KKEY = KKEY_DELETE;

// R: spawn a firepool line (2-press sequence: anchor / target). Was Numpad 3.
// Picked as a free letter (R = fiRe).
constexpr int ACT_BANG_SPAWN_FIREPOOL_KKEY = KKEY_R;

// `/` (forward slash): toggle stealth_debug overlay.
constexpr int ACT_BANG_TOGGLE_STEALTH_DEBUG_KKEY = KKEY_SLASH;

// `.` (period, KKEY_PERIOD) held: continuously spawn smoke particles above
// the player.
constexpr int ACT_BANG_SPAWN_SMOKE_KKEY = KKEY_PERIOD;

// O (no Shift): create an OB (object) at the player's position.
constexpr int ACT_BANG_CREATE_OB_KKEY = KKEY_O;

// Shift + A: spawn a fight-test thug near the player. Gated by ShiftFlag at
// the call site.
constexpr int ACT_BANG_SPAWN_FIGHT_THUG_KKEY = KKEY_A;

// Shift + I: spawn a bodyguard following the player. Distinct from
// ACT_BANG_DRAW_WAND_OVERLAY_KKEY (also I) by ShiftFlag at the call site.
constexpr int ACT_BANG_SPAWN_BODYGUARD_KKEY = KKEY_I;

// Shift + J: make Darci perform a random dance animation.
constexpr int ACT_BANG_DARCI_DANCE_KKEY = KKEY_J;

// Y (Shift block, held): draw fastnav collision-debug overlay.
constexpr int ACT_BANG_DRAW_FASTNAV_KKEY = KKEY_Y;

// D inside Shift block, also reads as `!ShiftFlag` — dead code in current
// source. Create a NIGHT_slight (debug light). Listed for completeness.
constexpr int ACT_BANG_CREATE_NIGHT_SLIGHT_KKEY = KKEY_D;

// Shift + D: ASSERT(false) to break into the debugger. Distinct from
// ACT_BANG_ROOM_BEHIND_CHECK_KKEY (also D) by ShiftFlag.
constexpr int ACT_BANG_DEBUGGER_BREAK_KKEY = KKEY_D;

// Shift + G: give Darci a gun (FLAGS_HAS_GUN). Distinct from
// ACT_BANG_TELEPORT_TO_MOUSE_KKEY (also G) by ShiftFlag.
constexpr int ACT_BANG_GIVE_GUN_KKEY = KKEY_G;

// Shift + H: dump a top-down plan-view shot of the level as TGA.
constexpr int ACT_BANG_PLAN_VIEW_SHOT_KKEY = KKEY_H;

// Shift + O: spawn a civilian chopper above the player. Distinct from
// ACT_BANG_CREATE_OB_KKEY (also O) by ShiftFlag.
constexpr int ACT_BANG_SPAWN_CHOPPER_KKEY = KKEY_O;

// Shift + M: spawn a mine at the mouse cursor world position.
constexpr int ACT_BANG_SPAWN_MINE_AT_MOUSE_KKEY = KKEY_M;

// F12 (no Shift): spawn a ring of weapons + health pickup around the player.
// Distinct from ACT_BANG_TOGGLE_CHEAT_OVERLAY_KKEY (also F12) — that requires
// ShiftFlag, this one fires when ShiftFlag is off.
constexpr int ACT_BANG_SPAWN_ALL_WEAPONS_KKEY = KKEY_F12;

// G (no Shift): teleport Darci to the mouse-cursor world position.
constexpr int ACT_BANG_TELEPORT_TO_MOUSE_KKEY = KKEY_G;

// U (held): repeatedly hit any nearby barrels with a sphere impact.
constexpr int ACT_BANG_HIT_BARREL_KKEY = KKEY_U;

// Q (held, no Shift, no Ctrl): draw road navigation debug. Distinct from
// ACT_BANG_QUIT_GAME_KKEY (Q + Ctrl) and ACT_BANG_SPEED_BOOST_KKEY (Q + Shift).
constexpr int ACT_BANG_DRAW_ROAD_DEBUG_KKEY = KKEY_Q;

#endif // GAME_ACTION_MAP_ACT_BANGUNSNOTGAMES_H
