#ifndef ACTORS_CORE_STATE_H
#define ACTORS_CORE_STATE_H

#include "engine/core/types.h"

struct Thing;

// uc_orig: StateFunction (fallen/Headers/State.h)
// Maps a state ID to its handler function — used to build state tables for all actor types.
typedef struct
{
    UBYTE State;
    void (*StateFn)(Thing*);
} StateFunction;

// uc_orig: GenusFunctions (fallen/Headers/State.h)
// Maps a genus (player/person/vehicle/etc.) type ID to its state function table.
typedef struct
{
    UBYTE Genus;
    StateFunction* StateFunctions;
} GenusFunctions;

// uc_orig: set_state_function (fallen/Headers/State.h)
// Looks up the state function for the given state index in the thing's class-specific table
// and assigns it to t_thing->StateFn. Also sets t_thing->State.
void set_state_function(Thing* t_thing, UBYTE state);

// uc_orig: set_generic_person_state_function (fallen/Headers/State.h)
// Like set_state_function but always uses the generic_people_functions table regardless of class.
void set_generic_person_state_function(Thing* t_thing, UBYTE state);

// uc_orig: set_generic_person_just_function (fallen/Headers/State.h)
// Assigns StateFn from generic_people_functions[state] without updating t_thing->State.
void set_generic_person_just_function(Thing* t_thing, UBYTE state);

#endif // ACTORS_CORE_STATE_H
