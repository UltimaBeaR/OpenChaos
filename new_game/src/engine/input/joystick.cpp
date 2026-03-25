// uc_orig: DIManager (fallen/DDLibrary/Source/DIManager.cpp)
// Rewritten: DirectInput replaced with gamepad abstraction layer (SDL3 backend).

#include "engine/input/joystick.h"
#include "engine/input/gamepad.h"

// uc_orig: GetInputDevice (fallen/DDLibrary/Source/DIManager.cpp)
BOOL GetInputDevice(UBYTE type, UBYTE sub_type, bool bActuallyGetOne)
{
    if (type == JOYSTICK && bActuallyGetOne) {
        gamepad_init();
    }

    return gamepad_state.connected ? UC_TRUE : UC_FALSE;
}

// uc_orig: ReadInputDevice (fallen/DDLibrary/Source/DIManager.cpp)
BOOL ReadInputDevice()
{
    gamepad_poll();
    return gamepad_state.connected ? UC_TRUE : UC_FALSE;
}
