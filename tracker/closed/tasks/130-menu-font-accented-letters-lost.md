# 130 — Menu font: accented letters lost

- Status: closed 2026-11-04
- Area: ui
- Tags: bug, gh-issue
- Created: 2026-11-04
- Related: GitHub issue [UltimaBeaR/OpenChaos#18](https://github.com/UltimaBeaR/OpenChaos/issues/18) (Allexin)

## Description
The last 10 characters of `frontend_fonttable` (which glyph box of `olyfont2.tga` is which character) were the
replacement character `EF BF BD`, so accented letters vanished from menus in non-English languages. Restored as
escapes: `È À Ü Ö Ä Ù Ú Ó Á É` (`\xC8 \xC0 \xDC \xD6 \xC4 \xD9 \xDA \xD3 \xC1 \xC9`).

## Important notes
- Cause: our commit `12d7ae5f` (2026-03-13) re-saved Windows-1252 files of `original_game/` as UTF-8 while adding
  analysis comments; stage 4 (`2751b41e`) then carried the damaged line into `new_game`. The bytes are intact in
  `62841682`, in the first commit of the original repo (`f9e2faf`) and in PieroZ's repo. Not a 2017 snapshot problem,
  as the issue assumes.
- Checked against `olyfont2.tga`: 64 boxes = 54 + 10, the letter shapes are in this order. Menu text is drawn additively,
  so the zero alpha of some boxes doesn't matter.
- The game data has no glyphs for `â ê ß ì ò í ¡ ¿` in the menu font; they stay missing (no art).
- All 73 damaged characters in `original_game/` (5 files) were restored from `62841682`, keeping the `claude-ai`
  comments. All C/C++ sources of `original_game/` are plain UTF-8 now; non-ASCII bytes in string and char literals are
  written as `\x` escapes, and the literals were checked byte by byte against `62841682`. Game data files read at
  runtime (`text/lang_*.txt` and the like) stay in Windows-1252 on purpose.
- Every non-ASCII literal of the game code in `62841682` was compared with the current code: this table was the only
  live damage.

## How to verify
In `config.ini` set `language=text\lang_french.txt` (or german/spanish/italian), start the game, open the main menu and
options: words with `é è à` (German `ä ö ü`) show the letters, nothing is skipped or misplaced. Switch back to
`lang_english.txt`: menus look as before.
