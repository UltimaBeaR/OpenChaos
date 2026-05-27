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
// Keyboard (new layout): accel = W (via INPUT_MASK_FORWARDS+MOVE inside
// INPUT_CAR_KB_ACCELERATE), brake = Space (via INPUT_MASK_JUMP inside
// INPUT_CAR_KB_DECELERATE). The on-foot semantics of W and Space are
// movement-forward and jump respectively — the same bits are reinterpreted
// in the in-car branch of apply_button_input_car. These KKEY constants
// below are semantic tags; the actual reads happen at the foot level
// (ACT_FOOT_MOVE_FORWARD_KKEY / ACT_FOOT_JUMP_KKEY).

constexpr int ACT_CAR_ACCEL_GBTN = GBTN_R2_DIGITAL; // DS: R2, Xbox: RT (digital bit)
constexpr int ACT_CAR_BRAKE_GBTN = GBTN_L2_DIGITAL; // DS: L2, Xbox: LT (digital bit)

// Analog versions of the trigger axes — drives proportional accel / brake
// scaling in vehicle.cpp::do_car_input. Read via input_trigger_raw(...) for
// byte resolution (0..255).
constexpr int ACT_CAR_ACCEL_GTRIG = GTRIG_R2;
constexpr int ACT_CAR_BRAKE_GTRIG = GTRIG_L2;

constexpr int ACT_CAR_ACCEL_KKEY = KKEY_W;     // semantic tag — see header comment
constexpr int ACT_CAR_BRAKE_KKEY = KKEY_SPACE; // semantic tag — see header comment

// "Go faster" modifier — kept as a semantic tag for historical reference.
// VEH_FASTER is now always-on in apply_button_input_car (no input gating),
// so this constant is unused at the call site. See input_actions.cpp.
constexpr int ACT_CAR_GO_FASTER_KKEY = KKEY_W; // semantic tag — see comment

// ---- Siren toggle ----------------------------------------------------------
// Toggles siren / lights in apply_button_input_car. Edge-triggered via
// input_frame's sticky press_pending (level signals would re-fire every
// physics tick while held). Keyboard E direct read; gamepad Triangle (Y)
// direct read. Triangle is no longer the menu-cancel button (moved to
// Circle) but the drain logic in process_hardware_level_input_for_player
// still drains its press_pending while not driving so an on-foot Triangle
// press doesn't leak into the first car-entry tick.

constexpr int ACT_CAR_SIREN_KKEY = KKEY_E;
constexpr int ACT_CAR_SIREN_GBTN = GBTN_NORTH; // DS: Triangle, Xbox: Y

// ---- Open dev console (gameplay-level hotkey) ------------------------------
// F9 opens the dev console from gameplay. Same scancode as
// ACT_FOOT_OPEN_DEV_CONSOLE_KKEY — separate constant per rules.md (different
// contexts). The actual call site in game_tick.cpp reads both constants.

constexpr int ACT_CAR_OPEN_DEV_CONSOLE_KKEY = KKEY_F9;

#endif // GAME_ACTION_MAP_ACT_CAR_H
