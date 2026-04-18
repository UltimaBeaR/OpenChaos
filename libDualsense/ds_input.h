#pragma once

// DualSense HID input report parser.
//
// Parses the normalized 64-byte input report into a POD struct. The
// input pointer must already have the Report ID byte stripped:
//   - USB: pass report starting at byte after the 0x01 Report ID
//   - BT:  pass report starting at byte after the 0x31 Report ID +
//          1 byte of BT framing (net +2 strip)
//
// After stripping, the byte offsets are identical for USB and BT
// which is what this parser assumes.

#include <cstdint>

namespace oc::dualsense {

// Parsed DualSense input state. All values are in raw device units
// (sticks centered at 128, triggers 0..255). Translation to game
// units (-1..+1 sticks, 0..1 triggers) is done by ds_bridge.
struct InputState {
    // Analog
    std::uint8_t left_stick_x;   // 0..255, center 128
    std::uint8_t left_stick_y;   // 0..255, center 128, Y+ down
    std::uint8_t right_stick_x;
    std::uint8_t right_stick_y;
    std::uint8_t left_trigger;   // 0..255
    std::uint8_t right_trigger;  // 0..255

    // Face buttons
    bool cross;
    bool circle;
    bool square;
    bool triangle;

    // Shoulders
    bool l1;
    bool r1;
    bool l2;      // digital threshold from device
    bool r2;

    // D-Pad (decoded from hat byte)
    bool dpad_up;
    bool dpad_down;
    bool dpad_left;
    bool dpad_right;

    // Menu / system
    bool start;       // Options
    bool share;       // Create
    bool ps_button;
    bool touchpad_click;
    bool mute;

    // Sticks click
    bool l3;
    bool r3;

    // Adaptive trigger feedback status (per-trigger state byte).
    //   bit 4        = effect-active flag (motor currently engaged)
    //   bits 0..3    = internal motor state nybble
    // Meaningful for effect modes Feedback(0x21), Weapon(0x25),
    // Vibration(0x26). Off(0x05) reports zero.
    std::uint8_t left_trigger_feedback;   // raw byte from offset 0x2A
    std::uint8_t right_trigger_feedback;  // raw byte from offset 0x29
    bool         left_trigger_effect_active;
    bool         right_trigger_effect_active;

    // Battery. Level is reported by the controller as a 4-bit nibble
    // which maps to 10% bins per daidr `BatteryLevel` enum:
    //   0..9  → 0-9%, 10-19%, ..., 90-99%  (lower bound of the bin)
    //   10    → 100% exactly
    //   11    → UNKNOWN (battery_level_percent keeps its previous
    //                    value, no flicker)
    // Values > 11 are private firmware codes; treated same as UNKNOWN.
    std::uint8_t battery_level_percent;  // 0..100
    bool         battery_charging;       // charging_status == 1
    bool         battery_full;           // charging_status == 2 (charging_complete)

    // Audio peripheral presence (byte 53, status1).
    bool headphone_connected;  // 3.5 mm TRRS jack detected
    bool mic_connected;        // external microphone on the jack

    // Touchpad fingers. Hardware reports up to 2 simultaneous contacts.
    // Coordinates are raw touchpad pixels (X: 0..1919, Y: 0..1079). When
    // finger is not touching, `*_down` is false and X/Y hold the last
    // position the finger had while down (per HID report convention).
    std::uint16_t touchpad_finger_1_x;
    std::uint16_t touchpad_finger_1_y;
    bool          touchpad_finger_1_down;
    std::uint8_t  touchpad_finger_1_id;
    std::uint16_t touchpad_finger_2_x;
    std::uint16_t touchpad_finger_2_y;
    bool          touchpad_finger_2_down;
    std::uint8_t  touchpad_finger_2_id;

    // Motion sensors. All values are raw int16/uint32/int8 device units —
    // apply Sony calibration data (feature report 0x05) to convert to
    // physical units (deg/s for gyro, g for accel). Without calibration
    // values are suitable for relative motion / gesture detection only.
    std::int16_t  gyro_pitch;
    std::int16_t  gyro_yaw;
    std::int16_t  gyro_roll;
    std::int16_t  accel_x;
    std::int16_t  accel_y;
    std::int16_t  accel_z;
    std::uint32_t motion_timestamp;    // device microseconds, wraps
    std::int8_t   motion_temperature;  // internal sensor temp, device units
};

// Parse a normalized 64-byte DualSense input report into `out`.
// `report` must point at a buffer of at least 55 bytes of parsed
// data (Report ID / BT framing already stripped by the caller).
void parse_input_report(const std::uint8_t* report, InputState* out);

// ---- Normalization helpers -----------------------------------------
//
// Convert raw HID bytes into conventional game units. Kept in the
// library so every consumer (bridge, tester, future ports) uses one
// formula — raw/centre/range constants only live in one place.

// Stick axis (0..255, centre 128) → signed float in [-1, +1] with
// centre at 0. Raw 0 produces -128/127 ≈ -1.008 which the caller
// might reject; this helper clamps to [-1, +1] so callers don't have
// to. No Y-axis inversion — that's a game convention, left to the
// caller (e.g. `ds_bridge` flips Y for game-space "up is positive").
float normalize_stick_axis(std::uint8_t raw);

// Analog trigger (0..255) → float in [0, 1].
float normalize_trigger(std::uint8_t raw);

struct Device;  // fwd decl from ds_device.h

// High-level input read: drains the HID queue keeping only the most
// recent report, strips the transport-specific framing bytes, and
// parses the payload into `out`. Recommended entry point for typical
// consumers — hides the buffer sizing, framing offsets, and queue
// drain logic of the low-level API.
//
// Returns:
//    > 0   a new report was parsed; `out` is now fresh
//    0     no new report this frame; `out` is unchanged (use previous
//          state)
//    < 0   device disconnected; `dev` was auto-closed and `out` is
//          unchanged
int device_read_input(Device* dev, InputState* out);

} // namespace oc::dualsense
