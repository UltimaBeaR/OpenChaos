#include "assets/formats/elev_globals.h"

// Path to the last map file that was loaded. Stored so it can be reused on level restart.
// uc_orig: ELEV_last_map_loaded (fallen/Source/elev.cpp)
CBYTE ELEV_last_map_loaded[MAX_PATH];

// Scratch buffer used to read and discard unused .ucm header fields during level loading.
// 2048 bytes to handle the largest possible field read (paths, skill data, etc).
// uc_orig: junk (fallen/Source/elev.cpp)
CBYTE junk[2048];

// Temporary storage for each EventPoint record as it is parsed from the .ucm file.
// uc_orig: event_point (fallen/Source/elev.cpp)
EventPoint event_point;

// When non-zero, triggers PSX-compatible map behaviour on PC (read from config.ini key "iamapsx").
// uc_orig: iamapsx (fallen/Source/elev.cpp)
SLONG iamapsx = 0;

// Non-zero when a quick-load (save-state restore) is in progress instead of a fresh level load.
// uc_orig: quick_load (fallen/Source/elev.cpp)
SLONG quick_load = 0;

// Set to 1 while the loading screen is active.
// uc_orig: loading_screen_active (fallen/Source/elev.cpp)
UBYTE loading_screen_active;

// The four filename strings filled in by ELEV_load_name() from the .ucm binary header.
// Retained across calls so that reload_level() can re-use the last-loaded paths.
// uc_orig: ELEV_fname_map (fallen/Source/elev.cpp)
CBYTE ELEV_fname_map[_MAX_PATH];

// uc_orig: ELEV_fname_lighting (fallen/Source/elev.cpp)
CBYTE ELEV_fname_lighting[_MAX_PATH];

// uc_orig: ELEV_fname_citsez (fallen/Source/elev.cpp)
CBYTE ELEV_fname_citsez[_MAX_PATH];

// uc_orig: ELEV_fname_level (fallen/Source/elev.cpp)
CBYTE ELEV_fname_level[_MAX_PATH];
