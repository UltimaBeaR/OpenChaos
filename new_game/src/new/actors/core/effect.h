#ifndef ACTORS_CORE_EFFECT_H
#define ACTORS_CORE_EFFECT_H

// Prototype expanding-ring effect, used only in very early testing.
// Sets DrawType=DT_EFFECT and runs an expanding/fading radius animation.
// Not used in the final game (init_effect is never called externally).

struct Thing;

// uc_orig: init_effect (fallen/Source/Effect.cpp)
void init_effect(Thing* e_thing);

#endif // ACTORS_CORE_EFFECT_H
