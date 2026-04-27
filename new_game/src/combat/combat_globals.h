#ifndef COMBAT_COMBAT_GLOBALS_H
#define COMBAT_COMBAT_GLOBALS_H

#include "game/game.h"

// Maximum pool size for GangAttack / history structs.
// Reused LRU-style when the pool is full.
// uc_orig: MAX_HISTORY (fallen/Headers/combat.h)
#define MAX_HISTORY 20

// Ring-buffer size for recording recent attack sequences (must be even).
// uc_orig: MAX_MOVES (fallen/Headers/combat.h)
#define MAX_MOVES 16

// Faction combat type identifiers stored in fight_tree[][9] (HitType column).
// uc_orig: COMBAT_NONE (fallen/Headers/combat.h)
#define COMBAT_NONE 0
// uc_orig: COMBAT_PUNCH (fallen/Headers/combat.h)
#define COMBAT_PUNCH 1
// uc_orig: COMBAT_KICK (fallen/Headers/combat.h)
#define COMBAT_KICK 2
// uc_orig: COMBAT_KNIFE (fallen/Headers/combat.h)
#define COMBAT_KNIFE 3

// Tracks up to 8 attackers (one per compass direction) for a single victim.
// Used by AI to prevent all enemies piling in from the same angle.
// uc_orig: GangAttack (fallen/Headers/combat.h)
struct GangAttack {
    UWORD Owner; // THING_NUMBER of the person being attacked (the victim)
    UWORD Perp[8]; // THING_NUMBERs of attackers per compass slot (N/NE/E/SE/S/SW/W/NW)
    UWORD LastUsed; // GAME_TURN when this entry was last active
    UBYTE Index;
    UBYTE Count;
};

// Per-grapple move: proximity and angle requirements for the attacker.
// Peep=1 means Darci only, Peep=2 means Roper only.
// uc_orig: Grapples (fallen/Source/Combat.cpp)
struct Grapples {
    UWORD Anim;
    SWORD Dist;
    SWORD Range;
    SWORD Angle;
    SWORD DAngle;
    SWORD Peep;
};

// Column indices into fight_tree[][10].
// uc_orig: FIGHT_TREE_DAMAGE (fallen/Source/Combat.cpp)
#define FIGHT_TREE_DAMAGE 8
// uc_orig: FIGHT_TREE_HIT_TYPE (fallen/Source/Combat.cpp)
#define FIGHT_TREE_HIT_TYPE 9

// Melee combat state machine: 22 nodes (0 = root/idle).
// Each row is a FightTree node with 10 SWORD columns:
//   [0] Anim        — animation to play
//   [1] Finish      — return-to-idle node
//   [2] NextPunch1  — next node on first punch input
//   [3] NextPunch2  — next node on second punch input
//   [4] NextKick1   — next node on first kick input
//   [5] NextKick2   — next node on second kick input
//   [6] NextJump    — next node on jump input
//   [7] NextBlock   — next node on block input
//   [8] Damage      — damage dealt (FIGHT_TREE_DAMAGE)
//   [9] HitType     — COMBAT_PUNCH/KICK/KNIFE/NONE (FIGHT_TREE_HIT_TYPE)
// uc_orig: fight_tree (fallen/Source/Combat.cpp)
extern SWORD fight_tree[][10];

// Hit response table indexed by [height + behind*3].
// Each entry: [animation, knockdown_flag]. Height: 0=low, 1=mid, 2=high.
// uc_orig: take_hit (fallen/Source/Combat.cpp)
extern UBYTE take_hit[7][2];

// Available grapple moves for both Darci and Roper.
// Terminated with a zero-anim sentinel entry.
// uc_orig: grapples (fallen/Source/Combat.cpp)
extern struct Grapples grapples[];

// Temp THING_INDEX buffer used across multiple sphere-query functions in combat.cpp.
// Declared as file-scope to avoid repeated stack allocations in nested calls.
// uc_orig: found (fallen/Source/Combat.cpp)
extern THING_INDEX found[16];

// Pool of gang-attack tracker structs. Index 0 is unused (NULL sentinel).
// uc_orig: gang_attacks (fallen/Headers/combat.h)
extern struct GangAttack gang_attacks[MAX_HISTORY];

#endif // COMBAT_COMBAT_GLOBALS_H
