// OpenGL 4.1 context management via SDL3.

#ifndef ENGINE_GRAPHICS_BACKEND_OPENGL_GL_CONTEXT_H
#define ENGINE_GRAPHICS_BACKEND_OPENGL_GL_CONTEXT_H

#include <cstdint>

// Create an OpenGL 4.1 core profile context on the SDL3 window.
// Must be called after SetupHost() creates the SDL3 window.
// vsync_enabled is passed straight to sdl3_gl_configure_vsync() — see that
// API for how it interacts with window mode and OS to pick a strategy.
bool gl_context_create(int32_t width, int32_t height, bool vsync_enabled);

// Destroy the GL context.
void gl_context_destroy();

// Swap buffers (present frame).
void gl_context_swap();

// Get current framebuffer dimensions.
int32_t gl_context_get_width();
int32_t gl_context_get_height();

#endif // ENGINE_GRAPHICS_BACKEND_OPENGL_GL_CONTEXT_H
