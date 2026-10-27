# 70 — Gamma correction — make it work and return the setting

- Status: 🔵 open
- Area: ui
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
In 1.0 the gamma setting was removed from the menu (because `ge_is_gamma_available` returns false). Need to figure out why gamma correction is unavailable (depends on the backend / SDL3 swap chain), implement it via software correction in a shader if the SDL3 path doesn't work, return the setting to the UI. Important for different monitors (especially the Steam Deck OLED).

Old-list status: *Implement*.
