#include "engine/graphics/text/truetype_globals.h"

// uc_orig: FontHeight (fallen/DDEngine/Source/truetype.cpp)
int tt_FontHeight = 0;

// uc_orig: pShadowSurface (fallen/DDEngine/Source/truetype.cpp)
IDirectDrawSurface4* tt_pShadowSurface = nullptr;

// uc_orig: pShadowPalette (fallen/DDEngine/Source/truetype.cpp)
IDirectDrawPalette* tt_pShadowPalette = nullptr;

// uc_orig: hFont (fallen/DDEngine/Source/truetype.cpp)
HFONT tt_hFont = nullptr;

// uc_orig: hMidFont (fallen/DDEngine/Source/truetype.cpp)
HFONT tt_hMidFont = nullptr;

// uc_orig: hSmallFont (fallen/DDEngine/Source/truetype.cpp)
HFONT tt_hSmallFont = nullptr;

// uc_orig: hOldFont (fallen/DDEngine/Source/truetype.cpp)
HFONT tt_hOldFont = nullptr;

// uc_orig: Texture (fallen/DDEngine/Source/truetype.cpp)
D3DTexture tt_Texture[TT_NUM_PAGES];

// uc_orig: Cache (fallen/DDEngine/Source/truetype.cpp)
CacheLine tt_Cache[TT_MAX_CACHELINES];

// uc_orig: NumCacheLines (fallen/DDEngine/Source/truetype.cpp)
int tt_NumCacheLines = 0;

// uc_orig: PixMapping (fallen/DDEngine/Source/truetype.cpp)
UWORD tt_PixMapping[256] = {};

// uc_orig: Commands (fallen/DDEngine/Source/truetype.cpp)
TextCommand tt_Commands[TT_MAX_TEXTCOMMANDS];
