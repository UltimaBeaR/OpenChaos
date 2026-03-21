#ifndef MISSIONS_ELEV_GLOBALS_H
#define MISSIONS_ELEV_GLOBALS_H

#include "Game.h"
#include "fallen/Headers/Mission.h"

// uc_orig: ELEV_last_map_loaded (fallen/Source/elev.cpp)
extern CBYTE ELEV_last_map_loaded[MAX_PATH];

// uc_orig: junk (fallen/Source/elev.cpp)
extern CBYTE junk[2048];

// uc_orig: event_point (fallen/Source/elev.cpp)
extern EventPoint event_point;

// uc_orig: iamapsx (fallen/Source/elev.cpp)
extern SLONG iamapsx;

// uc_orig: quick_load (fallen/Source/elev.cpp)
extern SLONG quick_load;

// uc_orig: loading_screen_active (fallen/Source/elev.cpp)
extern UBYTE loading_screen_active;

// uc_orig: ELEV_fname_map (fallen/Source/elev.cpp)
extern CBYTE ELEV_fname_map[_MAX_PATH];

// uc_orig: ELEV_fname_lighting (fallen/Source/elev.cpp)
extern CBYTE ELEV_fname_lighting[_MAX_PATH];

// uc_orig: ELEV_fname_citsez (fallen/Source/elev.cpp)
extern CBYTE ELEV_fname_citsez[_MAX_PATH];

// uc_orig: ELEV_fname_level (fallen/Source/elev.cpp)
extern CBYTE ELEV_fname_level[_MAX_PATH];

#endif // MISSIONS_ELEV_GLOBALS_H
