// Per-frame input state aggregator. See input_frame.h for design.

#include "engine/input/input_frame.h"
#include "engine/input/gamepad_globals.h"
#include "engine/input/gamepad.h" // gamepad_poll
#include "engine/input/joystick.h" // ReadInputDevice
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks for auto-repeat

#include <string.h>

namespace {

constexpr SLONG INPUT_KEY_COUNT = 256;
constexpr SLONG INPUT_BTN_COUNT = 32;

// Keyboard snapshots — derived from input_frame's OWN event-tracked held
// state, NOT from Keys[]. Decoupled from Keys[] because consumers (menu
// handlers etc.) clear Keys[KB_X] = 0 after consume, which would otherwise
// leak into the next frame's snapshot and break auto-repeat for held keys.
UBYTE s_keys_curr[INPUT_KEY_COUNT];
UBYTE s_keys_prev[INPUT_KEY_COUNT];

// Event-driven held state — set by SDL key_down hook, cleared by key_up
// hook. Independent of Keys[]. Source of truth for snapshot.
UBYTE s_keys_event_held[INPUT_KEY_COUNT];

// One-frame press marker — set on key_down, cleared at end of update.
// OR'd into curr so a press+release in the same frame is still visible
// for one snapshot (otherwise held would already be 0 by snapshot time).
UBYTE s_keys_pressed_during_frame[INPUT_KEY_COUNT];

// Sticky press flag — set on key_down, cleared only by explicit
// input_key_consume(). Survives across frames. For consumers that may not
// run every frame (e.g. physics tick).
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

// Auto-repeat "armed" flags. Set to 1 when just_pressed fires for this
// key/btn/stick-dir IN A CALL to input_*_just_pressed_or_repeat, cleared
// when held becomes false. Auto-repeat ticks only when armed — so a key
// held into a context that just started consulting input_frame doesn't
// trigger immediate auto-fire from a stale next_fire.
//
// Example: user holds stick during gameplay, opens pause menu. Menu's
// first auto-repeat call sees held=true, just_pressed=false (rising edge
// already passed), armed=false (never set in this consumer's history).
// Returns false until user releases and re-presses.
UBYTE s_keys_repeat_armed[INPUT_KEY_COUNT];
UBYTE s_btns_repeat_armed[INPUT_BTN_COUNT];
UBYTE s_stick_dir_repeat_armed[INPUT_STICK_COUNT][INPUT_DIR_COUNT];

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

// Virtual-direction thresholds in RAW 0..65535 units, distance from center
// (32768). Independent of the continuous-output deadzone — virtual directions
// are designed for menu navigation where sensitivity should match the
// original gamemenu.cpp threshold (GM_NOISE_TOLERANCE = 4096 from center).
// Hysteresis (release < press) prevents flicker on stick wobble at boundary.
constexpr int STICK_DIR_PRESS_RAW   = 4096; // matches gamemenu.cpp behavior
constexpr int STICK_DIR_RELEASE_RAW = 2048; // half — hysteresis

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
// raw_axis:        raw 0..65535 axis value (lX/lY/rX/rY).
// positive_dir:    true if this direction triggers when delta > +threshold
//                  (e.g. right or down), false if delta < -threshold (left/up).
bool compute_dir(bool already_pressed, int raw_axis, bool positive_dir)
{
    int threshold = already_pressed ? STICK_DIR_RELEASE_RAW : STICK_DIR_PRESS_RAW;
    int delta = raw_axis - STICK_RAW_CENTER;
    return positive_dir ? (delta > threshold) : (delta < -threshold);
}

} // anon

void input_frame_init()
{
    memset(s_keys_curr, 0, sizeof(s_keys_curr));
    memset(s_keys_prev, 0, sizeof(s_keys_prev));
    memset(s_keys_event_held, 0, sizeof(s_keys_event_held));
    memset(s_keys_pressed_during_frame, 0, sizeof(s_keys_pressed_during_frame));
    memset(s_keys_press_pending, 0, sizeof(s_keys_press_pending));
    memset(s_btns_curr, 0, sizeof(s_btns_curr));
    memset(s_btns_prev, 0, sizeof(s_btns_prev));
    memset(s_stick_dir_curr, 0, sizeof(s_stick_dir_curr));
    memset(s_stick_dir_prev, 0, sizeof(s_stick_dir_prev));
    memset(s_keys_next_fire, 0, sizeof(s_keys_next_fire));
    memset(s_btns_next_fire, 0, sizeof(s_btns_next_fire));
    memset(s_stick_dir_next_fire, 0, sizeof(s_stick_dir_next_fire));
    memset(s_keys_repeat_armed, 0, sizeof(s_keys_repeat_armed));
    memset(s_btns_repeat_armed, 0, sizeof(s_btns_repeat_armed));
    memset(s_stick_dir_repeat_armed, 0, sizeof(s_stick_dir_repeat_armed));
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
        // OR pressed-during-frame so a same-frame press+release is visible
        // for exactly one snapshot. Cleared after read so next frame falls
        // back to held-only.
        s_keys_curr[i] = (s_keys_event_held[i] || s_keys_pressed_during_frame[i]) ? 1 : 0;
        s_keys_pressed_during_frame[i] = 0;
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
        // Raw axis values (0..65535, center 32768). lY=0 is up.
        int raw_x = (s == INPUT_STICK_LEFT) ? gamepad_state.lX : gamepad_state.rX;
        int raw_y = (s == INPUT_STICK_LEFT) ? gamepad_state.lY : gamepad_state.rY;

        bool was_up    = s_stick_dir_curr[s][INPUT_STICK_DIR_UP];
        bool was_down  = s_stick_dir_curr[s][INPUT_STICK_DIR_DOWN];
        bool was_left  = s_stick_dir_curr[s][INPUT_STICK_DIR_LEFT];
        bool was_right = s_stick_dir_curr[s][INPUT_STICK_DIR_RIGHT];

        bool up    = compute_dir(was_up,    raw_y, /*positive_dir=*/false);
        bool down  = compute_dir(was_down,  raw_y, /*positive_dir=*/true);
        bool left  = compute_dir(was_left,  raw_x, /*positive_dir=*/false);
        bool right = compute_dir(was_right, raw_x, /*positive_dir=*/true);

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
    s_keys_event_held[scancode] = 1;
    s_keys_pressed_during_frame[scancode] = 1;
    s_keys_press_pending[scancode] = 1;
}

void input_frame_on_key_up(UBYTE scancode)
{
    s_keys_event_held[scancode] = 0;
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
    if (!input_key_held(kb_code)) {
        s_keys_repeat_armed[kb_code] = 0; // disarm on release
        return false;
    }
    uint64_t now = sdl3_get_ticks();
    if (input_key_just_pressed(kb_code)) {
        s_keys_next_fire[kb_code] = now + INPUT_REPEAT_INITIAL_MS;
        s_keys_repeat_armed[kb_code] = 1;
        return true;
    }
    if (s_keys_repeat_armed[kb_code] && now >= s_keys_next_fire[kb_code]) {
        s_keys_next_fire[kb_code] = now + INPUT_REPEAT_PERIOD_MS;
        return true;
    }
    return false;
}

bool input_btn_just_pressed_or_repeat(SLONG btn_idx)
{
    if (!btn_in_range(btn_idx)) return false;
    if (!input_btn_held(btn_idx)) {
        s_btns_repeat_armed[btn_idx] = 0;
        return false;
    }
    uint64_t now = sdl3_get_ticks();
    if (input_btn_just_pressed(btn_idx)) {
        s_btns_next_fire[btn_idx] = now + INPUT_REPEAT_INITIAL_MS;
        s_btns_repeat_armed[btn_idx] = 1;
        return true;
    }
    if (s_btns_repeat_armed[btn_idx] && now >= s_btns_next_fire[btn_idx]) {
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
    if (!input_stick_held(stick, dir)) {
        s_stick_dir_repeat_armed[stick][dir] = 0;
        return false;
    }
    uint64_t now = sdl3_get_ticks();
    if (input_stick_just_pressed(stick, dir)) {
        s_stick_dir_next_fire[stick][dir] = now + INPUT_REPEAT_INITIAL_MS;
        s_stick_dir_repeat_armed[stick][dir] = 1;
        return true;
    }
    if (s_stick_dir_repeat_armed[stick][dir] && now >= s_stick_dir_next_fire[stick][dir]) {
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

// ---- InputAutoRepeat --------------------------------------------------------

bool InputAutoRepeat::tick_combined(bool any_just_pressed, bool any_held)
{
    if (!any_held) {
        // Combined released — disarm and reset prev state for next press.
        armed = false;
        was_held = false;
        return false;
    }
    // Combined rising edge: at least one source had a snapshot-level rising
    // edge in this frame AND combined wasn't already held last frame. This
    // ignores per-source rising edges that happen while combined is already
    // held (adding a second source mid-repeat) and ignores inputs that were
    // held into a context that just started polling (was_held stays false
    // because we never observed combined held before).
    const bool combined_just_pressed = any_just_pressed && !was_held;
    was_held = true;

    const uint64_t now = sdl3_get_ticks();
    if (combined_just_pressed) {
        armed = true;
        next_fire = now + INPUT_REPEAT_INITIAL_MS;
        return true;
    }
    if (armed && now >= next_fire) {
        next_fire = now + INPUT_REPEAT_PERIOD_MS;
        return true;
    }
    return false;
}
