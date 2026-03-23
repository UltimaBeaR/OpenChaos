#include "engine/graphics/graphics_api/host.h"
#include "engine/graphics/graphics_api/host_globals.h"
#include "engine/graphics/graphics_api/wind_procs_globals.h"  // app_inactive, restore_surfaces
#include "engine/graphics/graphics_api/gd_display.h"   // the_display, hDDLibWindow
#include "engine/graphics/graphics_api/wind_procs.h"    // DDLibShellProc
#include "missions/game_types.h"
#include "engine/audio/sound.h"
#include "engine/audio/mfx.h"                  // MFX_init, MFX_term (already migrated)
#include <MFStdLib.h>                           // MFTime, SetupMemory, ResetMemory, MF_main

#define PAUSE_TIMEOUT 500
#define PAUSE         (1 << 0)
#define PAUSE_ACK     (1 << 1)

// Forward declarations for keyboard subsystem (not yet migrated to new/).
BOOL SetupKeyboard(void);
void ResetKeyboard(void);
extern void ClearLatchedKeys();

// Forward declaration for best-found device initialisation.
void init_best_found(void);

// uc_orig: DDLibThread (fallen/DDLibrary/Source/GHost.cpp)
// Shell thread entry point. Creates the main window, runs the Win32 message loop,
// and sets ShellActive to UC_FALSE when the loop exits.
static DWORD DDLibThread(LPVOID param)
{
    MSG msg;

    hDDLibWindow = CreateWindowEx(
        0,
        "Urban Chaos",
        "Urban Chaos",
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        HWND_DESKTOP,
        NULL,
        hGlobalThisInst,
        NULL);

    ShowWindow(hDDLibWindow, iGlobalWinMode);
    UpdateWindow(hDDLibWindow);

    ShellActive = UC_TRUE;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    ShellActive = UC_FALSE;

    return 0;
}

// uc_orig: SetupHost (fallen/DDLibrary/Source/GHost.cpp)
// Registers the window class, creates the main window, and initialises memory,
// keyboard and sound. Returns UC_TRUE if the window was created successfully.
BOOL SetupHost(ULONG flags)
{
    DWORD id;

    ShellActive = UC_FALSE;

    if (!SetupMemory())
        return UC_FALSE;
    if (!SetupKeyboard())
        return UC_FALSE;

    // Register the window class and create the shell window.
    DDLibClass.hInstance = hGlobalThisInst;
    DDLibClass.lpszClassName = TEXT("Urban Chaos");
    DDLibClass.lpfnWndProc = DDLibShellProc;
    DDLibClass.style = 0;
    DDLibClass.hIcon = NULL; // icon removed (DDlib.rc deleted)
    DDLibClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    DDLibClass.lpszMenuName = NULL;
    DDLibClass.cbClsExtra = 0;
    DDLibClass.cbWndExtra = 0;
    DDLibClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClass(&DDLibClass);

    hDDLibWindow = CreateWindowEx(
        0,
        "Urban Chaos",
        "Urban Chaos",
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        HWND_DESKTOP,
        NULL,
        hGlobalThisInst,
        NULL);

    if (hDDLibWindow) {
        // Initialise the sound manager; failure is non-fatal.
        MFX_init();

        // Keyboard accelerators removed (DDlib.rc deleted).
        hDDLibAccel = NULL;

        ShellActive = UC_TRUE;
    }

    the_game.DarciStrength = 0;
    the_game.DarciStamina = 0;
    the_game.DarciSkill = 0;
    the_game.DarciConstitution = 0;

    return ShellActive;
}

// uc_orig: ResetHost (fallen/DDLibrary/Source/GHost.cpp)
// Shuts down sound, keyboard, and memory, then unregisters the window class.
void ResetHost(void)
{
    MFX_term();

    ResetKeyboard();
    ResetMemory();

    UnregisterClass(TEXT("Urban Chaos"), GetModuleHandle(NULL));
}

// uc_orig: ShellPaused (fallen/DDLibrary/Source/GHost.cpp)
// Game-thread side of the pause handshake: acknowledges a pending pause request and
// spins until the pause is lifted. The early return means the pause logic is currently
// disabled — preserved as-is from the original.
void ShellPaused(void)
{
    return;

    // claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
    DWORD timeout;

    if (PauseFlags & PAUSE) {
        PauseFlags |= PAUSE_ACK;
        timeout = GetTickCount();
        while (PauseFlags & PAUSE)
        {
            if (Keys[KB_L]) {
                LibShellMessage("ShellPauseOff: Timeout", __FILE__, __LINE__);
            }
        }
        PauseFlags |= PAUSE_ACK;
    }
}

// uc_orig: ShellPauseOn (fallen/DDLibrary/Source/GHost.cpp)
// Requests a pause of the game thread and waits for acknowledgement, then switches
// the display to GDI mode. The early return after toGDI means the synchronisation
// logic is currently disabled — preserved as-is from the original.
void ShellPauseOn(void)
{
    the_display.toGDI();
    return;

    // claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
    DWORD timeout;

    PauseCount++;
    if (PauseCount == 1) {
        PauseFlags |= PAUSE;
        timeout = GetTickCount();
        while (!(PauseFlags & PAUSE_ACK))
        {
            if (Keys[KB_L]) {
                LibShellMessage("ShellPauseOff: Timeout", __FILE__, __LINE__);
            }
        }
        PauseFlags &= ~PAUSE_ACK;

        the_display.toGDI();
    }
}

// uc_orig: ShellPauseOff (fallen/DDLibrary/Source/GHost.cpp)
// Resumes the game thread after a ShellPauseOn call. The early return means the
// synchronisation logic is currently disabled — preserved as-is from the original.
void ShellPauseOff(void)
{
    return;

    // claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
    DWORD timeout;

    if (PauseCount == 0)
        return;

    if (PauseCount == 1) {
        the_display.fromGDI();

        PauseFlags &= ~PAUSE;
        timeout = GetTickCount();
        while (!(PauseFlags & PAUSE_ACK))
        {
            if (Keys[KB_L]) {
                LibShellMessage("ShellPauseOff: Timeout", __FILE__, __LINE__);
            }
        }
        PauseFlags &= ~PAUSE_ACK;
    }
    PauseCount--;
}

// uc_orig: LibShellActive (fallen/DDLibrary/Source/GHost.cpp)
// Pumps the Windows message queue, processing all pending messages. While the app is
// inactive and in fullscreen, sleeps 100 ms per iteration to yield CPU. Restores all
// DirectDraw surfaces (including frontend fullscreen surfaces) when restore_surfaces is
// set after the window regains focus. Returns UC_TRUE while the shell window is alive.
BOOL LibShellActive(void)
{
    SLONG result;
    MSG msg;

    // Release any keys that were held when focus was lost.
    ClearLatchedKeys();

    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            result = (SLONG)GetMessage(&msg, NULL, 0, 0);
            if (result) {
                if (!TranslateAccelerator(hDDLibWindow, hDDLibAccel, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } else {
                ShellActive = UC_FALSE;
            }
        }

        if (app_inactive && the_display.lp_DD4 && the_display.IsFullScreen()) {
            Sleep(100);
        } else {
            break;
        }
    }

    if (restore_surfaces) {
        if (the_display.lp_DD4) {
            the_display.lp_DD4->RestoreAllSurfaces();

            extern void FRONTEND_restore_screenfull_surfaces(void);

            FRONTEND_restore_screenfull_surfaces();
        }

        restore_surfaces = UC_FALSE;
    }

    return ShellActive;
}

// uc_orig: LibShellChanged (fallen/DDLibrary/Source/GHost.cpp)
// Returns UC_TRUE if the display configuration changed since the last call and clears the
// changed flag so subsequent calls return UC_FALSE until the next change.
BOOL LibShellChanged(void)
{
    if (the_display.IsDisplayChanged()) {
        the_display.DisplayChangedOff();
        return UC_TRUE;
    }
    return UC_FALSE;
}

// uc_orig: LibShellMessage (fallen/DDLibrary/Source/GHost.cpp)
// Pops an Abort/Retry/Ignore message box with the supplied message, source file, and
// line number. Abort calls exit(1), Retry triggers __debugbreak, Ignore continues.
BOOL LibShellMessage(const char* pMessage, const char* pFile, ULONG dwLine)
{
    BOOL result;
    CBYTE buff1[512],
        buff2[512];
    ULONG flag;

    if (pMessage == NULL) {
        pMessage = "Looks like a coder has caught a bug.";
    }

    wsprintf(buff1, "Uh oh, something bad's happened!");
    wsprintf(buff2, "%s\n\n%s\n\nIn   : %s\nline : %u", buff1, pMessage, pFile, dwLine);
    strcat(buff2, "\n\nAbort=Kill Application, Retry=Debug, Ignore=Continue");
    flag = MB_ABORTRETRYIGNORE | MB_ICONSTOP | MB_DEFBUTTON3;

    result = UC_FALSE;
    the_display.toGDI();
    switch (MessageBox(hDDLibWindow, buff2, "Mucky Foot Message", flag)) {
    case IDABORT:
        exit(1);
        break;
    case IDCANCEL:
    case IDIGNORE:
        break;
    case IDRETRY:
        DebugBreak();
        break;
    }

    the_display.fromGDI();

    return result;
}

// uc_orig: Time (fallen/DDLibrary/Source/GHost.cpp)
// Fills the MFTime structure with the current local time. Note: Ticks wraps after
// ~49 days (GetTickCount overflow).
void Time(MFTime* the_time)
{
    SYSTEMTIME new_time;

    GetLocalTime(&new_time);
    the_time->Hours = new_time.wHour;
    the_time->Minutes = new_time.wMinute;
    the_time->Seconds = new_time.wSecond;
    the_time->MSeconds = new_time.wMilliseconds;
    the_time->DayOfWeek = new_time.wDayOfWeek;
    the_time->Day = new_time.wDay;
    the_time->Month = new_time.wMonth;
    the_time->Year = new_time.wYear;
    the_time->Ticks = GetTickCount(); // Identified as an overflow issue that occurs every 49 days
}

// uc_orig: WinMain (fallen/DDLibrary/Source/GHost.cpp)
// Real entry point implementation. Stores WinMain parameters in globals, then uses a named
// event to ensure only one instance of Urban Chaos can run at a time before calling
// MF_main. Called from src/main.cpp.
int HOST_run(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPTSTR lpszArgs, int iWinMode)
{
    // Store WinMain parameters for use by the rest of the engine.
    lpszGlobalArgs = lpszArgs;
    iGlobalWinMode = iWinMode;
    hGlobalThisInst = hThisInst;
    hGlobalPrevInst = hPrevInst;

    init_best_found();

    // Create a named event so only one instance can run at a time.
    // CreateEventA always succeeds; if the event already existed ERROR_ALREADY_EXISTS
    // is set, meaning another instance is running — bail out.
    // The event is automatically destroyed when the process exits.
    HANDLE hEvent = CreateEventA(NULL, UC_FALSE, UC_FALSE, "UrbanChaosExclusionZone");
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
        return MF_main(argc, argv);
    }

    return ERROR_ALREADY_EXISTS;
}

// uc_orig: TraceText (MFStdLib/Source/StdLib/StdFile.cpp)
// Formats a message string and sends it to the debugger via OutputDebugString.
// Used via the TRACE macro throughout the codebase for debug logging.
void TraceText(char* fmt, ...)
{
    char message[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    OutputDebugString(message);
}
