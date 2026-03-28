#ifndef ENGINE_PLATFORM_WIND_PROCS_H
#define ENGINE_PLATFORM_WIND_PROCS_H

#include <windows.h>

// uc_orig: DDLibShellProc (fallen/DDLibrary/Source/WindProcs.cpp)
// Main window procedure: handles app activation, window resize/move, input, and quit messages.
LRESULT CALLBACK DDLibShellProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif // ENGINE_PLATFORM_WIND_PROCS_H
