#ifndef AI_COMBAT_GLOBALS_H
#define AI_COMBAT_GLOBALS_H

#include "Game.h"

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
    UWORD Owner;     // THING_NUMBER of the person being attacked (the victim)
    UWORD Perp[8];   // THING_NUMBERs of attackers per compass slot (N/NE/E/SE/S/SW/W/NW)
    UWORD LastUsed;  // GAME_TURN when this entry was last active
    UBYTE Index;
    UBYTE Count;
};

// Columns of fight_tree[][10]. The table uses raw SWORD arrays for data compactness.
// uc_orig: FightTree (fallen/Source/Combat.cpp)
struct FightTree {
    UBYTE Anim;
    UBYTE Finish;
    UBYTE NextPunch1;
    UBYTE NextPunch2;
    UBYTE NextKick1;
    UBYTE NextKick2;
    UBYTE NextJump;
    UBYTE NextBlock;
    UBYTE Damage;
    UBYTE HitType;
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
// Columns: [Anim, Finish, NextPunch1, NextPunch2, NextKick1, NextKick2,
//           NextJump, NextBlock, Damage, HitType]
// uc_orig: fight_tree (fallen/Source/Combat.cpp)
extern SWORD fight_tree[][10];

// Hit response table indexed by [height + behind*3].
// Each entry: [animation, knockdown_flag]. Height: 0=low, 1=mid, 2=high.
// uc_orig: take_hit (fallen/Source/Combat.cpp)
extern UBYTE take_hit[7][2];

// Punch animation candidates grouped by power level for AI selection.
// uc_orig: punches (fallen/Source/Combat.cpp)
extern SWORD punches[4][5];

// Kick animation candidates grouped by power level for AI selection.
// uc_orig: kicks (fallen/Source/Combat.cpp)
extern SWORD kicks[4][5];

// Available grapple moves for both Darci and Roper.
// Terminated with a zero-anim sentinel entry.
// uc_orig: grapples (fallen/Source/Combat.cpp)
extern struct Grapples grapples[];

// Neck snap grapple (only accessible to player via step-forward input).
// uc_orig: grapple (fallen/Source/Combat.cpp)
extern SWORD grapple[];

// Temp THING_INDEX buffer used across multiple sphere-query functions in combat.cpp.
// Declared as file-scope to avoid repeated stack allocations in nested calls.
// uc_orig: found (fallen/Source/Combat.cpp)
extern THING_INDEX found[16];

// Pool of gang-attack tracker structs. Index 0 is unused (NULL sentinel).
// uc_orig: gang_attacks (fallen/Headers/combat.h)
extern struct GangAttack gang_attacks[MAX_HISTORY];

#endif // AI_COMBAT_GLOBALS_H
