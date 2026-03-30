#include "ui/hud/panel_globals.h"
#include "ui/hud/panel.h"
#include "engine/graphics/pipeline/poly.h"

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

#define PIC(uv) ((uv) / 256.0F)
#define PICALL1(a, b, c, d) \
    {                       \
        PIC(a), PIC(b), PIC(c), PIC(d), 1 \
    }
#define PICALL0(a, b, c, d) \
    {                       \
        PIC(a), PIC(b), PIC(c), PIC(d), 0 \
    }

// uc_orig: PANEL_ic (fallen/DDEngine/Source/panel.cpp)
PANEL_Ic PANEL_ic[PANEL_IC_NUMBER] = {
    { PIC(0), PIC(0), PIC(99), PIC(81), 0 },       // BACKBOX
    { PIC(1), PIC(83), PIC(191), PIC(137), 0 },     // AK47
    { PIC(100), PIC(1), PIC(140), PIC(64), 0 },     // GRENADE
    { PIC(141), PIC(1), PIC(146), PIC(13), 0 },     // AK47_AMMO_ONE
    { PIC(158), PIC(1), PIC(208), PIC(13), 0 },     // AK47_AMMO_GROUP
    { PIC(142), PIC(15), PIC(221), PIC(75), 0 },    // PISTOL
    { PIC(0), PIC(144), PIC(55), PIC(255), 0 },     // DARCI_OUTLINE
    { PIC(55), PIC(144), PIC(109), PIC(256), 0 },   // ROPER_OUTLINE
    { PIC(110), PIC(149), PIC(160), PIC(255), 0 },  // DARCI
    { PIC(160), PIC(148), PIC(210), PIC(255), 0 },  // ROPER
    { PIC(210), PIC(95), PIC(254), PIC(256), 0 },   // SHOTGUN

    PICALL1(1, 1, 75, 50),    // ROPER_HAND
    PICALL1(75, 5, 151, 46),  // DARCI_HAND
    PICALL1(1, 55, 97, 70),   // KNIFE
    PICALL1(5, 81, 45, 93),   // SHOTGUN_AMMO_GROUP
    PICALL1(48, 81, 53, 93),  // SHOTGUN_AMMO_ONE
    PICALL1(16, 95, 66, 104), // PISTOL_AMMO_GROUP
    PICALL1(5, 95, 10, 104),  // PISTOL_AMMO_ONE
    PICALL1(15, 108, 39, 122),// BIG_AMMO_GROUP
    PICALL1(5, 108, 13, 122), // BIG_AMMO_ONE
    PICALL1(18, 123, 54, 141),// GRENADE_AMMO_GROUP
    PICALL1(4, 123, 16, 141), // GRENADE_AMMO_ONE

    PICALL0(241, 77, 255, 94), // DIGIT_0
    PICALL0(225, 3, 240, 20),  // DIGIT_1
    PICALL0(225, 21, 240, 38), // DIGIT_2
    PICALL0(225, 40, 240, 57), // DIGIT_3
    PICALL0(240, 3, 256, 20),  // DIGIT_4
    PICALL0(241, 21, 255, 38), // DIGIT_5
    PICALL0(241, 40, 255, 57), // DIGIT_6
    PICALL0(226, 59, 240, 76), // DIGIT_7
    PICALL0(241, 59, 255, 76), // DIGIT_8
    PICALL0(226, 77, 240, 94), // DIGIT_9

    PICALL1(3, 141, 170, 164),    // BASEBALLBAT
    PICALL1(11, 176, 44, 223),    // EXPLOSIVES
    PICALL1(208, 187, 236, 214),  // GRIM_REAPER
    PICALL1(190, 125, 221, 256),  // DANGER

    PICALL1(224, 108, 242, 172),  // BUBBLE_START
    PICALL1(242, 108, 243, 172),  // BUBBLE_MIDDLE
    PICALL1(243, 108, 256, 172),  // BUBBLE_END

    PICALL1(140, 197, 154, 211),  // DOT

    PICALL1(79, 225, 100, 252),   // CLIP_PISTOL
    PICALL1(121, 227, 143, 252),  // CLIP_SHOTGUN
    PICALL1(152, 225, 174, 250),  // CLIP_AK47

    PICALL1(205, 215, 254, 256),  // GEAR_BOX
    PICALL1(178, 225, 199, 249),  // GEAR_LOW
    PICALL1(240, 182, 254, 205),  // GEAR_HIGH
};

// uc_orig: PANEL_page (fallen/DDEngine/Source/panel.cpp)
unsigned short PANEL_page[4][PANEL_PAGE_NUMBER] = {
    { POLY_PAGE_IC_NORMAL,
        POLY_PAGE_IC_ALPHA,
        POLY_PAGE_IC_ADDITIVE,
        POLY_PAGE_IC_ALPHA_END },

    { POLY_PAGE_IC2_NORMAL,
        POLY_PAGE_IC2_ALPHA,
        POLY_PAGE_IC2_ADDITIVE,
        POLY_PAGE_IC2_ALPHA_END }
};

// uc_orig: PANEL_beat (fallen/DDEngine/Source/panel.cpp)
PANEL_Beat PANEL_beat[2][PANEL_NUM_BEATS];
// uc_orig: PANEL_beat_head (fallen/DDEngine/Source/panel.cpp)
int PANEL_beat_head[2];
// uc_orig: PANEL_beat_tick (fallen/DDEngine/Source/panel.cpp)
unsigned int PANEL_beat_tick[2];
// uc_orig: PANEL_beat_last_ammo (fallen/DDEngine/Source/panel.cpp)
int PANEL_beat_last_ammo[2];
// uc_orig: PANEL_beat_last_specialtype (fallen/DDEngine/Source/panel.cpp)
int PANEL_beat_last_specialtype[2];
// uc_orig: PANEL_beat_x (fallen/DDEngine/Source/panel.cpp)
float PANEL_beat_x[2];

// uc_orig: PANEL_ammo (fallen/DDEngine/Source/panel.cpp)
PANEL_Ammo PANEL_ammo[PANEL_AMMO_NUMBER] = {
    { 5.0F, 9.0F, 10, PANEL_IC_PISTOL_AMMO_GROUP, PANEL_IC_PISTOL_AMMO_ONE },
    { 5.0F, 12.0F, 8, PANEL_IC_SHOTGUN_AMMO_GROUP, PANEL_IC_SHOTGUN_AMMO_ONE },
    { 5.0F, 12.0F, 10, PANEL_IC_AK47_AMMO_GROUP, PANEL_IC_AK47_AMMO_ONE },
    { 12.0F, 18.0F, 3, PANEL_IC_GRENADE_AMMO_GROUP, PANEL_IC_GRENADE_AMMO_ONE },
};

// uc_orig: PANEL_toss (fallen/DDEngine/Source/panel.cpp)
PANEL_Toss PANEL_toss[PANEL_MAX_TOSSES];
// uc_orig: PANEL_toss_last (fallen/DDEngine/Source/panel.cpp)
int PANEL_toss_last = 0;
// uc_orig: PANEL_toss_tick (fallen/DDEngine/Source/panel.cpp)
uint64_t PANEL_toss_tick = 0;

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
long PANEL_help_timer = 0;

// uc_orig: PANEL_wide_top_person (fallen/DDEngine/Source/panel.cpp)
unsigned short PANEL_wide_top_person = 0;
// uc_orig: PANEL_wide_bot_person (fallen/DDEngine/Source/panel.cpp)
unsigned short PANEL_wide_bot_person = 0;
// uc_orig: PANEL_wide_top_is_talking (fallen/DDEngine/Source/panel.cpp)
long PANEL_wide_top_is_talking = 0;
// uc_orig: PANEL_wide_text (fallen/DDEngine/Source/panel.cpp)
char PANEL_wide_text[256] = {};

// uc_orig: PANEL_beacon_colour (fallen/DDEngine/Source/panel.cpp)
unsigned long PANEL_beacon_colour[PANEL_MAX_BEACON_COLOURS] = {
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
long PANEL_sign_which = 0;
// uc_orig: PANEL_sign_flip (fallen/DDEngine/Source/panel.cpp)
long PANEL_sign_flip = 0;
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
unsigned long dwPseudorandomSeed = 0;
