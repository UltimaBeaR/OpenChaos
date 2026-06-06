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
// Steering reads the virtual movement-intent X axis (the same packed word that
// foot movement uses) via input_virtual_axis(input, ACT_CAR_STEER_VAXIS) in
// apply_button_input_car — see ACT_CAR_STEER_VAXIS below. The discrete
// INPUT_MASK_LEFT / RIGHT bits gate it (no deflection → centred). Accelerate /
// brake / reverse are read DIRECTLY from their KKEY / GBTN constants below
// (input_key_held / input_btn_held in apply_button_input_car) — see the
// per-control comment further down.
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

// ---- Accelerate / brake / reverse ------------------------------------------
// Three independent in-car controls (OpenChaos scheme — the original coupled
// brake and reverse onto one button):
//   Gas     — keyboard W     / gamepad R2 (RT)
//   Brake   — keyboard Space / gamepad L1 (LB)   — hard-brake, never reverses
//   Reverse — keyboard S     / gamepad L2 (LT)   — drive backwards
//
// Both gamepad (R2/L1/L2) and keyboard (W/Space/S) are read DIRECTLY via
// input_btn_held / input_key_held in apply_button_input_car using the GBTN /
// KKEY constants below. (Keyboard can't go through the WASD analog-stick bits:
// W and S are opposite ends of the same axis and would cancel when both held.)
//
// Priority (resolved in vehicle.cpp::pedals): brake > reverse > gas.

constexpr int ACT_CAR_ACCEL_GBTN = GBTN_R2_DIGITAL; // DS: R2, Xbox: RT (digital bit)
constexpr int ACT_CAR_BRAKE_GBTN = GBTN_L1; // DS: L1, Xbox: LB (digital bumper)
constexpr int ACT_CAR_REVERSE_GBTN = GBTN_L2_DIGITAL; // DS: L2, Xbox: LT (digital bit)

// Analog trigger axis — drives proportional acceleration (gas) scaling in
// vehicle.cpp::pedals. Read via input_trigger_raw(...) (0..255). Brake (L1
// bumper) has no analog axis; reverse drives at a fixed rate (no analog).
constexpr int ACT_CAR_ACCEL_GTRIG = GTRIG_R2;

// Read directly in apply_button_input_car (input_key_held) — W and S are
// opposite ends of the WASD analog axis, so they can't go through the stick
// bits (they'd cancel); they're read as individual keys.
constexpr int ACT_CAR_ACCEL_KKEY = KKEY_W; // gas
constexpr int ACT_CAR_BRAKE_KKEY = KKEY_SPACE; // brake
constexpr int ACT_CAR_REVERSE_KKEY = KKEY_S; // reverse

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

// ---- Steering (virtual movement-intent X axis) -----------------------------
// Car steering reads the horizontal component of the unified movement-intent
// vector (same packed word as on-foot movement) via
// input_virtual_axis(input, ACT_CAR_STEER_VAXIS) in steering_wheel /
// apply_button_input_car. Device-agnostic: fed by the gamepad left stick or
// keyboard A/D. See VAXIS_* in input_codes.h.

constexpr int ACT_CAR_STEER_VAXIS = VAXIS_X;

// ---- Open dev console (gameplay-level hotkey) ------------------------------
// F9 opens the dev console from gameplay. Same scancode as
// ACT_FOOT_OPEN_DEV_CONSOLE_KKEY — separate constant per rules.md (different
// contexts). The actual call site in game_tick.cpp reads both constants.

constexpr int ACT_CAR_OPEN_DEV_CONSOLE_KKEY = KKEY_F9;

#endif // GAME_ACTION_MAP_ACT_CAR_H
