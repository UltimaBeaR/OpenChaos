#include "combat/combat_globals.h"
#include "things/characters/anim_ids.h"

// uc_orig: fight_tree (fallen/Source/Combat.cpp)
SWORD fight_tree[][10] = {
    { 0, 0, 1, 1, 6, 6, 0, 0, 0, COMBAT_NONE },
    { ANIM_PUNCH_COMBO1, 2, 3, 0, 11, 11, 0, 0, 10, COMBAT_PUNCH }, // 1
    { ANIM_PUNCH_RETURN1, 0, 1, 1, 0, 0, 0, 0, 0, COMBAT_NONE }, // 2
    { ANIM_PUNCH_COMBO2, 4, 5, 1, 12, 12, 0, 0, 30, COMBAT_PUNCH }, // 3
    { ANIM_PUNCH_RETURN2, 0, 1, 1, 0, 0, 0, 0, 0, COMBAT_NONE }, // 4
    { ANIM_PUNCH_COMBO3, 0, 0, 0, 0, 0, 0, 0, 60, COMBAT_PUNCH }, // 5

    { ANIM_KICK_COMBO1, 7, 7, 7, 8, 0, 0, 0, 10, COMBAT_KICK }, // 6
    { ANIM_KICK_RETURN1, 0, 1, 0, 6, 0, 0, 0, 0, COMBAT_NONE }, // 7
    { ANIM_KICK_COMBO2, 9, 13, 13, 10, 0, 0, 0, 30, COMBAT_KICK }, // 8
    { ANIM_KICK_RETURN2, 0, 1, 0, 6, 0, 0, 0, 0, COMBAT_NONE }, // 9
    { ANIM_KICK_COMBO3, 0, 0, 0, 6, 0, 0, 0, 60, COMBAT_KICK }, // 10

    { ANIM_PUNCH_COMBO2b, 0, 0, 0, 0, 0, 0, 0, 30, COMBAT_KICK }, // 11
    { ANIM_PUNCH_COMBO3b, 0, 0, 0, 0, 0, 0, 0, 80, COMBAT_KICK }, // 12
    { ANIM_KICK_COMBO3b, 0, 0, 0, 0, 0, 0, 0, 80, COMBAT_PUNCH }, // 13

    { ANIM_KNIFE_ATTACK1, 15, 16, 0, 0, 0, 0, 0, 30, COMBAT_KNIFE }, // 14
    { ANIM_KNIFE_ATTACK1_RET, 0, 14, 14, 0, 0, 0, 0, 0, COMBAT_NONE }, // 15
    { ANIM_KNIFE_ATTACK2, 17, 18, 14, 0, 0, 0, 0, 60, COMBAT_KNIFE }, // 16
    { ANIM_KNIFE_ATTACK2_RET, 0, 14, 14, 0, 0, 0, 0, 0, COMBAT_NONE }, // 17
    { ANIM_KNIFE_ATTACK3, 0, 0, 0, 0, 0, 0, 0, 80, COMBAT_KNIFE }, // 18

    { ANIM_BAT_HIT1, 20, 21, 0, 0, 0, 0, 0, 60, COMBAT_KNIFE }, // 19
    { ANIM_BAT_HIT1_RET, 0, 19, 0, 0, 0, 0, 0, 0, COMBAT_NONE }, // 20
    { ANIM_BAT_HIT2, 0, 0, 0, 0, 0, 0, 0, 90, COMBAT_KNIFE }, // 21
};

// Lookup table: incoming hit [height + behind*3] -> [anim, knockdown_flag].
// 0=front-low(KD), 1=front-mid, 2=front-hi, 3=back-low(KD), 4=back-mid, 5=back-hi.
// uc_orig: take_hit (fallen/Source/Combat.cpp)
UBYTE take_hit[7][2] = {
    { ANIM_KD_FRONT_LOW, 1 },
    { ANIM_HIT_FRONT_MID, 0 },
    { ANIM_HIT_FRONT_HI, 0 },
    { ANIM_KD_BACK_LOW, 1 },
    { ANIM_HIT_BACK_MID, 0 },
    { ANIM_HIT_BACK_HI, 0 },
    { 0, 0 }
};

// uc_orig: grapples (fallen/Source/Combat.cpp)
struct Grapples grapples[] = {
    { ANIM_PISTOL_WHIP, 75, 65, 1024, 0, 1 },
    { ANIM_STRANGLE, 50, 20, 0, 0, 2 },
    { ANIM_HEADBUTT, 65, 20, 0, 0, 2 },
    { ANIM_GRAB_ARM, 60, 20, 0, 0, 1 },
    { 0, 0, 0, 0, 0 },
};

// uc_orig: found (fallen/Source/Combat.cpp)
THING_INDEX found[16];

// uc_orig: gang_attacks (fallen/Headers/combat.h)
struct GangAttack gang_attacks[MAX_HISTORY];
