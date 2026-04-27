// Mouse input handling.
// Tracks button states, cursor position, and delta movement. Positions
// are stored in scene-FBO pixel coordinates; window-pixel events from
// SDL3 are mapped via composition_window_to_fbo before being published.
// Events that land on outer pillar/letterbox bars are dropped — UI only
// lives inside the FBO.

#include "engine/input/mouse.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/graphics/graphics_engine/backend_opengl/postprocess/composition.h"

// Scene FBO dimensions (defined in d3d/display_globals.cpp). Used to
// centre the cursor after a warp — the warp itself targets the window
// centre in OS coordinates, but the tracked "old" position must be in
// FBO coordinates to match mouse events.
extern SLONG ScreenWidth;
extern SLONG ScreenHeight;

// Called from SDL3 event loop on mouse movement.
// x, y are client-area coordinates (real-window pixels after HiDPI fix).
// uc_orig: derived from MouseProc/WM_MOUSEMOVE (fallen/DDLibrary/Source/GMouse.cpp)
void mouse_on_move(int x, int y)
{
    int fbo_x = 0, fbo_y = 0;
    if (!composition_window_to_fbo(x, y, &fbo_x, &fbo_y)) {
        // Mouse in outer bar → event dropped. Don't touch MouseX/Y or
        // delta — the next in-bounds event will re-anchor relative to
        // OldMouseX/Y, which stay at the last valid FBO position.
        return;
    }

    MouseX = fbo_x;
    MouseY = fbo_y;

    MouseDX = MouseX - OldMouseX;
    MouseDY = MouseY - OldMouseY;
    MousePoint.X = MouseX;
    MousePoint.Y = MouseY;
    MouseMoved = 1;

    OldMouseX = MouseX;
    OldMouseY = MouseY;
}

// Called from SDL3 event loop on mouse button press/release.
// button: 0=left, 1=right, 2=middle.
// uc_orig: derived from MouseProc (fallen/DDLibrary/Source/GMouse.cpp)
void mouse_on_button(int button, bool down, int x, int y)
{
    int fbo_x = 0, fbo_y = 0;
    if (!composition_window_to_fbo(x, y, &fbo_x, &fbo_y)) {
        // Click landed in an outer bar. Drop — UI elements cannot be
        // there, so a click there is never meant for the game.
        return;
    }

    switch (button) {
    case 0: // Left
        LeftButton = down ? 1 : 0;
        if (down && !LeftMouse.ButtonState) {
            LeftMouse.ButtonState = 1;
            LeftMouse.MouseX = fbo_x;
            LeftMouse.MouseY = fbo_y;
            LeftMouse.MousePoint.X = fbo_x;
            LeftMouse.MousePoint.Y = fbo_y;
        }
        break;
    case 1: // Right
        RightButton = down ? 1 : 0;
        if (down && !RightMouse.ButtonState) {
            RightMouse.ButtonState = 1;
            RightMouse.MouseX = fbo_x;
            RightMouse.MouseY = fbo_y;
            RightMouse.MousePoint.X = fbo_x;
            RightMouse.MousePoint.Y = fbo_y;
        }
        break;
    case 2: // Middle
        MiddleButton = down ? 1 : 0;
        if (down && !MiddleMouse.ButtonState) {
            MiddleMouse.ButtonState = 1;
            MiddleMouse.MouseX = fbo_x;
            MiddleMouse.MouseY = fbo_y;
            MiddleMouse.MousePoint.X = fbo_x;
            MiddleMouse.MousePoint.Y = fbo_y;
        }
        break;
    }
}

// uc_orig: RecenterMouse (fallen/DDLibrary/Source/GMouse.cpp)
void RecenterMouse(void)
{
    int cx, cy;
    sdl3_window_get_center(&cx, &cy);
    sdl3_warp_mouse_global(cx, cy);

    // Anchor the delta-tracker at the FBO centre — window centre maps to
    // FBO centre under aspect-fit blit, so the next mouse event produces
    // a sensible delta regardless of outer-bar geometry.
    OldMouseX = ScreenWidth / 2;
    OldMouseY = ScreenHeight / 2;
}
