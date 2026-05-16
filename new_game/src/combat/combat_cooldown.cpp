#include "combat/combat_cooldown.h"

#include "things/core/player.h" // MAX_PLAYERS
#include "engine/debug/dbglog/dbglog.h" // OC_DEBUG_LOG on-screen log (no-op when off)

// Cooldown lengths in real in-game seconds. UC_PHYSICS_DESIGN_HZ is the
// fixed rate GAME_TURN advances at, so seconds * HZ = tick count.
#define COOLDOWN_SLIDE_SECONDS 5
#define COOLDOWN_ARREST_SECONDS 15
#define COOLDOWN_GRAPPLE_SECONDS 7
#define COOLDOWN_BLOCK_SECONDS 2

static const SLONG s_cd_turns[COOLDOWN_COUNT] = {
    COOLDOWN_SLIDE_SECONDS * UC_PHYSICS_DESIGN_HZ,   // COOLDOWN_SLIDE
    COOLDOWN_ARREST_SECONDS * UC_PHYSICS_DESIGN_HZ,  // COOLDOWN_ARREST
    COOLDOWN_GRAPPLE_SECONDS * UC_PHYSICS_DESIGN_HZ, // COOLDOWN_GRAPPLE
    COOLDOWN_BLOCK_SECONDS * UC_PHYSICS_DESIGN_HZ,   // COOLDOWN_BLOCK
};

// Earliest GAME_TURN at which each (player, action) may fire again.
// GAME_TURN starts at 0 and only grows, so the zero-init left by
// combat_cooldown_reset() reads as "everything ready".
static SLONG s_next_ready[MAX_PLAYERS][COOLDOWN_COUNT];

bool combat_cooldown_ready(SLONG player_id, SLONG action)
{
    SLONG pidx = player_id - 1; // Person.PlayerID is 1-based

    // Defensive: out-of-range -> never block (no behaviour change).
    if (pidx < 0 || pidx >= MAX_PLAYERS)
        return true;
    if (action < 0 || action >= COOLDOWN_COUNT)
        return true;

    return (SLONG)GAME_TURN >= s_next_ready[pidx][action];
}

void combat_cooldown_arm(SLONG player_id, SLONG action)
{
    SLONG pidx = player_id - 1;

    if (pidx < 0 || pidx >= MAX_PLAYERS)
        return;
    if (action < 0 || action >= COOLDOWN_COUNT)
        return;

    s_next_ready[pidx][action] = (SLONG)GAME_TURN + s_cd_turns[action];
}

void combat_cooldown_reset(void)
{
    for (SLONG p = 0; p < MAX_PLAYERS; p++)
        for (SLONG a = 0; a < COOLDOWN_COUNT; a++)
            s_next_ready[p][a] = 0;
}

#if OC_DEBUG_LOG
void combat_cooldown_note(SLONG player_id, SLONG action, const char* name, bool fired)
{
    SLONG pidx = player_id - 1;
    // Player-only log (like combo_gate): never spam the on-screen log
    // with NPC actions (e.g. AI grapples).
    if (pidx < 0 || pidx >= MAX_PLAYERS || action < 0 || action >= COOLDOWN_COUNT)
        return;
    SLONG rem = s_next_ready[pidx][action] - (SLONG)GAME_TURN; // ticks left
    if (rem < 0)
        rem = 0;
    // GAME_TURN ticks at UC_PHYSICS_DESIGN_HZ -> tenths of a second.
    SLONG tenths = rem * 10 / UC_PHYSICS_DESIGN_HZ;

    // NAME (white)  detail (green=fired / orange=remaining)  RESULT.
    // Tabs pad columns so successive lines align (see dbglog.h).
    DBGLOG_begin();
    DBGLOG_seg(DBGLOG_color("white"), "%s\t\t", name);
    if (fired) {
        DBGLOG_seg(DBGLOG_color("green"), "ready\t\t");
        DBGLOG_seg(DBGLOG_color("green"), "FIRED");
    } else {
        DBGLOG_seg(DBGLOG_color("orange"), "%d.%ds\t\t",
            (int)(tenths / 10), (int)(tenths % 10));
        DBGLOG_seg(DBGLOG_color("red"), "BLOCKED");
    }
    DBGLOG_commit();
}
#else
void combat_cooldown_note(SLONG, SLONG, const char*, bool) {}
#endif
