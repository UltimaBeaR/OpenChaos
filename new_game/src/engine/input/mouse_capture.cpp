#include "engine/input/mouse_capture.h"

#include "engine/graphics/graphics_engine/game_graphics_engine.h" // ge_is_fullscreen
#include "engine/input/input_frame.h" // input_debug_modifier_active, input_mouse_drain_rel
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

// Previous tick's GS_PLAY_GAME bit — for distinguishing "level just
// started" (the bit went 0 → 1) from "pause menu closed" (the bit was
// already set, only GAMEMENU_menu_type changed). On a fresh level
// start we auto-engage even if the cursor is outside the client area;
// on pause-menu exit we keep the strict "cursor inside" gate.
bool s_was_in_play_game = false;

// When true, all engage paths are blocked and any active capture is
// kept released. Used by the video player for the playback duration —
// videos aren't gameplay even during a cutscene, and a click on the
// window must not engage the camera path.
bool s_suppressed = false;

// True while the debug modifier (F1) is held and we've temporarily
// released the cursor so the dev can click on the world (teleport to
// mouse, spawn mine at mouse, etc.). On F1 release we re-engage capture
// — but ONLY if (a) capture was active when F1 was first pressed,
// (b) the window is still focused, (c) the cursor is still inside the
// client area. If any of those is missing on release we drop the
// "we want capture" state and the dev re-engages by clicking, same as
// a fresh start.
bool s_temp_release_for_debug = false;

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

    // Drain any accumulated mouse relative-motion delta on EVERY capture
    // transition (both off→on and on→off). On off→on it prevents the
    // first post-recapture frame from consuming motion that accumulated
    // while the cursor was free (e.g. user moved cursor to click on the
    // world during F1-debug-temp-release; on F1 release the camera
    // would otherwise swing by the OS-cursor travel distance). On on→off
    // it's a defensive drain so the next consumer (UI overlay, etc.)
    // starts from zero.
    input_mouse_drain_rel();
}

} // namespace

void mouse_capture_update()
{
    if (s_suppressed) {
        s_capture_requested = false;
        s_temp_release_for_debug = false;
        apply_capture(false);
        return;
    }

    const bool now_in_gameplay = is_in_active_gameplay();
    const bool focused = !app_inactive;
    const bool visible = !sdl3_window_is_minimized();

    // Auto-engage on non-gameplay → gameplay edge. Two distinct cases:
    //
    //  (a) Level just started/restarted — GS_PLAY_GAME bit went 0 → 1.
    //      On a fresh level load the user expects the game to grab the
    //      mouse immediately so they can play, even if their cursor
    //      happened to be on another monitor or over a title bar when
    //      the loading finished. Focus + visible is enough; we skip the
    //      cursor-inside-client-area check.
    //
    //  (b) Returning to gameplay from a pause/menu overlay — GS_PLAY_GAME
    //      was already set, only GAMEMENU_menu_type went back to 0.
    //      Here we keep the strict "cursor inside" gate: if the user's
    //      mouse is on another monitor (e.g. they alt-tabbed to chat),
    //      we don't snatch it back; they re-engage by clicking.
    const bool play_game_edge = (!s_was_in_play_game) && (GAME_STATE & GS_PLAY_GAME);
    const bool gameplay_edge = (!s_was_in_gameplay) && now_in_gameplay;
    if (gameplay_edge && focused && visible) {
        if (play_game_edge || cursor_inside_client_area())
            s_capture_requested = true;
    }
    s_was_in_gameplay = now_in_gameplay;
    s_was_in_play_game = (GAME_STATE & GS_PLAY_GAME) != 0;

    // Forced release: any of the necessary conditions for capture has
    // gone away. We don't remember "wanted capture" across these
    // transitions — the user must trigger fresh (re-click, or get a new
    // gameplay-entry edge) to re-engage.
    if (!now_in_gameplay || !focused || !visible) {
        s_capture_requested = false;
        s_temp_release_for_debug = false;
    }

    // Debug-modifier temp release: while F1 is held in debug mode and
    // capture was active, free the cursor so the dev can click into the
    // world for teleport / spawn-at-mouse / etc. On F1 release we
    // re-engage capture only if the standard click-engagement conditions
    // still hold; otherwise we drop the requested state so the dev gets
    // a fresh engagement next time they click.
    const bool debug_mod = input_debug_modifier_active();
    if (debug_mod && s_capture_requested && !s_temp_release_for_debug) {
        s_temp_release_for_debug = true;
    } else if (!debug_mod && s_temp_release_for_debug) {
        s_temp_release_for_debug = false;
        // Re-engagement gate — same standard checks used elsewhere.
        if (!(focused && visible && cursor_inside_client_area())) {
            s_capture_requested = false;
        }
    }

    // Effective capture state = requested AND not in temp-release.
    apply_capture(s_capture_requested && !s_temp_release_for_debug);
}

bool mouse_capture_on_button(int /*button*/, bool down)
{
    if (!down)
        return false;
    if (s_suppressed)
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

void mouse_capture_set_suppressed(bool suppressed)
{
    s_suppressed = suppressed;
    if (suppressed) {
        s_capture_requested = false;
        s_was_in_gameplay = false;
        s_was_in_play_game = false;
        apply_capture(false);
    }
}
