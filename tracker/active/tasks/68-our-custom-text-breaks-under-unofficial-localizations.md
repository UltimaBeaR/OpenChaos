# 68 — Our custom text breaks under unofficial localizations (fonts)

- Status: 🔵 open
- Area: ui
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**The essence of the problem.** Unofficial localizations (e.g. Russian fan ones) rewrite the **game's font atlases** (`font.TGA` for FONT2D, `olyfont2.tga` for MenuFont — these are **data resources**, not code): they overwrite the English glyphs with Cyrillic. Their translation is designed **only for the text that was already in the original game** and for the original resources — but **not** for our **custom additions** (which aren't in the original: our menu items, the help screen — controls etc., debug, console). So our text, which we deliberately write in English and draw with the standard game fonts, **breaks** under such a localization (drawn with Cyrillic glyphs/garbage). **Why it can't just be fixed:** (1) we can't ship the original font image — it's a game data resource (license); code-level built-in fonts are fine (uc_orig FONT under MIT, font8x8 — public domain). (2) **At runtime you can't tell an English install from a Russified one** (no marker, the atlases differ across patches) → you can't do "one thing under English, another under Russian". (3) Our own built-in font visually differs from the original → it's jarring. **Broken (on the game atlases):** menu items (MENUFONT/`olyfont2.tga`), the body of the help screens (FONT2D/`font.TGA` + inline button glyphs), the F9 console. **Already NOT broken:** the F1 legend/debug (the built-in FONT). **This will need to be solved properly with the fonts to fix it "right"** — how exactly is not yet decided (we'll discuss separately). **What we decided for 1.0 (stop-gap):** translate our custom help text (the help menu items + the contents of the controls screens) to the **built-in font** — it doesn't break under any localization, **but visually it will differ** from the default game font (accepted deliberately; the help is on a separate screen, not standing next to the game text). The F9 console — for now we ignore it or do it later.

Old-list status: *Decide (font fix)*.
