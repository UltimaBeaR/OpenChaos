// Expanding-ring effect: prototype code from December 1997, never used in the final game.
// init_effect has no external callers; process_effect is only set as a StateFn internally.

#include "missions/game_types.h"

#include "actors/core/effect.h"

// Reuse the OnFace and Index fields to store speed and radius.
#define SPEED OnFace
#define RADIUS Index

// uc_orig: process_effect (fallen/Source/Effect.cpp)
static void process_effect(Thing* e_thing);

// Sets up an expanding-ring effect thing. Initial speed=128, radius grows each tick.
// uc_orig: init_effect (fallen/Source/Effect.cpp)
void init_effect(Thing* e_thing)
{
    e_thing->DrawType = DT_EFFECT;
    e_thing->SPEED = 128;
    e_thing->RADIUS = -((e_thing->SPEED * 50) / 64);

    e_thing->StateFn = (void (*)(Thing*))process_effect;

    add_thing_to_map(e_thing);
}

// Expands the ring each tick by the current speed (which decays 50/64 per tick).
// Frees the thing when speed reaches zero.
// uc_orig: process_effect (fallen/Source/Effect.cpp)
static void process_effect(Thing* e_thing)
{
    e_thing->SPEED = (e_thing->SPEED * 50) / 64;
    if (e_thing->SPEED)
        e_thing->RADIUS += e_thing->SPEED;
    else {
        remove_thing_from_map(e_thing);
        free_thing(e_thing);
    }
}
