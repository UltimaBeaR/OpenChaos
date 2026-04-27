// uc_orig: animal.cpp (fallen/Source/animal.cpp)
// Animal system — allocation, animation, and state dispatch for non-human creatures.
// Note: Most functions contain ASSERT(0) stubs — the animal system was not completed
// in the pre-release build.

#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "things/animals/animal.h"

// uc_orig: init_animals (fallen/Source/animal.cpp)
void init_animals(void)
{
    memset((UBYTE*)ANIMALS, 0, sizeof(ANIMALS));
    ANIMAL_COUNT = 0;
}

// uc_orig: free_animal (fallen/Source/animal.cpp)
void free_animal(Thing* p_thing)
{
    ASSERT(0);
    return;
}
