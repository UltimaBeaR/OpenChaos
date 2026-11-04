# Closed tasks

| # | Area | Closed | Tags | Outcome |
|---|------|--------|------|---------|
| [99](99-crash-in-the-fallen-police-dialogue.md) | crash | 2026-11-04 | bug, gh-issue | **Crash in The Fallen during the police dialogue** — not a bug in current releases: the assert was removed in `e6d5d8dc` (part of v1.0.2); the user checked debug main and the published v1.0.2; the reporter ran an old binary and confirmed on GitHub #10 (closed). |
| [129](129-sprint-does-not-consume-stamina.md) | gameplay | 2026-11-04 | bug, gh-issue | **Sprint does not consume stamina** — fixed: stamina does not recover while the sprint button is held, as with the shared button in the original (regression from `f53b49cf`, GitHub #11). |
| [130](130-menu-font-accented-letters-lost.md) | ui | 2026-11-04 | bug, gh-issue | **Menu font: accented letters lost** — fixed: the menu font table was restored with escapes (damage came from our commit `12d7ae5f`, GitHub #18); the user confirmed the letters in the German sound menu. |
| [132](132-save-table-pools-8-byte-alignment.md) | architecture | 2026-11-04 | bug, gh-issue | **save_table pools: 8-byte alignment on x64** — fixed: every pool is rounded to 8 bytes (GitHub #13); the user ran the game, no new crashes. |
| [133](133-save-table-plats-wmove-indices.md) | architecture | 2026-11-04 | bug, gh-issue | **SAVE_TABLE_PLATS / SAVE_TABLE_WMOVE point at the wrong entries** — fixed: the indices are 44 and 45 now (GitHub #12); the user played the missions, no problems. |
