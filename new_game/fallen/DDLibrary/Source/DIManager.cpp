// DIManager.cpp
// Guy Simmons, 19th February 1998

#include "DDLib.h"
#include "FFManager.h"


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
    LPDIRECTINPUTDEVICE pDevice;

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
    HRESULT hr;

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

#ifdef MAD_AM_I

    CoCreateInstance(
        CLSID_DirectInput8,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IDirectInput8W,
        (void**)&OS_joy_direct_input);

    extern HINSTANCE hGlobalThisInst;

    hr = OS_joy_direct_input->Initialize(hGlobalThisInst, DIRECTINPUT_VERSION);
#else
    return;
#endif

    if (hr != DI_OK) {
        if (hr == DIERR_BETADIRECTINPUTVERSION) {
            return;
        }

        if (hr == DIERR_OLDDIRECTINPUTVERSION) {
            return;
        }

        return;
    }

    /*

hr = DirectInputCreateEx(
                    OS_this_instance,
                    DIRECTINPUT_VERSION,
               &OS_joy_direct_input,
                    NULL);

if (FAILED(hr))
    {
            //
            // No direct input!
            //

    return;
    }

    */

    //
    // Find a joystick.
    //

    hr = OS_joy_direct_input->EnumDevices(
        DIDEVTYPE_JOYSTICK,
        OS_joy_enum,
        NULL,
        DIEDFL_ATTACHEDONLY);

    if (OS_joy_input_device == NULL || OS_joy_input_device2 == NULL) {
        //
        // The joystick wasn't properly found.
        //

        OS_joy_input_device = NULL;
        OS_joy_input_device2 = NULL;

        return;
    }

    //
    // So we can get the nice 'n' simple joystick data format.
    //

    OS_joy_input_device->SetDataFormat(&c_dfDIJoystick);

    //
    // Grab the joystick exclusively when our window in the foreground.
    //

    OS_joy_input_device->SetCooperativeLevel(
        hDDLibWindow,
        DISCL_EXCLUSIVE | DISCL_FOREGROUND);

    //
    // What is the range of the joystick?
    //

    DIPROPRANGE diprg;

    //
    // In x...
    //

    diprg.diph.dwSize = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow = DIPH_BYOFFSET;
    diprg.diph.dwObj = DIJOFS_X;
    diprg.lMin = 0;
    diprg.lMax = 0;

    OS_joy_input_device->GetProperty(
        DIPROP_RANGE,
        &diprg.diph);

    OS_joy_x_range_min = diprg.lMin;
    OS_joy_x_range_max = diprg.lMax;

    //
    // In y...
    //

    diprg.diph.dwSize = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow = DIPH_BYOFFSET;
    diprg.diph.dwObj = DIJOFS_Y;
    diprg.lMin = 0;
    diprg.lMax = 0;

    OS_joy_input_device->GetProperty(
        DIPROP_RANGE,
        &diprg.diph);

    OS_joy_y_range_min = diprg.lMin;
    OS_joy_y_range_max = diprg.lMax;
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

