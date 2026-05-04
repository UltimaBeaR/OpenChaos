#ifndef ENGINE_INPUT_INPUT_FRAME_H
#define ENGINE_INPUT_INPUT_FRAME_H

#include "engine/core/types.h"

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

// ---- Gamepad buttons --------------------------------------------------------
// btn_idx = index into gamepad_state.rgbButtons[] (0..31).
// Common indices: 0=Cross/A, 1=Circle/B, 2=Square/X, 3=Triangle/Y, 6=Start,
// 9=L1, 10=R1, 11..14=DPad up/down/left/right, 15=L2 digital, 16=R2 digital.

bool input_btn_held(SLONG btn_idx);
bool input_btn_just_pressed(SLONG btn_idx);
bool input_btn_just_released(SLONG btn_idx);

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

// ---- Internal: SDL event hook ----------------------------------------------
// Called from keyboard_key_down() on key press to set sticky press_pending.

void input_frame_on_key_down(UBYTE scancode);

#endif // ENGINE_INPUT_INPUT_FRAME_H
