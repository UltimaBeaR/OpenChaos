// Per-frame input state aggregator. See input_frame.h for design.

#include "engine/input/input_frame.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/gamepad_globals.h"
#include "engine/input/gamepad.h" // gamepad_poll
#include "engine/input/joystick.h" // ReadInputDevice
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks for auto-repeat

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

// Virtual stick directions — boolean per (stick, dir). Computed from
// continuous stick values with hysteresis so wobbling near the threshold
// doesn't flicker pressed/released. Mutual exclusion: simultaneous up+down
// or left+right cancel each other (no clear input intent).
//   [0] = INPUT_STICK_LEFT, [1] = INPUT_STICK_RIGHT
//   [0..3] = INPUT_STICK_DIR_UP/DOWN/LEFT/RIGHT
constexpr SLONG INPUT_STICK_COUNT = 2;
constexpr SLONG INPUT_DIR_COUNT   = 4;
UBYTE s_stick_dir_curr[INPUT_STICK_COUNT][INPUT_DIR_COUNT];
UBYTE s_stick_dir_prev[INPUT_STICK_COUNT][INPUT_DIR_COUNT];

// Auto-repeat next-fire timestamps. Set on rising edge to (now + initial
// delay). On every fire after that, advanced by repeat period. uint64_t is
// wall-clock ms from sdl3_get_ticks().
uint64_t s_keys_next_fire[INPUT_KEY_COUNT];
uint64_t s_btns_next_fire[INPUT_BTN_COUNT];
uint64_t s_stick_dir_next_fire[INPUT_STICK_COUNT][INPUT_DIR_COUNT];

// Auto-repeat cadence — same values as gamemenu.cpp's existing logic so the
// migrated menu navigation feels identical. Single source of truth: changing
// these here changes auto-repeat for every consumer.
constexpr uint64_t INPUT_REPEAT_INITIAL_MS = 400;
constexpr uint64_t INPUT_REPEAT_PERIOD_MS  = 150;

// Stick deadzone in raw 0..65535 units (center 32768). 8192 = ~25%.
// Picked to match the existing gamemenu.cpp threshold (GM_NOISE_TOLERANCE
// = 4096) but doubled, since input_frame is meant for general use including
// game movement which needs slightly more deadzone to avoid drift.
constexpr int STICK_RAW_CENTER   = 32768;
constexpr int STICK_RAW_DEADZONE = 8192;

// Virtual-direction thresholds with hysteresis. PRESS = need to deflect at
// least this much past the dead-band to enter "pressed". RELEASE = drop back
// below this to exit "pressed". Using the post-deadzone normalized [-1, +1]
// scale, so PRESS = 0.5 means ~50% deflection past deadzone.
constexpr float STICK_DIR_PRESS_THRESHOLD   = 0.5f;
constexpr float STICK_DIR_RELEASE_THRESHOLD = 0.25f;

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
bool stick_in_range(InputStickId stick) { return SLONG(stick) >= 0 && SLONG(stick) < INPUT_STICK_COUNT; }
bool dir_in_range(InputStickDir dir) { return SLONG(dir) >= 0 && SLONG(dir) < INPUT_DIR_COUNT; }

// Compute current pressed-state for one virtual direction with hysteresis.
// already_pressed: previous-frame state for this direction (used to pick the
// release threshold instead of press threshold — hysteresis).
// val:             post-deadzone stick value on the relevant axis [-1, +1].
// positive_dir:    true if this direction triggers when val > +threshold,
//                  false if val < -threshold.
bool compute_dir(bool already_pressed, float val, bool positive_dir)
{
    float threshold = already_pressed ? STICK_DIR_RELEASE_THRESHOLD : STICK_DIR_PRESS_THRESHOLD;
    return positive_dir ? (val > threshold) : (val < -threshold);
}

} // anon

void input_frame_init()
{
    memset(s_keys_curr, 0, sizeof(s_keys_curr));
    memset(s_keys_prev, 0, sizeof(s_keys_prev));
    memset(s_keys_press_pending, 0, sizeof(s_keys_press_pending));
    memset(s_btns_curr, 0, sizeof(s_btns_curr));
    memset(s_btns_prev, 0, sizeof(s_btns_prev));
    memset(s_stick_dir_curr, 0, sizeof(s_stick_dir_curr));
    memset(s_stick_dir_prev, 0, sizeof(s_stick_dir_prev));
    memset(s_keys_next_fire, 0, sizeof(s_keys_next_fire));
    memset(s_btns_next_fire, 0, sizeof(s_btns_next_fire));
    memset(s_stick_dir_next_fire, 0, sizeof(s_stick_dir_next_fire));
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

    // Virtual stick directions — recompute from continuous stick values with
    // hysteresis. Done after gamepad snapshot so input_stick_x/y reads fresh
    // state.
    memcpy(s_stick_dir_prev, s_stick_dir_curr, sizeof(s_stick_dir_curr));
    for (SLONG s = 0; s < INPUT_STICK_COUNT; s++) {
        InputStickId stick = InputStickId(s);
        float x = input_stick_x(stick);
        float y = input_stick_y(stick); // y < 0 = up (lY=0 is up after deadzone map)

        bool was_up    = s_stick_dir_curr[s][INPUT_STICK_DIR_UP];
        bool was_down  = s_stick_dir_curr[s][INPUT_STICK_DIR_DOWN];
        bool was_left  = s_stick_dir_curr[s][INPUT_STICK_DIR_LEFT];
        bool was_right = s_stick_dir_curr[s][INPUT_STICK_DIR_RIGHT];

        bool up    = compute_dir(was_up,    y, /*positive_dir=*/false);
        bool down  = compute_dir(was_down,  y, /*positive_dir=*/true);
        bool left  = compute_dir(was_left,  x, /*positive_dir=*/false);
        bool right = compute_dir(was_right, x, /*positive_dir=*/true);

        // Mutual exclusion: simultaneous opposite directions cancel.
        if (up && down)    { up = false; down = false; }
        if (left && right) { left = false; right = false; }

        s_stick_dir_curr[s][INPUT_STICK_DIR_UP]    = up    ? 1 : 0;
        s_stick_dir_curr[s][INPUT_STICK_DIR_DOWN]  = down  ? 1 : 0;
        s_stick_dir_curr[s][INPUT_STICK_DIR_LEFT]  = left  ? 1 : 0;
        s_stick_dir_curr[s][INPUT_STICK_DIR_RIGHT] = right ? 1 : 0;
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

// ---- Auto-repeat ------------------------------------------------------------
// Pattern: just_pressed sets next_fire = now + initial_delay. Subsequent fires
// while held happen when now >= next_fire, advancing next_fire by repeat
// period. Returns true on first press AND every repeat tick.

bool input_key_just_pressed_or_repeat(SLONG kb_code)
{
    if (!key_in_range(kb_code)) return false;
    uint64_t now = sdl3_get_ticks();
    if (input_key_just_pressed(kb_code)) {
        s_keys_next_fire[kb_code] = now + INPUT_REPEAT_INITIAL_MS;
        return true;
    }
    if (input_key_held(kb_code) && now >= s_keys_next_fire[kb_code]) {
        s_keys_next_fire[kb_code] = now + INPUT_REPEAT_PERIOD_MS;
        return true;
    }
    return false;
}

bool input_btn_just_pressed_or_repeat(SLONG btn_idx)
{
    if (!btn_in_range(btn_idx)) return false;
    uint64_t now = sdl3_get_ticks();
    if (input_btn_just_pressed(btn_idx)) {
        s_btns_next_fire[btn_idx] = now + INPUT_REPEAT_INITIAL_MS;
        return true;
    }
    if (input_btn_held(btn_idx) && now >= s_btns_next_fire[btn_idx]) {
        s_btns_next_fire[btn_idx] = now + INPUT_REPEAT_PERIOD_MS;
        return true;
    }
    return false;
}

// ---- Stick virtual directions -----------------------------------------------

bool input_stick_held(InputStickId stick, InputStickDir dir)
{
    if (!stick_in_range(stick) || !dir_in_range(dir)) return false;
    return s_stick_dir_curr[stick][dir];
}

bool input_stick_just_pressed(InputStickId stick, InputStickDir dir)
{
    if (!stick_in_range(stick) || !dir_in_range(dir)) return false;
    return s_stick_dir_curr[stick][dir] && !s_stick_dir_prev[stick][dir];
}

bool input_stick_just_released(InputStickId stick, InputStickDir dir)
{
    if (!stick_in_range(stick) || !dir_in_range(dir)) return false;
    return !s_stick_dir_curr[stick][dir] && s_stick_dir_prev[stick][dir];
}

bool input_stick_just_pressed_or_repeat(InputStickId stick, InputStickDir dir)
{
    if (!stick_in_range(stick) || !dir_in_range(dir)) return false;
    uint64_t now = sdl3_get_ticks();
    if (input_stick_just_pressed(stick, dir)) {
        s_stick_dir_next_fire[stick][dir] = now + INPUT_REPEAT_INITIAL_MS;
        return true;
    }
    if (input_stick_held(stick, dir) && now >= s_stick_dir_next_fire[stick][dir]) {
        s_stick_dir_next_fire[stick][dir] = now + INPUT_REPEAT_PERIOD_MS;
        return true;
    }
    return false;
}

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
