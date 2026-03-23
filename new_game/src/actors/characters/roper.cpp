#include "actors/characters/roper.h"
#include "actors/characters/roper_globals.h"
#include "fallen/Headers/Game.h"
#include "actors/core/statedef.h"
#include "actors/characters/anim_ids.h"

// uc_orig: calc_height_at (fallen/Source/Roper.cpp)
extern SLONG calc_height_at(SLONG x, SLONG z);

// uc_orig: fn_roper_init (fallen/Source/Roper.cpp)
void fn_roper_init(Thing* t_thing)
{
    t_thing->DrawType = DT_ROT_MULTI;
    t_thing->Draw.Tweened->Roll = 0;
    t_thing->Draw.Tweened->Tilt = 0;
    t_thing->Draw.Tweened->AnimTween = 0;
    t_thing->Draw.Tweened->TweenStage = 0;
    t_thing->Draw.Tweened->QueuedFrame = NULL;

    t_thing->Genus.Person->Health = health[t_thing->Genus.Person->PersonType];

    add_thing_to_map(t_thing);
    set_person_idle(t_thing);
}

// uc_orig: fn_roper_normal (fallen/Source/Roper.cpp)
// Empty stub — Roper is controlled via the generic person AI in the final game.
void fn_roper_normal(Thing* t_thing)
{
    return;
}
