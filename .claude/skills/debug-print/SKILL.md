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

## Verified methods

### CONSOLE_text(text, delay)
- **Header:** `engine/console/console.h`
- **Conditions:** none — works unconditionally
- **Behavior:** queues HUD message, shown with delay between messages (default 4000ms)
- **Drawback:** big delay between messages, unsuitable for per-frame output
- **Use for:** cheat messages, one-off notifications, game events
- **Status:** ✅ verified working

### MSG_add(fmt, ...)
- **Header:** `engine/console/message.h`
- **Conditions:** requires `allow_debug_keys` (F9 → bangunsnotgames) + `ControlFlag` (Ctrl toggle)
- **Behavior:** scrolling debug log on the left side of the screen
- **Rendering path:** MSG_draw() called from AENG_draw_messages() in screen_flip()
- **Use for:** per-frame debug tracing, state logging, variable inspection
- **Status:** ✅ verified working

## Unverified methods (may or may not render on screen)

These exist in code and look functional but have NOT been tested. Do not use without
explicit user confirmation that they want to try one.

### CONSOLE_text_at(x, y, delay, fmt, ...)
- **Header:** `engine/console/console.h`
- **Conditions:** none
- **Behavior:** message at fixed screen position, replaces previous at same position

### CONSOLE_status(text)
- **Header:** `engine/console/console.h`
- **Conditions:** none
- **Behavior:** persistent string in top-left corner

### FONT2D_DrawString(str, x, y, colour, fade)
- **Header:** `engine/graphics/text/font2d.h`
- **Conditions:** none
- **Behavior:** fast text via texture atlas, renders for one frame only (must call every frame)
- **Variants:** FONT2D_DrawStringCentred(), FONT2D_DrawStringWrap(), FONT2D_DrawString_3d()

### draw_text_at(x, y, message, font_id)
- **Header:** `engine/graphics/text/text.h`
- **Conditions:** none
- **Behavior:** proportional font via D3D quads, nice looking
- **Variants:** draw_centre_text_at()

### FONT_draw(x, y, fmt, ...) / FONT_draw_coloured_text()
- **Header:** `engine/graphics/text/font.h`
- **Conditions:** requires ge_lock_screen()/ge_unlock_screen()
- **Behavior:** bitmap text (yellow with red shadow)

### FONT_buffer_add() / FONT_buffer_draw()
- **Header:** `engine/graphics/text/font.h`
- **Conditions:** none (locks screen internally)
- **Behavior:** batched bitmap text rendering

### PANEL_new_text() / PANEL_new_info_message()
- **Header:** `ui/hud/panel.h`
- **Conditions:** none
- **Behavior:** floating text above character / short info at bottom of HUD (2 sec)

### AENG_world_text(x, y, z, fmt, ...)
- **Header:** `engine/graphics/pipeline/aeng.h`
- **Conditions:** ControlFlag && allow_debug_keys (at call sites)
- **Behavior:** 3D text in world coordinates, projected to screen

## How to enable debug overlay

1. F9 → type `bangunsnotgames` → Enter (enables `allow_debug_keys`)
2. Ctrl — toggle display (switches `debug_overlay_locked_on`)
3. Shift+F12 — toggle `cheat` variable (0↔2), shows FPS/coordinates via overlay.cpp
