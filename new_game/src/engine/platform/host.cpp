#include "engine/platform/host.h"
#include "engine/platform/host_globals.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/io/oc_config.h"
#include "engine/platform/wind_procs_globals.h" // app_inactive
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/input/keyboard.h"
#include "engine/input/mouse.h"
#include "engine/input/mouse_capture.h"
#include "engine/input/input_frame.h"

#include <stdio.h>
#include <signal.h>
#ifndef _WIN32
#include <execinfo.h>
#endif

#include "game/game_types.h"
#include "engine/audio/mfx.h"
#include "engine/platform/uc_common.h"

// Forward declaration for best-found device initialisation.
void init_best_found(void);

// Windows accessibility-shortcut suppression (Sticky/Filter/Toggle Keys popups).
// Real implementation in crash_handler_win.cpp; no-ops elsewhere. Disabled while
// the game window is focused, restored on focus loss / exit. See that file.
#ifdef _WIN32
extern "C" void win_disable_accessibility_shortcuts(void);
extern "C" void win_restore_accessibility_shortcuts(void);
#else
static inline void win_disable_accessibility_shortcuts(void) {}
static inline void win_restore_accessibility_shortcuts(void) {}
#endif

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

static void on_mouse_move(int x, int y, int xrel, int yrel)
{
    mouse_on_move(x, y, xrel, yrel);
}

static void on_mouse_button(int button, bool down, int /*x*/, int /*y*/)
{
    // Capture state machine consumes the engagement click. Subsequent
    // clicks (capture already engaged) pass through to input_frame, which
    // exposes them to gameplay via input_mouse_btn_*(). The wait-for-
    // release gate inside input_frame (armed on UI overlay close by
    // input_consume_all_held_until_released) prevents a held button from
    // leaking across overlay transitions.
    if (mouse_capture_on_button(button, down))
        return;
    if (down)
        input_frame_on_mouse_button_down(button);
    else
        input_frame_on_mouse_button_up(button);
}

void host_on_focus_changed(bool focused)
{
    if (focused) {
        app_inactive = UC_FALSE;
        // In fullscreen there's no reason for the OS cursor to be
        // visible — the game covers the entire screen. In windowed
        // mode the cursor stays visible by default so the user can
        // interact with the title bar / desktop without alt-tabbing.
        // (Capture for camera control toggles SDL relative mouse mode
        // independently — that hides the cursor regardless of this
        // setting.)
        if (ge_is_fullscreen())
            sdl3_hide_cursor();
    } else {
        app_inactive = UC_TRUE;
        // Show the cursor while focus is elsewhere so the user can
        // interact with whatever they alt-tabbed into.
        if (ge_is_fullscreen())
            sdl3_show_cursor();
    }
}

static void on_focus_gained()
{
    host_on_focus_changed(true);
    // Suppress the Windows Sticky/Filter/Toggle Keys popups while we have focus.
    win_disable_accessibility_shortcuts();
}

static void on_focus_lost()
{
    host_on_focus_changed(false);
    // Restore the user's accessibility settings so they work on the desktop.
    win_restore_accessibility_shortcuts();
}

static void on_window_moved()
{
    ge_update_display_rect(sdl3_window_get_native_handle(), ge_is_fullscreen());
}

// Runtime window resize is coalesced naturally by the event-pump cadence.
// `host_process_pending_resize` runs once per LibShellActive iteration,
// AFTER `sdl3_poll_events` has drained every queued resize event into our
// latch — so no matter how many WM_SIZE / configure events arrived
// (Windows modal loop drops the whole burst on release; Mac/Linux drips
// one per frame during a smooth drag), we re-create the FBO at most once
// per frame. The video player and any other secondary loop that pumps
// SDL events itself (bypassing LibShellActive) MUST also call
// host_process_pending_resize from inside its loop, otherwise resizes
// queued during the loop never get applied. No timer-based debounce
// needed; that previously delayed the post-release apply by 150 ms and
// made the old-aspect FBO visible (with composition letterbox bars) for
// ~9 frames.
static bool s_resize_pending = false;

static void on_window_resized()
{
    ge_update_display_rect(sdl3_window_get_native_handle(), ge_is_fullscreen());
    s_resize_pending = true;
}

void host_process_pending_resize(void)
{
    if (!s_resize_pending)
        return;
    ge_resize_display();
    s_resize_pending = false;

    if (!ge_is_fullscreen()) {
        bool maximized = sdl3_window_is_maximized();
        OC_CONFIG_set_int("video", "windowed_maximized", maximized ? 1 : 0);
        if (!maximized) {
            int w = 0, h = 0;
            sdl3_window_get_size(&w, &h);
            if (w > 0 && h > 0) {
                OC_CONFIG_set_int("video", "windowed_width", w);
                OC_CONFIG_set_int("video", "windowed_height", h);
            }
        }
    }
}

// Live-drag repaint — fires from SDL_AddEventWatch synchronously while
// the OS is in its modal resize loop (Windows edge-drag) AND for every
// regular resize event push. The main game loop may or may not be
// pumping at this moment (it's frozen during the modal loop, and the
// video player runs its own SDL_PollEvent loop bypassing LibShellActive
// entirely), so we use this single-source-of-truth path for two jobs:
//
//   1. Paint the new client area immediately via ge_present_for_drag —
//      otherwise the OS reveals black where the window expanded and
//      shows nothing until somebody else swaps.
//
//   2. Latch s_resize_pending so the post-event FBO recreation happens
//      whenever some pump path next calls host_process_pending_resize.
//      Without this latch a resize that happens during a video would
//      never get applied (video_player's raw SDL_PollEvent doesn't
//      route to on_window_resized), and the menu after the video would
//      keep rendering on the pre-resize FBO.
static void on_window_resize_live()
{
    ge_present_for_drag();
    s_resize_pending = true;
}

static void on_close()
{
    // Tear down everything immediately. ShellActive=FALSE breaks any
    // `while (SHELL_ACTIVE)` loop (top-level game(), mission loop,
    // outro). GAME_STATE=0 kept for legacy paths that branched on it.
    // Without this, Alt+F4 / window-X used to drop the player back into
    // the main menu instead of exiting on the first try.
    ShellActive = UC_FALSE;
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

    // Per-frame input aggregator: zeros snapshots and sticky press flags.
    // Call site for input_frame_update is at the bottom of LibShellActive.
    input_frame_init();

    // Create the SDL3 window (hidden until GL context is ready).
    bool fullscreen = OC_CONFIG_get_int("video", "fullscreen", 0) != 0;
    if (!sdl3_window_create("Urban Chaos",
            OC_CONFIG_get_int("video", "windowed_width", 640),
            OC_CONFIG_get_int("video", "windowed_height", 480),
            fullscreen)) {
        return UC_FALSE;
    }

    if (!fullscreen && OC_CONFIG_get_int("video", "windowed_maximized", 0))
        sdl3_window_maximize();

    // Set initial display rect from window position.
    ge_update_display_rect(sdl3_window_get_native_handle(), ge_is_fullscreen());

    // Register SDL3 event callbacks.
    SDL3_Callbacks cb = {};
    cb.on_key_down = on_key_down;
    cb.on_key_up = on_key_up;
    cb.on_mouse_move = on_mouse_move;
    cb.on_mouse_button = on_mouse_button;
    cb.on_focus_gained = on_focus_gained;
    cb.on_focus_lost = on_focus_lost;
    cb.on_window_moved = on_window_moved;
    cb.on_window_resized = on_window_resized;
    cb.on_window_resize_live = on_window_resize_live;
    cb.on_close = on_close;
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

// uc_orig: LibShellActive (fallen/DDLibrary/Source/GHost.cpp)
// Polls the SDL3 event queue, dispatching input/window events via callbacks.
// Returns UC_TRUE while the app is alive.
BOOL LibShellActive(void)
{
    if (!sdl3_poll_events()) {
        ShellActive = UC_FALSE;
    }

    // Authoritative focus polling. SDL3 focus events in fullscreen-desktop
    // on Windows 11 have been observed to lag the OS state (Win-key press
    // didn't deliver FOCUS_LOST cleanly; click-to-refocus took multiple
    // attempts before FOCUS_GAINED fired). sdl3_window_has_focus reads
    // GetForegroundWindow directly on Windows — same edge-detection here
    // as in the video player's secondary loop. SDL event-driven path
    // (on_focus_gained / on_focus_lost) stays as a no-cost redundancy:
    // host_on_focus_changed is idempotent on identical state.
    {
        static bool s_last_focused = true;
        const bool focused_now = sdl3_window_has_focus();
        if (focused_now != s_last_focused) {
            host_on_focus_changed(focused_now);
            s_last_focused = focused_now;
        }
    }

    // After pumping events, apply any debounced window resize. Running this
    // here — at the top of the frame, before game logic / rendering begins —
    // guarantees the scene FBO is not destroyed mid-frame (see Risk #2 in
    // new_game_devlog/fullscreen_transition/runtime_window_resize_plan.md).
    host_process_pending_resize();

    // Per-frame input aggregator: snapshot keyboard + gamepad after SDL events
    // pumped, compute edges. All consumers (game / menu / frontend / debug)
    // can read unified state via input_key_*/input_btn_*. Sticky press_pending
    // for keyboard is set inline in keyboard_key_down → input_frame_on_key_down.
    input_frame_update();

    // Apply mouse-capture state for the upcoming frame. One-frame lag after
    // game-state transitions (pause/resume, mission end) is fine: cursor
    // re-appears next frame, imperceptible to the user.
    mouse_capture_update();

    return ShellActive;
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
    if (g_exit_log_written)
        return;
    g_exit_log_written = true;

    // Restore system-global accessibility settings on normal exit.
    win_restore_accessibility_shortcuts();

    FILE* f = fopen("crash_log.txt", "w");
    if (!f)
        return;

    write_exit_timestamp(f, "Clean exit");
    fprintf(f, "Type: Normal exit (returned from main or exit() called)\n");
    fflush(f);
    fclose(f);
}

// SIGABRT handler: catches abort() which bypasses both atexit and exception filters.
static void abort_signal_handler(int sig)
{
    if (g_exit_log_written) {
        signal(sig, SIG_DFL);
        raise(sig);
        return;
    }
    g_exit_log_written = true;

    // Restore system-global accessibility settings before aborting.
    win_restore_accessibility_shortcuts();

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
    if (g_exit_log_written) {
        signal(sig, SIG_DFL);
        raise(sig);
        return;
    }
    g_exit_log_written = true;

    FILE* f = fopen("crash_log.txt", "w");
    if (!f) {
        signal(sig, SIG_DFL);
        raise(sig);
        return;
    }

    write_exit_timestamp(f, "Crash (signal)");
    fprintf(f, "Signal: %d (%s)\n", sig,
        sig == SIGSEGV ? "SIGSEGV" : sig == SIGABRT ? "SIGABRT"
            : sig == SIGFPE                         ? "SIGFPE"
                                                    : "unknown");

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
    signal(SIGFPE, crash_signal_handler);
#endif

    // Suppress the Windows accessibility-shortcut popups (Sticky/Filter/Toggle
    // Keys) up front — the window starts focused, and on_focus_gained re-applies
    // it on every alt-tab back. Restored on focus loss and on all exit paths.
    win_disable_accessibility_shortcuts();

    init_best_found();

    return MF_main((UWORD)argc_in, argv_in);
}
