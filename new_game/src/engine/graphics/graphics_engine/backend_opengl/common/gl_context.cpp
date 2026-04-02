// OpenGL 4.1 context creation via SDL3.

#include "engine/graphics/graphics_engine/backend_opengl/common/gl_context.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
#include "engine/platform/sdl3_bridge.h"

#include <stdio.h>

static int32_t s_width  = 640;
static int32_t s_height = 480;

// GLAD loader callback — forwards to SDL3's GL proc address.
static GLADapiproc gl_get_proc(const char* name)
{
    return (GLADapiproc)sdl3_gl_get_proc_address(name);
}

bool gl_context_create(int32_t width, int32_t height)
{
    s_width = width;
    s_height = height;

    // Resize the SDL3 window to match the requested client area.
    sdl3_window_set_size(width, height);

    // SDL3 window must already exist (created by SetupHost).
    if (!sdl3_gl_create_context(4, 1)) {
        fprintf(stderr, "OpenGL: SDL3 GL context creation failed\n");
        return false;
    }

    // Load GL function pointers via GLAD.
    int version = gladLoadGL((GLADloadfunc)gl_get_proc);
    if (!version) {
        fprintf(stderr, "OpenGL: GLAD failed to load GL functions\n");
        sdl3_gl_destroy_context();
        return false;
    }

    fprintf(stderr, "OpenGL vendor:   %s\n", (const char*)glGetString(GL_VENDOR));
    fprintf(stderr, "OpenGL renderer: %s\n", (const char*)glGetString(GL_RENDERER));
    fprintf(stderr, "OpenGL version:  %s\n", (const char*)glGetString(GL_VERSION));

    int swap_interval = 0;
    if (sdl3_gl_get_swap_interval(&swap_interval))
        fprintf(stderr, "OpenGL swap interval: %d\n", swap_interval);
    else
        fprintf(stderr, "OpenGL swap interval: unknown\n");

    // Show the window now that GL is ready.
    sdl3_window_show();

    // Use actual drawable size for the viewport (may differ from window size on HiDPI).
    int draw_w, draw_h;
    sdl3_window_get_drawable_size(&draw_w, &draw_h);
    glViewport(0, 0, draw_w, draw_h);

    return true;
}

void gl_context_destroy()
{
    sdl3_gl_destroy_context();
}

void gl_context_swap()
{
    sdl3_gl_swap();
}

int32_t gl_context_get_width()  { return s_width; }
int32_t gl_context_get_height() { return s_height; }
