#ifndef UI_INPUT_GLYPHS_INPUT_PROMPT_MAP_H
#define UI_INPUT_GLYPHS_INPUT_PROMPT_MAP_H

// Editable glyph mapping for input prompts (catalog + future help texts).
//
// Each input has a display name plus, per device atlas, the glyph CELL to use.
// All atlases are uniform grids of 64px cells (each atlas has its own column/row
// COUNT, but the cell size is the same). A cell is a 0-based {col, row} measured
// from the atlas PNG's top-left — so to set a glyph you just open the PNG, count
// the cell, and write {col, row}. Addressing by raw cell (not by metadata sprite
// name) lets you point at ANY glyph in the image, including ones the metadata
// table doesn't name.
//
// HOW TO FILL: every controller/keyboard/mouse cell below starts as {0,0} (a
// placeholder — the top-left cell). Replace each with the real {col, row}. Use
// {INPUT_PROMPT_NONE, INPUT_PROMPT_NONE} when an input has no glyph on that
// device (the catalog then skips it for that device).
//
// Atlases (see input_glyphs.cpp): kbm = keyboard & mouse, xbox = generic
// (Xbox-style ABXY), ps = PlayStation.

#define INPUT_PROMPT_NONE (-1)

struct InputPromptCell {
    int col;
    int row;
};

struct InputPrompt {
    const char* name; // label shown in the catalog
    InputPromptCell kbm;  // cell in the keyboard & mouse atlas
    InputPromptCell xbox; // cell in the generic (Xbox-style) atlas
    InputPromptCell ps;   // cell in the PlayStation atlas
};

extern const InputPrompt INPUT_PROMPTS[];
extern const int INPUT_PROMPT_COUNT;

#endif // UI_INPUT_GLYPHS_INPUT_PROMPT_MAP_H
