# 27 — Puddles spawn above ground level

- Status: 🔵 open
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Sometimes a puddle appears noticeably above the ground — as if it magnetized to the top of a nearby curb. Hypothesis: when spawning, the puddle tries to "jump" onto the level of the nearest curb instead of the ground. Separately: the placement of puddles differs from PieroZ for us (in a specific spot we have a puddle but PieroZ doesn't, and vice versa) — which means the **RNG diverged** somewhere (a different seed or a different order of rand() calls). It's worth figuring out the RNG divergence in general. **Repro (as of 2026-04-15):** the starting mission with the cop car (timed laps) — drive forward to the end, on the left ahead in the corner there will be this puddle. The repro may stop working if the RNG changes.

Old-list status: *Investigate*.

Note (2026-10-27): commit `87e0a9a6` unified `rand` across platforms (`uc_rand`) — the "RNG divergence vs PieroZ" part may look different now.
