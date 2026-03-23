#ifndef AI_PCOM_H
#define AI_PCOM_H

#include <platform.h>
#include "missions/game_types.h"

// ============================================================
// PCOM — Person COMportment — NPC AI system
//
// Every NPC calls PCOM_process_person() each game tick.
// Two parallel state machines:
//   1. pcom_ai / pcom_ai_state  — high-level "what am I doing"
//   2. pcom_move_state / pcom_move_substate — low-level movement
// ============================================================

// --- High-level AI types (pcom_ai field) ---

// uc_orig: PCOM_AI_NONE (fallen/Headers/pcom.h)
#define PCOM_AI_NONE 0
// uc_orig: PCOM_AI_CIV (fallen/Headers/pcom.h)
#define PCOM_AI_CIV 1
// uc_orig: PCOM_AI_GUARD (fallen/Headers/pcom.h)
#define PCOM_AI_GUARD 2
// uc_orig: PCOM_AI_ASSASIN (fallen/Headers/pcom.h)
#define PCOM_AI_ASSASIN 3
// uc_orig: PCOM_AI_BOSS (fallen/Headers/pcom.h)
#define PCOM_AI_BOSS 4
// uc_orig: PCOM_AI_COP (fallen/Headers/pcom.h)
#define PCOM_AI_COP 5
// uc_orig: PCOM_AI_GANG (fallen/Headers/pcom.h)
#define PCOM_AI_GANG 6
// uc_orig: PCOM_AI_DOORMAN (fallen/Headers/pcom.h)
#define PCOM_AI_DOORMAN 7
// uc_orig: PCOM_AI_BODYGUARD (fallen/Headers/pcom.h)
#define PCOM_AI_BODYGUARD 8
// uc_orig: PCOM_AI_DRIVER (fallen/Headers/pcom.h)
#define PCOM_AI_DRIVER 9
// uc_orig: PCOM_AI_BDISPOSER (fallen/Headers/pcom.h)
#define PCOM_AI_BDISPOSER 10
// uc_orig: PCOM_AI_BIKER (fallen/Headers/pcom.h)
#define PCOM_AI_BIKER 11
// uc_orig: PCOM_AI_FIGHT_TEST (fallen/Headers/pcom.h)
#define PCOM_AI_FIGHT_TEST 12
// uc_orig: PCOM_AI_BULLY (fallen/Headers/pcom.h)
#define PCOM_AI_BULLY 13
// uc_orig: PCOM_AI_COP_DRIVER (fallen/Headers/pcom.h)
#define PCOM_AI_COP_DRIVER 14
// uc_orig: PCOM_AI_SUICIDE (fallen/Headers/pcom.h)
#define PCOM_AI_SUICIDE 15
// uc_orig: PCOM_AI_FLEE_PLAYER (fallen/Headers/pcom.h)
#define PCOM_AI_FLEE_PLAYER 16
// uc_orig: PCOM_AI_KILL_COLOUR (fallen/Headers/pcom.h)
#define PCOM_AI_KILL_COLOUR 17
// uc_orig: PCOM_AI_MIB (fallen/Headers/pcom.h)
#define PCOM_AI_MIB 18
// uc_orig: PCOM_AI_BANE (fallen/Headers/pcom.h)
#define PCOM_AI_BANE 19
// uc_orig: PCOM_AI_HYPOCHONDRIA (fallen/Headers/pcom.h)
#define PCOM_AI_HYPOCHONDRIA 20
// uc_orig: PCOM_AI_SHOOT_DEAD (fallen/Headers/pcom.h)
#define PCOM_AI_SHOOT_DEAD 21
// uc_orig: PCOM_AI_NUMBER (fallen/Headers/pcom.h)
#define PCOM_AI_NUMBER 22

// --- Movement mode (pcom_move field, base patrol/wander style) ---

// uc_orig: PCOM_MOVE_STILL (fallen/Headers/pcom.h)
#define PCOM_MOVE_STILL 1
// uc_orig: PCOM_MOVE_PATROL (fallen/Headers/pcom.h)
#define PCOM_MOVE_PATROL 2
// uc_orig: PCOM_MOVE_PATROL_RAND (fallen/Headers/pcom.h)
#define PCOM_MOVE_PATROL_RAND 3
// uc_orig: PCOM_MOVE_WANDER (fallen/Headers/pcom.h)
#define PCOM_MOVE_WANDER 4
// uc_orig: PCOM_MOVE_FOLLOW (fallen/Headers/pcom.h)
#define PCOM_MOVE_FOLLOW 5
// uc_orig: PCOM_MOVE_WARM_HANDS (fallen/Headers/pcom.h)
#define PCOM_MOVE_WARM_HANDS 6
// uc_orig: PCOM_MOVE_FOLLOW_ON_SEE (fallen/Headers/pcom.h)
#define PCOM_MOVE_FOLLOW_ON_SEE 7
// uc_orig: PCOM_MOVE_DANCE (fallen/Headers/pcom.h)
#define PCOM_MOVE_DANCE 8
// uc_orig: PCOM_MOVE_HANDS_UP (fallen/Headers/pcom.h)
#define PCOM_MOVE_HANDS_UP 9
// uc_orig: PCOM_MOVE_TIED_UP (fallen/Headers/pcom.h)
#define PCOM_MOVE_TIED_UP 10
// uc_orig: PCOM_MOVE_NUMBER (fallen/Headers/pcom.h)
#define PCOM_MOVE_NUMBER 11

// --- Personality flags (pcom_bent bitmask) ---

// uc_orig: PCOM_BENT_LAZY (fallen/Headers/pcom.h)
#define PCOM_BENT_LAZY (1 << 0)
// uc_orig: PCOM_BENT_DILIGENT (fallen/Headers/pcom.h)
#define PCOM_BENT_DILIGENT (1 << 1)
// uc_orig: PCOM_BENT_GANG (fallen/Headers/pcom.h)
#define PCOM_BENT_GANG (1 << 2)
// uc_orig: PCOM_BENT_FIGHT_BACK (fallen/Headers/pcom.h)
#define PCOM_BENT_FIGHT_BACK (1 << 3)
// uc_orig: PCOM_BENT_ONLY_KILL_PLAYER (fallen/Headers/pcom.h)
#define PCOM_BENT_ONLY_KILL_PLAYER (1 << 4)
// uc_orig: PCOM_BENT_ROBOT (fallen/Headers/pcom.h)
#define PCOM_BENT_ROBOT (1 << 5)
// uc_orig: PCOM_BENT_RESTRICTED (fallen/Headers/pcom.h)
#define PCOM_BENT_RESTRICTED (1 << 6)
// uc_orig: PCOM_BENT_PLAYERKILL (fallen/Headers/pcom.h)
#define PCOM_BENT_PLAYERKILL (1 << 7)
// uc_orig: PCOM_BENT_NUMBER (fallen/Headers/pcom.h)
#define PCOM_BENT_NUMBER 8

// --- High-level AI states (pcom_ai_state field) ---

// uc_orig: PCOM_AI_STATE_PLAYER (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_PLAYER 0
// uc_orig: PCOM_AI_STATE_NORMAL (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_NORMAL 1
// uc_orig: PCOM_AI_STATE_INVESTIGATING (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_INVESTIGATING 2
// uc_orig: PCOM_AI_STATE_SEARCHING (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_SEARCHING 3
// uc_orig: PCOM_AI_STATE_KILLING (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_KILLING 4
// uc_orig: PCOM_AI_STATE_SLEEPING (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_SLEEPING 5
// uc_orig: PCOM_AI_STATE_FLEE_PLACE (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_FLEE_PLACE 6
// uc_orig: PCOM_AI_STATE_FLEE_PERSON (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_FLEE_PERSON 7
// uc_orig: PCOM_AI_STATE_FOLLOWING (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_FOLLOWING 8
// uc_orig: PCOM_AI_STATE_NAVTOKILL (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_NAVTOKILL 9
// uc_orig: PCOM_AI_STATE_HOMESICK (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_HOMESICK 10
// uc_orig: PCOM_AI_STATE_LOOKAROUND (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_LOOKAROUND 11
// uc_orig: PCOM_AI_STATE_FINDCAR (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_FINDCAR 12
// uc_orig: PCOM_AI_STATE_BDEACTIVATE (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_BDEACTIVATE 13
// uc_orig: PCOM_AI_STATE_LEAVECAR (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_LEAVECAR 14
// uc_orig: PCOM_AI_STATE_SNIPE (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_SNIPE 15
// uc_orig: PCOM_AI_STATE_WARM_HANDS (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_WARM_HANDS 16
// uc_orig: PCOM_AI_STATE_FINDBIKE (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_FINDBIKE 17
// uc_orig: PCOM_AI_STATE_KNOCKEDOUT (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_KNOCKEDOUT 18
// uc_orig: PCOM_AI_STATE_TAUNT (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_TAUNT 19
// uc_orig: PCOM_AI_STATE_ARREST (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_ARREST 20
// uc_orig: PCOM_AI_STATE_TALK (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_TALK 21
// uc_orig: PCOM_AI_STATE_GRAPPLED (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_GRAPPLED 22
// uc_orig: PCOM_AI_STATE_HITCH (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_HITCH 23
// uc_orig: PCOM_AI_STATE_AIMLESS (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_AIMLESS 24
// uc_orig: PCOM_AI_STATE_HANDS_UP (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_HANDS_UP 25
// uc_orig: PCOM_AI_STATE_SUMMON (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_SUMMON 26
// uc_orig: PCOM_AI_STATE_GETITEM (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_GETITEM 27
// uc_orig: PCOM_AI_STATE_NUMBER (fallen/Headers/pcom.h)
#define PCOM_AI_STATE_NUMBER 28

// --- AI sub-states ---

// uc_orig: PCOM_AI_SUBSTATE_NONE (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_NONE 0
// uc_orig: PCOM_AI_SUBSTATE_SUPRISED (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_SUPRISED 1
// uc_orig: PCOM_AI_SUBSTATE_WALKOVER (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_WALKOVER 2
// uc_orig: PCOM_AI_SUBSTATE_LOOK (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_LOOK 3
// uc_orig: PCOM_AI_SUBSTATE_PUNCHING (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_PUNCHING 4
// uc_orig: PCOM_AI_SUBSTATE_KICKING (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_KICKING 5
// uc_orig: PCOM_AI_SUBSTATE_LEGIT (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_LEGIT 6
// uc_orig: PCOM_AI_SUBSTATE_HUNTING (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_HUNTING 7
// uc_orig: PCOM_AI_SUBSTATE_AIMING (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_AIMING 8
// uc_orig: PCOM_AI_SUBSTATE_NOMOREAMMO (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_NOMOREAMMO 9
// uc_orig: PCOM_AI_SUBSTATE_GOTOCAR (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_GOTOCAR 10
// uc_orig: PCOM_AI_SUBSTATE_GETINCAR (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_GETINCAR 11
// uc_orig: PCOM_AI_SUBSTATE_GOTOBOMB (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_GOTOBOMB 12
// uc_orig: PCOM_AI_SUBSTATE_CUTWIRES (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_CUTWIRES 13
// uc_orig: PCOM_AI_SUBSTATE_PARKCAR (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_PARKCAR 14
// uc_orig: PCOM_AI_SUBSTATE_LEAVECAR (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_LEAVECAR 15
// uc_orig: PCOM_AI_SUBSTATE_GOTOFIRE (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_GOTOFIRE 16
// uc_orig: PCOM_AI_SUBSTATE_WARMUP (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_WARMUP 17
// uc_orig: PCOM_AI_SUBSTATE_GOTOBIKE (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_GOTOBIKE 18
// uc_orig: PCOM_AI_SUBSTATE_HUNTING_SLIDE (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_HUNTING_SLIDE 19
// uc_orig: PCOM_AI_SUBSTATE_TALK_ASK (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_TALK_ASK 21
// uc_orig: PCOM_AI_SUBSTATE_TALK_TELL (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_TALK_TELL 22
// uc_orig: PCOM_AI_SUBSTATE_TALK_LISTEN (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_TALK_LISTEN 23
// uc_orig: PCOM_AI_SUBSTATE_HITCHING (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_HITCHING 24
// uc_orig: PCOM_AI_SUBSTATE_SUMMON_START (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_SUMMON_START 25
// uc_orig: PCOM_AI_SUBSTATE_SUMMON_FLOAT (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_SUMMON_FLOAT 27
// uc_orig: PCOM_AI_SUBSTATE_DRAW_H2H (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_DRAW_H2H 28
// uc_orig: PCOM_AI_SUBSTATE_CANTFIND (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_CANTFIND 28
// uc_orig: PCOM_AI_SUBSTATE_WAITING (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_WAITING 29
// uc_orig: PCOM_AI_SUBSTATE_NUMBER (fallen/Headers/pcom.h)
#define PCOM_AI_SUBSTATE_NUMBER 30

// --- Inventory flags (pcom_has bitmask) ---

// uc_orig: PCOM_HAS_GUN (fallen/Headers/pcom.h)
#define PCOM_HAS_GUN (1 << 0)
// uc_orig: PCOM_HAS_BALLOON (fallen/Headers/pcom.h)
#define PCOM_HAS_BALLOON (1 << 1)
// uc_orig: PCOM_HAS_SHOTGUN (fallen/Headers/pcom.h)
#define PCOM_HAS_SHOTGUN (1 << 2)
// uc_orig: PCOM_HAS_BASEBALLBAT (fallen/Headers/pcom.h)
#define PCOM_HAS_BASEBALLBAT (1 << 3)
// uc_orig: PCOM_HAS_KNIFE (fallen/Headers/pcom.h)
#define PCOM_HAS_KNIFE (1 << 4)
// uc_orig: PCOM_HAS_AK47 (fallen/Headers/pcom.h)
#define PCOM_HAS_AK47 (1 << 5)
// uc_orig: PCOM_HAS_GRENADE (fallen/Headers/pcom.h)
#define PCOM_HAS_GRENADE (1 << 6)

// --- Look-at target types ---

// uc_orig: PCOM_LOOKAT_NOTHING (fallen/Headers/pcom.h)
#define PCOM_LOOKAT_NOTHING 0
// uc_orig: PCOM_LOOKAT_THING (fallen/Headers/pcom.h)
#define PCOM_LOOKAT_THING 1
// uc_orig: PCOM_LOOKAT_WAYPOINT (fallen/Headers/pcom.h)
#define PCOM_LOOKAT_WAYPOINT 2
// uc_orig: PCOM_LOOKAT_DIRT (fallen/Headers/pcom.h)
#define PCOM_LOOKAT_DIRT 3

// --- Combat kill flags (fight-test dummy) ---

// uc_orig: PCOM_COMBAT_SLIDE (fallen/Headers/pcom.h)
#define PCOM_COMBAT_SLIDE (1 << 0)
// uc_orig: PCOM_COMBAT_COMBO_PPP (fallen/Headers/pcom.h)
#define PCOM_COMBAT_COMBO_PPP (1 << 1)
// uc_orig: PCOM_COMBAT_COMBO_KKK (fallen/Headers/pcom.h)
#define PCOM_COMBAT_COMBO_KKK (1 << 2)
// uc_orig: PCOM_COMBAT_COMBO_ANY (fallen/Headers/pcom.h)
#define PCOM_COMBAT_COMBO_ANY (1 << 3)
// uc_orig: PCOM_COMBAT_GRAPPLE_ATTACK (fallen/Headers/pcom.h)
#define PCOM_COMBAT_GRAPPLE_ATTACK (1 << 4)
// uc_orig: PCOM_COMBAT_SIDE_KICK (fallen/Headers/pcom.h)
#define PCOM_COMBAT_SIDE_KICK (1 << 5)
// uc_orig: PCOM_COMBAT_BACK_KICK (fallen/Headers/pcom.h)
#define PCOM_COMBAT_BACK_KICK (1 << 6)

// --- Sound types for PCOM_oscillate_tympanum ---

// uc_orig: PCOM_SOUND_FOOTSTEP (fallen/Headers/pcom.h)
#define PCOM_SOUND_FOOTSTEP 1
// uc_orig: PCOM_SOUND_UNUSUAL (fallen/Headers/pcom.h)
#define PCOM_SOUND_UNUSUAL 2
// uc_orig: PCOM_SOUND_HEY (fallen/Headers/pcom.h)
#define PCOM_SOUND_HEY 3
// uc_orig: PCOM_SOUND_ALARM (fallen/Headers/pcom.h)
#define PCOM_SOUND_ALARM 4
// uc_orig: PCOM_SOUND_FIGHT (fallen/Headers/pcom.h)
#define PCOM_SOUND_FIGHT 5
// uc_orig: PCOM_SOUND_GUNSHOT (fallen/Headers/pcom.h)
#define PCOM_SOUND_GUNSHOT 6
// uc_orig: PCOM_SOUND_DROP (fallen/Headers/pcom.h)
#define PCOM_SOUND_DROP 7
// uc_orig: PCOM_SOUND_DROP_MED (fallen/Headers/pcom.h)
#define PCOM_SOUND_DROP_MED 8
// uc_orig: PCOM_SOUND_DROP_BIG (fallen/Headers/pcom.h)
#define PCOM_SOUND_DROP_BIG 9
// uc_orig: PCOM_SOUND_VAN (fallen/Headers/pcom.h)
#define PCOM_SOUND_VAN 10
// uc_orig: PCOM_SOUND_BANG (fallen/Headers/pcom.h)
#define PCOM_SOUND_BANG 11
// uc_orig: PCOM_SOUND_MINE (fallen/Headers/pcom.h)
#define PCOM_SOUND_MINE 12
// uc_orig: PCOM_SOUND_LOOKINGATME (fallen/Headers/pcom.h)
#define PCOM_SOUND_LOOKINGATME 13
// uc_orig: PCOM_SOUND_WANKER (fallen/Headers/pcom.h)
#define PCOM_SOUND_WANKER 14
// uc_orig: PCOM_SOUND_DRAW_GUN (fallen/Headers/pcom.h)
#define PCOM_SOUND_DRAW_GUN 15
// uc_orig: PCOM_SOUND_GRENADE_FLY (fallen/Headers/pcom.h)
#define PCOM_SOUND_GRENADE_FLY 16
// uc_orig: PCOM_SOUND_GRENADE_HIT (fallen/Headers/pcom.h)
#define PCOM_SOUND_GRENADE_HIT 17

// --- Gang / find buffer sizes ---

// uc_orig: PCOM_MAX_GANG_PEOPLE (fallen/Source/pcom.cpp)
#define PCOM_MAX_GANG_PEOPLE 64
// uc_orig: PCOM_MAX_GANGS (fallen/Source/pcom.cpp)
#define PCOM_MAX_GANGS 16
// uc_orig: PCOM_MAX_FIND (fallen/Source/pcom.cpp)
#define PCOM_MAX_FIND 16

// --- Public API ---

// uc_orig: PCOM_init (fallen/Headers/pcom.h)
void PCOM_init(void);

// uc_orig: PCOM_create_person (fallen/Headers/pcom.h)
THING_INDEX PCOM_create_person(
    SLONG type,
    SLONG colour,
    SLONG group,
    SLONG ai,
    SLONG ai_other,
    SLONG ai_skill,
    SLONG move,
    SLONG move_follow,
    SLONG bent,
    SLONG has,
    SLONG drop,
    SLONG zone,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG yaw,
    SLONG random,
    ULONG flag1 = 0,
    ULONG flag2 = 0);

// uc_orig: PCOM_create_player (fallen/Headers/pcom.h)
THING_INDEX PCOM_create_player(
    SLONG type,
    SLONG has,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG id,
    SLONG yaw);

// uc_orig: PCOM_change_person_attributes (fallen/Headers/pcom.h)
void PCOM_change_person_attributes(
    Thing* p_person,
    SLONG colour,
    SLONG group,
    SLONG ai,
    SLONG ai_other,
    SLONG move,
    SLONG move_follow,
    SLONG bent,
    SLONG yaw);

// uc_orig: PCOM_oscillate_tympanum (fallen/Headers/pcom.h)
void PCOM_oscillate_tympanum(
    SLONG type,
    Thing* p_person,
    SLONG sound_x,
    SLONG sound_y,
    SLONG sound_z,
    UBYTE store_it = 1);

// uc_orig: PCOM_knockdown_happened (fallen/Headers/pcom.h)
void PCOM_knockdown_happened(Thing* p_person);

// uc_orig: PCOM_attack_happened (fallen/Headers/pcom.h)
void PCOM_attack_happened(Thing* p_victim, Thing* p_attacker);

// uc_orig: PCOM_attack_happened_but_missed (fallen/Headers/pcom.h)
void PCOM_attack_happened_but_missed(Thing* p_victim, Thing* p_attacker);

// uc_orig: PCOM_youre_being_grappled (fallen/Headers/pcom.h)
void PCOM_youre_being_grappled(Thing* p_victim, Thing* p_attacker);

// uc_orig: PCOM_process_person (fallen/Headers/pcom.h)
void PCOM_process_person(Thing* p_person);

// uc_orig: PCOM_jumping_navigating_person_continue_moving (fallen/Headers/pcom.h)
SLONG PCOM_jumping_navigating_person_continue_moving(Thing* p_person);

// uc_orig: PCOM_person_state_debug (fallen/Headers/pcom.h)
CBYTE* PCOM_person_state_debug(Thing* p_person);

// uc_orig: PCOM_make_people_talk_to_eachother (fallen/Headers/pcom.h)
void PCOM_make_people_talk_to_eachother(
    Thing* p_person_a,
    Thing* p_person_b,
    UBYTE is_a_asking_a_question,
    UBYTE stay_looking_at_eachother,
    UBYTE make_the_person_talked_at_listen = UC_TRUE);

// uc_orig: PCOM_stop_people_talking_to_eachother (fallen/Headers/pcom.h)
void PCOM_stop_people_talking_to_eachother(Thing* p_person_a, Thing* p_person_b);

// uc_orig: PCOM_person_doing_nothing_important (fallen/Headers/pcom.h)
SLONG PCOM_person_doing_nothing_important(Thing* p_person);

// uc_orig: PCOM_person_a_hates_b (fallen/Headers/pcom.h)
SLONG PCOM_person_a_hates_b(Thing* p_person_a, Thing* p_person_b);

// uc_orig: PCOM_person_wants_to_kill (fallen/Headers/pcom.h)
THING_INDEX PCOM_person_wants_to_kill(Thing* p_person);

// uc_orig: PCOM_cop_aiming_at_you (fallen/Headers/pcom.h)
SLONG PCOM_cop_aiming_at_you(Thing* p_person, Thing* p_cop);

// uc_orig: PCOM_make_driver_run_away (fallen/Headers/pcom.h)
void PCOM_make_driver_run_away(Thing* p_driver, Thing* p_scary);

// uc_orig: PCOM_set_person_ai_talk_to (fallen/Headers/pcom.h)
void PCOM_set_person_ai_talk_to(
    Thing* p_person,
    Thing* p_person_talked_at,
    UBYTE talk_substate,
    UBYTE stay_looking_at_eachother);

// uc_orig: PCOM_if_i_wanted_to_jump_how_fast_should_i_do_it (fallen/Headers/pcom.h)
SLONG PCOM_if_i_wanted_to_jump_how_fast_should_i_do_it(Thing* p_person);

// uc_orig: PCOM_call_cop_to_arrest_me (fallen/Headers/pcom.h)
SLONG PCOM_call_cop_to_arrest_me(Thing* p_person, SLONG store_it);

// Additional functions used internally and referenced from other modules.

// uc_orig: PCOM_add_gang_member (fallen/Source/pcom.cpp)
void PCOM_add_gang_member(THING_INDEX person, UBYTE group);

// uc_orig: PCOM_set_person_ai_normal (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_normal(Thing* p_person);

// uc_orig: PCOM_person_has_any_sort_of_gun (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_any_sort_of_gun(Thing* p_person);

// uc_orig: PCOM_person_has_any_sort_of_h2h (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_any_sort_of_h2h(Thing* p_person);

// uc_orig: PCOM_person_has_any_sort_of_gun_with_ammo (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_any_sort_of_gun_with_ammo(Thing* p_person);

// uc_orig: PCOM_person_has_gun_in_hand (fallen/Source/pcom.cpp)
SLONG PCOM_person_has_gun_in_hand(Thing* p_person);

// uc_orig: PCOM_are_there_people_who_want_to_enter (fallen/Source/pcom.cpp)
SLONG PCOM_are_there_people_who_want_to_enter(Thing* p_vehicle);

// uc_orig: PCOM_set_person_move_still (fallen/Source/pcom.cpp)
void PCOM_set_person_move_still(Thing* p_person);

// uc_orig: PCOM_set_person_move_mav_to_xz (fallen/Source/pcom.cpp)
void PCOM_set_person_move_mav_to_xz(Thing* p_person, SLONG dest_x, SLONG dest_z, SLONG speed);

// uc_orig: PCOM_set_person_move_mav_to_thing (fallen/Source/pcom.cpp)
void PCOM_set_person_move_mav_to_thing(Thing* p_person, Thing* p_target, SLONG speed);

// uc_orig: PCOM_set_person_move_mav_to_waypoint (fallen/Source/pcom.cpp)
void PCOM_set_person_move_mav_to_waypoint(Thing* p_person, SLONG waypoint, SLONG speed);

// uc_orig: PCOM_renav (fallen/Source/pcom.cpp)
void PCOM_renav(Thing* p_person);

// uc_orig: PCOM_finished_nav (fallen/Source/pcom.cpp)
SLONG PCOM_finished_nav(Thing* p_person);

// uc_orig: PCOM_set_person_move_pause (fallen/Source/pcom.cpp)
void PCOM_set_person_move_pause(Thing* p_person);

// uc_orig: PCOM_set_person_move_animation (fallen/Source/pcom.cpp)
void PCOM_set_person_move_animation(Thing* p_person, SLONG anim);

// uc_orig: PCOM_set_person_move_punch (fallen/Source/pcom.cpp)
void PCOM_set_person_move_punch(Thing* p_person);

// uc_orig: PCOM_set_person_move_kick (fallen/Source/pcom.cpp)
void PCOM_set_person_move_kick(Thing* p_person);

// uc_orig: PCOM_set_person_move_pickup_special (fallen/Source/pcom.cpp)
void PCOM_set_person_move_pickup_special(Thing* p_person, Thing* p_special);

// uc_orig: PCOM_set_person_move_arrest (fallen/Source/pcom.cpp)
void PCOM_set_person_move_arrest(Thing* p_person);

// uc_orig: PCOM_set_person_move_draw_gun (fallen/Source/pcom.cpp)
void PCOM_set_person_move_draw_gun(Thing* p_person);

// uc_orig: PCOM_set_person_move_draw_h2h (fallen/Source/pcom.cpp)
void PCOM_set_person_move_draw_h2h(Thing* p_person, SLONG special);

// uc_orig: PCOM_set_person_move_gun_away (fallen/Source/pcom.cpp)
void PCOM_set_person_move_gun_away(Thing* p_person);

// uc_orig: PCOM_set_person_move_shoot (fallen/Source/pcom.cpp)
void PCOM_set_person_move_shoot(Thing* p_person);

// uc_orig: PCOM_set_person_substate_goto_or_3pturn (fallen/Source/pcom.cpp)
void PCOM_set_person_substate_goto_or_3pturn(Thing* p_person);

// uc_orig: PCOM_set_person_substate_goto (fallen/Source/pcom.cpp)
void PCOM_set_person_substate_goto(Thing* p_person);

// uc_orig: PCOM_set_person_move_driveto (fallen/Source/pcom.cpp)
void PCOM_set_person_move_driveto(Thing* p_person, SLONG waypoint);

// uc_orig: PCOM_set_person_move_park_car (fallen/Source/pcom.cpp)
void PCOM_set_person_move_park_car(Thing* p_person, SLONG waypoint);

// uc_orig: PCOM_set_person_move_drive_down (fallen/Source/pcom.cpp)
void PCOM_set_person_move_drive_down(Thing* p_person, SLONG n1, SLONG n2);

// uc_orig: PCOM_set_person_move_park_car_on_road (fallen/Source/pcom.cpp)
void PCOM_set_person_move_park_car_on_road(Thing* p_person);

// uc_orig: PCOM_set_person_move_goto_thing_slide (fallen/Source/pcom.cpp)
void PCOM_set_person_move_goto_thing_slide(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_set_person_move_circle (fallen/Source/pcom.cpp)
void PCOM_set_person_move_circle(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_set_person_move_getincar (fallen/Source/pcom.cpp)
void PCOM_set_person_move_getincar(Thing* p_person, Thing* p_vehicle, SLONG am_i_a_passenger, SLONG door);

// uc_orig: PCOM_set_person_move_leavecar (fallen/Source/pcom.cpp)
void PCOM_set_person_move_leavecar(Thing* p_person);

// uc_orig: PCOM_set_person_ai_homesick (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_homesick(Thing* p_person);

// uc_orig: PCOM_set_person_ai_flee_person (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_flee_person(Thing* p_person, Thing* p_scary);

// uc_orig: PCOM_set_person_ai_flee_place (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_flee_place(Thing* p_person, SLONG x, SLONG z);

// uc_orig: PCOM_set_person_ai_kill_person (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_kill_person(Thing* p_person, Thing* p_target, SLONG alert_gang = UC_TRUE);

// uc_orig: PCOM_set_person_ai_aimless (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_aimless(Thing* p_person);

// uc_orig: PCOM_set_person_ai_navtokill_shoot (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_navtokill_shoot(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_set_person_ai_navtokill (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_navtokill(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_set_person_ai_follow (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_follow(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_set_person_ai_findcar (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_findcar(Thing* p_person, UWORD car);

// uc_orig: PCOM_set_person_ai_bdeactivate (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_bdeactivate(Thing* p_person, Thing* p_bomb);

// uc_orig: PCOM_set_person_ai_snipe (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_snipe(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_set_person_ai_warm_hands (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_warm_hands(Thing* p_person);

// uc_orig: PCOM_set_person_ai_hands_up (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_hands_up(Thing* p_person, Thing* p_cop);

// uc_orig: PCOM_set_person_ai_hitch (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_hitch(Thing* p_person, Thing* p_vehicle);

// uc_orig: PCOM_set_person_ai_taunt (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_taunt(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_set_person_ai_summon (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_summon(Thing* p_person);

// uc_orig: PCOM_set_person_ai_getitem (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_getitem(Thing* p_person, Thing* p_special, SLONG move_speed, SLONG excar_state, SLONG excar_arg);

// uc_orig: PCOM_set_person_ai_knocked_out (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_knocked_out(Thing* p_person);

// uc_orig: PCOM_set_person_ai_arrest (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_arrest(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_set_person_ai_leavecar (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_leavecar(Thing* p_person, SLONG excar_state, SLONG excar_substate, SLONG excar_arg);

// uc_orig: PCOM_set_person_ai_investigate (fallen/Source/pcom.cpp)
void PCOM_set_person_ai_investigate(Thing* p_person, SLONG odd_x, SLONG odd_z);

// uc_orig: PCOM_process_getitem (fallen/Source/pcom.cpp)
void PCOM_process_getitem(Thing* p_person);

// uc_orig: PCOM_process_summon (fallen/Source/pcom.cpp)
void PCOM_process_summon(Thing* p_person);

// uc_orig: PCOM_new_gang_attack (fallen/Source/pcom.cpp)
void PCOM_new_gang_attack(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_get_dist_between (fallen/Source/pcom.cpp)
SLONG PCOM_get_dist_between(Thing* p_person_a, Thing* p_person_b);

// uc_orig: PCOM_get_dist_from_home (fallen/Source/pcom.cpp)
SLONG PCOM_get_dist_from_home(Thing* p_person);

// uc_orig: PCOM_get_angle_for_dir (fallen/Source/pcom.cpp)
UWORD PCOM_get_angle_for_dir(SLONG dir);

// uc_orig: PCOM_get_dx_dz_for_dir (fallen/Source/pcom.cpp)
void PCOM_get_dx_dz_for_dir(SLONG dir, SLONG* dx, SLONG* dz);

// uc_orig: PCOM_get_random_duration (fallen/Source/pcom.cpp)
SLONG PCOM_get_random_duration(SLONG min, SLONG max);

// uc_orig: PCOM_should_i_try_to_los_mav_to_person (fallen/Source/pcom.cpp)
SLONG PCOM_should_i_try_to_los_mav_to_person(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_get_person_navsquare (fallen/Source/pcom.cpp)
void PCOM_get_person_navsquare(Thing* p_person, SLONG* map_dest_x, SLONG* map_dest_z);

// uc_orig: PCOM_get_vehicle_navsquare (fallen/Source/pcom.cpp)
void PCOM_get_vehicle_navsquare(Thing* p_vehicle, SLONG* map_dest_x, SLONG* map_dest_z, SLONG i_am_a_passenger, Thing* p_person);

// uc_orig: PCOM_position_person_to_sit_on_prim (fallen/Source/pcom.cpp)
SLONG PCOM_position_person_to_sit_on_prim(Thing* p_person, SLONG prim, SLONG prim_x, SLONG prim_y, SLONG prim_z, SLONG prim_yaw, SLONG dont_teleport);

// uc_orig: PCOM_get_flee_from_pos (fallen/Source/pcom.cpp)
void PCOM_get_flee_from_pos(Thing* p_person, SLONG* from_x, SLONG* from_z);

// uc_orig: PCOM_get_person_dest (fallen/Source/pcom.cpp)
void PCOM_get_person_dest(Thing* p_person, SLONG* dest_x, SLONG* dest_z);

// uc_orig: PCOM_get_mav_action_pos (fallen/Source/pcom.cpp)
void PCOM_get_mav_action_pos(Thing* p_person, SLONG* dest_x, SLONG* dest_z);

// uc_orig: PCOM_person_dist_from (fallen/Source/pcom.cpp)
SLONG PCOM_person_dist_from(Thing* p_person, SLONG world_x, SLONG world_z);

// uc_orig: PCOM_find_hiding_place (fallen/Source/pcom.cpp)
SLONG PCOM_find_hiding_place(Thing* p_person, SLONG* hide_x, SLONG* hide_z);

// uc_orig: PCOM_player_is_doing_something_naughty (fallen/Source/pcom.cpp)
SLONG PCOM_player_is_doing_something_naughty(Thing* darci);

// uc_orig: PCOM_alert_my_gang_to_a_fight (fallen/Source/pcom.cpp)
void PCOM_alert_my_gang_to_a_fight(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_alert_my_gang_to_flee (fallen/Source/pcom.cpp)
void PCOM_alert_my_gang_to_flee(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_should_fake_person_attack_darci (fallen/Source/pcom.cpp)
SLONG PCOM_should_fake_person_attack_darci(Thing* p_person);

// uc_orig: get_rate_of_fire (fallen/Source/pcom.cpp)
SLONG get_rate_of_fire(Thing* p_person);

// uc_orig: person_has_gun_or_grenade_out (fallen/Source/pcom.cpp)
SLONG person_has_gun_or_grenade_out(Thing* p_person);

// Tick constants — must be defined before the inline duration helpers below.
// uc_orig: PCOM_TICKS_PER_TURN (fallen/Source/pcom.cpp)
#define PCOM_TICKS_PER_TURN 16
// uc_orig: PCOM_TICKS_PER_SEC (fallen/Source/pcom.cpp)
#define PCOM_TICKS_PER_SEC (16 * 20)

// uc_orig: PCOM_get_duration (fallen/Source/pcom.cpp)
inline SLONG PCOM_get_duration(SLONG tenths)
{
    return tenths * (PCOM_TICKS_PER_SEC / 10);
}

// uc_orig: PCOM_get_duration100 (fallen/Source/pcom.cpp)
inline SLONG PCOM_get_duration100(SLONG hun)
{
    return hun * (PCOM_TICKS_PER_SEC / 100);
}

// uc_orig: PersonIsMIB (fallen/Source/pcom.cpp)
BOOL PersonIsMIB(Thing* p_person);

// uc_orig: PCOM_get_zone_for_position (fallen/Source/pcom.cpp)
UBYTE PCOM_get_zone_for_position(Thing* p_person);

// uc_orig: PCOM_get_zone_for_position (fallen/Source/pcom.cpp)  [overload: x,z variant]
UBYTE PCOM_get_zone_for_position(SLONG x, SLONG z);

// uc_orig: PCOM_can_i_see_person_to_attack (fallen/Source/pcom.cpp)
Thing* PCOM_can_i_see_person_to_attack(Thing* p_person);

// uc_orig: PCOM_can_i_see_person_to_bully (fallen/Source/pcom.cpp)
Thing* PCOM_can_i_see_person_to_bully(Thing* p_person);

// uc_orig: PCOM_can_i_see_person_to_arrest (fallen/Source/pcom.cpp)
Thing* PCOM_can_i_see_person_to_arrest(Thing* p_person);

// uc_orig: PCOM_can_i_see_person_to_taunt (fallen/Source/pcom.cpp)
Thing* PCOM_can_i_see_person_to_taunt(Thing* p_person);

// uc_orig: PCOM_get_next_patrol_waypoint (fallen/Source/pcom.cpp)
SLONG PCOM_get_next_patrol_waypoint(Thing* p_person);

// uc_orig: PCOM_target_sprinting_towards_me (fallen/Source/pcom.cpp)
SLONG PCOM_target_sprinting_towards_me(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_target_could_shoot_me (fallen/Source/pcom.cpp)
SLONG PCOM_target_could_shoot_me(Thing* p_person, Thing* p_shooter);

// uc_orig: PCOM_do_regen (fallen/Source/pcom.cpp)
SLONG PCOM_do_regen(Thing* p_person);

// uc_orig: PCOM_alert_nearby_mib_to_attack (fallen/Source/pcom.cpp)
void PCOM_alert_nearby_mib_to_attack(Thing* p_person);

// uc_orig: PCOM_find_bodyguard_victim (fallen/Source/pcom.cpp)
Thing* PCOM_find_bodyguard_victim(Thing* p_bodyguard, Thing* p_client);

// uc_orig: PCOM_find_runover_thing (fallen/Source/pcom.cpp)
SLONG PCOM_find_runover_thing(Thing* p_person, SLONG dangle);

// uc_orig: PCOM_find_bomb (fallen/Source/pcom.cpp)
UWORD PCOM_find_bomb(Thing* p_person);

// uc_orig: PCOM_is_there_an_item_i_should_get (fallen/Source/pcom.cpp)
Thing* PCOM_is_there_an_item_i_should_get(Thing* p_person);

// uc_orig: PCOM_follow_speed (fallen/Source/pcom.cpp)
SLONG PCOM_follow_speed(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_teleport_home (fallen/Source/pcom.cpp)
void PCOM_teleport_home(Thing* p_person);

// uc_orig: PCOM_process_person (already declared above via header)

// uc_orig: PCOM_process_default (fallen/Source/pcom.cpp)
void PCOM_process_default(Thing* p_person);

// uc_orig: PCOM_process_state_change (fallen/Source/pcom.cpp)
void PCOM_process_state_change(Thing* p_person);

// uc_orig: PCOM_process_movement (fallen/Source/pcom.cpp)
void PCOM_process_movement(Thing* p_person);

// uc_orig: init_arrest (fallen/Source/pcom.cpp)
void init_arrest(void);

// uc_orig: do_arrests (fallen/Source/pcom.cpp)
void do_arrests(void);

// uc_orig: should_person_regen (fallen/Source/pcom.cpp)
SLONG should_person_regen(Thing* p_person);

// uc_orig: PCOM_process_driving_still (fallen/Source/pcom.cpp)
void PCOM_process_driving_still(Thing* p_person);

// uc_orig: PCOM_process_driving_patrol (fallen/Source/pcom.cpp)
void PCOM_process_driving_patrol(Thing* p_person);

// uc_orig: PCOM_process_driving_wander (fallen/Source/pcom.cpp)
void PCOM_process_driving_wander(Thing* p_person);

// uc_orig: PCOM_process_patrol (fallen/Source/pcom.cpp)
void PCOM_process_patrol(Thing* p_person);

// uc_orig: PCOM_process_wander (fallen/Source/pcom.cpp)
void PCOM_process_wander(Thing* p_person);

// uc_orig: PCOM_process_killing (fallen/Source/pcom.cpp)
void PCOM_process_killing(Thing* p_person);

// uc_orig: PCOM_process_fleeing (fallen/Source/pcom.cpp)
void PCOM_process_fleeing(Thing* p_person);

// uc_orig: PCOM_process_investigating (fallen/Source/pcom.cpp)
void PCOM_process_investigating(Thing* p_person);

// uc_orig: PCOM_process_following (fallen/Source/pcom.cpp)
void PCOM_process_following(Thing* p_person);

// uc_orig: PCOM_find_mib_appear_pos (fallen/Source/pcom.cpp)
void PCOM_find_mib_appear_pos(Thing* p_mib, Thing* p_target, SLONG* appear_x, SLONG* appear_z);

// uc_orig: draw_view_line (fallen/Source/pcom.cpp)
void draw_view_line(Thing* p_person, Thing* p_target);

// uc_orig: PCOM_process_navtokill (fallen/Source/pcom.cpp)
void PCOM_process_navtokill(Thing* p_person);

// uc_orig: PCOM_process_findcar (fallen/Source/pcom.cpp)
void PCOM_process_findcar(Thing* p_person);

// uc_orig: PCOM_process_talk (fallen/Source/pcom.cpp)
void PCOM_process_talk(Thing* p_person);

// uc_orig: PCOM_process_hands_up (fallen/Source/pcom.cpp)
void PCOM_process_hands_up(Thing* p_person);

// uc_orig: PCOM_process_hitch (fallen/Source/pcom.cpp)
void PCOM_process_hitch(Thing* p_person);

// uc_orig: PCOM_process_knockedout (fallen/Source/pcom.cpp)
void PCOM_process_knockedout(Thing* p_person);

// uc_orig: PCOM_process_taunt (fallen/Source/pcom.cpp)
void PCOM_process_taunt(Thing* p_person);

// uc_orig: PCOM_process_arrest (fallen/Source/pcom.cpp)
void PCOM_process_arrest(Thing* p_person);

// uc_orig: PCOM_process_homesick (fallen/Source/pcom.cpp)
void PCOM_process_homesick(Thing* p_person);

// uc_orig: PCOM_process_bdeactivate (fallen/Source/pcom.cpp)
void PCOM_process_bdeactivate(Thing* p_person);

// uc_orig: PCOM_process_leavecar (fallen/Source/pcom.cpp)
void PCOM_process_leavecar(Thing* p_person);

// uc_orig: PCOM_process_snipe (fallen/Source/pcom.cpp)
void PCOM_process_snipe(Thing* p_person);

// uc_orig: PCOM_process_warm_hands (fallen/Source/pcom.cpp)
void PCOM_process_warm_hands(Thing* p_person);

// uc_orig: PCOM_process_normal (fallen/Source/pcom.cpp)
void PCOM_process_normal(Thing* p_person);

#endif // AI_PCOM_H
