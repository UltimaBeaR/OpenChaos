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

#include <cstdio>

namespace {

// ---------------------------------------------------------------------------
// Layout constants — logical 640×480 coordinate space (rects scale with
// pipeline; text scales via input_debug_text helper).
// ---------------------------------------------------------------------------

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

// Touchpad visualisation box — hardware reports 1920×1080, scaled down
// roughly 1:10 here. Positioned in the left column below the stick row.
constexpr SLONG TP_X = 20;
constexpr SLONG TP_Y = 180;
constexpr SLONG TP_W = 160;
constexpr SLONG TP_H = 64;

constexpr SLONG RAW_Y = 255;

// Menu section — rumble / lightbar / player LED stacks under the raw
// numeric row. Each widget has a title row + value rows, ~12 px/row.
constexpr SLONG MENU_X      = 20;
constexpr SLONG RUMBLE_Y    = 275;
constexpr SLONG LIGHTBAR_Y  = 335;
constexpr SLONG PLAYER_LED_Y = 395;

constexpr int RUMBLE_ROWS     = 3;
constexpr int LIGHTBAR_ROWS   = 3;
// DualSense player LED byte has 5 bits but hardware pairs them
// symmetrically (bits 0 & 4 mirror — outer pair; bits 1 & 3 mirror —
// inner pair; bit 2 is the lone center LED). So we expose 3 logical
// rows, one per pair, matching the PlayerLed::{Outer,Inner,Center}
// constants in libDualsense.
constexpr int PLAYER_LED_ROWS = 3;

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
    AENG_draw_rect(x, y, w, h, 0x202020, 11, POLY_PAGE_COLOUR);

    // Read finger state (zeroed when DualSense not active).
    int f1_x = 0, f1_y = 0, f2_x = 0, f2_y = 0;
    bool f1_down = false, f2_down = false;
    input_debug_read_ds_touchpad(&f1_x, &f1_y, &f1_down, &f2_x, &f2_y, &f2_down);

    // Map hardware pixel (0..1919, 0..1079) to widget pixel.
    auto to_wx = [&](int hx) { return x + (hx * w) / 1919; };
    auto to_wy = [&](int hy) { return y + (hy * h) / 1079; };

    if (f1_down) {
        AENG_draw_rect(to_wx(f1_x) - 2, to_wy(f1_y) - 2, 4, 4,
                       0x30FF30, 12, POLY_PAGE_COLOUR);
    }
    if (f2_down) {
        AENG_draw_rect(to_wx(f2_x) - 2, to_wy(f2_y) - 2, 4, 4,
                       0xFF30FF, 12, POLY_PAGE_COLOUR);
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
// Player LED widget (5 toggle rows)
// ---------------------------------------------------------------------------

// Three logical rows — outer pair, inner pair, center single. Each row's
// bitmask comes straight from PlayerLed:: constants in libDualsense.
constexpr uint8_t PLAYER_LED_BITS[PLAYER_LED_ROWS] = {
    0x01 | 0x10,  // Outer pair  (bits 0 and 4)
    0x02 | 0x08,  // Inner pair  (bits 1 and 3)
    0x04,         // Center      (bit 2)
};
static const char* PLAYER_LED_NAMES[PLAYER_LED_ROWS] = {
    "Outer pair (LED 1 & 5)",
    "Inner pair (LED 2 & 4)",
    "Center (LED 3)",
};

uint8_t s_player_led_mask      = 0;
int     s_player_led_last_mask = -1;

int render_player_led(SLONG x, SLONG y, int local_cursor)
{
    input_debug_text(x, y, 220, 200, 120, 1, "Player LEDs (live)");

    if (local_cursor >= 0 && local_cursor < PLAYER_LED_ROWS) {
        const InputDebugNav& n = input_debug_nav();
        if (n.enter) {
            s_player_led_mask ^= PLAYER_LED_BITS[local_cursor];
        }
    }

    if ((int)s_player_led_mask != s_player_led_last_mask) {
        ds_set_player_led(s_player_led_mask);
        ds_update_output();
        s_player_led_last_mask = s_player_led_mask;
    }

    for (int row = 0; row < PLAYER_LED_ROWS; row++) {
        const bool selected = (row == local_cursor);
        // Pair is considered "on" when any of its bits is set — since
        // the firmware mirrors, toggling the pair flips all its bits
        // together so all-on and all-off are the only states reached.
        const bool on = (s_player_led_mask & PLAYER_LED_BITS[row]) != 0;
        const char* prefix = selected ? "> " : "  ";
        if (on) {
            input_debug_text(x + 4, y + 14 + row * 12,
                selected ? 120 : 80, 255, selected ? 120 : 80, 1,
                "%s[x] %s", prefix, PLAYER_LED_NAMES[row]);
        } else {
            input_debug_text(x + 4, y + 14 + row * 12,
                selected ? 255 : 150,
                selected ? 255 : 150,
                selected ? 120 : 150, 1,
                "%s[ ] %s", prefix, PLAYER_LED_NAMES[row]);
        }
    }

    return PLAYER_LED_ROWS;
}

} // namespace

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

    // Send the clean state now so there's no lingering user-set LED
    // visible between panel close and the next game-tick refresh.
    ds_set_player_led(0);
    ds_update_output();
}

void input_debug_render_dualsense_page()
{
    // Layout renders unconditionally; live widgets gate inside the
    // read-through wrapper so a brief switch to keyboard doesn't wipe
    // the page. Active/inactive indication is on the tab up top.
    const GamepadState& s = input_debug_read_gamepad_for(INPUT_DEVICE_DUALSENSE);

    input_debug_text(20, CONTENT_Y, 255, 255, 255, 1, "DualSense");

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

    // DualSense feedback indicators — act bit + raw feedback-state nibble.
    const bool  l2_act = input_debug_read_ds_effect_active(false);
    const bool  r2_act = input_debug_read_ds_effect_active(true);
    const uint8_t l2_fb = input_debug_read_ds_feedback(false);
    const uint8_t r2_fb = input_debug_read_ds_feedback(true);
    input_debug_draw_checkbox(LEFT_TRIG_X  + TRIG_W + 6, TRIG_Y,      "act", l2_act);
    input_debug_text(         LEFT_TRIG_X  + TRIG_W + 6, TRIG_Y + 12, 180, 180, 200, 1,
                              "fb=%u", (unsigned)l2_fb);
    input_debug_draw_checkbox(RIGHT_TRIG_X + TRIG_W + 6, TRIG_Y,      "act", r2_act);
    input_debug_text(         RIGHT_TRIG_X + TRIG_W + 6, TRIG_Y + 12, 180, 180, 200, 1,
                              "fb=%u", (unsigned)r2_fb);

    // Buttons block — PS labels, same layout as gamepad page but
    // with DS-specific button names + touchpad click / mute.
    constexpr SLONG BX = 290;
    constexpr SLONG BY = 70;
    constexpr SLONG BW = 38;
    constexpr SLONG BH = 12;
    input_debug_draw_button(BX + BW * 0, BY + BH * 0, "Cross",    btn(s, 0));
    input_debug_draw_button(BX + BW * 1, BY + BH * 0, "Circle",   btn(s, 1));
    input_debug_draw_button(BX + BW * 2, BY + BH * 0, "Square",   btn(s, 2));
    input_debug_draw_button(BX + BW * 3, BY + BH * 0, "Triangle", btn(s, 3));
    input_debug_draw_button(BX + BW * 0, BY + BH * 2, "L1",      btn(s, 9));
    input_debug_draw_button(BX + BW * 1, BY + BH * 2, "R1",      btn(s, 10));
    input_debug_draw_button(BX + BW * 2, BY + BH * 2, "Share",   btn(s, 4));
    input_debug_draw_button(BX + BW * 3, BY + BH * 2, "Options", btn(s, 6));
    input_debug_draw_button(BX + BW * 0, BY + BH * 4, "L3",   btn(s, 7));
    input_debug_draw_button(BX + BW * 1, BY + BH * 4, "R3",   btn(s, 8));
    input_debug_draw_button(BX + BW * 2, BY + BH * 4, "PS",   btn(s, 5));
    input_debug_draw_button(BX + BW * 3, BY + BH * 4, "Mute", btn(s, 18));
    input_debug_draw_button(BX + BW * 0, BY + BH * 6, "U", btn(s, 11));
    input_debug_draw_button(BX + BW * 1, BY + BH * 6, "D", btn(s, 12));
    input_debug_draw_button(BX + BW * 2, BY + BH * 6, "L", btn(s, 13));
    input_debug_draw_button(BX + BW * 3, BY + BH * 6, "R", btn(s, 14));
    input_debug_draw_button(BX + BW * 0, BY + BH * 8, "Touchpad click", btn(s, 17));

    // Touchpad XY finger visualisation.
    draw_touchpad_viz(TP_X, TP_Y, TP_W, TP_H);

    // Raw numeric read-out.
    input_debug_text(20, RAW_Y, 150, 150, 180, 1,
        "lX=%d  lY=%d  rX=%d  rY=%d  L2=%u  R2=%u",
        s.lX, s.lY, s.rX, s.rY, s.trigger_left, s.trigger_right);

    // ----- Menu: rumble + lightbar + player LEDs -----
    // A single page-local cursor walks through all interactive rows in
    // order. Widgets receive `local_cursor` = page cursor minus their
    // starting row offset, so only one widget at a time is focused.
    static int s_cursor = 0;
    const int total_rows = RUMBLE_ROWS + LIGHTBAR_ROWS + PLAYER_LED_ROWS;

    const InputDebugNav& n = input_debug_nav();
    if (n.up)   s_cursor = (s_cursor - 1 + total_rows) % total_rows;
    if (n.down) s_cursor = (s_cursor + 1) % total_rows;

    int base = 0;
    base += input_debug_render_rumble_test(MENU_X, RUMBLE_Y,    s_cursor - base);
    base += render_lightbar               (MENU_X, LIGHTBAR_Y,  s_cursor - base);
    base += render_player_led             (MENU_X, PLAYER_LED_Y, s_cursor - base);
}
