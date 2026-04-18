// DualSense bridge — implementation on top of libDualsense.
// See ds_bridge.h for the public API.

#include "engine/platform/ds_bridge.h"

#include <libDualsense/ds_device.h>
#include <libDualsense/ds_input.h>
#include <libDualsense/ds_output.h>
#include <libDualsense/ds_trigger.h>

#include <algorithm>
#include <cstring>

using namespace oc::dualsense;

// ===========================================================================
// Module state
// ===========================================================================

// Device re-enumeration cadence when disconnected, seconds.
static constexpr float RECONNECT_INTERVAL_SEC = 1.0f;

// Bluetooth silence timeout — if no input reports arrive for this long,
// treat the device as disconnected and close the handle. Needed because
// BT has no cable-yanked signal: SDL_hid_read keeps returning 0 forever
// when the controller is switched off / out of range / BT unpaired on the
// controller side. Without this timeout ds_is_connected() stays true and
// the game never hands off to a fallback controller (e.g. Xbox).
// DualSense streams reports at ~250 Hz even at idle, so a couple of
// seconds of silence is definitive. USB uses the hid_read -1 path and
// doesn't need this.
static constexpr float BT_SILENT_TIMEOUT_SEC = 2.0f;

static Device      s_device;
static InputState  s_input  = {};
static OutputState s_output = {};
static bool            s_output_dirty   = true;
static float           s_reconnect_acc  = 0.0f;
static float           s_bt_silent_acc  = 0.0f;

// ===========================================================================
// Helpers
// ===========================================================================

// Convert an unsigned 8-bit stick axis (center 128) into a signed
// [-1, +1] float. Y axis is flipped so that up is positive, matching
// the convention used by the rest of the game.
static float stick_axis(std::uint8_t raw, bool flip)
{
    // Raw 0..255 with neutral at 128. Raw=0 gives -128/127 ≈ -1.008
    // which overflows downstream DI scaling (gamepad.cpp multiplies
    // by 32767 then adds 32768, producing a negative out-of-range
    // value). Clamp to keep the converted value inside [-1, +1].
    float f = (static_cast<float>(raw) - 128.0f) / 127.0f;
    f = std::clamp(f, -1.0f, 1.0f);
    return flip ? -f : f;
}

static float trigger_axis(std::uint8_t raw)
{
    return static_cast<float>(raw) / 255.0f;
}

// Map game "hand" parameter (0=left, 1=right, 2=both) to trigger
// slot pointers. Returns nullptr for the slots that should not be
// touched. Both slots point at OutputState.trigger_{left,right}.
static void pick_slots(std::uint8_t hand,
                       std::uint8_t** out_left,
                       std::uint8_t** out_right)
{
    *out_left  = (hand == 0 || hand == 2) ? s_output.trigger_left  : nullptr;
    *out_right = (hand == 1 || hand == 2) ? s_output.trigger_right : nullptr;
}

// ===========================================================================
// Lifecycle
// ===========================================================================

void ds_init()
{
    hid_init();
    s_device        = Device{};
    s_input         = InputState{};
    s_output        = OutputState{};
    // Initialize trigger slots to Off (mode 0x05) rather than leaving
    // them all-zero (mode 0x00 is undefined / "reset" in some refs).
    trigger_off(s_output.trigger_left);
    trigger_off(s_output.trigger_right);
    s_output_dirty  = true;
    s_reconnect_acc = RECONNECT_INTERVAL_SEC;  // try immediately on first poll
}

void ds_shutdown()
{
    if (s_device.connected) {
        device_close(&s_device);
    }
    hid_shutdown();
}

void ds_poll_registry(float delta_time)
{
    if (s_device.connected) return;

    s_reconnect_acc += delta_time;
    if (s_reconnect_acc < RECONNECT_INTERVAL_SEC) return;
    s_reconnect_acc = 0.0f;

    if (device_open_first(&s_device)) {
        s_bt_silent_acc = 0.0f;
        // BT handshake: tells the controller to hand over LED /
        // lightbar / player-LED subsystems to us. No-op on USB.
        device_send_init_packet(&s_device);
        // Force a normal output packet on the next frame so LED/trigger
        // state applies. On BT the ~33 ms between this init and the next
        // packet gives the controller time to process the init.
        s_output_dirty = true;
    }
}

bool ds_update_input(float delta_time)
{
    if (!s_device.connected) return false;

    const int n = device_read_input(&s_device, &s_input);
    if (n < 0) {
        // Disconnected — device_read_input already closed the handle.
        s_bt_silent_acc = 0.0f;
        return false;
    }
    if (n == 0) {
        // No new report this frame. On USB the controller streams reports
        // continuously and a read-error would surface any disconnect via
        // the n < 0 branch above. On Bluetooth there is no such signal —
        // hid_read returns 0 forever once the controller goes away — so
        // we watch for prolonged silence and force-close the handle.
        if (s_device.connection == Connection::Bluetooth) {
            s_bt_silent_acc += delta_time;
            if (s_bt_silent_acc >= BT_SILENT_TIMEOUT_SEC) {
                device_close(&s_device);
                s_bt_silent_acc = 0.0f;
                return false;
            }
        }
        return true;
    }

    s_bt_silent_acc = 0.0f;
    return true;
}

void ds_update_output()
{
    if (!s_device.connected) return;
    if (!s_output_dirty) return;

    const int written = device_send_output(&s_device, s_output);
    if (written < 0) {
        // Disconnected — device_send_output already closed the handle.
        return;
    }

    s_output_dirty = false;
}

bool ds_is_connected()
{
    return s_device.connected;
}

// ===========================================================================
// Input readout
// ===========================================================================

bool ds_get_input(DS_InputState* out)
{
    if (!out) return false;
    if (!s_device.connected) return false;

    out->left_stick_x  = stick_axis(s_input.left_stick_x,  /*flip=*/false);
    out->left_stick_y  = stick_axis(s_input.left_stick_y,  /*flip=*/true);
    out->right_stick_x = stick_axis(s_input.right_stick_x, /*flip=*/false);
    out->right_stick_y = stick_axis(s_input.right_stick_y, /*flip=*/true);
    out->trigger_left  = trigger_axis(s_input.left_trigger);
    out->trigger_right = trigger_axis(s_input.right_trigger);

    out->cross    = s_input.cross;
    out->circle   = s_input.circle;
    out->square   = s_input.square;
    out->triangle = s_input.triangle;

    out->l1         = s_input.l1;
    out->r1         = s_input.r1;
    out->l2_digital = s_input.l2;
    out->r2_digital = s_input.r2;

    out->dpad_up    = s_input.dpad_up;
    out->dpad_down  = s_input.dpad_down;
    out->dpad_left  = s_input.dpad_left;
    out->dpad_right = s_input.dpad_right;

    out->start     = s_input.start;
    out->share     = s_input.share;
    out->ps_button = s_input.ps_button;

    out->l3 = s_input.l3;
    out->r3 = s_input.r3;

    out->mute           = s_input.mute;
    out->touchpad_click = s_input.touchpad_click;

    // Feedback state is the low nibble of the raw byte; bit 4 is the
    // effect-active flag carried separately in bLeftTriggerEffectActive.
    out->left_trigger_feedback_state  = s_input.left_trigger_feedback  & 0x0F;
    out->right_trigger_feedback_state = s_input.right_trigger_feedback & 0x0F;
    out->left_trigger_effect_active   = s_input.left_trigger_effect_active;
    out->right_trigger_effect_active  = s_input.right_trigger_effect_active;

    out->touchpad_finger_1_x    = s_input.touchpad_finger_1_x;
    out->touchpad_finger_1_y    = s_input.touchpad_finger_1_y;
    out->touchpad_finger_1_down = s_input.touchpad_finger_1_down;
    out->touchpad_finger_2_x    = s_input.touchpad_finger_2_x;
    out->touchpad_finger_2_y    = s_input.touchpad_finger_2_y;
    out->touchpad_finger_2_down = s_input.touchpad_finger_2_down;

    return true;
}

// ===========================================================================
// Output setters — LED, vibration
// ===========================================================================

void ds_set_lightbar(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_output.lightbar_r == r && s_output.lightbar_g == g && s_output.lightbar_b == b)
        return;
    s_output.lightbar_r = r;
    s_output.lightbar_g = g;
    s_output.lightbar_b = b;
    s_output_dirty = true;
}

void ds_set_player_led(uint8_t led_mask)
{
    if (s_output.player_led_mask == led_mask) return;
    s_output.player_led_mask       = led_mask;
    s_output.player_led_brightness = 0;  // 0 = brightest on DualSense
    s_output_dirty = true;
}

void ds_set_vibration(uint8_t left, uint8_t right)
{
    if (s_output.rumble_left == left && s_output.rumble_right == right) return;
    s_output.rumble_left  = left;
    s_output.rumble_right = right;
    s_output_dirty = true;
}

// ===========================================================================
// Output setters — adaptive trigger effects
// ===========================================================================

void ds_trigger_off(uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_off(l); s_output_dirty = true; }
    if (r) { trigger_off(r); s_output_dirty = true; }
}

// Game API: (start_zone=position, strength_raw, hand). Parameters
// are raw bytes (e.g. 20/200), DualSense mode 0x01 (Simple_Feedback).
void ds_trigger_resistance(uint8_t start_zone, uint8_t strength, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_simple_feedback(l, start_zone, strength); s_output_dirty = true; }
    if (r) { trigger_simple_feedback(r, start_zone, strength); s_output_dirty = true; }
}

// Zone-based Feedback (mode 0x21). position 0..9, strength 0..8.
void ds_trigger_feedback(uint8_t position, uint8_t strength, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_feedback(l, position, strength); s_output_dirty = true; }
    if (r) { trigger_feedback(r, position, strength); s_output_dirty = true; }
}

// Game API: (start_zone, end_zone, strength, unused, hand).
// 4th parameter is unused (kept for ABI compat with ds_bridge.h).
void ds_trigger_weapon(uint8_t start_zone, uint8_t end_zone, uint8_t strength,
                       uint8_t /*unused*/, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_weapon(l, start_zone, end_zone, strength); s_output_dirty = true; }
    if (r) { trigger_weapon(r, start_zone, end_zone, strength); s_output_dirty = true; }
}

// Vibration (mode 0x26). position 0..9, amplitude 0..8, frequency in Hz.
void ds_trigger_vibration(uint8_t position, uint8_t amplitude, uint8_t frequency, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_vibration(l, position, amplitude, frequency); s_output_dirty = true; }
    if (r) { trigger_vibration(r, position, amplitude, frequency); s_output_dirty = true; }
}

// Game API: (start_zone, snap_back, hand). duaLib Bow takes four
// parameters (start, end, strength, snap); pick sensible defaults
// that match what the legacy call likely intended (full-range bow
// with max strength = snap).
void ds_trigger_bow(uint8_t start_zone, uint8_t snap_back, uint8_t hand)
{
    const std::uint8_t end = 8;
    const std::uint8_t strength = snap_back;
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_bow(l, start_zone, end, strength, snap_back); s_output_dirty = true; }
    if (r) { trigger_bow(r, start_zone, end, strength, snap_back); s_output_dirty = true; }
}

void ds_debug_get_trigger_slots(uint8_t left[10], uint8_t right[10])
{
    std::memcpy(left,  s_output.trigger_left,  10);
    std::memcpy(right, s_output.trigger_right, 10);
}

// Game API: (start, behavior_flag, force, amplitude, period,
// frequency, hand). Maps to trigger_machine with uniform amplitudes.
void ds_trigger_machine(uint8_t start_zone, uint8_t /*behavior_flag*/, uint8_t /*force*/,
                        uint8_t amplitude, uint8_t period, uint8_t frequency, uint8_t hand)
{
    const std::uint8_t end = 9;
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_machine(l, start_zone, end, amplitude, amplitude, frequency, period); s_output_dirty = true; }
    if (r) { trigger_machine(r, start_zone, end, amplitude, amplitude, frequency, period); s_output_dirty = true; }
}

// ===========================================================================
// Full-parameter / additional effects (used by the input debug panel)
// ===========================================================================

void ds_trigger_bow_full(uint8_t start, uint8_t end,
                         uint8_t strength, uint8_t snap, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_bow(l, start, end, strength, snap); s_output_dirty = true; }
    if (r) { trigger_bow(r, start, end, strength, snap); s_output_dirty = true; }
}

void ds_trigger_machine_full(uint8_t start, uint8_t end,
                             uint8_t amplitude_a, uint8_t amplitude_b,
                             uint8_t frequency, uint8_t period, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_machine(l, start, end, amplitude_a, amplitude_b, frequency, period); s_output_dirty = true; }
    if (r) { trigger_machine(r, start, end, amplitude_a, amplitude_b, frequency, period); s_output_dirty = true; }
}

void ds_trigger_galloping(uint8_t start, uint8_t end,
                          uint8_t first_foot, uint8_t second_foot,
                          uint8_t frequency, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_galloping(l, start, end, first_foot, second_foot, frequency); s_output_dirty = true; }
    if (r) { trigger_galloping(r, start, end, first_foot, second_foot, frequency); s_output_dirty = true; }
}

void ds_trigger_simple_weapon(uint8_t start_raw, uint8_t end_raw,
                              uint8_t strength_raw, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_simple_weapon(l, start_raw, end_raw, strength_raw); s_output_dirty = true; }
    if (r) { trigger_simple_weapon(r, start_raw, end_raw, strength_raw); s_output_dirty = true; }
}

void ds_trigger_simple_vibration(uint8_t position, uint8_t amplitude,
                                 uint8_t frequency, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_simple_vibration(l, position, amplitude, frequency); s_output_dirty = true; }
    if (r) { trigger_simple_vibration(r, position, amplitude, frequency); s_output_dirty = true; }
}

void ds_trigger_limited_feedback(uint8_t position, uint8_t strength, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_limited_feedback(l, position, strength); s_output_dirty = true; }
    if (r) { trigger_limited_feedback(r, position, strength); s_output_dirty = true; }
}

void ds_trigger_limited_weapon(uint8_t start_raw, uint8_t end_raw,
                               uint8_t strength, uint8_t hand)
{
    std::uint8_t *l, *r;
    pick_slots(hand, &l, &r);
    if (l) { trigger_limited_weapon(l, start_raw, end_raw, strength); s_output_dirty = true; }
    if (r) { trigger_limited_weapon(r, start_raw, end_raw, strength); s_output_dirty = true; }
}
