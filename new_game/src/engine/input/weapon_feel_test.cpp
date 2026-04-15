// Weapon feel — hardware motor delay test (temporary, 2026-04-15)
//
// Dead-simple observation mode:
//   - '\' toggles the test on/off.
//   - While on: the character does not actually fire, and the DualSense
//     Weapon25 effect is held continuously on. The player taps R2 at their
//     own pace, varying speed from slow to rapid, and feels by ear/hand
//     which taps produced a physical click and which did not.
//   - The on-screen status line shows live timing measurements so the
//     player can correlate their tap cadence with dropped clicks:
//         "RELEASED Xms  |  last gap YYYms"
//     where RELEASED is how long R2 has currently been below the release
//     threshold, and "last gap" is the interval between the two most
//     recent R2 presses (press-to-press cadence).
//
// No keys to press, no timed prompts, no reaction pressure. The player is
// the instrument; the test just gets out of the way and displays numbers.

#include "engine/input/weapon_feel_test.h"
#include "engine/input/weapon_feel.h"
#include "engine/input/gamepad.h"
#include "engine/input/gamepad_globals.h"
#include "engine/input/keyboard.h"
#include "engine/input/keyboard_globals.h"
#include "engine/console/console.h"
#include "engine/graphics/text/font.h"

#include <chrono>
#include <cstdio>
#include <cstring>

namespace {

constexpr int k_reset_threshold = 80;
constexpr int k_fire_threshold  = 200;
// Approximate Weapon25 click point in trigger-range units. Zone 7 out
// of 15 → roughly (7/15)*255 ≈ 119. This is where the physical click
// mechanically happens — crossing it upward = click fires (if armed),
// dropping below it = hardware rearms. Used by the gap counter so
// registration matches what the player actually feels.
constexpr int k_click_point   = 119;

using clk = std::chrono::steady_clock;

bool s_test_on = false;

// Live state so we can compute release-duration and press gap.
bool  s_was_released = true;
bool  s_was_pressed  = false;
clk::time_point s_release_time;
clk::time_point s_last_press_time;
bool  s_had_press    = false;
long long s_last_gap_ms = -1; // interval between last two presses

// Only register a new press if the trigger was fully released
// (R2 ≤ reset_threshold) since the previous press. Otherwise a
// partial re-press that never lifted high enough to rearm the
// hardware click would still be counted, polluting the history.
bool s_released_since_press = true;

// Running history of all press gaps (ms) since the test was started. Drawn
// on-screen as small text so the player can read off the minimum gap at
// which a click still fires.
constexpr int k_max_gaps = 500;
int  s_gaps[k_max_gaps];
int  s_gap_count = 0;

// Click-point calibration samples. SPACE records the current R2 value,
// which the player sets by slowly pulling the trigger, stopping immediately
// after feeling the click, and holding still before pressing SPACE.
constexpr int k_max_click_samples = 64;
int  s_click_samples[k_max_click_samples];
int  s_click_sample_count = 0;

void set_status(const char* text)
{
    CONSOLE_status((CBYTE*)text);
}

void refresh_status()
{
    char buf[160];
    const auto now = clk::now();

    long long rel_ms = -1;
    if (s_was_released) {
        rel_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - s_release_time).count();
    }

    if (s_last_gap_ms >= 0 && rel_ms >= 0) {
        std::snprintf(buf, sizeof(buf),
            "TEST ON  released %lldms  last gap %lldms  (\\ exits)",
            rel_ms, s_last_gap_ms);
    } else if (s_last_gap_ms >= 0) {
        std::snprintf(buf, sizeof(buf),
            "TEST ON  PRESSED  last gap %lldms  (\\ exits)",
            s_last_gap_ms);
    } else if (rel_ms >= 0) {
        std::snprintf(buf, sizeof(buf),
            "TEST ON  released %lldms  (tap R2 to start)  (\\ exits)",
            rel_ms);
    } else {
        std::snprintf(buf, sizeof(buf),
            "TEST ON  PRESSED  (release R2)  (\\ exits)");
    }
    set_status(buf);
}

void enter_test()
{
    s_test_on = true;
    s_had_press = false;
    s_last_gap_ms = -1;
    s_was_released = true;
    s_was_pressed = false;
    s_released_since_press = true;
    s_release_time = clk::now();
    s_gap_count = 0;
    s_click_sample_count = 0;
    weapon_feel_debug_log("=== TEST ON (observation mode) ===");
    // Force mode to AIM_GUN unconditionally while test is active. Normal
    // gamepad_triggers_update is bypassed via weapon_feel_test_is_active.
    gamepad_test_set_trigger_mode(1 /* AIM_GUN */);
    refresh_status();
}

void leave_test()
{
    s_test_on = false;
    weapon_feel_debug_log("=== TEST OFF ===");
    CONSOLE_status((CBYTE*)"");
}

} // namespace

bool weapon_feel_test_is_active()
{
    return s_test_on;
}

void weapon_feel_test_tick()
{
    if (Keys[KB_BACKSLASH]) {
        Keys[KB_BACKSLASH] = 0;
        if (s_test_on) leave_test();
        else           enter_test();
        return;
    }

    if (!s_test_on) return;

    // SPACE — sample current r2 as a click-point calibration point. The
    // player stops the trigger immediately after feeling a click and
    // presses SPACE while holding it stationary, so reaction time does
    // not contaminate the measurement.
    if (Keys[KB_SPACE]) {
        Keys[KB_SPACE] = 0;
        if (s_click_sample_count < k_max_click_samples) {
            const int v = gamepad_state.trigger_right;
            s_click_samples[s_click_sample_count++] = v;
            weapon_feel_debug_log("TEST click sample #%d r2=%d",
                s_click_sample_count, v);
        }
    }

    // Keep AIM_GUN continuously asserted in case any other path ever
    // overwrote it. Cheap — apply_trigger_mode inside short-circuits on
    // no-op but the explicit re-call is still a HID write, so guard it
    // with a "once per tick only if changed" cache.
    // (Current gamepad_test_set_trigger_mode does not no-op check; calling
    // it every tick would flood the HID pipe. Only re-assert on toggle.)

    const int r2 = gamepad_state.trigger_right;
    const auto now = clk::now();

    const bool released = r2 <= k_reset_threshold;
    // "pressed" here means "crossed the physical click point upward" —
    // the moment the hardware would emit a click (if armed). Using the
    // click point instead of fire_threshold keeps count events aligned
    // with what the player actually feels in the trigger.
    const bool pressed  = r2 >  k_click_point;

    // Rising edge of "released" — R2 just dropped below reset.
    if (released && !s_was_released) {
        s_release_time = now;
    }
    // Hardware rearms the Weapon25 click whenever the trigger drops back
    // below the click point. Match that here: as soon as r2 is at or
    // below click_point, we consider the click "armable" again.
    if (r2 <= k_click_point) {
        s_released_since_press = true;
    }
    // Rising edge of "pressed" — R2 just crossed fire_threshold — AND we
    // have had a full release since the previous press.
    if (pressed && !s_was_pressed && s_released_since_press) {
        if (s_had_press) {
            s_last_gap_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - s_last_press_time).count();
            if (s_gap_count < k_max_gaps) {
                s_gaps[s_gap_count++] = (int)s_last_gap_ms;
            }
        }
        s_last_press_time = now;
        s_had_press = true;
        s_released_since_press = false;
        weapon_feel_debug_log("TEST press r2=%d gap=%lldms", r2, s_last_gap_ms);
    }

    s_was_released = released;
    s_was_pressed  = pressed;

    refresh_status();

    // Draw running gap history on the left side, small pixel font, multi
    // column if the list grows long. Re-queued every frame because the
    // FONT_buffer is cleared each render.
    constexpr int col_x    = 10;
    constexpr int col_y    = 100;
    constexpr int line_h   = 8;
    constexpr int col_w    = 40;    // px between columns
    constexpr int max_lines_per_col = 40;

    char lbuf[32];
    for (int i = 0; i < s_gap_count; i++) {
        const int col  = i / max_lines_per_col;
        const int line = i % max_lines_per_col;
        const int x    = col_x + col * col_w;
        const int y    = col_y + line * line_h;
        std::snprintf(lbuf, sizeof(lbuf), "%d", s_gaps[i]);
        FONT_buffer_add(x, y, 255, 255, 255, 1, (CBYTE*)"%s", lbuf);
    }

    // Live big r2 value near the top-right area — used during click-point
    // calibration. Player watches this while slowly pressing the trigger.
    std::snprintf(lbuf, sizeof(lbuf), "r2=%d", r2);
    FONT_buffer_add(300, col_y, 255, 255, 0, 1, (CBYTE*)"%s", lbuf);
    FONT_buffer_add(300, col_y + line_h, 200, 200, 200, 1,
        (CBYTE*)"SPACE=sample click");

    // Live trigger feedback state straight from the controller. State 1 =
    // currently inside the active effect zone; rising edge of state to 1
    // is the moment the hardware physically clicked.
    const uint8_t fb = gamepad_get_right_trigger_feedback_state();
    const bool    act = gamepad_get_right_trigger_effect_active();
    std::snprintf(lbuf, sizeof(lbuf), "fb=%u act=%d", (unsigned)fb, act ? 1 : 0);
    FONT_buffer_add(300, col_y + line_h * 2, 0, 255, 255, 1, (CBYTE*)"%s", lbuf);

    // Click sample list + running average, rightmost column.
    int sum = 0;
    for (int i = 0; i < s_click_sample_count; i++) sum += s_click_samples[i];
    const int avg = s_click_sample_count > 0 ? sum / s_click_sample_count : 0;
    std::snprintf(lbuf, sizeof(lbuf), "click samples avg=%d", avg);
    FONT_buffer_add(300, col_y + line_h * 3, 255, 180, 0, 1,
        (CBYTE*)"%s", lbuf);
    for (int i = 0; i < s_click_sample_count; i++) {
        std::snprintf(lbuf, sizeof(lbuf), "%d", s_click_samples[i]);
        FONT_buffer_add(300, col_y + line_h * (4 + i),
            255, 180, 0, 1, (CBYTE*)"%s", lbuf);
    }
}
