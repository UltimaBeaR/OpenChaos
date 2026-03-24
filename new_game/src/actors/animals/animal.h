#ifndef ACTORS_ANIMALS_ANIMAL_H
#define ACTORS_ANIMALS_ANIMAL_H

#include "engine/core/types.h"
#include "engine/core/vector.h"
#include "actors/core/state.h"
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

// uc_orig: alloc_animal (fallen/Headers/animal.h)
// Allocates an animal Thing of the given type.
struct Thing* alloc_animal(UBYTE type);

// uc_orig: free_animal (fallen/Headers/animal.h)
// Frees an animal's Thing, DrawTween, and Animal structures.
void free_animal(struct Thing* animal_thing);

// uc_orig: ANIMAL_create (fallen/Headers/animal.h)
// Creates an animal Thing at the given position and triggers its STATE_INIT.
Thing* ANIMAL_create(GameCoord pos, UBYTE type);

// uc_orig: ANIMAL_animate (fallen/Headers/animal.h)
// Advances the animal's tween animation one step. Returns 1 if the animation finished.
UBYTE ANIMAL_animate(Thing* animal);

// uc_orig: ANIMAL_set_anim (fallen/Headers/animal.h)
// Changes the current animation on an animal's DrawTween.
void ANIMAL_set_anim(Thing* thing, SLONG anim);

// uc_orig: ANIMAL_draw (fallen/Headers/animal.h)
// Draws the animal — provided for AENG.
void ANIMAL_draw(Thing* p_thing);

// uc_orig: ANIMAL_get_animal (fallen/Headers/animal.h)
// Returns the Animal struct associated with an animal Thing.
Animal* ANIMAL_get_animal(Thing* animal_thing);

#endif // ACTORS_ANIMALS_ANIMAL_H
