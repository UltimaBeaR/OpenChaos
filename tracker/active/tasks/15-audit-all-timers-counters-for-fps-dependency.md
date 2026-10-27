# 15 — Audit all timers/counters for FPS dependency

- Status: 🔵 open
- Area: gameplay
- Tags: analysis, bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**A bug class.** The original was render-locked (frame == tick, ~15–22 FPS), so many delays/timers in the code are written as "increment per frame" and were tuned to this fixed FPS. Our render is **unlocked** (200+ FPS) — any such counter that ticks in the render loop, not by the physics tick or wall-clock, "matures" as many times faster as the FPS is higher → delays collapse, animations/windows flash, events fire earlier than real time. **Already caught and fixed one by one:** the tutorial dialog grace ([game.cpp](../../../new_game/src/game/game.cpp)), menu animations/fades ([gamemenu.cpp](../../../new_game/src/ui/menus/gamemenu.cpp)), the Mission Completed overlay delay + voice wait (gamemenu.cpp, 2026-06-04), the `PYRO_FIREBOMB` explosion particle spawn ([pyro.cpp](../../../new_game/src/effects/combat/pyro.cpp)). These are scattered findings — **there was no systematic pass.** **Task:** have an agent go through the whole codebase, find all counters/timers/time accumulators (patterns `+= N` in the render path, `*_counter`, `*_timer`, `*_wait`, `*_delay`, dependencies on `TICK_RATIO`/`TICK_SHIFT` outside the physics tick, etc.), for each determine: (1) does it tick by render frame, by physics tick, or by wall-clock; (2) does it visually/gameplay-wise depend on FPS; (3) does it need to be moved to wall-clock (`sdl3_get_ticks()` / accumulating `millisecs`) or to the physics tick. Compile a prioritized list, fix in a batch. **Approach samples** — the already-completed fixes listed above.

Old-list status: *Planned*.

## Important notes
- Related design-rate questions found in the archive sweep (2026-10-27): Timer1/Timer2 at 20 vs 30 Hz, particles
  15 → 20 Hz, bench health, `FIRE_process` rates — see
  [original_tick_rates/summary.md](../../../archive/new_game_devlog/fps_unlock/original_tick_rates/summary.md) L173-203 and
  [render_interpolation/plans.md](../../../archive/new_game_devlog/fps_unlock/render_interpolation/plans.md) L90-101. Render-side stepping is in #117.
