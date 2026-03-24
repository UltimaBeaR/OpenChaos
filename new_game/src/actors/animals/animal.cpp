// uc_orig: animal.cpp (fallen/Source/animal.cpp)
// Animal system — allocation, animation, and state dispatch for non-human creatures.
// Note: Most functions contain ASSERT(0) stubs — the animal system was not completed
// in the pre-release build.

#include <platform.h>
#include "missions/game_types.h"
#include "engine/animation/anim_types.h"     // GameKeyFrame, GameKeyFrameChunk
#include "assets/anim_globals.h"              // game_chunk, global_anim_array
#include "actors/core/statedef.h"
#include "actors/animals/animal.h"
#include "actors/animals/animal_globals.h"

// uc_orig: init_animals (fallen/Source/animal.cpp)
void init_animals(void)
{
    memset((UBYTE*)ANIMALS, 0, sizeof(ANIMALS));
    ANIMAL_COUNT = 0;
}

// uc_orig: alloc_animal (fallen/Source/animal.cpp)
Thing* alloc_animal(UBYTE type)
{
    ASSERT(WITHIN(type, 1, ANIMAL_NUMBER - 1));
    ASSERT(0);
    return (0);
}

// uc_orig: free_animal (fallen/Source/animal.cpp)
void free_animal(Thing* p_thing)
{
    ASSERT(0);
    return;
}

// uc_orig: ANIMAL_create (fallen/Source/animal.cpp)
Thing* ANIMAL_create(GameCoord pos, UBYTE type)
{
    alloc_animal(type);
    return (0);
}

// uc_orig: ANIMAL_animatetween (fallen/Source/animal.cpp)
// Internal helper: advances a DrawTween by one step for an animal.
static UBYTE ANIMAL_animatetween(DrawTween* draw_info)
{
    SLONG tween_step;
    ASSERT(0);

    tween_step = draw_info->CurrentFrame->TweenStep << 1;

    tween_step = (tween_step * TICK_RATIO) >> TICK_SHIFT;
    if (tween_step == 0)
        tween_step = 1;
    draw_info->AnimTween += tween_step;

    if (draw_info->AnimTween > 256) {
        draw_info->AnimTween -= 256;

        // Forward declaration for local use — advance_keyframe is in darci.cpp.
        SLONG advance_keyframe(DrawTween * draw_info);

        return advance_keyframe(draw_info);
    }
    return 0;
}

// uc_orig: ANIMAL_animate (fallen/Source/animal.cpp)
UBYTE ANIMAL_animate(Thing* animal)
{
    UBYTE j = 0;
    ASSERT(0);
    j = ANIMAL_animatetween(animal->Draw.Tweened);
    return j;
}

// uc_orig: ANIMAL_set_anim (fallen/Source/animal.cpp)
void ANIMAL_set_anim(Thing* thing, SLONG anim)
{
    ASSERT(0);
    return;
}

// uc_orig: ANIMAL_get_animal (fallen/Source/animal.cpp)
Animal* ANIMAL_get_animal(struct Thing* animal_thing)
{
    Animal* animal;

    ASSERT(WITHIN(animal_thing, TO_THING(1), TO_THING(MAX_THINGS)));
    ASSERT(animal_thing->Class == CLASS_ANIMAL);

    animal = animal_thing->Genus.Animal;

    ASSERT(WITHIN(animal, TO_ANIMAL(1), TO_ANIMAL(MAX_ANIMALS - 1)));

    return animal;
}

// uc_orig: ANIMAL_draw (fallen/Headers/animal.h)
// Provided for AENG — stub in the pre-release build.
void ANIMAL_draw(Thing* p_thing)
{
    // Stub.
}
