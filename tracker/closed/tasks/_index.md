# Closed tasks

| # | Area | Closed | Tags | Outcome |
|---|------|--------|------|---------|
| [99](99-crash-in-the-fallen-police-dialogue.md) | crash | 2026-11-04 | bug, gh-issue | **Crash in The Fallen during the police dialogue** — not a bug in current releases: the assert was removed in `e6d5d8dc` (part of v1.0.2); the user checked debug main and the published v1.0.2; the reporter ran an old binary and confirmed on GitHub #10 (closed). |
| [129](129-sprint-does-not-consume-stamina.md) | gameplay | 2026-11-04 | bug, gh-issue | **Sprint does not consume stamina** — fixed: stamina does not recover while the sprint button is held, as with the shared button in the original (regression from `f53b49cf`, GitHub #11). |
| [130](130-menu-font-accented-letters-lost.md) | ui | 2026-11-04 | bug, gh-issue | **Menu font: accented letters lost** — fixed: the menu font table was restored with escapes (damage came from our commit `12d7ae5f`, GitHub #18); the user confirmed the letters in the German sound menu. |
