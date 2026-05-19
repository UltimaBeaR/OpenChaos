#version 410 core

// Character shadow-silhouette vertex shader — skeletal_skinning_plan.md,
// Milestone 1E (GPU render-to-texture replacement for the SMAP software
// rasteriser).
//
// Reuses the persistent skin meshes (GESkinVertex layout, same VBOs as the
// body draw) but with a completely different projection: instead of the
// camera-baked MM palette, each bone carries its pure WORLD affine
// (bone-local -> world), and a single u_shadow_proj maps world space onto
// the sun's shadow plane / into the shadow-texture sub-rect.
//
// Per-bone palette (u_xform), 3 vec4 per bone, world affine:
//   row0 = (r00, r01, r02, pos_x)
//   row1 = (r10, r11, r12, pos_y)
//   row2 = (r20, r21, r22, pos_z)
// rotation already pre-divided by 32768 on the CPU (Matrix33 is fixed-point
// x32768; the per-bone divide is cheap and keeps the shader branch-free).
// world = rot * a_position + pos  -- matches matrix_transform_small + offset
// used by the body/SMAP CPU path (shape parity; not byte-exact, by design).

const int MAX_BONES = 32; // = GE_SKIN_MAX_BONES (game_graphics_engine.h)

layout(location = 0) in vec3 a_position; // model (bone-local) space
layout(location = 4) in uint a_bone;     // matrix-palette index

uniform vec4 u_xform[MAX_BONES * 3]; // 3 rows per bone (world affine)
uniform mat4 u_shadow_proj;          // world -> shadow-clip (built per person)

void main()
{
    int b = int(a_bone) * 3;
    vec4 r0 = u_xform[b + 0];
    vec4 r1 = u_xform[b + 1];
    vec4 r2 = u_xform[b + 2];

    vec3 world;
    world.x = dot(r0.xyz, a_position) + r0.w;
    world.y = dot(r1.xyz, a_position) + r1.w;
    world.z = dot(r2.xyz, a_position) + r2.w;

    gl_Position = u_shadow_proj * vec4(world, 1.0);
}
