#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "engine/graphics/resources/console.h"
#include "engine/graphics/resources/console_globals.h"
#include "engine/graphics/resources/font2d.h"
#include "engine/graphics/resources/font.h"
#include "engine/graphics/pipeline/poly.h"

// Forward declarations for panel (panel.cpp not yet migrated).
// uc_orig: PANEL_new_text (fallen/DDEngine/Headers/panel.h)
extern void PANEL_new_text(struct Thing* who, SLONG delay, CBYTE* fmt, ...);


// Returns TRUE when there are no active positioned messages to display.
// uc_orig: CONSOLE_no_text (fallen/DDEngine/Source/console.cpp)
static SLONG CONSOLE_no_text(void)
{
    for (SLONG i = 0; i < CONSOLE_MAX_MESSES; i++) {
        if (CONSOLE_mess[i].delay) {
            return FALSE;
        }
    }
    return TRUE;
}

// uc_orig: CONSOLE_font (fallen/DDEngine/Headers/console.h)
void CONSOLE_font(CBYTE* fontpath, float scale)
{
    // Font selection is stubbed — was intended for a 3D font object.
}

// uc_orig: CONSOLE_draw (fallen/DDEngine/Headers/console.h)
void CONSOLE_draw(void)
{
    console_last_tick = console_this_tick;
    console_this_tick = GetTickCount();

    // Clamp wildly large deltas (e.g. when waking from sleep).
    if (console_this_tick - console_last_tick > 4000) {
        console_last_tick = console_this_tick - 1000;
    }

    if (CONSOLE_no_text() && !console_Data[0].Age) {
        if (*console_status_text) {
            POLY_frame_init(FALSE, FALSE);
            FONT2D_DrawString(console_status_text, 10, 10, 0x7f00ff00, 256, POLY_PAGE_FONT2D);
            POLY_frame_draw(FALSE, TRUE);
        }
        return;
    }

    POLY_frame_init(FALSE, FALSE);

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

    POLY_frame_draw(FALSE, TRUE);

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

    // Dead code — the old direct-to-buffer path is preserved 1:1.
    SLONG i;
    CBYTE* ch;
    CBYTE* ch1;
    CBYTE* ch2;
    CBYTE* pipe;

    CBYTE temp[1024];

    if (text == NULL) {
        return;
    }

    for (ch = text; *ch; ch++) {
        if (!isspace(*ch)) {
            goto found_non_white_space;
        }
    }

    return;

found_non_white_space:;

    pipe = strchr(text, '|');

    if (pipe) {
        strcpy(temp, text);
        temp[pipe - text] = '\000';

        CONSOLE_text(temp, delay);
        CONSOLE_text(&temp[pipe + 1 - text], delay);

        return;
    }

    if (strlen(text) <= CONSOLE_WIDTH) {
        ch1 = text;
        ch2 = NULL;
    } else {
        strcpy(temp, text);

        for (i = CONSOLE_WIDTH - 1; i > 0; i--) {
            if (isspace(temp[i])) {
                temp[i] = '\000';
                ch1 = temp;
                ch2 = temp + i + 1;
                goto split_up_line;
            }
        }

        return;

    split_up_line:;
    }

    i = 0;
    while ((i < CONSOLE_LINES) && (console_Data[i].Age))
        i++;
    if (i == CONSOLE_LINES) {
        CONSOLE_scroll();
        i = CONSOLE_LINES - 1;
    }
    console_Data[i].Age = delay;
    strncpy(console_Data[i].Text, ch1, CONSOLE_WIDTH - 1);
    console_Data[i].Text[CONSOLE_WIDTH - 1] = 0;

    for (ch = console_Data[i].Text; *ch; *ch++ = toupper(*ch))
        ;

    CONSOLE_text(ch2, delay);
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
        console_this_tick = GetTickCount();
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
