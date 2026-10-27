# 6 — Random crashes during gameplay

- Status: 🔵 open
- Area: crash
- Tags: analysis, bug, intermittent
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Significantly rarer after the ASan fixes (90%+ stable). Most likely several different bugs — memory corruption, uninit pointers, out-of-bounds. Plan: run under ASan, fix pointwise. **Update 2026-04-11:** found and fixed 6 bugs (action_dying overflow, ELEV strcpy overlap, BAT ASSERT, MUSIC overflow, PCOM ASSERT, EWAY ASSERT). Whatever surfaces in the smoke test (see the 1.0 plan) — we catch it there; everything else is left for post-1.0.

Old-list status: *Investigate (ASan)*.
