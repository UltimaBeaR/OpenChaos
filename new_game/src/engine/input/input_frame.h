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
//     press+release latching from keyboard.cpp's Released[] mechanism)
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

// ---- Stick virtual directions (Phase 2 — currently stubbed) ----------------

enum InputStickId {
    INPUT_STICK_LEFT = 0,
    INPUT_STICK_RIGHT,
};

enum InputStickDir {
    INPUT_STICK_DIR_UP = 0,
    INPUT_STICK_DIR_DOWN,
    INPUT_STICK_DIR_LEFT,
    INPUT_STICK_DIR_RIGHT,
};

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
//       input_key_held(KB_DOWN)
//    || input_stick_held(INPUT_STICK_LEFT, INPUT_STICK_DIR_DOWN));
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

// ---- Internal: SDL event hooks ---------------------------------------------
// Called from keyboard event handlers. Maintain an independent held-state
// array driven only by these events, decoupled from the public Keys[] array
// which consumers may mutate (e.g. menu handlers clearing Keys[KB_X] = 0
// after consume). Without that decoupling, a consumer's clear leaks into
// the next frame's snapshot and breaks auto-repeat for held keys.

void input_frame_on_key_down(UBYTE scancode);
void input_frame_on_key_up(UBYTE scancode);

#endif // ENGINE_INPUT_INPUT_FRAME_H
