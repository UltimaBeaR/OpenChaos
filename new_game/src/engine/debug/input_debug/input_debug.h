#pragma once

// Modal input debug panel.
//
// Opens a full-screen black overlay that blocks normal game input so the
// player can inspect and test input devices (keyboard, generic gamepad,
// DualSense) without character actions firing.
//
// Iteration 1: skeleton only — backdrop, enter/exit, input block, no pages.
//
// Toggle: F11. Exit: ESC.

void input_debug_open();
void input_debug_close();
void input_debug_toggle();
bool input_debug_is_active();

// Called every frame from the game loop; handles panel-local input
// (ESC to exit, future page switching and per-page controls).
// No-op when the panel is not active.
void input_debug_tick();

// Queues the panel graphics for the current frame. Must be called from the
// same place as other HUD overlays (before draw_screen flushes the 2D
// queue), same as gamepad_debug_draw. No-op when the panel is not active.
void input_debug_render();
