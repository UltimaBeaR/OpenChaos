#include "combat/combat_test_mode.h"

#include "game/game.h" // GAME_TURN, Random, NET_PERSON, UC_PHYSICS_DESIGN_HZ
#include "ai/pcom.h" // PCOM_create_person, PCOM_* constants, aggro
#include "things/characters/person.h" // is_person_dead, PERSON_* types
#include "things/core/thing.h" // CLASS_PERSON, TO_THING, THING_INDEX
#include "engine/core/math.h" // SIN / COS (PSX 0..2047 angle)
#include "engine/console/console.h" // CONSOLE_text (bottom popup)

#include <stdio.h>
#include <stdint.h>

// Max bandits we track at once. Way past any sane target; the array is
// the only cap.
#define CTM_MAX_TRACKED 64

// Respawn delay after a kill, in GAME_TURN ticks. GAME_TURN advances at
// UC_PHYSICS_DESIGN_HZ and pauses on pause, so this is real seconds.
#define CTM_RESPAWN_SECONDS 5
#define CTM_RESPAWN_DELAY_TURNS (CTM_RESPAWN_SECONDS * UC_PHYSICS_DESIGN_HZ)

// Number of armament tiers (0..CTM_ARMAMENT_TIERS-1).
#define CTM_ARMAMENT_TIERS 5

// Spawn ring around the player, in raw WorldPos units.
// 1 metre = 32768 raw units: 1 mapsquare = 65536 raw (WorldPos.X>>16),
// "world unit" = mapsquare/256, UNITS_PER_METER = 128 -> 128*256 raw.
// Ring is an annulus: inner 7 m (nothing spawns closer than this, so
// never right in your face) to outer 10 m (Ø 20 m circle).
#define CTM_RAW_PER_METRE 32768
#define CTM_SPAWN_R_MIN (7 * CTM_RAW_PER_METRE) // inner exclusion radius
#define CTM_SPAWN_R_SPAN (3 * CTM_RAW_PER_METRE) // +3 m -> outer radius 10 m

// Tier 4: chance (percent) a spawn is an MIB instead of a bandit.
#define CTM_MIB_PERCENT 30

// Max AI skill passed to PCOM_create_person (matches the engine's
// 0..15 skill range used elsewhere for test spawns).
#define CTM_SKILL_RANGE 16

// uc_orig: PCOM_set_person_ai_kill_person is declared in ai/pcom.h.

static SLONG s_target = 0; // desired live bandit count (0 = mode off)
static SLONG s_armament = 0; // 0..CTM_ARMAMENT_TIERS-1
static THING_INDEX s_list[CTM_MAX_TRACKED];
static SLONG s_n = 0; // tracked entries in s_list
// One pending respawn per kill: each entry is the GAME_TURN at which a
// make-up bandit is due. A death schedules its own entry, timed from
// when that death is detected -- so two kills 2 s apart respawn 2 s
// apart, each 5 s after its own kill.
static SLONG s_pend[CTM_MAX_TRACKED];
static SLONG s_pend_n = 0;

static const char* s_tier_name[CTM_ARMAMENT_TIERS] = {
    "unarmed",
    "melee",
    "melee+pistol",
    "guns+nades",
    "heavy+MIB",
};

// Drop tracked entries whose thing is gone or dead. A killed bandit is
// dropped the moment it dies (before its corpse vanishes), which is
// what arms the post-kill respawn timer.
static SLONG ctm_alive(void)
{
    SLONG w = 0;
    for (SLONG r = 0; r < s_n; r++) {
        THING_INDEX idx = s_list[r];
        Thing* p = idx ? TO_THING(idx) : nullptr;
        if (p && p->Class == CLASS_PERSON && !is_person_dead(p))
            s_list[w++] = idx;
    }
    s_n = w;
    return s_n;
}

// Pick person type + PCOM_HAS_* loadout for the current armament tier.
static void ctm_pick_loadout(SLONG* out_type, SLONG* out_has)
{
    // Bandit visual variations (non-MIB).
    static const SLONG thug[] = { PERSON_THUG_RASTA, PERSON_THUG_GREY, PERSON_THUG_RED };
    SLONG type = thug[Random() % 3];
    SLONG has = 0;

    const SLONG melee = (Random() & 1) ? PCOM_HAS_KNIFE : PCOM_HAS_BASEBALLBAT;

    switch (s_armament) {
    case 0: // unarmed
        break;
    case 1: // knives / bats
        has = melee;
        break;
    case 2: // mostly melee, sometimes a pistol
        has = (Random() % 100 < 30) ? PCOM_HAS_GUN : melee;
        break;
    case 3: { // pistols often, rarer shotgun/AK, plus grenades
        SLONG r = Random() % 100;
        if (r < 60)
            has = PCOM_HAS_GUN;
        else if (r < 80)
            has = (Random() & 1) ? PCOM_HAS_SHOTGUN : PCOM_HAS_AK47;
        else
            has = melee;
        if (Random() % 100 < 40)
            has |= PCOM_HAS_GRENADE;
        break;
    }
    case 4: // shotgun/AK 50:50 + grenades; some are MIBs
        if (Random() % 100 < CTM_MIB_PERCENT) {
            static const SLONG mib[] = { PERSON_MIB1, PERSON_MIB2, PERSON_MIB3 };
            type = mib[Random() % 3];
            has = 0; // MIBs come with their own behaviour/weapon
        } else {
            has = (Random() & 1) ? PCOM_HAS_SHOTGUN : PCOM_HAS_AK47;
            if (Random() % 100 < 40)
                has |= PCOM_HAS_GRENADE;
        }
        break;
    }

    *out_type = type;
    *out_has = has;
}

// Spawn one bandit in the ring around the player, aggroed on the player.
static void ctm_spawn_one(void)
{
    if (s_n >= CTM_MAX_TRACKED)
        return;

    Thing* darci = NET_PERSON(0);
    if (!darci)
        return;

    SLONG ang = Random() & 2047; // PSX angle 0..2047
    SLONG r = CTM_SPAWN_R_MIN + (SLONG)(Random() % CTM_SPAWN_R_SPAN);
    // 64-bit: SIN/COS are 16.16 fixed (|.| up to 65536) and r is large
    // raw units (~3e5), so the product overflows 32-bit. >>16 undoes the
    // fixed-point scale, leaving a raw-unit offset.
    SLONG x = darci->WorldPos.X + (SLONG)(((int64_t)SIN(ang) * r) >> 16);
    SLONG z = darci->WorldPos.Z + (SLONG)(((int64_t)COS(ang) * r) >> 16);
    SLONG y = darci->WorldPos.Y;

    SLONG type, has;
    ctm_pick_loadout(&type, &has);

    THING_INDEX idx = PCOM_create_person(
        type,
        0, // colour
        0, // group
        PCOM_AI_GANG,
        0, // ai_other
        Random() % CTM_SKILL_RANGE, // ai_skill
        PCOM_MOVE_WANDER,
        0, // move_follow
        PCOM_BENT_FIGHT_BACK,
        has, // pcom_has -> weapon
        0, // drop
        0, // zone
        x, y, z,
        ang, // yaw
        0); // random

    if (idx) {
        PCOM_set_person_ai_kill_person(TO_THING(idx), NET_PERSON(0), 0);
        s_list[s_n++] = idx;
    }
}

static void ctm_notify(void)
{
    char buf[96];
    if (s_target <= 0) {
        CONSOLE_text((CBYTE*)"combat test OFF");
        return;
    }
    snprintf(buf, sizeof(buf), "combat test: enemies=%d  armament=%d/%d (%s)",
        (int)s_target, (int)s_armament, (int)(CTM_ARMAMENT_TIERS - 1),
        s_tier_name[s_armament]);
    CONSOLE_text((CBYTE*)buf);
}

void combat_test_inc(void)
{
    s_target++;
    // Manual bump spawns right away (the 5 s delay is only for post-kill
    // make-up spawns). We're now at target, so any pending respawns from
    // earlier kills are redundant -- drop them.
    while (ctm_alive() < s_target && s_n < CTM_MAX_TRACKED)
        ctm_spawn_one();
    s_pend_n = 0;
    ctm_notify();
}

void combat_test_dec(void)
{
    if (s_target > 0)
        s_target--;
    // Alive extras are intentionally left for the player to finish off;
    // update() trims any now-excess pending respawns.
    ctm_notify();
}

void combat_test_cycle_armament(void)
{
    s_armament = (s_armament + 1) % CTM_ARMAMENT_TIERS;
    ctm_notify();
}

void combat_test_update(void)
{
    if (s_target <= 0) {
        ctm_alive(); // keep the list tidy even while off
        s_pend_n = 0;
        return;
    }

    SLONG alive = ctm_alive();

    // A fresh deficit means a bandit just died: queue one make-up
    // respawn, timed from NOW (i.e. from this kill). Each kill gets its
    // own entry, so kills 2 s apart respawn 2 s apart.
    while (alive + s_pend_n < s_target && s_pend_n < CTM_MAX_TRACKED)
        s_pend[s_pend_n++] = (SLONG)GAME_TURN + CTM_RESPAWN_DELAY_TURNS;

    // Target was lowered below what's scheduled -> drop newest pending.
    while (alive + s_pend_n > s_target && s_pend_n > 0)
        s_pend_n--;

    // Fire every pending whose time has come; keep the rest.
    SLONG w = 0;
    for (SLONG i = 0; i < s_pend_n; i++) {
        if ((SLONG)GAME_TURN >= s_pend[i])
            ctm_spawn_one(); // becomes live; entry consumed
        else
            s_pend[w++] = s_pend[i];
    }
    s_pend_n = w;
}

void combat_test_reset(void)
{
    s_target = 0;
    s_armament = 0;
    s_n = 0;
    s_pend_n = 0;
}
