#pragma once

// Gamepad abstraction layer.
// Supports three input modes: keyboard/mouse, Xbox/generic (SDL3), DualSense (Dualsense-Multiplatform).
// Active mode switches automatically based on last input device.

#include <cstdint>

// uc_orig: DIJOYSTATE (DirectInput) — GamepadState mirrors the fields that game code uses,
// with the same value ranges (lX/lY: 0..65535, center 32768; rgbButtons: 0x80 = pressed).
struct GamepadState {
    int32_t lX;            // left stick X, 0..65535 (center 32768)
    int32_t lY;            // left stick Y, 0..65535 (center 32768)
    int32_t rX;            // right stick X, 0..65535 (center 32768)
    int32_t rY;            // right stick Y, 0..65535 (center 32768)
    uint8_t rgbButtons[32]; // 0x80 = pressed, compatible with DIJOYSTATE
    uint8_t trigger_left;  // L2 analog: 0 (released) .. 255 (fully pressed)
    uint8_t trigger_right; // R2 analog: 0 (released) .. 255 (fully pressed)
    bool connected;
    bool dpad_active;      // true when D-Pad is providing axis values (digital, not analog)
};

enum InputDeviceType {
    INPUT_DEVICE_KEYBOARD_MOUSE,
    INPUT_DEVICE_XBOX,       // Xbox / generic gamepad (via SDL3)
    INPUT_DEVICE_DUALSENSE,  // DualSense (via Dualsense-Multiplatform)
};

void gamepad_init();
void gamepad_shutdown();
void gamepad_poll();
void gamepad_rumble(uint16_t low_freq, uint16_t high_freq, uint32_t duration_ms);

// PS1-style vibration: fast motor (0=off, 1=on) + slow motor (0-255 intensity).
// Uses maximum tracking: only updates a motor if new value > current value.
// Motors decay each tick via gamepad_rumble_tick().
// uc_orig: PSX_SetShock (fallen/psxlib/Source/GDisplay.cpp)
void gamepad_set_shock(int fast, int slow);

// Apply per-tick motor decay and send current values to controller.
// Call once per game tick from the main loop.
void gamepad_rumble_tick();

// Immediately stop all vibration. Call on death, level restart, menu transition.
void gamepad_rumble_stop();

// Reset DualSense LED to default (blue). Call on level end, menu transition.
void gamepad_led_reset();

InputDeviceType gamepad_get_device_type();

// Update DualSense LED lightbar based on game state. No-op for Xbox/keyboard.
// health_fraction: 0.0 (dead) to 1.0 (full health).
// siren: true when driving a police car with siren active (red/blue flash).
// Call once per game tick.
void gamepad_led_update(float health_fraction, bool siren);

// Update DualSense adaptive trigger effects based on gameplay context.
// No-op for Xbox/keyboard. Only sends HID commands when mode changes.
// in_car: player is driving a vehicle (brake pedal feel on L2).
// weapon_ready: weapon trigger click should be active (gun drawn and not
//               currently in a non-firing state — on cooldown, aiming at a
//               surrendered target, etc). Dry-fire (no ammo) should still
//               click, so caller should NOT gate on ammo here.
// current_weapon: SPECIAL_* constant for the weapon currently held by the
//                 player; selects the WeaponFeelProfile that tunes the click.
//                 Pass SPECIAL_NONE if unarmed — the default profile will
//                 opt out of the click entirely.
// Call once per game tick.
void gamepad_triggers_update(bool in_car, bool weapon_ready, int32_t current_weapon);

// Immediately disarm the weapon trigger click on a fire event. Called
// whenever the game registers a real shot so the HID mode switch to NONE
// goes out on the same frame, preventing a stale Weapon25 click from
// firing during the game-side cooldown that follows. The duration of the
// lockout is NOT time-based — it's driven by the game's Timer1 via
// gamepad_triggers_update(weapon_ready) and the R2-position gate inside
// gamepad_triggers_update. The parameter is reserved for future use.
void gamepad_triggers_lockout(int reserved);

// Disable all adaptive trigger effects. Call on pause, menu, death, level transition.
void gamepad_triggers_off();

// Mark a button to be consumed (zeroed) on every poll until released.
// Use when exiting a menu to prevent the button from triggering a gameplay action.
void gamepad_consume_until_released(uint8_t button_index);

// Adaptive trigger feedback status — reported by the controller for the
// currently active effect. State value: 0 = before effect zone, 1 = inside,
// 2 = past. Effect active = controller signals the effect is engaged.
// Only meaningful for effect modes Off (0x05), Feedback (0x21),
// Weapon (0x25), Vibration/MachineGun (0x26). DualSense only — values
// stay 0/false on other devices.
uint8_t gamepad_get_right_trigger_feedback_state();
uint8_t gamepad_get_left_trigger_feedback_state();
bool    gamepad_get_right_trigger_effect_active();
bool    gamepad_get_left_trigger_effect_active();

// TEST HOOK — weapon_feel_test hardware motor delay measurement.
// mode: 0 = force NONE, 1 = force AIM_GUN. Bypasses the normal
// gamepad_triggers_update state machine. Temporary, to be removed along
// with weapon_feel_test.{h,cpp}.
void gamepad_test_set_trigger_mode(int mode);
