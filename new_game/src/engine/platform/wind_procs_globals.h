#ifndef ENGINE_PLATFORM_WIND_PROCS_GLOBALS_H
#define ENGINE_PLATFORM_WIND_PROCS_GLOBALS_H

#include "engine/core/types.h"

// uc_orig: app_inactive (fallen/DDLibrary/Source/WindProcs.cpp)
// Non-zero when the application has lost focus (WM_ACTIVATEAPP with wParam=0).
extern SLONG app_inactive;

// uc_orig: restore_surfaces (fallen/DDLibrary/Source/WindProcs.cpp)
// Set to UC_TRUE when the app regains focus and surfaces may need to be restored.
extern SLONG restore_surfaces;

#endif // ENGINE_PLATFORM_WIND_PROCS_GLOBALS_H
