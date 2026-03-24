#ifndef ENGINE_INPUT_MOUSE_GLOBALS_H
#define ENGINE_INPUT_MOUSE_GLOBALS_H

#include "core/types.h"

// uc_orig: LastMouse (MFStdLib/Headers/StdMouse.h)
struct LastMouse {
    SLONG ButtonState,
        MouseX,
        MouseY;
    MFPoint MousePoint;
};

// Public mouse state.

// uc_orig: MouseMoved (MFStdLib/Headers/StdMouse.h)
extern volatile UBYTE MouseMoved;
// uc_orig: LeftButton (MFStdLib/Headers/StdMouse.h)
extern volatile UBYTE LeftButton;
// uc_orig: MiddleButton (MFStdLib/Headers/StdMouse.h)
extern volatile UBYTE MiddleButton;
// uc_orig: RightButton (MFStdLib/Headers/StdMouse.h)
extern volatile UBYTE RightButton;
// uc_orig: MouseX (MFStdLib/Headers/StdMouse.h)
extern volatile SLONG MouseX;
// uc_orig: MouseY (MFStdLib/Headers/StdMouse.h)
extern volatile SLONG MouseY;
// uc_orig: LeftMouse (MFStdLib/Headers/StdMouse.h)
extern volatile LastMouse LeftMouse;
// uc_orig: MiddleMouse (MFStdLib/Headers/StdMouse.h)
extern volatile LastMouse MiddleMouse;
// uc_orig: RightMouse (MFStdLib/Headers/StdMouse.h)
extern volatile LastMouse RightMouse;
// uc_orig: MousePoint (MFStdLib/Headers/StdMouse.h)
extern volatile MFPoint MousePoint;

// uc_orig: MouseDX (fallen/DDLibrary/Source/GMouse.cpp)
extern volatile SLONG MouseDX;
// uc_orig: MouseDY (fallen/DDLibrary/Source/GMouse.cpp)
extern volatile SLONG MouseDY;

// uc_orig: OldMouseX (fallen/DDLibrary/Source/GMouse.cpp)
extern SLONG OldMouseX;
// uc_orig: OldMouseY (fallen/DDLibrary/Source/GMouse.cpp)
extern SLONG OldMouseY;

#endif // ENGINE_INPUT_MOUSE_GLOBALS_H
