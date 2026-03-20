#include "engine/graphics/graphics_api/wind_procs.h"
#include "engine/graphics/graphics_api/wind_procs_globals.h"
#include "fallen/DDLibrary/Headers/DDLib.h"     // Temporary: DDLib includes, the_display
#include "assets/bink_client.h"                  // BinkMessage
#include "fallen/Headers/Game.h"                 // Temporary: GAME_STATE

extern void MFX_QUICK_stop(void);

// Forward declarations for input hook callbacks.
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam);

// uc_orig: DDLibShellProc (fallen/DDLibrary/Source/WindProcs.cpp)
// Main window procedure. Tracks app activation state, updates the display rect on
// move/resize, routes mouse and keyboard messages to their respective hook procs,
// and initiates the normal quit sequence on WM_CLOSE.
LRESULT CALLBACK DDLibShellProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int result;
    SLONG w, h, bpp, refresh;
    ChangeDDInfo change_info;
    GUID *D3D_guid,
        *DD_guid;
    HMENU hMenu;
    HINSTANCE hInstance;

    BinkMessage(hWnd, message, wParam, lParam);

    switch (message) {
    case WM_ACTIVATEAPP:

        if (!wParam) {
            // Lost focus — mark app as inactive so the main loop can yield CPU.
            app_inactive = TRUE;
        } else {
            app_inactive = FALSE;
            restore_surfaces = TRUE;
        }

        break;

    case WM_SIZE:
    case WM_MOVE:

        // Keep the_display.DisplayRect in sync with the actual window position.
        if (the_display.IsFullScreen()) {
            SetRect(&the_display.DisplayRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        } else {
            GetClientRect(hWnd, &the_display.DisplayRect);
            ClientToScreen(hWnd, (LPPOINT)&the_display.DisplayRect);
            ClientToScreen(hWnd, (LPPOINT)&the_display.DisplayRect + 1);
        }
        break;
    case WM_MOUSEMOVE:
    case WM_RBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
        MouseProc(message, wParam, lParam);
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
        KeyboardProc(message, wParam, lParam);
        break;
    case WM_CLOSE:
        // Set GAME_STATE to 0 so the normal game exit path runs instead of destroying immediately.
        GAME_STATE = 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
