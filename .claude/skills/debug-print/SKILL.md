---
name: debug-print
description: >
  On-screen debug text rendering — choosing the right function for displaying text in-game.
  TRIGGER when: about to write code that prints/displays/shows text on the game screen,
  user asks to add debug output, status display, or any on-screen text.
  Also trigger when about to use ANY of these functions: CONSOLE_text, CONSOLE_text_at,
  MSG_add, FONT2D_DrawString, draw_text_at, FONT_draw, FONT_buffer_add, PANEL_new_text,
  AENG_world_text, CONSOLE_status.
  This skill prevents wasted time from using non-working or inappropriate text rendering methods.
---

# Debug Print — On-Screen Text Reference

Before writing ANY code that displays text on the game screen, consult this reference
to pick the right method. Several methods exist but not all have been verified to work.
Using an unverified method wastes time — we've been burned by this before.

## Rules

- **Never guess** which text function to use — always consult this reference first
- **Only use verified methods** unless the user explicitly asks to test an unverified one
- If unsure, default to CONSOLE_text (always works) or MSG_add (needs debug mode)
- **Always tell the user what they need to do to see the output.** If the chosen method
  requires activation steps (e.g. MSG_add needs F9 → bangunsnotgames → Ctrl toggle),
  explain this to the user every time. The user may not remember or may not know how
  a particular rendering method works. Never assume the user knows — always remind them.

## Verified WORKING

### CONSOLE_text(text, delay)
- **Header:** `engine/console/console.h`
- **Conditions:** none — works unconditionally
- **Behavior:** queues HUD message, shown with delay between messages (default 4000ms)
- **Drawback:** big delay between messages, unsuitable for per-frame output
- **Use for:** cheat messages, one-off notifications, game events

### MSG_add(fmt, ...)
- **Header:** `engine/console/message.h`
- **Conditions:** requires `allow_debug_keys` (F9 → bangunsnotgames) + `ControlFlag` (Ctrl toggle)
- **Behavior:** scrolling debug log on the LEFT side of the screen — many messages visible at once
- **Key advantage:** the ONLY method that shows many messages simultaneously as a running log.
  All other methods show one message at a time. Use MSG_add when you need to continuously
  output data every frame and track changes over time (variable values, states, coordinates, etc.)
- **Use for:** per-frame debug tracing, state logging, variable inspection, continuous monitoring

### CONSOLE_status(text)
- **Header:** `engine/console/console.h`
- **Conditions:** none
- **Behavior:** persistent green text in top-left corner, does NOT auto-expire
- **Use for:** persistent status indicator

### PANEL_new_text(who, delay, fmt, ...)
- **Header:** `ui/hud/panel.h`
- **Conditions:** none
- **Behavior:** radio message with icon and sound effect. Same queue as CONSOLE_text.
- **Use for:** in-game radio messages, story events

### PANEL_new_info_message(fmt, ...)
- **Header:** `ui/hud/panel.h`
- **Conditions:** none
- **Behavior:** sliding message in bottom-left corner, auto-expires after ~2 sec
- **Use for:** short gameplay info, pickup notifications

## Not working (do not use)

| Method | Status | Details |
|--------|--------|---------|
| CONSOLE_text_at | ❌ not working | nothing visible |
| FONT2D_DrawString | ❌ not working | breaks character shadows |
| draw_text_at | ❌ partially | renders text on shadows, not on screen |
| FONT_buffer_add | ❌ not working | nothing visible |
| AENG_world_text | ❌ not working | uses FONT_buffer_add internally |
| FONT_draw | ❓ not tested | requires ge_lock_screen which returns NULL in OpenGL |

Details and investigation: `new_game_devlog/debug_text_rendering_test.md`

## How to enable debug overlay

1. F9 → type `bangunsnotgames` → Enter (enables `allow_debug_keys`)
2. Ctrl — toggle display (switches `debug_overlay_locked_on`)
3. Shift+F12 — toggle `cheat` variable (0↔2), shows FPS/coordinates via overlay.cpp
