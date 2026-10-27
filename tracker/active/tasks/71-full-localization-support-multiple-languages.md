# 71 — Full localization support (multiple languages)

- Status: 🔵 open
- Area: ui
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
In 1.0 the language setting was removed (only English left). **Architectural problem:** the original `lang_*.txt` files contain strings at fixed positions, our new menu items don't fit there — when substituting the file everything shifts. **Solution (scheme):** don't use the original files directly, but **generate our own localization files** in `openchaos/` (analogous to `config.json`) — on first launch convert the original `lang_*.txt` into a base + append our additional strings on top. The user can edit their file if desired. The original file is only the source of the initial generation, not used directly at runtime.

Old-list status: *Implement*.
