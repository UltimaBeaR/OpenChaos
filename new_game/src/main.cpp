#include <windows.h>
#include "engine/graphics/graphics_api/host.h"

// Win32 application entry point.
// This is a thin wrapper — all real startup logic lives in HOST_run().
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPTSTR lpszArgs, int iWinMode)
{
    return HOST_run(hThisInst, hPrevInst, lpszArgs, iWinMode);
}
