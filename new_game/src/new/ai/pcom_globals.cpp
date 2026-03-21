#include "ai/pcom_globals.h"

// uc_orig: PCOM_ai_state_name (fallen/Source/pcom.cpp)
CBYTE* PCOM_ai_state_name[PCOM_AI_STATE_NUMBER] = {
    "Player",
    "Normal",
    "Investigating",
    "Searching",
    "Killing",
    "Sleeping",
    "Flee Place",
    "Flee Person",
    "Following",
    "Navtokil",
    "Homesick",
    "Lookaround",
    "Findcar",
    "Deactivate bomb",
    "Leave car",
    "Snipe",
    "Warmhands",
    "Findbike",
    "Knocked out",
    "Taunt",
    "Arrest",
    "Talking",
    "Grappled",
    "Enter car as passenger",
    "Aimless",
    "Hands up",
    "Summon",
    "Get item"
};

// uc_orig: PCOM_ai_substate_name (fallen/Source/pcom.cpp)
CBYTE* PCOM_ai_substate_name[PCOM_AI_SUBSTATE_NUMBER] = {
    "None",
    "Suprised",
    "Walkover",
    "Look",
    "Punching",
    "Kicking",
    "Leg-it!",
    "Hunting",
    "Aiming",
    "No more ammo",
    "Goto car",
    "Get in car",
    "Goto bomb",
    "Cut wires",
    "Park car",
    "Leave car",
    "Goto fire",
    "Warm up",
    "Goto bike",
    "Hunt Slide",
    "Talk ask",
    "Talk tell",
    "Talk listen",
    "Hitching",
    "Start summon",
    "Mid summon",
    "Draw h2h",
    "Can't find",
    "Waiting"
};

// uc_orig: PCOM_ai_name (fallen/Source/pcom.cpp)
CBYTE* PCOM_ai_name[PCOM_AI_NUMBER] = {
    "player",
    "civillian",
    "guard",
    "assasin",
    "boss",
    "cop",
    "gang",
    "doorman",
    "bodyguard",
    "driver",
    "bomb-disposer",
    "biker",
    "fight test",
    "bully",
    "cop driver",
    "suicide",
    "flee player",
    "kill colour",
    "M.I.B.",
    "Bane",
    "Hypochondria",
    "Shoot dead"
};

// uc_orig: PCOM_bent_name (fallen/Source/pcom.cpp)
CBYTE* PCOM_bent_name[PCOM_BENT_NUMBER] = {
    "Lazy ",
    "Diligent ",
    "Gang ",
    "Fight-back ",
    "Kill-just-the-player ",
    "Robotic ",
    "Restricted ",
    "Player-kill"
};

// uc_orig: PCOM_move_name (fallen/Source/pcom.cpp)
CBYTE* PCOM_move_name[PCOM_MOVE_NUMBER] = {
    "NULL",
    "Still",
    "PATROL",
    "PATROL_RAND",
    "WANDER",
    "FOLLOW",
    "WARM_HANDS",
    "FOLLOW_ON_SEE",
    "DANCE",
    "HANDS_UP",
    "TIED_UP",
};

// uc_orig: PCOM_move_state_name (fallen/Source/pcom.cpp)
CBYTE* PCOM_move_state_name[] = {
    "Player",
    "Still",
    "Goto XZ",
    "Goto waypoint",
    "Goto thing",
    "Pause",
    "Animation",
    "Circle",
    "Driveto",
    "Follow",
    "Park car",
    "Drive down",
    "Biketo",
    "Park bike",
    "Bike down",
    "Number",
    "Grapple",
    "goto thing slide",
    "wait circle",
    "Unused",
    "Unused",
    "Unused"
};

// uc_orig: PCOM_gang_person (fallen/Source/pcom.cpp)
THING_INDEX PCOM_gang_person[PCOM_MAX_GANG_PEOPLE];

// uc_orig: PCOM_gang_person_upto (fallen/Source/pcom.cpp)
SLONG PCOM_gang_person_upto;

// uc_orig: PCOM_gang (fallen/Source/pcom.cpp)
PCOM_Gang PCOM_gang[PCOM_MAX_GANGS];

// uc_orig: PCOM_found (fallen/Source/pcom.cpp)
UWORD PCOM_found[PCOM_MAX_FIND];

// uc_orig: PCOM_found_num (fallen/Source/pcom.cpp)
SLONG PCOM_found_num;
