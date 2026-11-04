# 129 — Sprint does not consume stamina

- Status: closed 2026-11-04
- Area: gameplay
- Tags: bug, gh-issue
- Created: 2026-11-04
- Original: [original-prompts/129-sprint-does-not-consume-stamina.md](../original-prompts/129-sprint-does-not-consume-stamina.md)
- Related: GitHub issue [UltimaBeaR/OpenChaos#11](https://github.com/UltimaBeaR/OpenChaos/issues/11) (xStickerGamerx)

## Description
Since sprint moved to its own button, sprinting never drains stamina. Stamina drains by 1 per 4 ticks while sprinting
and recovers by 1 per tick unless ACTION is held. In the original, sprint WAS the ACTION button, so recovery paused by
itself. Commit `f53b49cf` (2026-05-31) moved sprint to a separate button; the recovery check still looks at ACTION only,
so recovery outruns the drain. Not a deliberate decision.

## Important notes
- Fix: recovery is also paused while the sprint button is held (`person.cpp`, helper `continue_pressing_sprint`), the
  same as the held shared button in the original code — whatever the person is doing, including running at normal speed
  after exhaustion and standing still. The user decided on full original behaviour (a standing-still exception was
  tried and dropped).
- The old held-ACTION check was removed from recovery: ACTION no longer sprints, so it has nothing to do with stamina.
- The reporter's proposed fix (`Mode != PERSON_MODE_SPRINT`) was not used: after exhaustion the mode drops to run and
  recovery would resume with the button held, sprint re-enters above the start threshold, stamina is zeroed again
  below 10 — a loop of out-of-breath sounds and speed changes until release.

## How to verify
Any street mission, on foot, stamina pips visible on the HUD.
1. Run holding Sprint (Shift / Circle): pips go down, at the end the out-of-breath sound plays **once** and the
   character keeps running at normal speed while the button stays held; the pips stay empty.
2. Keep holding Sprint and stop moving: pips stay empty.
3. Release Sprint: pips recover (standing or running), and sprint works again.
