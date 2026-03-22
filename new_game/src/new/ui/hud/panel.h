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

#endif // UI_HUD_PANEL_H
