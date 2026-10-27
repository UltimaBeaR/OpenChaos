# 9 — Slowdowns during NPC automatic fire

- Status: 🔵 open
- Area: gameplay
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Noticeable FPS drops when an NPC fires an automatic weapon continuously. Manifests: **(1) Gatecrasher** — a thug at the closed gate fires without stopping. **(2) Seek, Sneak and Seize** — a MIB at the building entrance. Possibly related to the number of bullet effects (particles, traces, sounds) during continuous fire. **Additional observation 2026-04-15:** the NPC in these situations fires **noticeably faster** than the normal automatic rate (possibly a separate script/timer) — because of this a "million" casings pour out (see the separate casings bug), which probably gives the main contribution to the slowdown. **Important clarification:** in normal play the bug practically doesn't reproduce — without an invincibility cheat the NPC one-shots the player in half a second and the scenario "standing under automatic fire for a long time" physically doesn't happen. The symptom manifests only with the immortality cheat. It's worth profiling and optimizing when possible.

Old-list status: *Investigate (manifests mainly with a cheat)*.
