#ifndef ENGINE_INPUT_JOYSTICK_H
#define ENGINE_INPUT_JOYSTICK_H

#include "engine/core/types.h"
#include "engine/input/joystick_globals.h"

// Legacy device type constants (kept for API compatibility).
// uc_orig: JOYSTICK (fallen/DDLibrary/Headers/DIManager.h)
#define JOYSTICK 4

// uc_orig: DI_DRIVER_INIT (fallen/DDLibrary/Headers/DIManager.h)
#define DI_DRIVER_INIT (1 << 0)
// uc_orig: DI_DEVICE_VALID (fallen/DDLibrary/Headers/DIManager.h)
#define DI_DEVICE_VALID (1 << 0)

// uc_orig: ENABLE_REMAPPING (fallen/DDLibrary/Headers/DIManager.h)
#define ENABLE_REMAPPING 0

// uc_orig: GetInputDevice (fallen/DDLibrary/Source/DIManager.cpp)
BOOL GetInputDevice(UBYTE type, UBYTE sub_type, bool bActuallyGetOne = UC_TRUE);
// uc_orig: ReadInputDevice (fallen/DDLibrary/Source/DIManager.cpp)
BOOL ReadInputDevice(void);

#endif // ENGINE_INPUT_JOYSTICK_H
