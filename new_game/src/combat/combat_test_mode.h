#ifndef COMBAT_COMBAT_TEST_MODE_H
#define COMBAT_COMBAT_TEST_MODE_H

// OpenChaos addition (not in the original game).
//
// Combat-testing harness, hidden behind the `bangunsnotgames` cheat
// (the same gate as every other debug key -- callers sit inside
// process_controls' `if (!allow_debug_keys) return;` block, so this is
// never reachable in a normal build). Spawns bandits in a small ring
// around the player; they immediately aggro and the live count is kept
// topped up to a target, so melee tuning can be exercised on demand.
// Three debug keys drive it (see the F1 legend).

// Raise the target enemy count by one (turns the mode on if it was
// off) and spawn immediately up to the new target.
void combat_test_inc(void);

// Lower the target by one (min 0). Already-alive enemies are left
// alone -- only the respawn ceiling drops. Reaching 0 ends the mode.
void combat_test_dec(void);

// Cycle the armament tier (0..4). Affects only future spawns.
void combat_test_cycle_armament(void);

// Per-frame upkeep: prune dead bandits and respawn (staggered, ~5 s
// after a kill) to keep the live count at the target. Call from the
// debug block of process_controls.
void combat_test_update(void);

// Drop all tracking and turn the mode off. Call on level (re)load so
// stale THING_INDEXes from a previous level are never touched.
void combat_test_reset(void);

#endif // COMBAT_COMBAT_TEST_MODE_H
