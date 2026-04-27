// Input debug panel — Xbox / generic gamepad page.
//
// Two sub-pages cycled via TAB:
//   VIEW  — physical controller layout viz (mirrors the DualSense page
//           so switching tabs doesn't visually shuffle sticks / triggers)
//   TESTS — rumble test
//
// Xbox button labels: A/B/X/Y face diamond, U/D/L/R d-pad, LB/RB
// shoulders, LT/RT triggers, Back/Start centre buttons, Xbox/Guide top
// centre. D-pad and left stick are rendered in DS order (d-pad under the
// stick column) rather than the physical Xbox layout — user preference
// since "Xbox-compatible" controllers vary in that layout.

#include "engine/debug/input_debug/input_debug.h"

#include "engine/input/gamepad.h"

namespace {

// Normalize a raw 0..65535 axis (centre 32768) to [-1..+1].
float norm_axis(int32_t raw)
{
    return ((float)raw - 32768.0f) / 32768.0f;
}

bool btn(const GamepadState& s, int i)
{
    return (s.rgbButtons[i] & 0x80) != 0;
}

enum XboxSub {
    XBOX_SUB_VIEW = 0,
    XBOX_SUB_TESTS,
    XBOX_SUB_COUNT,
};
XboxSub s_sub = XBOX_SUB_VIEW;

// ---------------------------------------------------------------------------
// VIEW — controller layout viz
// ---------------------------------------------------------------------------
//
// Mirrors the DualSense VIEW positions (LSTK_CX=160, RSTK_CX=480, trigger
// bars at x=200/418, etc.) so the user sees the same anchor points on
// both tabs. Label differences only — Xbox has Back / Start / Guide
// where DS has Share / Options / PS, no Mute equivalent, and no
// touchpad; the Xbox (Guide) button occupies the top-centre spot the
// DualSense touchpad sits in.

void render_xbox_view(const GamepadState& s)
{
    input_debug_text(20, 48, 255, 255, 255, 1, "Gamepad (Xbox / generic)");

    constexpr SLONG L_TRIG_X = 200;
    constexpr SLONG R_TRIG_X = 418;
    constexpr SLONG TR_Y = 70;
    constexpr SLONG TR_W = 22;
    constexpr SLONG TR_H = 96;
    constexpr SLONG LSTK_CX = 160;
    constexpr SLONG RSTK_CX = 480;

    // LB / RB centred over their bars.
    input_debug_draw_button(L_TRIG_X + TR_W / 2 - 6, 55, "LB", btn(s, 9));
    input_debug_draw_button(R_TRIG_X + TR_W / 2 - 6, 55, "RB", btn(s, 10));

    input_debug_draw_trigger_bar(L_TRIG_X, TR_Y, TR_W, TR_H,
        s.trigger_left, 255, "LT");
    input_debug_draw_trigger_bar(R_TRIG_X, TR_Y, TR_W, TR_H,
        s.trigger_right, 255, "RT");

    // Xbox (Guide) button at top-centre — where the DS touchpad lives.
    // SDL gamepad Guide maps to rgbButtons[5] on Xbox controllers.
    input_debug_draw_button(320 - 12, 100, "Xbox", btn(s, 5));

    // Back / Start flank the screen centre, well below the Xbox button
    // at the top. On the physical Xbox controller these are near the
    // centre of the body (just below the Xbox/Guide logo), so place
    // them at mid-screen rather than over the stick columns like the
    // DualSense Share / Options. "Back" = 24 px, "Start" = 30 px wide.
    input_debug_draw_button(278, 240, "Back", btn(s, 4));
    input_debug_draw_button(335, 240, "Start", btn(s, 6));

    // D-pad diamond centred on the left stick column.
    constexpr SLONG DPAD_CY = 235;
    input_debug_draw_button(LSTK_CX - 3, DPAD_CY - 15, "U", btn(s, 11));
    input_debug_draw_button(LSTK_CX - 15, DPAD_CY, "L", btn(s, 13));
    input_debug_draw_button(LSTK_CX + 9, DPAD_CY, "R", btn(s, 14));
    input_debug_draw_button(LSTK_CX - 3, DPAD_CY + 15, "D", btn(s, 12));

    // Face buttons diamond centred on the right stick column
    // (Y top, X left, B right, A bottom — standard Xbox).
    constexpr SLONG FACE_CY = 235;
    input_debug_draw_button(RSTK_CX - 3, FACE_CY - 15, "Y", btn(s, 3));
    input_debug_draw_button(RSTK_CX - 15, FACE_CY, "X", btn(s, 2));
    input_debug_draw_button(RSTK_CX + 9, FACE_CY, "B", btn(s, 1));
    input_debug_draw_button(RSTK_CX - 3, FACE_CY + 15, "A", btn(s, 0));

    // Sticks with LS / RS click indicators and numeric readouts — same
    // positions as the DualSense VIEW sub-page.
    constexpr SLONG STK_SIZE = 96;
    constexpr SLONG STK_CY = 330;
    constexpr SLONG STK_TITLE_Y = STK_CY - STK_SIZE / 2 - 12;
    constexpr SLONG STK_READ_Y = STK_CY + STK_SIZE / 2 + 8;

    input_debug_draw_stick(LSTK_CX, STK_CY, STK_SIZE,
        norm_axis(s.lX), norm_axis(s.lY), "Left stick");
    input_debug_draw_button(LSTK_CX + 34, STK_TITLE_Y, "LS", btn(s, 7));
    input_debug_text(LSTK_CX - STK_SIZE / 2, STK_READ_Y, 180, 180, 180, 1,
        "X=%5d  Y=%5d", s.lX, s.lY);

    input_debug_draw_stick(RSTK_CX, STK_CY, STK_SIZE,
        norm_axis(s.rX), norm_axis(s.rY), "Right stick");
    input_debug_draw_button(RSTK_CX + 37, STK_TITLE_Y, "RS", btn(s, 8));
    input_debug_text(RSTK_CX - STK_SIZE / 2, STK_READ_Y, 180, 180, 180, 1,
        "X=%5d  Y=%5d", s.rX, s.rY);
}

// ---------------------------------------------------------------------------
// TESTS — rumble only (Xbox has no lightbar or player LEDs)
// ---------------------------------------------------------------------------

void render_xbox_tests()
{
    input_debug_text(20, 48, 255, 255, 255, 1,
        "Xbox rumble test  (TAB to return)");

    static int s_cursor = 0;
    const InputDebugNav& n = input_debug_nav();
    const int total = 3;
    if (n.up)
        s_cursor = (s_cursor - 1 + total) % total;
    if (n.down)
        s_cursor = (s_cursor + 1) % total;

    input_debug_render_rumble_test(20, 80, s_cursor);
}

} // namespace

void input_debug_gamepad_toggle_sub()
{
    s_sub = (XboxSub)(((int)s_sub + 1) % (int)XBOX_SUB_COUNT);
}

void input_debug_gamepad_reset_sub()
{
    s_sub = XBOX_SUB_VIEW;
}

void input_debug_render_gamepad_page()
{
    const GamepadState& s = input_debug_read_gamepad_for(INPUT_DEVICE_XBOX);

    switch (s_sub) {
    case XBOX_SUB_VIEW:
        render_xbox_view(s);
        break;
    case XBOX_SUB_TESTS:
        render_xbox_tests();
        break;
    default:
        break;
    }
}
