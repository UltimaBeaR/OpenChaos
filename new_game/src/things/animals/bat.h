#ifndef THINGS_ANIMALS_BAT_H
#define THINGS_ANIMALS_BAT_H

#include "engine/core/types.h"

struct Thing;

// GameCoord is defined in Structs.h — use guard pattern for standalone use.
#ifndef THING_INDEX
#define THING_INDEX UWORD
#endif

// uc_orig: BAT_TYPE_UNUSED (fallen/Headers/bat.h)
#define BAT_TYPE_UNUSED 0
// uc_orig: BAT_TYPE_BAT (fallen/Headers/bat.h)
#define BAT_TYPE_BAT 1
// uc_orig: BAT_TYPE_GARGOYLE (fallen/Headers/bat.h)
#define BAT_TYPE_GARGOYLE 2
// uc_orig: BAT_TYPE_BALROG (fallen/Headers/bat.h)
#define BAT_TYPE_BALROG 3
// uc_orig: BAT_TYPE_BANE (fallen/Headers/bat.h)
#define BAT_TYPE_BANE 4

// uc_orig: Bat (fallen/Headers/bat.h)
// Per-bat instance data stored in the_game.Bats[].
typedef struct
{
    UBYTE type; // BAT_TYPE_*
    UBYTE health;
    UBYTE state; // BAT_STATE_*
    UBYTE substate; // BAT_SUBSTATE_*
    UBYTE home_x; // Home mapsquare X (WorldPos.X >> 16)
    UBYTE home_z; // Home mapsquare Z (WorldPos.Z >> 16)
    UWORD target; // THING_INDEX of current attack target, or 0
    UWORD timer;
    UWORD flag; // BAT_FLAG_* bits
    SWORD want_x; // Target position (WorldPos units >> 8)
    SWORD want_y;
    SWORD want_z;
    UWORD glow; // Bane: how much it glows
    SLONG dx; // Balrog: movement delta X
    SLONG dy;
    SLONG dz;

} Bat;

// uc_orig: BatPtr (fallen/Headers/bat.h)
typedef Bat* BatPtr;

// uc_orig: RBAT_MAX_BATS (fallen/Headers/bat.h)
#define RBAT_MAX_BATS 40
// uc_orig: BAT_MAX_BATS (fallen/Headers/bat.h)
#define BAT_MAX_BATS (save_table[SAVE_TABLE_BAT].Maximum)

// uc_orig: BAT_init (fallen/Headers/bat.h)
void BAT_init(void);

// uc_orig: BAT_create (fallen/Headers/bat.h)
THING_INDEX BAT_create(
    SLONG type,
    SLONG x,
    SLONG z,
    UWORD yaw);

// uc_orig: BAT_apply_hit (fallen/Headers/bat.h)
void BAT_apply_hit(
    Thing* p_me,
    Thing* p_aggressor,
    SLONG damage);

// Functions used internally but also called from other modules.

// uc_orig: BAT_normal (fallen/Source/bat.cpp)
void BAT_normal(Thing* p_thing);

// uc_orig: BAT_change_state (fallen/Source/bat.cpp)
void BAT_change_state(Thing* p_thing);

// uc_orig: BAT_set_anim (fallen/Source/bat.cpp)
void BAT_set_anim(Thing* p_thing, SLONG anim);

// uc_orig: BAT_animate (fallen/Source/bat.cpp)
SLONG BAT_animate(Thing* p_thing);

// uc_orig: BAT_turn_to_target (fallen/Source/bat.cpp)
SLONG BAT_turn_to_target(Thing* p_thing);

// uc_orig: BAT_turn_to_place (fallen/Source/bat.cpp)
SLONG BAT_turn_to_place(Thing* p_thing, SLONG world_x, SLONG world_z);

// uc_orig: BAT_emit_fireball (fallen/Source/bat.cpp)
void BAT_emit_fireball(Thing* p_thing);

// uc_orig: BAT_balrog_slide_along (fallen/Source/bat.cpp)
void BAT_balrog_slide_along(
    SLONG x1, SLONG z1,
    SLONG* x2, SLONG* z2);

// uc_orig: BAT_find_summon_people (fallen/Source/bat.cpp)
void BAT_find_summon_people(Thing* p_thing);

// uc_orig: BAT_process_bane_sparks (fallen/Source/bat.cpp)
void BAT_process_bane_sparks(Thing* p_thing);

#endif // THINGS_ANIMALS_BAT_H
