#ifndef ACTORS_CHARACTERS_COP_H
#define ACTORS_CHARACTERS_COP_H

#include "engine/core/types.h"
#include "actors/core/state.h"

struct Thing;

// Animation indices for cop/NPC characters.
// uc_orig: COP_ANIM_WALK (fallen/Headers/Cop.h)
#define COP_ANIM_WALK (1)
// uc_orig: COP_ANIM_STAND (fallen/Headers/Cop.h)
#define COP_ANIM_STAND (2)
// uc_orig: COP_ANIM_HIT (fallen/Headers/Cop.h)
#define COP_ANIM_HIT (3)
// uc_orig: COP_ANIM_IDLE (fallen/Headers/Cop.h)
#define COP_ANIM_IDLE (4)
// uc_orig: COP_ANIM_KICK (fallen/Headers/Cop.h)
#define COP_ANIM_KICK (5)
// uc_orig: COP_ANIM_HEAD_HIT_SMALL (fallen/Headers/Cop.h)
#define COP_ANIM_HEAD_HIT_SMALL (6)
// uc_orig: COP_ANIM_GUT_HIT_SMALL (fallen/Headers/Cop.h)
#define COP_ANIM_GUT_HIT_SMALL (7)
// uc_orig: COP_ANIM_GUT_DEATH (fallen/Headers/Cop.h)
#define COP_ANIM_GUT_DEATH (8)
// uc_orig: COP_ANIM_DEATH (fallen/Headers/Cop.h)
#define COP_ANIM_DEATH (9)
// uc_orig: COP_ANIM_JAB (fallen/Headers/Cop.h)
#define COP_ANIM_JAB (10)
// uc_orig: COP_ANIM_DIE_KNECK (fallen/Headers/Cop.h)
#define COP_ANIM_DIE_KNECK (12)
// uc_orig: COP_ANIM_BLOCK (fallen/Headers/Cop.h)
#define COP_ANIM_BLOCK (23)
// uc_orig: COP_ANIM_JUMP_UP_GRAB (fallen/Headers/Cop.h)
#define COP_ANIM_JUMP_UP_GRAB (17)
// uc_orig: COP_ANIM_PULL_UP (fallen/Headers/Cop.h)
#define COP_ANIM_PULL_UP (18)
// uc_orig: COP_ANIM_JAB2 (fallen/Headers/Cop.h)
#define COP_ANIM_JAB2 (25)
// uc_orig: COP_ANIM_IDLE2 (fallen/Headers/Cop.h)
#define COP_ANIM_IDLE2 (14)
// uc_orig: COP_ANIM_IDLE3 (fallen/Headers/Cop.h)
#define COP_ANIM_IDLE3 (15)
// uc_orig: COP_ANIM_BREATHE (fallen/Headers/Cop.h)
#define COP_ANIM_BREATHE (11)

// uc_orig: fn_cop_init (fallen/Headers/Cop.h)
void fn_cop_init(Thing* t_thing);

// uc_orig: fn_cop_normal (fallen/Headers/Cop.h)
void fn_cop_normal(Thing* t_thing);

#endif // ACTORS_CHARACTERS_COP_H
