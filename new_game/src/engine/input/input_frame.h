#ifndef ENGINE_INPUT_INPUT_FRAME_H
#define ENGINE_INPUT_INPUT_FRAME_H

#include "engine/core/types.h"

#include <cstdint>

// =============================================================================
// Per-frame input state — captured at start of each render frame, exposed to
// all consumers as a unified API. Replaces ad-hoc edge-detect / sticky / auto-
// repeat patterns scattered across menu, gameplay, debug-key code paths.
//
// Runs inside LibShellActive() once per render frame after SDL events have
// been pumped, so:
//   - keyboard snapshot reflects this-frame events (incl. same-frame
//     press+release latching via s_keys_pressed_during_frame, set by the
//     SDL key-down hook and OR'd into curr inside input_frame_update)
//   - gamepad poll happens here too, then snapshot is captured
//
// Computes:
//   - just_pressed   — rising edge between prev and curr snapshot (one frame)
//   - just_released  — falling edge
//   - press_pending  — sticky: set in SDL key-down handler, cleared on consume
//
// Sticky press_pending exists ONLY for keyboard. Gamepad is polled (not
// event-driven) so fast presses between two polls are invisible regardless;
// for gamepad use just_pressed.
//
// Plan & migration list → new_game_devlog/input_system/current_plan.md.
// =============================================================================

void input_frame_init();

// Called once per render frame from LibShellActive() after sdl3_poll_events.
// Polls gamepad, then snapshots keyboard + gamepad and computes edges.
void input_frame_update();

// ---- Keyboard ---------------------------------------------------------------

bool input_key_held(SLONG kb_code);
bool input_key_just_pressed(SLONG kb_code);
bool input_key_just_released(SLONG kb_code);

// Live event-tracked held state: reflects the most recent SDL key-down /
// key-up event immediately, without waiting for the next
// input_frame_update() snapshot. Use ONLY from inside SDL event handlers
// where the just-applied event must be visible to subsequent code in the
// same call (e.g. keyboard.cpp::update_modifier_flags recomputing
// ShiftFlag/ControlFlag). Most consumers should use
// input_key_held instead — it's snapshot-stable across the frame and
// won't see mid-frame state flips.
bool input_key_event_held(SLONG kb_code);

// Sticky: was there a key-down event since the last consume? Survives across
// frames (unlike just_pressed which is true for exactly one frame). Use when
// the consumer may not run every frame (e.g. physics tick which can be skipped
// when the accumulator hasn't filled) and a press must not be missed.
bool input_key_press_pending(SLONG kb_code);
void input_key_consume(SLONG kb_code);

// Force a synthesised release: clear the current snapshot, event-held flag,
// and press_pending — so subsequent `input_key_held` / `input_key_just_pressed`
// reads in the SAME frame see the key as not held. Use when one consumer
// "consumes" a key for its action and wants downstream consumers in the same
// render frame to NOT see the press leak (e.g. pause menu's SPACE confirm
// must not also fire INPUT_MASK_JUMP via get_hardware_input level read on
// the same frame). The next physical key-down event from SDL re-arms
// normally — synthesis only suppresses CURRENT held-state, not future presses.
void input_key_force_release(SLONG kb_code);

// Mark all keys currently event-held (i.e. pressed by SDL key-down with
// no matching key-up yet) to be ignored until they physically release.
// While armed, input_key_held / input_key_just_pressed / input_key_press_pending
// all return false for the key, and the underlying snapshot is forced to 0.
// Auto-clears on the next snapshot where event_held has fallen back to 0
// (SDL key-up arrived). Use when closing a UI overlay so a key held into
// gameplay doesn't fire as a fresh press in the first gameplay tick.
void input_keyboard_consume_all_held_until_released();

// Returns the scancode of the most recent SDL key-down event since the
// last input_last_key_consume(). Returns 0 if no press is pending.
//
// For text-input use cases that care about WHICH scancode was pressed
// (rebind UI, debug console text entry, skip-detection that compares the
// scancode against a list). Standard action / hotkey checks should use
// input_key_just_pressed which is per-key edge-detect.
UBYTE input_last_key();
void input_last_key_consume();

// ---- Gamepad buttons --------------------------------------------------------
// btn_idx = index into gamepad_state.rgbButtons[] (0..31).
// Common indices: 0=Cross/A, 1=Circle/B, 2=Square/X, 3=Triangle/Y, 6=Start,
// 9=L1, 10=R1, 11..14=DPad up/down/left/right, 15=L2 digital, 16=R2 digital.

bool input_btn_held(SLONG btn_idx);
bool input_btn_just_pressed(SLONG btn_idx);
bool input_btn_just_released(SLONG btn_idx);

// Sticky: was there a rising edge on this gamepad button since the last
// consume? Survives across frames (unlike just_pressed which is true for
// exactly one render frame). Use when the consumer runs on physics tick
// (NOT render frame): at low physics Hz a render-frame-only just_pressed
// may have already cleared by the time the next physics tick samples it.
//
// Drain rule: a consumer that wants pending semantics must consume() the
// flag in EVERY relevant frame whether or not it acted on it, otherwise
// stale flags from outside the consumer's active window leak in. Typical
// pattern: drain at every physics tick during the consumer's active state
// (e.g. while driving for siren), drain at every tick when NOT in active
// state too if the same physical button has another use elsewhere
// (Triangle = menu cancel + car siren; SPACE = jump + car siren).
bool input_btn_press_pending(SLONG btn_idx);
void input_btn_consume(SLONG btn_idx);

// ---- Auto-repeat (Phase 3 — currently stubbed to just_pressed only) --------

bool input_key_just_pressed_or_repeat(SLONG kb_code);
bool input_btn_just_pressed_or_repeat(SLONG btn_idx);

// ---- Stick virtual directions ----------------------------------------------
//
// Stick id and direction are passed as `int` — values come from the
// GAXIS_LEFT / GAXIS_RIGHT and GDIR_UP / DOWN / LEFT / RIGHT constants in
// game/action_map/input_codes.h. Was an `enum InputStickId / InputStickDir`
// pair; replaced with a typedef because the #defines in input_codes.h now own
// the names (which keeps the action-map device-code list authoritative).

using InputStickId  = int;
using InputStickDir = int;

bool input_stick_held(InputStickId stick, InputStickDir dir);
bool input_stick_just_pressed(InputStickId stick, InputStickDir dir);
bool input_stick_just_released(InputStickId stick, InputStickDir dir);
bool input_stick_just_pressed_or_repeat(InputStickId stick, InputStickDir dir);

// ---- Stick continuous values ------------------------------------------------
// Returns float [-1.0, 1.0] with deadzone applied. Up = -1, Down = +1 on Y
// (matches the raw gamepad_state convention: lY=0 for up).

float input_stick_x(InputStickId stick);
float input_stick_y(InputStickId stick);

// ---- Triggers ---------------------------------------------------------------
// trigger_idx: 15 = L2, 16 = R2 (matches rgbButtons indices for digital path).
// Returns float [0.0, 1.0].

float input_trigger(SLONG trigger_idx);

// ---- Mouse buttons ----------------------------------------------------------
// mbtn_idx = MBTN_LEFT / MBTN_MIDDLE / MBTN_RIGHT (0/1/2). Wired from SDL
// mouse-button events in host.cpp::on_mouse_button — only events that occur
// AFTER mouse capture is engaged are forwarded; the engagement click itself
// is consumed by mouse_capture_on_button and never reaches input_frame.
// On capture disengage (UI overlay open, focus loss), held buttons are
// scrubbed by input_consume_all_held_until_released so they don't leak
// across the transition.

bool input_mouse_btn_held(SLONG mbtn_idx);
bool input_mouse_btn_just_pressed(SLONG mbtn_idx);
bool input_mouse_btn_just_released(SLONG mbtn_idx);

// Sticky: was there a rising edge since the last consume? Same semantics as
// input_btn_press_pending for the gamepad.
bool input_mouse_btn_press_pending(SLONG mbtn_idx);
void input_mouse_btn_consume(SLONG mbtn_idx);

// ---- Mouse motion / position ------------------------------------------------
// Wrappers around mouse_globals (cursor position) filled by mouse.cpp from
// SDL3 events. Single source of truth for mouse reads.

// Cursor position in scene-FBO pixel coordinates (post-composition mapping).
SLONG input_mouse_x();
SLONG input_mouse_y();

// Read and reset the accumulated relative mouse-motion delta. Window-
// pixel units, signed. Call once per tick from the camera (or any other
// per-frame consumer); subsequent calls within the same tick return 0
// until the next mouse event fires. Consumers MUST gate this on
// mouse_capture_is_active() — when capture is off the delta is OS
// cursor motion, not gameplay input.
void input_mouse_consume_rel(SLONG* out_dx, SLONG* out_dy);

// Drop the accumulated mouse relative-motion delta on the floor. Same as
// input_mouse_consume_rel but discards the value. Use when closing a UI
// overlay so motion that accumulated while mouse capture was off
// (overlay open) doesn't fire as one big camera swing on the first
// gameplay tick after capture re-engages.
void input_mouse_drain_rel();

// ---- Raw axis / trigger / connection accessors -----------------------------
// Lower-level wrappers around gamepad_state for callers that need integer /
// byte semantics (bit-packing into Player->Pressed, weapon_feel byte
// thresholds) rather than float / virtual-direction APIs.
//
// Use these instead of touching gamepad_state directly so that input_frame
// remains the single source of truth for hardware input reads. See
// input_system/migration_checklist.md for the wider migration policy.

bool input_gamepad_connected();

// True iff the D-Pad is currently held in any direction. When true, the
// gamepad layer clamps lX/lY to 0/65535 so input_stick_*_axis below picks up
// the D-Pad as full-deflection movement. Use this to gate analog-mode logic
// (bit-packing, proportional speed) which only makes sense for the actual
// stick, not for D-Pad-as-stick.
bool input_dpad_active();

// Raw axis value 0..65535 (centre 32768), POST-override — the D-Pad clamps
// lX/lY to 0 / 65535 when held, so this value reflects D-Pad-as-stick.
//   - Bit-packing into Player->Pressed bits 18-31 (analog magnitude consumed
//     by process_analogue_movement).
//   - Level-trigger digital movement flags (LEFT/RIGHT/FORWARDS/BACKWARDS)
//     where the same threshold applies for both stick and D-Pad-as-stick.
// Distinct from input_stick_x/y (float -1..1, deadzone applied — for
// proportional movement) and from input_stick_held (pre-override raw + 4096
// menu threshold — for menu nav where stick + D-Pad must be independent
// signals for antagonist suppression).
int input_stick_x_axis(InputStickId stick);
int input_stick_y_axis(InputStickId stick);

// Same units as input_stick_*_axis, but PRE-D-Pad-override (i.e. lX_raw /
// lY_raw / rX_raw / rY_raw). Use when stick and D-Pad must be treated as
// independent input sources — typically menu code that ORs all three sources
// (keyboard + stick + D-Pad) for navigation and reads the physical stick
// deflection separately for dominant-axis disambiguation. Game movement
// should use input_stick_*_axis (post-override) so D-Pad-as-stick works.
int input_stick_x_axis_raw(InputStickId stick);
int input_stick_y_axis_raw(InputStickId stick);

// Raw trigger byte 0..255 (idx 15 = L2, 16 = R2). Same units as
// gamepad_state.trigger_left/right. For weapon_feel_evaluate_fire which
// uses byte-resolution thresholds; float input_trigger() loses precision
// through round-trip.
int input_trigger_raw(SLONG trigger_idx);

// ---- Combined-source auto-repeat -------------------------------------------
//
// Single auto-repeat throttle on a caller-supplied "any held" boolean.
// Use when one logical action has multiple physical sources (e.g. menu nav
// down = keyboard down OR stick down OR D-pad down) and you want exactly one
// auto-repeat cadence on the combined input — not separate per-source timers
// that could interleave to a 2× rate when several sources are held at once.
//
// Treats all sources as one combined entity:
//   any_held: rising → fire + start initial delay
//   any_held: falling → disarm (next press starts fresh)
//   any_held stays true while sources change (one held, second pressed,
//   first released, etc.) → no edge, timer continues uninterrupted
//
// Caller computes the combined bool by OR'ing input_*_held() results.
// One InputAutoRepeat instance per logical action, typically a static local
// or a member of the consumer's state.
//
//   static InputAutoRepeat ar_down;
//   bool nav_down = ar_down.tick_combined(
//       input_key_held(KKEY_DOWN)
//    || input_stick_held(GAXIS_LEFT, GDIR_DOWN));
struct InputAutoRepeat {
    bool     was_held  = false;
    bool     armed     = false;
    uint64_t next_fire = 0;

    // Returns true on a genuine combined rising edge AND on each auto-repeat
    // tick while combined input is held. Cadence: INPUT_REPEAT_INITIAL_MS for
    // the first auto-repeat after rising edge, INPUT_REPEAT_PERIOD_MS for
    // subsequent ticks (single source of truth for menu auto-repeat).
    //
    // Caller passes two booleans:
    //   any_just_pressed — OR of input_*_just_pressed() across sources, i.e.
    //                      "did any source see a rising edge in this frame's
    //                      snapshot". From input_frame snapshot, NOT from
    //                      tick_combined's own state.
    //   any_held         — OR of input_*_held() across sources.
    //
    // Logic uses (any_just_pressed && !was_held) as the rising edge of the
    // COMBINED input — so adding a second source while one is already held
    // doesn't restart the timer (was_held is already true), and a held-from-
    // before-context input doesn't fire on first call (any_just_pressed is
    // false because no rising edge happened in this frame's snapshot).
    bool tick_combined(bool any_just_pressed, bool any_held);
};

// Drop every sticky press_pending flag (keyboard + gamepad) at once. Use
// at a transition where stale press_pending from before the transition
// must not leak into post-transition consumers — e.g. on pause-menu open
// (so a Cross press the player made to jump just before opening pause
// doesn't immediately surface to the menu as Resume) and at the moment
// the pause slowdown ramp completes (so any press the player made during
// the ramp doesn't carry into the now-active menu).
//
// Unlike input_consume_all_held_until_released this does NOT scrub
// gamepad_state — held buttons / triggers / sticks pass through to
// gameplay as normal, so the slowdown ramp animation is not interrupted.
void input_drain_all_press_pending();

// ---- Bulk consume after UI overlay close -----------------------------------
// One-shot helper that arms wait-for-release / wait-for-rest on every
// currently active input source AND drains the mouse delta:
//
//   - keyboard keys held when called (input_keyboard_consume_all_held_until_released)
//   - gamepad buttons currently down (gamepad_consume_all_held_buttons_until_released)
//   - gamepad triggers above the rest threshold (gamepad_consume_held_triggers_until_released)
//   - gamepad sticks deflected beyond the rest tolerance (gamepad_consume_held_sticks_until_rest)
//   - accumulated mouse relative motion (input_mouse_drain_rel)
//
// Call when a UI overlay (pause menu, mission won/lost, "are you sure?")
// closes back to gameplay so nothing held into the overlay leaks into
// the first gameplay tick. Each gated source clears its own gate when
// the hardware reports it back to rest — the user has to physically
// release and re-press to fire a fresh gameplay action.
void input_consume_all_held_until_released();

// ---- Internal: SDL event hooks ---------------------------------------------
// Called from keyboard event handlers. Maintain an independent held-state
// array driven only by these events, decoupled from the public Keys[] array
// which consumers may mutate (e.g. menu handlers clearing Keys[KKEY_X] = 0
// after consume). Without that decoupling, a consumer's clear leaks into
// the next frame's snapshot and breaks auto-repeat for held keys.

void input_frame_on_key_down(UBYTE scancode);
void input_frame_on_key_up(UBYTE scancode);

// Called from host.cpp::on_mouse_button after mouse_capture_on_button has
// declined to consume the click. button is in MBTN_LEFT/MIDDLE/RIGHT (0/1/2).
void input_frame_on_mouse_button_down(SLONG mbtn_idx);
void input_frame_on_mouse_button_up(SLONG mbtn_idx);

// ---- Debug modifier / gameplay gating ---------------------------------------
//
// In `bangunsnotgames` debug mode, the player holds F1 as the global debug
// modifier. F1 doubles as the help-legend trigger: held alone (no other
// key pressed) it shows the overlay for 5 seconds; F1+key fires a debug
// action and the overlay auto-hides on the first non-F1 keypress.
//
// While F1 is held in debug mode:
//   - ALL bangunsnotgames hotkeys fire only on F1+key (suppresses individual
//     conflicts with WASD/E/S/D/Tab/1-8/etc.)
//   - Gameplay input is suppressed at every entry point — so pressing W
//     during F1-hold doesn't make Darci move forward AND spawn water
//     particles at the same time.
//
// input_debug_modifier_active() — true when F1 is held AND debug mode is on.
// Used at each `if (allow_debug_keys && ...)` call site (replaces the bare
// allow_debug_keys check). Same one-stop helper everywhere.
//
// input_gameplay_enabled() — opposite gate for gameplay-input call sites.
// Currently just `!input_debug_modifier_active()`, but designed as a single
// future-proof gate: if other "suppress gameplay" conditions ever arise
// (modal dialog open, cutscene driving the camera, etc.) they extend this
// helper, no per-site edit needed.

bool input_debug_modifier_active();
bool input_gameplay_enabled();

#endif // ENGINE_INPUT_INPUT_FRAME_H
