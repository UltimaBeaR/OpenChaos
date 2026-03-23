#ifndef UI_HUD_PANEL_H
#define UI_HUD_PANEL_H

// Public interface for the HUD panel system — health bars, gun sights,
// timers, inventory overlay, text messages, and full-screen effects.
// The panel renders directly into the poly submission buffer (POLY_add_*).

// uc_orig: PANEL_scanner_poo (fallen/DDEngine/Source/panel.cpp)
extern unsigned char PANEL_scanner_poo;

// uc_orig: PANEL_start (fallen/DDEngine/Source/panel.cpp)
void PANEL_start(void);

// uc_orig: PANEL_finish (fallen/DDEngine/Source/panel.cpp)
void PANEL_finish(void);

// uc_orig: PANEL_draw_gun_sight (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_gun_sight(long mx, long my, long mz, long accuracy, long scale);

// uc_orig: PANEL_draw_timer (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_timer(long time_in_hundredths, long x, long y);

// uc_orig: PANEL_draw_buffered (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_buffered(void);

// uc_orig: PANEL_draw_health_bar (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_health_bar(long x, long y, long percentage);

// uc_orig: PANEL_new_text_init (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_text_init(void);

// uc_orig: PANEL_new_text (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_text(struct Thing* who, long delay, char* fmt, ...);

// uc_orig: PANEL_new_help_message (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_help_message(char* fmt, ...);

// uc_orig: PANEL_new_info_message (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_info_message(char* fmt, ...);

// Signs shown briefly on-screen (road signs in vehicle missions).
#define PANEL_SIGN_WHICH_UTURN 0
#define PANEL_SIGN_WHICH_TURN_RIGHT 1
#define PANEL_SIGN_WHICH_DONT_TAKE_RIGHT_TURN 2
#define PANEL_SIGN_WHICH_STOP 3
#define PANEL_SIGN_FLIP_LEFT_AND_RIGHT (1 << 0)
#define PANEL_SIGN_FLIP_TOP_AND_BOTTOM (1 << 1)

// uc_orig: PANEL_flash_sign (fallen/DDEngine/Source/panel.cpp)
void PANEL_flash_sign(long which, long flip);

// uc_orig: PANEL_darken_screen (fallen/DDEngine/Source/panel.cpp)
void PANEL_darken_screen(long x);

// uc_orig: PANEL_fadeout_init (fallen/DDEngine/Source/panel.cpp)
void PANEL_fadeout_init(void);

// uc_orig: PANEL_fadeout_start (fallen/DDEngine/Source/panel.cpp)
void PANEL_fadeout_start(void);

// uc_orig: PANEL_fadeout_draw (fallen/DDEngine/Source/panel.cpp)
void PANEL_fadeout_draw(void);

// uc_orig: PANEL_fadeout_finished (fallen/DDEngine/Source/panel.cpp)
long PANEL_fadeout_finished(void);

// uc_orig: PANEL_last (fallen/DDEngine/Source/panel.cpp)
void PANEL_last(void);

// uc_orig: PANEL_draw_completion_bar (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_completion_bar(long completion);

// uc_orig: PANEL_GetNextDepthBodge (fallen/DDEngine/Source/panel.cpp)
float PANEL_GetNextDepthBodge(void);

// uc_orig: PANEL_ResetDepthBodge (fallen/DDEngine/Source/panel.cpp)
void PANEL_ResetDepthBodge(void);

// uc_orig: PANEL_enable_screensaver (fallen/DDEngine/Source/panel.cpp)
void PANEL_enable_screensaver(void);

// uc_orig: PANEL_disable_screensaver (fallen/DDEngine/Source/panel.cpp)
void PANEL_disable_screensaver(bool bImmediately = false);

// uc_orig: PANEL_screensaver_draw (fallen/DDEngine/Source/panel.cpp)
void PANEL_screensaver_draw(void);

// uc_orig: PANEL_draw_quad (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_quad(
    float left,
    float top,
    float right,
    float bottom,
    long page,
    unsigned long colour = 0xffffffff,
    float u1 = 0.0F,
    float v1 = 0.0F,
    float u2 = 1.0F,
    float v2 = 1.0F);

// uc_orig: PANEL_inventory (fallen/DDEngine/Source/panel.cpp)
void PANEL_inventory(struct Thing* darci, struct Thing* player);

// Internal helpers used by later chunks of panel.cpp (chunk 2+) before they are migrated.
// Will become static-equivalent once all chunks are migrated.
// uc_orig: PANEL_crap_text (fallen/DDEngine/Source/panel.cpp)
void PANEL_crap_text(int x, int y, char* string);

// uc_orig: PANEL_draw_face (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_face(long x, long y, long face, long size);

// uc_orig: PANEL_draw_number (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_number(float x, float y, unsigned char digit);

// uc_orig: PANEL_draw_local_health (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_local_health(long mx, long my, long mz, long percentage, long radius = 60);

// uc_orig: PANEL_funky_quad (fallen/DDEngine/Source/panel.cpp)
void PANEL_funky_quad(long which, long x, long y, long panel_page, unsigned long colour,
    float width = -1.0F, float height = -1.0F);

// uc_orig: PANEL_new_toss (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_toss(long type, float sx, float sy);

// uc_orig: PANEL_do_tosses (fallen/DDEngine/Source/panel.cpp)
void PANEL_do_tosses(void);

// uc_orig: PANEL_new_face (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_face(struct Thing* who, float x, float y, long size);

// ============================================================
// Chunk 2 helpers: used by PANEL_last() (chunk 3, still in old/).
// Made non-static to allow old/ code to reference them.
// Will be made static once PANEL_last() is migrated (iteration 141).
// ============================================================

// uc_orig: PANEL_new_text_process (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_text_process(void);

// uc_orig: PANEL_help_message_do (fallen/DDEngine/Source/panel.cpp)
void PANEL_help_message_do(void);

// uc_orig: PANEL_new_widescreen (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_widescreen(void);

// Lsprite type and table (needed by PANEL_last in old/).
// uc_orig: PANEL_LSPRITE_BACKGROUND (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_BACKGROUND 0
// uc_orig: PANEL_LSPRITE_AK47 (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_AK47 1
// uc_orig: PANEL_LSPRITE_SHOTGUN (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_SHOTGUN 2
// uc_orig: PANEL_LSPRITE_LOW_GEAR (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_LOW_GEAR 3
// uc_orig: PANEL_LSPRITE_HIGH_GEAR (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_HIGH_GEAR 4
// uc_orig: PANEL_LSPRITE_GRENADE (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_GRENADE 5
// uc_orig: PANEL_LSPRITE_ARROW (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_ARROW 6
// uc_orig: PANEL_LSPRITE_QMARK (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_QMARK 7
// uc_orig: PANEL_LSPRITE_DOT (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_DOT 8
// uc_orig: PANEL_LSPRITE_PISTOL (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_PISTOL 9
// uc_orig: PANEL_LSPRITE_TEXT_BOX (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_TEXT_BOX 10
// uc_orig: PANEL_LSPRITE_BBB (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_BBB 11
// uc_orig: PANEL_LSPRITE_FIST (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_FIST 12
// uc_orig: PANEL_LSPRITE_EXPLOSIVES (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_EXPLOSIVES 13
// uc_orig: PANEL_LSPRITE_KNIFE (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_KNIFE 14
// uc_orig: PANEL_LSPRITE_NUMBER (fallen/DDEngine/Source/panel.cpp)
#define PANEL_LSPRITE_NUMBER 15

// uc_orig: PANEL_Lsprite (fallen/DDEngine/Source/panel.cpp)
typedef struct
{
    long page;
    float u1;
    float v1;
    float u2;
    float v2;
} PANEL_Lsprite;

// uc_orig: PANEL_lsprite (fallen/DDEngine/Source/panel.cpp)
extern PANEL_Lsprite PANEL_lsprite[PANEL_LSPRITE_NUMBER];

// uc_orig: PANEL_last_arrow (fallen/DDEngine/Source/panel.cpp)
void PANEL_last_arrow(float x, float y, float angle, float size, unsigned long colour, unsigned char is_dot);

// uc_orig: PANEL_last_bubble (fallen/DDEngine/Source/panel.cpp)
void PANEL_last_bubble(float x1, float y1, float x2, float y2);

// uc_orig: BodgePageIntoAddAlpha (fallen/DDEngine/Source/panel.cpp)
long BodgePageIntoAddAlpha(long oldpage);
// uc_orig: BodgePageIntoAdd (fallen/DDEngine/Source/panel.cpp)
long BodgePageIntoAdd(long oldpage);
// uc_orig: BodgePageIntoSub (fallen/DDEngine/Source/panel.cpp)
long BodgePageIntoSub(long oldpage);

#endif // UI_HUD_PANEL_H
