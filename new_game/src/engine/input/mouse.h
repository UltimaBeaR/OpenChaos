#ifndef ENGINE_INPUT_MOUSE_H
#define ENGINE_INPUT_MOUSE_H

#include "engine/input/mouse_globals.h"

// SDL3 mouse-motion event entry point. (x, y) are real client-area
// pixels — composition_window_to_fbo() maps them to MouseX/MouseY in
// scene-FBO coordinates. (xrel, yrel) are relative motion deltas
// since the previous event — accumulated into MouseRelDX/MouseRelDY
// for the mouse camera (which reads via input_mouse_consume_rel each
// tick).
void mouse_on_move(int x, int y, int xrel, int yrel);

#endif // ENGINE_INPUT_MOUSE_H
