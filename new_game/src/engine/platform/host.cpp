#include "engine/platform/host.h"
#include "engine/platform/host_globals.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/platform/wind_procs_globals.h"  // app_inactive, restore_surfaces
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

// Platform globals (still needed by some legacy code).
extern volatile HWND hDDLibWindow;

#include "game/game_types.h"
#include "engine/audio/sound.h"
#include "engine/audio/mfx.h"
#include "engine/platform/uc_common.h"

// Forward declarations for keyboard subsystem (not yet migrated to new/).
BOOL SetupKeyboard(void);
void ResetKeyboard(void);
extern void ClearLatchedKeys();

// Forward declaration for best-found device initialisation.
void init_best_found(void);

// ---------------------------------------------------------------------------
// SDL3 event callbacks
// ---------------------------------------------------------------------------

static void on_key_down(uint8_t scancode)
{
    // Simulate the lParam that KeyboardProc expects:
    // bits 16-23 = scan code, bit 24 = extended flag, bit 31 = 0 (key down).
    uint8_t base = scancode;
    LPARAM lParam;
    if (scancode >= 0x80) {
        base = scancode - 0x80;
        lParam = ((LPARAM)base << 16) | (1 << 24); // extended key
    } else {
        lParam = ((LPARAM)base << 16);
    }
    extern LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
    KeyboardProc(WM_KEYDOWN, 0, lParam);
}

static void on_key_up(uint8_t scancode)
{
    uint8_t base = scancode;
    LPARAM lParam;
    if (scancode >= 0x80) {
        base = scancode - 0x80;
        lParam = ((LPARAM)base << 16) | (1 << 24) | (1u << 31); // extended + transition
    } else {
        lParam = ((LPARAM)base << 16) | (1u << 31); // transition bit = key up
    }
    extern LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
    KeyboardProc(WM_KEYUP, 0, lParam);
}

static void on_mouse_move(int x, int y)
{
    extern LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam);
    MouseProc(WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
}

static void on_mouse_button(int button, bool down, int x, int y)
{
    extern LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam);
    int msg;
    switch (button) {
    case 0: msg = down ? WM_LBUTTONDOWN : WM_LBUTTONUP; break;
    case 1: msg = down ? WM_RBUTTONDOWN : WM_RBUTTONUP; break;
    case 2: msg = down ? WM_MBUTTONDOWN : WM_MBUTTONUP; break;
    default: return;
    }
    MouseProc(msg, 0, MAKELPARAM(x, y));
}

static void on_focus_gained()
{
    app_inactive = UC_FALSE;
    restore_surfaces = UC_TRUE;
}

static void on_focus_lost()
{
    app_inactive = UC_TRUE;
}

static void on_window_moved()
{
    ge_update_display_rect(sdl3_window_get_native_handle(), ge_is_fullscreen());
}

static void on_window_resized()
{
    ge_update_display_rect(sdl3_window_get_native_handle(), ge_is_fullscreen());
}

static void on_close()
{
    GAME_STATE = 0;
}

// uc_orig: SetupHost (fallen/DDLibrary/Source/GHost.cpp)
// Initialises memory, keyboard, sound, and creates the SDL3 window.
BOOL SetupHost(ULONG flags)
{
    ShellActive = UC_FALSE;

    if (!SetupMemory())
        return UC_FALSE;
    if (!SetupKeyboard())
        return UC_FALSE;

    // Create the SDL3 window (hidden until GL context is ready).
    if (!sdl3_window_create("Urban Chaos", 640, 480)) {
        return UC_FALSE;
    }

    // Set hDDLibWindow for legacy code that still needs the native handle.
    hDDLibWindow = (HWND)sdl3_window_get_native_handle();

    // Register SDL3 event callbacks.
    SDL3_Callbacks cb = {};
    cb.on_key_down       = on_key_down;
    cb.on_key_up         = on_key_up;
    cb.on_mouse_move     = on_mouse_move;
    cb.on_mouse_button   = on_mouse_button;
    cb.on_focus_gained   = on_focus_gained;
    cb.on_focus_lost     = on_focus_lost;
    cb.on_window_moved   = on_window_moved;
    cb.on_window_resized = on_window_resized;
    cb.on_close          = on_close;
    sdl3_set_callbacks(&cb);

    // Initialise the sound manager; failure is non-fatal.
    MFX_init();

    hDDLibAccel = NULL;
    ShellActive = UC_TRUE;

    the_game.DarciStrength = 0;
    the_game.DarciStamina = 0;
    the_game.DarciSkill = 0;
    the_game.DarciConstitution = 0;

    return ShellActive;
}

// uc_orig: ResetHost (fallen/DDLibrary/Source/GHost.cpp)
// Shuts down sound, keyboard, memory, and destroys the SDL3 window.
void ResetHost(void)
{
    MFX_term();
    ResetKeyboard();
    ResetMemory();

    sdl3_window_destroy();
}

// uc_orig: ShellPaused (fallen/DDLibrary/Source/GHost.cpp)
void ShellPaused(void)
{
    return;
}

// uc_orig: ShellPauseOn (fallen/DDLibrary/Source/GHost.cpp)
void ShellPauseOn(void)
{
    ge_to_gdi();
    return;
}

// uc_orig: ShellPauseOff (fallen/DDLibrary/Source/GHost.cpp)
void ShellPauseOff(void)
{
    return;
}

// uc_orig: LibShellActive (fallen/DDLibrary/Source/GHost.cpp)
// Polls the SDL3 event queue, dispatching input/window events via callbacks.
// Returns UC_TRUE while the app is alive.
BOOL LibShellActive(void)
{
    ClearLatchedKeys();

    if (!sdl3_poll_events()) {
        ShellActive = UC_FALSE;
    }

    if (restore_surfaces) {
        ge_restore_all_surfaces();

        extern void FRONTEND_restore_screenfull_surfaces(void);
        FRONTEND_restore_screenfull_surfaces();

        restore_surfaces = UC_FALSE;
    }

    return ShellActive;
}

// uc_orig: LibShellChanged (fallen/DDLibrary/Source/GHost.cpp)
BOOL LibShellChanged(void)
{
    if (ge_is_display_changed()) {
        ge_clear_display_changed();
        return UC_TRUE;
    }
    return UC_FALSE;
}

// uc_orig: LibShellMessage (fallen/DDLibrary/Source/GHost.cpp)
BOOL LibShellMessage(const char* pMessage, const char* pFile, ULONG dwLine)
{
    if (pMessage == NULL) {
        pMessage = "Looks like a coder has caught a bug.";
    }

    fprintf(stderr, "=== Mucky Foot Message ===\n%s\nIn: %s line %u\n", pMessage, pFile, dwLine);

#ifdef _WIN32
    // On Windows, still show a message box for developer convenience.
    char buff[1024];
    snprintf(buff, sizeof(buff), "%s\n\nIn   : %s\nline : %u\n\nAbort=Kill, Retry=Debug, Ignore=Continue",
             pMessage, pFile, dwLine);
    ge_to_gdi();
    switch (MessageBoxA((HWND)sdl3_window_get_native_handle(), buff, "Mucky Foot Message",
                        MB_ABORTRETRYIGNORE | MB_ICONSTOP | MB_DEFBUTTON3)) {
    case IDABORT:  exit(1); break;
    case IDRETRY:  DebugBreak(); break;
    default: break;
    }
    ge_from_gdi();
#endif

    return UC_FALSE;
}

// uc_orig: Time (fallen/DDLibrary/Source/GHost.cpp)
void Time(MFTime* the_time)
{
#ifdef _WIN32
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
    the_time->Ticks = GetTickCount();
#else
    // TODO: cross-platform time implementation (Stage 8 follow-up)
    memset(the_time, 0, sizeof(*the_time));
#endif
}

// uc_orig: WinMain (fallen/DDLibrary/Source/GHost.cpp)
// Crash handler: writes crash_log.txt with exception info, RVA, and stack addresses.
#ifdef _WIN32
static LONG WINAPI crash_handler(EXCEPTION_POINTERS* ep)
{
    FILE* f = fopen("crash_log.txt", "w");
    if (!f) return EXCEPTION_CONTINUE_SEARCH;

    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "Crash at %04d-%02d-%02d %02d:%02d:%02d\n\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    HMODULE hmod = GetModuleHandle(NULL);
    uintptr_t base = (uintptr_t)hmod;

    fprintf(f, "Exception 0x%08lX at address 0x%p\n",
            ep->ExceptionRecord->ExceptionCode,
            ep->ExceptionRecord->ExceptionAddress);
    if (ep->ExceptionRecord->ExceptionCode == 0xC0000005) {
        fprintf(f, "Access violation %s address 0x%p\n",
                ep->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
                (void*)(uintptr_t)ep->ExceptionRecord->ExceptionInformation[1]);
    }
    fprintf(f, "Module base: 0x%p\n", (void*)hmod);
    fprintf(f, "Crash RVA: 0x%lx\n",
            (unsigned long)((uintptr_t)ep->ExceptionRecord->ExceptionAddress - base));

    CONTEXT* ctx = ep->ContextRecord;
    fprintf(f, "ESP=0x%08lx EBP=0x%08lx ECX=0x%08lx\n", ctx->Esp, ctx->Ebp, ctx->Ecx);

    fprintf(f, "\nStack return addresses (module RVAs):\n");
    DWORD* sp = (DWORD*)ctx->Esp;
    DWORD modEnd = (DWORD)(base + 0x400000);
    for (int i = 0; i < 64 && sp; i++) {
        __try {
            DWORD val = sp[i];
            if (val > (DWORD)base && val < modEnd) {
                fprintf(f, "  ESP[%d] RVA=0x%lx\n", i, (unsigned long)(val - base));
            }
        } __except(1) { break; }
    }

    fflush(f);
    fclose(f);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

int HOST_run(int argc_in, char* argv_in[])
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(crash_handler);
    hGlobalThisInst = GetModuleHandle(NULL);
#endif

    init_best_found();

    // Single-instance guard (Release only).
#if defined(NDEBUG) && defined(_WIN32)
    CreateEventA(NULL, UC_FALSE, UC_FALSE, "UrbanChaosExclusionZone");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 1;
    }
#endif

    return MF_main(argc, argv);
}

// uc_orig: TraceText (MFStdLib/Source/StdLib/StdFile.cpp)
void TraceText(char* fmt, ...)
{
    char message[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

#ifdef _WIN32
    OutputDebugStringA(message);
#else
    fprintf(stderr, "%s", message);
#endif
}
