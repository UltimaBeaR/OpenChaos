#include "work_screen_globals.h"

// uc_orig: CurrentPalette (fallen/DDLibrary/Source/GWorkScreen.cpp)
UBYTE CurrentPalette[256 * 3],
    // uc_orig: WorkScreenDepth (fallen/DDLibrary/Source/GWorkScreen.cpp)
    WorkScreenDepth,
    // uc_orig: WorkScreen (fallen/DDLibrary/Source/GWorkScreen.cpp)
    *WorkScreen,
    // uc_orig: WorkWindow (fallen/DDLibrary/Source/GWorkScreen.cpp)
    *WorkWindow;

// uc_orig: WorkScreenHeight (fallen/DDLibrary/Source/GWorkScreen.cpp)
SLONG WorkScreenHeight,
    // uc_orig: WorkScreenWidth (fallen/DDLibrary/Source/GWorkScreen.cpp)
    WorkScreenWidth,
    // uc_orig: WorkScreenPixelWidth (fallen/DDLibrary/Source/GWorkScreen.cpp)
    WorkScreenPixelWidth,
    // uc_orig: WorkWindowHeight (fallen/DDLibrary/Source/GWorkScreen.cpp)
    WorkWindowHeight,
    // uc_orig: WorkWindowWidth (fallen/DDLibrary/Source/GWorkScreen.cpp)
    WorkWindowWidth;

// uc_orig: WorkWindowRect (fallen/DDLibrary/Source/GWorkScreen.cpp)
MFRect WorkWindowRect;
