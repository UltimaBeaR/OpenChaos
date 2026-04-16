#pragma once

// DualSense adaptive trigger effect factories.
// These builders write a 10-byte trigger effect descriptor into the
// output report slot for either L2 or R2.
//
// Slot layout (10 bytes starting at `out`):
//   out[0]     = effect mode code (see TriggerEffect enum)
//   out[1..9]  = mode-specific parameters
//
// All functions return false (and leave `out` zeroed) if parameters
// are out of range. On invalid / zero-strength input they fall back
// to Off mode, matching duaLib semantics.

#include <cstdint>

namespace oc::dualsense {

enum class TriggerEffect : std::uint8_t {
    Off              = 0x05,  // canonical "no effect" mode
    Feedback         = 0x21,  // continuous resistance from position
    Weapon           = 0x25,  // click at a position, fixed strength
    Vibration        = 0x26,  // periodic vibration above position
    Bow              = 0x22,  // build-up resistance, snap release
    Galloping        = 0x23,  // two-beat pattern
    Machine          = 0x27,  // two amplitudes + frequency + period
    Simple_Feedback  = 0x01,  // legacy raw mode code
    Simple_Weapon    = 0x02,
    Simple_Vibration = 0x06,
    Limited_Feedback = 0x11,
    Limited_Weapon   = 0x12,
};

// Clear the 10-byte slot (mode=Off, rest zero).
bool trigger_off(std::uint8_t out[10]);

// Continuous resistance starting at `position` (0..9), `strength` 0..8.
// strength == 0 → falls back to Off.
bool trigger_feedback(std::uint8_t out[10], std::uint8_t position, std::uint8_t strength);

// Single click between `start` (2..7) and `end` (start+1..8), strength 0..8.
bool trigger_weapon(std::uint8_t out[10],
                    std::uint8_t start, std::uint8_t end, std::uint8_t strength);

// Sustained vibration from `position` (0..9) onwards, amplitude 0..8, frequency in Hz.
bool trigger_vibration(std::uint8_t out[10],
                       std::uint8_t position, std::uint8_t amplitude, std::uint8_t frequency);

// Bow-draw resistance between `start` and `end`, strength 0..8, snap 0..8.
bool trigger_bow(std::uint8_t out[10],
                 std::uint8_t start, std::uint8_t end,
                 std::uint8_t strength, std::uint8_t snap);

// Machine (two-beat) effect: positions, two amplitudes 0..7, frequency, period.
bool trigger_machine(std::uint8_t out[10],
                     std::uint8_t start, std::uint8_t end,
                     std::uint8_t amplitude_a, std::uint8_t amplitude_b,
                     std::uint8_t frequency, std::uint8_t period);

} // namespace oc::dualsense
