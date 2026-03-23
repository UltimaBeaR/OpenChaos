#include "ui/hud/overlay_globals.h"
#include "assets/xlat_str.h"

// uc_orig: help_text (fallen/Source/overlay.cpp)
CBYTE* help_text[] = {
    "Jump up to grab cables",
    "Press action to pickup items",
    "Press action to use vehicles",
    "Press action to use bikes",
    ""
};

// uc_orig: help_xlat (fallen/Source/overlay.cpp)
UWORD help_xlat[] = { X_GRAB_CABLE, X_PICK_UP, X_ENTER_VEHICLE, X_USE_BIKE };

// uc_orig: MAX_TRACK (fallen/Source/overlay.cpp) — size constant used by arrays below
#define MAX_TRACK 4

// uc_orig: panel_enemy (fallen/Source/overlay.cpp)
struct TrackEnemy panel_enemy[MAX_TRACK];

// uc_orig: panel_gun_sight (fallen/Source/overlay.cpp)
struct TrackEnemy panel_gun_sight[MAX_TRACK];

// uc_orig: track_count (fallen/Source/overlay.cpp)
UWORD track_count = 0;

// uc_orig: timer_prev (fallen/Source/overlay.cpp)
SWORD timer_prev = 0;
