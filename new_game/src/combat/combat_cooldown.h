#ifndef COMBAT_COMBAT_COOLDOWN_H
#define COMBAT_COMBAT_COOLDOWN_H

#include "game/game.h"

// OpenChaos addition (not in the original game).
//
// Per-player cooldown gate for special melee actions (slide/leg-sweep,
// arrest, grapple). After the player performs a gated action it cannot
// be performed again until its cooldown elapses. Time is measured in
// GAME_TURN ticks, which freeze on pause, so the windows are real
// in-game seconds.
//
// NPCs are NEVER gated: every call site is guarded by PlayerID before
// reaching here, so AI combat runs on the untouched original path.

enum CombatCooldown {
    COOLDOWN_SLIDE = 0, // leg-sweep (kick while blocking)
    COOLDOWN_ARREST,    // arresting a downed person
    COOLDOWN_GRAPPLE,   // initiating a grapple
    COOLDOWN_BLOCK,     // entering the defensive block/duck
    COOLDOWN_ROLL,      // evasive side flip/roll (jump + direction)
    COOLDOWN_COUNT,
};

// True if `action` is off cooldown for player `player_id` (1-based, as
// stored in Person.PlayerID) and may be performed now. Callers MUST
// already have checked PlayerID != 0.
bool combat_cooldown_ready(SLONG player_id, SLONG action);

// Starts `action`'s cooldown for `player_id`. Call right after the
// action actually fires. No-op for a non-player id.
void combat_cooldown_arm(SLONG player_id, SLONG action);

// Clears all cooldowns. Call on level (re)load.
void combat_cooldown_reset(void);

// Logs one cooldown attempt to the on-screen OC_DEBUG_LOG. `fired` =
// the action was allowed (cooldown ready) and is happening now; false
// = it was blocked by the still-running cooldown. `name` is the short
// label shown. No-op unless the OC_DEBUG_LOG build flag is on.
void combat_cooldown_note(SLONG player_id, SLONG action, const char* name, bool fired);

#endif // COMBAT_COMBAT_COOLDOWN_H
