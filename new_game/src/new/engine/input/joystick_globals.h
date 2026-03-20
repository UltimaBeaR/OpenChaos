#ifndef ENGINE_INPUT_JOYSTICK_GLOBALS_H
#define ENGINE_INPUT_JOYSTICK_GLOBALS_H

#include "core/types.h"
#include <windows.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0700
#endif
#include <dinput.h>

// uc_orig: the_state (fallen/DDLibrary/Source/DIManager.cpp)
extern DIJOYSTATE the_state;

// uc_orig: OS_joy_direct_input (fallen/DDLibrary/Source/DIManager.cpp)
extern IDirectInput* OS_joy_direct_input;
// uc_orig: OS_joy_input_device (fallen/DDLibrary/Source/DIManager.cpp)
extern IDirectInputDevice* OS_joy_input_device;
// uc_orig: OS_joy_input_device2 (fallen/DDLibrary/Source/DIManager.cpp)
extern IDirectInputDevice2* OS_joy_input_device2;

// uc_orig: OS_joy_x_range_min (fallen/DDLibrary/Source/DIManager.cpp)
extern SLONG OS_joy_x_range_min;
// uc_orig: OS_joy_x_range_max (fallen/DDLibrary/Source/DIManager.cpp)
extern SLONG OS_joy_x_range_max;
// uc_orig: OS_joy_y_range_min (fallen/DDLibrary/Source/DIManager.cpp)
extern SLONG OS_joy_y_range_min;
// uc_orig: OS_joy_y_range_max (fallen/DDLibrary/Source/DIManager.cpp)
extern SLONG OS_joy_y_range_max;

#endif // ENGINE_INPUT_JOYSTICK_GLOBALS_H
