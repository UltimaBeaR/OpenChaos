#pragma once

// Modal input debug panel.
//
// Opens a full-screen translucent overlay that blocks normal game input so
// the player can inspect and test input devices (keyboard, Xbox/generic
// gamepad, DualSense) without character actions firing.
//
// Toggle: F11. Exit: ESC. Page switch: 1/2/3 while open.

#include <cstdint>

#include "engine/input/gamepad.h"      // GamepadState, InputDeviceType
#include "engine/platform/uc_common.h"

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void input_debug_open();
void input_debug_close();
void input_debug_toggle();
bool input_debug_is_active();

// Per-frame: drive panel input (ESC exit, 1/2/3 page switch, future
// page-local controls). No-op when inactive.
void input_debug_tick();

// Per-frame: queue panel graphics into the HUD 2D batch. Call from the
// same spot as other HUD overlays — right after OVERLAY_handle, inside
// the render pass. No-op when inactive.
void input_debug_render();

// ---------------------------------------------------------------------------
// Pages
// ---------------------------------------------------------------------------

enum InputDebugPage {
    INPUT_DEBUG_PAGE_KEYBOARD = 0,
    INPUT_DEBUG_PAGE_GAMEPAD,      // Xbox / generic
    INPUT_DEBUG_PAGE_DUALSENSE,
    INPUT_DEBUG_PAGE_COUNT,
};

// Called from input_debug_render once the panel backdrop and header are
// drawn. Each page file implements one of these.
void input_debug_render_keyboard_page();
void input_debug_render_gamepad_page();
void input_debug_render_dualsense_page();

// ---------------------------------------------------------------------------
// Read-through wrappers for page code
// ---------------------------------------------------------------------------
//
// Pages call these instead of touching gamepad_state_raw() / Keys[] directly.
// When the page's device doesn't match the currently-active input source,
// the wrappers return a quiet "idle" value (sticks centred, zero buttons,
// zero triggers, key held = false). This lets page layout render in full
// while still hiding live input from inactive devices.

// Returns the raw gamepad snapshot when page_device == active input source,
// otherwise a static idle state (sticks at centre, everything else zero).
const GamepadState& input_debug_read_gamepad_for(InputDeviceType page_device);

// Returns Keys[scancode] when keyboard is the active input source,
// otherwise false. Use this from the keyboard page instead of Keys[] to
// silence inactive presses without losing layout.
bool input_debug_key_held(unsigned char scancode);

// DualSense trigger feedback readings. Returns 0 / false when DualSense
// is not the active input source, so indicators go dark on inactive DS.
// right_trigger: true = R2, false = L2.
// Feedback state: low nibble of the HID feedback byte (0..15).
uint8_t input_debug_read_ds_feedback(bool right_trigger);
// Effect-active bit (bit 4 of the feedback byte): the trigger is currently
// engaged with its adaptive-trigger resistance.
bool    input_debug_read_ds_effect_active(bool right_trigger);

// ---------------------------------------------------------------------------
// Navigation edge-detect (shared across all interactive widgets)
// ---------------------------------------------------------------------------
//
// The panel runs a single edge detect for the arrow / Enter keys once per
// frame. Widgets never edge-detect themselves — they read `input_debug_nav`
// and act on its fields when their row is focused. Page code drives its
// cursor from the same struct.
struct InputDebugNav {
    bool up;
    bool down;
    bool left;
    bool right;
    bool enter;
};

const InputDebugNav& input_debug_nav();

// ---------------------------------------------------------------------------
// Rumble test widget (shared between gamepad + DualSense pages)
// ---------------------------------------------------------------------------
//
// Occupies 3 menu rows: low motor, high motor, fire pulse. When the
// supplied `local_cursor` is in [0..2] the widget treats that row as
// focused and acts on the current input_debug_nav edges (left/right on
// numeric rows, Enter on the action row). Returns the row count (3) so
// the caller can advance its own page-level cursor offset.
int input_debug_render_rumble_test(SLONG x, SLONG y, int local_cursor);

// ---------------------------------------------------------------------------
// DualSense-only read-through wrappers
// ---------------------------------------------------------------------------

// Touchpad finger positions. Returns (0, 0, false) for each finger when
// DualSense is not the active input source. Coordinates are raw hardware
// pixels (X 0..1919, Y 0..1079). `down` is the contact flag.
void input_debug_read_ds_touchpad(int* f1_x, int* f1_y, bool* f1_down,
                                  int* f2_x, int* f2_y, bool* f2_down);

// Reset DualSense page widget state — called on panel open/close so the
// lightbar / player LED tests start from a clean slate and don't leave
// the controller in a user-set state after exit. Implemented in
// input_debug_dualsense.cpp.
void input_debug_dualsense_reset_state();

// ---------------------------------------------------------------------------
// Widget helpers (shared between pages)
// ---------------------------------------------------------------------------
//
// All widgets queue into the POLY batch opened by input_debug_render —
// they do NOT wrap in their own PANEL_start/PANEL_finish. Colours are
// 0xRRGGBB on POLY_PAGE_COLOUR. Text goes through FONT_buffer_add which
// is independent of the POLY batch.

// Analog-stick box with a red dot at the current deflection.
// cx, cy = centre of the box. size = width/height. nx, ny = normalized
// deflection in [-1..+1] (as from (lX - 32768)/32768).
void input_debug_draw_stick(SLONG cx, SLONG cy, SLONG size,
                            float nx, float ny,
                            const char* label);

// Vertical trigger bar. Fill grows from the bottom proportional to
// value/max. w, h = bar dimensions. label shown below the bar with
// the numeric value.
void input_debug_draw_trigger_bar(SLONG x, SLONG y, SLONG w, SLONG h,
                                  int value, int max_val,
                                  const char* label);

// Single-line button state: bright green when pressed, dim grey when not.
// Useful for dense grids of buttons in a page.
void input_debug_draw_button(SLONG x, SLONG y,
                             const char* label, bool pressed);

// Checkbox-style indicator for a boolean flag: "[x] label" on, "[ ] label" off.
// Useful for DualSense feedback act bits and similar single-bit states.
void input_debug_draw_checkbox(SLONG x, SLONG y,
                               const char* label, bool on);

// Text rendering that matches the rects drawn by AENG_draw_rect. The
// underlying FONT_buffer_add writes to the backbuffer at literal window
// pixels, while AENG_draw_rect uses logical 640x480 that the pipeline
// scales up. This wrapper takes logical coords and scales them to pixel
// space so text lines up with the rects. ALWAYS use this (not raw
// FONT_buffer_add) inside the panel.
void input_debug_text(SLONG x_logical, SLONG y_logical,
                      unsigned char r, unsigned char g, unsigned char b,
                      unsigned char shadow,
                      const char* fmt, ...);
