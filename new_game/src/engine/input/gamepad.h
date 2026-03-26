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
InputDeviceType gamepad_get_device_type();
