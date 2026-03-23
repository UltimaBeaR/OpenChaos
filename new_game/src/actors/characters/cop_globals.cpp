#include <platform.h>
#include "actors/characters/cop_globals.h"
#include "actors/characters/cop.h"
#include "actors/core/statedef.h"

// uc_orig: cop_states (fallen/Source/Cop.cpp)
// State table for cops and most NPC person types. fn_cop_normal is a no-op stub in pre-release —
// actual NPC AI runs through pcom.cpp (PCOM_process_person).
StateFunction cop_states[10] = {
    { STATE_INIT, fn_cop_init },
    { STATE_NORMAL, fn_cop_normal },
    { STATE_HIT, NULL },
    { STATE_ABOUT_TO_REMOVE, NULL },
    { STATE_REMOVE_ME, NULL }
};
