# 80 — Full multi-platform testing

- Status: 🔵 open
- Area: platform
- Tags: analysis
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Extended testing that isn't done in 1.0 (there — only a smoke test on Windows). After the release: (a) **Testing on a clean machine** (without dev dependencies) — that the project builds conveniently from source / installs from the release package, no hidden dev dependencies. More of a process task about development than about the game. (b) **A full run of all missions on each platform** — Windows, macOS, Steam Deck — not just a smoke test, but a proper playthrough with all the mechanics. (c) **Gamepad checks on all platforms** — DualSense/Xbox on macOS (where we didn't test in 1.0); the Steam Deck's built-in gamepad. Before 1.0 this is unrealistic, but after the release it should be done to understand what actually works beyond Windows.

Old-list status: *Planned*.
