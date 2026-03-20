// DirectInput joystick management.
// Enumerates, acquires, and polls joystick via IDirectInputDevice2.

#include <windows.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0700
#endif
#include <dinput.h>
#include <string.h>

#include "engine/input/joystick.h"

// uc_orig: OS_joy_enum (fallen/DDLibrary/Source/DIManager.cpp)
BOOL CALLBACK OS_joy_enum(
    LPCDIDEVICEINSTANCE instance,
    LPVOID context);

// uc_orig: OS_joy_init (fallen/DDLibrary/Source/DIManager.cpp)
void OS_joy_init(void);

// uc_orig: OS_joy_poll (fallen/DDLibrary/Source/DIManager.cpp)
SLONG OS_joy_poll(void);

// uc_orig: OS_joy_enum (fallen/DDLibrary/Source/DIManager.cpp)
BOOL CALLBACK OS_joy_enum(
    LPCDIDEVICEINSTANCE instance,
    LPVOID context)
{
    HRESULT hr;

    hr = OS_joy_direct_input->CreateDevice(
        instance->guidInstance,
        &OS_joy_input_device,
        NULL);

    if (FAILED(hr)) {
        OS_joy_input_device = NULL;
        OS_joy_input_device2 = NULL;

        return DIENUM_CONTINUE;
    }

    OS_joy_input_device->QueryInterface(
        IID_IDirectInputDevice2,
        (LPVOID*)&OS_joy_input_device2);

    return DIENUM_STOP;
}

// uc_orig: OS_joy_init (fallen/DDLibrary/Source/DIManager.cpp)
void OS_joy_init(void)
{
    OS_joy_direct_input = NULL;
    OS_joy_input_device = NULL;
    OS_joy_input_device2 = NULL;

    CoInitialize(NULL);
}

// uc_orig: OS_joy_poll (fallen/DDLibrary/Source/DIManager.cpp)
SLONG OS_joy_poll(void)
{
    HRESULT hr;

    if (OS_joy_direct_input == NULL || OS_joy_input_device == NULL || OS_joy_input_device2 == NULL) {
        memset(&the_state, 0, sizeof(the_state));

        return FALSE;
    }

    SLONG acquired_already = FALSE;

try_again_after_acquiring:;

    {
        OS_joy_input_device2->Poll();

        hr = OS_joy_input_device->GetDeviceState(sizeof(the_state), &the_state);

        if (hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST) {
            if (acquired_already) {
                memset(&the_state, 0, sizeof(the_state));

                return FALSE;
            } else {
                hr = OS_joy_input_device->Acquire();

                if (hr == DI_OK) {
                    acquired_already = TRUE;

                    goto try_again_after_acquiring;
                } else {
                    memset(&the_state, 0, sizeof(the_state));

                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

// uc_orig: GetInputDevice (fallen/DDLibrary/Source/DIManager.cpp)
BOOL GetInputDevice(UBYTE type, UBYTE sub_type, bool bActuallyGetOne)
{
    if (type == JOYSTICK && bActuallyGetOne) {
        if (!OS_joy_direct_input) {
            OS_joy_init();
        }
    }

    if (OS_joy_input_device && OS_joy_input_device2) {
        return TRUE;
    } else {
        return FALSE;
    }
}

// uc_orig: ReadInputDevice (fallen/DDLibrary/Source/DIManager.cpp)
BOOL ReadInputDevice()
{
    return OS_joy_poll();
}
