// Modal input debug panel — core, page dispatcher, widget helpers.
// See input_debug.h. Page content lives in input_debug_<page>.cpp.

#include "engine/debug/input_debug/input_debug.h"

#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/text/font.h"
#include "engine/input/keyboard.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/gamepad.h"
#include "engine/input/gamepad_globals.h"
#include "ui/hud/panel.h"

#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

namespace {

bool s_active = false;
InputDebugPage s_page = INPUT_DEBUG_PAGE_KEYBOARD;

// Higher layer = in front. Backdrop sits below content; widgets on top.
constexpr SLONG LAYER_BACKDROP = 10;
constexpr SLONG LAYER_CONTENT  = 11;
constexpr SLONG LAYER_ACCENT   = 12;

const char* page_name(InputDebugPage p)
{
    switch (p) {
        case INPUT_DEBUG_PAGE_KEYBOARD:  return "Keyboard";
        case INPUT_DEBUG_PAGE_GAMEPAD:   return "Gamepad";
        case INPUT_DEBUG_PAGE_DUALSENSE: return "DualSense";
        default:                         return "?";
    }
}

const char* active_device_name()
{
    switch (active_input_device) {
        case INPUT_DEVICE_KEYBOARD_MOUSE: return "Keyboard/Mouse";
        case INPUT_DEVICE_XBOX:           return "Xbox/Generic";
        case INPUT_DEVICE_DUALSENSE:      return "DualSense";
        default:                          return "?";
    }
}

// Idle gamepad state returned to pages whose device isn't currently active.
// Sticks at 32768 (centre) so stick widgets draw the dot at centre rather
// than in a corner; everything else zero (buttons released, triggers idle).
GamepadState make_idle_state()
{
    GamepadState s{};
    s.lX = s.lY = s.rX = s.rY = 32768;
    return s;
}
const GamepadState s_idle_state = make_idle_state();

} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void input_debug_open()  { s_active = true; }
void input_debug_close() { s_active = false; }
void input_debug_toggle(){ s_active = !s_active; }
bool input_debug_is_active() { return s_active; }

// ---------------------------------------------------------------------------
// Tick — input consumption while the panel is open
// ---------------------------------------------------------------------------

void input_debug_tick()
{
    if (!s_active) return;

    // Re-poll the gamepad ourselves. While the panel is open, the normal
    // callers of ReadInputDevice (GAMEMENU_process, get_hardware_input,
    // PAUSE_handler) are gated off — so without this explicit call the
    // gamepad state would freeze and the panel would show stale input.
    // gamepad_poll internally captures the pre-scrub snapshot for us.
    gamepad_poll();

    // ESC closes the panel. Consume so pause menu doesn't see it.
    if (Keys[KB_ESC]) {
        Keys[KB_ESC] = 0;
        s_active = false;
        return;
    }

    // 1 / 2 / 3 — switch page. Consume to keep the game clean.
    if (Keys[KB_1]) { Keys[KB_1] = 0; s_page = INPUT_DEBUG_PAGE_KEYBOARD; }
    if (Keys[KB_2]) { Keys[KB_2] = 0; s_page = INPUT_DEBUG_PAGE_GAMEPAD; }
    if (Keys[KB_3]) { Keys[KB_3] = 0; s_page = INPUT_DEBUG_PAGE_DUALSENSE; }
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
    const SLONG w = (SLONG)POLY_screen_width;
    const SLONG h = (SLONG)POLY_screen_height;
    AENG_draw_rect(0, 0, w, h, 0xCC000000, LAYER_BACKDROP, POLY_PAGE_COLOUR_ALPHA);
}

static void draw_header()
{
    // Title / active device / page picker on the first two lines, then a
    // thin separator line across the screen.
    char buf[128];

    std::snprintf(buf, sizeof(buf),
        "INPUT DEBUG PANEL     ACTIVE: %s", active_device_name());
    FONT_buffer_add(10, 6, 255, 255, 255, 1, (CBYTE*)"%s", buf);

    std::snprintf(buf, sizeof(buf),
        "PAGE: %s     [1] Keyboard   [2] Gamepad   [3] DualSense     F11 / ESC exit",
        page_name(s_page));
    FONT_buffer_add(10, 20, 200, 200, 200, 1, (CBYTE*)"%s", buf);

    // Separator under the header (1px tall, full width).
    const SLONG w = (SLONG)POLY_screen_width;
    AENG_draw_rect(0, 34, w, 1, 0x808080, LAYER_ACCENT, POLY_PAGE_COLOUR);
}

void input_debug_render()
{
    if (!s_active) return;

    PANEL_start();

    draw_backdrop();
    draw_header();

    switch (s_page) {
        case INPUT_DEBUG_PAGE_KEYBOARD:  input_debug_render_keyboard_page();  break;
        case INPUT_DEBUG_PAGE_GAMEPAD:   input_debug_render_gamepad_page();   break;
        case INPUT_DEBUG_PAGE_DUALSENSE: input_debug_render_dualsense_page(); break;
        default: break;
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
    if (nx < -1.0f) nx = -1.0f; else if (nx > 1.0f) nx = 1.0f;
    if (ny < -1.0f) ny = -1.0f; else if (ny > 1.0f) ny = 1.0f;

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

    // Label above the box.
    FONT_buffer_add(x0, y0 - 12, 200, 200, 200, 1, (CBYTE*)"%s", label);
}

void input_debug_draw_trigger_bar(SLONG x, SLONG y, SLONG w, SLONG h,
                                  int value, int max_val, const char* label)
{
    if (value < 0) value = 0;
    if (value > max_val) value = max_val;
    if (max_val <= 0) max_val = 1;

    // Dim outline box.
    AENG_draw_rect(x, y, w, h, 0x202020, LAYER_CONTENT, POLY_PAGE_COLOUR);
    // Yellow fill from the bottom.
    const SLONG fill_h = (h * value) / max_val;
    if (fill_h > 0) {
        AENG_draw_rect(x, y + h - fill_h, w, fill_h,
                       0xFFCC00, LAYER_ACCENT, POLY_PAGE_COLOUR);
    }

    // Numeric readout below bar.
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%s=%d", label, value);
    FONT_buffer_add(x - 4, y + h + 3, 200, 200, 200, 1, (CBYTE*)"%s", buf);
}

void input_debug_draw_button(SLONG x, SLONG y, const char* label, bool pressed)
{
    if (pressed) {
        FONT_buffer_add(x, y, 0, 255, 0, 1, (CBYTE*)"[%s]", label);
    } else {
        FONT_buffer_add(x, y, 110, 110, 110, 1, (CBYTE*)" %s ", label);
    }
}

void input_debug_draw_checkbox(SLONG x, SLONG y, const char* label, bool on)
{
    if (on) {
        FONT_buffer_add(x, y, 0, 255, 0, 1, (CBYTE*)"[x] %s", label);
    } else {
        FONT_buffer_add(x, y, 130, 130, 130, 1, (CBYTE*)"[ ] %s", label);
    }
}

// ---------------------------------------------------------------------------
// Read-through wrappers
// ---------------------------------------------------------------------------

const GamepadState& input_debug_read_gamepad_for(InputDeviceType page_device)
{
    if (active_input_device != page_device) return s_idle_state;
    return gamepad_state_raw();
}

bool input_debug_key_held(unsigned char scancode)
{
    if (active_input_device != INPUT_DEVICE_KEYBOARD_MOUSE) return false;
    return Keys[scancode] != 0;
}
