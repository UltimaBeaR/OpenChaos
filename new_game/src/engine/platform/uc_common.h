// platform.h
// uc_orig: MFStdLib.h (MFStdLib/Headers/MFStdLib.h)
// Guy Simmons, 18th December 1997.
// Main platform abstraction header: core types, host API.

#ifndef ENGINE_PLATFORM_UC_COMMON_H
#define ENGINE_PLATFORM_UC_COMMON_H

// Standard C includes.
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

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

// Fixed virtual resolution — always 640x480 (actual window size is in ScreenWidth/Height).
#define DisplayWidth  640
#define DisplayHeight 480
extern SLONG DisplayBPP;

SLONG OpenDisplay(ULONG width, ULONG height, ULONG depth, ULONG flags);
SLONG CloseDisplay(void);

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
    // BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t (SDL_GetTicks returns uint64_t)
    uint64_t Ticks; // Milliseconds since SDL init (was GetTickCount)
};

SLONG main(UWORD argc, char** argv);
BOOL SetupHost(ULONG flags);
void ResetHost(void);
void TraceText(char* error, ...);
BOOL LibShellActive(void);
BOOL LibShellChanged(void);
BOOL LibShellMessage(const char* pMessage, const char* pFile, ULONG dwLine);

// Assert: logs condition, file, line to crash_log.txt and stderr, then aborts.
// Writes crash_log.txt directly (before abort's SIGABRT handler) so details are preserved.
#ifndef ASSERT
void uc_assert_fail(const char* expr, const char* file, int line);
#define ASSERT(e) \
    do { if (!(e)) uc_assert_fail(#e, __FILE__, __LINE__); } while(0)
#endif

#include "engine/core/macros.h"

#endif // ENGINE_PLATFORM_UC_COMMON_H
