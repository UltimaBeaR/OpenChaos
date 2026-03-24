#include "engine/platform/host_globals.h"

// uc_orig: iGlobalWinMode (fallen/DDLibrary/Source/GHost.cpp)
int iGlobalWinMode;

// uc_orig: ShellID (fallen/DDLibrary/Source/GHost.cpp)
DWORD ShellID;

// uc_orig: hDDLibAccel (fallen/DDLibrary/Source/GHost.cpp)
HACCEL hDDLibAccel;

// uc_orig: hDDLibThread (fallen/DDLibrary/Source/GHost.cpp)
HANDLE hDDLibThread;

// uc_orig: hGlobalPrevInst (fallen/DDLibrary/Source/GHost.cpp)
// uc_orig: hGlobalThisInst (fallen/DDLibrary/Source/GHost.cpp)
HINSTANCE hGlobalPrevInst,
    hGlobalThisInst;

// uc_orig: lpszGlobalArgs (fallen/DDLibrary/Source/GHost.cpp)
LPSTR lpszGlobalArgs;

// uc_orig: DDLibClass (fallen/DDLibrary/Source/GHost.cpp)
WNDCLASS DDLibClass;

// uc_orig: ShellActive (fallen/DDLibrary/Source/GHost.cpp)
volatile BOOL ShellActive;

// uc_orig: PauseFlags (fallen/DDLibrary/Source/GHost.cpp)
// uc_orig: PauseCount (fallen/DDLibrary/Source/GHost.cpp)
volatile ULONG PauseFlags = 0,
               PauseCount = 0;

// uc_orig: argc (fallen/DDLibrary/Source/GHost.cpp)
UWORD argc;

// uc_orig: argv (fallen/DDLibrary/Source/GHost.cpp)
LPTSTR argv[MAX_PATH];
