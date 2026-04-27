#ifndef UI_HUD_PANEL_GLOBALS_H
#define UI_HUD_PANEL_GLOBALS_H

// Global state for the HUD panel system.

#include <stdint.h>
#include "engine/graphics/pipeline/poly.h"

// uc_orig: PANEL_scanner_poo (fallen/DDEngine/Source/panel.cpp)
extern unsigned char PANEL_scanner_poo;

// Screen position of the panel's bottom-left corner.
// uc_orig: m_iPanelXPos (fallen/DDEngine/Source/panel.cpp)
extern int m_iPanelXPos;
// uc_orig: m_iPanelYPos (fallen/DDEngine/Source/panel.cpp)
extern int m_iPanelYPos;

// Timer display buffer (PANEL_draw_timer queues here; PANEL_draw_buffered flushes).
typedef struct
{
    float time;
    float x;
    float y;
} PANEL_Store;

#define PANEL_MAX_STORES 8

// uc_orig: PANEL_store (fallen/DDEngine/Source/panel.cpp)
extern PANEL_Store PANEL_store[PANEL_MAX_STORES];
// uc_orig: PANEL_store_upto (fallen/DDEngine/Source/panel.cpp)
extern int PANEL_store_upto;

// Floating text message (NPC speech / radio chatter) circular queue.
#define PANEL_TEXT_MAX_LENGTH 300
typedef struct
{
    struct Thing* who; // NULL = computer/radio message
    char text[PANEL_TEXT_MAX_LENGTH + 2];
    int delay;  // 0 = unused
    int turns;  // number of game turns this has been alive
} PANEL_Text;

#define PANEL_MAX_TEXTS 8

// uc_orig: PANEL_text (fallen/DDEngine/Source/panel.cpp)
extern PANEL_Text PANEL_text[PANEL_MAX_TEXTS];
// uc_orig: PANEL_text_head (fallen/DDEngine/Source/panel.cpp)
extern int PANEL_text_head;
// uc_orig: PANEL_text_tail (fallen/DDEngine/Source/panel.cpp)
extern int PANEL_text_tail;
// uc_orig: PANEL_text_tick (fallen/DDEngine/Source/panel.cpp)
extern uint64_t PANEL_text_tick;

// Face size constants for PANEL_new_face().
// uc_orig: PANEL_FACE_LARGE (fallen/DDEngine/Source/panel.cpp)
#define PANEL_FACE_LARGE 1
// uc_orig: PANEL_FACE_SMALL (fallen/DDEngine/Source/panel.cpp)
#define PANEL_FACE_SMALL 2

// Help message (set by PANEL_new_help_message, displayed by PANEL_help_message_do).
// uc_orig: PANEL_help_message (fallen/DDEngine/Source/panel.cpp)
extern char PANEL_help_message[256];
// uc_orig: PANEL_help_timer (fallen/DDEngine/Source/panel.cpp)
extern SLONG PANEL_help_timer;

// Widescreen cut-scene: current top/bottom speaker thing indices and text.
// uc_orig: PANEL_wide_top_person (fallen/DDEngine/Source/panel.cpp)
extern unsigned short PANEL_wide_top_person;
// uc_orig: PANEL_wide_bot_person (fallen/DDEngine/Source/panel.cpp)
extern unsigned short PANEL_wide_bot_person;
// uc_orig: PANEL_wide_top_is_talking (fallen/DDEngine/Source/panel.cpp)
extern SLONG PANEL_wide_top_is_talking;
// uc_orig: PANEL_wide_text (fallen/DDEngine/Source/panel.cpp)
extern char PANEL_wide_text[256];

// Beacon colour table (one colour per beacon slot, cycled by beacon index mod 12).
// uc_orig: PANEL_MAX_BEACON_COLOURS (fallen/DDEngine/Source/panel.cpp)
#define PANEL_MAX_BEACON_COLOURS 12
// uc_orig: PANEL_beacon_colour (fallen/DDEngine/Source/panel.cpp)
extern ULONG PANEL_beacon_colour[PANEL_MAX_BEACON_COLOURS];

// Fade-out effect state: timestamp when PANEL_fadeout_start() was called (0 = inactive).
// uc_orig: PANEL_fadeout_time (fallen/DDEngine/Source/panel.cpp)
extern int64_t PANEL_fadeout_time;

// Zoom/spin multipliers for the fade-out effect texture transform.
// uc_orig: angle_mul (fallen/DDEngine/Source/panel.cpp)
extern float angle_mul;
// uc_orig: zoom_mul (fallen/DDEngine/Source/panel.cpp)
extern float zoom_mul;

// Road sign flash state.
// uc_orig: PANEL_sign_which (fallen/DDEngine/Source/panel.cpp)
extern SLONG PANEL_sign_which;
// uc_orig: PANEL_sign_flip (fallen/DDEngine/Source/panel.cpp)
extern SLONG PANEL_sign_flip;
// uc_orig: PANEL_sign_time (fallen/DDEngine/Source/panel.cpp)
extern uint64_t PANEL_sign_time;

// Info message (bottom of panel, 2-second display).
// uc_orig: PANEL_info_message (fallen/DDEngine/Source/panel.cpp)
extern char PANEL_info_message[512];
// uc_orig: PANEL_info_time (fallen/DDEngine/Source/panel.cpp)
extern uint64_t PANEL_info_time;

// Screensaver state (bouncing logo overlay when the game is idle).
// uc_orig: bScreensaverEnabled (fallen/DDEngine/Source/panel.cpp)
extern bool bScreensaverEnabled;
// uc_orig: iScreenSaverDarkness (fallen/DDEngine/Source/panel.cpp)
extern int iScreenSaverDarkness;
// uc_orig: iScreensaverXPos (fallen/DDEngine/Source/panel.cpp)
extern int iScreensaverXPos;
// uc_orig: iScreensaverYPos (fallen/DDEngine/Source/panel.cpp)
extern int iScreensaverYPos;
// uc_orig: iScreensaverXInc (fallen/DDEngine/Source/panel.cpp)
extern int iScreensaverXInc;
// uc_orig: iScreensaverYInc (fallen/DDEngine/Source/panel.cpp)
extern int iScreensaverYInc;
// uc_orig: iScreensaverAngle (fallen/DDEngine/Source/panel.cpp)
extern int iScreensaverAngle;
// uc_orig: iScreensaverAngleInc (fallen/DDEngine/Source/panel.cpp)
extern int iScreensaverAngleInc;
// uc_orig: dwPseudorandomSeed (fallen/DDEngine/Source/panel.cpp)
extern ULONG dwPseudorandomSeed;

#endif // UI_HUD_PANEL_GLOBALS_H
