#include "engine/input/mouse_capture.h"

#include "engine/graphics/graphics_engine/game_graphics_engine.h" // ge_is_fullscreen
#include "engine/platform/sdl3_bridge.h" // sdl3_set_relative_mouse_mode, cursor / window queries
#include "engine/platform/wind_procs_globals.h" // app_inactive
#include "game/game_globals.h" // the_game (GAME_STATE via game_types.h)
#include "game/game_types.h" // GS_* bits + macros
#include "ui/menus/gamemenu_globals.h" // GAMEMENU_menu_type

namespace {

// Requested capture state. Set by event triggers (click, gameplay-entry
// transition), cleared by release triggers (gameplay exit, focus loss,
// window hide). Apply happens in mouse_capture_update().
bool s_capture_requested = false;

// Currently applied SDL state. Compared against requested to detect
// transitions — only then do we touch SDL.
bool s_currently_applied = false;

// Previous tick's gameplay flag — for edge detection of the
// non-gameplay → gameplay transition that auto-engages capture.
bool s_was_in_gameplay = false;

bool is_in_active_gameplay()
{
    // GS_PLAY_GAME is the bit set while a mission is running. Any overlay
    // that stops accepting gameplay input (pause, mission-won, mission-
    // lost, "are you sure?" confirmation) is shown via GAMEMENU and sets
    // GAMEMENU_menu_type to a non-zero value — that's the single check
    // that covers all of them. (The GF_PAUSED flag from the original code
    // is dead in our port; pause goes through GAMEMENU exclusively.)
    if (!(GAME_STATE & GS_PLAY_GAME))
        return false;
    if (GAME_STATE & (GS_LEVEL_WON | GS_LEVEL_LOST))
        return false;
    if (GAMEMENU_menu_type != 0)
        return false;
    return true;
}

// Returns true when the OS cursor is positioned inside our window's
// client area. Fullscreen short-circuits to true — the window covers the
// entire screen, so any cursor position is "inside" by definition.
bool cursor_inside_client_area()
{
    if (ge_is_fullscreen())
        return true;

    int cx = 0, cy = 0;
    sdl3_get_global_mouse_pos(&cx, &cy);

    int wx = 0, wy = 0;
    sdl3_window_get_position(&wx, &wy);

    int ww = 0, wh = 0;
    sdl3_window_get_size(&ww, &wh);

    return cx >= wx && cx < wx + ww && cy >= wy && cy < wy + wh;
}

void apply_capture(bool capture)
{
    if (capture == s_currently_applied)
        return;
    s_currently_applied = capture;

    sdl3_set_relative_mouse_mode(capture);

    // After releasing relative mode in windowed mode, restore the OS
    // cursor — SDL can leave the window-class cursor at NULL on
    // relative-mode toggle, which makes the cursor invisible on hover
    // over our window despite the global SDL show flag.
    // In fullscreen, focus_gained / focus_lost own cursor visibility
    // (hidden while focused) so we leave it alone there.
    if (!capture && !ge_is_fullscreen())
        sdl3_show_cursor();
}

} // namespace

void mouse_capture_update()
{
    const bool now_in_gameplay = is_in_active_gameplay();
    const bool focused = !app_inactive;
    const bool visible = !sdl3_window_is_minimized();

    // Auto-engage on non-gameplay → gameplay edge, but only when the
    // window already has focus and the cursor is over it. Typical case:
    // unpausing while the user's mouse was hovering over the game
    // window. If the cursor is elsewhere (other monitor, title bar),
    // the user has to click to engage — same as a fresh start.
    if (!s_was_in_gameplay && now_in_gameplay) {
        if (focused && visible && cursor_inside_client_area())
            s_capture_requested = true;
    }
    s_was_in_gameplay = now_in_gameplay;

    // Forced release: any of the necessary conditions for capture has
    // gone away. We don't remember "wanted capture" across these
    // transitions — the user must trigger fresh (re-click, or get a new
    // gameplay-entry edge) to re-engage.
    if (!now_in_gameplay || !focused || !visible)
        s_capture_requested = false;

    apply_capture(s_capture_requested);
}

void mouse_capture_release()
{
    s_capture_requested = false;
    // Reset the gameplay-edge tracker so that returning to gameplay
    // after the release (e.g. after a cutscene video ends) registers as
    // a fresh non-gameplay → gameplay transition — that's the path the
    // auto-engage logic in mouse_capture_update relies on.
    s_was_in_gameplay = false;
    apply_capture(false);
}

bool mouse_capture_on_button(int /*button*/, bool down)
{
    if (!down)
        return false;
    if (!is_in_active_gameplay())
        return false;
    if (app_inactive)
        return false;
    if (sdl3_window_is_minimized())
        return false;
    if (!cursor_inside_client_area())
        return false;
    if (s_capture_requested)
        return false; // Already engaged; click passes through to gameplay.

    s_capture_requested = true;
    return true; // Consume the engagement click.
}

bool mouse_capture_is_active()
{
    return s_currently_applied;
}
