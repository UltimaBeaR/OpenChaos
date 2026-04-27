#include "engine/platform/uc_common.h"
#include <stdio.h>
#include <stdlib.h>
#include "game/game_types.h"
#include "assets/formats/anim.h"
#include "engine/core/memory.h"
#include "things/characters/anim_ids.h"
#include "things/characters/person_types.h"  // ANIM_TYPE_DARCI/ROPER/CIV/ROPER2

// File-local animation index constants for the "new Roper" animation set (roper.all).
// These indices match the animation's position in the .all file.
// PC-only — PSX build uses psxroper.all with different indices.

// uc_orig: NROPER_ANIM_YOMP (fallen/Source/Anim.cpp)
#define NROPER_ANIM_YOMP (1)
// uc_orig: NROPER_ANIM_STAND_READY (fallen/Source/Anim.cpp)
#define NROPER_ANIM_STAND_READY (2)
// uc_orig: NROPER_ANIM_BREATHE (fallen/Source/Anim.cpp)
#define NROPER_ANIM_BREATHE (3)
// uc_orig: NROPER_ANIM_RUNNING_JUMP (fallen/Source/Anim.cpp)
#define NROPER_ANIM_RUNNING_JUMP (4)
// uc_orig: NROPER_ANIM_RUNNING_JUMP_FLY (fallen/Source/Anim.cpp)
#define NROPER_ANIM_RUNNING_JUMP_FLY (5)
// uc_orig: NROPER_ANIM_RUNNING_JUMP_LAND (fallen/Source/Anim.cpp)
#define NROPER_ANIM_RUNNING_JUMP_LAND (6)
// uc_orig: NROPER_ANIM_RUNNING_JUMP_LAND_RUN (fallen/Source/Anim.cpp)
#define NROPER_ANIM_RUNNING_JUMP_LAND_RUN (7)
// uc_orig: NROPER_ANIM_DRAW_GUN (fallen/Source/Anim.cpp)
#define NROPER_ANIM_DRAW_GUN (8)
// uc_orig: NROPER_ANIM_GUN_AIM (fallen/Source/Anim.cpp)
#define NROPER_ANIM_GUN_AIM (9)
// uc_orig: NROPER_ANIM_GUN_SHOOT (fallen/Source/Anim.cpp)
#define NROPER_ANIM_GUN_SHOOT (10)
// uc_orig: NROPER_ANIM_LISTEN (fallen/Source/Anim.cpp)
#define NROPER_ANIM_LISTEN (11)
// uc_orig: NROPER_ANIM_JUMP_SPOT_TAKEOFF (fallen/Source/Anim.cpp)
#define NROPER_ANIM_JUMP_SPOT_TAKEOFF (12)
// uc_orig: NROPER_ANIM_JUMP_SPOT_LAND (fallen/Source/Anim.cpp)
#define NROPER_ANIM_JUMP_SPOT_LAND (13)
// uc_orig: NROPER_ANIM_JUMP_SPOT_STATIC (fallen/Source/Anim.cpp)
#define NROPER_ANIM_JUMP_SPOT_STATIC (14)
// uc_orig: NROPER_ANIM_TELL (fallen/Source/Anim.cpp)
#define NROPER_ANIM_TELL (15)
// uc_orig: NROPER_ANIM_SHOTGUN_TAKEOUT (fallen/Source/Anim.cpp)
#define NROPER_ANIM_SHOTGUN_TAKEOUT (16)
// uc_orig: NROPER_ANIM_SHOTGUN_SHOOT (fallen/Source/Anim.cpp)
#define NROPER_ANIM_SHOTGUN_SHOOT (17)
// uc_orig: NROPER_ANIM_SWIG_FLASK (fallen/Source/Anim.cpp)
#define NROPER_ANIM_SWIG_FLASK (19)
// uc_orig: NROPER_ANIM_YOMP_START (fallen/Source/Anim.cpp)
#define NROPER_ANIM_YOMP_START (20)
// uc_orig: NROPER_ANIM_AK_TAKEOUT (fallen/Source/Anim.cpp)
#define NROPER_ANIM_AK_TAKEOUT (21)
// uc_orig: NROPER_ANIM_AK_SHOOT (fallen/Source/Anim.cpp)
#define NROPER_ANIM_AK_SHOOT (22)
// uc_orig: NROPER_ANIM_AK_AIM (fallen/Source/Anim.cpp)
#define NROPER_ANIM_AK_AIM (23)
// uc_orig: NROPER_ANIM_AK_AIM_L (fallen/Source/Anim.cpp)
#define NROPER_ANIM_AK_AIM_L (24)
// uc_orig: NROPER_ANIM_AK_AIM_R (fallen/Source/Anim.cpp)
#define NROPER_ANIM_AK_AIM_R (25)
// uc_orig: NROPER_ANIM_SHOTGUN_AIM (fallen/Source/Anim.cpp)
#define NROPER_ANIM_SHOTGUN_AIM (26)
// uc_orig: NROPER_ANIM_SHOTGUN_AIM_L (fallen/Source/Anim.cpp)
#define NROPER_ANIM_SHOTGUN_AIM_L (27)
// uc_orig: NROPER_ANIM_SHOTGUN_AIM_R (fallen/Source/Anim.cpp)
#define NROPER_ANIM_SHOTGUN_AIM_R (28)
// uc_orig: NROPER_ANIM_GUN_AIM_L (fallen/Source/Anim.cpp)
#define NROPER_ANIM_GUN_AIM_L (30)
// uc_orig: NROPER_ANIM_GUN_AIM_R (fallen/Source/Anim.cpp)
#define NROPER_ANIM_GUN_AIM_R (31)
// uc_orig: NROPER_ANIM_WALK_BACKWARDS (fallen/Source/Anim.cpp)
#define NROPER_ANIM_WALK_BACKWARDS (46)
// uc_orig: NROPER_ANIM_RUN_WITH_AK (fallen/Source/Anim.cpp)
#define NROPER_ANIM_RUN_WITH_AK (47)
// uc_orig: NROPER_ANIM_WALK_BACKWARDS_WITH_AK (fallen/Source/Anim.cpp)
#define NROPER_ANIM_WALK_BACKWARDS_WITH_AK (48)

// Carry animations
// uc_orig: NROPER_PICKUP_CARRY (fallen/Source/Anim.cpp)
#define NROPER_PICKUP_CARRY (34)
// uc_orig: NROPER_START_WALK_CARRY (fallen/Source/Anim.cpp)
#define NROPER_START_WALK_CARRY (35)
// uc_orig: NROPER_WALK_CARRY (fallen/Source/Anim.cpp)
#define NROPER_WALK_CARRY (36)
// uc_orig: NROPER_WALK_STOP_CARRY (fallen/Source/Anim.cpp)
#define NROPER_WALK_STOP_CARRY (37)
// uc_orig: NROPER_PUTDOWN_CARRY (fallen/Source/Anim.cpp)
#define NROPER_PUTDOWN_CARRY (38)
// uc_orig: NROPER_PICKUP_CARRY_V (fallen/Source/Anim.cpp)
#define NROPER_PICKUP_CARRY_V (39)
// uc_orig: NROPER_START_WALK_CARRY_V (fallen/Source/Anim.cpp)
#define NROPER_START_WALK_CARRY_V (40)
// uc_orig: NROPER_WALK_CARRY_V (fallen/Source/Anim.cpp)
#define NROPER_WALK_CARRY_V (41)
// uc_orig: NROPER_WALK_STOP_CARRY_V (fallen/Source/Anim.cpp)
#define NROPER_WALK_STOP_CARRY_V (42)
// uc_orig: NROPER_PUTDOWN_CARRY_V (fallen/Source/Anim.cpp)
#define NROPER_PUTDOWN_CARRY_V (43)
// uc_orig: NROPER_STAND_CARRY (fallen/Source/Anim.cpp)
#define NROPER_STAND_CARRY (44)
// uc_orig: NROPER_STAND_CARRY_V (fallen/Source/Anim.cpp)
#define NROPER_STAND_CARRY_V (45)

// uc_orig: NROPER_ANIM_CLIMB_OVER_FENCE (fallen/Source/Anim.cpp)
#define NROPER_ANIM_CLIMB_OVER_FENCE (91)

// Dual-pistol animation set (PC-only; not in psxroper.all)
// uc_orig: NROPER_TWO_PISTOL_DRAW (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_DRAW (76)
// uc_orig: NROPER_TWO_PISTOL_FIRE (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_FIRE (77)
// uc_orig: NROPER_TWO_PISTOL_AWAY (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_AWAY (78)
// uc_orig: NROPER_TWO_PISTOL_RUN (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_RUN (72)
// uc_orig: NROPER_TWO_PISTOL_AIM (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_AIM (90)
// uc_orig: NROPER_TWO_PISTOL_AIM_L (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_AIM_L (79)
// uc_orig: NROPER_TWO_PISTOL_AIM_R (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_AIM_R (80)
// uc_orig: NROPER_TWO_PISTOL_CROUCH (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_CROUCH (87)
// uc_orig: NROPER_TWO_PISTOL_CROUCH_HOLD (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_CROUCH_HOLD (88)
// uc_orig: NROPER_TWO_PISTOL_CROUCH_STAND_UP (fallen/Source/Anim.cpp)
#define NROPER_TWO_PISTOL_CROUCH_STAND_UP (89)

// Shotgun crouch animations
// uc_orig: NROPER_SHOTGUN_CROUCH (fallen/Source/Anim.cpp)
#define NROPER_SHOTGUN_CROUCH (84)
// uc_orig: NROPER_SHOTGUN_CROUCH_HOLD (fallen/Source/Anim.cpp)
#define NROPER_SHOTGUN_CROUCH_HOLD (85)
// uc_orig: NROPER_SHOTGUN_CROUCH_STAND_UP (fallen/Source/Anim.cpp)
#define NROPER_SHOTGUN_CROUCH_STAND_UP (86)
// uc_orig: NROPER_ANIM_SHOTGUN_RUN (fallen/Source/Anim.cpp)
#define NROPER_ANIM_SHOTGUN_RUN (74)
// uc_orig: NROPER_ANIM_SHOTGUN_START_RUN (fallen/Source/Anim.cpp)
#define NROPER_ANIM_SHOTGUN_START_RUN (82)

// Forward declaration for convert_anim (defined in io.cpp / interact.cpp — not migrated yet)
// uc_orig: convert_anim (fallen/Source/Anim.cpp)
void convert_anim(Anim* key_list, GameKeyFrameChunk* p_chunk, KeyFrameChunk* the_chunk);

// Forward declarations for animation loading functions (defined in io.cpp — not migrated yet)
extern SLONG load_anim_system(struct GameKeyFrameChunk* game_chunk, CBYTE* name, SLONG peep);
extern SLONG append_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG start_anim, SLONG load_mesh);
extern UWORD darci_normal_count;
extern SLONG playing_combat_tutorial(void);
extern SLONG playing_level(const CBYTE* name);

// uc_orig: ANIM_init (fallen/Source/Anim.cpp)
void ANIM_init(void)
{
    SLONG c0;
    for (c0 = 0; c0 < MAX_GAME_CHUNKS; c0++) {
        game_chunk[c0].PeopleTypes = 0;
        game_chunk[c0].AnimKeyFrames = 0;
        game_chunk[c0].AnimList = 0;
        game_chunk[c0].TheElements = 0;
        game_chunk[c0].FightCols = 0;
    }
    for (c0 = 0; c0 < MAX_ANIM_CHUNKS; c0++) {
        anim_chunk[c0].PeopleTypes = 0;
        anim_chunk[c0].AnimKeyFrames = 0;
        anim_chunk[c0].AnimList = 0;
        anim_chunk[c0].TheElements = 0;
        anim_chunk[c0].FightCols = 0;
    }
}

// uc_orig: ANIM_fini (fallen/Source/Anim.cpp)
void ANIM_fini(void)
{
    free_game_chunk(&game_chunk[ANIM_TYPE_DARCI]);
    free_game_chunk(&game_chunk[ANIM_TYPE_ROPER]);
    free_game_chunk(&game_chunk[ANIM_TYPE_ROPER2]);
    free_game_chunk(&game_chunk[ANIM_TYPE_CIV]);
}

// uc_orig: init_anim_prims (fallen/Source/Anim.cpp)
void init_anim_prims(void)
{
    SLONG c0;
    for (c0 = 0; c0 < MAX_ANIM_CHUNKS; c0++) {
        anim_chunk[c0].MultiObject[0] = 0;
    }
}

// No-op placeholder; originally intended to clear test_chunk before editor reuse.
// uc_orig: setup_anim_stuff (fallen/Source/Anim.cpp)
static void setup_anim_stuff(void) {}

// Load .all animation files for all runtime character types.
// Load order: darci1.all, roper.all, rthug.all, roper2.all,
//   police1.all appended to Roper at index 200,
//   newciv.all at CIV_M_START, newcivf.all at CIV_F_START.
// Level-specific: trainer.all on tutorial, banesuit.all on estate2/Album1.
// uc_orig: setup_people_anims (fallen/Source/Anim.cpp)
void setup_people_anims(void)
{
    setup_anim_stuff();
    load_anim_system(&game_chunk[ANIM_TYPE_DARCI], "darci1.all", 1);

    load_anim_system(&game_chunk[ANIM_TYPE_ROPER], "roper.all", 0);

    load_anim_system(&game_chunk[ANIM_TYPE_CIV], "rthug.all", 2);

    load_anim_system(&game_chunk[ANIM_TYPE_ROPER2], "roper2.all", 0);

    append_anim_system(&game_chunk[ANIM_TYPE_ROPER], "police1.all", 200, 0);
    append_anim_system(&game_chunk[ANIM_TYPE_CIV], "newciv.all", CIV_M_START, 1);
    append_anim_system(&game_chunk[ANIM_TYPE_CIV], "newcivf.all", CIV_F_START, 1);

    if (playing_combat_tutorial()) {
        game_chunk[ANIM_TYPE_CIV].MultiObject[0] = next_prim_multi_object;
        game_chunk[ANIM_TYPE_CIV].MultiObject[1] = next_prim_multi_object;
        game_chunk[ANIM_TYPE_CIV].MultiObject[2] = next_prim_multi_object;
        append_anim_system(&game_chunk[ANIM_TYPE_ROPER], "trainer.all", 240, 1);
    }
    if (playing_level("semtex.ucm")) {
        semtex = 1;
    } else {
        semtex = 0;
    }

    if (playing_level("estate2.ucm") || playing_level("Album1.ucm")) {
        game_chunk[ANIM_TYPE_CIV].MultiObject[6] = next_prim_multi_object;
        append_anim_system(&game_chunk[ANIM_TYPE_ROPER], "banesuit.all", 240, 1);
        estate = 1;
    } else {
        estate = 0;
    }

    darci_normal_count = next_prim_point;
}

// Van animation loading — originally loads slot 5, but everything is commented out.
// The van system was experimental; slot 5 is unused at runtime.
// Sets next_game_chunk = 4 (Darci=0, Roper=1, CIV=2, Roper2=3).
// uc_orig: setup_extra_anims (fallen/Source/Anim.cpp)
void setup_extra_anims(void)
{
    next_game_chunk = 4;
}

// Build the global_anim_array[character_type][action] dispatch table.
// Step 1: fill all 4 rows with Darci's animations as baseline fallback.
// Step 2: apply CIV overrides (thug walk, civ idle breathe).
// Step 3: apply new Roper overrides (NROPER_* constants for PC build).
// Step 4: apply universal overrides for all character types (sit, strangle, carry, etc.).
// The large commented-out old-Roper section is dead code from a prior design iteration.
// uc_orig: setup_global_anim_array (fallen/Source/Anim.cpp)
void setup_global_anim_array(void)
{
    SLONG c0;
    for (c0 = 0; c0 < ANIM_END; c0++) {
        global_anim_array[0][c0] = game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        global_anim_array[1][c0] = game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        global_anim_array[2][c0] = game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        global_anim_array[3][c0] = game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
    }

    // CIV overrides
    global_anim_array[ANIM_TYPE_CIV][ANIM_WALK] = game_chunk[ANIM_TYPE_CIV].AnimList[THUG_ANIM_WALK];
    global_anim_array[ANIM_TYPE_CIV][ANIM_BREATHE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_IDLE_BREATHE];
    global_anim_array[ANIM_TYPE_CIV][ANIM_IDLE_SCRATCH1] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_IDLE_PHONE_ANSWER];

    // New Roper overrides (PC build)
    global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_UP_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_LOOP];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_ON_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_START];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_YOMP] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_YOMP];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_STAND_READY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_STAND_READY];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_BREATHE] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_BREATHE];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_RUN_JUMP_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_MID_AIR_TWEEN_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP_FLY];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_RIGHT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP_LAND_RUN];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_STAND] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP_LAND];

    global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_DRAW] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_DRAW];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_SHOOT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_FIRE];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_AIM_AHEAD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_AIM];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_GUN_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_AIM_L];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_GUN_RIGHT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_AIM_R];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_JOG] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_RUN];

    global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_DUCK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_CROUCH];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_DUCK_HOLD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_CROUCH_HOLD];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_STANDUP] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_CROUCH_STAND_UP];

    global_anim_array[ANIM_TYPE_ROPER][ANIM_STANDING_JUMP] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_JUMP_SPOT_TAKEOFF];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_VERT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_JUMP_SPOT_LAND];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_FALLING] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_JUMP_SPOT_STATIC];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_FALLING_QUEUED] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_JUMP_SPOT_STATIC];

    global_anim_array[ANIM_TYPE_ROPER][ANIM_TALK_TELL] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_TELL];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_TALK_LISTEN] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_LISTEN];

    global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_DRAW] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_TAKEOUT];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_AIM] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_AIM];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_FIRE] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_SHOOT];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_AHEAD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_AIM];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_AIM_L];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_RIGHT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_AIM_R];

    global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_DUCK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_SHOTGUN_CROUCH];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_DUCK_HOLD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_SHOTGUN_CROUCH_HOLD];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_STANDUP] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_SHOTGUN_CROUCH_STAND_UP];

    // AK47 overrides (reuse shotgun aim slots)
    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_AHEAD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_AK_AIM];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_AK_AIM_L];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_RIGHT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_AK_AIM_R];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_AK_FIRE] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_AK_SHOOT];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_AK_JOG] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_RUN];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_YOMP_START_AK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_START_RUN];

    global_anim_array[ANIM_TYPE_ROPER][ANIM_RUN_JUMP_LEFT_AK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_MID_AIR_TWEEN_LEFT_AK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP_FLY];

    global_anim_array[ANIM_TYPE_ROPER][ANIM_IDLE_SCRATCH1] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SWIG_FLASK];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_YOMP_START] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_YOMP_START];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_BACK_WALK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_WALK_BACKWARDS];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_BACK_WALK_AK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_WALK_BACKWARDS_WITH_AK];

    global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_UP_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_LOOP];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_ON_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_START];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_OVER_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_CLIMB_OVER_FENCE];

    // roper2 animations
    global_anim_array[ANIM_TYPE_ROPER][ANIM_CRAWL] = game_chunk[ANIM_TYPE_ROPER2].AnimList[144];
    global_anim_array[ANIM_TYPE_ROPER][ANIM_DANGLE] = game_chunk[ANIM_TYPE_ROPER2].AnimList[223];

    // Universal overrides applied to all 4 character types
    for (c0 = 1; c0 < 4; c0++) {
        global_anim_array[c0][ANIM_SIT_DOWN] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_SIT];
        global_anim_array[c0][ANIM_SIT_IDLE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_READ_PAPER1];
    }

    for (c0 = 0; c0 < 4; c0++) {
        global_anim_array[c0][ANIM_STRANGLE] = game_chunk[ANIM_TYPE_ROPER2].AnimList[268];
        global_anim_array[c0][ANIM_STRANGLE_VICTIM] = game_chunk[ANIM_TYPE_ROPER2].AnimList[269];

        global_anim_array[c0][ANIM_HEADBUTT] = game_chunk[ANIM_TYPE_ROPER2].AnimList[270];
        global_anim_array[c0][ANIM_HEADBUTT_VICTIM] = game_chunk[ANIM_TYPE_ROPER2].AnimList[271];

        global_anim_array[c0][ANIM_WANKER] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_WANKER];
        global_anim_array[c0][ANIM_ENTER_TAXI] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_ENTER_TAXI];

        global_anim_array[c0][ANIM_DANCE_BOOGIE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_DANCE_BOOGIE];
        global_anim_array[c0][ANIM_DANCE_WOOGIE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_DANCE_WOOGIE];
        global_anim_array[c0][ANIM_DANCE_HEADBANG] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_DANCE_HEADBANG];
        global_anim_array[c0][ANIM_WARM_HANDS] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_WARM_HANDS];

        global_anim_array[c0][ANIM_HANDS_UP] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_M_ANIM_HANDS_UP];
        global_anim_array[c0][ANIM_HANDS_UP_LOOP] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_M_ANIM_HANDS_UP_LOOP];
        global_anim_array[c0][ANIM_HANDS_UP_LIE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_M_ANIM_HANDS_UP_LIE];

        global_anim_array[c0][ANIM_PICKUP_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_PICKUP_CARRY];
        global_anim_array[c0][ANIM_START_WALK_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_START_WALK_CARRY];
        global_anim_array[c0][ANIM_WALK_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_WALK_CARRY];
        global_anim_array[c0][ANIM_WALK_STOP_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_WALK_STOP_CARRY];
        global_anim_array[c0][ANIM_PUTDOWN_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_PUTDOWN_CARRY];
        global_anim_array[c0][ANIM_STAND_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_STAND_CARRY];

        global_anim_array[c0][ANIM_PICKUP_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_PICKUP_CARRY_V];
        global_anim_array[c0][ANIM_START_WALK_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_START_WALK_CARRY_V];
        global_anim_array[c0][ANIM_WALK_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_WALK_CARRY_V];
        global_anim_array[c0][ANIM_WALK_STOP_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_WALK_STOP_CARRY_V];
        global_anim_array[c0][ANIM_PUTDOWN_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_PUTDOWN_CARRY_V];
        global_anim_array[c0][ANIM_STAND_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_STAND_CARRY_V];
    }
}

// Free all heap-allocated arrays inside a GameKeyFrameChunk and null its pointers.
// uc_orig: free_game_chunk (fallen/Source/Anim.cpp)
void free_game_chunk(GameKeyFrameChunk* the_chunk)
{
    if (the_chunk->PeopleTypes)
        MemFree((UBYTE*)the_chunk->PeopleTypes);

    if (the_chunk->AnimKeyFrames)
        MemFree((UBYTE*)the_chunk->AnimKeyFrames);
    if (the_chunk->AnimList)
        MemFree((UBYTE*)the_chunk->AnimList);
    if (the_chunk->TheElements)
        MemFree((UBYTE*)the_chunk->TheElements);
    if (the_chunk->FightCols)
        MemFree((UBYTE*)the_chunk->FightCols);

    the_chunk->PeopleTypes = 0;
    the_chunk->AnimKeyFrames = 0;
    the_chunk->AnimList = 0;
    the_chunk->TheElements = 0;
    the_chunk->FightCols = 0;
}
