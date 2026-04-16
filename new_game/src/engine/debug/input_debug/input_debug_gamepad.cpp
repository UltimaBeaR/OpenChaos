// Input debug panel — Xbox / generic gamepad page.
//
// Shows live stick / trigger / button state using the pre-scrub snapshot.
// Xbox button labels (A/B/X/Y, LB/RB, Back/Start). Same layout as the
// DualSense page (which uses DS labels and adds DS-specific widgets).

#include "engine/debug/input_debug/input_debug.h"

#include "engine/graphics/text/font.h"
#include "engine/input/gamepad.h"
#include "engine/input/gamepad_globals.h"

#include <cstdio>

namespace {

constexpr SLONG STICK_SIZE   = 96;
constexpr SLONG STICK_Y      = 110;
constexpr SLONG LEFT_STICK_X = 100;
constexpr SLONG RIGHT_STICK_X = 540;

constexpr SLONG TRIG_W       = 22;
constexpr SLONG TRIG_H       = 96;
constexpr SLONG TRIG_Y       = 60;
constexpr SLONG LEFT_TRIG_X  = 220;
constexpr SLONG RIGHT_TRIG_X = 418;

constexpr SLONG CONTENT_Y = 48;

// Normalize a raw 0..65535 axis (centre 32768) to [-1..+1].
float norm_axis(int32_t raw)
{
    return ((float)raw - 32768.0f) / 32768.0f;
}

bool btn(const GamepadState& s, int i)
{
    return (s.rgbButtons[i] & 0x80) != 0;
}

} // namespace

void input_debug_render_gamepad_page()
{
    // Only the currently-active input source renders live state — prevents
    // confusion when DualSense presses show up on this page and vice versa.
    if (active_input_device != INPUT_DEVICE_XBOX) {
        FONT_buffer_add(20, CONTENT_Y, 200, 140, 140, 1,
            (CBYTE*)"Xbox / generic gamepad not active.");
        FONT_buffer_add(20, CONTENT_Y + 12, 150, 150, 150, 1,
            (CBYTE*)"Press a button on your Xbox controller — live state will appear here.");
        return;
    }

    const GamepadState& s = gamepad_state_raw();

    FONT_buffer_add(20, CONTENT_Y, 255, 255, 255, 1,
        (CBYTE*)"Gamepad (Xbox / generic)%s",
        s.connected ? "" : "  (disconnected)");

    // Sticks.
    input_debug_draw_stick(LEFT_STICK_X,  STICK_Y + STICK_SIZE / 2, STICK_SIZE,
                           norm_axis(s.lX), norm_axis(s.lY), "Left stick");
    input_debug_draw_stick(RIGHT_STICK_X, STICK_Y + STICK_SIZE / 2, STICK_SIZE,
                           norm_axis(s.rX), norm_axis(s.rY), "Right stick");

    // Triggers (0..255).
    input_debug_draw_trigger_bar(LEFT_TRIG_X,  TRIG_Y, TRIG_W, TRIG_H,
                                 s.trigger_left,  255, "LT");
    input_debug_draw_trigger_bar(RIGHT_TRIG_X, TRIG_Y, TRIG_W, TRIG_H,
                                 s.trigger_right, 255, "RT");

    // Buttons — centre column. Two rows of 4 face buttons on top, then a
    // row of shoulders / menu keys, then stick clicks, then D-Pad.
    constexpr SLONG BX = 290;   // buttons block X
    constexpr SLONG BY = 70;    // buttons block Y (first row)
    constexpr SLONG BW = 28;    // horizontal pitch
    constexpr SLONG BH = 12;    // vertical pitch

    // Face buttons (A/B/X/Y).
    input_debug_draw_button(BX + BW * 0, BY + BH * 0, "A", btn(s, 0));
    input_debug_draw_button(BX + BW * 1, BY + BH * 0, "B", btn(s, 1));
    input_debug_draw_button(BX + BW * 2, BY + BH * 0, "X", btn(s, 2));
    input_debug_draw_button(BX + BW * 3, BY + BH * 0, "Y", btn(s, 3));

    // Shoulders + menu.
    input_debug_draw_button(BX + BW * 0, BY + BH * 2, "LB",     btn(s, 9));
    input_debug_draw_button(BX + BW * 1, BY + BH * 2, "RB",     btn(s, 10));
    input_debug_draw_button(BX + BW * 2, BY + BH * 2, "Back",   btn(s, 4));
    input_debug_draw_button(BX + BW * 3 + 10, BY + BH * 2, "Start", btn(s, 6));

    // Stick clicks.
    input_debug_draw_button(BX + BW * 0, BY + BH * 4, "LS", btn(s, 7));
    input_debug_draw_button(BX + BW * 1, BY + BH * 4, "RS", btn(s, 8));

    // D-Pad — arrow glyphs as letters since we use the bitmap font.
    input_debug_draw_button(BX + BW * 0, BY + BH * 6, "U", btn(s, 11));
    input_debug_draw_button(BX + BW * 1, BY + BH * 6, "D", btn(s, 12));
    input_debug_draw_button(BX + BW * 2, BY + BH * 6, "L", btn(s, 13));
    input_debug_draw_button(BX + BW * 3, BY + BH * 6, "R", btn(s, 14));

    // Raw numeric read-out — helps verify the visualization.
    char buf[128];
    std::snprintf(buf, sizeof(buf),
        "lX=%d  lY=%d  rX=%d  rY=%d  LT=%u  RT=%u",
        s.lX, s.lY, s.rX, s.rY, s.trigger_left, s.trigger_right);
    FONT_buffer_add(20, 230, 150, 150, 180, 1, (CBYTE*)"%s", buf);
}
