#include "engine/input/mouse_globals.h"

// uc_orig: MouseMoved (fallen/DDLibrary/Source/GMouse.cpp)
volatile UBYTE MouseMoved = 0,
// uc_orig: LeftButton (fallen/DDLibrary/Source/GMouse.cpp)
               LeftButton = 0,
// uc_orig: MiddleButton (fallen/DDLibrary/Source/GMouse.cpp)
               MiddleButton = 0,
// uc_orig: RightButton (fallen/DDLibrary/Source/GMouse.cpp)
               RightButton = 0;
// uc_orig: MouseX (fallen/DDLibrary/Source/GMouse.cpp)
volatile SLONG MouseX,
// uc_orig: MouseY (fallen/DDLibrary/Source/GMouse.cpp)
    MouseY;

// uc_orig: MouseDX (fallen/DDLibrary/Source/GMouse.cpp)
volatile SLONG MouseDX,
// uc_orig: MouseDY (fallen/DDLibrary/Source/GMouse.cpp)
    MouseDY;

// uc_orig: OldMouseX (fallen/DDLibrary/Source/GMouse.cpp)
SLONG OldMouseX,
// uc_orig: OldMouseY (fallen/DDLibrary/Source/GMouse.cpp)
    OldMouseY;

// uc_orig: LeftMouse (fallen/DDLibrary/Source/GMouse.cpp)
volatile LastMouse LeftMouse = { 0, 0, 0, { 0, 0 } },
// uc_orig: MiddleMouse (fallen/DDLibrary/Source/GMouse.cpp)
                   MiddleMouse = { 0, 0, 0, { 0, 0 } },
// uc_orig: RightMouse (fallen/DDLibrary/Source/GMouse.cpp)
                   RightMouse = { 0, 0, 0, { 0, 0 } };
// uc_orig: MousePoint (fallen/DDLibrary/Source/GMouse.cpp)
volatile MFPoint MousePoint = { 0, 0 };
