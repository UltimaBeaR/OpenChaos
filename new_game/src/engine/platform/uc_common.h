// platform.h
// uc_orig: MFStdLib.h (MFStdLib/Headers/MFStdLib.h)
// Guy Simmons, 18th December 1997.
// Main platform abstraction header: Windows/DirectX includes, core types, host API.

#ifndef ENGINE_PLATFORM_UC_COMMON_H
#define ENGINE_PLATFORM_UC_COMMON_H

// Standard C includes.
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// Library defines.
#define _MF_WINDOWS

#ifndef _WIN32
#define _WIN32
#endif

#ifndef WIN32
#define WIN32
#endif

// Windows includes.
#define D3D_OVERLOADS
#include <windows.h>
#include <windowsx.h>
// D3D/DDraw headers removed from umbrella — include directly where needed.
// (Stage 7: renderer abstraction, D3D headers should only be in graphics_api/ and graphics_engine_d3d.cpp)

#include "engine/core/types.h"

// uc_orig: StdFile.h (MFStdLib/Headers/StdFile.h)
#include "engine/io/file.h"

// uc_orig: StdKeybd.h (MFStdLib/Headers/StdKeybd.h)
#include "engine/input/keyboard.h"

// uc_orig: StdMaths.h (MFStdLib/Headers/StdMaths.h)
#include "engine/core/math.h"

#include "engine/core/memory.h"

// uc_orig: StdMouse.h (MFStdLib/Headers/StdMouse.h)
#include "engine/input/mouse.h"

// Display
#define FLAGS_USE_3D (1 << 1)
#define FLAGS_USE_WORKSCREEN (1 << 2)

extern UBYTE WorkScreenDepth,
    *WorkScreen;
extern SLONG WorkScreenHeight,
    WorkScreenPixelWidth,
    WorkScreenWidth;
// Fixed virtual resolution — always 640x480 (actual window size is in RealDisplayWidth/Height).
#define DisplayWidth  640
#define DisplayHeight 480
extern SLONG DisplayBPP;

SLONG OpenDisplay(ULONG width, ULONG height, ULONG depth, ULONG flags);
SLONG SetDisplay(ULONG width, ULONG height, ULONG depth);
SLONG CloseDisplay(void);
SLONG ClearDisplay(UBYTE r, UBYTE g, UBYTE b);
void* LockWorkScreen(void);
void UnlockWorkScreen(void);
void ShowWorkScreen(ULONG flags);
void ClearWorkScreen(UBYTE colour);

// Host
#define H_CREATE_LOG (1 << 0)
#define SHELL_ACTIVE (LibShellActive())

#define main(ac, av) MF_main(ac, av)

struct MFTime {
    SLONG Hours,
        Minutes,
        Seconds,
        MSeconds;
    SLONG DayOfWeek, //	0 - 6;		Sunday		=	0
        Day,
        Month, //	1 - 12;		January		=	1
        Year;
    // claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
    DWORD Ticks; // Number of ticks(milliseconds) since windows started.
};

SLONG main(UWORD argc, TCHAR** argv);
BOOL SetupHost(ULONG flags);
void ResetHost(void);
void TraceText(char* error, ...);
BOOL LibShellActive(void);
BOOL LibShellChanged(void);
BOOL LibShellMessage(const char* pMessage, const char* pFile, ULONG dwLine);

#ifndef ASSERT
#define ASSERT(e) \
    {             \
    }
#endif

#include "engine/core/macros.h"

#endif // ENGINE_PLATFORM_UC_COMMON_H
