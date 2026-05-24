// Mouse input handling. Tracks only cursor position in scene-FBO pixel
// coordinates; window-pixel events from SDL3 are mapped via
// composition_window_to_fbo before being published. Events that land on
// outer pillar/letterbox bars are dropped — UI only lives inside the FBO.

#include "engine/input/mouse.h"
#include "engine/graphics/graphics_engine/backend_opengl/postprocess/composition.h"

void mouse_on_move(int x, int y)
{
    int fbo_x = 0, fbo_y = 0;
    if (!composition_window_to_fbo(x, y, &fbo_x, &fbo_y))
        return;

    MouseX = fbo_x;
    MouseY = fbo_y;
}
