// Mouse input handling via Windows messages.
// Tracks button states, cursor position, and delta movement.

#include <windows.h>

#include "engine/input/mouse.h"

// Temporary: hDDLibWindow lives in GDisplay (not yet migrated).
// Include DDlib.h which brings in GDisplay.h with proper DirectX headers.
#include "fallen/DDLibrary/Headers/DDlib.h"

// uc_orig: MouseMoved (fallen/DDLibrary/Source/GMouse.cpp)
volatile UBYTE MouseMoved = 0,
// uc_orig: LeftButton (fallen/DDLibrary/Source/GMouse.cpp)
               LeftButton = 0,
// uc_orig: MiddleButton (fallen/DDLibrary/Source/GMouse.cpp)
               MiddleButton = 0,
// uc_orig: RightButton (fallen/DDLibrary/Source/GMouse.cpp)
               RightButton = 0;
// uc_orig: MouseX (fallen/DDLibrary/Source/GMouse.cpp)
volatile SLONG MouseX,
// uc_orig: MouseY (fallen/DDLibrary/Source/GMouse.cpp)
    MouseY;

// uc_orig: MouseDX (fallen/DDLibrary/Source/GMouse.cpp)
volatile SLONG MouseDX,
// uc_orig: MouseDY (fallen/DDLibrary/Source/GMouse.cpp)
    MouseDY;

// uc_orig: OldMouseX (fallen/DDLibrary/Source/GMouse.cpp)
SLONG OldMouseX,
// uc_orig: OldMouseY (fallen/DDLibrary/Source/GMouse.cpp)
    OldMouseY;

// uc_orig: LeftMouse (fallen/DDLibrary/Source/GMouse.cpp)
volatile LastMouse LeftMouse = { 0, 0, 0, { 0, 0 } },
// uc_orig: MiddleMouse (fallen/DDLibrary/Source/GMouse.cpp)
                   MiddleMouse = { 0, 0, 0, { 0, 0 } },
// uc_orig: RightMouse (fallen/DDLibrary/Source/GMouse.cpp)
                   RightMouse = { 0, 0, 0, { 0, 0 } };
// uc_orig: MousePoint (fallen/DDLibrary/Source/GMouse.cpp)
volatile MFPoint MousePoint = { 0, 0 };

// uc_orig: MouseProc (fallen/DDLibrary/Source/GMouse.cpp)
LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam)
{
    switch (code) {
    case WM_MOUSEMOVE:
        MouseX = LOWORD(lParam);
        MouseY = HIWORD(lParam);

        MouseDX = MouseX - OldMouseX;
        MouseDY = MouseY - OldMouseY;
        MousePoint.X = MouseX;
        MousePoint.Y = MouseY;
        MouseMoved = 1;

        OldMouseX = MouseX;
        OldMouseY = MouseY;

        break;
    case WM_RBUTTONUP:
        RightButton = 0;
        break;
    case WM_RBUTTONDOWN:
        RightButton = 1;
        if (!RightMouse.ButtonState) {
            RightMouse.ButtonState = 1;
            RightMouse.MouseX = LOWORD(lParam);
            RightMouse.MouseY = HIWORD(lParam);
            RightMouse.MousePoint.X = LOWORD(lParam);
            RightMouse.MousePoint.Y = HIWORD(lParam);
        }
        break;
    case WM_RBUTTONDBLCLK:
        break;
    case WM_LBUTTONUP:
        LeftButton = 0;
        break;
    case WM_LBUTTONDOWN:
        LeftButton = 1;
        if (!LeftMouse.ButtonState) {
            LeftMouse.ButtonState = 1;
            LeftMouse.MouseX = LOWORD(lParam);
            LeftMouse.MouseY = HIWORD(lParam);
            LeftMouse.MousePoint.X = LOWORD(lParam);
            LeftMouse.MousePoint.Y = HIWORD(lParam);
        }
        break;
    case WM_LBUTTONDBLCLK:
        break;
    case WM_MBUTTONUP:
        MiddleButton = 0;
        break;
    case WM_MBUTTONDOWN:
        MiddleButton = 1;
        if (!MiddleMouse.ButtonState) {
            MiddleMouse.ButtonState = 1;
            MiddleMouse.MouseX = LOWORD(lParam);
            MiddleMouse.MouseY = HIWORD(lParam);
            MiddleMouse.MousePoint.X = LOWORD(lParam);
            MiddleMouse.MousePoint.Y = HIWORD(lParam);
        }
        break;
    case WM_MBUTTONDBLCLK:
        break;
    }
    return FALSE;
}

// uc_orig: RecenterMouse (fallen/DDLibrary/Source/GMouse.cpp)
void RecenterMouse(void)
{
    RECT client_rect;
    POINT p;

    GetWindowRect(hDDLibWindow, &client_rect);

    SetCursorPos((client_rect.left + client_rect.right) >> 1, (client_rect.top + client_rect.bottom) >> 1);
    p.x = (client_rect.left + client_rect.right) >> 1;
    p.y = (client_rect.top + client_rect.bottom) >> 1;

    ScreenToClient(hDDLibWindow, &p);

    OldMouseX = p.x;
    OldMouseY = p.y;
}
