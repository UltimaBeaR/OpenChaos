// platform.h
// uc_orig: MFStdLib.h (MFStdLib/Headers/MFStdLib.h)
// Guy Simmons, 18th December 1997.
// Main platform abstraction header: Windows/DirectX includes, core types, host API.

#ifndef PLATFORM_PLATFORM_H
#define PLATFORM_PLATFORM_H

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

// Windows/DirectX includes.
#define D3D_OVERLOADS
#include <windows.h>
#include <windowsx.h>
#include <d3dtypes.h>
#include <ddraw.h>
// For the DX8 headers, you need to define this to get old interfaces.
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0700
#endif
#include <dinput.h>
#include <d3d.h>

#include "core/types.h"

// uc_orig: StdFile.h (MFStdLib/Headers/StdFile.h)
#include "engine/io/file.h"

// uc_orig: StdKeybd.h (MFStdLib/Headers/StdKeybd.h)
#include "engine/input/keyboard.h"

// uc_orig: StdMaths.h (MFStdLib/Headers/StdMaths.h)
#include "core/math.h"

#include "core/memory.h"

// uc_orig: StdMouse.h (MFStdLib/Headers/StdMouse.h)
#include "engine/input/mouse.h"

// Display
#define FLAGS_USE_3DFX (1 << 0)
#define FLAGS_USE_3D (1 << 1)
#define FLAGS_USE_WORKSCREEN (1 << 2)

extern UBYTE WorkScreenDepth,
    *WorkScreen;
extern SLONG WorkScreenHeight,
    WorkScreenPixelWidth,
    WorkScreenWidth;
extern SLONG DisplayWidth,
    DisplayHeight,
    DisplayBPP;

SLONG OpenDisplay(ULONG width, ULONG height, ULONG depth, ULONG flags);
SLONG SetDisplay(ULONG width, ULONG height, ULONG depth);
SLONG CloseDisplay(void);
SLONG ClearDisplay(UBYTE r, UBYTE g, UBYTE b);
void FadeDisplay(UBYTE mode);
void* LockWorkScreen(void);
void UnlockWorkScreen(void);
void ShowWorkScreen(ULONG flags);
void ClearWorkScreen(UBYTE colour);

// Host
#define SHELL_NAME "Mucky Foot Shell\0"
#define H_CREATE_LOG (1 << 0)
#define SHELL_ACTIVE (LibShellActive())
#define SHELL_CHANGED (LibShellChanged())

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

#define NoError 0

#ifndef ASSERT
#define ASSERT(e) \
    {             \
    }
#endif

#include "core/macros.h"

#endif // PLATFORM_PLATFORM_H
