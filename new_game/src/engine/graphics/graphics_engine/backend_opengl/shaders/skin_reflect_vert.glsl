#version 410 core

// Skinned character water-reflection vertex shader — P2-I.
// Reuses the body's bind-space VBO + per-frame skin palette (so any soft
// weights from P2-E auto-rig apply to the reflection without separate
// setup). After skinning, mirrors world Y about the water plane and runs
// the same camera*projection*viewport bake the body uses, then adds a
// height-above-water fade to the fog alpha so reflections taper off the
// further the original was above the water surface.

// Shared layout (MAX_BONES, a_position / a_bones / a_weights, u_skin)
// substituted in at shader-embed time — see _skin_common.glsl.
#include "_skin_common.glsl"

// Body-specific attribute we still need for sampling the body texture.
// color / specular / normal are NOT used — reflection lighting is a flat
// per-character NIGHT-tinted colour (computed on the CPU).
layout(location = 3) in vec2 a_texcoord;

// Per-character flat colour (NIGHT_get_light_at(pelvis), halved for the
// dimmed-reflection look — see the legacy CPU formula in figure.cpp:
// `colour >>= 1; specular >>= 1; colour |= 0xff000000`). BGRA-ordered to
// match the body shader's a_color convention.
uniform vec4 u_reflect_color;
uniform vec4 u_reflect_specular;

// Water plane world Y. Used both for the mirror flip (reflected_y = 2H -
// y) and for the distance-above-water fog factor.
uniform float u_reflect_height;
// 255 / FIGURE_MAX_DY (FIGURE_MAX_DY = 128) — the legacy CPU multiplier
// for the height-above-water fog accumulator.
uniform float u_reflect_dy_scale;

// Same projection state as skin_world_vert.glsl (the per-character
// camera*projection*viewport bake).
uniform vec4 u_screen_xform[4]; // 4 rows; column 1 unused
uniform vec4 u_viewport;
uniform float u_zclip;

// Same camera fog state as the body (camera-distance flat per character).
uniform float u_fog_view_z;
uniform float u_fog_fade_start;
uniform float u_fog_fade_scale;

out vec4  v_color;
out vec4  v_specular;
out vec2  v_texcoord;
out float v_view_z;

// Same helper-style as skin_shadow_vert.glsl — readability over the body
// shader's manually-unrolled form. Reflection isn't perf-critical the way
// body is on integrated GPUs (puddles are scarce, draw count is small).
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
    // Multi-bone blend in bind-space → world-space (same blend the body and
    // the shadow do — shares the per-frame skin palette).
    vec3 world = vec3(0.0);
    if (a_weights.x > 0.0) world += a_weights.x * skin_one(int(a_bones.x), a_position);
    if (a_weights.y > 0.0) world += a_weights.y * skin_one(int(a_bones.y), a_position);
    if (a_weights.z > 0.0) world += a_weights.z * skin_one(int(a_bones.z), a_position);
    if (a_weights.w > 0.0) world += a_weights.w * skin_one(int(a_bones.w), a_position);

    // Mirror about the water plane: y > H above → y < H below. The camera
    // still looks at the original world from above, but sees the mirrored
    // geometry through the puddle.
    world.y = 2.0 * u_reflect_height - world.y;

    // Distance from the water plane in the reflected frame: positive when
    // the vertex is below the water surface (= the original was above).
    // Drives the height-above-water fog accumulator below.
    float dy = u_reflect_height - world.y;

    // --- Flat colour (per-character NIGHT colour, halved on the CPU) ----
    // Both inputs are BGRA-ordered (CPU side packs them with the same
    // convention as a_color). Swap to RGBA on output.
    v_color = u_reflect_color.zyxw;

    // --- Fog: camera-distance flat + height-above-water per-vertex -----
    // The legacy CPU path computes the same `fmul = clamp(255 - fe, 0,
    // 255)` accumulator the body does, then adds `distance * 255/MAX_DY`
    // on top (figure.cpp:3301-3306). 1:1 here so reflections fade the same
    // way they used to before P2-I.
    int   fe          = int((u_fog_view_z - u_fog_fade_start) * u_fog_fade_scale);
    int   fmul_camera = clamp(255 - fe, 0, 255);
    int   fmul_total  = fmul_camera + int(dy * u_reflect_dy_scale);
    fmul_total        = clamp(fmul_total, 0, 255);
    float fog_a       = float(fmul_total) * (1.0 / 255.0);
    v_specular        = vec4(u_reflect_specular.z, u_reflect_specular.y, u_reflect_specular.x, fog_a);
    v_texcoord        = a_texcoord;

    // --- Screen transform: identical to skin_world_vert.glsl -----------
    vec4 r0 = u_screen_xform[0];
    vec4 r1 = u_screen_xform[1];
    vec4 r2 = u_screen_xform[2];
    vec4 r3 = u_screen_xform[3];

    float sx = world.x * r0.y + world.y * r1.y + world.z * r2.y + r3.y;
    float sy = world.x * r0.z + world.y * r1.z + world.z * r2.z + r3.z;
    float sw = world.x * r0.w + world.y * r1.w + world.z * r2.w + r3.w;

    if (sw < 0.001) sw = 0.001;
    float rhw = 1.0 / sw;
    float screen_x = sx * rhw;
    float screen_y = sy * rhw;
    float zbuf = 1.0 - u_zclip / sw;

    float ndc_x = (screen_x - 0.5 - u_viewport.x) / u_viewport.z * 2.0 - 1.0;
    float ndc_y = 1.0 - (screen_y - 0.5 - u_viewport.y) / u_viewport.w * 2.0;
    float ndc_z = zbuf * 2.0 - 1.0;

    float w = sw;
    gl_Position = vec4(ndc_x * w, ndc_y * w, ndc_z * w, w);
    v_view_z = w;
}
