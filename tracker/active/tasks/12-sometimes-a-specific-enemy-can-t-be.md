# 12 — Sometimes a specific enemy can't be targeted

- Status: 🔵 open
- Area: gameplay
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Repro via combat test mode (debug key `=`):** spawn many enemies at once. Sometimes one specific enemy **cannot be targeted** — the target marker simply isn't placed, no matter how you turn/move; other enemies nearby target normally. Hypotheses: (1) a limit on the number of candidates in the spherical target search (`STANCE_MAX_FIND` / `PCOM_MAX_FIND` = 16, `found[16]` buffer in combat.cpp) — with a large number of enemies the "extra" ones don't make it into the selection and can't be targeted; (2) this enemy doesn't pass the filter (LOS / angle / state); (3) overflow/overwrite of the temp candidate buffer. Most likely manifests **only with an artificially large number of enemies** (normal gameplay doesn't spawn that many).

Old-list status: *Investigate*.
