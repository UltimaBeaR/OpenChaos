#ifndef ENGINE_INPUT_MOUSE_H
#define ENGINE_INPUT_MOUSE_H

#include "engine/input/mouse_globals.h"

// SDL3 mouse-motion event entry point. (x, y) are real client-area
// pixels — composition_window_to_fbo() maps them to MouseX/MouseY in
// scene-FBO coordinates.
void mouse_on_move(int x, int y);

#endif // ENGINE_INPUT_MOUSE_H
