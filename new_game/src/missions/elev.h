#ifndef MISSIONS_ELEV_H
#define MISSIONS_ELEV_H

#include "game/game_types.h"
#include "missions/elev_globals.h"

// Parses a .ucm mission file and translates all EventPoints into EWAY waypoints.
// Also resets game state (things, persons, items) ready for a fresh level start.
// If fname_level is NULL the reset still runs but no file is read (used for blank test levels).
// uc_orig: ELEV_load_level (fallen/Source/elev.cpp)
void ELEV_load_level(CBYTE* fname_level);

// Loads sound wave list and starts ambient sound after map init.
// Called at the end of ELEV_game_init to finalise audio setup.
// uc_orig: ELEV_game_init_common (fallen/Source/elev.cpp)
void ELEV_game_init_common(
    CBYTE* fname_map,
    CBYTE* fname_lighting,
    CBYTE* fname_citsez,
    CBYTE* fname_level);

// Full game world initialisation: loads map, lighting, entities, nav data.
// Returns UC_TRUE on success.
// uc_orig: ELEV_game_init (fallen/Source/elev.cpp)
SLONG ELEV_game_init(
    CBYTE* fname_map,
    CBYTE* fname_lighting,
    CBYTE* fname_citsez,
    CBYTE* fname_level);

// Derives a sibling filename by stripping src's directory and extension,
// then appending a dot and the given ext (e.g. "city.iam" → "city.lgt").
// uc_orig: ELEV_create_similar_name (fallen/Source/elev.cpp)
void ELEV_create_similar_name(CBYTE* dest, CBYTE* src, CBYTE* ext);

// Reads map/lighting/citsez paths from the .ucm header, then calls ELEV_game_init.
// Returns UC_TRUE on success.
// uc_orig: ELEV_load_name (fallen/Source/elev.cpp)
SLONG ELEV_load_name(CBYTE* fname_level);

// Handles level selection at startup: replay playback, STARTSCR_mission, or file dialogs.
// uc_orig: ELEV_load_user (fallen/Source/elev.cpp)
SLONG ELEV_load_user(SLONG mission);

// Reloads the current level from ELEV_fname_level.
// uc_orig: reload_level (fallen/Source/elev.cpp)
void reload_level(void);

#endif // MISSIONS_ELEV_H
