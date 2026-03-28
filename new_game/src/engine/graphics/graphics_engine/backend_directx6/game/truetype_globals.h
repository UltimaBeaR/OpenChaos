#ifndef ENGINE_GRAPHICS_GE_D3D_TRUETYPE_GLOBALS_H
#define ENGINE_GRAPHICS_GE_D3D_TRUETYPE_GLOBALS_H

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "engine/core/types.h"
#include "engine/graphics/text/truetype.h"
#include "engine/graphics/graphics_engine/backend_directx6/common/d3d_texture.h"

// uc_orig: CacheLine (fallen/DDEngine/Headers/truetype.h)
// One row-slice of a texture page used to cache a rendered line of text.
// D3D-specific: references D3DTexture directly.
struct CacheLine {
    TextCommand* owner; // owning TextCommand, NULL if free
    int sx, sy;         // screen position to blit to
    int width;          // width used in this slice
    int height;         // height of this slice

    D3DTexture* texture; // which texture page this slice lives in
    int y;               // Y offset within the texture page
};

// Anti-alias factor: text is rendered at 2x resolution then downsampled.
// uc_orig: AA_SIZE (fallen/DDEngine/Source/truetype.cpp)
#define TT_AA_SIZE 2

// Number of 256x256 texture pages allocated for the glyph cache.
// uc_orig: NUM_TT_PAGES (fallen/DDEngine/Source/truetype.cpp)
#define TT_NUM_PAGES 4

// Minimum and maximum allowed rendered font height in pixels.
// uc_orig: MIN_TT_HEIGHT (fallen/DDEngine/Source/truetype.cpp)
#define TT_MIN_HEIGHT 8
// uc_orig: MAX_TT_HEIGHT (fallen/DDEngine/Source/truetype.cpp)
#define TT_MAX_HEIGHT 64

// Derived: max cache lines = pages * (256 / min_height).
// uc_orig: MAX_LINES_PER_PAGE (fallen/DDEngine/Source/truetype.cpp)
#define TT_MAX_LINES_PER_PAGE (256 / TT_MIN_HEIGHT)
// uc_orig: MAX_CACHELINES (fallen/DDEngine/Source/truetype.cpp)
#define TT_MAX_CACHELINES (TT_MAX_LINES_PER_PAGE * TT_NUM_PAGES)

// Max simultaneous text commands (current + pending).
// uc_orig: MAX_TEXTCOMMANDS (fallen/DDEngine/Source/truetype.cpp)
#define TT_MAX_TEXTCOMMANDS 32

// Height of the font in pixels — set at init time from INI.
// uc_orig: FontHeight (fallen/DDEngine/Source/truetype.cpp)
extern int tt_FontHeight;

// Off-screen DirectDraw surface used as a GDI drawing target for text rendering.
// uc_orig: pShadowSurface (fallen/DDEngine/Source/truetype.cpp)
extern IDirectDrawSurface4* tt_pShadowSurface;

// Palette attached to the 8bpp shadow surface.
// uc_orig: pShadowPalette (fallen/DDEngine/Source/truetype.cpp)
extern IDirectDrawPalette* tt_pShadowPalette;

// GDI font handles: full size, 3/4 size, 1/2 size, and the previous font saved during init.
// uc_orig: hFont (fallen/DDEngine/Source/truetype.cpp)
extern HFONT tt_hFont;
// uc_orig: hMidFont (fallen/DDEngine/Source/truetype.cpp)
extern HFONT tt_hMidFont;
// uc_orig: hSmallFont (fallen/DDEngine/Source/truetype.cpp)
extern HFONT tt_hSmallFont;
// uc_orig: hOldFont (fallen/DDEngine/Source/truetype.cpp)
extern HFONT tt_hOldFont;

// Texture pages for the glyph cache.
// uc_orig: Texture (fallen/DDEngine/Source/truetype.cpp)
extern D3DTexture tt_Texture[TT_NUM_PAGES];

// Cache line descriptors — one entry per font-height-sized row of texture.
// uc_orig: Cache (fallen/DDEngine/Source/truetype.cpp)
extern CacheLine tt_Cache[TT_MAX_CACHELINES];

// Number of valid cache line entries (depends on FontHeight set at init).
// uc_orig: NumCacheLines (fallen/DDEngine/Source/truetype.cpp)
extern int tt_NumCacheLines;

// Maps 8bpp greyscale shadow surface pixel value to the current texture pixel format.
// uc_orig: PixMapping (fallen/DDEngine/Source/truetype.cpp)
extern UWORD tt_PixMapping[256];

// Active and pending text draw commands for this frame.
// uc_orig: Commands (fallen/DDEngine/Source/truetype.cpp)
extern TextCommand tt_Commands[TT_MAX_TEXTCOMMANDS];

#endif // ENGINE_GRAPHICS_GE_D3D_TRUETYPE_GLOBALS_H
