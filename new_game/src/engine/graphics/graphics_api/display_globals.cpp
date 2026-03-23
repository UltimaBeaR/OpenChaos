// Definitions of globals declared in gd_display.h and display_globals.h.
// All non-static globals from GDisplay.cpp live here.

#include "engine/graphics/graphics_api/display_globals.h"
#include "engine/graphics/graphics_api/gd_display.h"

// uc_orig: RealDisplayWidth (fallen/DDLibrary/Source/GDisplay.cpp)
SLONG RealDisplayWidth = 0;
// uc_orig: RealDisplayHeight (fallen/DDLibrary/Source/GDisplay.cpp)
SLONG RealDisplayHeight = 0;
// uc_orig: DisplayBPP (fallen/DDLibrary/Source/GDisplay.cpp)
SLONG DisplayBPP = 0;

// uc_orig: the_display (fallen/DDLibrary/Source/GDisplay.cpp)
Display the_display;

// uc_orig: hDDLibStyle (fallen/DDLibrary/Source/GDisplay.cpp)
volatile SLONG hDDLibStyle = 0;
// uc_orig: hDDLibStyleEx (fallen/DDLibrary/Source/GDisplay.cpp)
volatile SLONG hDDLibStyleEx = 0;
// uc_orig: hDDLibWindow (fallen/DDLibrary/Source/GDisplay.cpp)
volatile HWND hDDLibWindow = NULL;
// uc_orig: hDDLibMenu (fallen/DDLibrary/Source/GDisplay.cpp)
volatile HMENU hDDLibMenu = NULL;

// uc_orig: VideoRes (fallen/DDLibrary/Source/GDisplay.cpp)
int VideoRes = -1;

// uc_orig: eDisplayType (fallen/DDLibrary/Source/GDisplay.cpp)
enumDisplayType eDisplayType;

// uc_orig: image_mem (fallen/DDLibrary/Source/GDisplay.cpp)
UBYTE* image_mem = NULL;

// uc_orig: image (fallen/DDLibrary/Source/GDisplay.cpp)
UBYTE* image = NULL;

// uc_orig: mirror (fallen/DDLibrary/Source/GDisplay.cpp)
LPDIRECTDRAWSURFACE4 mirror = NULL;

// uc_orig: m_lpLastBackground (fallen/DDLibrary/Source/GDisplay.cpp)
LPDIRECTDRAWSURFACE4 m_lpLastBackground = NULL;

// uc_orig: clumpfile (fallen/DDLibrary/Source/GDisplay.cpp)
char clumpfile[MAX_PATH] = "";
// uc_orig: clumpsize (fallen/DDLibrary/Source/GDisplay.cpp)
size_t clumpsize = 0;
