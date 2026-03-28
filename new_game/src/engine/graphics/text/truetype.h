#ifndef ENGINE_GRAPHICS_TEXT_TRUETYPE_H
#define ENGINE_GRAPHICS_TEXT_TRUETYPE_H

#include "engine/core/types.h"

// If TRUETYPE is defined, TrueType fonts are used; otherwise the bitmap font system is used.
// uc_orig: TRUETYPE (fallen/DDEngine/Headers/truetype.h)
// #define TRUETYPE

// Maximum bytes of text data per TextCommand.
// uc_orig: MAX_TT_TEXT (fallen/DDEngine/Headers/truetype.h)
#define MAX_TT_TEXT (2048 - 48)

// uc_orig: TextCommand (fallen/DDEngine/Headers/truetype.h)
// A single queued text draw request — stores all parameters needed to render
// and cache one string.
struct TextCommand {
    char data[MAX_TT_TEXT]; // text content
    int nbytes;             // byte length of data
    int nchars;             // character count (MBCS-aware)
    int x, y;              // screen origin
    int rx;                 // right margin
    int scale;              // scale factor (256 = 1x)
    ULONG rgb;              // text colour
    int command;            // justification mode (see TextCommands)
    int validity;           // slot state (see Validity)
    bool in_cache;          // whether this command's glyphs are cached
    int lines;              // number of wrapped lines
    int fwidth;             // formatted width in pixels
};

// uc_orig: TextCommands (fallen/DDEngine/Headers/truetype.h)
// Justification modes for DrawTextTT.
enum TextCommands {
    LeftJustify  = 0,
    RightJustify,
    Centred,
};

// uc_orig: Validity (fallen/DDEngine/Headers/truetype.h)
// State of a TextCommand slot.
enum Validity {
    Free    = 0,  // slot is available
    Current,      // slot is in use this frame
    Pending,      // slot was used last frame, pending cleanup
};

// Initialise TrueType rendering — creates shadow surface, font, and texture cache.
// Must be called before any DrawTextTT calls.
// uc_orig: TT_Init (fallen/DDEngine/Headers/truetype.h)
extern void TT_Init();

// Release all TrueType resources.
// uc_orig: TT_Term (fallen/DDEngine/Headers/truetype.h)
extern void TT_Term();

// Call once per frame before flipping: processes pending commands and blits cached text.
// uc_orig: PreFlipTT (fallen/DDEngine/Headers/truetype.h)
extern void PreFlipTT();

// Queue a text draw request. Returns the Y coordinate of the line below the last drawn line.
// 'width' (optional) receives the right edge X after drawing.
// uc_orig: DrawTextTT (fallen/DDEngine/Headers/truetype.h)
extern int DrawTextTT(char* string, int x, int y, int rx, int scale, ULONG rgb, int command, long* width = NULL);

// Returns the height of the TrueType font in pixels.
// uc_orig: GetTextHeightTT (fallen/DDEngine/Headers/truetype.h)
extern int GetTextHeightTT();

#endif // ENGINE_GRAPHICS_TEXT_TRUETYPE_H
