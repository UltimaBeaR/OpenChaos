#pragma once

// Debug hotkey legend overlay.
//
// Lightweight on-screen help that lists every developer-only key. Shown
// for 5 seconds when:
//   - the user presses F1 (while bangunsnotgames is active), or
//   - bangunsnotgames is toggled on (so the user knows immediately what
//     the mode unlocks).
//
// No modal behaviour — the overlay just draws text in a corner and
// disappears when the timer elapses. Game input continues normally
// while it's visible.

void debug_help_tick(); // call every frame (edge-detect F1, tick timer)
void debug_help_render(); // call inside HUD pass; no-op when hidden
void debug_help_show(float seconds); // explicit show (used by bangunsnotgames toggle)
void debug_help_notify_bangunsnotgames_on(); // called from parse_console when the cheat turns on
