#version 410 core

// Minimal vertex shader for post-process fullscreen/subrect quads.
// Vertices are in NDC [-1, +1]; no transform or matrix — positioning and
// clipping of the affected screen region are done on the CPU (scissor +
// NDC rectangle coords).

layout(location = 0) in vec2 a_pos;

void main()
{
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
