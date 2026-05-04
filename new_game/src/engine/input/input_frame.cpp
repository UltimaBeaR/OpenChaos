// Per-frame input state aggregator. See input_frame.h for design.

#include "engine/input/input_frame.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/gamepad_globals.h"
#include "engine/input/gamepad.h" // gamepad_poll
#include "engine/input/joystick.h" // ReadInputDevice

#include <string.h>

namespace {

constexpr SLONG INPUT_KEY_COUNT = 256;
constexpr SLONG INPUT_BTN_COUNT = 32;

// Keyboard snapshots — derived from Keys[] each input_frame_update call.
UBYTE s_keys_curr[INPUT_KEY_COUNT];
UBYTE s_keys_prev[INPUT_KEY_COUNT];

// Sticky press flag — set in keyboard_key_down via input_frame_on_key_down,
// cleared only by explicit input_key_consume(). Survives frames.
UBYTE s_keys_press_pending[INPUT_KEY_COUNT];

// Gamepad button snapshots — derived from gamepad_state.rgbButtons[] each
// input_frame_update call. No sticky for gamepad: it is polled, not event-
// driven, so events between two polls are invisible regardless of any
// sticky scheme.
UBYTE s_btns_curr[INPUT_BTN_COUNT];
UBYTE s_btns_prev[INPUT_BTN_COUNT];

// Stick deadzone in raw 0..65535 units (center 32768). 8192 = ~25%.
// Picked to match the existing gamemenu.cpp threshold (GM_NOISE_TOLERANCE
// = 4096) but doubled, since input_frame is meant for general use including
// game movement which needs slightly more deadzone to avoid drift.
constexpr int STICK_RAW_CENTER   = 32768;
constexpr int STICK_RAW_DEADZONE = 8192;

// Map raw 0..65535 (center 32768) to float [-1.0, 1.0] with deadzone applied.
float apply_stick_deadzone(int raw)
{
    int delta = raw - STICK_RAW_CENTER;
    if (delta > -STICK_RAW_DEADZONE && delta < STICK_RAW_DEADZONE) {
        return 0.0f;
    }
    int sign = (delta > 0) ? 1 : -1;
    int abs_excursion = (sign > 0 ? delta : -delta) - STICK_RAW_DEADZONE;
    int max_excursion = STICK_RAW_CENTER - STICK_RAW_DEADZONE;
    float val = float(abs_excursion) / float(max_excursion);
    if (val > 1.0f) val = 1.0f;
    return float(sign) * val;
}

bool key_in_range(SLONG kb_code) { return kb_code >= 0 && kb_code < INPUT_KEY_COUNT; }
bool btn_in_range(SLONG btn_idx) { return btn_idx >= 0 && btn_idx < INPUT_BTN_COUNT; }

} // anon

void input_frame_init()
{
    memset(s_keys_curr, 0, sizeof(s_keys_curr));
    memset(s_keys_prev, 0, sizeof(s_keys_prev));
    memset(s_keys_press_pending, 0, sizeof(s_keys_press_pending));
    memset(s_btns_curr, 0, sizeof(s_btns_curr));
    memset(s_btns_prev, 0, sizeof(s_btns_prev));
}

void input_frame_update()
{
    // Refresh gamepad state. ReadInputDevice() drains gamepad events and
    // updates gamepad_state. Other call sites still call this themselves
    // (game.cpp, gamemenu.cpp, frontend.cpp, input_actions.cpp) — those calls
    // become near-no-ops after this one because the event queue is already
    // drained, but they continue to work for backward compatibility.
    ReadInputDevice();

    memcpy(s_keys_prev, s_keys_curr, sizeof(s_keys_curr));
    for (SLONG i = 0; i < INPUT_KEY_COUNT; i++) {
        s_keys_curr[i] = Keys[i] ? 1 : 0;
    }

    memcpy(s_btns_prev, s_btns_curr, sizeof(s_btns_curr));
    for (SLONG i = 0; i < INPUT_BTN_COUNT; i++) {
        s_btns_curr[i] = (gamepad_state.rgbButtons[i] & 0x80) ? 1 : 0;
    }
}

void input_frame_on_key_down(UBYTE scancode)
{
    s_keys_press_pending[scancode] = 1;
}

// ---- Keyboard ---------------------------------------------------------------

bool input_key_held(SLONG kb_code)
{
    return key_in_range(kb_code) && s_keys_curr[kb_code];
}

bool input_key_just_pressed(SLONG kb_code)
{
    return key_in_range(kb_code) && s_keys_curr[kb_code] && !s_keys_prev[kb_code];
}

bool input_key_just_released(SLONG kb_code)
{
    return key_in_range(kb_code) && !s_keys_curr[kb_code] && s_keys_prev[kb_code];
}

bool input_key_press_pending(SLONG kb_code)
{
    return key_in_range(kb_code) && s_keys_press_pending[kb_code];
}

void input_key_consume(SLONG kb_code)
{
    if (key_in_range(kb_code)) {
        s_keys_press_pending[kb_code] = 0;
    }
}

// ---- Gamepad buttons --------------------------------------------------------

bool input_btn_held(SLONG btn_idx)
{
    return btn_in_range(btn_idx) && s_btns_curr[btn_idx];
}

bool input_btn_just_pressed(SLONG btn_idx)
{
    return btn_in_range(btn_idx) && s_btns_curr[btn_idx] && !s_btns_prev[btn_idx];
}

bool input_btn_just_released(SLONG btn_idx)
{
    return btn_in_range(btn_idx) && !s_btns_curr[btn_idx] && s_btns_prev[btn_idx];
}

// ---- Auto-repeat — Phase 3 stub --------------------------------------------

bool input_key_just_pressed_or_repeat(SLONG kb_code)
{
    return input_key_just_pressed(kb_code);
}

bool input_btn_just_pressed_or_repeat(SLONG btn_idx)
{
    return input_btn_just_pressed(btn_idx);
}

// ---- Stick virtual directions — Phase 2 stub -------------------------------

bool input_stick_held(InputStickId, InputStickDir) { return false; }
bool input_stick_just_pressed(InputStickId, InputStickDir) { return false; }
bool input_stick_just_released(InputStickId, InputStickDir) { return false; }
bool input_stick_just_pressed_or_repeat(InputStickId, InputStickDir) { return false; }

// ---- Stick continuous -------------------------------------------------------

float input_stick_x(InputStickId stick)
{
    int raw = (stick == INPUT_STICK_LEFT) ? gamepad_state.lX : gamepad_state.rX;
    return apply_stick_deadzone(raw);
}

float input_stick_y(InputStickId stick)
{
    int raw = (stick == INPUT_STICK_LEFT) ? gamepad_state.lY : gamepad_state.rY;
    return apply_stick_deadzone(raw);
}

// ---- Triggers ---------------------------------------------------------------

float input_trigger(SLONG trigger_idx)
{
    if (trigger_idx == 15) return float(gamepad_state.trigger_left)  / 255.0f;
    if (trigger_idx == 16) return float(gamepad_state.trigger_right) / 255.0f;
    return 0.0f;
}
