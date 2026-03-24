#include "things/characters/cop.h"
#include "things/characters/cop_globals.h"
#include "engine/platform/platform.h"
#include "missions/game_types.h"
#include "engine/animation/anim_types.h"      // GameKeyFrame
#include "assets/anim_globals.h"               // global_anim_array
#include "things/characters/person.h"          // set_thing_velocity, set_person_idle, health, set_anim
#include "things/characters/person_globals.h"  // health[]
#include "things/core/statedef.h"
#include "things/characters/anim_ids.h"

// Forward declarations for functions defined in other not-yet-migrated files.
// uc_orig: calc_height_at (fallen/Source/Cop.cpp)
extern SLONG calc_height_at(SLONG x, SLONG z);
// uc_orig: person_normal_animate (fallen/Source/Cop.cpp)
extern SLONG person_normal_animate(Thing* p_person);
// uc_orig: change_velocity_to (fallen/Source/Cop.cpp)
extern void change_velocity_to(Thing* p_person, SWORD velocity);
// uc_orig: person_normal_move (fallen/Source/Cop.cpp)
extern void person_normal_move(Thing* p_person);
// uc_orig: fn_person_moveing (fallen/Source/Cop.cpp)
extern void fn_person_moveing(Thing* p_person);
// uc_orig: fn_person_idle (fallen/Source/Cop.cpp)
extern void fn_person_idle(Thing* p_person);
// uc_orig: fn_person_jumping (fallen/Source/Cop.cpp)
extern void fn_person_jumping(Thing* p_person);
// uc_orig: fn_person_dangling (fallen/Source/Cop.cpp)
extern void fn_person_dangling(Thing* p_person);
// uc_orig: fn_person_laddering (fallen/Source/Cop.cpp)
extern void fn_person_laddering(Thing* p_person);
// uc_orig: fn_person_climbing (fallen/Source/Cop.cpp)
extern void fn_person_climbing(Thing* p_person);
// uc_orig: fn_person_fighting (fallen/Source/Cop.cpp)
extern void fn_person_fighting(Thing* p_person);
// uc_orig: fn_person_recoil (fallen/Source/Cop.cpp)
extern void fn_person_recoil(Thing* p_person);
// uc_orig: fn_person_dying (fallen/Source/Cop.cpp)
extern void fn_person_dying(Thing* p_person);
// uc_orig: fn_person_dead (fallen/Source/Cop.cpp)
extern void fn_person_dead(Thing* p_person);
// uc_orig: fn_person_gun (fallen/Source/Cop.cpp)
extern void fn_person_gun(Thing* p_person);
// uc_orig: fn_person_wait (fallen/Source/Cop.cpp)
extern void fn_person_wait(Thing* p_person);
// uc_orig: fn_person_fight (fallen/Source/Cop.cpp)
extern void fn_person_fight(Thing* p_person);
// uc_orig: fn_person_stand_up (fallen/Source/Cop.cpp)
extern void fn_person_stand_up(Thing* p_person);

// uc_orig: fn_cop_init (fallen/Source/Cop.cpp)
void fn_cop_init(Thing* t_thing)
{
    t_thing->DrawType = DT_ROT_MULTI;
    t_thing->Draw.Tweened->Roll = 0;
    t_thing->Draw.Tweened->Tilt = 0;
    t_thing->Draw.Tweened->AnimTween = 0;
    t_thing->Draw.Tweened->TweenStage = 0;
    t_thing->Draw.Tweened->CurrentFrame = global_anim_array[t_thing->Genus.Person->AnimType][ANIM_STAND_READY];
    t_thing->Draw.Tweened->NextFrame = t_thing->Draw.Tweened->CurrentFrame->NextFrame;
    t_thing->Draw.Tweened->QueuedFrame = NULL;
    t_thing->Genus.Person->Health = health[t_thing->Genus.Person->PersonType];

    set_thing_velocity(t_thing, 10);

    set_person_idle(t_thing);
    add_thing_to_map(t_thing);
}

// uc_orig: fn_cop_normal (fallen/Source/Cop.cpp)
// Stub — NPC AI runs through pcom.cpp in the final game, not here.
void fn_cop_normal(Thing* t_thing)
{
}

// uc_orig: fn_cop_fight (fallen/Source/Cop.cpp)
// Stub — combat code is entirely commented out in the pre-release version.
void fn_cop_fight(Thing* p_person)
{
}
