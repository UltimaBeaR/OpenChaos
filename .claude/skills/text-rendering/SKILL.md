---
name: text-rendering
description: >
  In-game text rendering reference — all functions for displaying text on screen.
  TRIGGER when about to write or modify ANY code that puts text on screen, even
  incidentally: primary task is drawing text, OR task is something else (new
  debug panel, overlay, menu, HUD widget, status line, tooltip, message popup)
  and it happens to need a text label. Trigger even for one-off debug prints,
  placeholder labels, or "just one line of text" inside a larger non-text task.
  Also trigger when investigating or explaining how existing text rendering
  works, when choosing between text functions, or when shadow-corruption /
  text-invisible bugs come up and text rendering is suspected.
  Also trigger when encountering, copying, or about to write ANY of these
  functions: CONSOLE_text, CONSOLE_text_at, MSG_add, FONT2D_DrawString,
  draw_text_at, FONT_draw, FONT_draw_coloured_text, FONT_buffer_add,
  PANEL_new_text, AENG_world_text, CONSOLE_status, PANEL_new_info_message,
  MENUFONT_Draw.
  Also trigger on keywords (English or Russian): "show text", "display text",
  "print on screen", "on-screen message", "debug print", "HUD label", "status
  line", "tooltip", "popup text", "screen text", "print to game", "вывести на
  экран", "показать текст", "нарисовать текст", "написать на экране",
  "отладочный вывод", "показать значение", "экранная печать".
  DO NOT use for non-text 2D primitives (rects, bars, sprites) — use the
  `hud-rendering` skill instead.
  This skill prevents wasted time from using wrong or inappropriate text
  methods and from shadow-corruption bugs.
---

# Text Rendering — Complete Reference

All functions for displaying text on screen. Covers game-tick-safe methods (for debug
and gameplay), render-pass-only internals, and menu rendering.

## Quick decision guide

| Need | Function | Works from game tick? |
|------|----------|----------------------|
| Text at screen position with timeout | `CONSOLE_text_at` | ✅ yes |
| Persistent status text | `CONSOLE_status` | ✅ yes |
| Per-frame scrolling log (many lines) | `MSG_add` | ✅ yes (debug overlay) |
| Radio message with sound | `PANEL_new_text` | ✅ yes |
| Short sliding notification | `PANEL_new_info_message` | ✅ yes |
| One-off HUD message | `CONSOLE_text` | ✅ yes |
| Pixel text in render path | `FONT_draw` / `FONT_draw_coloured_text` | ❌ render path only |
| Deferred pixel text (queued) | `FONT_buffer_add` | ✅ yes (flushed in render) |
| 3D world-space label | `AENG_world_text` | ❌ render pass only |
| Menu text | `MENUFONT_Draw` | N/A (menu pipeline) |

## Rules

- **Never guess** which text function to use — always consult this reference first.
- **Only use game-tick-safe methods** for debug output unless you are writing render-pass code.
- **Always tell the user what they need to do to see the output.** If the method requires
  activation steps (e.g. MSG_add needs F9 → bangunsnotgames → Ctrl), explain this every time.
  Never assume the user knows — always remind them.

## Where text works and doesn't

| Context | Game-tick-safe funcs | Render-pass funcs | Pixel funcs (FONT_draw) |
|---------|---------------------|-------------------|------------------------|
| In-game | ✅ | ✅ | ✅ |
| Pause menu (overlay) | ✅ | ✅ | ✅ |
| Main menu (attract) | ❌ | ❌ | ✅ (need manual insert in attract.cpp) |

**Main menu** has its own render loop (`game_attract_mode` in `attract.cpp`). It does NOT call
`CONSOLE_draw()`, `FONT_buffer_draw()`, or the game loop in `game.cpp`. Only `MENUFONT_Draw`
and `FONT_draw` (with manual `ge_lock_screen` before `AENG_flip`) work there.

**Pause menu** is an overlay on top of the game render — same render pipeline as in-game.
All in-game text methods work in pause menu.

---

## Game-tick-safe functions

These can be called from anywhere in game logic (game tick, AI, physics, input handlers).
They queue text for later rendering in the appropriate render phase.

### CONSOLE_text_at(x, y, delay, fmt, ...)
- **Header:** `engine/console/console.h`
- **Conditions:** none — works unconditionally
- **Behavior:** white text at specific screen pixel coordinates (x, y).
- **Timing:** auto-expires after `delay` ms (e.g. 3000 = 3 sec).
- **Capacity:** up to `CONSOLE_MAX_MESSES` (16) simultaneous positioned messages.
- **Same position:** if a message already exists at the same (x, y) — text is replaced,
  timer is reset. Different (x, y) — separate slot.
- **Internals:** queues into `CONSOLE_mess[]`, rendered by `CONSOLE_draw()` via `FONT2D_DrawString`
  during the render phase (inside `POLY_frame_init/draw`).
- **Best for:** positioned debug text, variable display at fixed screen location.

### CONSOLE_status(text)
- **Header:** `engine/console/console.h`
- **Conditions:** none — works unconditionally
- **Behavior:** persistent green text in top-left corner (10, 10).
- **Timing:** does **NOT auto-expire**. Stays on screen indefinitely until explicitly cleared.
  This is by design — internally it's just `strcpy` into a static buffer, no timer.
- **To clear:** call `CONSOLE_status((CBYTE*)"")`.
- **Only one message:** each call overwrites the previous. No queue.
- **Best for:** persistent status indicator ("mode X enabled", "recording ON").

### MSG_add(fmt, ...)
- **Header:** `engine/console/message.h`
- **Conditions:** requires `allow_debug_keys` (F9 → type `bangunsnotgames` → Enter)
  **AND** `ControlFlag` (Ctrl key toggle). Both must be active.
- **Behavior:** scrolling debug log on the LEFT side of the screen. Many messages visible at once.
- **Timing:** messages scroll off as new ones arrive. No fixed timer per message.
- **Key advantage:** the ONLY method that shows many messages simultaneously as a running log.
  All other methods show one message at a time or have limited slots.
- **Best for:** per-frame debug tracing, state logging, continuous variable monitoring.
- **⚠️ Always remind the user:** "To see MSG_add output: F9 → bangunsnotgames → Enter → Ctrl."

### CONSOLE_text(text, delay)
- **Header:** `engine/console/console.h`
- **Conditions:** none — works unconditionally
- **Behavior:** routes through `PANEL_new_text` internally — same radio message system.
- **Timing:** `delay` ms on screen (default 4000 ms). Long delay between messages.
- **Drawback:** unsuitable for per-frame output — one message at a time with long pauses.
- **Best for:** cheat activation messages, one-off game event notifications.

### PANEL_new_text(who, delay, fmt, ...)
- **Header:** `ui/hud/panel.h`
- **Conditions:** none — works unconditionally
- **Behavior:** radio message with walkie-talkie icon and sound effect. Bottom of screen.
- **Timing:** stays on screen for `delay` ms (e.g. 3000 = 3 sec), then disappears.
- **Duplicate handling (same text, same `who`):** resets timer, no new slot, sound does NOT repeat.
- **Different text:** adds to queue. Multiple messages display as a **stack** (up to 3 visible
  simultaneously). Each has its own timer. Sound plays immediately on each call (does not
  wait for previous message to expire).
- **`who` parameter:** `NULL` for generic messages. Pass a `Thing*` to associate with a character.
- **Best for:** in-game radio messages, story event text, NPC dialogue.

### PANEL_new_info_message(fmt, ...)
- **Header:** `ui/hud/panel.h`
- **Conditions:** none — works unconditionally
- **Behavior:** sliding message in bottom-left corner.
- **Timing:** auto-expires after ~2 sec. Each call triggers a new slide-in immediately.
- **Best for:** short gameplay info, pickup notifications, item collected.

### FONT_buffer_add(x, y, r, g, b, shadow, fmt, ...)
- **Header:** `engine/graphics/text/font.h`
- **Conditions:** none — can be called from game tick.
- **Behavior:** queues text into a message buffer. NOT rendered immediately — rendered later
  by `FONT_buffer_draw()` which is called unconditionally in `game.cpp` before screen flip.
- **Rendering:** pixel-based (5×7 bitmap font), via `ge_lock_screen` → `FONT_draw_coloured_char`.
- **Coordinates:** ⚠️ **LITERAL window pixels** — *not* the logical 640×480 coord system
  used by the 2D HUD (`AENG_draw_rect`, `POLY_add_quad`, health bars, etc.). This bites
  whenever text is mixed with HUD rects: the rects scale up with the window while text
  stays glued to literal pixel positions. At 1024×768 the rect covering "logical x=218..420"
  lands around actual pixel 349..672, while text at `x=224` stays at literal pixel 224 — so
  the label you *thought* would sit inside that rect ends up far to its left. See the "Mixing
  text and HUD rects" section below for the scaling recipe.
- **Color:** standard RGB order: `r, g, b`.
- **Shadow:** if `shadow != 0`, draws a darker copy at (+1, +1) behind the text.
- **Capacity:** `FONT_MAX_MESSAGES` messages per frame, shared text buffer `FONT_BUFFER_SIZE`.
  Buffer is cleared after each `FONT_buffer_draw()` call.
- **Best for:** deferred pixel text from game tick. Simpler than CONSOLE_text_at if you
  don't need timers — just call every frame.

---

## Render-pass-only functions

These add geometry to the poly pipeline or draw pixels directly to the backbuffer.
**Cannot be called from game tick** — either corrupts shadows or text gets overwritten.
Used internally by the game-tick-safe wrappers above.

### FONT2D_DrawString(chr, x, y, rgb, scale, page, fade)
- **Header:** `engine/graphics/text/font2d.h`
- **What it is:** adds font polygon quads to a poly page (`POLY_PAGE_FONT2D`).
  This is the internal workhorse — `CONSOLE_status`, `CONSOLE_text_at`, HUD elements
  all use it under the hood.
- **From game tick — DO NOT USE:**
  - Without `POLY_frame_init/draw` wrapper → **corrupts character shadows** (VB slot reuse,
    same root cause as `shadow_corruption_investigation.md`).
  - With `POLY_frame_init/draw` wrapper → shadows OK but **text invisible** (overwritten
    by main render pass).
- **Where it works:** inside render-pass code wrapped in `POLY_frame_init/POLY_frame_draw`
  (`CONSOLE_draw`, overlay draw, HUD draw, etc.).
- **For debug from game tick → use `CONSOLE_text_at` instead.**

### FONT_draw(x, y, fmt, ...)
- **Header:** `engine/graphics/text/font.h`
- **What it is:** pixel-based bitmap font (5×7). Yellow text with red shadow at (+1, +1).
  Uses `ge_lock_screen` / `ge_plot_pixel` / `ge_unlock_screen` — draws pixels directly
  to the CPU backbuffer copy.
- **Must be called between `ge_lock_screen()` and `ge_unlock_screen()`.**
- **Works in:** game.cpp render path (before flip), CONSOLE_draw, and **main menu**
  (if manually inserted in `attract.cpp` before `AENG_flip()`).
- **Does NOT work from game tick** — text is overwritten by subsequent rendering.
- **Variant:** `FONT_draw_coloured_text(x, y, r, g, b, fmt, ...)` — same but custom RGB color,
  no shadow.
- **No shadow corruption risk** — does not use poly pages.

### AENG_world_text(x, y, z, red, blue, green, shadow, fmt, ...)
- **Header:** `engine/graphics/pipeline/aeng.h`
- **What it is:** projects 3D world coordinates to screen via `POLY_transform`, then calls
  `FONT_buffer_add` with the resulting screen position.
- **⚠️ Color parameter order is RBG, NOT RGB!** Parameters are `red, blue, green`.
  Internally it passes them to `FONT_buffer_add` as `red, green, blue` (swaps blue/green).
  Example: to get magenta (R=255,G=0,B=255) pass `0xff, 0xff, 0x00` (red=ff, blue=ff, green=00).
- **Where it works:** must be called from render pass (inside `AENG_draw_city` or
  `AENG_draw_warehouse` person-drawing loops). Used for debug labels above characters
  (`aeng.cpp`, outdoor ~line 6168, indoor ~line 7660).
- **Two render loops:** outdoor (`AENG_draw_city`) and indoor (`AENG_draw_warehouse`).
  If you add a call in one but not the other, it will only show on matching levels.
- **Position:** `(x, y, z)` are world coordinates. `y` is height (up). Person's feet are at
  `WorldPos.Y >> 8`. For text above head use approximately `+ 0x40` to `+ 0x50`.
  Due to perspective projection, text drifts away from the visual model at large distances —
  this is normal and inherent to the function.
- **Silent drop:** if the projected point is behind the camera or outside the view frustum,
  `pp.IsValid()` returns false and text is silently dropped. No error.
- **Unique capability:** the only method that places text at a 3D world position.

### draw_text_at(x, y, message, font_id) — main POLY frame only
- **Header:** `engine/graphics/text/text.h`
- **What it is:** 2D text using bitmap font atlas texture (`TEXTURE_page_font`). Adds quads
  to `POLY_PAGE_TEXT` with additive blend (Src=One, Dst=One). White additive text from font.tga.
- **Where it works:** ONLY inside the main POLY frame created by `AENG_draw()`. Confirmed
  working when called before `POLY_frame_draw(UC_TRUE, UC_TRUE)` in `AENG_draw`.
- **Does NOT work from:** game tick, `CONSOLE_draw`, any separate `POLY_frame_init/draw`,
  or main menu (attract mode does not call `AENG_draw`).
- **Requires setup:** `text_fudge` (extern BOOL) and `text_colour` (extern ULONG) must be set.
  `text_fudge = UC_FALSE; text_colour = 0x00ffffff;` for plain white text.
- **Variant:** `draw_centre_text_at(x, y, message, font_id, flag)` — centers horizontally.
  Used by `process_bullet_points` for mission intro text.
- **Not suitable for debug output** — requires code inside AENG_draw. Use `CONSOLE_text_at`.

---

## Menu text rendering

### MENUFONT_Draw
- **Header:** `engine/graphics/text/menufont.h`
- **What it is:** the menu's own text rendering system. Used by `FRONTEND_loop()` and
  widget system (`widget.cpp`) for all menu text.
- **Only works in menu render pipeline** — not available during game rendering.
- **Not for debug use** — it's the menu UI system.

### Debug text in menu
For debug output in the main menu, use `FONT_draw` with manual `ge_lock_screen/unlock`
inserted in `attract.cpp` before `AENG_flip()`. This is the only confirmed working
approach for menu debug text. Example:
```cpp
void* scr = ge_lock_screen();
if (scr) {
    FONT_draw(10, 10, "debug text here");
    ge_unlock_screen();
}
```

---

## How to enable debug overlay

1. F9 → type `bangunsnotgames` → Enter (enables `allow_debug_keys`)
2. Ctrl — toggle display (switches `debug_overlay_locked_on`)
3. Shift+F12 — toggle `cheat` variable (0↔2), shows FPS/coordinates via overlay.cpp

## ⚠️ Mixing text and HUD rects — coordinate-space mismatch

**Read this whenever you place text near or inside a rectangle drawn by
`AENG_draw_rect`, `POLY_add_quad`, health bars, or any other 2D HUD primitive.**

The two families use different coordinate systems:

| Family | Coord space | Notes |
|--------|-------------|-------|
| `AENG_draw_rect`, `POLY_add_*`, HUD primitives | **Logical 640×480** | Pipeline scales to actual window via the 2D projection matrix. `POLY_screen_width`/`height` are hardcoded to `DisplayWidth`/`Height` = 640/480. |
| `FONT_buffer_add`, `FONT_draw`, `FONT_draw_coloured_text` | **Literal window pixels** | Writes directly to CPU backbuffer via `ge_plot_pixel`. No scaling. Coords are interpreted as actual pixels of the current window. |
| `FONT2D_DrawString`, `draw_text_at` | **Logical 640×480** | Goes through `POLY_add_quad` → same page system as HUD rects. Scales with window. |
| `CONSOLE_text_at` | Logical (via `FONT2D_DrawString`) | Scales correctly. |

### Symptom

On a window larger than 640×480, text placed at the same coords as a rect
ends up shifted left/up — often clustered against the top-left corner while
rects are spread across the full width.

### Fix

When mixing `FONT_buffer_add` / `FONT_draw` text with logical-coord rects,
scale the text coordinates to pixels manually:

```cpp
extern SLONG RealDisplayWidth;
extern SLONG RealDisplayHeight;

SLONG tx(SLONG logical) { return (logical * RealDisplayWidth)  / 640; }
SLONG ty(SLONG logical) { return (logical * RealDisplayHeight) / 480; }

// Logical 640×480 coords for both rect and text.
AENG_draw_rect(tab_x, tab_y, tab_w, tab_h, 0x000000, 2, POLY_PAGE_COLOUR);
FONT_buffer_add(tx(tab_x + 6), ty(tab_y + 4), 255, 255, 255, 1, (CBYTE*)"%s", label);
```

Or wrap with a helper that takes logical coords and forwards to
`FONT_buffer_add` after scaling. The input debug panel at
[new_game/src/engine/debug/input_debug/input_debug.cpp](../../../new_game/src/engine/debug/input_debug/input_debug.cpp)
uses `input_debug_text` for exactly this reason.

### Easier alternative

If your text is part of the HUD/overlay layer anyway, use
`FONT2D_DrawString` (or `CONSOLE_text_at`) instead of `FONT_buffer_add`.
Those live in the same logical 640×480 coord space as rects — no scaling
needed.

### When literal-pixel coords are OK

Some callers intentionally stick text in the literal top-left of the
window regardless of window size (debug FPS, F2 gamepad overlay). Small
pixel offsets like (10, 20) sit in the top-left corner on any window
size, which is exactly what you want for a "corner debug readout". Don't
scale in those cases — just be aware the text won't align with any
scaled HUD geometry.

## Shadow corruption warning

Any function that adds geometry to poly pages (FONT2D_DrawString, draw_text_at) — if called
from game tick (before `POLY_frame_init` in the render pass) — will corrupt character shadows.
Root cause: VB slot reuse between frames. Wrapping in `POLY_frame_init/draw` prevents the
corruption but text gets overwritten by the main render. Details: `shadow_corruption_investigation.md`.

Game-tick-safe functions (CONSOLE_text_at, CONSOLE_status, MSG_add, PANEL_*, FONT_buffer_add)
do NOT cause shadow corruption — they queue text for rendering in the correct phase.

## Details and investigation history

Full investigation log: `new_game_devlog/debug_text_rendering_test.md`
