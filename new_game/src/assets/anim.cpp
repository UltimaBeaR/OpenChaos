#include <platform.h>
#include <stdio.h>
#include <stdlib.h>
#include "missions/game_types.h"
#include "assets/anim.h"
#include "core/memory.h"
#include "actors/characters/anim_ids.h"
#include "actors/characters/person_types.h"  // ANIM_TYPE_DARCI/ROPER/CIV/ROPER2

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

// Compress a CMatrix33 into GameKeyFrameElementComp encoding.
// Stores only 2 of 3 elements per row; the 3rd (the largest) is reconstructed at runtime
// via sqrt(1 - a^2 - b^2). The Pad byte encodes which element was the largest and its sign.
// Row 0: bits 0-3 encode dominance; bit 0 = row0 sign, bits 2-3 = which omitted.
// Row 1: bits 4-5 encode dominance; bit 1 = row1 sign.
// uc_orig: SetCMatrixComp (fallen/Source/Anim.cpp)
void SetCMatrixComp(GameKeyFrameElementComp* e, CMatrix33* cm)
{
    e->Pad = 0;

    SBYTE a, b, c;

    a = ((cm->M[0] & CMAT0_MASK) >> 22);
    b = ((cm->M[0] & CMAT1_MASK) >> 12);
    c = ((cm->M[0] & CMAT2_MASK) >> 02);

    if (abs(a) > abs(b)) {
        if (abs(a) > abs(c)) {
            e->m00 = b;
            e->m01 = c;
            e->Pad |= 0;
            if (a < 0)
                e->Pad |= 1;
        } else {
            e->m00 = a;
            e->m01 = b;
            e->Pad |= 4;
            if (c < 0)
                e->Pad |= 1;
        }
    } else {
        if (abs(b) > abs(c)) {
            e->m00 = a;
            e->m01 = c;
            e->Pad |= 8;
            if (b < 0)
                e->Pad |= 1;
        } else {
            e->m00 = a;
            e->m01 = b;
            e->Pad |= 4;
            if (c < 0)
                e->Pad |= 1;
        }
    }

    a = ((cm->M[1] & CMAT0_MASK) >> 22);
    b = ((cm->M[1] & CMAT1_MASK) >> 12);
    c = ((cm->M[1] & CMAT2_MASK) >> 02);

    if (abs(a) > abs(b)) {
        if (abs(a) > abs(c)) {
            e->m10 = b;
            e->m11 = c;
            e->Pad |= 0;
            if (a < 0)
                e->Pad |= 2;
        } else {
            e->m10 = a;
            e->m11 = b;
            e->Pad |= 16;
            if (c < 0)
                e->Pad |= 2;
        }
    } else {
        if (abs(b) > abs(c)) {
            e->m10 = a;
            e->m11 = c;
            e->Pad |= 32;
            if (b < 0)
                e->Pad |= 2;
        } else {
            e->m10 = a;
            e->m11 = b;
            e->Pad |= 16;
            if (c < 0)
                e->Pad |= 2;
        }
    }
}

// uc_orig: init_anim_prims (fallen/Source/Anim.cpp)
void init_anim_prims(void)
{
    SLONG c0;
    for (c0 = 0; c0 < MAX_ANIM_CHUNKS; c0++) {
        anim_chunk[c0].MultiObject[0] = 0;
    }
}

// uc_orig: reset_anim_stuff (fallen/Source/Anim.cpp)
void reset_anim_stuff(void)
{
    if (test_chunk) {
        test_chunk->MultiObject = 0;
        test_chunk->ElementCount = 0;
    }
}

// No-op placeholder; originally intended to clear test_chunk before editor reuse.
// uc_orig: setup_anim_stuff (fallen/Source/Anim.cpp)
static void setup_anim_stuff(void)
{
    // intentionally empty (see original comment)
}

// uc_orig: set_next_prim_point (fallen/Source/Anim.cpp)
void set_next_prim_point(SLONG v)
{
    next_prim_point = v;
}

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
    Anim* key_list;
    key_list = NULL;

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

// uc_orig: calc_sub_objects_position_no_tween (fallen/Source/Anim.cpp)
void calc_sub_objects_position_no_tween(GameKeyFrame* cur_frame, UWORD object, SLONG* x, SLONG* y, SLONG* z)
{
    struct Matrix31 offset;
    struct GameKeyFrameElement* anim_info;

    anim_info = &cur_frame->FirstElement[object];

    *x = (anim_info->OffsetX) >> TWEEN_OFFSET_SHIFT;
    *y = (anim_info->OffsetY) >> TWEEN_OFFSET_SHIFT;
    *z = (anim_info->OffsetZ) >> TWEEN_OFFSET_SHIFT;
}

// Stub: originally intended to adjust prim multi-object bone matrices for animation.
// Body is empty — never fully implemented in the prerelease build.
// uc_orig: fix_multi_object_for_anim (fallen/Source/Anim.cpp)
void fix_multi_object_for_anim(UWORD obj, struct PrimMultiAnim* p_anim)
{
    SLONG sp, ep;
    UWORD c0, c1;
    struct PrimObject* p_obj;
    struct Matrix33* mat;
    struct Matrix31 temp;
}

// ---- Anim class methods ----

// uc_orig: Anim::Anim (fallen/Source/Anim.cpp)
Anim::Anim()
{
    CurrentFrame = NULL;
    FrameListStart = NULL;
    FrameListEnd = NULL;
    FrameCount = 0;
    LastAnim = NULL;
    NextAnim = NULL;
    AnimFlags = 0;
}

// uc_orig: Anim::~Anim (fallen/Source/Anim.cpp)
Anim::~Anim()
{
    while (FrameCount) {
        CurrentFrame = FrameListStart;
        FrameListStart = CurrentFrame->NextFrame;
        if (CurrentFrame)
            MemFree(CurrentFrame);
        FrameCount--;
    }
}

// uc_orig: Anim::StartLooping (fallen/Source/Anim.cpp)
void Anim::StartLooping(void)
{
    if (FrameListEnd) {
        FrameListEnd->NextFrame = FrameListStart;
    }
}

// uc_orig: Anim::StopLooping (fallen/Source/Anim.cpp)
void Anim::StopLooping(void)
{
    if (FrameListEnd) {
        FrameListEnd->NextFrame = 0;
    }
}

// uc_orig: Anim::SetAllTweens (fallen/Source/Anim.cpp)
void Anim::SetAllTweens(SLONG tween)
{
    KeyFrame* frame_ptr;

    frame_ptr = FrameListStart;

    while (frame_ptr) {
        frame_ptr->TweenStep = tween;
        if (frame_ptr == FrameListEnd)
            break;
        frame_ptr = frame_ptr->NextFrame;
    }
}

// uc_orig: Anim::AddKeyFrame (fallen/Source/Anim.cpp)
void Anim::AddKeyFrame(KeyFrame* the_frame)
{
    KeyFrame* frame_ptr;

    if (the_frame) {
        StopLooping();
        frame_ptr = (KeyFrame*)MemAlloc(sizeof(KeyFrame));
        *frame_ptr = *the_frame;
        frame_ptr->NextFrame = NULL;
        frame_ptr->PrevFrame = NULL;
        frame_ptr->TweenStep = the_frame->TweenStep;
        frame_ptr->Fight = the_frame->Fight;
        if (FrameListStart == NULL) {
            FrameListStart = frame_ptr;
            FrameListEnd = FrameListStart;
            FrameListEnd->Flags |= ANIM_FLAG_LAST_FRAME;
        } else if (CurrentFrame == NULL) {
            if (FrameListEnd) {
                FrameListEnd->NextFrame = frame_ptr;
                frame_ptr->PrevFrame = FrameListEnd;
            }
            FrameListEnd->Flags &= ~ANIM_FLAG_LAST_FRAME;
            FrameListEnd = frame_ptr;
            FrameListEnd->Flags |= ANIM_FLAG_LAST_FRAME;
        } else {
            frame_ptr->PrevFrame = CurrentFrame->PrevFrame;
            if (CurrentFrame->PrevFrame)
                CurrentFrame->PrevFrame->NextFrame = frame_ptr;
            else
                FrameListStart = frame_ptr;
            CurrentFrame->PrevFrame = frame_ptr;
            frame_ptr->NextFrame = CurrentFrame;
            CurrentFrame = 0;
        }
        FrameCount++;
        if (AnimFlags & ANIM_LOOP)
            StartLooping();
    }
}

// uc_orig: Anim::RemoveKeyFrame (fallen/Source/Anim.cpp)
void Anim::RemoveKeyFrame(KeyFrame* the_frame)
{
    if (the_frame) {
        StopLooping();
        if (the_frame->NextFrame) {
            the_frame->NextFrame->PrevFrame = the_frame->PrevFrame;
        } else {
            FrameListEnd = the_frame->PrevFrame;
            if (FrameListEnd)
                FrameListEnd->Flags = ANIM_FLAG_LAST_FRAME;
        }
        if (the_frame->PrevFrame) {
            the_frame->PrevFrame->NextFrame = the_frame->NextFrame;
        } else
            FrameListStart = the_frame->NextFrame;
        MemFree(the_frame);
        FrameCount--;
        if (AnimFlags & ANIM_LOOP)
            StartLooping();
    }
}

// ---- Character class methods ----

// uc_orig: Character::AddAnim (fallen/Source/Anim.cpp)
void Character::AddAnim(Anim* the_anim)
{
}

// uc_orig: Character::RemoveAnim (fallen/Source/Anim.cpp)
void Character::RemoveAnim(Anim* the_anim)
{
}

// Convert raw KeyFrameElement data from source chunk into compressed GameKeyFrameElement format.
// Skips elements with p_reloc[i] == 0xffff (duplicates already mapped elsewhere).
// uc_orig: convert_elements (fallen/Source/Anim.cpp)
void convert_elements(KeyFrameChunk* the_chunk, GameKeyFrameChunk* game_chunk, UWORD* p_reloc, SLONG max_ele)
{
    struct KeyFrameElement* first_element;
    struct KeyFrameElement* last_element;
    struct KeyFrameElement* element;
    SLONG count = 0;
    UWORD pos = 0;

    first_element = the_chunk->FirstElement;
    last_element = the_chunk->LastElement;

    for (element = first_element; element < last_element; element++) {

        if (p_reloc[count] != 0xffff) {
            pos = p_reloc[count];

            SetCMatrix(&game_chunk->TheElements[pos], &element->CMatrix);
            game_chunk->TheElements[pos].OffsetX = (SWORD)element->OffsetX;
            game_chunk->TheElements[pos].OffsetY = (SWORD)element->OffsetY;
            game_chunk->TheElements[pos].OffsetZ = (SWORD)element->OffsetZ;
        }

        count++;
    }
    game_chunk->MaxElements = max_ele;
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
