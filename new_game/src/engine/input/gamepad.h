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

// Mark a button to be consumed (zeroed) on every poll until released.
// Use when exiting a menu to prevent the button from triggering a gameplay action.
void gamepad_consume_until_released(uint8_t button_index);
