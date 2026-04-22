#ifndef ENGINE_GRAPHICS_BACKEND_OPENGL_POSTPROCESS_WIBBLE_EFFECT_H
#define ENGINE_GRAPHICS_BACKEND_OPENGL_POSTPROCESS_WIBBLE_EFFECT_H

// Backend-internal teardown hook for the wibble post-process pass.
// Program, VAO/VBO, and scratch texture are created lazily on first
// ge_apply_wibble() call; this releases them on context shutdown.
void gl_wibble_effect_shutdown();

#endif // ENGINE_GRAPHICS_BACKEND_OPENGL_POSTPROCESS_WIBBLE_EFFECT_H
