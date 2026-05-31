#ifndef UI_INPUT_GLYPHS_H
#define UI_INPUT_GLYPHS_H

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
float input_glyph_draw(InputGlyphKey key, float x, float y, float line_height);

#endif // UI_INPUT_GLYPHS_H
