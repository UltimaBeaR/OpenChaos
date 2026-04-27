#ifndef THINGS_ANIMALS_ANIMAL_H
#define THINGS_ANIMALS_ANIMAL_H

#include "engine/core/types.h"
#include "engine/core/vector.h"
#include "things/core/state.h"
#include <stddef.h>

struct Thing;

// uc_orig: MAX_ANIMALS (fallen/Headers/animal.h)
#define MAX_ANIMALS 6

// uc_orig: ANIMAL_NONE (fallen/Headers/animal.h)
#define ANIMAL_NONE 0
// uc_orig: ANIMAL_CANID (fallen/Headers/animal.h)
#define ANIMAL_CANID 1
// uc_orig: ANIMAL_NUMBER (fallen/Headers/animal.h)
#define ANIMAL_NUMBER 2

// uc_orig: Animal (fallen/Headers/animal.h)
// Per-animal instance data. Stored in the_game.Animals[].
typedef struct
{
    Thing* target;   // Currently chasing or barking at this thing.
    UWORD counter;   // Random delays and timing.
    UWORD dist;      // Generically useful for pathfinding/chasing.
    UWORD starty;    // Initial height — may be removed.
    UBYTE AnimalType; // Species: ANIMAL_CANID etc.
    UBYTE substate;  // Behaviour sub-state.
    UBYTE map_x;     // Ready-shifted world X position on map.
    UBYTE map_z;     // Ready-shifted world Z position on map.
    UBYTE dest_x, dest_z; // Temporary destination.
    UBYTE home_x, home_z; // Spawn/home point.
    UBYTE extra;     // Animal-specific extra data.
    UBYTE padding;
} Animal;

// uc_orig: AnimalPtr (fallen/Headers/animal.h)
typedef Animal* AnimalPtr;

// uc_orig: ANIMAL_functions (fallen/Headers/animal.h)
// Dispatch table mapping animal genus to state function tables.
extern GenusFunctions ANIMAL_functions[ANIMAL_NUMBER];

// uc_orig: init_animals (fallen/Headers/animal.h)
// Zeros out the Animals array in the_game and resets the animal counter.
void init_animals(void);


#endif // THINGS_ANIMALS_ANIMAL_H
