#include "ui/hud/panel_globals.h"
#include "ui/hud/panel.h"
// uc_orig: PANEL_scanner_poo (fallen/DDEngine/Source/panel.cpp)
unsigned char PANEL_scanner_poo = 0;

// uc_orig: m_iPanelXPos (fallen/DDEngine/Source/panel.cpp)
int m_iPanelXPos = 0;
// uc_orig: m_iPanelYPos (fallen/DDEngine/Source/panel.cpp)
int m_iPanelYPos = 480;

// uc_orig: PANEL_store (fallen/DDEngine/Source/panel.cpp)
PANEL_Store PANEL_store[PANEL_MAX_STORES];
// uc_orig: PANEL_store_upto (fallen/DDEngine/Source/panel.cpp)
int PANEL_store_upto = 0;

// uc_orig: PANEL_text (fallen/DDEngine/Source/panel.cpp)
PANEL_Text PANEL_text[PANEL_MAX_TEXTS];
// uc_orig: PANEL_text_head (fallen/DDEngine/Source/panel.cpp)
int PANEL_text_head = 0;
// uc_orig: PANEL_text_tail (fallen/DDEngine/Source/panel.cpp)
int PANEL_text_tail = 0;
// uc_orig: PANEL_text_tick (fallen/DDEngine/Source/panel.cpp)
uint64_t PANEL_text_tick = 0;

// uc_orig: PANEL_help_message (fallen/DDEngine/Source/panel.cpp)
char PANEL_help_message[256] = {};
// uc_orig: PANEL_help_timer (fallen/DDEngine/Source/panel.cpp)
SLONG PANEL_help_timer = 0;

// uc_orig: PANEL_wide_top_person (fallen/DDEngine/Source/panel.cpp)
unsigned short PANEL_wide_top_person = 0;
// uc_orig: PANEL_wide_bot_person (fallen/DDEngine/Source/panel.cpp)
unsigned short PANEL_wide_bot_person = 0;
// uc_orig: PANEL_wide_top_is_talking (fallen/DDEngine/Source/panel.cpp)
SLONG PANEL_wide_top_is_talking = 0;
// uc_orig: PANEL_wide_text (fallen/DDEngine/Source/panel.cpp)
char PANEL_wide_text[256] = {};

// uc_orig: PANEL_beacon_colour (fallen/DDEngine/Source/panel.cpp)
ULONG PANEL_beacon_colour[PANEL_MAX_BEACON_COLOURS] = {
    0xffff00,
    0xccff00,
    0x4488ff,
    0xff0000,
    0x00ff00,
    0x0000ff,
    0xff4400,
    0xffffff,
    0xff00ff,
    0xaaffaa,
    0x00ffff,
    0xff4488
};

// uc_orig: PANEL_fadeout_time (fallen/DDEngine/Source/panel.cpp)
int64_t PANEL_fadeout_time = 0;

// uc_orig: angle_mul (fallen/DDEngine/Source/panel.cpp)
float angle_mul = 0.004F;
// uc_orig: zoom_mul (fallen/DDEngine/Source/panel.cpp)
float zoom_mul = 0.500F;

// uc_orig: PANEL_sign_which (fallen/DDEngine/Source/panel.cpp)
SLONG PANEL_sign_which = 0;
// uc_orig: PANEL_sign_flip (fallen/DDEngine/Source/panel.cpp)
SLONG PANEL_sign_flip = 0;
// uc_orig: PANEL_sign_time (fallen/DDEngine/Source/panel.cpp)
uint64_t PANEL_sign_time = 0;

// uc_orig: PANEL_info_message (fallen/DDEngine/Source/panel.cpp)
char PANEL_info_message[512] = {};
// uc_orig: PANEL_info_time (fallen/DDEngine/Source/panel.cpp)
uint64_t PANEL_info_time = 0;

// Texture atlas coordinates for each PANEL_LSPRITE_* icon.
// All coordinates are in 256x256 atlas space, converted to 0..1 UVs.
// uc_orig: PANEL_lsprite (fallen/DDEngine/Source/panel.cpp)
#define PLS(p, a, b, c, d) { p, float(a) / 256.0F, float(b) / 256.F, float(c) / 256.F, float(d) / 256.F }
PANEL_Lsprite PANEL_lsprite[PANEL_LSPRITE_NUMBER] = {
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 0, 90, 212, 256),     // display
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 70, 52, 141, 89),     // AK
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 0, 52, 69, 88),       // Shotgun
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 175, 0, 246, 50),     // LoGear
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 104, 0, 174, 50),     // HiGear
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 70, 0, 104, 46),      // Grenade
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 224, 102, 242, 114),  // Arrow
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 226, 76, 246, 103),   // ?
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 230, 176, 238, 184),  // Dot
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 0, 0, 69, 50),        // Pistol
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 218, 128, 250, 160),  // TxtBox
    PLS(POLY_PAGE_LASTPANEL2_ALPHA, 0, 124, 71, 168),    // BBB
    PLS(POLY_PAGE_LASTPANEL2_ALPHA, 126, 124, 162, 168), // Fist
    PLS(POLY_PAGE_LASTPANEL2_ALPHA, 72, 123, 120, 161),  // splosive
    PLS(POLY_PAGE_LASTPANEL_ALPHA, 141, 62, 212, 84),    // knife
};
#undef PLS

// Screensaver state — bouncing logo overlay shown when the game is left idle.
// uc_orig: bScreensaverEnabled (fallen/DDEngine/Source/panel.cpp)
bool bScreensaverEnabled = false;
// uc_orig: iScreenSaverDarkness (fallen/DDEngine/Source/panel.cpp)
int iScreenSaverDarkness = 0;
// uc_orig: iScreensaverXPos (fallen/DDEngine/Source/panel.cpp)
int iScreensaverXPos = 320;
// uc_orig: iScreensaverYPos (fallen/DDEngine/Source/panel.cpp)
int iScreensaverYPos = 240;
// uc_orig: iScreensaverXInc (fallen/DDEngine/Source/panel.cpp)
int iScreensaverXInc = 4;
// uc_orig: iScreensaverYInc (fallen/DDEngine/Source/panel.cpp)
int iScreensaverYInc = 4;
// uc_orig: iScreensaverAngle (fallen/DDEngine/Source/panel.cpp)
int iScreensaverAngle = 0;
// uc_orig: iScreensaverAngleInc (fallen/DDEngine/Source/panel.cpp)
int iScreensaverAngleInc = 0x2ff;
// uc_orig: dwPseudorandomSeed (fallen/DDEngine/Source/panel.cpp)
ULONG dwPseudorandomSeed = 0;
