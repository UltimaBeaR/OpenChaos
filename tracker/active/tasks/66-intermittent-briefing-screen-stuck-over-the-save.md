# 66 — Intermittent: briefing screen stuck over the save screen

- Status: 🔵 open
- Area: ui
- Tags: bug, intermittent
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Reproduced **once on 2026-04-11** after successfully completing Insane Assault — the briefing screen (Darci with a pistol, the upper part of the torso) got stuck full-screen over the save screen. The gamepad didn't respond, Enter/space on the keyboard helped. On a repeat playthrough it didn't reproduce — an intermittent bug. Possibly related to the bug "briefing over the loading screen" (fixed 2026-04-14) — the same mechanism of `ge_set_background_override` not being reset. If it's this — it should already be fixed. Observe.

Old-list status: *Observe*.
