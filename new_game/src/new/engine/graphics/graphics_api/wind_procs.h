#ifndef ENGINE_GRAPHICS_GRAPHICS_API_WIND_PROCS_H
#define ENGINE_GRAPHICS_GRAPHICS_API_WIND_PROCS_H

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "core/types.h"
// Temporary: DDDriverInfo, D3DDeviceInfo, DDModeInfo not yet migrated to new/
#include "fallen/DDLibrary/Headers/DDManager.h"  // Temporary: until dd_manager is migrated

// uc_orig: ChangeDDInfo (fallen/DDLibrary/Headers/WindProcs.h)
// Holds the current and requested driver/device/mode for a display change operation.
typedef struct
{
    DDDriverInfo *DriverCurrent,
        *DriverNew;

    D3DDeviceInfo *DeviceCurrent,
        *DeviceNew;

    DDModeInfo *ModeCurrent,
        *ModeNew;
} ChangeDDInfo;

// uc_orig: DDLibShellProc (fallen/DDLibrary/Source/WindProcs.cpp)
// Main window procedure: handles app activation, window resize/move, input, and quit messages.
LRESULT CALLBACK DDLibShellProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif // ENGINE_GRAPHICS_GRAPHICS_API_WIND_PROCS_H
