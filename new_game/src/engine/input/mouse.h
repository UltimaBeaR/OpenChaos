#ifndef ENGINE_INPUT_MOUSE_H
#define ENGINE_INPUT_MOUSE_H

#include "engine/input/mouse_globals.h"

// uc_orig: RecenterMouse (fallen/DDLibrary/Source/GMouse.cpp)
void RecenterMouse(void);

// Direct input callbacks (called from SDL3 event loop).
void mouse_on_move(int x, int y);
void mouse_on_button(int button, bool down, int x, int y);

#endif // ENGINE_INPUT_MOUSE_H
