#version 410 core

// Vertex shader for pre-lit world-space vertices (GEVertexLit).
// Applies World * View * Projection transform.

layout(location = 0) in vec3  a_position;  // world x, y, z
layout(location = 1) in float a_pad;       // _reserved padding
layout(location = 2) in vec4  a_color;     // BGRA (normalized from uint32)
layout(location = 3) in vec4  a_specular;  // BGRA (normalized from uint32)
layout(location = 4) in vec2  a_texcoord;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec4 v_color;
out vec4 v_specular;
out vec2 v_texcoord;
out float v_view_z;

void main()
{
    v_color    = a_color.zyxw;
    v_specular = a_specular.zyxw;
    v_texcoord = a_texcoord;

    vec4 world_pos = u_world * vec4(a_position, 1.0);
    vec4 view_pos  = u_view * world_pos;
    gl_Position    = u_projection * view_pos;
    v_view_z       = -view_pos.z;  // positive distance from camera
}
