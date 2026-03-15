// Thug.cpp
// Guy Simmons, 8th February 1998.
// claude-ai: Thug — NPC-враги. thug_states[] используется только для PERSON_THUG_*.
// claude-ai: fn_thug_init() содержит ASSERT(0) — намеренный crash при вызове (код сломан в пре-релизе).
// claude-ai: В финальной игре thugs инициализируются через fn_cop_init() (cop_states), не через thug_states.

#include "Game.h"
// #include	"Command.h"
#include "Thug.h"
#include "statedef.h"
#include "animate.h"
#include "pap.h"

SLONG calc_height_at(SLONG x, SLONG z);
SLONG person_normal_animate(Thing* p_person);

#define THUG_IDLE 1

#undef THUG_ANIM_WALK
#define THUG_ANIM_WALK 1
#define THUG_ANIM_IDLE 0

//---------------------------------------------------------------

StateFunction thug_states[] = {
    { STATE_INIT, fn_thug_init },
    { STATE_NORMAL, fn_thug_normal },
    { STATE_HIT, NULL },
    { STATE_ABOUT_TO_REMOVE, NULL },
    { STATE_REMOVE_ME, NULL }
};

//---------------------------------------------------------------

// claude-ai: ASSERT(0) в начале — этот init намеренно вызывает crash. Не используется в финальной игре.
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
    //	t_thing->Genus.Person->Health		=	200;
    t_thing->Genus.Person->Health = health[t_thing->Genus.Person->PersonType];

    // Initialise the cop to be walking.
    //	t_thing->Genus.Person->State		=	0;
    set_thing_velocity(t_thing, 10);

    // Set up the command index;
    //	t_thing->Genus.Person->Command		=	t_thing->Genus.Person->CommandRef;

    set_anim(t_thing, ANIM_STAND_READY);

    set_state_function(t_thing, STATE_NORMAL);
    //	set_person_idle(t_thing);
    add_thing_to_map(t_thing);
}

//---------------------------------------------------------------

void fn_thug_normal(Thing* t_thing)
{
}

//---------------------------------------------------------------
