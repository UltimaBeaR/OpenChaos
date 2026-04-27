#include "things/core/state.h"
#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "things/core/player_globals.h"          // player_functions
#include "things/characters/person_globals.h"     // people_functions, generic_people_functions
#include "things/vehicles/vehicle_globals.h"      // VEH_statefunctions
#include "things/animals/animal_globals.h"        // ANIMAL_functions
#include "things/vehicles/chopper_globals.h"      // CHOPPER_functions
#include "effects/combat/pyro_globals.h"                 // PYRO_functions

// uc_orig: set_state_function (fallen/Source/State.cpp)
void set_state_function(Thing* t_thing, UBYTE state)
{
    StateFunction* functions = NULL;

    switch (t_thing->Class) {
    case CLASS_NONE:
        break;
    case CLASS_PLAYER:
        functions = player_functions[t_thing->Genus.Player->PlayerType].StateFunctions;
        break;
    case CLASS_CAMERA:
        break;
    case CLASS_PROJECTILE:
        break;
    case CLASS_BUILDING:
        break;
    case CLASS_PERSON:
        functions = people_functions[t_thing->Genus.Person->PersonType].StateFunctions;
        break;
    case CLASS_VEHICLE:
        functions = VEH_statefunctions;
        break;
    case CLASS_ANIMAL:
        functions = ANIMAL_functions[t_thing->Genus.Animal->AnimalType].StateFunctions;
        break;
    case CLASS_CHOPPER:
        functions = CHOPPER_functions[t_thing->Genus.Chopper->ChopperType].StateFunctions;
        break;
    case CLASS_PYRO:
        functions = PYRO_functions[t_thing->Genus.Pyro->PyroType].StateFunctions;
        break;
    default:
        ASSERT(0);
    }

    if (functions) {
        if (state < 5)
            ASSERT(functions[state].State == state);

        t_thing->StateFn = functions[state].StateFn;
        t_thing->State = state;
    }
}

// uc_orig: set_generic_person_state_function (fallen/Source/State.cpp)
void set_generic_person_state_function(Thing* t_thing, UBYTE state)
{
    t_thing->StateFn = generic_people_functions[state].StateFn;
    t_thing->State = state;
}

