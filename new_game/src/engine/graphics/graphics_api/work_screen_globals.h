#ifndef ENGINE_GRAPHICS_GRAPHICS_API_WORK_SCREEN_GLOBALS_H
#define ENGINE_GRAPHICS_GRAPHICS_API_WORK_SCREEN_GLOBALS_H

#include "core/types.h"

// uc_orig: CurrentPalette (fallen/DDLibrary/Source/GWorkScreen.cpp)
extern UBYTE CurrentPalette[256 * 3];

// uc_orig: WorkScreenDepth (fallen/DDLibrary/Headers/GWorkScreen.h)
extern UBYTE WorkScreenDepth;
// uc_orig: WorkScreen (fallen/DDLibrary/Headers/GWorkScreen.h)
extern UBYTE* WorkScreen;
// uc_orig: WorkWindow (fallen/DDLibrary/Headers/GWorkScreen.h)
extern UBYTE* WorkWindow;
// uc_orig: WorkScreenHeight (fallen/DDLibrary/Headers/GWorkScreen.h)
extern SLONG WorkScreenHeight;
// uc_orig: WorkScreenWidth (fallen/DDLibrary/Headers/GWorkScreen.h)
extern SLONG WorkScreenWidth;
// uc_orig: WorkScreenPixelWidth (fallen/DDLibrary/Headers/GWorkScreen.h)
extern SLONG WorkScreenPixelWidth;
// uc_orig: WorkWindowHeight (fallen/DDLibrary/Headers/GWorkScreen.h)
extern SLONG WorkWindowHeight;
// uc_orig: WorkWindowWidth (fallen/DDLibrary/Headers/GWorkScreen.h)
extern SLONG WorkWindowWidth;
// uc_orig: WorkWindowRect (fallen/DDLibrary/Headers/GWorkScreen.h)
extern MFRect WorkWindowRect;

#endif // ENGINE_GRAPHICS_GRAPHICS_API_WORK_SCREEN_GLOBALS_H
