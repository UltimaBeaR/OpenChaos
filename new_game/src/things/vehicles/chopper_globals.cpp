#include "things/vehicles/chopper_globals.h"
#include "things/core/statedef.h"

// Forward declarations for state functions defined in chopper.cpp.
void CHOPPER_fn_init(Thing*);
void CHOPPER_fn_normal(Thing*);

// uc_orig: CIVILIAN_state_function (fallen/Source/chopper.cpp)
// State table for civilian-type choppers.
StateFunction CIVILIAN_state_function[] = {
    { STATE_INIT, CHOPPER_fn_init },
    { STATE_NORMAL, CHOPPER_fn_normal }
};

// uc_orig: CHOPPER_functions (fallen/Source/chopper.cpp)
// Genus dispatch table: maps chopper type to its state function table.
GenusFunctions CHOPPER_functions[CHOPPER_NUMB] = {
    { CHOPPER_NONE, NULL },
    { CHOPPER_CIVILIAN, CIVILIAN_state_function }
};
