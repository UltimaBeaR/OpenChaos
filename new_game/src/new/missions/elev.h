#ifndef MISSIONS_ELEV_H
#define MISSIONS_ELEV_H

#include "Game.h"
#include "missions/elev_globals.h"

// Parses a .ucm mission file and translates all EventPoints into EWAY waypoints.
// Also resets game state (things, persons, items) ready for a fresh level start.
// If fname_level is NULL the reset still runs but no file is read (used for blank test levels).
// uc_orig: ELEV_load_level (fallen/Source/elev.cpp)
void ELEV_load_level(CBYTE* fname_level);

// Not yet migrated — defined in old/fallen/Source/elev.cpp. Will move in iteration 76.
void ELEV_game_init_common(
    CBYTE* fname_map,
    CBYTE* fname_lighting,
    CBYTE* fname_citsez,
    CBYTE* fname_level);

SLONG ELEV_game_init(
    CBYTE* fname_map,
    CBYTE* fname_lighting,
    CBYTE* fname_citsez,
    CBYTE* fname_level);

void ELEV_create_similar_name(CBYTE* dest, CBYTE* src, CBYTE* ext);

SLONG ELEV_load_name(CBYTE* fname_level);

SLONG ELEV_load_user(SLONG mission);

void reload_level(void);

#endif // MISSIONS_ELEV_H
