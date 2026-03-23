#include "actors/animals/canid_globals.h"
#include "actors/animals/canid.h"
#include "actors/core/statedef.h"

// uc_orig: CANID_state_function (fallen/Source/canid.cpp)
StateFunction CANID_state_function[] = {
    { STATE_INIT,   CANID_fn_init   },
    { STATE_NORMAL, CANID_fn_normal }
};
