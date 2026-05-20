#version 410 core

// Character shadow-silhouette vertex shader — the skin version. Renders
// the SAME consolidated bind-space mesh the body uses (skin_world_vert.glsl)
// but projects to the shadow texture sub-rect instead of the camera, and
// outputs only coverage (the fragment shader writes 1.0 alpha).
//
// Reuses the body's VBO and the body's per-frame skin palette — any
// soft-weight deformation applied to the body shows up identically in
// its shadow.
//
// Per-bone palette (u_skin), 3 vec4 per bone — the world skin matrix
//   skin[i] = current[i] * inverse(bind[i])
// in M*v form (translation packed in .w). Same layout as
// skin_world_vert.glsl's u_skin — the SMAP path reuses the same palette
// the body draw will use later this frame.
//
// world_pos = Σ w_i * (skin[bone_i] * bind_pos)
//
// Then u_shadow_proj maps world → shadow-clip (orthographic onto the
// sun plane, into the shadow texture sub-rect). Standard mat4.

const int MAX_BONES = 32; // = GE_SKIN_MAX_BONES (game_graphics_engine.h)

layout(location = 0) in vec3  a_position; // BIND-space
layout(location = 6) in uvec4 a_bones;    // multi-bone palette indices
layout(location = 7) in vec4  a_weights;  // multi-bone weights, normalised 0..1

uniform vec4 u_skin[MAX_BONES * 3];
uniform mat4 u_shadow_proj;

vec3 skin_one(int b, vec3 p)
{
    vec4 r0 = u_skin[b * 3 + 0];
    vec4 r1 = u_skin[b * 3 + 1];
    vec4 r2 = u_skin[b * 3 + 2];
    return vec3(dot(r0.xyz, p) + r0.w,
                dot(r1.xyz, p) + r1.w,
                dot(r2.xyz, p) + r2.w);
}

void main()
{
    vec3 world = vec3(0.0);
    // Bone 0 is the dominant influence (trivial weights → w0=1, rest=0).
    // For soft-skinned seam verts a_weights distributes mass across up
    // to 4 bones; the shader does the standard LBS blend.
    if (a_weights.x > 0.0) world += a_weights.x * skin_one(int(a_bones.x), a_position);
    if (a_weights.y > 0.0) world += a_weights.y * skin_one(int(a_bones.y), a_position);
    if (a_weights.z > 0.0) world += a_weights.z * skin_one(int(a_bones.z), a_position);
    if (a_weights.w > 0.0) world += a_weights.w * skin_one(int(a_bones.w), a_position);

    gl_Position = u_shadow_proj * vec4(world, 1.0);
}
