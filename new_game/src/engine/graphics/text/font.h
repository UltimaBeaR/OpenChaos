#ifndef ENGINE_GRAPHICS_TEXT_FONT_H
#define ENGINE_GRAPHICS_TEXT_FONT_H

#include "engine/core/types.h"

// Maximum rendered character dimensions.
// uc_orig: FONT_HEIGHT (fallen/DDEngine/Headers/Font.h)
#define FONT_HEIGHT 9
// uc_orig: FONT_WIDTH (fallen/DDEngine/Headers/Font.h)
#define FONT_WIDTH 8

// Maximum string length for any single message.
// uc_orig: FONT_MAX_LENGTH (fallen/DDEngine/Headers/Font.h)
#define FONT_MAX_LENGTH 512

// Queues a message to be drawn later by FONT_buffer_draw.
// Parameters: position, rgb colour, shadowed (1) or not (0), printf-style format.
// uc_orig: FONT_buffer_add (fallen/DDEngine/Headers/Font.h)
void FONT_buffer_add(
    SLONG x,
    SLONG y,
    UBYTE red,
    UBYTE green,
    UBYTE blue,
    UBYTE shadowed_or_not,
    CBYTE* fmt, ...);

// Renders all queued messages and clears the queue.
// Locks and unlocks the screen internally.
// uc_orig: FONT_buffer_draw (fallen/DDEngine/Headers/Font.h)
void FONT_buffer_draw(void);

// Direct screen-pixel rendering functions.
// Caller is responsible for locking the screen via the_display.screen_lock().

// Draws text at (x,y) in yellow with a red shadow. Returns pixel width.
// uc_orig: FONT_draw (fallen/DDEngine/Headers/Font.h)
SLONG FONT_draw(SLONG x, SLONG y, CBYTE* fmt, ...);

// Draws text at (x,y) in the specified colour. Returns pixel width.
// uc_orig: FONT_draw_coloured_text (fallen/DDEngine/Headers/Font.h)
SLONG FONT_draw_coloured_text(
    SLONG x,
    SLONG y,
    UBYTE red,
    UBYTE green,
    UBYTE blue,
    CBYTE* fmt, ...);

// Draws a single character at (x,y) in the specified colour. Returns pixel width.
// uc_orig: FONT_draw_coloured_char (fallen/DDEngine/Headers/Font.h)
SLONG FONT_draw_coloured_char(
    SLONG x,
    SLONG y,
    UBYTE red,
    UBYTE green,
    UBYTE blue,
    CBYTE ch);

// Draws text word-wrapped into a roughly square box. Bottom-left at (x,y).
// uc_orig: FONT_draw_speech_bubble_text (fallen/DDEngine/Headers/Font.h)
void FONT_draw_speech_bubble_text(
    SLONG x,
    SLONG y,
    UBYTE red,
    UBYTE green,
    UBYTE blue,
    CBYTE* fmt, ...);

#endif // ENGINE_GRAPHICS_TEXT_FONT_H
