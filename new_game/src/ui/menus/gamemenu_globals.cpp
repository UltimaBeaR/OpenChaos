#include "ui/menus/gamemenu_globals.h"
#include "assets/xlat_str.h"

// Menu definitions: word[0] is the title, word[1..7] are the selectable items.
// uc_orig: GAMEMENU_menu (fallen/Source/gamemenu.cpp)
GAMEMENU_Menu GAMEMENU_menu[] = {
    { NULL },
    { X_GAME_PAUSED, X_RESUME_LEVEL, X_RESTART_LEVEL, X_ABANDON_GAME },
    { X_LEVEL_COMPLETE },
    { X_LEVEL_LOST, X_RESTART_LEVEL, X_ABANDON_GAME },
    { X_ARE_YOU_SURE, X_OKAY, X_CANCEL }
};

// uc_orig: GAMEMENU_menu_type (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_menu_type = 0;
// uc_orig: GAMEMENU_menu_selection (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_menu_selection = 0;
// uc_orig: GAMEMENU_background (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_background = 0;
// uc_orig: GAMEMENU_fadein_x (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_fadein_x = 0;
// uc_orig: GAMEMENU_slowdown (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_slowdown = 0;
// uc_orig: GAMEMENU_wait (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_wait = 0;
// uc_orig: GAMEMENU_level_lost_reason (fallen/Source/gamemenu.cpp)
CBYTE* GAMEMENU_level_lost_reason = nullptr;
