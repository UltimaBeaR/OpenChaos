// uc_orig: animal.cpp (fallen/Source/animal.cpp)
// Animal system — allocation, animation, and state dispatch for non-human creatures.
// Note: Most functions contain ASSERT(0) stubs — the animal system was not completed
// in the pre-release build.

#include "fallen/Headers/Game.h"
#include "actors/core/statedef.h"
#include "actors/animals/animal.h"
#include "actors/animals/animal_globals.h"

// Forward declarations for not-yet-migrated functions.
extern SLONG load_anim_system(struct GameKeyFrameChunk* game_chunk, CBYTE* name, SLONG peep = 0);

// uc_orig: alloc_tween (fallen/Source/animal.cpp)
// Allocates and initialises a DrawTween for a given animal body part.
// Currently stubs out with ASSERT(0) — not used in the pre-release build.
static DrawTween* alloc_tween(SLONG type, SLONG part)
{
    SLONG chunk;
    DrawTween* dt;

    ASSERT(0);
    return (0);

    chunk = 6;
    dt = alloc_draw_tween(DT_ANIMAL_PRIM);

    if (dt) {
        dt->Angle = 0;
        dt->Roll = 0;
        dt->Tilt = 0;

        dt->Angle = 0;
        dt->Roll = 0;
        dt->Tilt = 0;
        dt->AnimTween = 0;
        dt->TweenStage = 0;
        dt->NextFrame = NULL;
        dt->QueuedFrame = NULL;
        dt->TheChunk = &game_chunk[chunk];
        dt->CurrentFrame = game_chunk[chunk].AnimList[1];
        if (dt->CurrentFrame) {
            dt->NextFrame = dt->CurrentFrame->NextFrame;
        }
        dt->FrameIndex = 0;
        dt->Flags = 0;
    }

    return dt;
}

// uc_orig: init_animals (fallen/Source/animal.cpp)
void init_animals(void)
{
    memset((UBYTE*)ANIMALS, 0, sizeof(ANIMALS));
    ANIMAL_COUNT = 0;
}

// uc_orig: alloc_animal (fallen/Source/animal.cpp)
Thing* alloc_animal(UBYTE type)
{
    SLONG i;

    Thing* p_thing;
    Animal* p_animal;
    DrawTween* dt;

    THING_INDEX t_index;
    SLONG a_index;

    ASSERT(WITHIN(type, 1, ANIMAL_NUMBER - 1));
    ASSERT(0);
    return (0);

    for (i = 1; i < MAX_ANIMALS; i++) {
        if (ANIMALS[i].AnimalType == ANIMAL_NONE) {
            a_index = i;
            goto found_animal;
        }
    }

    return NULL;

found_animal:
    dt = alloc_tween(type, 0);
    if (dt == NULL) {
        TO_ANIMAL(a_index)->AnimalType = ANIMAL_NONE;
        return NULL;
    }

    t_index = alloc_primary_thing(CLASS_ANIMAL);

    if (t_index) {
        p_thing = TO_THING(t_index);
        p_animal = TO_ANIMAL(a_index);

        p_thing->Class = CLASS_ANIMAL;
        p_thing->Genus.Animal = p_animal;
        p_animal->AnimalType = type;

        p_thing->DrawType = DT_ANIMAL_PRIM;
        p_thing->Draw.Tweened = dt;

        return p_thing;
    } else {
        TO_ANIMAL(a_index)->AnimalType = ANIMAL_NONE;
        free_draw_tween(dt);
    }
}

// uc_orig: free_animal (fallen/Source/animal.cpp)
void free_animal(Thing* p_thing)
{
    SLONG i;
    Animal* animal = ANIMAL_get_animal(p_thing);
    ASSERT(0);
    return;

    free_draw_tween(p_thing->Draw.Tweened);

    animal->AnimalType = ANIMAL_NONE;
    ANIMAL_COUNT -= 1;

    free_thing(p_thing);
}

// uc_orig: ANIMAL_create (fallen/Source/animal.cpp)
Thing* ANIMAL_create(GameCoord pos, UBYTE type)
{
    Thing* p_thing = alloc_animal(type);
    return (0);

    if (p_thing != NULL) {
        p_thing->WorldPos = pos;

        add_thing_to_map(p_thing);

        set_state_function(p_thing, STATE_INIT);
    }

    return p_thing;
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
    UBYTE i, j = 0;
    ASSERT(0);
    j = ANIMAL_animatetween(animal->Draw.Tweened);
    return j;
}

// uc_orig: ANIMAL_set_anim (fallen/Source/animal.cpp)
void ANIMAL_set_anim(Thing* thing, SLONG anim)
{
    Animal* animal = ANIMAL_get_animal(thing);
    UBYTE i;
    SLONG chunk;
    DrawTween* dt;
    ASSERT(0);
    return;

    chunk = 6;
    i = 0;
    {
        dt = thing->Draw.Tweened;
        dt->AnimTween = 0;
        dt->NextFrame = NULL;
        dt->QueuedFrame = NULL;
        dt->CurrentFrame = game_chunk[chunk].AnimList[anim];
        dt->CurrentAnim = anim;
        dt->FrameIndex = 0;

        if (dt->CurrentFrame)
            dt->NextFrame = dt->CurrentFrame->NextFrame;
        chunk++;
    }
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

// uc_orig: GameKeyFrameChunk* ANIMAL_register (fallen/Headers/animal.h)
// Registers an animal body-part animation file. Stub in the pre-release build.
struct GameKeyFrameChunk* ANIMAL_register(char* filename)
{
    return NULL;
}
