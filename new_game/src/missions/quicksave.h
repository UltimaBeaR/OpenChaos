#ifndef MISSIONS_QUICKSAVE_H
#define MISSIONS_QUICKSAVE_H

// In-mission quick save / quick load (OpenChaos addition, behind the feature flag
// ext_in_mission_saves). See docs/topics/in_mission_saves.md.
//
// This is the only API the game code calls. Every function returns immediately
// (and touches no menu row, file or key) when the flag is off. All logic lives in
// missions/quicksave.cpp; the call sites are one line each in ui/menus/gamemenu.cpp
// and game/game.cpp.
//
// The module never sees a menu-type constant: gamemenu.cpp passes the row it opens
// plus a purpose enum, and performs every menu transition itself.

#include "engine/core/types.h"

// Why a save was refused, in the order the conditions are tested (the first hit wins).
enum QS_Refusal {
    QS_OK = 0,
    QS_REFUSED_FLAG,
    QS_REFUSED_NOT_IN_MISSION,
    QS_REFUSED_DEAD,
    QS_REFUSED_VEHICLE,
    QS_REFUSED_COMBAT,
    QS_REFUSED_ENEMY,
    QS_REFUSED_CUTSCENE,
    QS_REFUSED_OTHER
};

// How the campaign data embedded in a quick save compares with current progress.
enum QS_CampaignMatch {
    QS_CAMPAIGN_SAME = 0,
    QS_CAMPAIGN_SAVED_OLDER,
    QS_CAMPAIGN_DIFFERENT
};

// What QUICKSAVE_draw_menu_body() draws (one message at a time).
enum QS_Body {
    QS_BODY_NONE = 0,
    QS_BODY_SAVE_REFUSED,
    QS_BODY_LOAD_FAILED,
    QS_BODY_WARN_OLDER,
    QS_BODY_WARN_DIFFERENT
};

// Which menu is opening, as told by gamemenu.cpp.
enum QS_MenuKind {
    QS_MENU_OTHER = 0,
    QS_MENU_PAUSE,
    QS_MENU_LOST
};

// The answer to "Load quick save".
enum QS_Load {
    QS_LOAD_NONE = 0, // nothing to do; the menu body says why
    QS_LOAD_NOW, // start the reload (gamemenu returns GAMEMENU_DO_QUICK_LOAD)
    QS_LOAD_NEEDS_CONFIRM // campaign mismatch: gamemenu opens the SURE dialog
};

// Called by gamemenu.cpp whenever a menu opens. For the pause and the
// level-lost menus `row` is that menu's GAMEMENU_Menu.word[8] and the module inserts
// "Save game" / "Load game"; for every other menu `row` is NULL.
void QUICKSAVE_menu_hook(UBYTE* row, QS_MenuKind kind);

// Called inside GAMEMENU_draw's framed branch, after the items: the refusal line or
// the confirmation dialog's body text.
void QUICKSAVE_draw_menu_body(void);

// "Save game" was confirmed. Writes the quick save when saving is allowed and returns
// true (the menu closes); otherwise records the reason for the menu body and returns false.
bool QUICKSAVE_menu_save(void);

// "Load game" was confirmed. Validates the file and compares its campaign data.
QS_Load QUICKSAVE_menu_load(void);

// "Okay" was confirmed in the SURE dialog. True (and the restore is armed) only when
// the dialog was opened by QUICKSAVE_menu_load; false for every other use of SURE.
bool QUICKSAVE_confirm_load(void);

// Called first thing after a level (re)load, before the first tick: applies a pending
// quick load over the freshly loaded level.
void QUICKSAVE_apply_pending(void);

// Called when the player restarts the mission normally: drops a pending restore.
void QUICKSAVE_clear_pending(void);

#endif // MISSIONS_QUICKSAVE_H
