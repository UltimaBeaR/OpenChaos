// DualSense adaptive trigger effect factories.
// Byte packings adapted from third-party sources — see THIRD_PARTY_LICENSES.md.

#include "engine/platform/libDualsense/ds_trigger.h"

#include <cstring>

namespace oc::dualsense {

namespace {
inline void zero_slot(std::uint8_t out[10]) { std::memset(out, 0, 10); }
}

bool trigger_off(std::uint8_t out[10])
{
    zero_slot(out);
    out[0] = static_cast<std::uint8_t>(TriggerEffect::Off);
    return true;
}

bool trigger_feedback(std::uint8_t out[10], std::uint8_t position, std::uint8_t strength)
{
    if (position > 9 || strength > 8) { zero_slot(out); return false; }
    if (strength == 0) return trigger_off(out);

    const std::uint8_t force_value = static_cast<std::uint8_t>((strength - 1) & 0x07);
    std::uint32_t force_zones  = 0;
    std::uint16_t active_zones = 0;
    for (int i = position; i < 10; ++i) {
        force_zones  |= static_cast<std::uint32_t>(force_value << (3 * i));
        active_zones |= static_cast<std::uint16_t>(1u << i);
    }

    out[0] = static_cast<std::uint8_t>(TriggerEffect::Feedback);
    out[1] = static_cast<std::uint8_t>((active_zones >> 0) & 0xff);
    out[2] = static_cast<std::uint8_t>((active_zones >> 8) & 0xff);
    out[3] = static_cast<std::uint8_t>((force_zones  >> 0) & 0xff);
    out[4] = static_cast<std::uint8_t>((force_zones  >> 8) & 0xff);
    out[5] = static_cast<std::uint8_t>((force_zones  >> 16) & 0xff);
    out[6] = static_cast<std::uint8_t>((force_zones  >> 24) & 0xff);
    out[7] = 0;
    out[8] = 0;
    out[9] = 0;
    return true;
}

bool trigger_simple_feedback(std::uint8_t out[10],
                             std::uint8_t position_raw,
                             std::uint8_t strength_raw)
{
    zero_slot(out);
    out[0] = static_cast<std::uint8_t>(TriggerEffect::Simple_Feedback);
    out[1] = position_raw;
    out[2] = strength_raw;
    return true;
}

bool trigger_weapon(std::uint8_t out[10],
                    std::uint8_t start, std::uint8_t end, std::uint8_t strength)
{
    if (start < 2 || start > 7 || end > 8 || end <= start || strength > 8) {
        zero_slot(out); return false;
    }
    if (strength == 0) return trigger_off(out);

    const std::uint16_t start_and_stop =
        static_cast<std::uint16_t>((1u << start) | (1u << end));

    out[0] = static_cast<std::uint8_t>(TriggerEffect::Weapon);
    out[1] = static_cast<std::uint8_t>((start_and_stop >> 0) & 0xff);
    out[2] = static_cast<std::uint8_t>((start_and_stop >> 8) & 0xff);
    out[3] = static_cast<std::uint8_t>(strength - 1);
    out[4] = 0;
    out[5] = 0;
    out[6] = 0;
    out[7] = 0;
    out[8] = 0;
    out[9] = 0;
    return true;
}

bool trigger_vibration(std::uint8_t out[10],
                       std::uint8_t position, std::uint8_t amplitude, std::uint8_t frequency)
{
    if (position > 9 || amplitude > 8) { zero_slot(out); return false; }
    if (amplitude == 0 || frequency == 0) return trigger_off(out);

    const std::uint8_t strength_value = static_cast<std::uint8_t>((amplitude - 1) & 0x07);
    std::uint32_t amplitude_zones = 0;
    std::uint16_t active_zones    = 0;
    for (int i = position; i < 10; ++i) {
        amplitude_zones |= static_cast<std::uint32_t>(strength_value << (3 * i));
        active_zones    |= static_cast<std::uint16_t>(1u << i);
    }

    out[0] = static_cast<std::uint8_t>(TriggerEffect::Vibration);
    out[1] = static_cast<std::uint8_t>((active_zones    >> 0)  & 0xff);
    out[2] = static_cast<std::uint8_t>((active_zones    >> 8)  & 0xff);
    out[3] = static_cast<std::uint8_t>((amplitude_zones >> 0)  & 0xff);
    out[4] = static_cast<std::uint8_t>((amplitude_zones >> 8)  & 0xff);
    out[5] = static_cast<std::uint8_t>((amplitude_zones >> 16) & 0xff);
    out[6] = static_cast<std::uint8_t>((amplitude_zones >> 24) & 0xff);
    out[7] = 0;
    out[8] = 0;
    out[9] = frequency;
    return true;
}

bool trigger_bow(std::uint8_t out[10],
                 std::uint8_t start, std::uint8_t end,
                 std::uint8_t strength, std::uint8_t snap)
{
    if (start > 8 || end > 8 || start >= end || strength > 8 || snap > 8) {
        zero_slot(out); return false;
    }
    if (end == 0 || strength == 0 || snap == 0) return trigger_off(out);

    const std::uint16_t start_and_stop =
        static_cast<std::uint16_t>((1u << start) | (1u << end));
    const std::uint32_t force_pair = static_cast<std::uint32_t>(
        (((strength - 1) & 0x07) << 0) |
        (((snap     - 1) & 0x07) << 3));

    out[0] = static_cast<std::uint8_t>(TriggerEffect::Bow);
    out[1] = static_cast<std::uint8_t>((start_and_stop >> 0) & 0xff);
    out[2] = static_cast<std::uint8_t>((start_and_stop >> 8) & 0xff);
    out[3] = static_cast<std::uint8_t>((force_pair    >> 0) & 0xff);
    out[4] = static_cast<std::uint8_t>((force_pair    >> 8) & 0xff);
    out[5] = 0;
    out[6] = 0;
    out[7] = 0;
    out[8] = 0;
    out[9] = 0;
    return true;
}

bool trigger_machine(std::uint8_t out[10],
                     std::uint8_t start, std::uint8_t end,
                     std::uint8_t amplitude_a, std::uint8_t amplitude_b,
                     std::uint8_t frequency, std::uint8_t period)
{
    if (start > 8 || end > 9 || end <= start || amplitude_a > 7 || amplitude_b > 7) {
        zero_slot(out); return false;
    }
    if (frequency == 0) return trigger_off(out);

    const std::uint16_t start_and_stop =
        static_cast<std::uint16_t>((1u << start) | (1u << end));
    const std::uint32_t strength_pair = static_cast<std::uint32_t>(
        ((amplitude_a & 0x07) << 0) |
        ((amplitude_b & 0x07) << 3));

    out[0] = static_cast<std::uint8_t>(TriggerEffect::Machine);
    out[1] = static_cast<std::uint8_t>((start_and_stop >> 0) & 0xff);
    out[2] = static_cast<std::uint8_t>((start_and_stop >> 8) & 0xff);
    out[3] = static_cast<std::uint8_t>((strength_pair >> 0) & 0xff);
    out[4] = frequency;
    out[5] = period;
    out[6] = 0;
    out[7] = 0;
    out[8] = 0;
    out[9] = 0;
    return true;
}

bool trigger_galloping(std::uint8_t out[10],
                       std::uint8_t start, std::uint8_t end,
                       std::uint8_t first_foot, std::uint8_t second_foot,
                       std::uint8_t frequency)
{
    if (start > 8 || end > 9 || start >= end ||
        second_foot > 7 || first_foot > 6 || first_foot >= second_foot) {
        zero_slot(out); return false;
    }
    if (frequency == 0) return trigger_off(out);

    const std::uint16_t start_and_stop =
        static_cast<std::uint16_t>((1u << start) | (1u << end));
    const std::uint32_t time_and_ratio = static_cast<std::uint32_t>(
        ((second_foot & 0x07) << 0) |
        ((first_foot  & 0x07) << 3));

    out[0] = static_cast<std::uint8_t>(TriggerEffect::Galloping);
    out[1] = static_cast<std::uint8_t>((start_and_stop >> 0) & 0xff);
    out[2] = static_cast<std::uint8_t>((start_and_stop >> 8) & 0xff);
    out[3] = static_cast<std::uint8_t>((time_and_ratio >> 0) & 0xff);
    out[4] = frequency;
    out[5] = 0;
    out[6] = 0;
    out[7] = 0;
    out[8] = 0;
    out[9] = 0;
    return true;
}

bool trigger_simple_weapon(std::uint8_t out[10],
                           std::uint8_t start_raw, std::uint8_t end_raw,
                           std::uint8_t strength_raw)
{
    zero_slot(out);
    out[0] = static_cast<std::uint8_t>(TriggerEffect::Simple_Weapon);
    out[1] = start_raw;
    out[2] = end_raw;
    out[3] = strength_raw;
    return true;
}

bool trigger_simple_vibration(std::uint8_t out[10],
                              std::uint8_t position, std::uint8_t amplitude,
                              std::uint8_t frequency)
{
    if (amplitude == 0 || frequency == 0) return trigger_off(out);

    zero_slot(out);
    out[0] = static_cast<std::uint8_t>(TriggerEffect::Simple_Vibration);
    out[1] = frequency;
    out[2] = amplitude;
    out[3] = position;
    return true;
}

bool trigger_limited_feedback(std::uint8_t out[10],
                              std::uint8_t position, std::uint8_t strength)
{
    if (strength > 10) { zero_slot(out); return false; }
    if (strength == 0) return trigger_off(out);

    zero_slot(out);
    out[0] = static_cast<std::uint8_t>(TriggerEffect::Limited_Feedback);
    out[1] = position;
    out[2] = strength;
    return true;
}

bool trigger_limited_weapon(std::uint8_t out[10],
                            std::uint8_t start_raw, std::uint8_t end_raw,
                            std::uint8_t strength)
{
    if (start_raw < 0x10 || end_raw < start_raw ||
        (end_raw - start_raw) > 100 || strength > 10) {
        zero_slot(out); return false;
    }
    if (strength == 0) return trigger_off(out);

    zero_slot(out);
    out[0] = static_cast<std::uint8_t>(TriggerEffect::Limited_Weapon);
    out[1] = start_raw;
    out[2] = end_raw;
    out[3] = strength;
    return true;
}

} // namespace oc::dualsense
