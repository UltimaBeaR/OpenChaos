# 72 — Gamepad button hints in the menu (from PS1)

- Status: 🔵 open
- Area: ui
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
On PS1, the main menu (and possibly elsewhere) shows hints: ✕ — select, △ — back. We'd like to bring this back for PC — show only when a DualSense is connected. Needed: (1) pull the code from the PS1 version (`original_game/fallen/PSXENG/`), see how it was drawn, (2) port the drawing, (3) show conditionally on `active_input_device == gamepad`. Extending this idea to contextual hints everywhere — see [stage13.md](../../../archive/new_game_planning/stage13.md) section "Control hints".

Old-list status: *Implement*.
