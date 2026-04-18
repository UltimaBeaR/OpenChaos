// DualSense output report builder.
//
// Wire-format layout matches the DualSense HID protocol as implemented by
// daidr/dualsense-tester (Web HID reference, works on all OS + transport).
//
// The 47-byte common output payload is identical on USB and Bluetooth —
// only the framing around it differs.
//
//   USB wire (48 bytes total):
//     buf[0]       Report ID 0x02
//     buf[1..47]   47-byte common payload
//
//   Bluetooth wire (78 bytes total):
//     buf[0]       Report ID 0x31
//     buf[1]       (seq << 4), seq is a rotating 0..255 counter
//     buf[2]       magic 0x10
//     buf[3..49]   47-byte common payload
//     buf[50..73]  24 reserved bytes (zero)
//     buf[74..77]  CRC32 over buf[0..73] (prefix byte 0xA2 baked into CRC seed)
//
// Common 47-byte payload layout (p[] indexing):
//     p[0]  validFlag0          bit mask, see below
//     p[1]  validFlag1          bit mask, see below
//     p[2]  rumble_right        high-frequency motor
//     p[3]  rumble_left         low-frequency motor
//     p[4]  headphoneVolume     state.headphone_volume
//     p[5]  speakerVolume       state.speaker_volume
//     p[6]  micVolume           (not yet wired up)
//     p[7]  audioControl        upper nibble = route (0 = headphone,
//                                3 = speaker); lower nibble = flags
//     p[8]  muteLedControl      MuteLed enum value
//     p[9]  powerSaveMuteControl (not yet wired up)
//     p[10] adaptiveTriggerRightMode
//     p[11..20] adaptiveTriggerRightParam0..9  (our trigger_right[10] covers
//                               Mode+Param0..8; Param9 stays zero)
//     p[21] adaptiveTriggerLeftMode
//     p[22..31] adaptiveTriggerLeftParam0..9   (same as above)
//     p[32..35] Reserved0..3
//     p[36] hapticVolume
//     p[37] audioControl2       (not yet wired up)
//     p[38] validFlag2          bit mask, see below
//     p[39..40] Reserved7..8
//     p[41] lightbarSetup
//     p[42] ledBrightness       player LED brightness
//     p[43] playerIndicator     player LED mask
//     p[44..46] ledCRed/Green/Blue  lightbar colour

#include <libDualsense/ds_output.h>

#include <libDualsense/ds_crc.h>
#include <libDualsense/ds_device.h>

#include <cstring>

namespace oc::dualsense {

// Per-bit control flags that tell the controller which payload fields
// this packet is actually updating. Identical names and positions as
// daidr/dualsense-tester (OutputPanel.vue setValidFlagN() calls).
namespace ValidFlag0 {
    constexpr std::uint8_t RumbleRight     = 0x01;  // bit 0
    constexpr std::uint8_t RumbleLeft      = 0x02;  // bit 1
    constexpr std::uint8_t TriggerRight    = 0x04;  // bit 2
    constexpr std::uint8_t TriggerLeft     = 0x08;  // bit 3
    constexpr std::uint8_t HeadphoneVolume = 0x10;  // bit 4
    constexpr std::uint8_t SpeakerVolume   = 0x20;  // bit 5
    constexpr std::uint8_t MicVolume       = 0x40;  // bit 6
    constexpr std::uint8_t AudioControl    = 0x80;  // bit 7
}

namespace ValidFlag1 {
    constexpr std::uint8_t MicMute            = 0x01;  // bit 0
    constexpr std::uint8_t PowerSaveMute      = 0x02;  // bit 1
    constexpr std::uint8_t LightbarRGB        = 0x04;  // bit 2
    constexpr std::uint8_t ReleaseLedsDefault = 0x08;  // bit 3 — MUST stay 0 when setting LED/lightbar
    constexpr std::uint8_t PlayerLedMask      = 0x10;  // bit 4
    constexpr std::uint8_t OverallEffectsRate = 0x40;  // bit 6
}

namespace ValidFlag2 {
    constexpr std::uint8_t PlayerLedBrightness = 0x01;  // bit 0
    constexpr std::uint8_t LightbarSetup       = 0x02;  // bit 1
    constexpr std::uint8_t ImprovedRumble      = 0x04;  // bit 2
}

void build_output_report(const OutputState& state,
                         std::uint8_t* buf,
                         bool bluetooth)
{
    if (!buf) return;

    const std::size_t wire_len = bluetooth ? 78u : 48u;
    std::memset(buf, 0, wire_len);

    std::size_t payload_offset;
    if (bluetooth) {
        buf[0] = 0x31;

        // Rotating sequence counter in high nibble of byte 1 (low nibble
        // reserved = 0). Required by DualSense BT output protocol; the
        // controller deduplicates packets by this value.
        static std::uint8_t s_bt_seq = 0;
        buf[1] = static_cast<std::uint8_t>(s_bt_seq << 4);
        s_bt_seq = static_cast<std::uint8_t>((s_bt_seq + 1) & 0xFF);

        buf[2] = 0x10;  // magic
        payload_offset = 3;
    } else {
        buf[0] = 0x02;
        payload_offset = 1;
    }

    std::uint8_t* p = buf + payload_offset;

    // Activate the subset of validFlag bits for fields we actually write.
    // Bit 3 of validFlag1 ("ReleaseLedsDefault") must remain zero so the
    // controller keeps our custom LED/lightbar state.
    std::uint8_t validFlag0 =
          ValidFlag0::TriggerRight
        | ValidFlag0::TriggerLeft;

    // Rumble flags: only advertise them when there's actual rumble to
    // apply. daidr clears bits 0/1 every time it writes audio volumes
    // (OutputPanel.vue::speakerVolume/headphoneVolume setters call
    // `clearValidFlag0(0); clearValidFlag0(1)`) — empirically this is
    // required on Bluetooth, otherwise setting audioControl routing
    // while the rumble bits are active leaks audio to the speaker even
    // when headphone routing is selected. USB tolerates the conflict,
    // BT does not. We keep rumble bits active whenever amplitudes are
    // non-zero so the rumble-test widget still works alongside audio.
    if (state.rumble_left != 0 || state.rumble_right != 0) {
        validFlag0 |= ValidFlag0::RumbleRight | ValidFlag0::RumbleLeft;
    }

    if (state.audio_volumes_enabled) {
        // Advertise BOTH volume bits + AudioControl. Earlier revisions
        // set only the "active route" bit (mimicking daidr's per-drag
        // transactional behaviour), but that stops the controller from
        // applying the inactive output's volume — so the consumer's
        // explicit 0 to silence the leaking output never lands and the
        // previous level keeps playing. In a streaming-style driver
        // (where we re-send every tick) both bits must stay active so
        // every volume write takes effect.
        validFlag0 |= ValidFlag0::HeadphoneVolume
                    | ValidFlag0::SpeakerVolume
                    | ValidFlag0::AudioControl;
    }

    const std::uint8_t validFlag1 =
          ValidFlag1::MicMute
        | ValidFlag1::LightbarRGB
        | ValidFlag1::PlayerLedMask;

    const std::uint8_t validFlag2 =
          ValidFlag2::PlayerLedBrightness
        | ValidFlag2::LightbarSetup;

    p[0]  = validFlag0;
    p[1]  = validFlag1;
    p[2]  = state.rumble_right;
    p[3]  = state.rumble_left;
    if (state.audio_volumes_enabled) {
        p[4] = state.headphone_volume;
        p[5] = state.speaker_volume;
        // p[6] micVolume — still zero, not wired up (daidr UI doesn't
        // expose it either).
        // p[7] audioControl: high nibble selects route, low nibble is
        // flags. daidr writes the nibble per-drag (0x00 for headphone
        // slider, 0x30 for speaker slider); we expose it as the
        // explicit `audio_route` enum.
        p[7] = (state.audio_route == OutputState::AudioRoute::Speaker)
             ? 0x30
             : 0x00;
    }
    // If audio_volumes_enabled is false, p[4..7] stay zero and the
    // validFlag0 bits for audio fields are not set — controller ignores.
    p[8]  = static_cast<std::uint8_t>(state.mute_led);
    // p[9] powerSaveMuteControl — zero

    // Adaptive trigger effect slots. Right slot = 11 bytes at p[10..20]
    // (Mode + Param0..9), left = 11 bytes at p[21..31]. Our 10-byte
    // trigger descriptors cover Mode + Param0..8; the 11th byte (Param9)
    // stays zero from the memset.
    std::memcpy(&p[10], state.trigger_right, 10);
    std::memcpy(&p[21], state.trigger_left,  10);

    // p[32..35] Reserved0..3 — zero
    p[36] = state.haptic_volume;
    // p[37] audioControl2 — zero
    p[38] = validFlag2;
    // p[39..40] Reserved7..8 — zero
    p[41] = state.lightbar_setup;
    p[42] = state.player_led_brightness;
    p[43] = state.player_led_mask;
    p[44] = state.lightbar_r;
    p[45] = state.lightbar_g;
    p[46] = state.lightbar_b;

    // BT CRC32 over buf[0..73]. Virtual 0xA2 prefix is baked into the
    // CRC seed/table in ds_crc.cpp (see DS5W for the table derivation).
    if (bluetooth) {
        const std::uint32_t crc = crc32_compute(buf, 74);
        buf[74] = static_cast<std::uint8_t>((crc >> 0)  & 0xFF);
        buf[75] = static_cast<std::uint8_t>((crc >> 8)  & 0xFF);
        buf[76] = static_cast<std::uint8_t>((crc >> 16) & 0xFF);
        buf[77] = static_cast<std::uint8_t>((crc >> 24) & 0xFF);
    }
}

int device_send_output(Device* dev, const OutputState& state)
{
    if (!dev || !dev->connected) return -1;

    // Wire buffer large enough for BT (78) with slack for future growth.
    std::uint8_t buf[96] = {};
    const bool bt = (dev->connection == Connection::Bluetooth);
    build_output_report(state, buf, bt);
    return device_write(dev, buf, dev->output_report_size);
}

} // namespace oc::dualsense
