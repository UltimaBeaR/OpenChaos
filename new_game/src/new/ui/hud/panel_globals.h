#ifndef UI_HUD_PANEL_GLOBALS_H
#define UI_HUD_PANEL_GLOBALS_H

// Global state for the HUD panel system.

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

// HUD icon texture coordinates on the IC sprite pages.
typedef struct
{
    float u1;
    float v1;
    float u2;
    float v2;
    int page; // 0 = first IC page, 1 = second IC page
} PANEL_Ic;

// Icon indices into PANEL_ic[].
#define PANEL_IC_BACKBOX 0
#define PANEL_IC_AK47 1
#define PANEL_IC_GRENADE 2
#define PANEL_IC_AK47_AMMO_ONE 3
#define PANEL_IC_AK47_AMMO_GROUP 4
#define PANEL_IC_PISTOL 5
#define PANEL_IC_DARCI_OUTLINE 6
#define PANEL_IC_ROPER_OUTLINE 7
#define PANEL_IC_DARCI 8
#define PANEL_IC_ROPER 9
#define PANEL_IC_SHOTGUN 10
#define PANEL_IC_ROPER_HAND 11
#define PANEL_IC_DARCI_HAND 12
#define PANEL_IC_KNIFE 13
#define PANEL_IC_SHOTGUN_AMMO_GROUP 14
#define PANEL_IC_SHOTGUN_AMMO_ONE 15
#define PANEL_IC_PISTOL_AMMO_GROUP 16
#define PANEL_IC_PISTOL_AMMO_ONE 17
#define PANEL_IC_BIG_AMMO_GROUP 18
#define PANEL_IC_BIG_AMMO_ONE 19
#define PANEL_IC_GRENADE_AMMO_GROUP 20
#define PANEL_IC_GRENADE_AMMO_ONE 21
#define PANEL_IC_DIGIT_0 22
#define PANEL_IC_DIGIT_1 23
#define PANEL_IC_DIGIT_2 24
#define PANEL_IC_DIGIT_3 25
#define PANEL_IC_DIGIT_4 26
#define PANEL_IC_DIGIT_5 27
#define PANEL_IC_DIGIT_6 28
#define PANEL_IC_DIGIT_7 29
#define PANEL_IC_DIGIT_8 30
#define PANEL_IC_DIGIT_9 31
#define PANEL_IC_BASEBALLBAT 32
#define PANEL_IC_EXPLOSIVES 33
#define PANEL_IC_GRIM_REAPER 34
#define PANEL_IC_DANGER 35
#define PANEL_IC_BUBBLE_START 36
#define PANEL_IC_BUBBLE_MIDDLE 37
#define PANEL_IC_BUBBLE_END 38
#define PANEL_IC_DOT 39
#define PANEL_IC_CLIP_PISTOL 40
#define PANEL_IC_CLIP_SHOTGUN 41
#define PANEL_IC_CLIP_AK47 42
#define PANEL_IC_GEAR_BOX 43
#define PANEL_IC_GEAR_LOW 44
#define PANEL_IC_GEAR_HIGH 45
#define PANEL_IC_NUMBER 46

// uc_orig: PANEL_ic (fallen/DDEngine/Source/panel.cpp)
extern PANEL_Ic PANEL_ic[PANEL_IC_NUMBER];

// Page index selectors for icons, indexed by [player][page_type].
#define PANEL_PAGE_NORMAL 0
#define PANEL_PAGE_ALPHA 1
#define PANEL_PAGE_ADDITIVE 2
#define PANEL_PAGE_ALPHA_END 3
#define PANEL_PAGE_NUMBER 4

// uc_orig: PANEL_page (fallen/DDEngine/Source/panel.cpp)
extern unsigned short PANEL_page[4][PANEL_PAGE_NUMBER];

// Heartbeat waveform circular buffer (one per player).
typedef struct
{
    float x;      // 0..1 normalised position across the display
    float y;      // -1..1 normalised amplitude
    unsigned int colour;
} PANEL_Beat;

#define PANEL_NUM_BEATS 32

// uc_orig: PANEL_beat (fallen/DDEngine/Source/panel.cpp)
extern PANEL_Beat PANEL_beat[2][PANEL_NUM_BEATS];
// uc_orig: PANEL_beat_head (fallen/DDEngine/Source/panel.cpp)
extern int PANEL_beat_head[2];
// uc_orig: PANEL_beat_tick (fallen/DDEngine/Source/panel.cpp)
extern unsigned int PANEL_beat_tick[2];
// uc_orig: PANEL_beat_last_ammo (fallen/DDEngine/Source/panel.cpp)
extern int PANEL_beat_last_ammo[2];
// uc_orig: PANEL_beat_last_specialtype (fallen/DDEngine/Source/panel.cpp)
extern int PANEL_beat_last_specialtype[2];
// uc_orig: PANEL_beat_x (fallen/DDEngine/Source/panel.cpp)
extern float PANEL_beat_x[2];

// Ammo type display properties (icon size and page indices).
#define PANEL_AMMO_PISTOL 0
#define PANEL_AMMO_SHOTGUN 1
#define PANEL_AMMO_AK47 2
#define PANEL_AMMO_GRENADE 3
#define PANEL_AMMO_NUMBER 4

typedef struct
{
    float width;
    float height;
    int size_group;
    int page_group;
    int page_one;
} PANEL_Ammo;

// uc_orig: PANEL_ammo (fallen/DDEngine/Source/panel.cpp)
extern PANEL_Ammo PANEL_ammo[PANEL_AMMO_NUMBER];

// Animated toss (ammo flying off-screen) state.
typedef struct
{
    unsigned short used;
    unsigned short type;
    float x;
    float y;
    float angle;
    float dx;
    float dy;
} PANEL_Toss;

#define PANEL_MAX_TOSSES 8

// uc_orig: PANEL_toss (fallen/DDEngine/Source/panel.cpp)
extern PANEL_Toss PANEL_toss[PANEL_MAX_TOSSES];
// uc_orig: PANEL_toss_last (fallen/DDEngine/Source/panel.cpp)
extern int PANEL_toss_last;
// uc_orig: PANEL_toss_tick (fallen/DDEngine/Source/panel.cpp)
extern unsigned int PANEL_toss_tick;

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
extern unsigned int PANEL_text_tick;

// Face size constants for PANEL_new_face().
// uc_orig: PANEL_FACE_LARGE (fallen/DDEngine/Source/panel.cpp)
#define PANEL_FACE_LARGE 1
// uc_orig: PANEL_FACE_SMALL (fallen/DDEngine/Source/panel.cpp)
#define PANEL_FACE_SMALL 2

// Help message (set by PANEL_new_help_message, displayed by PANEL_help_message_do).
// uc_orig: PANEL_help_message (fallen/DDEngine/Source/panel.cpp)
extern char PANEL_help_message[256];
// uc_orig: PANEL_help_timer (fallen/DDEngine/Source/panel.cpp)
extern long PANEL_help_timer;

// Widescreen cut-scene: current top/bottom speaker thing indices and text.
// uc_orig: PANEL_wide_top_person (fallen/DDEngine/Source/panel.cpp)
extern unsigned short PANEL_wide_top_person;
// uc_orig: PANEL_wide_bot_person (fallen/DDEngine/Source/panel.cpp)
extern unsigned short PANEL_wide_bot_person;
// uc_orig: PANEL_wide_top_is_talking (fallen/DDEngine/Source/panel.cpp)
extern long PANEL_wide_top_is_talking;
// uc_orig: PANEL_wide_text (fallen/DDEngine/Source/panel.cpp)
extern char PANEL_wide_text[256];

// Beacon colour table (one colour per beacon slot, cycled by beacon index mod 12).
// uc_orig: PANEL_MAX_BEACON_COLOURS (fallen/DDEngine/Source/panel.cpp)
#define PANEL_MAX_BEACON_COLOURS 12
// uc_orig: PANEL_beacon_colour (fallen/DDEngine/Source/panel.cpp)
extern unsigned long PANEL_beacon_colour[PANEL_MAX_BEACON_COLOURS];

// Fade-out effect state: timestamp when PANEL_fadeout_start() was called (0 = inactive).
// uc_orig: PANEL_fadeout_time (fallen/DDEngine/Source/panel.cpp)
extern long PANEL_fadeout_time;

// Zoom/spin multipliers for the fade-out effect texture transform.
// uc_orig: angle_mul (fallen/DDEngine/Source/panel.cpp)
extern float angle_mul;
// uc_orig: zoom_mul (fallen/DDEngine/Source/panel.cpp)
extern float zoom_mul;

// Road sign flash state.
// uc_orig: PANEL_sign_which (fallen/DDEngine/Source/panel.cpp)
extern long PANEL_sign_which;
// uc_orig: PANEL_sign_flip (fallen/DDEngine/Source/panel.cpp)
extern long PANEL_sign_flip;
// uc_orig: PANEL_sign_time (fallen/DDEngine/Source/panel.cpp)
extern unsigned long PANEL_sign_time;

// Info message (bottom of panel, 2-second display).
// uc_orig: PANEL_info_message (fallen/DDEngine/Source/panel.cpp)
extern char PANEL_info_message[512];
// uc_orig: PANEL_info_time (fallen/DDEngine/Source/panel.cpp)
extern unsigned long PANEL_info_time;

#endif // UI_HUD_PANEL_GLOBALS_H
