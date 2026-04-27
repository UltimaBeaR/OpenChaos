#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "engine/console/console.h"
#include "engine/console/console_globals.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/graphics/text/font2d.h"
#include "engine/graphics/text/font.h"
#include "engine/graphics/pipeline/poly.h"

// Forward declarations for panel (panel.cpp not yet migrated).
// uc_orig: PANEL_new_text (fallen/DDEngine/Headers/panel.h)
extern void PANEL_new_text(struct Thing* who, SLONG delay, CBYTE* fmt, ...);

// Returns UC_TRUE when there are no active positioned messages to display.
// uc_orig: CONSOLE_no_text (fallen/DDEngine/Source/console.cpp)
static SLONG CONSOLE_no_text(void)
{
    for (SLONG i = 0; i < CONSOLE_MAX_MESSES; i++) {
        if (CONSOLE_mess[i].delay) {
            return UC_FALSE;
        }
    }
    return UC_TRUE;
}

// uc_orig: CONSOLE_font (fallen/DDEngine/Headers/console.h)
void CONSOLE_font(CBYTE* fontpath, float scale)
{
    // Stub — 3D font object was never implemented.
}

// uc_orig: CONSOLE_draw (fallen/DDEngine/Headers/console.h)
void CONSOLE_draw(void)
{
    console_last_tick = console_this_tick;
    console_this_tick = sdl3_get_ticks();

    // Clamp wildly large deltas (e.g. when waking from sleep).
    if (console_this_tick - console_last_tick > 4000) {
        console_last_tick = console_this_tick - 1000;
    }

    if (CONSOLE_no_text() && !console_Data[0].Age) {
        if (*console_status_text) {
            POLY_frame_init(UC_FALSE, UC_FALSE);
            FONT2D_DrawString(console_status_text, 10, 10, 0x7f00ff00, 256, POLY_PAGE_FONT2D);
            POLY_frame_draw(UC_FALSE, UC_TRUE);
        }
        return;
    }

    POLY_frame_init(UC_FALSE, UC_FALSE);

    if (*console_status_text) {
        FONT2D_DrawString(console_status_text, 10, 10, 0x7f00ff00, 256, POLY_PAGE_FONT2D);
    }

    // Draw scrolling lines with fade-out when Age drops below 512.
    for (SLONG i = 0; i < CONSOLE_LINES; i++) {
        if (console_Data[i].Age) {
            if (console_Data[i].Age > 512) {
                FONT2D_DrawString(console_Data[i].Text, 135, 370 + (17 * i), 0x00ff00, 256, POLY_PAGE_FONT2D);
            } else {
                FONT2D_DrawString(console_Data[i].Text, 135, 370 + (17 * i), 0x00ff00, 256, POLY_PAGE_FONT2D, (SWORD)((512 - console_Data[i].Age) >> 3));
            }

            console_Data[i].Age -= console_this_tick - console_last_tick;

            if (console_Data[i].Age < 0) {
                console_Data[i].Age = 0;
            }
        }
    }

    // Draw positioned messages from CONSOLE_text_at.
    // uc_orig: was commented out in original (used font->DrawString / 3D font);
    // re-enabled here with FONT2D_DrawString.
    for (SLONG i = 0; i < CONSOLE_MAX_MESSES; i++) {
        CONSOLE_Mess* cm = &CONSOLE_mess[i];
        if (cm->delay) {
            FONT2D_DrawString(cm->mess, cm->x, cm->y, 0xffffff, 256, POLY_PAGE_FONT2D);
            cm->delay -= console_this_tick - console_last_tick;
            if (cm->delay < 0) {
                cm->delay = 0;
            }
        }
    }

    POLY_frame_draw(UC_FALSE, UC_TRUE);

    if (!console_Data[0].Age) {
        CONSOLE_scroll();
    }
}

// uc_orig: CONSOLE_text (fallen/DDEngine/Headers/console.h)
void CONSOLE_text(CBYTE* text, SLONG delay)
{
    // Route through the panel system — the old scrolling logic below is dead.
    PANEL_new_text(NULL, delay, text);
    return;
}

// uc_orig: CONSOLE_scroll (fallen/DDEngine/Headers/console.h)
void CONSOLE_scroll(void)
{
    while (console_Data[0].Text[0] && !console_Data[0].Age) {
        for (UBYTE i = 1; i < CONSOLE_LINES; i++) {
            console_Data[i - 1] = console_Data[i];
        }
        console_Data[CONSOLE_LINES - 1].Age = 0;
        console_Data[CONSOLE_LINES - 1].Text[0] = 0;
    }
}

// uc_orig: CONSOLE_clear (fallen/DDEngine/Headers/console.h)
void CONSOLE_clear(void)
{
    memset(console_Data, 0, sizeof(console_Data));
    if (!console_this_tick) {
        console_this_tick = sdl3_get_ticks();
    }
}

// uc_orig: CONSOLE_text_at (fallen/DDEngine/Headers/console.h)
void CONSOLE_text_at(SLONG x, SLONG y, SLONG delay, CBYTE* fmt, ...)
{
    CBYTE message[FONT_MAX_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    // Convert to uppercase to match the game's style.
    for (CBYTE* ch = message; *ch; *ch++ = toupper(*ch))
        ;

    // Replace any existing message at the same position.
    for (SLONG i = 0; i < CONSOLE_MAX_MESSES; i++) {
        CONSOLE_Mess* cm = &CONSOLE_mess[i];
        if (cm->delay && cm->x == x && cm->y == y) {
            strncpy(cm->mess, message, CONSOLE_WIDTH);
            cm->delay = delay;
            return;
        }
    }

    // Find a free slot.
    for (SLONG i = 0; i < CONSOLE_MAX_MESSES; i++) {
        CONSOLE_Mess* cm = &CONSOLE_mess[i];
        if (cm->delay <= 0) {
            strncpy(cm->mess, message, CONSOLE_WIDTH);
            cm->delay = delay;
            cm->x = x;
            cm->y = y;
            return;
        }
    }
}

// uc_orig: CONSOLE_status (fallen/DDEngine/Source/console.cpp)
void CONSOLE_status(CBYTE* msg)
{
    strcpy(console_status_text, msg);
    for (CBYTE* ch = console_status_text; *ch; *ch++ = toupper(*ch))
        ;
}
