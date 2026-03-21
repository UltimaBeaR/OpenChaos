#ifndef AI_PCOM_GLOBALS_H
#define AI_PCOM_GLOBALS_H

#include "game.h"
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

// Priority order for assigning cardinal attack angles (compass sectors, clockwise).
// uc_orig: gang_angle_priority (fallen/Source/pcom.cpp)
extern UBYTE gang_angle_priority[8];

#endif // AI_PCOM_GLOBALS_H
