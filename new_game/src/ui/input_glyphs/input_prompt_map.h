#ifndef UI_INPUT_GLYPHS_INPUT_PROMPT_MAP_H
#define UI_INPUT_GLYPHS_INPUT_PROMPT_MAP_H

// Glyph map for input prompts. One row per glyph PER DEVICE — nothing is unified
// across devices: a keyboard key, an Xbox button and a DualSense button are
// separate entries with their own id, label and cell. Help text is written per
// device too, and references glyphs by `id` as an inline token, e.g. {kb_w},
// {xb_a}, {ps_cross}.
//
// Each row's cell is a 0-based {col, row} in that device's atlas PNG. All atlases
// are uniform 64px grids; cells are addressed by raw grid position (any glyph in
// the PNG is reachable). NOTE the atlas texture is loaded V-FLIPPED, so `row`
// counts FROM THE BOTTOM (see input_glyph_draw_cell). To convert a metadata pixel
// (x, y-from-top): col = x/64, row = (atlas_px/64 - 1) - y/64.
//
// id prefixes: kb_ (keyboard), ms_ (mouse), xb_ (Xbox), ps_ (PlayStation).

enum GlyphDevice {
    GLYPH_DEV_KBM,  // keyboard & mouse atlas
    GLYPH_DEV_XBOX, // generic (Xbox-style) atlas
    GLYPH_DEV_PS,   // PlayStation atlas
};

struct InputGlyph {
    const char* id;    // inline-token id used in help text (unique)
    const char* label; // human label shown in the dev catalog
    GlyphDevice dev;    // which atlas this glyph lives in
    int col;            // atlas cell column (0-based)
    int row;            // atlas cell row (0-based, counted from the bottom)
};

extern const InputGlyph INPUT_GLYPHS[];
extern const int INPUT_GLYPH_COUNT;

// Look up a glyph by id (name not NUL-terminated, length `len`). Null if unknown.
const InputGlyph* input_glyph_find(const char* id, unsigned long len);

#endif // UI_INPUT_GLYPHS_INPUT_PROMPT_MAP_H
