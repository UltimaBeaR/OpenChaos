#ifndef ACTORS_CHARACTERS_THUG_H
#define ACTORS_CHARACTERS_THUG_H

#include "engine/core/types.h"
#include "actors/core/state.h"

struct Thing;

// uc_orig: fn_thug_init (fallen/Headers/Thug.h)
// NOTE: fn_thug_init contains ASSERT(0) — intentional crash guard; not called in the final game.
void fn_thug_init(Thing* t_thing);

// uc_orig: fn_thug_normal (fallen/Headers/Thug.h)
void fn_thug_normal(Thing* t_thing);

#endif // ACTORS_CHARACTERS_THUG_H
