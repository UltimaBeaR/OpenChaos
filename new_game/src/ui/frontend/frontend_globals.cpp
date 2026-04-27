#include "ui/frontend/frontend_globals.h"
#include "assets/xlat_str.h"

// ---- Script buffer ---------------------------------------------------------

// uc_orig: loaded_in_script (fallen/Source/frontend.cpp)
CBYTE loaded_in_script[SCRIPT_MEMORY];
// uc_orig: loaded_in_script_read_upto (fallen/Source/frontend.cpp)
CBYTE* loaded_in_script_read_upto;

// ---- Mission order suggestion list -----------------------------------------

// uc_orig: suggest_order (fallen/Source/frontend.cpp)
CBYTE* suggest_order[] = {
    "testdrive1a.ucm",
    "FTutor1.ucm",
    "Assault1.ucm",
    "police1.ucm",
    "police2.ucm",
    "Bankbomb1.ucm",
    "police3.ucm",
    "Gangorder2.ucm",
    "police4.ucm",
    "bball2.ucm",
    "wstores1.ucm",
    "e3.ucm",
    "westcrime1.ucm",
    "carbomb1.ucm",
    "botanicc.ucm",
    "Semtex.ucm",
    "AWOL1.ucm",
    "mission2.ucm",
    "park2.ucm",
    "MIB.ucm",
    "skymiss2.ucm",
    "factory1.ucm",
    "estate2.ucm",
    "Stealtst1.ucm",
    "snow2.ucm",
    "Gordout1.ucm",
    "Baalrog3.ucm",
    "Finale1.ucm",

    "Gangorder1.ucm",
    "FreeCD1.ucm",
    "jung3.ucm",

    "fight1.ucm",
    "fight2.ucm",
    "testdrive2.ucm",
    "testdrive3.ucm",

    "Album1.ucm",

    "!"
};

// ---- Raw menu data table ---------------------------------------------------

// uc_orig: raw_menu_data (fallen/Source/frontend.cpp)
// Static table defining all menu screens and their items.
// Each entry: { screen, type, label_id, choices, data }
// Sentinel: { (UBYTE)-1, 0, 0, 0 }
RawMenuData raw_menu_data[] = {
    { FE_MAINMENU, OT_BUTTON, X_START, 0, FE_MAPSCREEN },
    { 0, OT_BUTTON, X_LOAD_GAME, 0, FE_LOADSCREEN },
    { 0, OT_BUTTON, X_SAVE_GAME, (CBYTE*)1, FE_SAVESCREEN },
    { 0, OT_BUTTON, X_OPTIONS, 0, FE_CONFIG },
    { 0, OT_BUTTON, X_CREDITS, 0, FE_CREDITS },
    { 0, OT_BUTTON, X_EXIT, 0, FE_QUIT },
    { FE_LOADSCREEN, OT_BUTTON, X_CANCEL, 0, FE_BACK },
    { FE_SAVESCREEN, OT_BUTTON, X_CANCEL, 0, FE_MAPSCREEN },
    { FE_SAVE_CONFIRM, OT_LABEL, X_ARE_YOU_SURE, 0, 0 },
    { 0, OT_BUTTON, X_OKAY, 0, FE_MAPSCREEN },
    { 0, OT_BUTTON, X_CANCEL, 0, FE_BACK },
    { FE_CONFIG, OT_BUTTON, X_OPTIONS, 0, FE_CONFIG_OPTIONS },
    { 0, OT_BUTTON, X_SOUND, 0, FE_CONFIG_AUDIO },
    { 0, OT_BUTTON, X_KEYBOARD, 0, FE_CONFIG_INPUT_KB },
    { 0, OT_BUTTON, X_OKAY, 0, FE_BACK },
    { FE_CONFIG_AUDIO, OT_SLIDER, X_VOLUME, 0, 128 },
    { 0, OT_SLIDER, X_AMBIENT, 0, 128 },
    { 0, OT_SLIDER, X_MUSIC, 0, 128 },
    { 0, OT_BUTTON, X_OKAY, 0, FE_BACK },
    { FE_CONFIG_INPUT_KB, OT_KEYPRESS, X_LEFT, 0, 0 },
    { 0, OT_KEYPRESS, X_RIGHT, 0, 0 },
    { 0, OT_KEYPRESS, X_FORWARDS, 0, 0 },
    { 0, OT_KEYPRESS, X_BACKWARDS, 0, 0 },
    { 0, OT_KEYPRESS, X_PUNCH, 0, 0 },
    { 0, OT_KEYPRESS, X_KICK, 0, 0 },
    { 0, OT_KEYPRESS, X_ACTION, 0, 0 },
    // X_RUN entry is commented out in original
    { 0, OT_KEYPRESS, X_JUMP, 0, 0 },
    { 0, OT_KEYPRESS, X_START, 0, 0 },
    { 0, OT_KEYPRESS, X_SELECT, 0, 0 },
    { 0, OT_LABEL, X_CAMERA, 0, 0 },
    { 0, OT_KEYPRESS, X_JUMP, 0, 0 },
    { 0, OT_KEYPRESS, X_LEFT, 0, 0 },
    { 0, OT_KEYPRESS, X_RIGHT, 0, 0 },
    { 0, OT_KEYPRESS, X_LOOK_AROUND, 0, 0 },
    { 0, OT_RESET, X_RESET_DEFAULT, 0, 0 },
    { 0, OT_BUTTON, X_OKAY, 0, FE_BACK },
    { FE_CONFIG_VIDEO, OT_MULTI, X_STARS, MC_YN, 1 },
    { 0, OT_MULTI, X_SHADOWS, MC_YN, 1 },
    { 0, OT_MULTI, X_PUDDLES, MC_YN, 1 },
    { 0, OT_MULTI, X_DIRT, MC_YN, 1 },
    { 0, OT_MULTI, X_MIST, MC_YN, 1 },
    { 0, OT_MULTI, X_RAIN, MC_YN, 1 },
    { 0, OT_MULTI, X_SKYLINE, MC_YN, 1 },
    { 0, OT_MULTI, X_CRINKLES, MC_YN, 1 },
    { 0, OT_LABEL, X_REFLECTIONS, 0, 0 },
    { 0, OT_MULTI, X_MOON, MC_YN, 1 },
    { 0, OT_MULTI, X_PEOPLE, MC_YN, 1 },
    { 0, OT_MULTI, X_PERSPECTIVE, MC_YN, 1 },
    { 0, OT_MULTI, X_BILINEAR, MC_YN, 1 },
    { 0, OT_BUTTON, X_OKAY, 0, FE_BACK },
    { FE_CONFIG_OPTIONS, OT_LABEL, X_SCANNER, 0, 0 },
    { 0, OT_MULTI, X_TRACK, MC_SCANNER, 0 },
    { 0, OT_BUTTON, X_OKAY, 0, FE_BACK },
    { FE_QUIT, OT_LABEL, X_ARE_YOU_SURE, 0, 0 },
    { 0, OT_BUTTON, X_OKAY, 0, FE_NO_REALLY_QUIT },
    { 0, OT_BUTTON, X_CANCEL, 0, FE_BACK },

    { (UBYTE)-1, 0, 0, 0 },
};

// ---- Menu choice string buffers --------------------------------------------

// uc_orig: menu_choice_yesno (fallen/Source/frontend.cpp)
CBYTE menu_choice_yesno[20];
// uc_orig: menu_choice_scanner (fallen/Source/frontend.cpp)
CBYTE menu_choice_scanner[255];

// ---- Background image names (per-theme) ------------------------------------

// uc_orig: menu_back_names (fallen/Source/frontend.cpp)
CBYTE* menu_back_names[] = { "title leaves1.tga", "title rain1.tga",
    "title snow1.tga", "title blood1.tga" };
// uc_orig: menu_map_names (fallen/Source/frontend.cpp)
CBYTE* menu_map_names[] = { "map leaves darci.tga", "map rain darci.tga",
    "map snow darci.tga", "map blood darci.tga" };
// uc_orig: menu_brief_names (fallen/Source/frontend.cpp)
CBYTE* menu_brief_names[] = { "briefing leaves darci.tga", "briefing rain darci.tga",
    "briefing snow darci.tga", "briefing blood darci.tga" };
// uc_orig: menu_config_names (fallen/Source/frontend.cpp)
CBYTE* menu_config_names[] = { "config leaves.tga", "config rain.tga",
    "config snow.tga", "config blood.tga" };

// uc_orig: frontend_fonttable (fallen/Source/frontend.cpp)
// Note: the 10 special chars at the end were originally Windows-1252 accented
// characters (shown as ? in the source), encoded as UTF-8 replacement chars.
CBYTE frontend_fonttable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!\":;'#$*-()[]\\/?\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd";

// uc_orig: FRONTEND_leaf_colours (fallen/Source/frontend.cpp)
ULONG FRONTEND_leaf_colours[4] = {
    0x665a3a,
    0x486448,
    0x246640,
    0x665E0E
};

// ---- Active menu state globals ---------------------------------------------

// uc_orig: menu_data (fallen/Source/frontend.cpp)
MenuData menu_data[50];
// uc_orig: menu_state (fallen/Source/frontend.cpp)
MenuState menu_state;
// uc_orig: menu_buffer (fallen/Source/frontend.cpp)
CBYTE menu_buffer[2048];

// uc_orig: grabbing_key (fallen/Source/frontend.cpp)
BOOL grabbing_key = 0;
// uc_orig: grabbing_pad (fallen/Source/frontend.cpp)
BOOL grabbing_pad = 0;
// uc_orig: m_bMovingPanel (fallen/Source/frontend.cpp)
BOOL m_bMovingPanel = UC_FALSE;

// uc_orig: kibble (fallen/Source/frontend.cpp)
Kibble kibble[512];
// uc_orig: kibble_off (fallen/Source/frontend.cpp)
UBYTE kibble_off[512];

// uc_orig: fade_state (fallen/Source/frontend.cpp)
SLONG fade_state = 0;
// uc_orig: fade_mode (fallen/Source/frontend.cpp)
UBYTE fade_mode = 1;
// uc_orig: fade_rgb (fallen/Source/frontend.cpp)
SLONG fade_rgb = 0x000000;
// uc_orig: menu_mode_queued (fallen/Source/frontend.cpp)
SBYTE menu_mode_queued = 0;
// uc_orig: menu_theme (fallen/Source/frontend.cpp)
UBYTE menu_theme = 1;
// uc_orig: menu_thrash (fallen/Source/frontend.cpp)
UBYTE menu_thrash = 0;

// uc_orig: districts (fallen/Source/frontend.cpp)
SWORD districts[40][3];
// uc_orig: district_count (fallen/Source/frontend.cpp)
SWORD district_count = 0;
// uc_orig: district_selected (fallen/Source/frontend.cpp)
SWORD district_selected = 0;
// uc_orig: district_flash (fallen/Source/frontend.cpp)
SWORD district_flash = 0;
// uc_orig: district_valid (fallen/Source/frontend.cpp)
UBYTE district_valid[40];

// uc_orig: mission_count (fallen/Source/frontend.cpp)
SWORD mission_count = 0;
// uc_orig: mission_selected (fallen/Source/frontend.cpp)
SWORD mission_selected = 0;
// uc_orig: mission_hierarchy (fallen/Source/frontend.cpp)
UBYTE mission_hierarchy[60];
// uc_orig: mission_cache (fallen/Source/frontend.cpp)
MissionCache mission_cache[60];

// uc_orig: complete_point (fallen/Source/frontend.cpp)
UBYTE complete_point = 0;
// uc_orig: mission_launch (fallen/Source/frontend.cpp)
UBYTE mission_launch = 0;
// uc_orig: previous_mission_launch (fallen/Source/frontend.cpp)
UBYTE previous_mission_launch = 0;
// uc_orig: cheating (fallen/Source/frontend.cpp)
BOOL cheating = 0;
// uc_orig: MidX (fallen/Source/frontend.cpp)
SLONG MidX = 0;
// uc_orig: MidY (fallen/Source/frontend.cpp)
SLONG MidY = 0;
// uc_orig: ScaleX (fallen/Source/frontend.cpp)
float ScaleX = 0.0f;
// uc_orig: ScaleY (fallen/Source/frontend.cpp)
float ScaleY = 0.0f;
// uc_orig: AllowSave (fallen/Source/frontend.cpp)
UBYTE AllowSave = 0;
// uc_orig: CurrentVidMode (fallen/Source/frontend.cpp)
SLONG CurrentVidMode = 0;
// uc_orig: CurrentBitDepth (fallen/Source/frontend.cpp)
UBYTE CurrentBitDepth = 16;
// uc_orig: save_slot (fallen/Source/frontend.cpp)
UBYTE save_slot = 0;
// uc_orig: bonus_this_turn (fallen/Source/frontend.cpp)
UBYTE bonus_this_turn = 0;
// uc_orig: bonus_state (fallen/Source/frontend.cpp)
UBYTE bonus_state = 0;

// uc_orig: the_script_file (fallen/Source/frontend.cpp)
// MISSION_SCRIPT is #defined to this buffer in frontend.cpp.
CBYTE the_script_file[MAX_PATH];

// uc_orig: IsEnglish (fallen/Headers/frontend.h)
UBYTE IsEnglish = 0;

// uc_orig: pcSpeechLanguageDir (fallen/Source/frontend.cpp)
char* pcSpeechLanguageDir = "talk2/";

// uc_orig: dwAutoPlayFMVTimeout (fallen/Source/frontend.cpp)
DWORD dwAutoPlayFMVTimeout = 0;

// uc_orig: GammaIndex (fallen/Source/frontend.cpp)
SLONG GammaIndex = 0;

// uc_orig: m_bGoIntoSaveScreen (fallen/Source/frontend.cpp)
bool m_bGoIntoSaveScreen = UC_FALSE;

// uc_orig: bCanChangeJoypadButtons (fallen/Source/frontend.cpp)
BOOL bCanChangeJoypadButtons = UC_FALSE;

// ---- DirectDraw background surfaces ----------------------------------------

// uc_orig: screenfull_back (fallen/Source/frontend.cpp)
GEScreenSurface screenfull_back = NULL;
// uc_orig: screenfull_map (fallen/Source/frontend.cpp)
GEScreenSurface screenfull_map = NULL;
// uc_orig: screenfull_config (fallen/Source/frontend.cpp)
GEScreenSurface screenfull_config = NULL;
// uc_orig: screenfull_brief (fallen/Source/frontend.cpp)
GEScreenSurface screenfull_brief = NULL;
// uc_orig: screenfull (fallen/Source/frontend.cpp)
// The current surface used for swipes/fades.
GEScreenSurface screenfull = NULL;

// uc_orig: lpFRONTEND_show_xition_LastBlit (fallen/Source/frontend.cpp)
GEScreenSurface lpFRONTEND_show_xition_LastBlit = NULL;

// ---- Savegame helpers -------------------------------------------------------

// uc_orig: best_found (fallen/Source/frontend.cpp)
UBYTE best_found[50][4];

// ---- Mission briefing audio -------------------------------------------------

// uc_orig: brief_wav (fallen/Source/frontend.cpp)
CBYTE* brief_wav[] = {
    "none", // 0
    "none", // 1
    "none", // 2
    "none", // 3
    "none", // 4
    "none", // 5
    "policem1.wav", // 6
    "policem.wav", // 7
    "policem.wav", // 8
    "policem2.wav", // 9
    "none", // 10
    "none", // 11
    "policem3.wav", // 12
    "none", // 13
    "policem4.wav", // 14
    "policem5.wav", // 15
    "policem6.wav", // 16
    "policem7.wav", // 17
    "policem8.wav", // 18
    "policem9.wav", // 19
    "policem10.wav", // 20
    "policem11.wav", // 21
    "policem12.wav", // 22
    "policem13.wav", // 23
    "policem14.wav", // 24
    "policem15.wav", // 25
    "policem16.wav", // 26
    "policem17.wav", // 27
    "policem18.wav", // 28
    "roperm19.wav", // 29
    "roperm20.wav", // 30
    "roperm21.wav", // 31
    "roperm23.wav", // 33 (index 32; original skipped 32)
    "roperm24.wav", // 34
    ""
};

// ---- Mission-specific flags set by FRONTEND_loop ---------------------------

// uc_orig: this_level_has_the_balrog (fallen/Source/frontend.cpp)
UBYTE this_level_has_the_balrog = 0;
// uc_orig: this_level_has_bane (fallen/Source/frontend.cpp)
UBYTE this_level_has_bane = 0;
// uc_orig: is_semtex (fallen/Source/frontend.cpp)
UBYTE is_semtex = 0;
