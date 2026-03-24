#ifndef ASSETS_STARTSCR_H
#define ASSETS_STARTSCR_H

#include "engine/platform/uc_common.h"

// Action codes returned by the start screen / frontend loop.
// uc_orig: STARTS_START (fallen/Headers/startscr.h)
#define STARTS_START 1
// uc_orig: STARTS_EDITOR (fallen/Headers/startscr.h)
#define STARTS_EDITOR 2
// uc_orig: STARTS_LOAD (fallen/Headers/startscr.h)
#define STARTS_LOAD 3
// uc_orig: STARTS_EXIT (fallen/Headers/startscr.h)
#define STARTS_EXIT 4
// uc_orig: STARTS_HOST (fallen/Headers/startscr.h)
#define STARTS_HOST 5
// uc_orig: STARTS_JOIN (fallen/Headers/startscr.h)
#define STARTS_JOIN 6
// uc_orig: STARTS_PLAYBACK (fallen/Headers/startscr.h)
#define STARTS_PLAYBACK 7
// uc_orig: STARTS_PSX (fallen/Headers/startscr.h)
#define STARTS_PSX 8
// uc_orig: STARTS_MULTI (fallen/Headers/startscr.h)
#define STARTS_MULTI 9
// uc_orig: STARTS_LANGUAGE_CHANGE (fallen/Headers/startscr.h)
#define STARTS_LANGUAGE_CHANGE 10

// Header for a menu page: index into the item array, item count, current selection, and type flags.
// uc_orig: StartMenu (fallen/Headers/startscr.h)
struct StartMenu {
    UBYTE StartIndex;
    UBYTE Count;
    UBYTE Current;
    UWORD Type;
};

// Simple menu item: display string, next menu index, action code, and two unused slots.
// uc_orig: StartMenuItemSimple (fallen/Headers/startscr.h)
struct StartMenuItemSimple {
    CBYTE* Str;
    SLONG NextMenu;
    SLONG Action;
    SLONG Dummy1;
    SLONG Dummy2;
};

// Complex menu item: main string plus up to 3 sub-strings (e.g. for option values),
// next menu index, action code, item index, and one unused slot.
// uc_orig: StartMenuItemComplex (fallen/Headers/startscr.h)
struct StartMenuItemComplex {
    CBYTE* Str;
    CBYTE* Strb[3];
    SLONG NextMenu;
    SLONG Action;
    SLONG Item;
    SLONG Dummy2;
};

// Called when a mission ends (won = 1 or lost = 0) to reset the frontend state.
// uc_orig: STARTSCR_notify_gameover (fallen/Headers/startscr.h)
void STARTSCR_notify_gameover(BOOL won);

// Callback type for MissionListCallback: receives the mission script filename.
// uc_orig: MISSION_callback (fallen/Headers/startscr.h)
typedef void (*MISSION_callback)(CBYTE* filename);

// Iterates available mission scripts and calls cb for each one.
// uc_orig: MissionListCallback (fallen/Headers/startscr.h)
void MissionListCallback(CBYTE* script, MISSION_callback cb);

#endif // ASSETS_STARTSCR_H
