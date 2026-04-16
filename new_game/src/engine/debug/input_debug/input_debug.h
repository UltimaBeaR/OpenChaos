#pragma once

// Modal input debug panel.
//
// Opens a full-screen translucent overlay that blocks normal game input so
// the player can inspect and test input devices (keyboard, Xbox/generic
// gamepad, DualSense) without character actions firing.
//
// Toggle: F11. Exit: ESC. Page switch: 1/2/3 while open.

#include <cstdint>

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
