// Mouse input handling.
// Tracks button states, cursor position, and delta movement.

#include "engine/input/mouse.h"
#include "engine/platform/sdl3_bridge.h"

// Called from SDL3 event loop on mouse movement.
// x, y are client-area coordinates.
// uc_orig: derived from MouseProc/WM_MOUSEMOVE (fallen/DDLibrary/Source/GMouse.cpp)
void mouse_on_move(int x, int y)
{
    MouseX = x;
    MouseY = y;

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
    switch (button) {
    case 0: // Left
        LeftButton = down ? 1 : 0;
        if (down && !LeftMouse.ButtonState) {
            LeftMouse.ButtonState = 1;
            LeftMouse.MouseX = x;
            LeftMouse.MouseY = y;
            LeftMouse.MousePoint.X = x;
            LeftMouse.MousePoint.Y = y;
        }
        break;
    case 1: // Right
        RightButton = down ? 1 : 0;
        if (down && !RightMouse.ButtonState) {
            RightMouse.ButtonState = 1;
            RightMouse.MouseX = x;
            RightMouse.MouseY = y;
            RightMouse.MousePoint.X = x;
            RightMouse.MousePoint.Y = y;
        }
        break;
    case 2: // Middle
        MiddleButton = down ? 1 : 0;
        if (down && !MiddleMouse.ButtonState) {
            MiddleMouse.ButtonState = 1;
            MiddleMouse.MouseX = x;
            MiddleMouse.MouseY = y;
            MiddleMouse.MousePoint.X = x;
            MiddleMouse.MousePoint.Y = y;
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

    // Convert screen center to client coordinates for delta tracking.
    int ww, wh;
    sdl3_window_get_size(&ww, &wh);
    OldMouseX = ww / 2;
    OldMouseY = wh / 2;
}
