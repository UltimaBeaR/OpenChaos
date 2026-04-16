// DualSense output report builder.
// Byte layout cross-checked against rafaelvaloto/Dualsense-Multiplatform
// GamepadOutput.cpp::OutputDualSense, which is empirically verified to
// work for lightbar and rumble on all platforms.

#include "engine/platform/libDualsense/ds_output.h"

#include "engine/platform/libDualsense/ds_crc.h"

#include <cstring>

namespace oc::dualsense {

// Feature-control flag defaults. These are the values rafaelvaloto
// initializes on device open and that work in practice:
//   0x57 = 0b01010111 — enables rumble + trigger + lightbar control
//   0xFF                — full vibration-mode bitmask
static constexpr std::uint8_t FEATURE_MODE_DEFAULT    = 0x57;
static constexpr std::uint8_t VIBRATION_MODE_DEFAULT  = 0xFF;

void build_output_report(const OutputState& state,
                         std::uint8_t* buf,
                         bool bluetooth)
{
    if (!buf) return;

    // Wire-report size and payload offset.
    const std::size_t wire_len = bluetooth ? 78u : 74u;
    std::memset(buf, 0, wire_len);

    std::size_t padding;
    if (bluetooth) {
        buf[0]   = 0x31;   // BT output Report ID
        buf[1]   = 0x02;   // sub-ID
        padding  = 2;

        // Alternating sequence counter at absolute offset 40 (= BT
        // payload[38], `AllowLightBrightnessChange` bit). rafaelvaloto
        // implements this via `buf[40] ^= 0x01` on a persistent
        // output buffer. Since we memset the local buffer every
        // frame, maintain our own toggling state here.
        static std::uint8_t s_bt_seq = 0;
        s_bt_seq ^= 0x01;
        buf[40] = s_bt_seq;
    } else {
        buf[0]  = 0x02;    // USB output Report ID
        padding = 1;

        // Per rafaelvaloto: fixed 0x07 at absolute offset 40 for USB
        // (= USB payload[39], `HapticLowPassFilter` bit + UNKBIT).
        buf[40] = 0x07;
    }

    // `payload` is the DualSense output payload (common layout after
    // transport-specific header is stripped).
    std::uint8_t* payload = buf + padding;

    // --- Feature control flags + rumble ---
    payload[0] = VIBRATION_MODE_DEFAULT;  // enable vibration subsystem
    payload[1] = FEATURE_MODE_DEFAULT;    // enable lightbar/trigger/LED
    payload[2] = state.rumble_right;
    payload[3] = state.rumble_left;

    // --- Adaptive trigger effect slots ---
    // Right trigger = offset 10..19, Left = 21..30.
    std::memcpy(&payload[10], state.trigger_right, 10);
    std::memcpy(&payload[21], state.trigger_left,  10);

    // --- Player LED + lightbar ---
    payload[42] = state.player_led_brightness;
    payload[43] = state.player_led_mask;
    payload[44] = state.lightbar_r;
    payload[45] = state.lightbar_g;
    payload[46] = state.lightbar_b;

    // --- BT CRC32 ---
    if (bluetooth) {
        // CRC is computed over bytes [0..73] and stored at [74..77].
        // The DualSense BT transport prepends a virtual 0xA2 byte to
        // the buffer before CRC — but looking at rafaelvaloto's
        // GamepadOutput.cpp, the working implementation computes CRC
        // directly over the first 74 bytes of the buffer (which
        // already starts with 0x31). Preserved to match the
        // empirically-verified reference.
        const std::uint32_t crc = crc32_compute(buf, 74);
        buf[0x4A] = static_cast<std::uint8_t>((crc >> 0)  & 0xFF);
        buf[0x4B] = static_cast<std::uint8_t>((crc >> 8)  & 0xFF);
        buf[0x4C] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
        buf[0x4D] = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
    }
}

} // namespace oc::dualsense
