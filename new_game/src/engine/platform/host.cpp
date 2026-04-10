#include "engine/platform/host.h"
#include "engine/platform/host_globals.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/platform/wind_procs_globals.h"  // app_inactive, restore_surfaces
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#ifndef _WIN32
#include <execinfo.h>
#endif

#include "game/game_types.h"
#include "engine/audio/sound.h"
#include "engine/audio/mfx.h"
#include "engine/platform/uc_common.h"

// Forward declaration for best-found device initialisation.
void init_best_found(void);

// ---------------------------------------------------------------------------
// SDL3 event callbacks
// ---------------------------------------------------------------------------

static void on_key_down(uint8_t scancode)
{
    keyboard_key_down(scancode);
}

static void on_key_up(uint8_t scancode)
{
    keyboard_key_up(scancode);
}

static void on_mouse_move(int x, int y)
{
    mouse_on_move(x, y);
}

static void on_mouse_button(int button, bool down, int x, int y)
{
    mouse_on_button(button, down, x, y);
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

    // Set initial display rect from window position.
    ge_update_display_rect(sdl3_window_get_native_handle(), ge_is_fullscreen());

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
    return UC_FALSE;
}

// uc_orig: Time (fallen/DDLibrary/Source/GHost.cpp)
void Time(MFTime* the_time)
{
    time_t raw = time(nullptr);
    struct tm* lt = localtime(&raw);
    the_time->Hours = lt->tm_hour;
    the_time->Minutes = lt->tm_min;
    the_time->Seconds = lt->tm_sec;
    the_time->MSeconds = 0; // localtime doesn't provide milliseconds
    the_time->DayOfWeek = lt->tm_wday; // 0=Sunday, same as SYSTEMTIME
    the_time->Day = lt->tm_mday;
    the_time->Month = lt->tm_mon + 1; // tm_mon is 0-based, MFTime is 1-based
    the_time->Year = lt->tm_year + 1900;
    the_time->Ticks = sdl3_get_ticks();
}

// ---------------------------------------------------------------------------
// Exit / crash logging — writes crash_log.txt on ANY termination.
//
// Coverage:
//   - atexit handler    → normal return from main(), exit() calls
//   - SIGABRT handler   → abort() (not caught by Windows exception filter)
//   - Exception filter  → access violation, div-by-zero, etc. (Windows, crash_handler_win.cpp)
//   - Console handler   → Ctrl+C, console window close (Windows, crash_handler_win.cpp)
//   - Signal handlers   → SIGSEGV, SIGFPE, SIGABRT (non-Windows)
//
// A flag prevents multiple handlers from overwriting each other.
// ---------------------------------------------------------------------------

// Shared flag: set by whichever handler fires first.
// crash_handler_win.cpp also references this via extern.
volatile bool g_exit_log_written = false;

static void write_exit_timestamp(FILE* f, const char* label)
{
    time_t raw = time(nullptr);
    struct tm* lt = localtime(&raw);
    fprintf(f, "%s at %04d-%02d-%02d %02d:%02d:%02d\n\n",
            label,
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
            lt->tm_hour, lt->tm_min, lt->tm_sec);
}

// atexit handler: catches normal exit() and return from main().
static void exit_log_handler(void)
{
    if (g_exit_log_written) return;
    g_exit_log_written = true;

    FILE* f = fopen("crash_log.txt", "w");
    if (!f) return;

    write_exit_timestamp(f, "Clean exit");
    fprintf(f, "Type: Normal exit (returned from main or exit() called)\n");
    fflush(f);
    fclose(f);
}

// SIGABRT handler: catches abort() which bypasses both atexit and exception filters.
static void abort_signal_handler(int sig)
{
    if (g_exit_log_written) { signal(sig, SIG_DFL); raise(sig); return; }
    g_exit_log_written = true;

    FILE* f = fopen("crash_log.txt", "w");
    if (f) {
        write_exit_timestamp(f, "Crash (abort)");
        fprintf(f, "Type: abort() called (SIGABRT)\n");
        fflush(f);
        fclose(f);
    }

    signal(sig, SIG_DFL);
    raise(sig);
}

#ifdef _WIN32
// Defined in crash_handler_win.cpp (separate TU to isolate windows.h).
extern "C" void install_crash_handler(void);
#else
static void crash_signal_handler(int sig)
{
    if (g_exit_log_written) { signal(sig, SIG_DFL); raise(sig); return; }
    g_exit_log_written = true;

    FILE* f = fopen("crash_log.txt", "w");
    if (!f) { signal(sig, SIG_DFL); raise(sig); return; }

    write_exit_timestamp(f, "Crash (signal)");
    fprintf(f, "Signal: %d (%s)\n", sig,
            sig == SIGSEGV ? "SIGSEGV" : sig == SIGABRT ? "SIGABRT" :
            sig == SIGFPE  ? "SIGFPE"  : "unknown");

    void* frames[32];
    int n = backtrace(frames, 32);
    char** syms = backtrace_symbols(frames, n);
    fprintf(f, "\nStack trace:\n");
    for (int i = 0; i < n; i++)
        fprintf(f, "  [%2d] %s\n", i, syms ? syms[i] : "?");

    fflush(f);
    fclose(f);

    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

// ASSERT() failure handler: writes details to crash_log.txt and stderr, then aborts.
void uc_assert_fail(const char* expr, const char* file, int line)
{
    g_exit_log_written = true;

    FILE* f = fopen("crash_log.txt", "w");
    if (f) {
        write_exit_timestamp(f, "Crash (ASSERT)");
        fprintf(f, "Assertion failed: %s\n", expr);
        fprintf(f, "File: %s\n", file);
        fprintf(f, "Line: %d\n", line);
        fflush(f);
        fclose(f);
    }

    fprintf(stderr, "ASSERT failed: %s (%s:%d)\n", expr, file, line);
    abort();
}

int HOST_run(int argc_in, char* argv_in[])
{
    // Timestamp at the very start of stderr (redirected to stderr.log by Makefile)
    {
        time_t raw = time(nullptr);
        struct tm* lt = localtime(&raw);
        fprintf(stderr, "=== Session started: %04d-%02d-%02d %02d:%02d:%02d ===\n",
                lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                lt->tm_hour, lt->tm_min, lt->tm_sec);
    }

    // Register exit/crash handlers for all termination paths.
    atexit(exit_log_handler);
    signal(SIGABRT, abort_signal_handler);

#ifdef _WIN32
    install_crash_handler();
#else
    signal(SIGSEGV, crash_signal_handler);
    signal(SIGFPE,  crash_signal_handler);
#endif

    init_best_found();

    return MF_main((UWORD)argc_in, argv_in);
}

// uc_orig: TraceText (MFStdLib/Source/StdLib/StdFile.cpp)
void TraceText(char* fmt, ...)
{
    char message[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    fprintf(stderr, "%s", message);
}
