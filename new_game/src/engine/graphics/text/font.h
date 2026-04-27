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

// Queues a message to be drawn later by FONT_buffer_draw. (x, y) are
// literal framebuffer pixels — use this for overlays that intentionally
// pin to a fixed pixel corner regardless of window size (F1 debug
// legend etc.). Most call sites should prefer FONT_buffer_add_virtual.
// uc_orig: FONT_buffer_add (fallen/DDEngine/Headers/Font.h)
void FONT_buffer_add(
    SLONG x,
    SLONG y,
    UBYTE red,
    UBYTE green,
    UBYTE blue,
    UBYTE shadowed_or_not,
    CBYTE* fmt, ...);

// Same as FONT_buffer_add but (x, y) are virtual screen pixels in the
// 480-tall game space (POLY_screen_width × POLY_screen_height). The flush
// scales them by PolyPage::s_XScale / s_YScale at draw time, so virtual
// coords land on the post-composition default framebuffer correctly at
// any OC_RENDER_SCALE / aspect. Use this for AENG_world_text style
// 3D-projected labels and any HUD text whose layout was computed in the
// virtual game space (matches AENG_draw_rect / PANEL_draw_quad scaling).
void FONT_buffer_add_virtual(
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


#endif // ENGINE_GRAPHICS_TEXT_FONT_H
