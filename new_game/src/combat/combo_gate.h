#ifndef COMBAT_COMBO_GATE_H
#define COMBAT_COMBO_GATE_H

#include "game/game.h"

// OpenChaos addition (not in the original game).
//
// Anti-mash gate for the PLAYER's melee combo chains. Each fight_tree
// node (the attack the player is about to perform) carries a proc
// chance. When a node fails its roll the attack is dropped and the
// combo ends through the game's normal return-to-idle path -- the same
// mechanism the old 300ms timing gate used, so there is no visual
// glitch.
//
// NPCs are NEVER gated: every call site is guarded by PlayerID before
// reaching here, so AI combat runs on the untouched original path.

// Rolls the gate for fight_tree `node` performed by player `player_id`
// (1-based, as stored in Person.PlayerID). Returns true if the attack
// is allowed. The caller MUST already have checked PlayerID != 0.
bool combo_gate_try(SLONG player_id, SLONG node);

#endif // COMBAT_COMBO_GATE_H
