# 31 — Puddle drop ripples appear only near the player (modal)

- Status: 🔵 open
- Area: render
- Tags: bug, intermittent
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
The animation of ripples/swirls from raindrops on the puddle surface only turned on when the character got close enough; at medium/long distance the puddle was without drop ripples. In the retail PC version the ripples are visible either constantly, or are culled only when the puddle is already out of sight. **History:** on 2026-05-19 the bug was marked fixed and moved to resolved. **2026-05-23 reopened:** on re-check it turned out the bug **hadn't gone anywhere.** **Repro (when in the "bad" mode):** the first mission with driving laps in the cop car. **The nature of the bug is modal, not positional.** When it reproduces — it reproduces **consistently and clearly** (move away from the puddle — drops gone, come closer — they appear). When it doesn't reproduce — it doesn't reproduce **consistently either** (drops visible at all distances). So the mission/level is not the trigger. There's some **hidden condition** switching the whole system between two stable modes. Possible directions for the next "bad" run: RNG seed, count of active puddles/particles at load time, FPS at mission-load time, the state of some LOD/cull flag between runs. We need to capture the maximum number of parameters during the next "bad" mode and compare with the "good" one.

Old-list status: *Observe*.
