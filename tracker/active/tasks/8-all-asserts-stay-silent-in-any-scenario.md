# 8 — All ASSERTs stay silent in any scenario (full hardening)

- Status: 🔵 open
- Area: crash
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Full hardening: no ASSERT should fire in any scenario (including edge cases and malicious "getting into a stuck loop"). In 1.0 this bar is **not reached** — the 1.0 criterion is softer (see stage12.md "1.0 stability principles"): crashes in normal play are unacceptable, isolated edge-case ASSERTs (like the ASSERT when pulling up onto a crate) are acceptable. The "crash on ASSERT" behavior is preserved in 1.0 and after (better an explicit crash than silent memory corruption). Post-1.0: go through all ASSERTs, for each decide — graceful fallback / remove / leave as a blocker with a log.

Old-list status: *Planned*.
