// Modal input debug panel — core, page dispatcher, widget helpers.
// See input_debug.h. Page content lives in input_debug_<page>.cpp.

#include "engine/debug/input_debug/input_debug.h"

#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypage.h" // FullscreenUIModeScope
#include "engine/graphics/text/font.h"
#include "engine/input/keyboard.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/gamepad.h"
#include "engine/input/gamepad_globals.h"
#include "engine/platform/ds_bridge.h"
#include "engine/platform/uc_common.h" // DisplayWidth, DisplayHeight
#include "ui/hud/panel.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

namespace {

bool s_active = false;
InputDebugPage s_page = INPUT_DEBUG_PAGE_KEYBOARD;

// Shorthand aliases for the layer constants declared in the header. Lower
// layer = closer to camera in this pipeline, so accent sits on top of
// content which sits on top of the backdrop. See the header comment on
// INPUT_DEBUG_LAYER_* for the full rationale.
constexpr SLONG LAYER_BACKDROP = INPUT_DEBUG_LAYER_BACKDROP;
constexpr SLONG LAYER_CONTENT = INPUT_DEBUG_LAYER_CONTENT;
constexpr SLONG LAYER_ACCENT = INPUT_DEBUG_LAYER_ACCENT;

InputDeviceType page_to_device(InputDebugPage p)
{
    switch (p) {
    case INPUT_DEBUG_PAGE_KEYBOARD:
        return INPUT_DEVICE_KEYBOARD_MOUSE;
    case INPUT_DEBUG_PAGE_GAMEPAD:
        return INPUT_DEVICE_XBOX;
    case INPUT_DEBUG_PAGE_DUALSENSE:
        return INPUT_DEVICE_DUALSENSE;
    default:
        return INPUT_DEVICE_KEYBOARD_MOUSE;
    }
}

// Idle gamepad state returned to pages whose device isn't currently active.
// Sticks at 32768 (centre) so stick widgets draw the dot at centre rather
// than in a corner; everything else zero (buttons released, triggers idle).
GamepadState make_idle_state()
{
    GamepadState s {};
    s.lX = s.lY = s.rX = s.rY = 32768;
    return s;
}
const GamepadState s_idle_state = make_idle_state();

// Navigation edge detection. `s_nav` holds the bools exposed via
// input_debug_nav(); `s_prev_*` tracks the key state from the previous
// frame so we emit a single edge on each press rather than one per frame
// while held.
InputDebugNav s_nav = {};
bool s_prev_up = false, s_prev_down = false;
bool s_prev_left = false, s_prev_right = false;
bool s_prev_enter = false;

void refresh_nav()
{
    const bool up = Keys[KB_UP] != 0;
    const bool down = Keys[KB_DOWN] != 0;
    const bool left = Keys[KB_LEFT] != 0;
    const bool right = Keys[KB_RIGHT] != 0;
    const bool ent = Keys[KB_ENTER] != 0;

    s_nav.up = up && !s_prev_up;
    s_nav.down = down && !s_prev_down;
    s_nav.left = left && !s_prev_left;
    s_nav.right = right && !s_prev_right;
    s_nav.enter = ent && !s_prev_enter;

    s_prev_up = up;
    s_prev_down = down;
    s_prev_left = left;
    s_prev_right = right;
    s_prev_enter = ent;
}

// Rumble test state. Motor amplitudes are shared across gamepad +
// DualSense pages — same physical hardware either way, just a different
// backend selected by gamepad_rumble internally.
enum RumbleRow {
    RUMBLE_ROW_LOW = 0,
    RUMBLE_ROW_HIGH,
    RUMBLE_ROW_FIRE,
    RUMBLE_ROW_COUNT,
};

int s_rumble_low = 128;
int s_rumble_high = 128;
bool s_rumble_firing = false;
std::chrono::steady_clock::time_point s_rumble_start;

constexpr int RUMBLE_STEP = 16;

int clamp_motor(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

void rumble_auto_stop_tick()
{
    // DualSense vibration is continuous (duration_ms ignored by the DS
    // path), so we stop it manually after the 200 ms window elapses.
    if (s_rumble_firing) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - s_rumble_start)
                                 .count();
        if (elapsed >= 200) {
            gamepad_rumble(0, 0, 0);
            s_rumble_firing = false;
        }
    }
}

void rumble_stop()
{
    if (s_rumble_firing) {
        gamepad_rumble(0, 0, 0);
        s_rumble_firing = false;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

static void isolate_controller_from_game()
{
    // Panel is modal and fully isolated — wipe anything the game was
    // driving on the controller so test widgets start from a known
    // state. Game's gamepad_*_update calls are gated off while the
    // panel is active; on close they re-apply next frame automatically
    // (except player LEDs, which the game doesn't drive — the panel's
    // own cleanup in input_debug_dualsense_reset_state handles those).
    gamepad_rumble_stop();
    gamepad_led_reset();
    gamepad_triggers_off();
    input_debug_dualsense_reset_state();
    input_debug_gamepad_reset_sub();
}

void input_debug_open()
{
    if (s_active)
        return;
    s_active = true;
    isolate_controller_from_game();
}

void input_debug_close()
{
    if (!s_active)
        return;
    s_active = false;
    rumble_stop();
    // Reset DS widget state so player LEDs / lightbar don't linger
    // until the next panel session, and the next session starts clean.
    input_debug_dualsense_reset_state();
    input_debug_gamepad_reset_sub();
}

void input_debug_toggle()
{
    if (s_active)
        input_debug_close();
    else
        input_debug_open();
}

bool input_debug_is_active() { return s_active; }

// ---------------------------------------------------------------------------
// Tick — input consumption while the panel is open
// ---------------------------------------------------------------------------

void input_debug_tick()
{
    if (!s_active)
        return;

    // Re-poll the gamepad ourselves. While the panel is open, the normal
    // callers of ReadInputDevice (GAMEMENU_process, get_hardware_input,
    // PAUSE_handler) are gated off — so without this explicit call the
    // gamepad state would freeze and the panel would show stale input.
    // gamepad_poll internally captures the pre-scrub snapshot for us.
    gamepad_poll();

    // Refresh navigation edges once per frame; widgets consume them
    // through input_debug_nav().
    refresh_nav();

    // ESC closes the panel. Consume so pause menu doesn't see it.
    // Route through input_debug_close so all cleanup (rumble_stop +
    // DS widget reset) runs — matches F11 close path.
    if (Keys[KB_ESC]) {
        Keys[KB_ESC] = 0;
        input_debug_close();
        return;
    }

    // 1 / 2 / 3 — switch page. Consume to keep the game clean.
    if (Keys[KB_1]) {
        Keys[KB_1] = 0;
        s_page = INPUT_DEBUG_PAGE_KEYBOARD;
    }
    if (Keys[KB_2]) {
        Keys[KB_2] = 0;
        s_page = INPUT_DEBUG_PAGE_GAMEPAD;
    }
    if (Keys[KB_3]) {
        Keys[KB_3] = 0;
        s_page = INPUT_DEBUG_PAGE_DUALSENSE;
    }

    // TAB cycles the current page through its sub-views (controller
    // viz → tests / triggers → back). Only pages that define sub-views
    // react — keyboard page has none.
    if (Keys[KB_TAB]) {
        if (s_page == INPUT_DEBUG_PAGE_DUALSENSE) {
            Keys[KB_TAB] = 0;
            input_debug_dualsense_toggle_sub();
        } else if (s_page == INPUT_DEBUG_PAGE_GAMEPAD) {
            Keys[KB_TAB] = 0;
            input_debug_gamepad_toggle_sub();
        }
    }

    // Keep the rumble pulse alive for its 200 ms window and then stop it.
    // Only meaningful on pages that actually use the rumble widget; on
    // the keyboard page there's no way to have started one.
    if (s_page == INPUT_DEBUG_PAGE_GAMEPAD || s_page == INPUT_DEBUG_PAGE_DUALSENSE) {
        rumble_auto_stop_tick();
    } else if (s_rumble_firing) {
        rumble_stop();
    }
}

const InputDebugNav& input_debug_nav()
{
    return s_nav;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
//
// One PANEL_start / PANEL_finish pair frames the backdrop AND all page
// widgets. Every 2D primitive queued between those calls flushes together
// this frame — avoids the shadow-page VB reuse bug documented in
// new_game_devlog/shadow_corruption_investigation.md (see the
// hud-rendering skill for the full rule).

static void draw_backdrop()
{
    // 80% opaque black covers 3D scene and HUD. POLY_PAGE_COLOUR_ALPHA
    // enables src-alpha blending; alpha in the high byte of the colour.
    // Logical 640×480 — non-uniformly stretched to the full FB by the
    // FullscreenUIModeScope wrapping the whole panel render (so the
    // backdrop and the rest of the panel cover edge-to-edge regardless
    // of window aspect).
    AENG_draw_rect(0, 0, DisplayWidth, DisplayHeight, 0xE6000000,
        LAYER_BACKDROP, POLY_PAGE_COLOUR_ALPHA);
}

// Three tabs across the top — one per page. Each tab shows the name,
// its hotkey, AND two independent highlights:
//   - "open"   — this is the currently-visible page (white box outline)
//   - "active" — the game currently treats this device as the active
//                input source (name text turns green)
// The two highlights stack: a tab can be both open AND active (bright
// green name inside a white box).
static void draw_one_tab(int idx, const char* name, const char* hotkey_hint,
    SLONG tab_x, SLONG tab_y, SLONG tab_w, SLONG tab_h)
{
    const InputDebugPage this_page = (InputDebugPage)idx;
    const bool is_open = (s_page == this_page);
    const bool is_active = (active_input_device == page_to_device(this_page));

    // White box outline around the open tab (4 thin rects).
    if (is_open) {
        AENG_draw_rect(tab_x, tab_y, tab_w, 1, 0xFFFFFF, LAYER_ACCENT, POLY_PAGE_COLOUR);
        AENG_draw_rect(tab_x, tab_y + tab_h - 1, tab_w, 1, 0xFFFFFF, LAYER_ACCENT, POLY_PAGE_COLOUR);
        AENG_draw_rect(tab_x, tab_y, 1, tab_h, 0xFFFFFF, LAYER_ACCENT, POLY_PAGE_COLOUR);
        AENG_draw_rect(tab_x + tab_w - 1, tab_y, 1, tab_h, 0xFFFFFF, LAYER_ACCENT, POLY_PAGE_COLOUR);
    }

    // Name colour: green when active, grey otherwise; bright when open.
    UBYTE nr, ng, nb;
    if (is_active) {
        nr = is_open ? 120 : 80;
        ng = is_open ? 255 : 180;
        nb = is_open ? 120 : 80;
    } else {
        nr = ng = nb = is_open ? 255 : 140;
    }

    input_debug_text(tab_x + 6, tab_y + 4, nr, ng, nb, 1, "%s", name);
    input_debug_text(tab_x + 6, tab_y + 16, 180, 180, 80, 1, "%s", hotkey_hint);
}

static void draw_tabs()
{
    const SLONG sw = DisplayWidth;
    const SLONG gap = 8;
    const SLONG tab_h = 28;
    const SLONG tab_w = (sw - 4 * gap) / 3;
    const SLONG tab_y = 4;

    draw_one_tab(INPUT_DEBUG_PAGE_KEYBOARD,
        "Keyboard", "press 1",
        gap + 0 * (tab_w + gap), tab_y, tab_w, tab_h);
    draw_one_tab(INPUT_DEBUG_PAGE_GAMEPAD,
        "Xbox controller", "press 2",
        gap + 1 * (tab_w + gap), tab_y, tab_w, tab_h);
    draw_one_tab(INPUT_DEBUG_PAGE_DUALSENSE,
        "DualSense controller", "press 3",
        gap + 2 * (tab_w + gap), tab_y, tab_w, tab_h);

    // Separator under the tab row.
    AENG_draw_rect(0, tab_y + tab_h + 4, sw, 1, 0x606060, LAYER_ACCENT, POLY_PAGE_COLOUR);
}

static void draw_footer()
{
    // Bottom hint line — controls summary. Kept as a single row so it
    // doesn't eat vertical real estate from the page content.
    input_debug_text(10, DisplayHeight - 14, 160, 160, 160, 1,
        "arrows navigate  |  left/right adjust  |  Enter activate  |  1/2/3 page  |  TAB DS sub-page  |  F11 / ESC exit");
}

void input_debug_render()
{
    if (!s_active)
        return;

    // Stretch virtual 640×480 panel coords non-uniformly across the entire
    // post-composition framebuffer. Picks the same xs/ys mapping as the
    // pre-`FONT_buffer_add_virtual` text path (`ScreenWidth/640`,
    // `ScreenHeight/480`) so on portrait windows the layout compresses
    // horizontally instead of overflowing off-screen the way the default
    // uniform-4:3 scope does. AddFan's PIXEL_HALF_OFFSET still applies on
    // top, so the backdrop / tabs cover edge-to-edge with no 1-px gap.
    PolyPage::FullscreenUIModeScope _panel_scope;

    PANEL_start();

    draw_backdrop();
    draw_tabs();
    draw_footer();

    switch (s_page) {
    case INPUT_DEBUG_PAGE_KEYBOARD:
        input_debug_render_keyboard_page();
        break;
    case INPUT_DEBUG_PAGE_GAMEPAD:
        input_debug_render_gamepad_page();
        break;
    case INPUT_DEBUG_PAGE_DUALSENSE:
        input_debug_render_dualsense_page();
        break;
    default:
        break;
    }

    PANEL_finish();
}

// ---------------------------------------------------------------------------
// Widget helpers
// ---------------------------------------------------------------------------

void input_debug_draw_stick(SLONG cx, SLONG cy, SLONG size,
    float nx, float ny, const char* label)
{
    // Clamp deflection so the dot never escapes the box on bad data.
    if (nx < -1.0f)
        nx = -1.0f;
    else if (nx > 1.0f)
        nx = 1.0f;
    if (ny < -1.0f)
        ny = -1.0f;
    else if (ny > 1.0f)
        ny = 1.0f;

    const SLONG half = size / 2;
    const SLONG x0 = cx - half;
    const SLONG y0 = cy - half;

    // Dim background box.
    AENG_draw_rect(x0, y0, size, size, 0x202020, LAYER_CONTENT, POLY_PAGE_COLOUR);
    // Subtle crosshair: horizontal + vertical thin lines through centre.
    AENG_draw_rect(x0, cy, size, 1, 0x404040, LAYER_CONTENT, POLY_PAGE_COLOUR);
    AENG_draw_rect(cx, y0, 1, size, 0x404040, LAYER_CONTENT, POLY_PAGE_COLOUR);

    // Red dot at deflection. 4×4 centred on the target pixel.
    const SLONG dx = (SLONG)(nx * (float)half);
    const SLONG dy = (SLONG)(ny * (float)half);
    AENG_draw_rect(cx + dx - 2, cy + dy - 2, 4, 4,
        0xFF3030, LAYER_ACCENT, POLY_PAGE_COLOUR);

    // Label centred above the box so the title visually lines up with
    // the stick centre (makes it easy to eyeball whether a d-pad /
    // face-button cluster below is aligned with the stick or not).
    const SLONG label_w_px = 6 * (SLONG)std::strlen(label);
    input_debug_text(cx - label_w_px / 2, y0 - 12, 200, 200, 200, 1, "%s", label);
}

void input_debug_draw_trigger_bar(SLONG x, SLONG y, SLONG w, SLONG h,
    int value, int max_val, const char* label)
{
    if (value < 0)
        value = 0;
    if (value > max_val)
        value = max_val;
    if (max_val <= 0)
        max_val = 1;

    // Dim outline box.
    AENG_draw_rect(x, y, w, h, 0x202020, LAYER_CONTENT, POLY_PAGE_COLOUR);
    // Yellow fill from the bottom.
    const SLONG fill_h = (h * value) / max_val;
    if (fill_h > 0) {
        AENG_draw_rect(x, y + h - fill_h, w, fill_h,
            0xFFCC00, LAYER_ACCENT, POLY_PAGE_COLOUR);
    }

    // Numeric readout below bar.
    input_debug_text(x - 4, y + h + 3, 200, 200, 200, 1, "%s=%d", label, value);
}

void input_debug_draw_button(SLONG x, SLONG y, const char* label, bool pressed)
{
    // Plain label — pressed / unpressed differ by colour only. Earlier
    // revisions wrapped the label in "[%s]" / " %s " but the bitmap
    // font renders the brackets as garbled placeholder glyphs, and the
    // framing was more noise than help anyway.
    if (pressed) {
        input_debug_text(x, y, 0, 255, 0, 1, "%s", label);
    } else {
        input_debug_text(x, y, 110, 110, 110, 1, "%s", label);
    }
}

void input_debug_draw_checkbox(SLONG x, SLONG y, const char* label, bool on)
{
    // Parentheses instead of square brackets — the bitmap font renders
    // '[' and ']' as garbled placeholder glyphs (same reason the button
    // widget dropped its framing in Iteration 8). '(' and ')' render
    // cleanly.
    if (on) {
        input_debug_text(x, y, 0, 255, 0, 1, "(x) %s", label);
    } else {
        input_debug_text(x, y, 130, 130, 130, 1, "( ) %s", label);
    }
}

void input_debug_text(SLONG x_logical, SLONG y_logical,
    unsigned char r, unsigned char g, unsigned char b,
    unsigned char shadow, const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    // (x_logical, y_logical) are virtual 640×480 panel coords. Text and
    // rects share the panel's `FullscreenUIModeScope` non-uniform scale
    // (`ScreenWidth/640` × `ScreenHeight/480`), so the whole layout
    // stretches together: text always sits where the rects expect it
    // regardless of window aspect. `FONT_buffer_add_virtual` resolves
    // these coords at flush time using the live PolyPage::s_XScale /
    // s_YScale, which inside the post-composition UI pass picks up the
    // scope's non-uniform mapping.
    FONT_buffer_add_virtual(x_logical, y_logical,
        r, g, b, shadow, (CBYTE*)"%s", buf);
}

// ---------------------------------------------------------------------------
// Read-through wrappers
// ---------------------------------------------------------------------------

const GamepadState& input_debug_read_gamepad_for(InputDeviceType page_device)
{
    if (active_input_device != page_device)
        return s_idle_state;
    return gamepad_state_raw();
}

bool input_debug_key_held(unsigned char scancode)
{
    if (active_input_device != INPUT_DEVICE_KEYBOARD_MOUSE)
        return false;
    return Keys[scancode] != 0;
}

uint8_t input_debug_read_ds_feedback(bool right_trigger)
{
    if (active_input_device != INPUT_DEVICE_DUALSENSE)
        return 0;
    return right_trigger ? gamepad_get_right_trigger_feedback_state()
                         : gamepad_get_left_trigger_feedback_state();
}

bool input_debug_read_ds_effect_active(bool right_trigger)
{
    if (active_input_device != INPUT_DEVICE_DUALSENSE)
        return false;
    return right_trigger ? gamepad_get_right_trigger_effect_active()
                         : gamepad_get_left_trigger_effect_active();
}

void input_debug_read_ds_touchpad(int* f1_x, int* f1_y, bool* f1_down,
    int* f2_x, int* f2_y, bool* f2_down)
{
    if (active_input_device != INPUT_DEVICE_DUALSENSE) {
        *f1_x = 0;
        *f1_y = 0;
        *f1_down = false;
        *f2_x = 0;
        *f2_y = 0;
        *f2_down = false;
        return;
    }
    DS_InputState s;
    if (!ds_get_input(&s)) {
        *f1_x = 0;
        *f1_y = 0;
        *f1_down = false;
        *f2_x = 0;
        *f2_y = 0;
        *f2_down = false;
        return;
    }
    *f1_x = s.touchpad_finger_1_x;
    *f1_y = s.touchpad_finger_1_y;
    *f1_down = s.touchpad_finger_1_down;
    *f2_x = s.touchpad_finger_2_x;
    *f2_y = s.touchpad_finger_2_y;
    *f2_down = s.touchpad_finger_2_down;
}

int input_debug_render_rumble_test(SLONG x, SLONG y, int local_cursor)
{
    input_debug_text(x, y, 220, 200, 120, 1, "Rumble test");

    // Handle input on the focused row.
    if (local_cursor >= 0 && local_cursor < RUMBLE_ROW_COUNT) {
        const InputDebugNav& n = input_debug_nav();
        if (n.left) {
            if (local_cursor == RUMBLE_ROW_LOW)
                s_rumble_low = clamp_motor(s_rumble_low - RUMBLE_STEP);
            if (local_cursor == RUMBLE_ROW_HIGH)
                s_rumble_high = clamp_motor(s_rumble_high - RUMBLE_STEP);
        }
        if (n.right) {
            if (local_cursor == RUMBLE_ROW_LOW)
                s_rumble_low = clamp_motor(s_rumble_low + RUMBLE_STEP);
            if (local_cursor == RUMBLE_ROW_HIGH)
                s_rumble_high = clamp_motor(s_rumble_high + RUMBLE_STEP);
        }
        if (n.enter && local_cursor == RUMBLE_ROW_FIRE) {
            const uint16_t lo = (uint16_t)((s_rumble_low << 8) | s_rumble_low);
            const uint16_t hi = (uint16_t)((s_rumble_high << 8) | s_rumble_high);
            gamepad_rumble(lo, hi, 200);
            s_rumble_firing = true;
            s_rumble_start = std::chrono::steady_clock::now();
        }
    }

    for (int row = 0; row < RUMBLE_ROW_COUNT; row++) {
        const bool selected = (row == local_cursor);
        const UBYTE r = selected ? 255 : 150;
        const UBYTE g = selected ? 255 : 150;
        const UBYTE b = selected ? 120 : 150;
        const char* prefix = selected ? "> " : "  ";
        const SLONG row_y = y + 14 + row * 12;

        switch (row) {
        case RUMBLE_ROW_LOW:
            input_debug_text(x + 4, row_y, r, g, b, 1,
                "%slow motor        %3d", prefix, s_rumble_low);
            break;
        case RUMBLE_ROW_HIGH:
            input_debug_text(x + 4, row_y, r, g, b, 1,
                "%shigh motor       %3d", prefix, s_rumble_high);
            break;
        case RUMBLE_ROW_FIRE:
            input_debug_text(x + 4, row_y, r, g, b, 1,
                "%sfire 200ms pulse%s",
                prefix, s_rumble_firing ? "    [FIRING]" : "");
            break;
        }
    }

    return RUMBLE_ROW_COUNT;
}
