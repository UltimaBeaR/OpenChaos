#ifndef ACTORS_ANIMALS_CANID_H
#define ACTORS_ANIMALS_CANID_H

#include "actors/core/state.h"

struct Thing;

// uc_orig: CANID_state_function (fallen/Source/canid.cpp)
// State dispatch table for canid (dog) entities: STATE_INIT and STATE_NORMAL.
extern StateFunction CANID_state_function[];

// uc_orig: CANID_init (fallen/Headers/canid.h)
// Called at STATE_INIT: records home position, transitions to STATE_NORMAL.
void CANID_init(Thing* canid);

// uc_orig: CANID_normal (fallen/Headers/canid.h)
// Per-frame update for a canid in STATE_NORMAL.
void CANID_normal(Thing* canid);

// uc_orig: CANID_register (fallen/Headers/canid.h)
// Registers canid animation data (stub in pre-release — all calls commented out).
void CANID_register();

// State machine handlers — used by CANID_state_function[] table.
// uc_orig: CANID_fn_init (fallen/Source/canid.cpp)
void CANID_fn_init(Thing* canid);
// uc_orig: CANID_fn_normal (fallen/Source/canid.cpp)
void CANID_fn_normal(Thing* canid);

#endif // ACTORS_ANIMALS_CANID_H
