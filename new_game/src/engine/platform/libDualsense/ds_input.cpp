// DualSense HID input report parser.

#include "engine/platform/libDualsense/ds_input.h"

namespace oc::dualsense {

// --- Button byte bit positions (after Report ID / header strip) ---
// Byte 0x07: low nibble = d-pad hat, high nibble = square/cross/circle/triangle
// Byte 0x08: L1/R1/L2/R2/Share/Options/L3/R3
// Byte 0x09: PS/TouchpadClick/Mute/Fn1/Fn2/Paddle1/Paddle2
namespace mask {
    // Byte 0x07 high nibble
    constexpr std::uint8_t Square   = 0x10;
    constexpr std::uint8_t Cross    = 0x20;
    constexpr std::uint8_t Circle   = 0x40;
    constexpr std::uint8_t Triangle = 0x80;

    // Byte 0x08
    constexpr std::uint8_t L1       = 0x01;
    constexpr std::uint8_t R1       = 0x02;
    constexpr std::uint8_t L2       = 0x04;
    constexpr std::uint8_t R2       = 0x08;
    constexpr std::uint8_t Share    = 0x10;
    constexpr std::uint8_t Options  = 0x20;
    constexpr std::uint8_t L3       = 0x40;
    constexpr std::uint8_t R3       = 0x80;

    // Byte 0x09
    constexpr std::uint8_t PS       = 0x01;
    constexpr std::uint8_t Touchpad = 0x02;
    constexpr std::uint8_t Mute     = 0x04;
}

void parse_input_report(const std::uint8_t* r, InputState* out)
{
    // Sticks (raw unsigned bytes; center ~128)
    out->left_stick_x  = r[0x00];
    out->left_stick_y  = r[0x01];
    out->right_stick_x = r[0x02];
    out->right_stick_y = r[0x03];

    // Analog triggers
    out->left_trigger  = r[0x04];
    out->right_trigger = r[0x05];

    // D-Pad hat (low nibble of byte 0x07)
    const std::uint8_t hat = r[0x07] & 0x0F;
    out->dpad_up    = false;
    out->dpad_down  = false;
    out->dpad_left  = false;
    out->dpad_right = false;
    switch (hat) {
        case 0: out->dpad_up = true; break;
        case 1: out->dpad_up = true; out->dpad_right = true; break;
        case 2: out->dpad_right = true; break;
        case 3: out->dpad_right = true; out->dpad_down = true; break;
        case 4: out->dpad_down = true; break;
        case 5: out->dpad_down = true; out->dpad_left = true; break;
        case 6: out->dpad_left = true; break;
        case 7: out->dpad_left = true; out->dpad_up = true; break;
        default: break;  // 8 = centered
    }

    // Face buttons (byte 0x07 high nibble)
    const std::uint8_t face = r[0x07];
    out->square   = (face & mask::Square)   != 0;
    out->cross    = (face & mask::Cross)    != 0;
    out->circle   = (face & mask::Circle)   != 0;
    out->triangle = (face & mask::Triangle) != 0;

    // Shoulders + menu + stick clicks (byte 0x08)
    const std::uint8_t bA = r[0x08];
    out->l1    = (bA & mask::L1)      != 0;
    out->r1    = (bA & mask::R1)      != 0;
    out->l2    = (bA & mask::L2)      != 0;
    out->r2    = (bA & mask::R2)      != 0;
    out->share = (bA & mask::Share)   != 0;
    out->start = (bA & mask::Options) != 0;
    out->l3    = (bA & mask::L3)      != 0;
    out->r3    = (bA & mask::R3)      != 0;

    // PS / touchpad click / mute (byte 0x09)
    const std::uint8_t bB = r[0x09];
    out->ps_button      = (bB & mask::PS)       != 0;
    out->touchpad_click = (bB & mask::Touchpad) != 0;
    out->mute           = (bB & mask::Mute)     != 0;

    // Adaptive trigger feedback status.
    // Offsets 0x29 (R2) and 0x2A (L2).
    // bit 4 = effect-active, low nibble = state nybble.
    const std::uint8_t r2_fb = r[0x29];
    const std::uint8_t l2_fb = r[0x2A];
    out->right_trigger_feedback       = r2_fb;
    out->left_trigger_feedback        = l2_fb;
    out->right_trigger_effect_active  = (r2_fb & 0x10) != 0;
    out->left_trigger_effect_active   = (l2_fb & 0x10) != 0;

    // Battery (byte 0x34 low nibble = level 0..8; byte 0x35 bit 3 =
    // charging; byte 0x36 bit 5 = fully charged)
    const std::uint8_t batt_lvl = r[0x34] & 0x0F;
    out->battery_level_percent = static_cast<std::uint8_t>((batt_lvl * 100u) / 8u);
    out->battery_charging      = (r[0x35] & 0x08) != 0;
    out->battery_full          = (r[0x36] & 0x20) != 0;

    // Headphone connected (byte 0x35 bit 0)
    out->headphone_connected = (r[0x35] & 0x01) != 0;
}

} // namespace oc::dualsense
