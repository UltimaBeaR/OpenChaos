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
    ge_update_display_rect(nullptr, ge_is_fullscreen());
}

static void on_window_resized()
{
    ge_update_display_rect(nullptr, ge_is_fullscreen());
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

// Crash handler: writes crash_log.txt with signal info.
static void crash_signal_handler(int sig)
{
    FILE* f = fopen("crash_log.txt", "w");
    if (!f) { signal(sig, SIG_DFL); raise(sig); return; }

    time_t raw = time(nullptr);
    struct tm* lt = localtime(&raw);
    fprintf(f, "Crash at %04d-%02d-%02d %02d:%02d:%02d\n\n",
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
            lt->tm_hour, lt->tm_min, lt->tm_sec);
    fprintf(f, "Signal: %d (%s)\n", sig,
            sig == SIGSEGV ? "SIGSEGV" : sig == SIGABRT ? "SIGABRT" :
            sig == SIGFPE  ? "SIGFPE"  : "unknown");
    fflush(f);
    fclose(f);

    signal(sig, SIG_DFL);
    raise(sig);
}

int HOST_run(int argc_in, char* argv_in[])
{
    signal(SIGSEGV, crash_signal_handler);
    signal(SIGABRT, crash_signal_handler);
    signal(SIGFPE,  crash_signal_handler);

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
