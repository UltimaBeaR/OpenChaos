// Per-frame input state aggregator. See input_frame.h for design.

#include "engine/input/input_frame.h"
#include "engine/input/gamepad_globals.h"
#include "engine/input/gamepad.h" // gamepad_poll
#include "engine/input/mouse_globals.h" // MouseX/MouseY for input_mouse_x/y
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks for auto-repeat
#include "engine/io/oc_config.h" // OC_CONFIG_get_float (stick deadzones)
#include "engine/core/types.h" // BOOL
#include "engine/debug/input_debug/input_debug.h" // input_debug_is_active
#include "game/action_map/input_codes.h" // GAXIS_* / GDIR_*
#include "game/action_map/act_bangunsnotgames.h" // ACT_BANG_DEBUG_MODIFIER_KKEY (F1 debug-modifier gate)

// Runtime debug-mode flag — set by typing "bangunsnotgames" in the dev
// console (game_tick_globals.cpp). Used by input_debug_modifier_active()
// below. Forward-declared here (rather than #include game_tick_globals.h)
// to keep input_frame's #include surface engine-only.
extern BOOL allow_debug_keys;

#include <string.h>

namespace {

constexpr SLONG INPUT_KEY_COUNT = 256;
constexpr SLONG INPUT_BTN_COUNT = 32;

// Keyboard snapshots — derived from input_frame's OWN event-tracked held
// state, NOT from Keys[]. Decoupled from Keys[] because consumers (menu
// handlers etc.) clear Keys[KKEY_X] = 0 after consume, which would otherwise
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

// Wait-for-release gate. While 1, the key is treated as not pressed by
// every public reader — snapshot curr is forced to 0 and press_pending
// is suppressed. Cleared on the first snapshot that observes the
// matching event_held flag back at 0 (SDL key-up arrived). Set by
// input_keyboard_consume_all_held_until_released for every key that
// is event-held at call time. Use case: UI overlay closes back to
// gameplay; a key the user was holding when the overlay opened must
// NOT register as a fresh press the moment gameplay resumes.
UBYTE s_keys_consume_until_released[INPUT_KEY_COUNT];

// Most recent keyboard scancode pressed since last consume. Used by
// text-input consumers (rebind UI, debug console, skip detection) that
// need the exact scancode of the latest keydown. Only hardware events
// update it.
UBYTE s_last_key = 0;

// Gamepad button snapshots — derived from gamepad_state.rgbButtons[] each
// input_frame_update call.
UBYTE s_btns_curr[INPUT_BTN_COUNT];
UBYTE s_btns_prev[INPUT_BTN_COUNT];

// Sticky press flag for gamepad — set whenever a rising edge is seen by
// a render-frame snapshot, cleared only by input_btn_consume(). Lets a
// physics-tick consumer (e.g. vehicle siren toggle at 1..20 Hz physics +
// any render rate) catch a rising edge that occurred on a render frame
// where its tick wasn't running. At low physics Hz the press window can
// be entirely between two ticks — sticky pending closes that gap.
UBYTE s_btns_press_pending[INPUT_BTN_COUNT];

// Mouse button state — event-driven (like keyboard) since SDL emits
// per-event. INPUT_MBTN_COUNT = 3 (LEFT / MIDDLE / RIGHT). Mirrors the
// keyboard state machine: event_held set by SDL down/up hooks; curr/prev
// snapshot at frame start; press_pending sticky-latched on rising edge;
// pressed_during_frame catches same-frame press+release. The wait-for-
// release gate (s_mbtns_consume_until_released) ensures a button held
// across a UI transition doesn't fire as a fresh action in the first
// post-transition gameplay tick — same semantic as the keyboard gate.
constexpr SLONG INPUT_MBTN_COUNT = 3;
UBYTE s_mbtns_curr[INPUT_MBTN_COUNT];
UBYTE s_mbtns_prev[INPUT_MBTN_COUNT];
UBYTE s_mbtns_event_held[INPUT_MBTN_COUNT];
UBYTE s_mbtns_pressed_during_frame[INPUT_MBTN_COUNT];
UBYTE s_mbtns_press_pending[INPUT_MBTN_COUNT];
UBYTE s_mbtns_consume_until_released[INPUT_MBTN_COUNT];

// Virtual stick directions — boolean per (stick, dir). Computed from
// continuous stick values with hysteresis so wobbling near the threshold
// doesn't flicker pressed/released. Mutual exclusion: simultaneous up+down
// or left+right cancel each other (no clear input intent).
//   [0] = GAXIS_LEFT, [1] = GAXIS_RIGHT
//   [0..3] = GDIR_UP/DOWN/LEFT/RIGHT
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

// Stick raw center. STICK_RAW_DEADZONE is the deadzone for the CONTINUOUS stick
// output (apply_stick_deadzone → input_stick_x/y). 8192 = ~25%.
constexpr int STICK_RAW_CENTER   = 32768;
constexpr int STICK_RAW_DEADZONE = 8192;

// Configurable stick deadzones, RAW units (distance from center 32768). Loaded
// from config in input_frame_init (gamepad section, stored as a fraction 0..1):
//   s_gameplay_deadzone_raw : in-game movement/aim deadzone — exposed via
//                             input_gameplay_deadzone_raw() for input_actions'
//                             get_hardware_input (the old NOISE_TOLERANCE).
//   s_menu_dir_press_raw    : menu-navigation virtual-direction threshold;
//                             s_menu_dir_release_raw is half of it (hysteresis
//                             against stick wobble at the boundary).
// Defaults correspond to a 0.25 fraction (8192 raw). The menu threshold used to
// be 4096 (a 0.125 fraction) — raised so controller drift doesn't auto-scroll
// menus when the stick is left untouched.
int s_gameplay_deadzone_raw = 8192;
int s_menu_dir_press_raw    = 8192;
int s_menu_dir_release_raw  = 4096;

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
bool mbtn_in_range(SLONG mbtn_idx) { return mbtn_idx >= 0 && mbtn_idx < INPUT_MBTN_COUNT; }
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
    int threshold = already_pressed ? s_menu_dir_release_raw : s_menu_dir_press_raw;
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
    memset(s_keys_consume_until_released, 0, sizeof(s_keys_consume_until_released));
    memset(s_btns_curr, 0, sizeof(s_btns_curr));
    memset(s_btns_prev, 0, sizeof(s_btns_prev));
    memset(s_btns_press_pending, 0, sizeof(s_btns_press_pending));
    memset(s_mbtns_curr, 0, sizeof(s_mbtns_curr));
    memset(s_mbtns_prev, 0, sizeof(s_mbtns_prev));
    memset(s_mbtns_event_held, 0, sizeof(s_mbtns_event_held));
    memset(s_mbtns_pressed_during_frame, 0, sizeof(s_mbtns_pressed_during_frame));
    memset(s_mbtns_press_pending, 0, sizeof(s_mbtns_press_pending));
    memset(s_mbtns_consume_until_released, 0, sizeof(s_mbtns_consume_until_released));
    memset(s_stick_dir_curr, 0, sizeof(s_stick_dir_curr));
    memset(s_stick_dir_prev, 0, sizeof(s_stick_dir_prev));
    memset(s_keys_next_fire, 0, sizeof(s_keys_next_fire));
    memset(s_btns_next_fire, 0, sizeof(s_btns_next_fire));
    memset(s_stick_dir_next_fire, 0, sizeof(s_stick_dir_next_fire));
    memset(s_keys_repeat_armed, 0, sizeof(s_keys_repeat_armed));
    memset(s_btns_repeat_armed, 0, sizeof(s_btns_repeat_armed));
    memset(s_stick_dir_repeat_armed, 0, sizeof(s_stick_dir_repeat_armed));
    s_last_key = 0;

    // Load stick deadzones from config. Stored as a fraction 0..1 of full
    // deflection; converted to RAW (× center). Clamped to [0, 0.9] so the stick
    // can never be fully dead. Runs after OC_CONFIG_load (config is read by the
    // surrounding host setup before this init).
    auto frac_to_raw = [](float f) -> int {
        if (f < 0.0f) f = 0.0f;
        if (f > 0.9f) f = 0.9f;
        return (int)(f * STICK_RAW_CENTER);
    };
    s_gameplay_deadzone_raw = frac_to_raw(OC_CONFIG_get_float("gamepad", "gameplay_stick_deadzone", 0.25f));
    s_menu_dir_press_raw    = frac_to_raw(OC_CONFIG_get_float("gamepad", "menu_stick_deadzone", 0.25f));
    s_menu_dir_release_raw  = s_menu_dir_press_raw / 2; // half — hysteresis
}

// In-game stick deadzone (RAW units, distance from center 32768), from the
// gamepad.gameplay_stick_deadzone config fraction. Consumed by input_actions'
// get_hardware_input as the movement/aim deadzone (the old NOISE_TOLERANCE).
int input_gameplay_deadzone_raw()
{
    return s_gameplay_deadzone_raw;
}

void input_frame_update()
{
    // Refresh gamepad state. gamepad_poll drains gamepad events and updates
    // gamepad_state. Single source of truth — all leaf consumers read via
    // input_frame, no other call site polls.
    gamepad_poll();

    memcpy(s_keys_prev, s_keys_curr, sizeof(s_keys_curr));
    for (SLONG i = 0; i < INPUT_KEY_COUNT; i++) {
        // OR pressed-during-frame so a same-frame press+release is visible
        // for exactly one snapshot. Cleared after read so next frame falls
        // back to held-only.
        s_keys_curr[i] = (s_keys_event_held[i] || s_keys_pressed_during_frame[i]) ? 1 : 0;
        s_keys_pressed_during_frame[i] = 0;

        // Wait-for-release: clear the gate as soon as the user has
        // physically released the key (event_held back to 0). Otherwise
        // mask the snapshot so the held key reads as not pressed AND
        // drain any sticky press_pending that fired since the gate was
        // armed. The gate is armed by input_keyboard_consume_all_held_until_released.
        if (s_keys_consume_until_released[i]) {
            if (!s_keys_event_held[i]) {
                s_keys_consume_until_released[i] = 0;
            } else {
                s_keys_curr[i] = 0;
                s_keys_press_pending[i] = 0;
            }
        }
    }

    memcpy(s_btns_prev, s_btns_curr, sizeof(s_btns_curr));
    for (SLONG i = 0; i < INPUT_BTN_COUNT; i++) {
        s_btns_curr[i] = (gamepad_state.rgbButtons[i] & 0x80) ? 1 : 0;
        // Sticky: latch on rising edge, only cleared by input_btn_consume.
        if (s_btns_curr[i] && !s_btns_prev[i]) {
            s_btns_press_pending[i] = 1;
        }
    }

    memcpy(s_mbtns_prev, s_mbtns_curr, sizeof(s_mbtns_curr));
    for (SLONG i = 0; i < INPUT_MBTN_COUNT; i++) {
        // OR pressed-during-frame so a same-frame press+release stays visible
        // for exactly one snapshot. Cleared after read.
        s_mbtns_curr[i] = (s_mbtns_event_held[i] || s_mbtns_pressed_during_frame[i]) ? 1 : 0;
        s_mbtns_pressed_during_frame[i] = 0;

        // Wait-for-release: same semantic as the keyboard gate. Cleared the
        // first snapshot after the user has physically released the button.
        if (s_mbtns_consume_until_released[i]) {
            if (!s_mbtns_event_held[i]) {
                s_mbtns_consume_until_released[i] = 0;
            } else {
                s_mbtns_curr[i] = 0;
                s_mbtns_press_pending[i] = 0;
            }
        }
    }

    // Virtual stick directions — recompute from continuous stick values with
    // hysteresis. Done after gamepad snapshot so input_stick_x/y reads fresh
    // state.
    memcpy(s_stick_dir_prev, s_stick_dir_curr, sizeof(s_stick_dir_curr));
    for (SLONG s = 0; s < INPUT_STICK_COUNT; s++) {
        // Read PRE-override stick values (lX_raw / lY_raw etc). lX/lY get
        // clamped to 0/65535 when the D-Pad is held, which would otherwise
        // hide a stick deflected in the opposite direction. Menu consumers
        // that need both signals (for antagonist suppression on stick + D-Pad)
        // OR D-Pad rgbButtons[11..14] alongside this virtual direction —
        // see migration_checklist.md, rule 3. Game code keeps reading lX/lY
        // directly where the D-Pad-as-full-deflection semantic is desired.
        int raw_x = (s == GAXIS_LEFT) ? gamepad_state.lX_raw : gamepad_state.rX_raw;
        int raw_y = (s == GAXIS_LEFT) ? gamepad_state.lY_raw : gamepad_state.rY_raw;

        bool was_up    = s_stick_dir_curr[s][GDIR_UP];
        bool was_down  = s_stick_dir_curr[s][GDIR_DOWN];
        bool was_left  = s_stick_dir_curr[s][GDIR_LEFT];
        bool was_right = s_stick_dir_curr[s][GDIR_RIGHT];

        bool up    = compute_dir(was_up,    raw_y, /*positive_dir=*/false);
        bool down  = compute_dir(was_down,  raw_y, /*positive_dir=*/true);
        bool left  = compute_dir(was_left,  raw_x, /*positive_dir=*/false);
        bool right = compute_dir(was_right, raw_x, /*positive_dir=*/true);

        // Mutual exclusion: simultaneous opposite directions cancel.
        if (up && down)    { up = false; down = false; }
        if (left && right) { left = false; right = false; }

        s_stick_dir_curr[s][GDIR_UP]    = up    ? 1 : 0;
        s_stick_dir_curr[s][GDIR_DOWN]  = down  ? 1 : 0;
        s_stick_dir_curr[s][GDIR_LEFT]  = left  ? 1 : 0;
        s_stick_dir_curr[s][GDIR_RIGHT] = right ? 1 : 0;
    }
}

void input_frame_on_key_down(UBYTE scancode)
{
    s_keys_event_held[scancode] = 1;
    s_keys_pressed_during_frame[scancode] = 1;
    s_keys_press_pending[scancode] = 1;
    s_last_key = scancode;
}

void input_frame_on_key_up(UBYTE scancode)
{
    s_keys_event_held[scancode] = 0;
}

void input_frame_on_mouse_button_down(SLONG mbtn_idx)
{
    if (!mbtn_in_range(mbtn_idx)) return;
    s_mbtns_event_held[mbtn_idx] = 1;
    s_mbtns_pressed_during_frame[mbtn_idx] = 1;
    s_mbtns_press_pending[mbtn_idx] = 1;
}

void input_frame_on_mouse_button_up(SLONG mbtn_idx)
{
    if (!mbtn_in_range(mbtn_idx)) return;
    s_mbtns_event_held[mbtn_idx] = 0;
}

// Accumulated vertical wheel notches since the last consume. Filled from SDL
// wheel events (host.cpp), drained by input_mouse_wheel_consume. Consume-based
// like the relative-motion delta, not per-frame-reset.
static SLONG s_mouse_wheel_accum = 0;

void input_frame_on_mouse_wheel(SLONG dy)
{
    s_mouse_wheel_accum += dy;
}

SLONG input_mouse_wheel_consume()
{
    const SLONG v = s_mouse_wheel_accum;
    s_mouse_wheel_accum = 0;
    return v;
}

// ---- Keyboard ---------------------------------------------------------------

bool input_key_held(SLONG kb_code)
{
    return key_in_range(kb_code) && s_keys_curr[kb_code];
}

bool input_key_event_held(SLONG kb_code)
{
    return key_in_range(kb_code) && s_keys_event_held[kb_code];
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

void input_key_force_release(SLONG kb_code)
{
    if (key_in_range(kb_code)) {
        s_keys_curr[kb_code] = 0;
        s_keys_event_held[kb_code] = 0;
        s_keys_pressed_during_frame[kb_code] = 0;
        s_keys_press_pending[kb_code] = 0;
    }
}

void input_keyboard_consume_all_held_until_released()
{
    // Arm every key that's currently event-held. The next
    // input_frame_update applies the gate (forces curr to 0 and drains
    // press_pending) and clears the gate per-key when event_held has
    // fallen back to 0.
    for (SLONG i = 0; i < INPUT_KEY_COUNT; i++) {
        if (s_keys_event_held[i]) {
            s_keys_consume_until_released[i] = 1;
        }
    }
}

UBYTE input_last_key()
{
    return s_last_key;
}

void input_last_key_consume()
{
    s_last_key = 0;
}

// ---- Gamepad buttons --------------------------------------------------------

// Device-active gate: any gameplay/menu code reading gamepad state should
// see "no input" when the active device is keyboard+mouse. Without this
// gate, leftover gamepad poll state can drive game logic even after the
// user switches to keyboard (or disconnects the gamepad mid-session) —
// canonical breakage was a pause menu auto-cycling because the gamepad's
// last-known stick direction was treated as held.
//
// Internal gamepad polling and auto-switch detection (which set
// active_input_device based on physical input) run in input_frame_update
// BEFORE any of these getters fire, so the gate doesn't lock the user
// into KBM — the first real gamepad input flips active_input_device to
// PAD and the next getter call returns the live state.
static inline bool gamepad_input_gated()
{
    return active_input_device == INPUT_DEVICE_KEYBOARD_MOUSE;
}

bool input_btn_held(SLONG btn_idx)
{
    if (gamepad_input_gated()) return false;
    return btn_in_range(btn_idx) && s_btns_curr[btn_idx];
}

bool input_btn_just_pressed(SLONG btn_idx)
{
    if (gamepad_input_gated()) return false;
    return btn_in_range(btn_idx) && s_btns_curr[btn_idx] && !s_btns_prev[btn_idx];
}

bool input_btn_just_released(SLONG btn_idx)
{
    if (gamepad_input_gated()) return false;
    return btn_in_range(btn_idx) && !s_btns_curr[btn_idx] && s_btns_prev[btn_idx];
}

bool input_btn_press_pending(SLONG btn_idx)
{
    if (gamepad_input_gated()) return false;
    return btn_in_range(btn_idx) && s_btns_press_pending[btn_idx];
}

void input_btn_consume(SLONG btn_idx)
{
    if (btn_in_range(btn_idx)) {
        s_btns_press_pending[btn_idx] = 0;
    }
}

// ---- Mouse buttons ----------------------------------------------------------

bool input_mouse_btn_held(SLONG mbtn_idx)
{
    return mbtn_in_range(mbtn_idx) && s_mbtns_curr[mbtn_idx];
}

bool input_mouse_btn_just_pressed(SLONG mbtn_idx)
{
    return mbtn_in_range(mbtn_idx) && s_mbtns_curr[mbtn_idx] && !s_mbtns_prev[mbtn_idx];
}

bool input_mouse_btn_just_released(SLONG mbtn_idx)
{
    return mbtn_in_range(mbtn_idx) && !s_mbtns_curr[mbtn_idx] && s_mbtns_prev[mbtn_idx];
}

bool input_mouse_btn_press_pending(SLONG mbtn_idx)
{
    return mbtn_in_range(mbtn_idx) && s_mbtns_press_pending[mbtn_idx];
}

void input_mouse_btn_consume(SLONG mbtn_idx)
{
    if (mbtn_in_range(mbtn_idx)) {
        s_mbtns_press_pending[mbtn_idx] = 0;
    }
}

// Arm the wait-for-release gate on every currently-held mouse button.
// Mirror of input_keyboard_consume_all_held_until_released. Called from
// input_consume_all_held_until_released on UI overlay close so a mouse
// button held into the overlay doesn't fire as a fresh press on the
// first gameplay tick after resume.
static void input_mouse_btn_consume_all_held_until_released()
{
    for (SLONG i = 0; i < INPUT_MBTN_COUNT; i++) {
        if (s_mbtns_event_held[i]) {
            s_mbtns_consume_until_released[i] = 1;
        }
    }
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
    if (gamepad_input_gated()) return false;
    if (!stick_in_range(stick) || !dir_in_range(dir)) return false;
    return s_stick_dir_curr[stick][dir];
}

bool input_stick_just_pressed(InputStickId stick, InputStickDir dir)
{
    if (gamepad_input_gated()) return false;
    if (!stick_in_range(stick) || !dir_in_range(dir)) return false;
    return s_stick_dir_curr[stick][dir] && !s_stick_dir_prev[stick][dir];
}

bool input_stick_just_released(InputStickId stick, InputStickDir dir)
{
    if (gamepad_input_gated()) return false;
    if (!stick_in_range(stick) || !dir_in_range(dir)) return false;
    return !s_stick_dir_curr[stick][dir] && s_stick_dir_prev[stick][dir];
}

bool input_stick_just_pressed_or_repeat(InputStickId stick, InputStickDir dir)
{
    if (gamepad_input_gated()) return false;
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
    if (gamepad_input_gated()) return 0.0f;
    int raw = (stick == GAXIS_LEFT) ? gamepad_state.lX : gamepad_state.rX;
    return apply_stick_deadzone(raw);
}

float input_stick_y(InputStickId stick)
{
    if (gamepad_input_gated()) return 0.0f;
    int raw = (stick == GAXIS_LEFT) ? gamepad_state.lY : gamepad_state.rY;
    return apply_stick_deadzone(raw);
}

// ---- Triggers ---------------------------------------------------------------

float input_trigger(SLONG trigger_idx)
{
    if (gamepad_input_gated()) return 0.0f;
    if (trigger_idx == 15) return float(gamepad_state.trigger_left)  / 255.0f;
    if (trigger_idx == 16) return float(gamepad_state.trigger_right) / 255.0f;
    return 0.0f;
}

// ---- Raw axis / trigger / connection accessors -----------------------------

// NOTE: input_gamepad_connected() is NOT gated by active_input_device — it
// reports the physical connection state and is used by the auto-switch
// detector itself, plus by code that genuinely needs to know about hardware
// presence (e.g. UI hints, controller-icon glyphs). Gameplay/menu logic
// reading gamepad INPUT goes through the gated getters above.
bool input_gamepad_connected()
{
    return gamepad_state.connected != 0;
}

bool input_dpad_active()
{
    if (gamepad_input_gated()) return false;
    return gamepad_state.dpad_active != 0;
}

// Stick axes return CENTRE (32768) when the active device is keyboard+mouse.
// Same device-active gate as the button/direction getters — gameplay/menu
// code reads these to decide whether the user is deflecting the stick, and
// must see "no deflection" when the user is on KBM (otherwise stale or
// disconnected-gamepad state drives logic).
int input_stick_x_axis(InputStickId stick)
{
    if (gamepad_input_gated()) return 32768;
    return (stick == GAXIS_LEFT) ? gamepad_state.lX : gamepad_state.rX;
}

int input_stick_y_axis(InputStickId stick)
{
    if (gamepad_input_gated()) return 32768;
    return (stick == GAXIS_LEFT) ? gamepad_state.lY : gamepad_state.rY;
}

int input_stick_x_axis_raw(InputStickId stick)
{
    if (gamepad_input_gated()) return 32768;
    return (stick == GAXIS_LEFT) ? gamepad_state.lX_raw : gamepad_state.rX_raw;
}

int input_stick_y_axis_raw(InputStickId stick)
{
    if (gamepad_input_gated()) return 32768;
    return (stick == GAXIS_LEFT) ? gamepad_state.lY_raw : gamepad_state.rY_raw;
}

int input_trigger_raw(SLONG trigger_idx)
{
    if (gamepad_input_gated()) return 0;
    if (trigger_idx == 15) return gamepad_state.trigger_left;
    if (trigger_idx == 16) return gamepad_state.trigger_right;
    return 0;
}

// ---- Mouse ------------------------------------------------------------------

SLONG input_mouse_x()
{
    return MouseX;
}

SLONG input_mouse_y()
{
    return MouseY;
}

void input_mouse_consume_rel(SLONG* out_dx, SLONG* out_dy)
{
    if (out_dx)
        *out_dx = MouseRelDX;
    if (out_dy)
        *out_dy = MouseRelDY;
    MouseRelDX = 0;
    MouseRelDY = 0;
}

void input_mouse_drain_rel()
{
    MouseRelDX = 0;
    MouseRelDY = 0;
}

void input_drain_all_press_pending()
{
    memset(s_keys_press_pending, 0, sizeof(s_keys_press_pending));
    memset(s_btns_press_pending, 0, sizeof(s_btns_press_pending));
    memset(s_mbtns_press_pending, 0, sizeof(s_mbtns_press_pending));
}

// ---- Bulk consume after UI overlay close ------------------------------------

void input_consume_all_held_until_released()
{
    input_keyboard_consume_all_held_until_released();
    gamepad_consume_all_held_buttons_until_released();
    gamepad_consume_held_triggers_until_released();
    gamepad_consume_held_sticks_until_rest();
    input_mouse_btn_consume_all_held_until_released();
    input_mouse_drain_rel();

    // Also drop sticky press_pending. The wait-for-release gates above only
    // suppress reads while a button stays physically held; a press that was
    // already released by the time the helper runs (event_held back to 0)
    // would still surface as press_pending to the next consumer. Concrete
    // case this closes: player tapped a camera-mode key (F5..F7 / END / DEL
    // / PGDN) just before opening pause, didn't touch it again in the menu,
    // and on the first gameplay tick after resume the camera mode re-fires.
    input_drain_all_press_pending();
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

// ---- Debug modifier / gameplay gating ---------------------------------------
// See input_frame.h for design notes.

bool input_debug_modifier_active()
{
    // F1 doubles as the debug-help legend trigger AND the global debug
    // modifier. Holding F1 alone shows the help; F1+key fires a debug
    // action (and the help overlay auto-hides inside debug_help_tick when
    // it sees a non-F1 keypress while F1 is held). One key for both jobs
    // — the legend is most useful when you want to discover what to press,
    // and once you press it you don't need the overlay anymore.
    return allow_debug_keys && input_key_held(ACT_BANG_DEBUG_MODIFIER_KKEY);
}

bool input_gameplay_enabled()
{
    // Single future-proof gate. Returns false whenever any condition wants
    // gameplay input suppressed. Conditions:
    //   - F1 debug modifier held (gameplay vs. debug keys mutex)
    //   - Modal input debug panel open (panel owns input; gameplay frozen)
    // Add other "suppress gameplay" conditions here as needed; call sites
    // don't need per-condition checks.
    if (input_debug_modifier_active())
        return false;
    if (input_debug_is_active())
        return false;
    return true;
}
