#ifndef ENGINE_INPUT_MOUSE_CAPTURE_H
#define ENGINE_INPUT_MOUSE_CAPTURE_H

// Mouse capture state machine: hides the cursor and enables SDL3 relative
// mouse mode so the mouse can drive camera / shooting without the cursor
// leaving the window. Capture is event-driven, not polled.
//
// Capture turns ON in two cases (each evaluated at the event moment —
// nothing is remembered):
//   - Mouse click inside the client area, with the window focused and
//     the game in active gameplay.
//   - Transition from non-gameplay to active gameplay (e.g. resuming from
//     pause, starting a mission from the main menu) while the window is
//     focused and the cursor sits inside the client area.
//
// Capture turns OFF on any of:
//   - Game leaves active gameplay (pause menu, mission end, main menu).
//   - Window loses OS focus.
//   - Window is minimized / hidden.
//
// "Active gameplay" = GS_PLAY_GAME set, no GS_LEVEL_WON / GS_LEVEL_LOST
// overlay, no GAMEMENU panel (pause / won / lost / "are you sure?").

void mouse_capture_update();

// Suppress capture engagement entirely (used by the video player —
// videos aren't gameplay even if GS_PLAY_GAME stays set during a
// cutscene, so clicks during a video must not engage the camera-
// capture path). Setting true also releases any current capture;
// setting false re-enables the normal engage path.
void mouse_capture_set_suppressed(bool suppressed);

// Mouse button event hook. Returns true when the click is consumed to
// engage capture (so the caller should NOT propagate it as gameplay
// input). Returns false otherwise.
bool mouse_capture_on_button(int button, bool down);

// True iff capture is currently active (cursor hidden + SDL relative
// mouse mode on). Future consumers (camera rotation, mouse-driven
// shooting) MUST gate their reads of mouse state on this — when capture
// is off the cursor is free for the user and mouse input is not meant
// for the game.
bool mouse_capture_is_active();

#endif // ENGINE_INPUT_MOUSE_CAPTURE_H
