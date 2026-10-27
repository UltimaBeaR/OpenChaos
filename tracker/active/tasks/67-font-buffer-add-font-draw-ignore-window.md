# 67 — FONT_buffer_add / FONT_draw ignore window scale

- Status: 🔵 open
- Area: ui
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
The pixel path of text (`FONT_buffer_add`, `FONT_draw`, `FONT_draw_coloured_text` — through `ge_plot_pixel` into the CPU backbuffer) uses literal window pixel coordinates, not logical 640×480. On a window larger than 640×480, calls like `FONT_draw(DisplayWidth >> 1, 10, "FPS: ...")` end up at point (320, 10) — not in the center of the screen as intended. Affected: FPS overlay, `gamepad_debug_draw` (F2), `AENG_draw_messages` and other small debug labels. **Gameplay is not affected** — the game HUD and menus use `FONT2D_DrawString`/`MENUFONT_Draw` which work through poly-pages and scale correctly. For our input-debug panel a local wrapper `input_debug_text` with scaling was made. **Universal fix:** add coordinate scaling inside `FONT_draw_coloured_char` (see `.claude/skills/text-rendering/SKILL.md` section "Mixing text and HUD rects" for full details). Caveat: the font itself is a pixel 5×7, after scaling positions the letters will stay the same size — the text will get sparse on large windows. For a full fix you also need to scale the glyph size, which is harder. Low priority.

Old-list status: *Refactoring (post-1.0)*.
