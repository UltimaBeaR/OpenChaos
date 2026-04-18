// Input debug panel — DualSense page.
//
// Shows DualSense-specific state and interactive tests: sticks /
// buttons / triggers (shared with gamepad page) + trigger feedback
// indicators + touchpad finger visualisation + rumble test + lightbar
// RGB test + player LED toggle test. A single page-local cursor walks
// through all interactive rows (rumble 3 + lightbar 3 + player LEDs 3).

#include "engine/debug/input_debug/input_debug.h"

#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/text/font.h"
#include "engine/input/gamepad.h"
#include "engine/input/gamepad_globals.h"
#include "engine/platform/ds_bridge.h"
#include "engine/platform/ds_bridge_debug.h"

#include <libDualsense/ds_calibration.h>
#include <libDualsense/ds_feature.h>
#include <libDualsense/ds_test.h>

#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>

namespace {

// ---------------------------------------------------------------------------
// Layout constants — logical 640×480 coordinate space (rects scale with
// pipeline; text scales via input_debug_text helper).
//
// The VIEW sub-page lays itself out from scratch (see render_ds_view)
// to approximate the physical DualSense layout — it doesn't share
// anchors with the Xbox page. The only shared piece is MENU_X below,
// for the tests sub-page.
// ---------------------------------------------------------------------------

// Menu x-column for the TESTS sub-page (rumble / lightbar / player LED
// test widgets). Each widget has a title row + value rows at ~12 px/row.
// Y positions are set by render_ds_tests.
constexpr SLONG MENU_X = 20;

constexpr int RUMBLE_ROWS   = 3;
constexpr int LIGHTBAR_ROWS = 3;
// PLAYER_LED_ROWS is defined next to render_player_led further down
// (depends on PLAYER_LED_MASK_ROWS + brightness row below it).

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

float norm_axis(int32_t raw)
{
    return ((float)raw - 32768.0f) / 32768.0f;
}

bool btn(const GamepadState& s, int i)
{
    return (s.rgbButtons[i] & 0x80) != 0;
}

// ---------------------------------------------------------------------------
// Touchpad visualisation (passive — no cursor rows)
// ---------------------------------------------------------------------------

void draw_touchpad_viz(SLONG x, SLONG y, SLONG w, SLONG h)
{
    // Outer box (dim grey).
    AENG_draw_rect(x, y, w, h, 0x202020, INPUT_DEBUG_LAYER_CONTENT, POLY_PAGE_COLOUR);

    // Read finger state (zeroed when DualSense not active).
    int f1_x = 0, f1_y = 0, f2_x = 0, f2_y = 0;
    bool f1_down = false, f2_down = false;
    input_debug_read_ds_touchpad(&f1_x, &f1_y, &f1_down, &f2_x, &f2_y, &f2_down);

    // Map hardware pixel (0..1919, 0..1079) to widget pixel.
    auto to_wx = [&](int hx) { return x + (hx * w) / 1919; };
    auto to_wy = [&](int hy) { return y + (hy * h) / 1079; };

    if (f1_down) {
        AENG_draw_rect(to_wx(f1_x) - 2, to_wy(f1_y) - 2, 4, 4,
                       0x30FF30, INPUT_DEBUG_LAYER_ACCENT, POLY_PAGE_COLOUR);
    }
    if (f2_down) {
        AENG_draw_rect(to_wx(f2_x) - 2, to_wy(f2_y) - 2, 4, 4,
                       0xFF30FF, INPUT_DEBUG_LAYER_ACCENT, POLY_PAGE_COLOUR);
    }

    input_debug_text(x, y - 12, 200, 200, 200, 1, "Touchpad");
    input_debug_text(x, y + h + 3, 180, 180, 180, 1,
        "f1 %s (%4d,%4d)   f2 %s (%4d,%4d)",
        f1_down ? "DN" : "  ", f1_x, f1_y,
        f2_down ? "DN" : "  ", f2_x, f2_y);
}

// ---------------------------------------------------------------------------
// Lightbar RGB widget (3 rows)
// ---------------------------------------------------------------------------

int s_lightbar_r = 0;
int s_lightbar_g = 0;
int s_lightbar_b = 255;   // default blue, matches gamepad_led_reset
int s_lightbar_last_r = -1, s_lightbar_last_g = -1, s_lightbar_last_b = -1;

constexpr int LIGHTBAR_STEP = 16;

int clamp_u8(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

int render_lightbar(SLONG x, SLONG y, int local_cursor)
{
    input_debug_text(x, y, 220, 200, 120, 1, "Lightbar (live)");

    if (local_cursor >= 0 && local_cursor < LIGHTBAR_ROWS) {
        const InputDebugNav& n = input_debug_nav();
        int* tgt = nullptr;
        if (local_cursor == 0) tgt = &s_lightbar_r;
        if (local_cursor == 1) tgt = &s_lightbar_g;
        if (local_cursor == 2) tgt = &s_lightbar_b;
        if (tgt) {
            if (n.left)  *tgt = clamp_u8(*tgt - LIGHTBAR_STEP);
            if (n.right) *tgt = clamp_u8(*tgt + LIGHTBAR_STEP);
        }
    }

    // Apply to controller when any channel changed. ds_set_lightbar
    // already no-ops on unchanged values, but we cache the last-sent set
    // to avoid even that check when nothing moved.
    if (s_lightbar_r != s_lightbar_last_r ||
        s_lightbar_g != s_lightbar_last_g ||
        s_lightbar_b != s_lightbar_last_b) {
        ds_set_lightbar((uint8_t)s_lightbar_r, (uint8_t)s_lightbar_g, (uint8_t)s_lightbar_b);
        ds_update_output();
        s_lightbar_last_r = s_lightbar_r;
        s_lightbar_last_g = s_lightbar_g;
        s_lightbar_last_b = s_lightbar_b;
    }

    const char* names[LIGHTBAR_ROWS] = { "R", "G", "B" };
    const int* values[LIGHTBAR_ROWS] = { &s_lightbar_r, &s_lightbar_g, &s_lightbar_b };

    for (int row = 0; row < LIGHTBAR_ROWS; row++) {
        const bool selected = (row == local_cursor);
        const UBYTE r = selected ? 255 : 150;
        const UBYTE g = selected ? 255 : 150;
        const UBYTE b = selected ? 120 : 150;
        const char* prefix = selected ? "> " : "  ";
        input_debug_text(x + 4, y + 14 + row * 12, r, g, b, 1,
            "%s%s    %3d", prefix, names[row], *values[row]);
    }

    return LIGHTBAR_ROWS;
}

// ---------------------------------------------------------------------------
// Player LED widget — 3 mask-pair toggles + 1 brightness row.
// ---------------------------------------------------------------------------

// Three logical rows for the 5-LED bar — outer pair, inner pair, centre
// single. Hardware mirrors bits symmetrically (0↔4, 1↔3) so we expose
// pairs rather than 5 raw toggles. Bitmask values match PlayerLed::
// constants in libDualsense.
constexpr int PLAYER_LED_MASK_ROWS = 3;
// One extra row under the mask toggles for global brightness (0..2,
// 0 = brightest). Kept inside this widget because brightness scales
// exactly the same LEDs that the mask toggles on/off — putting it
// elsewhere disconnects it visually from the thing it controls.
constexpr int PLAYER_LED_ROWS = PLAYER_LED_MASK_ROWS + 1;
constexpr int PLAYER_LED_BRIGHTNESS_ROW = PLAYER_LED_MASK_ROWS;  // last row

constexpr uint8_t PLAYER_LED_BITS[PLAYER_LED_MASK_ROWS] = {
    0x01 | 0x10,  // Outer pair  (bits 0 and 4)
    0x02 | 0x08,  // Inner pair  (bits 1 and 3)
    0x04,         // Centre      (bit 2)
};
static const char* PLAYER_LED_NAMES[PLAYER_LED_MASK_ROWS] = {
    "Outer pair (LED 1 & 5)",
    "Inner pair (LED 2 & 4)",
    "Centre (LED 3)",
};

uint8_t s_player_led_mask      = 0;
int     s_player_led_last_mask = -1;
int     s_led_brightness       = 0;        // 0 = brightest, 2 = dimmest
int     s_led_brightness_last  = -1;

int render_player_led(SLONG x, SLONG y, int local_cursor)
{
    input_debug_text(x, y, 220, 200, 120, 1, "Player LEDs (live)");

    if (local_cursor >= 0 && local_cursor < PLAYER_LED_MASK_ROWS) {
        const InputDebugNav& n = input_debug_nav();
        if (n.enter) {
            s_player_led_mask ^= PLAYER_LED_BITS[local_cursor];
        }
    } else if (local_cursor == PLAYER_LED_BRIGHTNESS_ROW) {
        const InputDebugNav& n = input_debug_nav();
        if (n.left  && s_led_brightness > 0) s_led_brightness -= 1;
        if (n.right && s_led_brightness < 2) s_led_brightness += 1;
    }

    if ((int)s_player_led_mask != s_player_led_last_mask) {
        ds_set_player_led(s_player_led_mask);
        ds_update_output();
        s_player_led_last_mask = s_player_led_mask;
    }
    if (s_led_brightness != s_led_brightness_last) {
        ds_set_player_led_brightness((uint8_t)s_led_brightness);
        ds_update_output();
        s_led_brightness_last = s_led_brightness;
    }

    for (int row = 0; row < PLAYER_LED_MASK_ROWS; row++) {
        const bool selected = (row == local_cursor);
        // Pair is considered "on" when any of its bits is set — since
        // the firmware mirrors, toggling the pair flips all its bits
        // together so all-on and all-off are the only states reached.
        const bool on = (s_player_led_mask & PLAYER_LED_BITS[row]) != 0;
        const char* prefix = selected ? "> " : "  ";
        if (on) {
            input_debug_text(x + 4, y + 14 + row * 12,
                selected ? 120 : 80, 255, selected ? 120 : 80, 1,
                "%s(x) %s", prefix, PLAYER_LED_NAMES[row]);
        } else {
            input_debug_text(x + 4, y + 14 + row * 12,
                selected ? 255 : 150,
                selected ? 255 : 150,
                selected ? 120 : 150, 1,
                "%s( ) %s", prefix, PLAYER_LED_NAMES[row]);
        }
    }

    // Brightness row, appended under the mask toggles.
    {
        const bool sel = (local_cursor == PLAYER_LED_BRIGHTNESS_ROW);
        const UBYTE r = sel ? 255 : 150;
        const UBYTE g = sel ? 255 : 150;
        const UBYTE b = sel ? 120 : 150;
        const char* prefix = sel ? "> " : "  ";
        input_debug_text(x + 4, y + 14 + PLAYER_LED_BRIGHTNESS_ROW * 12,
                         r, g, b, 1,
                         "%sbrightness       %d (0 = bright, 2 = dim)",
                         prefix, s_led_brightness);
    }

    return PLAYER_LED_ROWS;
}

// ---------------------------------------------------------------------------
// Mute LED widget — single cycle row (Off / On / Blink).
// ---------------------------------------------------------------------------
//
// Player LED brightness lives in the Player LEDs widget (the thing it
// modulates). Lightbar-setup byte was removed (daidr's own UI hides
// it; no observable effect on real hardware). Overall haptic volume
// (0..7 rumble gain) was also removed — the in-game tester doesn't
// have a real use for it, and shipping a knob that's only indirectly
// audible was confusing. The library setters (ds_set_haptic_volume,
// ds_set_lightbar_setup) stay in the bridge for API completeness.

constexpr int MISC_ROWS = 1;

int s_mute_led            = 0;          // 0 = Off, 1 = On, 2 = Blink
int s_mute_led_last       = -1;

const char* mute_led_name(int v)
{
    switch (v) {
        case 1:  return "On";
        case 2:  return "Blink";
        default: return "Off";
    }
}

int render_misc_outputs(SLONG x, SLONG y, int local_cursor)
{
    input_debug_text(x, y, 220, 200, 120, 1, "Mute LED (live)");

    if (local_cursor == 0) {
        const InputDebugNav& n = input_debug_nav();
        // Enter cycles Off → On → Blink → Off; left/right mirror for
        // nav symmetry.
        if (n.enter) s_mute_led = (s_mute_led + 1) % 3;
        if (n.right) s_mute_led = (s_mute_led + 1) % 3;
        if (n.left)  s_mute_led = (s_mute_led + 2) % 3;
    }

    if (s_mute_led != s_mute_led_last) {
        ds_set_mute_led((uint8_t)s_mute_led);
        s_mute_led_last = s_mute_led;
        ds_update_output();
    }

    const bool sel = (local_cursor == 0);
    const UBYTE r = sel ? 255 : 150;
    const UBYTE g = sel ? 255 : 150;
    const UBYTE b = sel ? 120 : 150;
    const char* p = sel ? "> " : "  ";
    input_debug_text(x + 4, y + 14, r, g, b, 1,
                     "%smic mute LED  %s", p, mute_led_name(s_mute_led));

    return MISC_ROWS;
}

// ---------------------------------------------------------------------------
// Controller audio widget: speaker/jack volume sliders + 1 kHz test
// tone. The panel keeps `audio_volumes_enabled=true` for the whole
// duration it is open so the volumes actually apply to the controller
// — no user-visible toggle for this, it'd only be a distraction in a
// tester UI.
// ---------------------------------------------------------------------------
//
// The test tone is a tester-only diagnostic: libDualsense deliberately
// does NOT ship a `play_test_tone()` helper (see
// `new_game_devlog/dualsense_libs_reference/lib_scope.md`). Instead we
// compose it here from the public `test_command()` primitive.
//
// Wire recipe per daidr's controlWaveOut() reference:
//   1. BUILTIN_MIC_CALIB_DATA_VERIFY (AUDIO, action 4) with a 20-byte
//      routing payload — tells the controller which physical output
//      the synthesized tone should come out of (speaker or jack).
//      Skipped when disabling.
//   2. WAVEOUT_CTRL (AUDIO, action 2) with 3 bytes {enable, 1, 0} —
//      actually starts or stops the tone generator.

constexpr int AUDIO_ROWS = 2;  // volume + test tone
constexpr int AUDIO_VOLUME_STEP = 8;
// Start the slider at something audible so the first cycle of the
// test tone plays at a reasonable volume without the user having to
// hunt for it.
constexpr int AUDIO_DEFAULT_VOL = 128;

// Test-tone source enum mirrors the UI cycle order.
enum AudioTestTone {
    AUDIO_TONE_OFF       = 0,
    AUDIO_TONE_SPEAKER   = 1,
    AUDIO_TONE_HEADPHONE = 2,
    AUDIO_TONE_COUNT,
};

// Single user-controlled volume slider. We never overwrite this — it
// reflects exactly what the user last set. Which physical output
// (speaker / jack) the value actually lands on is decided at tone
// start time: the active output gets s_volume, the inactive one gets
// 0 (hardware-leak workaround — the tone would physically leak into
// the non-selected output otherwise). When no tone is playing both
// hardware volumes are 0, but the UI slider still shows the user's
// preferred value so switching the tone back on restores the level.
int  s_volume         = AUDIO_DEFAULT_VOL;
int  s_volume_last    = -1;
int  s_test_tone      = AUDIO_TONE_OFF;
int  s_test_tone_last = -1;

// Audio test-tone sender — runs on a **detached background thread**.
//
// Rationale: each 0x80 feature-report write goes through
// SDL_hid_send_feature_report, which on some Windows + USB paths
// blocks for seconds inside HidD_SetFeature. Doing that on the main
// thread freezes the panel every time the user cycles the test-tone
// row. Fire-and-forget off the UI thread keeps cycling responsive.
static void send_audio_test_tone(int source)
{
    std::thread([source]() {
        using namespace oc::dualsense;
        Device* dev = ds_debug_get_device();
        if (!dev || !dev->connected) return;

        constexpr uint8_t ACTION_ROUTE_PREPARE = 4;  // BUILTIN_MIC_CALIB_DATA_VERIFY
        constexpr uint8_t ACTION_WAVEOUT_CTRL  = 2;

        // Use `test_command` (fire + poll 0x81) rather than
        // `test_command_set` (fire-only) for audio: empirically, a
        // 0x80 write on this firmware isn't acted on until the host
        // reads back a 0x81 — the set-only variant queues the command
        // but leaves it pending until the next test-command cycle
        // (which is why the tone used to start only after switching
        // to the Telemetry page, where its background loader started
        // hitting 0x81). Polling after each send drains that queue
        // synchronously so the tone starts / stops immediately.
        //
        // `test_command` returns PollFailed for these audio actions
        // because the controller doesn't emit a matching 0x81 response
        // — that's fine, the side-effect (tone on/off) still happens.
        std::uint8_t rx[72] = {};
        std::size_t  rx_n   = 0;

        // Each HID round-trip wrapped in the bridge device lock so a
        // main-thread disconnect can't free the SDL_hid_device* while
        // we're inside SDL_hid_send/get_feature_report. Lock released
        // between the route-prepare and waveout-control calls so the
        // main thread can squeeze in an input/output frame.

        // Routing payload is built by the library — byte layout /
        // magic offsets live in `ds_test.cpp::build_waveout_route_payload`,
        // consumers just pick an enum. Only sent when turning on.
        if (source == AUDIO_TONE_SPEAKER || source == AUDIO_TONE_HEADPHONE) {
            std::uint8_t route[20];
            build_waveout_route_payload(
                source == AUDIO_TONE_SPEAKER
                    ? WaveOutRoute::Speaker
                    : WaveOutRoute::Headphone,
                route);
            DSDebugDeviceLock lk;
            if (!dev->connected) return;
            test_command(dev, TestDevice::Audio, ACTION_ROUTE_PREPARE,
                         route, sizeof(route),
                         rx, sizeof(rx), &rx_n);
        }

        const std::uint8_t enable  = (source != AUDIO_TONE_OFF) ? 1 : 0;
        const std::uint8_t ctrl[3] = { enable, 1, 0 };
        rx_n = 0;
        {
            DSDebugDeviceLock lk;
            if (!dev->connected) return;
            test_command(dev, TestDevice::Audio, ACTION_WAVEOUT_CTRL,
                         ctrl, sizeof(ctrl),
                         rx, sizeof(rx), &rx_n);
        }
    }).detach();
}

// Force the test tone off and resync cache. Called from panel
// reset_state so we never leave the controller beeping.
static void audio_test_tone_force_off()
{
    if (s_test_tone != AUDIO_TONE_OFF) {
        send_audio_test_tone(AUDIO_TONE_OFF);
    }
    s_test_tone      = AUDIO_TONE_OFF;
    s_test_tone_last = -1;
}

static const char* test_tone_name(int v)
{
    switch (v) {
        case AUDIO_TONE_SPEAKER:   return "on via speaker";
        case AUDIO_TONE_HEADPHONE: return "on via 3.5mm jack";
        default:                   return "off";
    }
}

// Push the current (s_test_tone, s_volume) pair to the controller.
// Active output gets s_volume, inactive gets 0 (hardware-leak
// workaround — the tone would otherwise bleed into the non-selected
// output). Called on every change to either state.
static void audio_apply_state()
{
    switch (s_test_tone) {
    case AUDIO_TONE_SPEAKER:
        ds_set_audio_route(1);
        ds_set_speaker_volume((uint8_t)s_volume);
        ds_set_headphone_volume(0);
        break;
    case AUDIO_TONE_HEADPHONE:
        ds_set_audio_route(0);
        ds_set_headphone_volume((uint8_t)s_volume);
        ds_set_speaker_volume(0);
        break;
    case AUDIO_TONE_OFF:
    default:
        // Silence both hardware outputs. Leave s_volume alone so the
        // UI slider keeps showing the user's choice.
        ds_set_speaker_volume(0);
        ds_set_headphone_volume(0);
        break;
    }
}

int render_audio_outputs(SLONG x, SLONG y, int local_cursor)
{
    input_debug_text(x, y, 220, 200, 120, 1,
                     "Controller audio (speaker + 3.5mm jack)");

    // Keep the lib in "take over audio" mode while the OUTPUT page is
    // rendering. Idempotent (bridge setter no-ops on unchanged value),
    // flipped back off by reset_state when the panel closes.
    ds_set_audio_volumes_enabled(true);

    if (local_cursor >= 0 && local_cursor < AUDIO_ROWS) {
        const InputDebugNav& n = input_debug_nav();
        switch (local_cursor) {
        case 0:
            if (n.left)  s_volume = clamp_u8(s_volume - AUDIO_VOLUME_STEP);
            if (n.right) s_volume = clamp_u8(s_volume + AUDIO_VOLUME_STEP);
            break;
        case 1:
            // Enter cycles off → speaker → jack → off. left/right mirror.
            if (n.enter) s_test_tone = (s_test_tone + 1) % AUDIO_TONE_COUNT;
            if (n.right) s_test_tone = (s_test_tone + 1) % AUDIO_TONE_COUNT;
            if (n.left)  s_test_tone = (s_test_tone + AUDIO_TONE_COUNT - 1) % AUDIO_TONE_COUNT;
            break;
        }
    }

    // Volume slider is read live: any movement during an active tone
    // immediately updates the active output's hardware volume, the
    // inactive output stays at 0.
    const bool volume_changed    = (s_volume    != s_volume_last);
    const bool test_tone_changed = (s_test_tone != s_test_tone_last);

    if (volume_changed || test_tone_changed) {
        audio_apply_state();
        ds_update_output();
        s_volume_last    = s_volume;
    }
    if (test_tone_changed) {
        // send_audio_test_tone runs on a background thread; by the
        // time it gets to the prepare/WAVEOUT_CTRL pair, the output
        // report with the updated audio_route nibble has already been
        // pushed above via ds_update_output().
        send_audio_test_tone(s_test_tone);
        s_test_tone_last = s_test_tone;
    }

    {
        const bool sel = (local_cursor == 0);
        const UBYTE r = sel ? 255 : 150;
        const UBYTE g = sel ? 255 : 150;
        const UBYTE b = sel ? 120 : 150;
        const char* prefix = sel ? "> " : "  ";
        input_debug_text(x + 4, y + 14, r, g, b, 1,
                         "%svolume            %3d", prefix, s_volume);
    }
    {
        const bool sel    = (local_cursor == 1);
        const bool active = (s_test_tone != AUDIO_TONE_OFF);
        const UBYTE r = sel ? (active ? 120 : 255) : (active ? 80  : 150);
        const UBYTE g = sel ? 255                  : (active ? 255 : 150);
        const UBYTE b = sel ? (active ? 120 : 120) : (active ? 80  : 150);
        const char* prefix = sel ? "> " : "  ";
        input_debug_text(x + 4, y + 26, r, g, b, 1,
                         "%stest tone 1kHz    %s",
                         prefix, test_tone_name(s_test_tone));
    }

    return AUDIO_ROWS;
}

} // namespace

// Sub-pages of the DualSense tab, cycled through by TAB:
//   VIEW      — physical controller layout viz
//   INPUT     — raw readouts (sticks/triggers raw, buttons, battery,
//               headphone, fb byte, touchpad IDs, IMU raw + calibrated)
//   OUTPUT    — rumble / lightbar / player LED / mute LED / haptic
//               volume / lightbar setup / audio volumes test widgets
//   TRIGGERS  — adaptive trigger effect tester (all 12 modes)
//   TELEMETRY — firmware, PCBA, serial, barcodes, MCU/MAC, voltage,
//               system flags (read once on sub-page entry)
// Resets to VIEW on panel open so each session starts with the layout.
enum DualSenseSub {
    DS_SUB_VIEW = 0,
    DS_SUB_INPUT,
    DS_SUB_OUTPUT,
    DS_SUB_TRIGGERS,
    DS_SUB_TELEMETRY,
    DS_SUB_COUNT,
};
namespace {
DualSenseSub s_sub = DS_SUB_VIEW;
// Trigger tester state flags lifted up so toggle_sub / reset_state (which
// sit above the full trigger-tester namespace block further down) can
// poke them. Full tester state (TriggerTesterState s_trig, params) lives
// in its own block near render_ds_triggers.
bool s_trig_dirty       = false;
bool s_trig_first_frame = true;

// INPUT sub-page calibration cache — fetched once per panel session via
// get_sensor_calibration(). Required to convert raw IMU samples into
// physical deg/s / g. Invalidated by reset_state().
oc::dualsense::SensorCalibration s_imu_cal   = {};
bool                             s_imu_cal_loaded = false;

// TELEMETRY sub-page cache — all feature-report / test-command results
// fetched once per panel session. Each field has its own "ok" flag so
// partial failures still display what succeeded.
//
// Loading runs on a **detached background thread** because on Windows
// + USB some SDL_hid_get_feature_report paths block for multiple
// seconds per call, and doing 16 of them on the main thread freezes
// the panel (and, after ~5 seconds of no message-pump activity,
// Windows tags the whole window "Not Responding"). Per-frame pacing
// alone wasn't enough because the blocking happens inside one SDL
// call, not across calls. See `tel_thread_body` below.
//
// Reader / writer sync: a single `s_tel_mutex` protects `s_tel`.
// A generation counter (`s_tel_gen`) lets `reset_state` invalidate an
// in-flight load so a late-finishing thread doesn't overwrite a
// freshly-reset cache.
constexpr int TELEMETRY_LOAD_STEP_COUNT = 16;

struct TelemetryCache {
    bool loaded = false;
    int  load_step = 0;  // progress 0..TELEMETRY_LOAD_STEP_COUNT (driven by thread)

    oc::dualsense::FirmwareInfo fw = {};
    std::uint32_t               bt_patch    = 0;
    bool                        bt_patch_ok = false;
    oc::dualsense::SensorCalibration cal = {};

    std::uint64_t mcu_id    = 0;
    bool          mcu_ok    = false;

    std::uint8_t  bt_mac[6] = {};
    bool          bt_mac_ok = false;

    std::uint64_t pcba_id   = 0;
    bool          pcba_ok   = false;

    std::uint8_t  pcba_full[24] = {};
    std::size_t   pcba_full_len = 0;

    std::uint8_t  serial[32] = {};
    std::size_t   serial_len = 0;

    std::uint8_t  assemble[32] = {};
    std::size_t   assemble_len = 0;

    std::uint8_t  batt_barcode[32] = {};
    std::size_t   batt_barcode_len = 0;

    std::uint8_t  vcm_left[32] = {};
    std::size_t   vcm_left_len = 0;

    std::uint8_t  vcm_right[32] = {};
    std::size_t   vcm_right_len = 0;

    oc::dualsense::BatteryVoltageRaw batt_v = {};

    std::uint8_t pos_tracking    = 0;  bool pos_tracking_ok    = false;
    std::uint8_t always_on       = 0;  bool always_on_ok       = false;
    std::uint8_t auto_switchoff  = 0;  bool auto_switchoff_ok  = false;
};
TelemetryCache    s_tel = {};
std::mutex        s_tel_mutex;
std::atomic<bool> s_tel_load_active{false};
std::atomic<int>  s_tel_gen{0};
}

void input_debug_dualsense_toggle_sub()
{
    const DualSenseSub prev = s_sub;
    s_sub = (DualSenseSub)(((int)s_sub + 1) % (int)DS_SUB_COUNT);

    // Leaving the trigger tester — silence both triggers so the user
    // doesn't feel residual resistance on other sub-pages.
    if (prev == DS_SUB_TRIGGERS && s_sub != DS_SUB_TRIGGERS) {
        ds_trigger_off(2);
        ds_update_output();
    }
    // Entering the trigger tester — push the current tester state fresh
    // so the effect comes back exactly as the user left it last time.
    if (s_sub == DS_SUB_TRIGGERS && prev != DS_SUB_TRIGGERS) {
        s_trig_first_frame = true;
    }
}

void input_debug_dualsense_reset_state()
{
    // Restore LED defaults so that next panel session starts clean and
    // the controller isn't left in whatever state the previous user-set
    // test left behind. Player LED mask goes to 0 (no LEDs lit); lightbar
    // goes to the same default blue that gamepad_led_reset uses.
    s_lightbar_r = 0;
    s_lightbar_g = 0;
    s_lightbar_b = 255;
    s_lightbar_last_r = -1;
    s_lightbar_last_g = -1;
    s_lightbar_last_b = -1;

    s_player_led_mask      = 0;
    s_player_led_last_mask = -1;

    s_sub = DS_SUB_VIEW;

    // Trigger tester: re-push initial state the next time the user
    // enters the TRIGGERS sub-page. Also clear any pending edit flag.
    s_trig_first_frame = true;
    s_trig_dirty       = false;

    // Telemetry / calibration caches: invalidate so the next open
    // re-reads them. The controller may have been swapped between
    // sessions; stale cache would mislead the tester.
    s_imu_cal_loaded = false;
    {
        // Bump the generation so any in-flight telemetry thread's
        // final publish is discarded, and wipe the cache under the
        // same mutex so the wipe is atomic w.r.t. any concurrent
        // advance() call from the thread.
        s_tel_gen.fetch_add(1);
        std::lock_guard<std::mutex> lk(s_tel_mutex);
        s_tel = {};
    }

    // Extra output fields driven by the OUTPUT sub-page: return both
    // the UI state and the controller state to defaults so reopening
    // the panel doesn't inherit a previously-set value that the cached
    // "last applied" check would then skip sending.
    s_led_brightness = 0;                  s_led_brightness_last = -1;
    s_mute_led       = 0;                  s_mute_led_last       = -1;
    s_volume         = AUDIO_DEFAULT_VOL;  s_volume_last         = -1;

    // Force the diagnostic 1 kHz tone off (and resync its cache) so
    // the controller doesn't keep beeping between panel sessions.
    audio_test_tone_force_off();

    ds_set_mute_led(0);
    ds_set_audio_volumes_enabled(false);
    ds_set_speaker_volume(0);
    ds_set_headphone_volume(0);
    ds_set_audio_route(0);
    ds_set_player_led_brightness(0);

    // Send the clean state now so there's no lingering user-set LED
    // visible between panel close and the next game-tick refresh.
    ds_set_player_led(0);
    ds_update_output();
}

// ===========================================================================
// Sub-page: controller layout view
// ===========================================================================
//
// Mirrors the physical DualSense layout in logical 640×480 space as
// closely as the aspect ratio allows. DS-specific constants (not shared
// with the Xbox page) — the goal is to look like the controller, not
// match the Xbox tab's anchors.
//
//    [L1]                                                    [R1]
//    [L2 bar]      [Share]  [Touchpad]  [Options]      [R2 bar]
//                       [D-pad]           [Face buttons]
//                                   [PS] [Mute]
//    [Left stick L3]                              [Right stick R3]
//    X=...  Y=...                                     X=...  Y=...

static void render_ds_view(const GamepadState& s)
{
    // Widths below assume a ~6 px per-character bitmap font. Everything
    // is centred on that anchor so visual alignment matches the
    // physical DualSense: shoulders over their bars, Share above the
    // D-pad and Options above the face buttons (all centred on the
    // stick column below them), PS + Mute in the belly between sticks.
    input_debug_text(20, 48, 255, 255, 255, 1, "DualSense");

    constexpr SLONG L_TRIG_X = 200;
    constexpr SLONG R_TRIG_X = 418;
    constexpr SLONG TR_Y     = 70;
    constexpr SLONG TR_W     = 22;
    constexpr SLONG TR_H     = 96;
    constexpr SLONG LSTK_CX  = 160;
    constexpr SLONG RSTK_CX  = 480;

    // L1 / R1 centred over their bars.
    input_debug_draw_button(L_TRIG_X + TR_W / 2 - 6, 55, "L1", btn(s, 9));
    input_debug_draw_button(R_TRIG_X + TR_W / 2 - 6, 55, "R1", btn(s, 10));

    input_debug_draw_trigger_bar(L_TRIG_X, TR_Y, TR_W, TR_H,
                                 s.trigger_left,  255, "L2");
    input_debug_draw_trigger_bar(R_TRIG_X, TR_Y, TR_W, TR_H,
                                 s.trigger_right, 255, "R2");

    // Touchpad box + click indicator (act/fb feedback lives on the
    // TRIGGERS sub-page, not here).
    constexpr SLONG TPV_X = 232;
    constexpr SLONG TPV_Y = 80;
    constexpr SLONG TPV_W = 178;
    constexpr SLONG TPV_H = 70;
    draw_touchpad_viz(TPV_X, TPV_Y, TPV_W, TPV_H);
    input_debug_draw_button(TPV_X + TPV_W / 2 - 42, TPV_Y - 12,
                            "Touchpad click", btn(s, 17));

    // Share centred over the left stick column, Options over the right —
    // both raised to touchpad-height so they sit at the same level as
    // the touchpad (mirroring the physical DS where Share / Options
    // flank the touchpad, not the D-pad / face buttons below).
    input_debug_draw_button(LSTK_CX - 15, 100, "Share",   btn(s, 4));
    input_debug_draw_button(RSTK_CX - 21, 100, "Options", btn(s, 6));

    // D-pad diamond centred on the left stick column.
    constexpr SLONG DPAD_CY = 235;
    input_debug_draw_button(LSTK_CX -  3, DPAD_CY - 15, "U", btn(s, 11));
    input_debug_draw_button(LSTK_CX - 15, DPAD_CY,      "L", btn(s, 13));
    input_debug_draw_button(LSTK_CX +  9, DPAD_CY,      "R", btn(s, 14));
    input_debug_draw_button(LSTK_CX -  3, DPAD_CY + 15, "D", btn(s, 12));

    // Face buttons diamond centred on the right stick column.
    // Triangle / Cross get a small +3 / -3 x-nudge so their visible
    // weight lines up across top and bottom of the diamond (pure
    // geometric centring leaves them looking offset against each other
    // in the bitmap font).
    constexpr SLONG FACE_CY = 235;
    input_debug_draw_button(RSTK_CX - 21, FACE_CY - 15, "Triangle", btn(s, 3));
    input_debug_draw_button(RSTK_CX - 48, FACE_CY,      "Square",   btn(s, 2));
    input_debug_draw_button(RSTK_CX + 12, FACE_CY,      "Circle",   btn(s, 1));
    input_debug_draw_button(RSTK_CX - 18, FACE_CY + 15, "Cross",    btn(s, 0));

    // ----- Sticks at the bottom, L3 / R3 indicators on the title row,
    // numeric readouts below each box. -----
    constexpr SLONG STK_SIZE    = 96;
    constexpr SLONG STK_CY      = 330;
    constexpr SLONG STK_TITLE_Y = STK_CY - STK_SIZE / 2 - 12;       // 270
    constexpr SLONG STK_READ_Y  = STK_CY + STK_SIZE / 2 + 8;        // 386

    input_debug_draw_stick(LSTK_CX, STK_CY, STK_SIZE,
                           norm_axis(s.lX), norm_axis(s.lY), "Left stick");
    input_debug_draw_button(LSTK_CX + 34, STK_TITLE_Y, "L3", btn(s, 7));
    input_debug_text(LSTK_CX - STK_SIZE / 2, STK_READ_Y, 180, 180, 180, 1,
                     "X=%5d  Y=%5d", s.lX, s.lY);

    input_debug_draw_stick(RSTK_CX, STK_CY, STK_SIZE,
                           norm_axis(s.rX), norm_axis(s.rY), "Right stick");
    input_debug_draw_button(RSTK_CX + 37, STK_TITLE_Y, "R3", btn(s, 8));
    input_debug_text(RSTK_CX - STK_SIZE / 2, STK_READ_Y, 180, 180, 180, 1,
                     "X=%5d  Y=%5d", s.rX, s.rY);

    // PS + Mute stacked centrally between the stick boxes — the
    // "belly" of the controller. Screen width 640, midpoint 320.
    input_debug_draw_button(314, STK_CY - 8, "PS",   btn(s, 5));
    input_debug_draw_button(308, STK_CY + 8, "Mute", btn(s, 18));
}

// ===========================================================================
// Sub-page: adaptive trigger effect tester
// ===========================================================================
//
// Cursor layout:
//   row 0:   [x] L2 trigger           (Enter toggles enable)
//   row 1:   [x] R2 trigger           (Enter toggles enable)
//   row 2:   Effect: < Name >         (left/right cycles effect)
//   row 3+:  per-effect parameter rows (left/right adjusts)
//
// Each effect owns its own parameter state, so cycling Effect back to a
// previous selection restores the values you were tuning there.
//
// Read-only indicators render unconditionally in a bottom block, one
// column per trigger, showing: analog position, feedback state (low
// nibble), effect-active bit, and the raw 10-byte slot dump.

enum TriggerEffectIdx {
    TEFF_OFF = 0,
    TEFF_SIMPLE_FEEDBACK,
    TEFF_SIMPLE_WEAPON,
    TEFF_SIMPLE_VIBRATION,
    TEFF_FEEDBACK,
    TEFF_LIMITED_FEEDBACK,
    TEFF_WEAPON,
    TEFF_LIMITED_WEAPON,
    TEFF_VIBRATION,
    TEFF_BOW,
    TEFF_GALLOPING,
    TEFF_MACHINE,
    TEFF_COUNT,
};

static const char* const TEFF_NAMES[TEFF_COUNT] = {
    "Off",
    "Simple_Feedback",
    "Simple_Weapon",
    "Simple_Vibration",
    "Feedback",
    "Limited_Feedback",
    "Weapon",
    "Limited_Weapon",
    "Vibration",
    "Bow",
    "Galloping",
    "Machine",
};

namespace {

// Per-effect tunables. Initialized to sensible defaults so each effect
// is audibly/tactilely interesting the first time you switch to it.
struct TriggerTesterState {
    bool enable_l = false;
    bool enable_r = true;
    int  effect_idx = TEFF_OFF;

    // Off — no params.
    // Simple_Feedback: raw bytes, full resolution.
    int sfb_pos    = 128;
    int sfb_str    = 200;
    // Simple_Weapon: raw bytes.
    int sw_start   = 64;
    int sw_end     = 160;
    int sw_str     = 255;
    // Simple_Vibration: position raw, amplitude raw, frequency Hz raw.
    int sv_pos     = 0;
    int sv_amp     = 128;
    int sv_freq    = 40;
    // Feedback: zone 0..9, strength 0..8.
    int fb_pos     = 5;
    int fb_str     = 4;
    // Limited_Feedback: raw pos, strength 0..10.
    int lfb_pos    = 128;
    int lfb_str    = 5;
    // Weapon: start 2..7, end start+1..8, strength 0..8.
    int w_start    = 3;
    int w_end      = 5;
    int w_str      = 8;
    // Limited_Weapon: start_raw >=0x10, end_raw, strength 0..10.
    int lw_start   = 64;
    int lw_end     = 128;
    int lw_str     = 8;
    // Vibration (zone-based): pos 0..9, amp 0..8, freq Hz.
    int v_pos      = 0;
    int v_amp      = 6;
    int v_freq     = 40;
    // Bow: start 0..7, end start+1..8, strength 0..8, snap 0..8.
    int bw_start   = 2;
    int bw_end     = 6;
    int bw_str     = 6;
    int bw_snap    = 6;
    // Galloping: start 0..7, end start+1..8, first 0..6, second first+1..7, freq.
    int g_start    = 0;
    int g_end      = 9;
    int g_first    = 2;
    int g_second   = 5;
    int g_freq     = 20;
    // Machine: start 0..7, end start+1..9, ampA 0..7, ampB 0..7, freq, period.
    int m_start    = 1;
    int m_end      = 8;
    int m_ampA     = 7;
    int m_ampB     = 3;
    int m_freq     = 8;
    int m_period   = 80;
};

TriggerTesterState s_trig;

// s_trig_dirty + s_trig_first_frame declared near s_sub at top of file
// so toggle_sub / reset_state (which live above this block) can poke
// them. They're in an anonymous namespace too, so identifier names are
// visible here directly.

// Renders a left/right-editable numeric row. Returns 1 (row count).
// `this_row` is the local row index inside the tester (0 = enable-L
// row, etc.). The row is focused when local_cursor == this_row.
int trig_param_row(SLONG x, SLONG y, int local_cursor, int this_row,
                   const char* name, int* value, int min_v, int max_v, int step)
{
    const bool selected = (local_cursor == this_row);
    if (selected) {
        const InputDebugNav& n = input_debug_nav();
        int before = *value;
        if (n.left) {
            *value -= step;
            if (*value < min_v) *value = min_v;
        }
        if (n.right) {
            *value += step;
            if (*value > max_v) *value = max_v;
        }
        if (*value != before) s_trig_dirty = true;
    }
    const UBYTE r = selected ? 255 : 150;
    const UBYTE g = selected ? 255 : 150;
    const UBYTE b = selected ? 120 : 150;
    const char* prefix = selected ? "> " : "  ";
    input_debug_text(x + 4, y, r, g, b, 1,
                     "%s%-10s %4d", prefix, name, *value);
    return 1;
}

// Enter-toggle row (used for enable-L / enable-R). Returns 1.
int trig_toggle_row(SLONG x, SLONG y, int local_cursor, int this_row,
                    const char* label, bool* value)
{
    const bool selected = (local_cursor == this_row);
    if (selected) {
        const InputDebugNav& n = input_debug_nav();
        if (n.enter) {
            *value = !*value;
            s_trig_dirty = true;
        }
    }
    const UBYTE r = selected ? (*value ? 120 : 255) : (*value ? 80  : 150);
    const UBYTE g = selected ? 255                  : (*value ? 255 : 150);
    const UBYTE b = selected ? (*value ? 120 : 120) : (*value ? 80  : 150);
    const char* prefix = selected ? "> " : "  ";
    input_debug_text(x + 4, y, r, g, b, 1,
                     "%s(%s) %s", prefix, *value ? "x" : " ", label);
    return 1;
}

// Effect cycle row. Returns 1. left/right cycles the effect index.
int trig_effect_row(SLONG x, SLONG y, int local_cursor, int this_row)
{
    const bool selected = (local_cursor == this_row);
    if (selected) {
        const InputDebugNav& n = input_debug_nav();
        if (n.left) {
            s_trig.effect_idx = (s_trig.effect_idx - 1 + TEFF_COUNT) % TEFF_COUNT;
            s_trig_dirty = true;
        }
        if (n.right) {
            s_trig.effect_idx = (s_trig.effect_idx + 1) % TEFF_COUNT;
            s_trig_dirty = true;
        }
    }
    const UBYTE r = selected ? 255 : 180;
    const UBYTE g = selected ? 255 : 180;
    const UBYTE b = selected ? 120 : 180;
    const char* prefix = selected ? "> " : "  ";
    input_debug_text(x + 4, y, r, g, b, 1,
                     "%sEffect: < %s >", prefix, TEFF_NAMES[s_trig.effect_idx]);
    return 1;
}

// Dispatches per-effect parameter rows. `row_offset` is the absolute
// row index of the first parameter row (so selection maths works); the
// function returns the number of rows rendered.
int trig_effect_params(SLONG x, SLONG y, int local_cursor, int row_offset)
{
    // local_cursor is absolute (relative to the whole widget); convert
    // to param-local by subtracting row_offset.
    const int pc = local_cursor - row_offset;
    int r = 0;
    switch (s_trig.effect_idx) {
    case TEFF_OFF:
        break;
    case TEFF_SIMPLE_FEEDBACK:
        r += trig_param_row(x, y + r*12, pc, r, "position", &s_trig.sfb_pos, 0, 255, 8);
        r += trig_param_row(x, y + r*12, pc, r, "strength", &s_trig.sfb_str, 0, 255, 8);
        break;
    case TEFF_SIMPLE_WEAPON:
        r += trig_param_row(x, y + r*12, pc, r, "start",    &s_trig.sw_start, 0, 255, 8);
        r += trig_param_row(x, y + r*12, pc, r, "end",      &s_trig.sw_end,   0, 255, 8);
        r += trig_param_row(x, y + r*12, pc, r, "strength", &s_trig.sw_str,   0, 255, 8);
        break;
    case TEFF_SIMPLE_VIBRATION:
        r += trig_param_row(x, y + r*12, pc, r, "position",  &s_trig.sv_pos,  0, 255, 8);
        r += trig_param_row(x, y + r*12, pc, r, "amplitude", &s_trig.sv_amp,  0, 255, 8);
        r += trig_param_row(x, y + r*12, pc, r, "frequency", &s_trig.sv_freq, 0, 255, 5);
        break;
    case TEFF_FEEDBACK:
        r += trig_param_row(x, y + r*12, pc, r, "position", &s_trig.fb_pos, 0, 9, 1);
        r += trig_param_row(x, y + r*12, pc, r, "strength", &s_trig.fb_str, 0, 8, 1);
        break;
    case TEFF_LIMITED_FEEDBACK:
        r += trig_param_row(x, y + r*12, pc, r, "position", &s_trig.lfb_pos, 0, 255, 8);
        r += trig_param_row(x, y + r*12, pc, r, "strength", &s_trig.lfb_str, 0, 10,  1);
        break;
    case TEFF_WEAPON:
        r += trig_param_row(x, y + r*12, pc, r, "start",    &s_trig.w_start, 2, 7, 1);
        if (s_trig.w_end <= s_trig.w_start) s_trig.w_end = s_trig.w_start + 1;
        r += trig_param_row(x, y + r*12, pc, r, "end",      &s_trig.w_end,   s_trig.w_start + 1, 8, 1);
        r += trig_param_row(x, y + r*12, pc, r, "strength", &s_trig.w_str,   0, 8, 1);
        break;
    case TEFF_LIMITED_WEAPON:
        r += trig_param_row(x, y + r*12, pc, r, "start",    &s_trig.lw_start, 0x10, 255, 8);
        if (s_trig.lw_end < s_trig.lw_start) s_trig.lw_end = s_trig.lw_start;
        r += trig_param_row(x, y + r*12, pc, r, "end",      &s_trig.lw_end,   s_trig.lw_start, std::min(255, s_trig.lw_start + 100), 8);
        r += trig_param_row(x, y + r*12, pc, r, "strength", &s_trig.lw_str,   0, 10, 1);
        break;
    case TEFF_VIBRATION:
        r += trig_param_row(x, y + r*12, pc, r, "position",  &s_trig.v_pos,  0, 9,   1);
        r += trig_param_row(x, y + r*12, pc, r, "amplitude", &s_trig.v_amp,  0, 8,   1);
        r += trig_param_row(x, y + r*12, pc, r, "frequency", &s_trig.v_freq, 0, 255, 5);
        break;
    case TEFF_BOW:
        r += trig_param_row(x, y + r*12, pc, r, "start",    &s_trig.bw_start, 0, 7, 1);
        if (s_trig.bw_end <= s_trig.bw_start) s_trig.bw_end = s_trig.bw_start + 1;
        r += trig_param_row(x, y + r*12, pc, r, "end",      &s_trig.bw_end,   s_trig.bw_start + 1, 8, 1);
        r += trig_param_row(x, y + r*12, pc, r, "strength", &s_trig.bw_str,   0, 8, 1);
        r += trig_param_row(x, y + r*12, pc, r, "snap",     &s_trig.bw_snap,  0, 8, 1);
        break;
    case TEFF_GALLOPING:
        r += trig_param_row(x, y + r*12, pc, r, "start",    &s_trig.g_start,  0, 7, 1);
        if (s_trig.g_end <= s_trig.g_start) s_trig.g_end = s_trig.g_start + 1;
        r += trig_param_row(x, y + r*12, pc, r, "end",      &s_trig.g_end,    s_trig.g_start + 1, 9, 1);
        r += trig_param_row(x, y + r*12, pc, r, "first",    &s_trig.g_first,  0, 6, 1);
        if (s_trig.g_second <= s_trig.g_first) s_trig.g_second = s_trig.g_first + 1;
        r += trig_param_row(x, y + r*12, pc, r, "second",   &s_trig.g_second, s_trig.g_first + 1, 7, 1);
        r += trig_param_row(x, y + r*12, pc, r, "frequency",&s_trig.g_freq,   0, 255, 5);
        break;
    case TEFF_MACHINE:
        r += trig_param_row(x, y + r*12, pc, r, "start",    &s_trig.m_start,  0, 7, 1);
        if (s_trig.m_end <= s_trig.m_start) s_trig.m_end = s_trig.m_start + 1;
        r += trig_param_row(x, y + r*12, pc, r, "end",      &s_trig.m_end,    s_trig.m_start + 1, 9, 1);
        r += trig_param_row(x, y + r*12, pc, r, "ampA",     &s_trig.m_ampA,   0, 7, 1);
        r += trig_param_row(x, y + r*12, pc, r, "ampB",     &s_trig.m_ampB,   0, 7, 1);
        r += trig_param_row(x, y + r*12, pc, r, "frequency",&s_trig.m_freq,   0, 255, 5);
        r += trig_param_row(x, y + r*12, pc, r, "period",   &s_trig.m_period, 0, 255, 5);
        break;
    }
    return r;
}

// Number of param rows the currently-selected effect exposes. Used by
// the cursor bound (total_rows) without re-rendering.
int trig_effect_param_count()
{
    switch (s_trig.effect_idx) {
    case TEFF_OFF:               return 0;
    case TEFF_SIMPLE_FEEDBACK:   return 2;
    case TEFF_SIMPLE_WEAPON:     return 3;
    case TEFF_SIMPLE_VIBRATION:  return 3;
    case TEFF_FEEDBACK:          return 2;
    case TEFF_LIMITED_FEEDBACK:  return 2;
    case TEFF_WEAPON:            return 3;
    case TEFF_LIMITED_WEAPON:    return 3;
    case TEFF_VIBRATION:         return 3;
    case TEFF_BOW:               return 4;
    case TEFF_GALLOPING:         return 5;
    case TEFF_MACHINE:           return 6;
    default: return 0;
    }
}

// Applies the current effect to the supplied hand (0 = left, 1 = right).
void trig_apply_effect_to_hand(uint8_t hand)
{
    switch (s_trig.effect_idx) {
    case TEFF_OFF:
        ds_trigger_off(hand);
        break;
    case TEFF_SIMPLE_FEEDBACK:
        ds_trigger_resistance((uint8_t)s_trig.sfb_pos, (uint8_t)s_trig.sfb_str, hand);
        break;
    case TEFF_SIMPLE_WEAPON:
        ds_trigger_simple_weapon((uint8_t)s_trig.sw_start, (uint8_t)s_trig.sw_end,
                                 (uint8_t)s_trig.sw_str, hand);
        break;
    case TEFF_SIMPLE_VIBRATION:
        ds_trigger_simple_vibration((uint8_t)s_trig.sv_pos, (uint8_t)s_trig.sv_amp,
                                    (uint8_t)s_trig.sv_freq, hand);
        break;
    case TEFF_FEEDBACK:
        ds_trigger_feedback((uint8_t)s_trig.fb_pos, (uint8_t)s_trig.fb_str, hand);
        break;
    case TEFF_LIMITED_FEEDBACK:
        ds_trigger_limited_feedback((uint8_t)s_trig.lfb_pos, (uint8_t)s_trig.lfb_str, hand);
        break;
    case TEFF_WEAPON:
        ds_trigger_weapon((uint8_t)s_trig.w_start, (uint8_t)s_trig.w_end,
                          (uint8_t)s_trig.w_str, 0, hand);
        break;
    case TEFF_LIMITED_WEAPON:
        ds_trigger_limited_weapon((uint8_t)s_trig.lw_start, (uint8_t)s_trig.lw_end,
                                  (uint8_t)s_trig.lw_str, hand);
        break;
    case TEFF_VIBRATION:
        ds_trigger_vibration((uint8_t)s_trig.v_pos, (uint8_t)s_trig.v_amp,
                             (uint8_t)s_trig.v_freq, hand);
        break;
    case TEFF_BOW:
        ds_trigger_bow_full((uint8_t)s_trig.bw_start, (uint8_t)s_trig.bw_end,
                            (uint8_t)s_trig.bw_str,   (uint8_t)s_trig.bw_snap, hand);
        break;
    case TEFF_GALLOPING:
        ds_trigger_galloping((uint8_t)s_trig.g_start, (uint8_t)s_trig.g_end,
                             (uint8_t)s_trig.g_first, (uint8_t)s_trig.g_second,
                             (uint8_t)s_trig.g_freq, hand);
        break;
    case TEFF_MACHINE:
        ds_trigger_machine_full((uint8_t)s_trig.m_start, (uint8_t)s_trig.m_end,
                                (uint8_t)s_trig.m_ampA,  (uint8_t)s_trig.m_ampB,
                                (uint8_t)s_trig.m_freq,  (uint8_t)s_trig.m_period, hand);
        break;
    }
}

// Pushes the current tester state to the controller. Each trigger gets
// either the active effect (if enabled) or Off (if disabled) — so
// disabling a side cleanly silences it without needing to track what
// was last sent there.
void trig_apply_all()
{
    if (s_trig.enable_l) trig_apply_effect_to_hand(0);
    else                 ds_trigger_off(0);
    if (s_trig.enable_r) trig_apply_effect_to_hand(1);
    else                 ds_trigger_off(1);
    ds_update_output();
}

// Column-style read-only block for one trigger. Renders analog
// position (0..255 pull), the motor state nibble the controller
// reports (0..15; semantics vary per effect), the "effect engaged"
// flag, and the raw 10-byte slot the library is currently packing.
void trig_draw_indicators(SLONG x, SLONG y, bool right, int analog_value,
                          const std::uint8_t slot[10])
{
    const char* side = right ? "R2" : "L2";
    input_debug_text(x, y, 220, 200, 120, 1, "%s readout (live from controller)", side);

    input_debug_text(x + 4, y + 14, 200, 200, 200, 1,
                     "analog pull   %3d / 255", analog_value);

    const uint8_t fb = input_debug_read_ds_feedback(right);
    input_debug_text(x + 4, y + 26, 200, 200, 200, 1,
                     "motor state   %2u  (low nibble)", (unsigned)fb);

    const bool act = input_debug_read_ds_effect_active(right);
    input_debug_draw_checkbox(x + 4, y + 38, "effect engaged (act bit)", act);

    input_debug_text(x + 4, y + 52, 160, 160, 160, 1,
                     "outgoing slot bytes (mode + params 0..8):");
    input_debug_text(x + 4, y + 64, 160, 160, 160, 1,
                     "  %02X  %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                     slot[0],
                     slot[1], slot[2], slot[3], slot[4],
                     slot[5], slot[6], slot[7], slot[8], slot[9]);
}

} // namespace

static void render_ds_triggers()
{
    const GamepadState& s = input_debug_read_gamepad_for(INPUT_DEVICE_DUALSENSE);

    input_debug_text(20, 48, 255, 255, 255, 1,
                     "Adaptive trigger test  (TAB to cycle sub-pages)");

    // Fixed rows: enable-L, enable-R, effect. Param rows follow.
    constexpr int FIXED_ROWS = 3;
    const int param_rows = trig_effect_param_count();
    const int total_rows = FIXED_ROWS + param_rows;

    static int s_cursor = 0;
    if (s_cursor >= total_rows) s_cursor = total_rows - 1;
    if (s_cursor < 0)           s_cursor = 0;

    const InputDebugNav& n = input_debug_nav();
    if (n.up)   s_cursor = (s_cursor - 1 + total_rows) % total_rows;
    if (n.down) s_cursor = (s_cursor + 1) % total_rows;

    // Render rows. MENU_X matches the TESTS sub-page so switching between
    // tests / triggers doesn't horizontally shuffle widgets.
    int row_y = 80;
    row_y += 12 * trig_toggle_row(MENU_X, row_y, s_cursor, 0, "L2 trigger", &s_trig.enable_l);
    row_y += 12 * trig_toggle_row(MENU_X, row_y, s_cursor, 1, "R2 trigger", &s_trig.enable_r);
    row_y += 12 * trig_effect_row(MENU_X, row_y, s_cursor, 2);
    trig_effect_params(MENU_X, row_y, s_cursor, FIXED_ROWS);

    // Apply state to the controller when anything changed (or on the
    // first frame so the initial effect is pushed without any input).
    if (s_trig_first_frame || s_trig_dirty) {
        trig_apply_all();
        s_trig_dirty = false;
        s_trig_first_frame = false;
    }

    // Read-only indicators at the bottom: one column per trigger.
    std::uint8_t slot_l[10] = {};
    std::uint8_t slot_r[10] = {};
    ds_debug_get_trigger_slots(slot_l, slot_r);
    constexpr SLONG IND_Y  = 310;
    constexpr SLONG IND_XL = 20;
    constexpr SLONG IND_XR = 330;
    trig_draw_indicators(IND_XL, IND_Y, false, s.trigger_left,  slot_l);
    trig_draw_indicators(IND_XR, IND_Y, true,  s.trigger_right, slot_r);
}

// ===========================================================================
// Sub-page: raw input readouts (sticks/triggers raw bytes, full button
// set, battery, headphone, fb bytes, touchpad finger IDs, IMU raw +
// calibrated)
// ===========================================================================
//
// Passive — shows everything libDualsense parses from the input
// report, in raw device units where possible. The lib writes all these
// fields every tick (via ds_update_input → device_read_input); we pull
// the raw snapshot through ds_debug_get_raw_input() from ds_bridge.
//
// On the first frame after sub-page entry, load IMU factory
// calibration (feature report 0x05) once so the Motion column can
// show physical units next to raw ticks. The cache is invalidated in
// input_debug_dualsense_reset_state so a controller swap between
// panel sessions picks up the new cal.

static void ensure_imu_calibration_loaded()
{
    if (s_imu_cal_loaded) return;

    oc::dualsense::Device* dev = ds_debug_get_device();
    if (!dev || !dev->connected) return;

    // Runs on main thread, but the background telemetry / audio-tone
    // threads may be inside their own HID calls on the same device.
    // Take the bridge device lock to serialise.
    DSDebugDeviceLock lk;
    if (!dev->connected) return;
    if (oc::dualsense::get_sensor_calibration(dev, &s_imu_cal)) {
        s_imu_cal_loaded = s_imu_cal.valid;
    }
}

// Horizontal signed bar: dim background, coloured fill extending from
// the centre proportional to val/range. Used for gyro (deg/s) and
// accel (g) visualisation on the INPUT sub-page.
//   val in [-range, +range] draws a proportional fill;
//   |val| > range clamps to the end and draws in "overflow" red.
// x, y = top-left of the bar; w, h = dimensions.
static void draw_centered_bar(SLONG x, SLONG y, SLONG w, SLONG h,
                              float val, float range)
{
    // Background.
    AENG_draw_rect(x, y, w, h, 0x202020,
                   INPUT_DEBUG_LAYER_CONTENT, POLY_PAGE_COLOUR);
    // Centre tick (1px vertical line).
    const SLONG cx = x + w / 2;
    AENG_draw_rect(cx, y, 1, h, 0x606060,
                   INPUT_DEBUG_LAYER_CONTENT, POLY_PAGE_COLOUR);

    if (range <= 0.0f) return;
    float norm = val / range;
    bool overflow = false;
    if (norm > 1.0f)  { norm = 1.0f;  overflow = true; }
    if (norm < -1.0f) { norm = -1.0f; overflow = true; }

    const SLONG half     = w / 2;
    const SLONG fill_w   = (SLONG)((norm < 0.0f ? -norm : norm) * (float)half);
    if (fill_w <= 0) return;

    const unsigned int colour = overflow ? 0xFF4040 : 0x50C0FF;
    const SLONG x0 = (norm >= 0.0f) ? cx : (cx - fill_w);
    AENG_draw_rect(x0, y + 1, fill_w, h - 2, colour,
                   INPUT_DEBUG_LAYER_ACCENT, POLY_PAGE_COLOUR);
}

static const char* charge_status_text(bool charging, bool full, uint8_t raw_high_nibble)
{
    // battery_charging and battery_full are mutually exclusive already;
    // the raw nibble lets us also surface the error codes (10/11/15)
    // that daidr's InputInfo.vue exposes verbatim.
    if (full)     return "charging_complete";
    if (charging) return "charging";
    switch (raw_high_nibble) {
        case 0:  return "discharging";
        case 10: return "abnormal_voltage";
        case 11: return "abnormal_temperature";
        case 15: return "charging_error";
        default: return "unknown";
    }
}

static void render_ds_input()
{
    input_debug_text(20, 48, 255, 255, 255, 1,
                     "DualSense input  (extras not on the Layout view; TAB to cycle)");

    const oc::dualsense::InputState* in = ds_debug_get_raw_input();
    const bool connected = ds_is_connected();

    if (!connected) {
        input_debug_text(20, 80, 200, 80, 80, 1,
                         "DualSense not connected — raw readout unavailable");
        return;
    }

    // Load IMU calibration once on entry. Safe to call every frame;
    // no-op after the first successful load.
    ensure_imu_calibration_loaded();

    // Unlike the Layout view on the VIEW sub-page which shows sticks /
    // buttons / d-pad / triggers / touchpad finger positions visually,
    // this sub-page surfaces the values the library parses that do NOT
    // have a home on the controller layout: power/audio status, raw
    // trigger-feedback bytes (beyond the single "active" dot), touch
    // finger IDs, and motion sensors.

    // Pull the raw nibble from the status0 byte. We don't expose it
    // through InputState (the lib cooks it into bool charging / full),
    // so reconstruct for the "error codes" text. status0 is at HID
    // offset 52 after framing strip, but we derive charging/full from
    // bool fields — `raw_high_nibble` is only needed for the error
    // branches in charge_status_text. Best-effort: infer 0 = discharge,
    // anything else is error if the bools are both false.
    std::uint8_t charge_hi = 0;
    if (in->battery_charging) charge_hi = 1;
    else if (in->battery_full) charge_hi = 2;

    // ---- Column A: power / audio / trigger feedback / finger IDs ----
    constexpr SLONG COL_A_X = 15;
    SLONG y = 72;
    input_debug_text(COL_A_X, y, 220, 200, 120, 1, "Power / audio");
    y += 14;
    input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                     "battery     %3u%%",
                     (unsigned)in->battery_level_percent);
    y += 12;
    input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                     "status      %s",
                     charge_status_text(in->battery_charging, in->battery_full, charge_hi));
    y += 12;
    input_debug_draw_checkbox(COL_A_X + 4, y, "headphone jack",
                              in->headphone_connected);
    y += 12;
    input_debug_draw_checkbox(COL_A_X + 4, y, "microphone (on jack)",
                              in->mic_connected);
    y += 18;

    // (Adaptive-trigger feedback lives on the Triggers sub-page where
    // the user can actually see which effect is currently running —
    // duplicating it here was noise.)

    input_debug_text(COL_A_X, y, 220, 200, 120, 1, "Touch IDs");
    y += 14;
    input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                     "f1 id 0x%02X  %s",
                     (unsigned)in->touchpad_finger_1_id,
                     in->touchpad_finger_1_down ? "contact" : "lifted");
    y += 12;
    input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                     "f2 id 0x%02X  %s",
                     (unsigned)in->touchpad_finger_2_id,
                     in->touchpad_finger_2_down ? "contact" : "lifted");

    // ---- Column B: motion (gyro + accel with centered bars, raw,
    //                         calibrated, timestamp, temperature) -----
    constexpr SLONG COL_B_X      = 240;
    constexpr SLONG BAR_LABEL_W  = 38;   // px for "pitch" / "yaw  " / "roll " / "X" etc
    constexpr SLONG BAR_W        = 180;
    constexpr SLONG BAR_H        = 8;
    constexpr float GYRO_RANGE_DPS = 500.0f;   // visual scale; overflow is red
    constexpr float ACCEL_RANGE_G  = 2.0f;

    y = 72;
    input_debug_text(COL_B_X, y, 220, 200, 120, 1,
                     "Gyro  (bar scale \u00b1500 deg/s; red = out of range)");
    y += 14;

    auto draw_gyro_row = [&](const char* label, std::int16_t raw, float dps) {
        input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1, "%s", label);
        draw_centered_bar(COL_B_X + BAR_LABEL_W, y, BAR_W, BAR_H,
                          dps, GYRO_RANGE_DPS);
        if (s_imu_cal_loaded) {
            input_debug_text(COL_B_X + BAR_LABEL_W + BAR_W + 6, y,
                             200, 200, 200, 1,
                             "%+7.1f\u00b0/s  raw %+6d", dps, (int)raw);
        } else {
            input_debug_text(COL_B_X + BAR_LABEL_W + BAR_W + 6, y,
                             200, 200, 200, 1,
                             "raw %+6d  (no cal)", (int)raw);
        }
        y += 12;
    };

    if (s_imu_cal_loaded) {
        const float pdps = oc::dualsense::calibrate_gyro_pitch_deg_per_sec(in->gyro_pitch, s_imu_cal);
        const float ydps = oc::dualsense::calibrate_gyro_yaw_deg_per_sec  (in->gyro_yaw,   s_imu_cal);
        const float rdps = oc::dualsense::calibrate_gyro_roll_deg_per_sec (in->gyro_roll,  s_imu_cal);
        draw_gyro_row("pitch", in->gyro_pitch, pdps);
        draw_gyro_row("yaw",   in->gyro_yaw,   ydps);
        draw_gyro_row("roll",  in->gyro_roll,  rdps);
    } else {
        // No calibration — scale raw int16 by a heuristic so the bar
        // still shows motion. Full-scale ≈ 32767; use that as the
        // visual range.
        draw_gyro_row("pitch", in->gyro_pitch, (float)in->gyro_pitch * (GYRO_RANGE_DPS / 32767.0f));
        draw_gyro_row("yaw",   in->gyro_yaw,   (float)in->gyro_yaw   * (GYRO_RANGE_DPS / 32767.0f));
        draw_gyro_row("roll",  in->gyro_roll,  (float)in->gyro_roll  * (GYRO_RANGE_DPS / 32767.0f));
    }
    y += 8;

    input_debug_text(COL_B_X, y, 220, 200, 120, 1,
                     "Accel (bar scale \u00b12 g; red = out of range)");
    y += 14;

    auto draw_accel_row = [&](const char* label, std::int16_t raw, float g) {
        input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1, "%s", label);
        draw_centered_bar(COL_B_X + BAR_LABEL_W, y, BAR_W, BAR_H,
                          g, ACCEL_RANGE_G);
        if (s_imu_cal_loaded) {
            input_debug_text(COL_B_X + BAR_LABEL_W + BAR_W + 6, y,
                             200, 200, 200, 1,
                             "%+5.2f g  raw %+6d", g, (int)raw);
        } else {
            input_debug_text(COL_B_X + BAR_LABEL_W + BAR_W + 6, y,
                             200, 200, 200, 1,
                             "raw %+6d  (no cal)", (int)raw);
        }
        y += 12;
    };

    if (s_imu_cal_loaded) {
        const float axg = oc::dualsense::calibrate_accel_x_g(in->accel_x, s_imu_cal);
        const float ayg = oc::dualsense::calibrate_accel_y_g(in->accel_y, s_imu_cal);
        const float azg = oc::dualsense::calibrate_accel_z_g(in->accel_z, s_imu_cal);
        draw_accel_row("X", in->accel_x, axg);
        draw_accel_row("Y", in->accel_y, ayg);
        draw_accel_row("Z", in->accel_z, azg);
    } else {
        draw_accel_row("X", in->accel_x, (float)in->accel_x * (ACCEL_RANGE_G / 32767.0f));
        draw_accel_row("Y", in->accel_y, (float)in->accel_y * (ACCEL_RANGE_G / 32767.0f));
        draw_accel_row("Z", in->accel_z, (float)in->accel_z * (ACCEL_RANGE_G / 32767.0f));
    }
    y += 8;

    input_debug_text(COL_B_X, y, 220, 200, 120, 1, "Misc IMU");
    y += 14;
    input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1,
                     "timestamp %10u us", (unsigned)in->motion_timestamp);
    y += 12;
    input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1,
                     "temperature %4d (raw)", (int)in->motion_temperature);

    if (!s_imu_cal_loaded) {
        y += 16;
        input_debug_text(COL_B_X, y, 160, 160, 160, 1,
                         "(calibration unavailable \u2014 bars use raw-\u00b132767 fallback)");
    }
}

// ===========================================================================
// Sub-page: telemetry / feature reports (read once on entry, cached)
// ===========================================================================
//
// Pulls every read-only factory / identification / firmware report
// that libDualsense exposes and displays them. The load is a one-shot
// per panel session (or per TAB into this sub-page after a reset) —
// each feature report / test command is a synchronous HID round-trip,
// typically 5-20 ms; ~15 reports × 20 ms = a visible hitch on first
// entry, which is acceptable for a debug panel.

// Format a 32-bit firmware version as "major.minor.patch" the way
// daidr's formatThreePartVersion does: high-byte, next-byte,
// low-16-bit patch as decimals. Example: 0x0110002A → "1.16.42".
static void format_three_part_version(char* buf, std::size_t buf_size,
                                      std::uint32_t v)
{
    std::snprintf(buf, buf_size, "%u.%u.%u",
        (unsigned)((v >> 24) & 0xFFu),
        (unsigned)((v >> 16) & 0xFFu),
        (unsigned)( v        & 0xFFFFu));
}

// 16-bit "update" version: two hex halves separated by a dot.
// Example: 0x0630 → "6.30".
static void format_update_version(char* buf, std::size_t buf_size,
                                  std::uint16_t v)
{
    std::snprintf(buf, buf_size, "%X.%02X",
        (unsigned)((v >> 8) & 0xFFu),
        (unsigned)( v       & 0xFFu));
}

// DSP version: two 4-digit hex halves separated by an underscore.
// Example: 0x00031010 → "0003_1010".
static void format_dsp_version(char* buf, std::size_t buf_size,
                               std::uint32_t v)
{
    std::snprintf(buf, buf_size, "%04X_%04X",
        (unsigned)((v >> 16) & 0xFFFFu),
        (unsigned)( v        & 0xFFFFu));
}

// Map the DualSense serial-number "colour code" (characters 5..6 of
// the serial, Shift-JIS decoded — identical to ASCII for these
// printable characters) to the factory colour variant name. Mirrors
// daidr's DualSenseColorMap in ds.type.ts.
static const char* ds_color_variant_from_serial(const std::uint8_t* serial,
                                                 std::size_t len)
{
    if (len < 6) return nullptr;
    const std::uint16_t key =
        (static_cast<std::uint16_t>(serial[4]) << 8) |
         static_cast<std::uint16_t>(serial[5]);
    switch (key) {
        case (std::uint16_t('0') << 8) | '0': return "White";
        case (std::uint16_t('0') << 8) | '1': return "Midnight Black";
        case (std::uint16_t('0') << 8) | '2': return "Cosmic Red";
        case (std::uint16_t('0') << 8) | '3': return "Nova Pink";
        case (std::uint16_t('0') << 8) | '4': return "Galactic Purple";
        case (std::uint16_t('0') << 8) | '5': return "Starlight Blue";
        case (std::uint16_t('0') << 8) | '6': return "Grey Camouflage";
        case (std::uint16_t('0') << 8) | '7': return "Volcanic Red";
        case (std::uint16_t('0') << 8) | '8': return "Sterling Silver";
        case (std::uint16_t('0') << 8) | '9': return "Cobalt Blue";
        case (std::uint16_t('1') << 8) | '0': return "Chroma Teal";
        case (std::uint16_t('1') << 8) | '1': return "Chroma Indigo";
        case (std::uint16_t('1') << 8) | '2': return "Chroma Pearl";
        case (std::uint16_t('3') << 8) | '0': return "30th Anniversary";
        case (std::uint16_t('Z') << 8) | '1': return "God of War Ragnarok";
        case (std::uint16_t('Z') << 8) | '2': return "Spider-Man 2";
        case (std::uint16_t('Z') << 8) | '3': return "Astro Bot";
        case (std::uint16_t('Z') << 8) | '4': return "Fortnite";
        case (std::uint16_t('Z') << 8) | '6': return "The Last of Us";
        case (std::uint16_t('Z') << 8) | 'B': return "Icon Blue LE";
        case (std::uint16_t('Z') << 8) | 'E': return "Genshin Impact";
        default: return nullptr;
    }
}

// Second character of the serial encodes the PCB generation (BDM-010
// through BDM-050). iFixit docs reference:
// https://www.ifixit.com/Wiki/How_to_Identify_PS5_DualSense_Controller_Version
static const char* ds_board_version_from_serial(const std::uint8_t* serial,
                                                 std::size_t len)
{
    if (len < 2) return nullptr;
    switch (serial[1]) {
        case '1': return "BDM-010";
        case '2': return "BDM-020";
        case '3': return "BDM-030";
        case '4': return "BDM-040";
        case '5': return "BDM-050";
        default:  return nullptr;
    }
}

// Hex-dump helper: formats `len` bytes as 2-char hex pairs into `buf`
// (which must be at least `len*3 + 1` bytes). Separates with spaces.
static void hex_dump_line(char* buf, std::size_t buf_size,
                          const std::uint8_t* bytes, std::size_t len)
{
    std::size_t off = 0;
    for (std::size_t i = 0; i < len && off + 3 < buf_size; ++i) {
        int n = std::snprintf(buf + off, buf_size - off, "%02X ", (unsigned)bytes[i]);
        if (n <= 0) break;
        off += (std::size_t)n;
    }
    if (off < buf_size) buf[off] = 0;
    else if (buf_size > 0) buf[buf_size - 1] = 0;
}

// Background telemetry loader. Fills a LOCAL TelemetryCache, then
// publishes it into s_tel under s_tel_mutex, guarded by a generation
// counter so `reset_state` can invalidate a late-finishing load.
//
// Each HID call is wrapped in a `DSDebugDeviceLock` scope so a main-thread
// disconnect / hotplug close can't free the SDL_hid_device* while we're
// inside SDL_hid_get_feature_report. Lock is released between calls so
// main-thread ds_update_input / ds_update_output can run between our
// steps (otherwise a full BT load would freeze the panel for ~300 ms).
// If the device is closed mid-load the `dev->connected` check inside the
// lock scope short-circuits each remaining step.
static void tel_thread_body(int my_gen)
{
    using namespace oc::dualsense;
    Device* dev = ds_debug_get_device();

    TelemetryCache local = {};
    local.load_step = 0;

    // Per-step helper: take the device lock, verify still connected,
    // run the HID call, release. Returns false once device is gone.
    auto step = [&](auto&& fn) -> bool {
        if (s_tel_gen.load() != my_gen) return false;
        DSDebugDeviceLock lk;
        if (!dev || !dev->connected) return false;
        fn();
        return true;
    };

    auto advance = [&](int n) {
        if (s_tel_gen.load() != my_gen) return;
        std::lock_guard<std::mutex> lk(s_tel_mutex);
        s_tel.load_step = n;
    };

    step([&] { get_firmware_info(dev, &local.fw); });                                              advance(1);
    step([&] { local.bt_patch_ok = get_bt_patch_version(dev, &local.bt_patch); });                 advance(2);
    step([&] { get_sensor_calibration(dev, &local.cal); });                                        advance(3);
    step([&] { local.mcu_ok = get_mcu_unique_id(dev, &local.mcu_id); });                           advance(4);
    step([&] { local.bt_mac_ok = get_bd_mac_address(dev, local.bt_mac); });                        advance(5);
    step([&] { local.pcba_ok = get_pcba_id(dev, &local.pcba_id); });                               advance(6);
    step([&] { local.pcba_full_len = get_pcba_id_full(dev, local.pcba_full); });                   advance(7);
    step([&] { local.serial_len = get_serial_number(dev, local.serial); });                        advance(8);
    step([&] { local.assemble_len = get_assemble_parts_info(dev, local.assemble); });              advance(9);
    step([&] { local.batt_barcode_len = get_battery_barcode(dev, local.batt_barcode); });          advance(10);
    step([&] { local.vcm_left_len = get_vcm_left_barcode(dev, local.vcm_left); });                 advance(11);
    step([&] { local.vcm_right_len = get_vcm_right_barcode(dev, local.vcm_right); });              advance(12);
    step([&] { get_battery_voltage(dev, &local.batt_v); });                                        advance(13);
    step([&] { local.pos_tracking_ok = get_position_tracking_state(dev, &local.pos_tracking); }); advance(14);
    step([&] { local.always_on_ok = get_always_on_startup_state(dev, &local.always_on); });        advance(15);
    step([&] { local.auto_switchoff_ok = get_auto_switchoff_flag(dev, &local.auto_switchoff); });  advance(16);

    local.loaded    = true;
    local.load_step = TELEMETRY_LOAD_STEP_COUNT;

    // Publish if our generation is still current. Otherwise discard —
    // the panel was reset (or the controller swapped) while we worked.
    if (s_tel_gen.load() == my_gen) {
        std::lock_guard<std::mutex> lk(s_tel_mutex);
        if (s_tel_gen.load() == my_gen) {
            s_tel = local;
        }
    }

    s_tel_load_active.store(false);
}

// Kick off a telemetry load if one isn't already running and we
// haven't yet cached a result for the current generation.
static void start_telemetry_load_if_needed()
{
    if (s_tel.loaded) return;
    if (s_tel_load_active.exchange(true)) return;  // already running

    const int my_gen = s_tel_gen.load();
    std::thread t(tel_thread_body, my_gen);
    t.detach();
}

static void render_ds_telemetry()
{
    input_debug_text(20, 48, 255, 255, 255, 1,
                     "DualSense telemetry  (TAB to cycle sub-pages)");

    if (!ds_is_connected()) {
        input_debug_text(20, 80, 200, 80, 80, 1,
                         "DualSense not connected — telemetry unavailable");
        return;
    }

    // Telemetry loading runs on a detached background thread so
    // blocking HID calls can't freeze the panel.
    start_telemetry_load_if_needed();

    // Hold the mutex for the whole render — the thread writes s_tel
    // under the same mutex and each write is brief (single field
    // assignment), so contention is negligible but race-free.
    std::lock_guard<std::mutex> lk(s_tel_mutex);

    if (!s_tel.loaded) {
        input_debug_text(20, 66, 200, 200, 120, 1,
                         "Loading factory data (%d / %d)...  (running in background)",
                         s_tel.load_step, TELEMETRY_LOAD_STEP_COUNT);
    }

    // ---- Column A: firmware + calibration summary + IDs + PCBA ----
    constexpr SLONG COL_A_X = 15;
    SLONG y = 72;

    input_debug_text(COL_A_X, y, 220, 200, 120, 1, "Firmware (report 0x20)");
    y += 14;
    if (s_tel.fw.valid) {
        char main_fw[24], sbl_fw[24], mcu_dsp[24], dsp_fw[24], upd_ver[16];
        format_three_part_version(main_fw,  sizeof(main_fw),  s_tel.fw.mainFwVersion);
        format_three_part_version(sbl_fw,   sizeof(sbl_fw),   s_tel.fw.sblFwVersion);
        format_three_part_version(mcu_dsp,  sizeof(mcu_dsp),  s_tel.fw.spiderDspFwVersion);
        format_dsp_version       (dsp_fw,   sizeof(dsp_fw),   s_tel.fw.dspFwVersion);
        format_update_version    (upd_ver,  sizeof(upd_ver),  s_tel.fw.updateVersion);

        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "built            %s  %s", s_tel.fw.buildDate, s_tel.fw.buildTime);
        y += 12;
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "fwType  0x%04X   swSeries 0x%04X",
                         (unsigned)s_tel.fw.fwType, (unsigned)s_tel.fw.swSeries);
        y += 12;
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "hwInfo           0x%08X  (board %u)",
                         (unsigned)s_tel.fw.hwInfo, (unsigned)(s_tel.fw.hwInfo & 0xFFFFu));
        y += 12;
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "main FW          %s     update %s",
                         main_fw, upd_ver);
        y += 12;
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "SBL FW           %s     DSP FW %s",
                         sbl_fw, dsp_fw);
        y += 12;
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "MCU DSP FW       %s", mcu_dsp);
        y += 12;
        char dev_info[80];
        hex_dump_line(dev_info, sizeof(dev_info), s_tel.fw.deviceInfo, 12);
        input_debug_text(COL_A_X + 4, y, 180, 180, 180, 1,
                         "deviceInfo  %s", dev_info);
        y += 14;
    } else {
        input_debug_text(COL_A_X + 4, y, 200, 80, 80, 1, "(report failed)");
        y += 14;
    }

    input_debug_text(COL_A_X, y, 220, 200, 120, 1, "BT patch (report 0x22)");
    y += 14;
    if (s_tel.bt_patch_ok) {
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "patch   0x%08X", (unsigned)s_tel.bt_patch);
    } else {
        input_debug_text(COL_A_X + 4, y, 180, 180, 180, 1,
                         "(not available on this firmware)");
    }
    y += 16;

    input_debug_text(COL_A_X, y, 220, 200, 120, 1, "Sensor calibration (report 0x05)");
    y += 14;
    if (s_tel.cal.valid) {
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "gyro bias    p=%5d  y=%5d  r=%5d",
                         (int)s_tel.cal.gyro_pitch_bias,
                         (int)s_tel.cal.gyro_yaw_bias,
                         (int)s_tel.cal.gyro_roll_bias);
        y += 12;
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "gyro speed   +=%5d  -=%5d",
                         (int)s_tel.cal.gyro_speed_plus,
                         (int)s_tel.cal.gyro_speed_minus);
        y += 12;
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "accel X     +=%5d  -=%5d",
                         (int)s_tel.cal.accel_x_plus, (int)s_tel.cal.accel_x_minus);
        y += 12;
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "accel Y     +=%5d  -=%5d",
                         (int)s_tel.cal.accel_y_plus, (int)s_tel.cal.accel_y_minus);
        y += 12;
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "accel Z     +=%5d  -=%5d",
                         (int)s_tel.cal.accel_z_plus, (int)s_tel.cal.accel_z_minus);
        y += 14;
    } else {
        input_debug_text(COL_A_X + 4, y, 200, 80, 80, 1, "(report failed)");
        y += 14;
    }

    input_debug_text(COL_A_X, y, 220, 200, 120, 1, "Identifiers");
    y += 14;
    if (s_tel.mcu_ok) {
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "MCU unique  0x%016llX",
                         (unsigned long long)s_tel.mcu_id);
    } else {
        input_debug_text(COL_A_X + 4, y, 180, 180, 180, 1, "MCU unique  n/a");
    }
    y += 12;
    if (s_tel.bt_mac_ok) {
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "BT MAC      %02X:%02X:%02X:%02X:%02X:%02X",
                         (unsigned)s_tel.bt_mac[0], (unsigned)s_tel.bt_mac[1],
                         (unsigned)s_tel.bt_mac[2], (unsigned)s_tel.bt_mac[3],
                         (unsigned)s_tel.bt_mac[4], (unsigned)s_tel.bt_mac[5]);
    } else {
        input_debug_text(COL_A_X + 4, y, 180, 180, 180, 1, "BT MAC      n/a");
    }
    y += 12;
    if (s_tel.pcba_ok) {
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "PCBA (48b)  0x%012llX",
                         (unsigned long long)(s_tel.pcba_id & 0xFFFFFFFFFFFFull));
    } else {
        input_debug_text(COL_A_X + 4, y, 180, 180, 180, 1, "PCBA (48b)  n/a");
    }
    y += 12;
    if (s_tel.pcba_full_len > 0) {
        char hex[80];
        hex_dump_line(hex, sizeof(hex), s_tel.pcba_full, 12);
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "PCBA full   %s", hex);
        y += 12;
        hex_dump_line(hex, sizeof(hex), s_tel.pcba_full + 12, 12);
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "            %s", hex);
    } else {
        input_debug_text(COL_A_X + 4, y, 180, 180, 180, 1, "PCBA full   n/a");
    }
    y += 16;

    // Colour variant + board generation are both derived from the
    // serial number (daidr does the same — the serial encodes the
    // factory colour code in chars 5..6 and the PCB generation in
    // char 2). Only show if we were able to read the serial.
    input_debug_text(COL_A_X, y, 220, 200, 120, 1, "Model");
    y += 14;
    if (s_tel.serial_len > 0) {
        const char* colour = ds_color_variant_from_serial(s_tel.serial, s_tel.serial_len);
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "colour variant  %s", colour ? colour : "(unknown code)");
        y += 12;
        const char* board = ds_board_version_from_serial(s_tel.serial, s_tel.serial_len);
        input_debug_text(COL_A_X + 4, y, 200, 200, 200, 1,
                         "board version   %s", board ? board : "(unknown)");
    } else {
        input_debug_text(COL_A_X + 4, y, 180, 180, 180, 1,
                         "colour variant  n/a  (serial not readable)");
        y += 12;
        input_debug_text(COL_A_X + 4, y, 180, 180, 180, 1,
                         "board version   n/a");
    }

    // ---- Column B: serial / barcodes / voltage / flags ----
    constexpr SLONG COL_B_X = 330;
    y = 72;

    auto render_barcode = [&](const char* label,
                              const std::uint8_t* data, std::size_t len) {
        input_debug_text(COL_B_X, y, 220, 200, 120, 1, "%s", label);
        y += 14;
        if (len == 0) {
            input_debug_text(COL_B_X + 4, y, 180, 180, 180, 1, "(n/a)");
            y += 14;
            return;
        }
        // 32 bytes → 2 rows of 16 bytes each hex.
        char hex[64];
        const std::size_t row1 = len > 16 ? 16 : len;
        hex_dump_line(hex, sizeof(hex), data, row1);
        input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1, "%s", hex);
        y += 12;
        if (len > 16) {
            hex_dump_line(hex, sizeof(hex), data + 16, len - 16);
            input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1, "%s", hex);
            y += 12;
        }
        y += 2;
    };

    render_barcode("Serial number (Shift-JIS raw)",   s_tel.serial,       s_tel.serial_len);
    render_barcode("Assemble parts info",             s_tel.assemble,     s_tel.assemble_len);
    render_barcode("Battery barcode",                 s_tel.batt_barcode, s_tel.batt_barcode_len);
    render_barcode("VCM left barcode (adaptive L)",   s_tel.vcm_left,     s_tel.vcm_left_len);
    render_barcode("VCM right barcode (adaptive R)",  s_tel.vcm_right,    s_tel.vcm_right_len);

    input_debug_text(COL_B_X, y, 220, 200, 120, 1, "Battery voltage (raw)");
    y += 14;
    if (s_tel.batt_v.valid && s_tel.batt_v.len > 0) {
        char hex[32];
        hex_dump_line(hex, sizeof(hex), s_tel.batt_v.data, s_tel.batt_v.len);
        input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1,
                         "%u byte(s)  %s", (unsigned)s_tel.batt_v.len, hex);
    } else {
        input_debug_text(COL_B_X + 4, y, 180, 180, 180, 1, "(n/a)");
    }
    y += 16;

    input_debug_text(COL_B_X, y, 220, 200, 120, 1, "System flags");
    y += 14;
    input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1,
                     "position tracking  %s",
                     s_tel.pos_tracking_ok ? (s_tel.pos_tracking ? "ENABLED" : "disabled") : "n/a");
    y += 12;
    input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1,
                     "always-on startup  %s",
                     s_tel.always_on_ok ? (s_tel.always_on ? "on" : "off") : "n/a");
    y += 12;
    input_debug_text(COL_B_X + 4, y, 200, 200, 200, 1,
                     "auto switchoff     %s",
                     s_tel.auto_switchoff_ok ? (s_tel.auto_switchoff ? "on" : "off") : "n/a");
}

// ===========================================================================
// Sub-page: all output controls (rumble / lightbar / player LED /
// mute LED / haptic volume / lightbar setup / audio volumes)
// ===========================================================================
//
// One vertical column, one shared cursor that walks through every
// interactive row in order. Each widget is a self-contained function
// that returns its row count so the cursor math stays local.

static void render_ds_output()
{
    input_debug_text(20, 48, 255, 255, 255, 1,
                     "DualSense output  (TAB to cycle sub-pages)");

    static int s_cursor = 0;
    const int total_rows =
        RUMBLE_ROWS + LIGHTBAR_ROWS + PLAYER_LED_ROWS + MISC_ROWS + AUDIO_ROWS;

    const InputDebugNav& n = input_debug_nav();
    if (n.up)   s_cursor = (s_cursor - 1 + total_rows) % total_rows;
    if (n.down) s_cursor = (s_cursor + 1) % total_rows;

    int base = 0;
    base += input_debug_render_rumble_test(MENU_X, 80,  s_cursor - base);
    base += render_lightbar               (MENU_X, 142, s_cursor - base);
    base += render_player_led             (MENU_X, 204, s_cursor - base);
    base += render_misc_outputs           (MENU_X, 278, s_cursor - base);
    base += render_audio_outputs          (MENU_X, 316, s_cursor - base);
}

void input_debug_render_dualsense_page()
{
    // Sub-page dispatch. Main layout renders unconditionally; live
    // widgets gate inside the read-through wrapper so a brief switch to
    // keyboard doesn't wipe the page. TAB cycles through VIEW / INPUT /
    // OUTPUT / TRIGGERS / TELEMETRY (handled in input_debug_tick).
    const GamepadState& s = input_debug_read_gamepad_for(INPUT_DEVICE_DUALSENSE);

    switch (s_sub) {
        case DS_SUB_VIEW:      render_ds_view(s);      break;
        case DS_SUB_INPUT:     render_ds_input();      break;
        case DS_SUB_OUTPUT:    render_ds_output();     break;
        case DS_SUB_TRIGGERS:  render_ds_triggers();   break;
        case DS_SUB_TELEMETRY: render_ds_telemetry();  break;
        default: break;
    }
}
