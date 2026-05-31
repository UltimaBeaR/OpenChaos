#ifndef UI_INPUT_GLYPHS_H
#define UI_INPUT_GLYPHS_H

#include "engine/core/types.h" // SLONG
#include "ui/input_glyphs/input_prompt_map.h" // GlyphDevice

// Input-prompt glyph atlases (Kenney Input Prompts, CC0).
//
// Four PNG atlases are embedded into the executable at build time and decoded
// once at startup into their own GPU texture pages (TEXTURE_page_glyph_*).
// Atlases are named by glyph STYLE, not brand, to avoid carrying trademarks:
//   keyboard  — keyboard & mouse
//   generic   — Xbox-style face buttons (ABXY); used for INPUT_DEVICE_XBOX
//   ps        — PlayStation-style face buttons; used for INPUT_DEVICE_DUALSENSE
//   deck      — Steam Deck-style
// Drawing individual glyphs (per-cell UV lookup) comes later; this module only
// handles loading the atlases onto the GPU. Glyph cell coordinates are hardcoded
// in input_glyph_coords.{h,cpp} (generated once from the Kenney pack's atlas XMLs,
// which are not kept in the repo).

// Decode the four embedded glyph atlases and upload each to its texture page.
// Must be called once after the GL context and texture system are ready.
void input_glyphs_init(void);

// Draw a "rich-text" string: normal text interleaved with inline input-prompt
// glyphs, word-wrapped to `wrap_width` virtual px. Inline glyph tokens are
// substrings of the form `{id}` where `id` is a glyph id from input_prompt_map
// (e.g. "kb_w", "xb_a", "ps_cross"); each draws that device's atlas cell.
// Unknown tokens are skipped. Text is drawn with FONT2D at `text_scale`
// (256 == 1.0) in `colour` (0xRRGGBB). Explicit '\n' forces a line break.
// Drawing happens in the CALLER's current UI scope (virtual 640x480 coords) —
// the function does NOT push its own scope. Returns the total height drawn
// (number_of_lines * line_advance) so callers can lay out below it.
// `fade` (0 = opaque .. 255 = invisible) fades the whole block in/out (text and
// glyphs together) — drive it from a menu fade timer for a consistent reveal.
float input_glyph_text_draw(const char* str, float x, float y, float wrap_width,
                            SLONG text_scale, unsigned long colour, SWORD fade = 0);

// Draw rich text as a vertically-scrolled window. The WHOLE string is laid out
// into wrapped lines first; because the wrap depends only on the string, width,
// scale and the active device's glyph widths, it is identical on every call —
// so the line breaks never shift as the player scrolls. Only the WHOLE lines
// that fit in `view_height` (virtual px) are drawn, starting at line index
// *first_line; no partial line is ever drawn, so the window needs no clipping.
//
// *first_line is clamped to a valid range and WRITTEN BACK, so the caller's
// scroll value can never run past the ends (drive it with up/down nav and let
// this function bound it). *out_fit receives how many lines fit the window.
// Returns the total wrapped line count. Use first_line/out_fit/total to decide
// whether to show "more above / more below" scroll arrows.
SLONG input_glyph_text_draw_scrolled(const char* str, float x, float y,
                                     float wrap_width, SLONG text_scale,
                                     unsigned long colour, SWORD fade,
                                     SLONG* first_line, float view_height,
                                     SLONG* out_fit);

// Draw a glyph by raw atlas CELL (col, row) from the GIVEN device's atlas (all
// atlases are uniform 64px grids; cell is 0-based, row counted from the bottom).
// Returns the advance width. fade: 0 opaque..255 gone.
float input_glyph_draw_cell(GlyphDevice dev, int col, int row, float x, float y, float line_height, SWORD fade = 0);

// Draw the input-prompt CATALOG (dev test page, gated by the debug flag) as a vertically-scrolled
// list — one row per line, "<label> - <glyph>", for every glyph of the CURRENTLY
// ACTIVE device (so the list changes with the device). Same
// scroll contract as input_glyph_text_draw_scrolled: *first_line is clamped and
// written back, *out_fit gets the visible-row count, returns the total row count.
SLONG input_prompt_catalog_draw_scrolled(float x, float y, SLONG text_scale,
                                         unsigned long colour, SWORD fade,
                                         SLONG* first_line, float view_height,
                                         SLONG* out_fit);

#endif // UI_INPUT_GLYPHS_H
