#include "MFStdLib.h"
#include "actors/characters/roper_globals.h"
#include "actors/characters/roper.h"
#include "actors/core/statedef.h"

// uc_orig: roper_states (fallen/Source/Roper.cpp)
// State table for Roper. fn_roper_normal is a no-op stub — Roper AI was never completed in pre-release.
StateFunction roper_states[] = {
    { STATE_INIT, fn_roper_init },
    { STATE_NORMAL, fn_roper_normal },
    { STATE_HIT, NULL },
    { STATE_ABOUT_TO_REMOVE, NULL },
    { STATE_REMOVE_ME, NULL }
};
