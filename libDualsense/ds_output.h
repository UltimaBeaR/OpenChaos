#pragma once

// DualSense output report builder.
//
// Encapsulates all output state (lightbar, rumble, player LED,
// adaptive trigger effect slots, mute LED, haptic volume) and builds
// the wire-format HID output report for either USB or Bluetooth
// transport. For BT the trailing CRC32 is appended using ds_crc.

#include <cstddef>
#include <cstdint>

namespace oc::dualsense {

// Player LED mask values (DualSense has 5 LEDs below the touchpad).
// Values are standard Sony bitmasks: see nondebug/dualsense RE.
namespace PlayerLed {
    constexpr std::uint8_t Off     = 0x00;
    constexpr std::uint8_t Center  = 0x04;
    constexpr std::uint8_t Inner   = 0x02 | 0x08;
    constexpr std::uint8_t Outer   = 0x01 | 0x10;
    constexpr std::uint8_t All     = 0x1F;
}

// Mic mute LED state.
//   Off    — LED dark, mic capturing audio.
//   On     — LED steady, mic muted by controller.
//   Blink  — LED blinking, used by "all muted" (system-wide mute).
enum class MuteLed : std::uint8_t {
    Off   = 0,
    On    = 1,
    Blink = 2,
};

// Full output state. Each field is set by the game via setters in
// ds_bridge; the whole struct is then serialized into a HID packet
// by `build_output_report` and sent to the device.
struct OutputState {
    // Lightbar
    std::uint8_t lightbar_r = 0;
    std::uint8_t lightbar_g = 0;
    std::uint8_t lightbar_b = 0;

    // Rumble (DS4-compatible two-motor)
    std::uint8_t rumble_left  = 0;  // low-frequency motor
    std::uint8_t rumble_right = 0;  // high-frequency motor

    // Player LED
    std::uint8_t player_led_mask       = 0;
    std::uint8_t player_led_brightness = 0;

    // Mic mute LED state (below the PS button).
    MuteLed mute_led = MuteLed::Off;

    // Overall haptic (rumble) volume, 0..7. 0 = quietest, 7 = loudest.
    // Applied by the controller on top of per-motor rumble_left/right.
    std::uint8_t haptic_volume = 0;

    // Lightbar setup/override byte. Bit 1 ("fade-in at connect") is a
    // common use; 0 = no override (default).
    std::uint8_t lightbar_setup = 0;

    // Adaptive trigger effect slots — 10 bytes each.
    // Filled via ds_trigger functions (trigger_weapon, trigger_off,
    // trigger_feedback, etc.). Default = all zero = Off.
    std::uint8_t trigger_right[10] = {};
    std::uint8_t trigger_left[10]  = {};
};

// Build an output report into `buf` (size determined by transport).
// `buf` must be at least 48 bytes for USB or 78 bytes for BT.
// `bluetooth` selects between the USB (0x02) and BT (0x31) layouts;
// BT additionally gets a CRC32 appended at offset 0x4A..0x4D.
//
// After this returns, `buf` is ready to hand to device_write().
void build_output_report(const OutputState& state,
                         std::uint8_t* buf,
                         bool bluetooth);

} // namespace oc::dualsense
