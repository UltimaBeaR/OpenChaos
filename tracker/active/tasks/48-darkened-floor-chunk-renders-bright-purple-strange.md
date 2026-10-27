# 48 — Darkened floor chunk renders bright purple + strange dark areas

- Status: ❄️ deferred
- Area: render
- Tags: bug, retail-bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Symptom A (purple floor, first mission):** get into the car, drive ~half a meter forward. **Behind on the right** on the floor there's a darkened area (under the lamp hanging above). At a certain angle/position this chunk is drawn **bright purple** instead of darkened. If you drive a bit further — it disappears. If you reverse back into the same position — it doesn't repeat. **Reproduces in the retail PC version and the PieroZ build** — an original MuckyFoot bug, not our regression. **Hypotheses:** (1) lighting calculation overflow / sign-flip on a specific normal/light direction combination; (2) PolyPage / texture state corruption on a single frame when some nearby object (the lamp?) enables/disables a render state; (3) corrupted vertex colour from a stale cache. The one-sided trigger (only forward, not reverse) suggests this is a race condition / frame-order issue. **Symptom B (strange darkened areas):** on various map levels **strange darkened spots** have been noticed (they look unusual). When the character steps onto such an area — the whole picture (or the character) **darkens**. In retail it works the same way. The connection to symptom A is unknown — possibly one common mechanism (lighting/state corruption on specific cells or objects), possibly different. **Plan:** figure out what the darkened spots are, understand whether they're related to the purple floor. If it's a common mechanism — it's closed with a single fix, if not — two separate ones. **Low priority** — both symptoms are present in retail, not a blocker.

Old-list status: *Known (retail bug)*.
