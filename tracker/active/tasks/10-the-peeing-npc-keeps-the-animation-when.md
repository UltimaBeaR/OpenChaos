# 10 — The "peeing" NPC keeps the animation when fleeing

- Status: 🔵 open
- Area: gameplay
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Repro (Transmission Terminated):** at the start of the mission go left and then forward to the parking lot of several taxis — in the corner stands an NPC who is peeing. If an enemy runs past and attacks Darci, this NPC runs away (flee state), but **the peeing animation doesn't stop** — he runs and "pees" on the go at the same time. **Expected behavior:** when transitioning to the flee state, the peeing mechanic should explicitly stop (as it correctly works when you approach him — there the animation is interrupted). Apparently the NPC has an "approach-aware" cleanup path for the peeing mechanic (triggers on the player's approach), but the flee transition doesn't call this same cleanup function. One NPC on one mission, niche.

Old-list status: *Investigate (verify)*.
