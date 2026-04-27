#include "engine/console/message.h"
#include "engine/console/message_globals.h"
#include "engine/core/types.h"
#include "engine/input/keyboard.h"
#include "engine/input/keyboard_globals.h"
#include "engine/graphics/text/font.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

// allow_debug_keys defined in Controls.cpp
extern BOOL allow_debug_keys;

// uc_orig: MSG_TIMER (fallen/DDEngine/Source/Message.cpp)
#define MSG_TIMER 128

// uc_orig: MSG_add (fallen/DDEngine/Source/Message.cpp)
void MSG_add(char* fmt, ...)
{
    if (!allow_debug_keys)
        return;

    CBYTE message[512];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    /*
    for (i=0; i<MSG_MAX_MESSAGES; i++) {
        if (MSG_message[i].timer < oldtimer) {
            oldest = i;
            oldtimer = MSG_message[i].timer;
        }
    }
    */

    strncpy(MSG_message[current_message].message, message, MSG_MAX_LENGTH - 1);
    MSG_message[current_message].timer = MSG_TIMER;
    current_message++;
    if (current_message > MSG_MAX_MESSAGES - 2)
        current_message = 0;

    return;
}

// uc_orig: SCREEN_SIZE (fallen/DDEngine/Source/Message.cpp)
#define SCREEN_SIZE 45

// uc_orig: MSG_draw (fallen/DDEngine/Source/Message.cpp)
void MSG_draw(void)
{
    SLONG i, x, y;
    UBYTE red, green, blue;
    SLONG pos;
    SLONG size = 1;

    if (!allow_debug_keys)
        return;

    pos = current_message - SCREEN_SIZE + draw_message_offset;
    if (pos < 0)
        pos += MSG_MAX_MESSAGES;
    // BUGFIX: original had `pos > MSG_MAX_MESSAGES` which left pos == 1000
    // untouched and made the loop below read MSG_message[1000], one past
    // the 1000-element array. Caught by ASan. Same off-by-one in the
    // wrap at the bottom of this function.
    if (pos >= MSG_MAX_MESSAGES)
        pos -= MSG_MAX_MESSAGES;

    if (ShiftFlag)
        size = 20;
    if (Keys[KB_PPLUS])
        draw_message_offset += size;
    if (Keys[KB_PMINUS])
        draw_message_offset -= size;
    if (Keys[KB_PENTER])
        draw_message_offset = 0;

    for (i = 0; i < SCREEN_SIZE; i++) {
        x = 10;
        y = 10 + i * 10;

        if (MSG_message[pos].timer) {
            red = 223;
            green = 223;
            blue = 223;
            FONT_draw_coloured_text(x + 1, y + 1, red, 0, blue, MSG_message[pos].message);
            FONT_draw_coloured_text(x + 0, y + 0, red + 32, green + 32, blue + 32, MSG_message[pos].message);
        }

        pos++;
        if (pos >= MSG_MAX_MESSAGES)
            pos = 0;
    }
}
