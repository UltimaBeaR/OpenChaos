# 79 — macOS fullscreen: render area too narrow at startup

- Status: 🔵 open
- Area: platform
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
When launching in fullscreen mode on macOS: the loading screen and the start of the video render in a narrower area, after 1–2 seconds it abruptly expands to the correct size. Not reproducible on Windows. Hypotheses: the two-pass SDL3 HiDPI resize is applied with a delay in fullscreen, or `SDL_SetWindowFullscreen` is asynchronous on Cocoa / Core Animation — the first frames before the resize callback are drawn with the old dimensions.

Old-list status: *Post-1.0*.
