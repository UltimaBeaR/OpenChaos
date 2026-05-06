#include "engine/input/gamepad_globals.h"

// Stick axes initialised to centre (32768). lX/lY/rX/rY plus their *_raw
// counterparts (raw = before D-Pad override). Remaining fields zero-init.
GamepadState gamepad_state = { 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768 };
InputDeviceType active_input_device = INPUT_DEVICE_KEYBOARD_MOUSE;
