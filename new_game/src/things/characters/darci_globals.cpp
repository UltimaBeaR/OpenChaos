#include <stddef.h>
#include "things/core/statedef.h"
#include "things/characters/darci_globals.h"

struct Thing;

// Forward declarations for state functions defined in darci.cpp.
// uc_orig: fn_darci_init (fallen/Source/Darci.cpp)
void fn_darci_init(Thing*);
// uc_orig: fn_person_moveing (fallen/Source/Person.cpp)
void fn_person_moveing(Thing*);
// uc_orig: fn_person_idle (fallen/Source/Person.cpp)
void fn_person_idle(Thing*);

// uc_orig: darci_states (fallen/Source/Darci.cpp)
// State dispatch table for Darci (the player-controlled character).
// STATE_NORMAL = NULL because Darci has no AI mode — she is driven by player input.
StateFunction darci_states[] = {
    { STATE_INIT, fn_darci_init },
    { STATE_NORMAL, NULL },
    { STATE_HIT, NULL },
    { STATE_ABOUT_TO_REMOVE, NULL },
    { STATE_REMOVE_ME, NULL },
    { STATE_MOVEING, fn_person_moveing },
    { STATE_IDLE, fn_person_idle }
};

// uc_orig: just_started_falling_off_backwards (fallen/Source/Darci.cpp)
UBYTE just_started_falling_off_backwards;
