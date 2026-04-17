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

#include <algorithm>
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
                "%s(x) %s", prefix, PLAYER_LED_NAMES[row]);
        } else {
            input_debug_text(x + 4, y + 14 + row * 12,
                selected ? 255 : 150,
                selected ? 255 : 150,
                selected ? 120 : 150, 1,
                "%s( ) %s", prefix, PLAYER_LED_NAMES[row]);
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
namespace {
DualSenseSub s_sub = DS_SUB_VIEW;
// Trigger tester state flags lifted up so toggle_sub / reset_state (which
// sit above the full trigger-tester namespace block further down) can
// poke them. Full tester state (TriggerTesterState s_trig, params) lives
// in its own block near render_ds_triggers.
bool s_trig_dirty       = false;
bool s_trig_first_frame = true;
}

void input_debug_dualsense_toggle_sub()
{
    const DualSenseSub prev = s_sub;
    s_sub = (DualSenseSub)(((int)s_sub + 1) % (int)DS_SUB_COUNT);

    // Leaving the trigger tester — silence both triggers so the user
    // doesn't feel residual resistance on the VIEW / TESTS pages.
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

// Column-style read-only block for one trigger. Renders position /
// feedback state / act / raw 10-byte slot dump.
void trig_draw_indicators(SLONG x, SLONG y, bool right, int analog_value,
                          const std::uint8_t slot[10])
{
    const char* side = right ? "R2" : "L2";
    input_debug_text(x, y, 220, 200, 120, 1, "%s readout", side);

    input_debug_text(x + 4, y + 14, 200, 200, 200, 1,
                     "position %3d", analog_value);

    const uint8_t fb = input_debug_read_ds_feedback(right);
    input_debug_text(x + 4, y + 26, 200, 200, 200, 1,
                     "fb state %3u", (unsigned)fb);

    const bool act = input_debug_read_ds_effect_active(right);
    input_debug_draw_checkbox(x + 4, y + 38, "act", act);

    input_debug_text(x + 4, y + 50, 160, 160, 160, 1,
                     "slot %02X %02X %02X %02X %02X",
                     slot[0], slot[1], slot[2], slot[3], slot[4]);
    input_debug_text(x + 4, y + 62, 160, 160, 160, 1,
                     "     %02X %02X %02X %02X %02X",
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
