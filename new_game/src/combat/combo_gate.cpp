#include "combat/combo_gate.h"
#include "engine/debug/dbglog/dbglog.h" // DBGLOG (no-op unless OC_DEBUG_LOG)
#include "things/core/player.h" // MAX_PLAYERS

// Number of rows in fight_tree[][10] (combat_globals.cpp). Valid node
// indices are 0..COMBO_NODE_COUNT-1; node 0 is the root/idle node and
// is never gated. Keep in sync if fight_tree gains/loses rows.
#define COMBO_NODE_COUNT 22

// Random() returns a 16-bit value; we take the low 10 bits as a
// 0..COMBO_GATE_ROLL_MASK uniform roll and compare against
// chance * COMBO_GATE_ROLL_RANGE. chance 1.0 -> threshold 1024 which
// the roll (max 1023) can never reach -> always passes. chance 0.0 ->
// threshold 0 -> roll >= 0 always -> always blocks.
#define COMBO_GATE_ROLL_RANGE 1024
#define COMBO_GATE_ROLL_MASK (COMBO_GATE_ROLL_RANGE - 1)

// "Pity" / bad-luck protection. Every time a gated combo node FAILS
// its roll, that node's effective chance is bumped by COMBO_PITY_STEP
// for THIS player's next attempt of the SAME node, stacking on every
// further fail until it finally lands -- at which point it snaps back
// to the base s_chance[]. Per-player, per-node. +0.20 over a 0.40
// base reaches a guaranteed land by the 4th try (0.40 -> 0.60 ->
// 0.80 -> 1.00).
#define COMBO_PITY_STEP 0.20f

// Per-node proc chance (0.0..1.0 probability the hit lands), indexed by
// fight_tree node. 1.0 -> behaviour is 1:1 with the pre-gate combat.
// Tune individual rows from here.
static float s_chance[COMBO_NODE_COUNT] = {
    1.0f,  //  0  root/idle (never gated)
    1.0f,  //  1  PUNCH 1            -- 1st hit
    1.0f,  //  2  punch return 1
    0.80f, //  3  PUNCH 2            -- 2nd hit (hand-only: lowered, hands can't be dodged)
    1.0f,  //  4  punch return 2
    0.40f, //  5  PUNCH 3            -- 3rd hit (hand-only: lowered, hands can't be dodged)
    1.0f,  //  6  KICK 1             -- 1st hit
    1.0f,  //  7  kick return 1
    0.85f, //  8  KICK 2             -- 2nd hit
    1.0f,  //  9  kick return 2
    0.50f, // 10  KICK 3             -- 3rd hit, same button (kick x3)
    0.85f, // 11  PUNCH 2b           -- 2nd hit (punch1 -> kick, 2-hit combo)
    0.75f, // 12  PUNCH 3b           -- 3rd hit, button changes (punch, punch -> kick)
    0.75f, // 13  KICK 3b            -- 3rd hit, button changes (kick, kick -> punch)
    1.0f,  // 14  KNIFE 1            -- 1st hit
    1.0f,  // 15  knife return 1
    0.85f, // 16  KNIFE 2            -- 2nd hit
    1.0f,  // 17  knife return 2
    // Knife is an exception: 100/85/65. With a knife there is no
    // viable non-mash alternative (its cross-combos don't really work).
    0.65f, // 18  KNIFE 3            -- 3rd hit (knife exception)
    1.0f,  // 19  BAT 1              -- 1st hit
    1.0f,  // 20  bat return 1
    0.85f, // 21  BAT 2              -- 2nd hit (bat1 -> bat2, 2-hit combo)
};

// Accumulated pity bonus added on top of s_chance[node], one slot per
// (player, node). Zero = that node is at its base chance. Cleared back
// to zero the instant that node's roll succeeds. It self-clears on the
// first success after a level (re)load, so no explicit reset hook is
// needed (a stale bonus only makes the very first post-load attempt
// slightly easier, then it's gone).
static float s_fail_bonus[MAX_PLAYERS][COMBO_NODE_COUNT];

// Short human-readable names for the on-screen debug log. Return/idle
// nodes are never logged (they are not attack nodes) so "" is fine.
#if OC_DEBUG_LOG && OC_DEBUG_LOG_COMBAT
static const char* s_name[COMBO_NODE_COUNT] = {
    "", "PUNCH1", "", "PUNCH2", "", "PUNCH3",
    "KICK1", "", "KICK2", "", "KICK3",
    "PUNCH2b", "PUNCH3b", "KICK3b",
    "KNIFE1", "", "KNIFE2", "", "KNIFE3",
    "BAT1", "", "BAT2",
};

// Chance -> colour: 1.0 = green, 0.0 = red, 0.5 = yellow-ish. Higher
// chance reads greener, lower reads redder.
static ULONG combo_chance_rgb(float c)
{
    if (c < 0.0f) c = 0.0f;
    if (c > 1.0f) c = 1.0f;
    const SLONG lo = 0x30, hi = 0xff;
    UBYTE g = (UBYTE)(lo + (SLONG)(c * (float)(hi - lo)));
    UBYTE r = (UBYTE)(lo + (SLONG)((1.0f - c) * (float)(hi - lo)));
    return ((ULONG)r << 16) | ((ULONG)g << 8) | (ULONG)lo;
}

// One line per attack: NAME (white)  CHANCE% (gradient)  result
// (green/red). `chance` is the ACTUAL chance used for the roll (base +
// pity bonus); `base` is the unmodified s_chance[]. When the pity
// bonus is active they differ, so we print "actual% (base%)" -- when
// equal we print just "actual%". The gradient colour follows the
// actual chance. Column alignment is done with trailing tabs.
static void combo_log(SLONG node, bool pass, float base, float chance)
{
    int cur = (int)(chance * 100.0f + 0.5f);
    int bas = (int)(base * 100.0f + 0.5f);
    DBGLOG_begin();
    DBGLOG_seg(DBGLOG_color("white"), "%s\t\t", s_name[node]);
    if (cur != bas)
        DBGLOG_seg(combo_chance_rgb(chance), "%d%% (%d%%)\t\t", cur, bas);
    else
        DBGLOG_seg(combo_chance_rgb(chance), "%d%%\t\t", cur);
    DBGLOG_seg(pass ? DBGLOG_color("green") : DBGLOG_color("red"),
        "%s", pass ? "SUCCESS" : "FAIL");
    DBGLOG_commit();
}
#else
static inline void combo_log(SLONG, bool, float, float) {}
#endif

bool combo_gate_try(SLONG player_id, SLONG node)
{
    SLONG pidx = player_id - 1; // Person.PlayerID is 1-based

    // Defensive: never block on an out-of-range id/node -- letting the
    // hit through is always the safe failure mode (no behaviour change).
    if (pidx < 0 || pidx >= MAX_PLAYERS)
        return true;
    if (node <= 0 || node >= COMBO_NODE_COUNT)
        return true;

    float base = s_chance[node];
    float chance = base + s_fail_bonus[pidx][node];
    if (chance > 1.0f) chance = 1.0f; // >=1.0 already always passes

    SLONG roll = Random() & COMBO_GATE_ROLL_MASK;
    SLONG thresh = (SLONG)(chance * (float)COMBO_GATE_ROLL_RANGE);
    bool pass = roll < thresh;

    if (pass)
        s_fail_bonus[pidx][node] = 0.0f;             // landed -> back to base
    else
        s_fail_bonus[pidx][node] += COMBO_PITY_STEP; // missed -> easier next time

    combo_log(node, pass, base, chance);
    return pass;
}
