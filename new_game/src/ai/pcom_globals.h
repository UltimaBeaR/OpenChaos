#ifndef AI_PCOM_GLOBALS_H
#define AI_PCOM_GLOBALS_H

#include "game/game_types.h"
#include "ai/pcom.h"

// uc_orig: PCOM_ai_state_name (fallen/Source/pcom.cpp)
extern CBYTE* PCOM_ai_state_name[PCOM_AI_STATE_NUMBER];

// uc_orig: PCOM_ai_substate_name (fallen/Source/pcom.cpp)
extern CBYTE* PCOM_ai_substate_name[PCOM_AI_SUBSTATE_NUMBER];

// uc_orig: PCOM_ai_name (fallen/Source/pcom.cpp)
extern CBYTE* PCOM_ai_name[PCOM_AI_NUMBER];

// uc_orig: PCOM_bent_name (fallen/Source/pcom.cpp)
extern CBYTE* PCOM_bent_name[PCOM_BENT_NUMBER];

// uc_orig: PCOM_move_name (fallen/Source/pcom.cpp)
extern CBYTE* PCOM_move_name[PCOM_MOVE_NUMBER];

// uc_orig: PCOM_move_state_name (fallen/Source/pcom.cpp)
extern CBYTE* PCOM_move_state_name[];

// Gang tracking data — one slot per gang colour, sorted list of members.
// uc_orig: PCOM_gang_person (fallen/Source/pcom.cpp)
extern THING_INDEX PCOM_gang_person[PCOM_MAX_GANG_PEOPLE];

// uc_orig: PCOM_gang_person_upto (fallen/Source/pcom.cpp)
extern SLONG PCOM_gang_person_upto;

// uc_orig: PCOM_Gang (fallen/Source/pcom.cpp)
typedef struct
{
    UBYTE index;
    UBYTE number;
} PCOM_Gang;

// uc_orig: PCOM_gang (fallen/Source/pcom.cpp)
extern PCOM_Gang PCOM_gang[PCOM_MAX_GANGS];

// Shared scratch buffer for nearby-thing searches.
// uc_orig: PCOM_found (fallen/Source/pcom.cpp)
extern UWORD PCOM_found[PCOM_MAX_FIND];

// uc_orig: PCOM_found_num (fallen/Source/pcom.cpp)
extern SLONG PCOM_found_num;

// Bane boss summon targets — up to 4 bodies raised during PCOM_AI_STATE_SUMMON.
// uc_orig: PCOM_SUMMON_NUM_BODIES (fallen/Source/pcom.cpp)
#define PCOM_SUMMON_NUM_BODIES 4
// uc_orig: PCOM_summon (fallen/Source/pcom.cpp)
extern UWORD PCOM_summon[PCOM_SUMMON_NUM_BODIES];

// Deferred arrest queue — persons that must be arrested at end of frame.
// uc_orig: MAX_ARREST_ME (fallen/Source/pcom.cpp)
#define MAX_ARREST_ME 100
// uc_orig: arrest_me (fallen/Source/pcom.cpp)
extern Thing* arrest_me[MAX_ARREST_ME];
// uc_orig: next_arrest (fallen/Source/pcom.cpp)
extern UWORD next_arrest;

// The pedestrian or person that scared the current vehicle driver into fleeing.
// Set by PCOM_find_runover_thing, read by DriveCar.
// uc_orig: PCOM_runover_scary_person (fallen/Source/pcom.cpp)
extern Thing* PCOM_runover_scary_person;

// Noise propagation queue — collects sounds during physics/collision for
// batch processing at end of frame via process_noises().
// uc_orig: MAX_NOISE (fallen/Source/pcom.cpp)
#define MAX_NOISE 4

// uc_orig: Noise (fallen/Source/pcom.cpp)
struct Noise {
    UWORD Type;
    UWORD Person;
    SWORD X, Y, Z;
};

// uc_orig: noise_count (fallen/Source/pcom.cpp)
extern SWORD noise_count;
// uc_orig: noises (fallen/Source/pcom.cpp)
extern struct Noise noises[MAX_NOISE + 1];

// Static debug output buffer for PCOM_person_state_debug().
// uc_orig: PCOM_debug_string (fallen/Source/pcom.cpp)
extern CBYTE PCOM_debug_string[256];

#endif // AI_PCOM_GLOBALS_H
