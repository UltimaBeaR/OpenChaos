#include "actors/characters/thug.h"
#include "actors/characters/thug_globals.h"
#include <platform.h>
#include "missions/game_types.h"
#include "engine/animation/anim_types.h"      // GameKeyFrame
#include "assets/anim_globals.h"               // game_chunk
#include "actors/characters/person.h"          // set_thing_velocity, health, set_anim
#include "actors/characters/person_globals.h"  // health[]
#include "actors/core/statedef.h"
#include "actors/characters/anim_ids.h"
#include "world/map/pap_globals.h"

// uc_orig: calc_height_at (fallen/Source/Thug.cpp)
extern SLONG calc_height_at(SLONG x, SLONG z);
// uc_orig: person_normal_animate (fallen/Source/Thug.cpp)
extern SLONG person_normal_animate(Thing* p_person);

// uc_orig: fn_thug_init (fallen/Source/Thug.cpp)
// Intentional ASSERT(0) at start — this init is not used in the final game.
// Thugs are initialized through fn_cop_init (cop_states) instead.
void fn_thug_init(Thing* t_thing)
{
    ASSERT(0);
    t_thing->DrawType = DT_ROT_MULTI;
    t_thing->Draw.Tweened->Angle = 0;
    t_thing->Draw.Tweened->Roll = 0;
    t_thing->Draw.Tweened->Tilt = 0;
    t_thing->Draw.Tweened->AnimTween = 0;
    t_thing->Draw.Tweened->TweenStage = 0;
    t_thing->Draw.Tweened->QueuedFrame = NULL;
    t_thing->Draw.Tweened->TheChunk = &game_chunk[1];
    t_thing->Genus.Person->Health = health[t_thing->Genus.Person->PersonType];

    set_thing_velocity(t_thing, 10);

    set_anim(t_thing, ANIM_STAND_READY);

    set_state_function(t_thing, STATE_NORMAL);
    add_thing_to_map(t_thing);
}

// uc_orig: fn_thug_normal (fallen/Source/Thug.cpp)
void fn_thug_normal(Thing* t_thing)
{
}
