#include "ui/frontend/startscr_globals.h"
#include "engine/core/types.h" // _MAX_PATH (not in <stdlib.h> on macOS/Linux)

// uc_orig: STARTSCR_mission (fallen/Source/startscr.cpp)
// Bridge between the frontend (mission selection) and the level loader (ELEV_load).
char STARTSCR_mission[_MAX_PATH] = { 0 };
