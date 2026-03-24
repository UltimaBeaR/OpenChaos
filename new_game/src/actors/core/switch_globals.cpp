#include "engine/platform/platform.h"
#include "actors/core/switch_globals.h"
#include "actors/core/switch.h"

// uc_orig: switch_functions (fallen/Source/Switch.cpp)
// Maps each switch type to its per-frame processing function.
StateFunction switch_functions[] = {
    { SWITCH_NONE, NULL },
    { SWITCH_PLAYER, fn_switch_player },
    { SWITCH_THING, fn_switch_thing },
    { SWITCH_GROUP, fn_switch_group },
    { SWITCH_CLASS, fn_switch_class }
};
