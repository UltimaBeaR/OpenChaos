#version 410 core

// Vertex shader for pre-transformed vertices (GEVertexTL, screen-space).
// Converts D3D screen coordinates to OpenGL clip coordinates.
// D3D color format: 0xAARRGGBB — bytes in memory (little-endian): B, G, R, A.
// We read as GL_UNSIGNED_BYTE normalized, getting vec4(B, G, R, A), then swizzle.
//
// NOTE: This is the ONLY vertex shader used. All geometry — including world-space
// lit vertices (leaves, particles via ge_draw_indexed_primitive_lit) — is transformed
// on the CPU to screen-space TL format before drawing. There is no GPU-transform
// lit vertex shader. See known_issues_and_bugs.md for tech debt on GPU-transform path.

layout(location = 0) in vec3  a_position;  // screen x, y, z
layout(location = 1) in float a_rhw;       // reciprocal homogeneous W
layout(location = 2) in vec4  a_color;     // BGRA (normalized from uint32)
layout(location = 3) in vec4  a_specular;  // BGRA (normalized from uint32)
layout(location = 4) in vec2  a_texcoord;

// D3D viewport in screen coordinates (x, y, w, h).
uniform vec4 u_viewport;

out vec4 v_color;
out vec4 v_specular;
out vec2 v_texcoord;
out float v_view_z;

void main()
{
    // Swizzle BGRA → RGBA.
    v_color    = a_color.zyxw;
    v_specular = a_specular.zyxw;
    v_texcoord = a_texcoord;

    // Screen space → NDC relative to viewport.
    // D3D6 pixel center convention: integer coords (0,1,2...) are pixel centers.
    // OpenGL pixel center convention: half-integer coords (0.5,1.5,...) are centers.
    // Apply -0.5 offset to match D3D6 texel alignment (fixes UV bleeding at
    // sprite/glyph boundaries in texture atlases — visible as lines above menu
    // text and artifacts near HUD weapon icons).
    float ndc_x = (a_position.x - 0.5 - u_viewport.x) / u_viewport.z * 2.0 - 1.0;
    float ndc_y = 1.0 - (a_position.y - 0.5 - u_viewport.y) / u_viewport.w * 2.0;
    float ndc_z = a_position.z * 2.0 - 1.0;

    // Preserve perspective-correct interpolation via W = 1/rhw.
    float w = 1.0 / a_rhw;
    gl_Position = vec4(ndc_x * w, ndc_y * w, ndc_z * w, w);
    v_view_z = w;  // W ≈ view-space distance from camera
}
