#ifndef SHOOTING_GUNS_H
#define SHOOTING_GUNS_H

// Gun targeting system: auto-aim scoring and target selection.
// Used by both player (calc_target_score_new/find_target_new) and NPCs.

#include "engine/core/types.h"

struct Thing;

// Maximum auto-aim angular cone: ~51 degrees left/right (2048/7 ≈ 292 angle units).
// uc_orig: MAX_GUN_AIM (fallen/Headers/guns.h)
#define MAX_GUN_AIM 3000

// Returns gun range and spread for p_person's equipped weapon.
// Returns 1 if the person has a weapon, 0 otherwise.
// uc_orig: get_gun_aim_stats (fallen/Source/guns.cpp)
SLONG get_gun_aim_stats(Thing* p_person, SLONG* range, SLONG* spread);

// Computes auto-aim score for darci targeting p_target.
// Player-only: accounts for FOV cone, LOS, target class, and modifiers.
// uc_orig: calc_target_score_new (fallen/Source/guns.cpp)
SLONG calc_target_score_new(Thing* darci, Thing* p_target);

// Finds the best auto-aim target for p_person.
// Player: sphere search with score ranking. NPC: returns PCOM kill target.
// uc_orig: find_target_new (fallen/Source/guns.cpp)
THING_INDEX find_target_new(Thing* p_person);

#endif // SHOOTING_GUNS_H
