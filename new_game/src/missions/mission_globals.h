#ifndef MISSIONS_MISSION_GLOBALS_H
#define MISSIONS_MISSION_GLOBALS_H

#include "missions/mission.h"

// Flat pool of all mission slots. Indexed by mission number.
// uc_orig: mission_pool (fallen/Headers/Mission.h)
extern Mission mission_pool[MAX_MISSIONS];

// Pointer to the currently loaded mission slot.
// uc_orig: current_mission (fallen/Headers/Mission.h)
extern Mission* current_mission;

// Per-mission zone maps: 128x128 byte grid of ZF_* flags per cell.
// uc_orig: MissionZones (fallen/Headers/Mission.h)
extern CBYTE MissionZones[MAX_MISSIONS][128][128];

// Pointer to the currently active map.
// uc_orig: current_map (fallen/Headers/Mission.h)
extern GameMap* current_map;

// Flat pool of all map slots.
// uc_orig: game_maps (fallen/Headers/Mission.h)
extern GameMap game_maps[MAX_MAPS];

// Bitmask table: for each TT_* trigger type, which property fields are active.
// Sized by TT_NUMBER; defined in elev.cpp.
// uc_orig: WaypointUses (fallen/Headers/Mission.h)
extern CBYTE WaypointUses[TT_NUMBER];

#endif // MISSIONS_MISSION_GLOBALS_H
