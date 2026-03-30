#ifndef ENGINE_PLATFORM_HOST_GLOBALS_H
#define ENGINE_PLATFORM_HOST_GLOBALS_H

#include <windows.h>
#include "engine/core/types.h"

// uc_orig: hGlobalThisInst (fallen/DDLibrary/Headers/GHost.h)
// Used by DX6 display backend for DirectDraw cooperative level.
extern HINSTANCE hGlobalThisInst;

// uc_orig: ShellActive (fallen/DDLibrary/Source/GHost.cpp)
// UC_TRUE while the event loop is running.
extern volatile BOOL ShellActive;

#endif // ENGINE_PLATFORM_HOST_GLOBALS_H
