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

#endif // UI_INPUT_GLYPHS_H
