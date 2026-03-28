#ifndef UI_FRONTEND_FRONTEND_GLOBALS_H
#define UI_FRONTEND_FRONTEND_GLOBALS_H

#include "ui/frontend/frontend.h"

// ---- Script buffer ---------------------------------------------------------

// uc_orig: loaded_in_script (fallen/Source/frontend.cpp)
extern CBYTE loaded_in_script[SCRIPT_MEMORY];
// uc_orig: loaded_in_script_read_upto (fallen/Source/frontend.cpp)
extern CBYTE* loaded_in_script_read_upto;

// ---- Raw menu data table ---------------------------------------------------

// uc_orig: raw_menu_data (fallen/Source/frontend.cpp)
extern RawMenuData raw_menu_data[];
// uc_orig: suggest_order (fallen/Source/frontend.cpp)
extern CBYTE* suggest_order[];

// ---- Menu choice strings ---------------------------------------------------

// uc_orig: menu_choice_yesno (fallen/Source/frontend.cpp)
extern CBYTE menu_choice_yesno[20];
// uc_orig: menu_choice_scanner (fallen/Source/frontend.cpp)
extern CBYTE menu_choice_scanner[255];

// ---- Background image name arrays ------------------------------------------

// uc_orig: menu_back_names (fallen/Source/frontend.cpp)
extern CBYTE* menu_back_names[];
// uc_orig: menu_map_names (fallen/Source/frontend.cpp)
extern CBYTE* menu_map_names[];
// uc_orig: menu_brief_names (fallen/Source/frontend.cpp)
extern CBYTE* menu_brief_names[];
// uc_orig: menu_config_names (fallen/Source/frontend.cpp)
extern CBYTE* menu_config_names[];

// uc_orig: frontend_fonttable (fallen/Source/frontend.cpp)
extern CBYTE frontend_fonttable[];

// uc_orig: FRONTEND_leaf_colours (fallen/Source/frontend.cpp)
extern ULONG FRONTEND_leaf_colours[4];

// ---- Active menu state -----------------------------------------------------

// uc_orig: menu_data (fallen/Source/frontend.cpp)
extern MenuData menu_data[50];
// uc_orig: menu_state (fallen/Source/frontend.cpp)
extern MenuState menu_state;
// uc_orig: menu_buffer (fallen/Source/frontend.cpp)
extern CBYTE menu_buffer[2048];

// uc_orig: grabbing_key (fallen/Source/frontend.cpp)
extern BOOL grabbing_key;
// uc_orig: grabbing_pad (fallen/Source/frontend.cpp)
extern BOOL grabbing_pad;
// uc_orig: m_bMovingPanel (fallen/Source/frontend.cpp)
extern BOOL m_bMovingPanel;

// uc_orig: kibble (fallen/Source/frontend.cpp)
extern Kibble kibble[512];
// uc_orig: kibble_off (fallen/Source/frontend.cpp)
extern UBYTE kibble_off[512];

// uc_orig: fade_state (fallen/Source/frontend.cpp)
extern SLONG fade_state;
// uc_orig: fade_mode (fallen/Source/frontend.cpp)
extern UBYTE fade_mode;
// uc_orig: fade_rgb (fallen/Source/frontend.cpp)
extern SLONG fade_rgb;
// uc_orig: menu_mode_queued (fallen/Source/frontend.cpp)
extern SBYTE menu_mode_queued;
// uc_orig: menu_theme (fallen/Source/frontend.cpp)
extern UBYTE menu_theme;
// uc_orig: menu_thrash (fallen/Source/frontend.cpp)
extern UBYTE menu_thrash;

// uc_orig: districts (fallen/Source/frontend.cpp)
extern SWORD districts[40][3];
// uc_orig: district_count (fallen/Source/frontend.cpp)
extern SWORD district_count;
// uc_orig: district_selected (fallen/Source/frontend.cpp)
extern SWORD district_selected;
// uc_orig: district_flash (fallen/Source/frontend.cpp)
extern SWORD district_flash;
// uc_orig: district_valid (fallen/Source/frontend.cpp)
extern UBYTE district_valid[40];

// uc_orig: mission_count (fallen/Source/frontend.cpp)
extern SWORD mission_count;
// uc_orig: mission_selected (fallen/Source/frontend.cpp)
extern SWORD mission_selected;
// uc_orig: mission_hierarchy (fallen/Source/frontend.cpp)
extern UBYTE mission_hierarchy[60];
// uc_orig: mission_cache (fallen/Source/frontend.cpp)
extern MissionCache mission_cache[60];

// uc_orig: complete_point (fallen/Source/frontend.cpp)
extern UBYTE complete_point;
// uc_orig: mission_launch (fallen/Source/frontend.cpp)
extern UBYTE mission_launch;
// uc_orig: previous_mission_launch (fallen/Source/frontend.cpp)
extern UBYTE previous_mission_launch;
// uc_orig: cheating (fallen/Source/frontend.cpp)
extern BOOL cheating;
// uc_orig: MidX (fallen/Source/frontend.cpp)
extern SLONG MidX;
// uc_orig: MidY (fallen/Source/frontend.cpp)
extern SLONG MidY;
// uc_orig: ScaleX (fallen/Source/frontend.cpp)
extern float ScaleX;
// uc_orig: ScaleY (fallen/Source/frontend.cpp)
extern float ScaleY;
// uc_orig: AllowSave (fallen/Source/frontend.cpp)
extern UBYTE AllowSave;
// uc_orig: CurrentVidMode (fallen/Source/frontend.cpp)
extern SLONG CurrentVidMode;
// uc_orig: CurrentBitDepth (fallen/Source/frontend.cpp)
extern UBYTE CurrentBitDepth;
// uc_orig: save_slot (fallen/Source/frontend.cpp)
extern UBYTE save_slot;
// uc_orig: bonus_this_turn (fallen/Source/frontend.cpp)
extern UBYTE bonus_this_turn;
// uc_orig: bonus_state (fallen/Source/frontend.cpp)
extern UBYTE bonus_state;

// uc_orig: the_script_file (fallen/Source/frontend.cpp)
extern CBYTE the_script_file[MAX_PATH];

// uc_orig: MISSION_SCRIPT (fallen/Source/frontend.cpp)
// Macro aliasing the_script_file so existing code using MISSION_SCRIPT still compiles.
#define MISSION_SCRIPT the_script_file

// uc_orig: IsEnglish (fallen/Headers/frontend.h)
extern UBYTE IsEnglish;

// uc_orig: pcSpeechLanguageDir (fallen/Source/frontend.cpp)
extern char* pcSpeechLanguageDir;

// uc_orig: dwAutoPlayFMVTimeout (fallen/Source/frontend.cpp)
extern DWORD dwAutoPlayFMVTimeout;

// uc_orig: GammaIndex (fallen/Source/frontend.cpp)
extern SLONG GammaIndex;

// uc_orig: m_bGoIntoSaveScreen (fallen/Source/frontend.cpp)
extern bool m_bGoIntoSaveScreen;

// uc_orig: bCanChangeJoypadButtons (fallen/Source/frontend.cpp)
extern BOOL bCanChangeJoypadButtons;

// ---- Background screen surfaces ----------------------------------------

// uc_orig: screenfull_back (fallen/Source/frontend.cpp)
extern GEScreenSurface screenfull_back;
// uc_orig: screenfull_map (fallen/Source/frontend.cpp)
extern GEScreenSurface screenfull_map;
// uc_orig: screenfull_config (fallen/Source/frontend.cpp)
extern GEScreenSurface screenfull_config;
// uc_orig: screenfull_brief (fallen/Source/frontend.cpp)
extern GEScreenSurface screenfull_brief;
// uc_orig: screenfull (fallen/Source/frontend.cpp)
extern GEScreenSurface screenfull;

// uc_orig: lpFRONTEND_show_xition_LastBlit (fallen/Source/frontend.cpp)
extern GEScreenSurface lpFRONTEND_show_xition_LastBlit;

// ---- Savegame helpers -------------------------------------------------------

// uc_orig: best_found (fallen/Source/frontend.cpp)
// Per-mission best score array: [mission_id][4] bytes of score data.
extern UBYTE best_found[50][4];

// ---- Mission briefing audio -------------------------------------------------

// uc_orig: brief_wav (fallen/Source/frontend.cpp)
// Per-mission speech .wav filenames played during the mission briefing screen.
// Index = mission ObjID; "none" means no speech for that mission.
extern CBYTE* brief_wav[];

// ---- Mission-specific flags set by FRONTEND_loop ---------------------------

// uc_orig: this_level_has_the_balrog (fallen/Source/frontend.cpp)
// Set to UC_TRUE only for Baalrog3.ucm (the boss level).
extern UBYTE this_level_has_the_balrog;
// uc_orig: this_level_has_bane (fallen/Source/frontend.cpp)
// Set to UC_TRUE only for Finale1.ucm.
extern UBYTE this_level_has_bane;
// uc_orig: is_semtex (fallen/Source/frontend.cpp)
// Set to 1 for skymiss2.ucm.
extern UBYTE is_semtex;

#endif // UI_FRONTEND_FRONTEND_GLOBALS_H
