#ifndef UI_HUD_OVERLAY_GLOBALS_H
#define UI_HUD_OVERLAY_GLOBALS_H

#include "core/types.h"

struct Thing;

// Internal struct — defined here because it's needed for the extern array declarations.
// uc_orig: TrackEnemy (fallen/Source/overlay.cpp)
struct TrackEnemy {
    Thing* PThing;
    SWORD State;
    SWORD Timer;
    SWORD Face;
};

// uc_orig: help_text (fallen/Source/overlay.cpp)
extern CBYTE* help_text[];

// uc_orig: help_xlat (fallen/Source/overlay.cpp)
extern UWORD help_xlat[];

// uc_orig: panel_enemy (fallen/Source/overlay.cpp)
extern struct TrackEnemy panel_enemy[];

// uc_orig: panel_gun_sight (fallen/Source/overlay.cpp)
extern struct TrackEnemy panel_gun_sight[];

// uc_orig: track_count (fallen/Source/overlay.cpp)
extern UWORD track_count;

// uc_orig: timer_prev (fallen/Source/overlay.cpp)
extern SWORD timer_prev;

#endif // UI_HUD_OVERLAY_GLOBALS_H
