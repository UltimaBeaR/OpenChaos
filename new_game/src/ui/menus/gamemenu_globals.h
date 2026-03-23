#ifndef UI_MENUS_GAMEMENU_GLOBALS_H
#define UI_MENUS_GAMEMENU_GLOBALS_H

#include "core/types.h"

// Each menu entry: word[0] is the title string id, word[1..7] are the selectable item ids.
// uc_orig: GAMEMENU_Menu (fallen/Source/gamemenu.cpp)
typedef struct
{
    UBYTE word[8];
} GAMEMENU_Menu;

// Menu table indexed by GAMEMENU_MENU_TYPE_* constants.
// uc_orig: GAMEMENU_menu (fallen/Source/gamemenu.cpp)
extern GAMEMENU_Menu GAMEMENU_menu[];

// uc_orig: GAMEMENU_menu_type (fallen/Source/gamemenu.cpp)
extern SLONG GAMEMENU_menu_type;
// uc_orig: GAMEMENU_menu_selection (fallen/Source/gamemenu.cpp)
extern SLONG GAMEMENU_menu_selection;
// uc_orig: GAMEMENU_background (fallen/Source/gamemenu.cpp)
extern SLONG GAMEMENU_background;
// uc_orig: GAMEMENU_fadein_x (fallen/Source/gamemenu.cpp)
extern SLONG GAMEMENU_fadein_x;
// uc_orig: GAMEMENU_slowdown (fallen/Source/gamemenu.cpp)
extern SLONG GAMEMENU_slowdown;
// uc_orig: GAMEMENU_wait (fallen/Source/gamemenu.cpp)
extern SLONG GAMEMENU_wait;
// uc_orig: GAMEMENU_level_lost_reason (fallen/Source/gamemenu.cpp)
extern CBYTE* GAMEMENU_level_lost_reason;

#endif // UI_MENUS_GAMEMENU_GLOBALS_H
