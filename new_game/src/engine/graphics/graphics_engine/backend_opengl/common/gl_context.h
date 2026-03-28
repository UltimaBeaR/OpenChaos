// OpenGL 4.1 context management via WGL on the existing Win32 HWND.
// Cross-platform (SDL3) window creation will be done in Stage 8.

#ifndef ENGINE_GRAPHICS_BACKEND_OPENGL_GL_CONTEXT_H
#define ENGINE_GRAPHICS_BACKEND_OPENGL_GL_CONTEXT_H

#include <cstdint>

// Create an OpenGL 4.1 core profile context on the game window.
// Must be called after SetupHost() creates hDDLibWindow.
bool gl_context_create(int32_t width, int32_t height);

// Destroy the GL context.
void gl_context_destroy();

// Swap buffers (present frame).
void gl_context_swap();

// Get current framebuffer dimensions.
int32_t gl_context_get_width();
int32_t gl_context_get_height();

#endif // ENGINE_GRAPHICS_BACKEND_OPENGL_GL_CONTEXT_H
