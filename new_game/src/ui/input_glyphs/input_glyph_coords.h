#ifndef UI_INPUT_GLYPHS_INPUT_GLYPH_COORDS_H
#define UI_INPUT_GLYPHS_INPUT_GLYPH_COORDS_H

// Auto-generated once from the Kenney Input Prompts atlas XMLs (CC0).
// The source XMLs are NOT kept in the repo — regenerate from the pack if needed.
// Each entry is one glyph cell in the matching atlas (pixel coords).
// Atlas/page mapping: KEYBOARD=keyboard&mouse, GENERIC=Xbox-style,
// PS=PlayStation-style, DECK=Steam Deck-style (see input_glyphs.h).

struct InputGlyphRect {
    const char* name;
    short x, y, w, h;
};

constexpr int INPUT_GLYPH_KEYBOARD_ATLAS_PX = 1024;
extern const InputGlyphRect INPUT_GLYPH_KEYBOARD[];
constexpr int INPUT_GLYPH_KEYBOARD_COUNT = 243;

constexpr int INPUT_GLYPH_GENERIC_ATLAS_PX = 640;
extern const InputGlyphRect INPUT_GLYPH_GENERIC[];
constexpr int INPUT_GLYPH_GENERIC_COUNT = 99;

constexpr int INPUT_GLYPH_PS_ATLAS_PX = 768;
extern const InputGlyphRect INPUT_GLYPH_PS[];
constexpr int INPUT_GLYPH_PS_COUNT = 134;

constexpr int INPUT_GLYPH_DECK_ATLAS_PX = 704;
extern const InputGlyphRect INPUT_GLYPH_DECK[];
constexpr int INPUT_GLYPH_DECK_COUNT = 116;

#endif // UI_INPUT_GLYPHS_INPUT_GLYPH_COORDS_H
