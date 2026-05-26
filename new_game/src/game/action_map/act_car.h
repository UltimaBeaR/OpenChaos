#ifndef GAME_ACTION_MAP_ACT_CAR_H
#define GAME_ACTION_MAP_ACT_CAR_H

// =============================================================================
// ACT_CAR_* — action constants for gameplay in a vehicle: accelerate, brake,
// steering, siren, plus the F9 dev-console hotkey (alongside the foot one in
// act_foot.h — same scancode, two contexts).
//
// ESC → open pause menu lives in act_menu.h (ACT_MENU_TOGGLE_PAUSE_*) — same
// key in both foot and car, owned by menu context.
//
// Steering (analog) and steering keys are NOT bound here — they go through
// INPUT_MASK_LEFT / RIGHT and analog stick reads in apply_button_input_car,
// which are abstraction-level signals already covered by act_foot.h movement
// constants. The keyboard mapping for accelerate / brake / go-faster is also
// indirected via INPUT_CAR_KB_* input masks (composed of INPUT_MASK_PUNCH /
// _KICK / _JUMP from on-foot keyboard reads). The constants below are
// SEMANTIC TAGS for those KKEY mappings — they're not read directly anywhere,
// they document which on-foot key produces each in-car action.
//
// Naming rules, prefix table, suffix table → see
//   new_game_devlog/input_system/action_map/rules.md
// Step plan (this header is populated in step 3c.6) → see
//   new_game_devlog/input_system/action_map/plan.md
//
// Device codes used as values (KKEY_*, GBTN_*, ...) live in
//   new_game/src/game/action_map/input_codes.h
// =============================================================================

#include "game/action_map/input_codes.h"

// ---- Accelerate / brake ----------------------------------------------------
// Gamepad: R2 (digital trigger bit) = accel, L2 (digital) = brake. Read via
// input_btn_held in apply_button_input_car. Same physical scancode as
// ACT_FOOT_PUNCH_GTRIG / ACT_FOOT_TACTICAL_MODE_GTRIG (digital threshold vs
// analog) but distinct semantic — two constants.
//
// Keyboard: accel = Z (via INPUT_MASK_PUNCH inside INPUT_CAR_KB_ACCELERATE),
// brake = X (via INPUT_MASK_KICK inside INPUT_CAR_KB_DECELERATE). Reads happen
// at the foot level (KKEY_Z / KKEY_X — see ACT_FOOT_PUNCH_KKEY / KICK_KKEY)
// then go through the input-mask layer — these constants are semantic tags.

constexpr int ACT_CAR_ACCEL_GBTN = GBTN_R2_BTN; // DS: R2, Xbox: RT (digital bit)
constexpr int ACT_CAR_BRAKE_GBTN = GBTN_L2_BTN; // DS: L2, Xbox: LT (digital bit)

constexpr int ACT_CAR_ACCEL_KKEY = KKEY_Z; // semantic tag — see header comment
constexpr int ACT_CAR_BRAKE_KKEY = KKEY_X; // semantic tag — see header comment

// "Go faster" modifier (accel + brake combined). Composed in
// INPUT_CAR_KB_GOFASTER / INPUT_CAR_PAD_GOFASTER input masks; no direct
// constant read at the call site. Semantic tag only.
//
// Note: SPACE = both car-siren (direct read) AND "go faster" (via
// INPUT_MASK_JUMP inside INPUT_CAR_KB_GOFASTER). Two separate ACT names for
// the two semantics on the same scancode.

constexpr int ACT_CAR_GO_FASTER_KKEY = KKEY_SPACE; // semantic tag — see comment

// ---- Siren toggle ----------------------------------------------------------
// Toggles siren / lights in apply_button_input_car. Edge-triggered via
// input_frame's sticky press_pending (level signals would re-fire every
// physics tick while held). Keyboard SPACE direct read; gamepad Triangle (Y)
// direct read. Triangle is also the menu-cancel button — when not driving,
// process_hardware_level_input_for_player drains the press_pending so a
// menu-context Triangle press doesn't leak into the first car-entry tick.

constexpr int ACT_CAR_SIREN_KKEY = KKEY_SPACE;
constexpr int ACT_CAR_SIREN_GBTN = GBTN_NORTH; // DS: Triangle, Xbox: Y

// ---- Open dev console (gameplay-level hotkey) ------------------------------
// F9 opens the dev console from gameplay. Same scancode as
// ACT_FOOT_OPEN_DEV_CONSOLE_KKEY — separate constant per rules.md (different
// contexts). The actual call site in game_tick.cpp reads both constants.

constexpr int ACT_CAR_OPEN_DEV_CONSOLE_KKEY = KKEY_F9;

#endif // GAME_ACTION_MAP_ACT_CAR_H
