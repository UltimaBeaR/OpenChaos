// WindProcs.h
// Guy Simmons, 14th November 1997.

#ifndef FALLEN_DDLIBRARY_HEADERS_WINDPROCS_H
#define FALLEN_DDLIBRARY_HEADERS_WINDPROCS_H

#include "DDManager.h"

//---------------------------------------------------------------

typedef struct
{
    DDDriverInfo *DriverCurrent,
        *DriverNew;

    D3DDeviceInfo *DeviceCurrent,
        *DeviceNew;

    DDModeInfo *ModeCurrent,
        *ModeNew;
} ChangeDDInfo;

//---------------------------------------------------------------

LRESULT CALLBACK DDLibShellProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

//---------------------------------------------------------------

#endif // FALLEN_DDLIBRARY_HEADERS_WINDPROCS_H
