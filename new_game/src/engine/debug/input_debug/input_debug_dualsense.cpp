// Input debug panel — DualSense page.
//
// Iteration 2: same base widgets as the Gamepad page (sticks, triggers,
// buttons) with DualSense labels. DS-specific sections (touchpad XY
// visualisation, trigger feedback status + act checkbox, adaptive-trigger
// effect cycle with tunable parameters, lightbar / player LED tests,
// rumble test) are marked with TODO placeholders here and added in
// later iterations.

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

float norm_axis(int32_t raw)
{
    return ((float)raw - 32768.0f) / 32768.0f;
}

bool btn(const GamepadState& s, int i)
{
    return (s.rgbButtons[i] & 0x80) != 0;
}

} // namespace

void input_debug_render_dualsense_page()
{
    // Layout renders unconditionally; live widgets gate inside the
    // read-through wrapper so a brief switch to keyboard doesn't wipe
    // the page.
    const GamepadState& s = input_debug_read_gamepad_for(INPUT_DEVICE_DUALSENSE);
    const bool is_active = active_input_device == INPUT_DEVICE_DUALSENSE;

    FONT_buffer_add(20, CONTENT_Y, 255, 255, 255, 1,
        (CBYTE*)"DualSense");
    if (is_active) {
        FONT_buffer_add(140, CONTENT_Y, 0, 255, 0, 1, (CBYTE*)"[ACTIVE]");
    } else {
        FONT_buffer_add(140, CONTENT_Y, 200, 140, 140, 1, (CBYTE*)"[inactive — press a button on DualSense]");
    }

    // Sticks.
    input_debug_draw_stick(LEFT_STICK_X,  STICK_Y + STICK_SIZE / 2, STICK_SIZE,
                           norm_axis(s.lX), norm_axis(s.lY), "Left stick");
    input_debug_draw_stick(RIGHT_STICK_X, STICK_Y + STICK_SIZE / 2, STICK_SIZE,
                           norm_axis(s.rX), norm_axis(s.rY), "Right stick");

    // Triggers.
    input_debug_draw_trigger_bar(LEFT_TRIG_X,  TRIG_Y, TRIG_W, TRIG_H,
                                 s.trigger_left,  255, "L2");
    input_debug_draw_trigger_bar(RIGHT_TRIG_X, TRIG_Y, TRIG_W, TRIG_H,
                                 s.trigger_right, 255, "R2");

    // Buttons.
    constexpr SLONG BX = 290;
    constexpr SLONG BY = 70;
    constexpr SLONG BW = 38;
    constexpr SLONG BH = 12;

    // Face buttons (PlayStation labels).
    input_debug_draw_button(BX + BW * 0, BY + BH * 0, "Cross",    btn(s, 0));
    input_debug_draw_button(BX + BW * 1, BY + BH * 0, "Circle",   btn(s, 1));
    input_debug_draw_button(BX + BW * 2, BY + BH * 0, "Square",   btn(s, 2));
    input_debug_draw_button(BX + BW * 3, BY + BH * 0, "Triangle", btn(s, 3));

    // Shoulders + menu.
    input_debug_draw_button(BX + BW * 0, BY + BH * 2, "L1",      btn(s, 9));
    input_debug_draw_button(BX + BW * 1, BY + BH * 2, "R1",      btn(s, 10));
    input_debug_draw_button(BX + BW * 2, BY + BH * 2, "Share",   btn(s, 4));
    input_debug_draw_button(BX + BW * 3, BY + BH * 2, "Options", btn(s, 6));

    // Stick clicks + PS / mute.
    input_debug_draw_button(BX + BW * 0, BY + BH * 4, "L3",   btn(s, 7));
    input_debug_draw_button(BX + BW * 1, BY + BH * 4, "R3",   btn(s, 8));
    input_debug_draw_button(BX + BW * 2, BY + BH * 4, "PS",   btn(s, 5));
    input_debug_draw_button(BX + BW * 3, BY + BH * 4, "Mute", btn(s, 18));

    // D-Pad.
    input_debug_draw_button(BX + BW * 0, BY + BH * 6, "U", btn(s, 11));
    input_debug_draw_button(BX + BW * 1, BY + BH * 6, "D", btn(s, 12));
    input_debug_draw_button(BX + BW * 2, BY + BH * 6, "L", btn(s, 13));
    input_debug_draw_button(BX + BW * 3, BY + BH * 6, "R", btn(s, 14));

    // Touchpad click (digital only at this iteration).
    input_debug_draw_button(BX + BW * 0, BY + BH * 8, "Touchpad click", btn(s, 17));

    // Raw numeric read-out.
    char buf[128];
    std::snprintf(buf, sizeof(buf),
        "lX=%d  lY=%d  rX=%d  rY=%d  L2=%u  R2=%u",
        s.lX, s.lY, s.rX, s.rY, s.trigger_left, s.trigger_right);
    FONT_buffer_add(20, 230, 150, 150, 180, 1, (CBYTE*)"%s", buf);

    // TODO placeholders for DualSense-specific widgets.
    constexpr SLONG TODO_Y = 260;
    FONT_buffer_add(20, TODO_Y,          220, 180, 100, 1,
        (CBYTE*)"TODO (next iterations):");
    FONT_buffer_add(30, TODO_Y + 12,     180, 180, 180, 1,
        (CBYTE*)"- Touchpad XY finger positions (needs parsing in libDualsense)");
    FONT_buffer_add(30, TODO_Y + 24,     180, 180, 180, 1,
        (CBYTE*)"- Trigger feedback status + act bit indicator (L2 and R2)");
    FONT_buffer_add(30, TODO_Y + 36,     180, 180, 180, 1,
        (CBYTE*)"- Adaptive-trigger effect cycle with tunable parameters");
    FONT_buffer_add(30, TODO_Y + 48,     180, 180, 180, 1,
        (CBYTE*)"- Rumble test (both motors, tunable amplitudes)");
    FONT_buffer_add(30, TODO_Y + 60,     180, 180, 180, 1,
        (CBYTE*)"- Lightbar RGB test + player LED bar test");
}
