#pragma once

// DualSense output report builder.
//
// Encapsulates all output state (lightbar, rumble, player LED,
// adaptive trigger effect slots, mute LED, haptic volume, speaker /
// headphone audio volumes) and builds the wire-format HID output
// report for either USB or Bluetooth transport. For BT the trailing
// CRC32 is appended using ds_crc.

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

    // Audio routing: which physical output the controller drives.
    // daidr alternates between these two values in the audioControl
    // nibble of the output report (OutputPanel.vue::speakerVolume and
    // headphoneVolume setters):
    //   Headphone → audioControl high nibble = 0x0 (3.5 mm jack)
    //   Speaker   → audioControl high nibble = 0x3 (built-in speaker)
    // Only applied when `audio_volumes_enabled` is true.
    enum class AudioRoute : std::uint8_t {
        Headphone = 0,
        Speaker   = 1,
    };
    AudioRoute   audio_route           = AudioRoute::Headphone;

    // Audio output volumes. Only applied when `audio_volumes_enabled`
    // is true — otherwise the library does not touch the controller's
    // internal audio state, which stays at whatever the host OS /
    // previous app set it to. Set flag + route + volume to manage
    // audio from the game.
    bool         audio_volumes_enabled = false;
    std::uint8_t speaker_volume        = 0;   // 0..255 (built-in speaker)
    std::uint8_t headphone_volume      = 0;   // 0..255 (3.5 mm jack)

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

struct Device;  // fwd decl from ds_device.h

// High-level output write: builds the wire-format packet for the
// current transport and sends it. Recommended entry point for typical
// consumers — hides the buffer sizing and transport selection of the
// low-level API. Returns bytes written, or -1 on error / disconnect
// (in which case `dev` is auto-closed).
int device_send_output(Device* dev, const OutputState& state);

} // namespace oc::dualsense
