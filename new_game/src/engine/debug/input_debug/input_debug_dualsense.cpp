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

// Sub-pages of the DualSense tab, cycled through by TAB:
//   VIEW     — physical controller layout viz
//   TESTS    — rumble / lightbar / player LED test widgets
//   TRIGGERS — adaptive trigger effect tester (placeholder for now)
// Resets to VIEW on panel open so each session starts with the layout.
enum DualSenseSub {
    DS_SUB_VIEW = 0,
    DS_SUB_TESTS,
    DS_SUB_TRIGGERS,
    DS_SUB_COUNT,
};
namespace { DualSenseSub s_sub = DS_SUB_VIEW; }

void input_debug_dualsense_toggle_sub()
{
    s_sub = (DualSenseSub)(((int)s_sub + 1) % (int)DS_SUB_COUNT);
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
// Sub-page: adaptive trigger effect tester (placeholder)
// ===========================================================================

static void render_ds_triggers()
{
    input_debug_text(20, 48, 255, 255, 255, 1,
                     "Adaptive trigger test  (TAB to cycle sub-pages)");

    input_debug_text(20, 100, 200, 200, 200, 1, "TODO");
    input_debug_text(20, 120, 180, 180, 180, 1,
        "  * Effect selector: Off / Weapon / Feedback / Bow / Vibration / Machine / SimpleFeedback");
    input_debug_text(20, 134, 180, 180, 180, 1,
        "  * Tunable params (start position / end / strength)");
    input_debug_text(20, 148, 180, 180, 180, 1,
        "  * Live apply via ds_trigger_* calls");
    input_debug_text(20, 162, 180, 180, 180, 1,
        "  * Live act / fb readout from feedback status bytes");
    input_debug_text(20, 176, 180, 180, 180, 1,
        "  * Separate left / right trigger effect selection");
}

// ===========================================================================
// Sub-page: rumble / lightbar / player LED tests
// ===========================================================================

static void render_ds_tests()
{
    input_debug_text(20, 48, 255, 255, 255, 1,
                     "DualSense tests  (TAB to cycle sub-pages)");

    // Single cursor walks through all interactive rows: rumble (3) +
    // lightbar (3) + player-LED pairs (3).
    static int s_cursor = 0;
    const int total_rows = RUMBLE_ROWS + LIGHTBAR_ROWS + PLAYER_LED_ROWS;

    const InputDebugNav& n = input_debug_nav();
    if (n.up)   s_cursor = (s_cursor - 1 + total_rows) % total_rows;
    if (n.down) s_cursor = (s_cursor + 1) % total_rows;

    int base = 0;
    base += input_debug_render_rumble_test(MENU_X, 80,  s_cursor - base);
    base += render_lightbar               (MENU_X, 160, s_cursor - base);
    base += render_player_led             (MENU_X, 240, s_cursor - base);
}

void input_debug_render_dualsense_page()
{
    // Sub-page dispatch. Main layout renders unconditionally; live
    // widgets gate inside the read-through wrapper so a brief switch to
    // keyboard doesn't wipe the page. TAB toggles between view and tests
    // (handled in input_debug_tick).
    const GamepadState& s = input_debug_read_gamepad_for(INPUT_DEVICE_DUALSENSE);

    switch (s_sub) {
        case DS_SUB_VIEW:     render_ds_view(s);     break;
        case DS_SUB_TESTS:    render_ds_tests();     break;
        case DS_SUB_TRIGGERS: render_ds_triggers();  break;
        default: break;
    }
}
