// Prototype enemy initializer from December 1997. Dead code — init_enemy is never called.

#include "game.h"
#include "person.h"
#include "animate.h"

#include "actors/characters/enemy.h"

extern Thing* darci_thing;

// Sets up a tweened enemy thing with default stance animation.
// The actual StateFn assignment is commented out in the original.
// uc_orig: init_enemy (fallen/Source/Enemy.cpp)
void init_enemy(Thing* e_thing)
{
    e_thing->DrawType = DT_ROT_MULTI;

    e_thing->Draw.Tweened->Angle = 0;
    e_thing->Draw.Tweened->Roll = 0;
    e_thing->Draw.Tweened->Tilt = 0;
    e_thing->Draw.Tweened->AnimTween = 0;
    e_thing->Draw.Tweened->TweenStage = 0;
    e_thing->Draw.Tweened->TheChunk = &game_chunk[0];

    // e_thing->StateFn = (void(*)(Thing*))process_enemy;

    set_anim(e_thing, ANIM_STAND_READY);
    add_thing_to_map(e_thing);
}
