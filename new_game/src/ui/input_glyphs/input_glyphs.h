#ifndef UI_INPUT_GLYPHS_H
#define UI_INPUT_GLYPHS_H

#include "engine/core/types.h" // SLONG

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

// Logical input prompts a help screen can show. Each resolves to a device-
// specific glyph (keyboard key / Xbox or PlayStation face button) at draw time,
// based on active_input_device. Starter set — extend as the help content grows.
enum InputGlyphKey {
    INPUT_GLYPH_KEY_JUMP,
    INPUT_GLYPH_KEY_USE,
    INPUT_GLYPH_KEY_COUNT
};

// Draw the prompt for `key` matching the currently active input device at (x, y)
// in the CALLER's current UI scope (virtual 640x480 coords). `line_height` sizes
// the glyph; the (square) glyph is left-aligned at x and centred vertically
// within the line height, drawn at a "clean" size (a power-of-two fraction of
// the 64px source) snapped to whole framebuffer pixels so outlines stay crisp.
// Returns the ADVANCE width (glyph width + a small inter-glyph gap) so prompts
// can be laid out inline: `x += input_glyph_draw(...)`. Returns 0 if unresolved.
// `fade` (0 = opaque .. 255 = invisible) scales the glyph's alpha, for fade-in.
float input_glyph_draw(InputGlyphKey key, float x, float y, float line_height, SWORD fade = 0);

// Compute the advance width input_glyph_draw would return for `key` at the given
// `line_height`, WITHOUT drawing anything. Shares the size/snap math with
// input_glyph_draw so measure and draw never diverge. Returns 0 if unresolved.
float input_glyph_advance(InputGlyphKey key, float line_height);

// Draw a "rich-text" string: normal text interleaved with inline input-prompt
// glyphs, word-wrapped to `wrap_width` virtual px. Inline glyph tokens are
// substrings of the form `{name}` (name -> InputGlyphKey: "jump", "use");
// unknown tokens are skipped. Text is drawn with FONT2D at `text_scale`
// (256 == 1.0) in `colour` (0xRRGGBB). Explicit '\n' forces a line break.
// Drawing happens in the CALLER's current UI scope (virtual 640x480 coords) —
// the function does NOT push its own scope. Returns the total height drawn
// (number_of_lines * line_advance) so callers can lay out below it.
// `fade` (0 = opaque .. 255 = invisible) fades the whole block in/out (text and
// glyphs together) — drive it from a menu fade timer for a consistent reveal.
float input_glyph_text_draw(const char* str, float x, float y, float wrap_width,
                            SLONG text_scale, unsigned long colour, SWORD fade = 0);

#endif // UI_INPUT_GLYPHS_H
