// DIManager.cpp
// Guy Simmons, 19th February 1998

#include "DDLib.h"

// TEMP
DIJOYSTATE the_state;

// ========================================================
//
// JOYSTICK STUFF
//
// ========================================================

IDirectInput* OS_joy_direct_input;
IDirectInputDevice* OS_joy_input_device;
IDirectInputDevice2* OS_joy_input_device2; // We need this newer interface to poll the joystick.

SLONG OS_joy_x_range_min;
SLONG OS_joy_x_range_max;
SLONG OS_joy_y_range_min;
SLONG OS_joy_y_range_max;

//
// The callback function for enumerating joysticks.
//

BOOL CALLBACK OS_joy_enum(
    LPCDIDEVICEINSTANCE instance,
    LPVOID context)
{
    HRESULT hr;

    //
    // Get an interface to the joystick.
    //

    hr = OS_joy_direct_input->CreateDevice(
        instance->guidInstance,
        &OS_joy_input_device,
        NULL);

    if (FAILED(hr)) {
        //
        // Cant use this joystick for some reason!
        //

        OS_joy_input_device = NULL;
        OS_joy_input_device2 = NULL;

        return DIENUM_CONTINUE;
    }

    //
    // Query for the IDirectInputDevice2 interface.
    // We need this to poll the joystick.
    //

    OS_joy_input_device->QueryInterface(
        IID_IDirectInputDevice2,
        (LPVOID*)&OS_joy_input_device2);

    //
    // No need to find another joystick!
    //

    return DIENUM_STOP;
}

//
// Initialises the joystick.
//

void OS_joy_init(void)
{
    //
    // Initialise everything.
    //

    OS_joy_direct_input = NULL;
    OS_joy_input_device = NULL;
    OS_joy_input_device2 = NULL;

    //
    // Create the direct input object.
    //

    CoInitialize(NULL);
}

//
// Polls the joystick.
//

SLONG OS_joy_poll(void)
{
    HRESULT hr;

    if (OS_joy_direct_input == NULL || OS_joy_input_device == NULL || OS_joy_input_device2 == NULL) {
        //
        // No joystick detected.
        //

        memset(&the_state, 0, sizeof(the_state));

        return FALSE;
    }

    SLONG acquired_already = FALSE;

try_again_after_acquiring:;

    {
        //
        // We acquired the joystick okay.  Poll the joystick to
        // update its state.
        //

        OS_joy_input_device2->Poll();

        //
        // Finally get the state of the joystick.
        //

        hr = OS_joy_input_device->GetDeviceState(sizeof(the_state), &the_state);

        if (hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST) {
            if (acquired_already) {
                //
                // Oh dear!
                //

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

BOOL ReadInputDevice()
{
    return OS_joy_poll();
}
