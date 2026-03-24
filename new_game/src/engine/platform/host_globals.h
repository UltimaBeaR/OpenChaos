#ifndef ENGINE_PLATFORM_HOST_GLOBALS_H
#define ENGINE_PLATFORM_HOST_GLOBALS_H

#include <windows.h>
#include "engine/core/types.h"

// uc_orig: hGlobalPrevInst (fallen/DDLibrary/Headers/GHost.h)
extern HINSTANCE hGlobalPrevInst;
// uc_orig: hGlobalThisInst (fallen/DDLibrary/Headers/GHost.h)
extern HINSTANCE hGlobalThisInst;

// uc_orig: iGlobalWinMode (fallen/DDLibrary/Source/GHost.cpp)
// Window show mode passed in from WinMain (SW_SHOW, SW_SHOWMAXIMIZED, etc.).
extern int iGlobalWinMode;

// uc_orig: ShellID (fallen/DDLibrary/Source/GHost.cpp)
// Thread ID of the shell message-pump thread.
extern DWORD ShellID;

// uc_orig: hDDLibAccel (fallen/DDLibrary/Source/GHost.cpp)
// Keyboard accelerator table handle (NULL — accelerators removed when DDlib.rc was deleted).
extern HACCEL hDDLibAccel;

// uc_orig: hDDLibThread (fallen/DDLibrary/Source/GHost.cpp)
// Handle to the DDLib shell thread.
extern HANDLE hDDLibThread;

// uc_orig: lpszGlobalArgs (fallen/DDLibrary/Source/GHost.cpp)
// Command-line argument string forwarded from WinMain.
extern LPSTR lpszGlobalArgs;

// uc_orig: DDLibClass (fallen/DDLibrary/Source/GHost.cpp)
// WNDCLASS descriptor filled in and registered by SetupHost.
extern WNDCLASS DDLibClass;

// uc_orig: ShellActive (fallen/DDLibrary/Source/GHost.cpp)
// UC_TRUE while the shell message loop is running. Written from the thread proc and read
// from the main loop, so declared volatile.
extern volatile BOOL ShellActive;

// uc_orig: PauseFlags (fallen/DDLibrary/Source/GHost.cpp)
// Bitmask used to synchronise pause handshaking between threads (volatile).
extern volatile ULONG PauseFlags;

// uc_orig: PauseCount (fallen/DDLibrary/Source/GHost.cpp)
// Nesting counter for ShellPauseOn/ShellPauseOff (volatile).
extern volatile ULONG PauseCount;

// uc_orig: argc (fallen/DDLibrary/Source/GHost.cpp)
// Argument count built from the WinMain command-line string for MF_main.
extern UWORD argc;

// uc_orig: argv (fallen/DDLibrary/Source/GHost.cpp)
// Argument vector built from the WinMain command-line string for MF_main.
extern LPTSTR argv[MAX_PATH];

#endif // ENGINE_PLATFORM_HOST_GLOBALS_H
