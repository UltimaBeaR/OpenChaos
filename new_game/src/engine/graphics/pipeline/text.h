#ifndef ENGINE_GRAPHICS_PIPELINE_TEXT_H
#define ENGINE_GRAPHICS_PIPELINE_TEXT_H

#include "engine/core/types.h"

// Renders proportional bitmap text using D3D texture-mapped quads.
// Font data is stored in the texture page referenced by TEXTURE_page_font.

// uc_orig: text_fudge (fallen/DDEngine/Source/Text.cpp)
extern BOOL text_fudge;
// uc_orig: text_colour (fallen/DDEngine/Source/Text.cpp)
extern ULONG text_colour;

// Returns the pixel width of message up to the first newline.
// Sets *char_count to the number of characters measured.
// uc_orig: text_width (fallen/DDEngine/Headers/Text.h)
SLONG text_width(CBYTE* message, SLONG font_id, SLONG* char_count);

// Returns the pixel height of the tallest character in message up to the first newline.
// uc_orig: text_height (fallen/DDEngine/Headers/Text.h)
SLONG text_height(CBYTE* message, SLONG font_id, SLONG* char_count);

// Draws message at screen position (x, y) using font_id.
// uc_orig: draw_text_at (fallen/DDEngine/Headers/Text.h)
void draw_text_at(float x, float y, CBYTE* message, SLONG font_id);

// Draws message centred horizontally around x. If flag != 0, wraps on newlines.
// uc_orig: draw_centre_text_at (fallen/DDEngine/Headers/Text.h)
void draw_centre_text_at(float x, float y, CBYTE* message, SLONG font_id, SLONG flag);

// Legacy stub; body is a no-op return.
// uc_orig: show_text (fallen/DDEngine/Headers/Text.h)
void show_text(void);

#endif // ENGINE_GRAPHICS_PIPELINE_TEXT_H
