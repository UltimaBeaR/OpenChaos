#ifndef UI_FRONTEND_H
#define UI_FRONTEND_H

// uc_orig: frontend.h (fallen/Headers/frontend.h)
// Main frontend / menu system for Urban Chaos.
// Handles the main menu, mission select, briefing, config screens.

#include "MFStdLib.h"

// ---- Structs ---------------------------------------------------------------

// uc_orig: MissionData (fallen/Source/frontend.cpp)
struct MissionData {
    SLONG ObjID, GroupID, ParentID, ParentIsGroup;
    SLONG District;
    SLONG Type, Flags;
    CBYTE fn[255], ttl[255], brief[4096];
};

// uc_orig: RawMenuData (fallen/Source/frontend.cpp)
struct RawMenuData {
    UBYTE Menu;
    UBYTE Type;
    SWORD Label;
    CBYTE* Choices;
    SLONG Data;
};

// uc_orig: MenuData (fallen/Source/frontend.cpp)
struct MenuData {
    UBYTE Type;
    UBYTE LabelID;
    CBYTE* Label;
    CBYTE* Choices;
    SLONG Data;
    UWORD X, Y;
};

// uc_orig: MenuStack (fallen/Source/frontend.cpp)
struct MenuStack {
    UBYTE mode;
    UBYTE selected;
    SWORD scroll;
    CBYTE* title;
};

// uc_orig: MenuState (fallen/Source/frontend.cpp)
struct MenuState {
    UBYTE items;
    UBYTE selected;
    SWORD scroll;
    MenuStack stack[10];
    UBYTE stackpos;
    SBYTE mode;
    SWORD base;
    CBYTE* title;
};

// uc_orig: Kibble (fallen/Source/frontend.cpp)
// Animated particle used for the animated menu background (leaves, rain, snow).
struct Kibble {
    SLONG page;
    SLONG x, y;
    SWORD dx, dy;
    SWORD r, t, p, rd, td, pd;
    UBYTE type, size;
    ULONG rgb;
};

// uc_orig: MissionCache (fallen/Source/frontend.cpp)
struct MissionCache {
    CBYTE name[255];
    UBYTE id;
    UBYTE district;
};

// ---- Screen/menu mode IDs (FE_*) -------------------------------------------

// uc_orig: FE_MAINMENU (fallen/Source/frontend.cpp)
#define FE_MAINMENU (1)
// uc_orig: FE_MAPSCREEN (fallen/Source/frontend.cpp)
#define FE_MAPSCREEN (2)
// uc_orig: FE_MISSIONSELECT (fallen/Source/frontend.cpp)
#define FE_MISSIONSELECT (3)
// uc_orig: FE_MISSIONBRIEFING (fallen/Source/frontend.cpp)
#define FE_MISSIONBRIEFING (4)
// uc_orig: FE_LOADSCREEN (fallen/Source/frontend.cpp)
#define FE_LOADSCREEN (5)
// uc_orig: FE_SAVESCREEN (fallen/Source/frontend.cpp)
#define FE_SAVESCREEN (6)
// uc_orig: FE_CONFIG (fallen/Source/frontend.cpp)
#define FE_CONFIG (7)
// uc_orig: FE_CONFIG_VIDEO (fallen/Source/frontend.cpp)
#define FE_CONFIG_VIDEO (8)
// uc_orig: FE_CONFIG_AUDIO (fallen/Source/frontend.cpp)
#define FE_CONFIG_AUDIO (9)
// uc_orig: FE_CONFIG_INPUT_KB (fallen/Source/frontend.cpp)
#define FE_CONFIG_INPUT_KB (10)
// uc_orig: FE_CONFIG_INPUT_JP (fallen/Source/frontend.cpp)
#define FE_CONFIG_INPUT_JP (11)
// uc_orig: FE_CONFIG_OPTIONS (fallen/Source/frontend.cpp)
#define FE_CONFIG_OPTIONS (13)
// uc_orig: FE_QUIT (fallen/Source/frontend.cpp)
#define FE_QUIT (12)
// uc_orig: FE_SAVE_CONFIRM (fallen/Source/frontend.cpp)
#define FE_SAVE_CONFIRM (14)

// uc_orig: FE_NO_REALLY_QUIT (fallen/Source/frontend.cpp)
#define FE_NO_REALLY_QUIT (-1)
// uc_orig: FE_BACK (fallen/Source/frontend.cpp)
#define FE_BACK (-2)
// uc_orig: FE_START (fallen/Source/frontend.cpp)
#define FE_START (-3)
// uc_orig: FE_EDITOR (fallen/Source/frontend.cpp)
#define FE_EDITOR (-4)
// uc_orig: FE_CREDITS (fallen/Source/frontend.cpp)
#define FE_CREDITS (-5)

// ---- Menu object types (OT_*) ----------------------------------------------

// uc_orig: OT_BUTTON (fallen/Source/frontend.cpp)
#define OT_BUTTON (1)
// uc_orig: OT_BUTTON_L (fallen/Source/frontend.cpp)
#define OT_BUTTON_L (2)
// uc_orig: OT_SLIDER (fallen/Source/frontend.cpp)
#define OT_SLIDER (3)
// uc_orig: OT_MULTI (fallen/Source/frontend.cpp)
#define OT_MULTI (4)
// uc_orig: OT_KEYPRESS (fallen/Source/frontend.cpp)
#define OT_KEYPRESS (5)
// uc_orig: OT_LABEL (fallen/Source/frontend.cpp)
#define OT_LABEL (6)
// uc_orig: OT_PADPRESS (fallen/Source/frontend.cpp)
#define OT_PADPRESS (7)
// uc_orig: OT_RESET (fallen/Source/frontend.cpp)
#define OT_RESET (8)
// uc_orig: OT_PADMOVE (fallen/Source/frontend.cpp)
#define OT_PADMOVE (9)

// uc_orig: MC_YN (fallen/Source/frontend.cpp)
#define MC_YN (CBYTE*)(1)
// uc_orig: MC_SCANNER (fallen/Source/frontend.cpp)
#define MC_SCANNER (CBYTE*)(2)

// ---- Misc defines ----------------------------------------------------------

// uc_orig: BIG_FONT_SCALE (fallen/Source/frontend.cpp)
#define BIG_FONT_SCALE (192)
// uc_orig: BIG_FONT_SCALE_FRENCH (fallen/Source/frontend.cpp)
#define BIG_FONT_SCALE_FRENCH (176)
// uc_orig: SMALL_FONT_SCALE (fallen/Source/frontend.cpp)
#define SMALL_FONT_SCALE (256)

// uc_orig: AUTOPLAY_FMV_DELAY (fallen/Source/frontend.cpp)
#define AUTOPLAY_FMV_DELAY (1000 * 60 * 2)

// uc_orig: SCRIPT_MEMORY (fallen/Source/frontend.cpp)
#define SCRIPT_MEMORY (20 * 1024)

// uc_orig: KIBBLE_Z (fallen/Source/frontend.cpp)
#define KIBBLE_Z 0.5

// ---- Functions (public API) ------------------------------------------------

// uc_orig: FRONTEND_init (fallen/Headers/frontend.h)
void FRONTEND_init(bool bGoToTitleScreen = UC_FALSE);
// uc_orig: FRONTEND_loop (fallen/Headers/frontend.h)
SBYTE FRONTEND_loop();
// uc_orig: FRONTEND_level_won (fallen/Headers/frontend.h)
void FRONTEND_level_won();
// uc_orig: FRONTEND_level_lost (fallen/Headers/frontend.h)
void FRONTEND_level_lost();

// uc_orig: FRONTEND_display (fallen/Source/frontend.cpp)
void FRONTEND_display(void);

// uc_orig: CacheScriptInMemory (fallen/Source/frontend.cpp)
void CacheScriptInMemory(CBYTE* script_fname);
// uc_orig: FileOpenScript (fallen/Source/frontend.cpp)
void FileOpenScript(void);
// uc_orig: FileCloseScript (fallen/Source/frontend.cpp)
void FileCloseScript(void);
// uc_orig: LoadStringScript (fallen/Source/frontend.cpp)
CBYTE* LoadStringScript(CBYTE* txt);

// uc_orig: FRONTEND_scr_add (fallen/Source/frontend.cpp)
void FRONTEND_scr_add(LPDIRECTDRAWSURFACE4* screen, UBYTE* image_data);
// uc_orig: FRONTEND_scr_img_load_into_screenfull (fallen/Source/frontend.cpp)
void FRONTEND_scr_img_load_into_screenfull(CBYTE* name, LPDIRECTDRAWSURFACE4* screen);
// uc_orig: FRONTEND_scr_unload_theme (fallen/Source/frontend.cpp)
void FRONTEND_scr_unload_theme();
// uc_orig: FRONTEND_scr_new_theme (fallen/Source/frontend.cpp)
void FRONTEND_scr_new_theme(CBYTE* fname_back, CBYTE* fname_map, CBYTE* fname_brief, CBYTE* fname_config);
// uc_orig: FRONTEND_restore_screenfull_surfaces (fallen/Source/frontend.cpp)
void FRONTEND_restore_screenfull_surfaces(void);

// uc_orig: FRONTEND_ParseMissionData (fallen/Source/frontend.cpp)
void FRONTEND_ParseMissionData(CBYTE* text, CBYTE version, MissionData* mdata);
// uc_orig: FRONTEND_LoadString (fallen/Source/frontend.cpp)
CBYTE* FRONTEND_LoadString(MFFileHandle& file, CBYTE* txt);
// uc_orig: FRONTEND_SaveString (fallen/Source/frontend.cpp)
void FRONTEND_SaveString(MFFileHandle& file, CBYTE* txt);

// uc_orig: FRONTEND_AlterAlpha (fallen/Source/frontend.cpp)
SLONG FRONTEND_AlterAlpha(SLONG rgb, SWORD add, SBYTE shift);
// uc_orig: FRONTEND_recenter_menu (fallen/Source/frontend.cpp)
void FRONTEND_recenter_menu(void);
// uc_orig: FRONTEND_fix_rgb (fallen/Source/frontend.cpp)
ULONG FRONTEND_fix_rgb(ULONG rgb, BOOL sel);
// uc_orig: FRONTEND_draw_title (fallen/Source/frontend.cpp)
void FRONTEND_draw_title(SLONG x, SLONG y, SLONG cutx, CBYTE* str, BOOL wibble, BOOL r_to_l);

// uc_orig: FRONTEND_init_xition (fallen/Source/frontend.cpp)
void FRONTEND_init_xition(void);
// uc_orig: FRONTEND_show_xition (fallen/Source/frontend.cpp)
void FRONTEND_show_xition();
// uc_orig: FRONTEND_stop_xition (fallen/Source/frontend.cpp)
void FRONTEND_stop_xition();

// uc_orig: FRONTEND_draw_button (fallen/Source/frontend.cpp)
void FRONTEND_draw_button(SLONG x, SLONG y, UBYTE which, UBYTE flash = UC_FALSE);
// uc_orig: FRONTEND_kibble_draw (fallen/Source/frontend.cpp)
void FRONTEND_kibble_draw();
// uc_orig: FRONTEND_DrawSlider (fallen/Source/frontend.cpp)
void FRONTEND_DrawSlider(MenuData* md);
// uc_orig: FRONTEND_DrawMulti (fallen/Source/frontend.cpp)
void FRONTEND_DrawMulti(MenuData* md, ULONG rgb);
// uc_orig: FRONTEND_DrawKey (fallen/Source/frontend.cpp)
void FRONTEND_DrawKey(MenuData* md);
// uc_orig: FRONTEND_DrawPad (fallen/Source/frontend.cpp)
void FRONTEND_DrawPad(MenuData* md);

// uc_orig: FRONTEND_kibble_init_one (fallen/Source/frontend.cpp)
void FRONTEND_kibble_init_one(Kibble* k, UBYTE type);
// uc_orig: FRONTEND_kibble_init (fallen/Source/frontend.cpp)
void FRONTEND_kibble_init();
// uc_orig: FRONTEND_kibble_flurry (fallen/Source/frontend.cpp)
void FRONTEND_kibble_flurry();
// uc_orig: FRONTEND_kibble_process (fallen/Source/frontend.cpp)
void FRONTEND_kibble_process();

#endif // UI_FRONTEND_H
