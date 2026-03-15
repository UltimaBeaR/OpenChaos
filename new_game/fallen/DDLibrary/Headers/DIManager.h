// DIManager.h
// Guy Simmons, 19th February 1998.

#ifndef DIMANAGER_H
#define DIMANAGER_H


//---------------------------------------------------------------

#define MOUSE DIDEVTYPE_MOUSE
#define KEYBOARD DIDEVTYPE_KEYBOARD
#define JOYSTICK DIDEVTYPE_JOYSTICK

//---------------------------------------------------------------

#define DI_DRIVER_INIT (1 << 0)

#define DI_DEVICE_VALID (1 << 0)
#define DI_DEVICE_NEEDS_POLL (1 << 1)

// PC doesn't
#define ENABLE_REMAPPING 0

//---------------------------------------------------------------

// If bActuallyGetOne is FALSE, then just the current types are set up, no device is actually grabbed.
void ClearPrimaryDevice(void);
BOOL GetInputDevice(UBYTE type, UBYTE sub_type, bool bActuallyGetOne = TRUE);
BOOL ReadInputDevice(void);


//---------------------------------------------------------------

#endif
