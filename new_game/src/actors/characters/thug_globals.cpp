#include <platform.h>
#include "actors/characters/thug_globals.h"
#include "actors/characters/thug.h"
#include "actors/core/statedef.h"

// uc_orig: thug_states (fallen/Source/Thug.cpp)
// State table for thug NPCs. fn_thug_init has ASSERT(0) — thugs are initialized via fn_cop_init
// in the final game, not through this table.
StateFunction thug_states[] = {
    { STATE_INIT, fn_thug_init },
    { STATE_NORMAL, fn_thug_normal },
    { STATE_HIT, NULL },
    { STATE_ABOUT_TO_REMOVE, NULL },
    { STATE_REMOVE_ME, NULL }
};
