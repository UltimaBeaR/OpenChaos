# 11 — "Green button" cutscene restarts several times (Day of Reckoning)

- Status: 🔵 open
- Area: gameplay
- Tags: bug, intermittent
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Repro (Finale1.ucm, 2026-05-08):** after pressing the "green button" (a scripted cutscene trigger) the cutscene starts playing, **immediately cuts off, starts over** and so on several times, then plays normally. Possibly related to pressing the skip button multiple times at that moment. **Hypotheses:** (1) an accumulated `press_pending` from previous skip presses — at the cutscene start the very first tick reads a stale skip → cuts off, EWAY restarts the cutscene from the beginning; (2) the cutscene system retriggers if conditions (camera arrived, audio ready) are recomputed instantly after a skip; (3) double-trigger — the trigger button emits an EWAY launch + skip-input on the same tick. Related by class to the already fixed "ESC from the main menu opens the pause menu" — possibly a force_release of the skip buttons is needed when entering an EWAY cutscene. **Hard to reproduce reliably.**

Old-list status: *Investigate (intermittent)*.
