// Shader compilation and program linking utilities for OpenGL 4.1 core profile.

#ifndef ENGINE_GRAPHICS_BACKEND_OPENGL_GL_SHADER_H
#define ENGINE_GRAPHICS_BACKEND_OPENGL_GL_SHADER_H

#include <cstdint>

// Compile a shader from source. Returns shader ID, or 0 on failure (logs error).
uint32_t gl_shader_compile(uint32_t type, const char* source);

// Link vertex + fragment shaders into a program. Returns program ID, or 0 on failure.
// Deletes the individual shaders after linking.
uint32_t gl_shader_link(uint32_t vert, uint32_t frag);

// Compile vertex + fragment sources and link into a program. Returns program ID, or 0 on failure.
uint32_t gl_shader_create_program(const char* vert_source, const char* frag_source);

#endif // ENGINE_GRAPHICS_BACKEND_OPENGL_GL_SHADER_H
