#version 410 core

// GPU character transform + lighting vertex shader — Iteration 1,
// Milestones 1A (transform) + 1B (lighting).
// Plan: new_game_devlog/skeletal_skinning_plan.md
//
// Replaces the per-vertex CPU transform AND per-vertex lighting in
// ge_draw_multi_matrix (DrawIndPrimMM, the D3D MultiMatrix extension) for
// the figure path. Pairs with the SAME fragment shader as the TL path
// (common_frag.glsl) — outputs match tl_vert.glsl exactly so the
// textured/alpha pipeline is reused unchanged.
//
// A ge_draw_multi_matrix call references one or more bone matrices via a
// per-vertex index (a_bone). u_xform is the palette: 4 consecutive vec4s
// per matrix = the GEMatrix rows in memory order (_r1.._r4). Uploaded as
// glUniform4fv(loc, palette_n*4, &matrices[0]).
//
// KEY 1:1 (transform): byte-exact replica of the CPU block in
// polypage.cpp ge_draw_multi_matrix (the _12/_22/_32/_42 column picking,
// the z<0.001 guard, zbuf = 1 - P/z), then tl_vert.glsl's screen→clip
// (incl. the −0.5 D3D6 pixel-center offset). D3D color 0xAARRGGBB is BGRA
// in memory → swizzle .zyxw.
//
// KEY 1:1 (lighting, 1B): when u_skin_unlit, color is derived exactly
// like the CPU half-Lambert ramp lookup:
//   cos = dot(normal, light_dir[bone]) / 251   (undo fNormScale=251)
//   wrap = cos*0.5 + 0.5                        (half-Lambert)
//   idx  = clamp(int(wrap*64), 0, 63)           (C cast truncates → int())
//   color = fade_table[idx]                     (0x00RRGGBB, ULONG)
// Same numbers in → same color out. When !u_skin_unlit (lit path) the
// per-vertex color is used as-is (Milestone 1A behaviour).
//
// KEY 1:1 (fog, 1C): the CPU packed a flat per-character fog factor into
// specular.a from a single scalar g_mm_fog_view_z (view-Z of the
// character, constant for the whole MM call — NOT per-vertex). Replicated
// byte-exact here:
//   e     = int((fog_view_z - fade_start) * fade_scale)  (SLONG cast)
//   multi = clamp(255 - e, 0, 255)
//   spec.a = multi / 255
// a_specular now carries only the original specular RGB (lit highlight;
// 0 for the character path); the .a is computed here. The fragment
// shader's existing fog/table-fog select then runs unchanged → 1:1.

const int MAX_BONES = 32; // = GE_SKIN_MAX_BONES (game_graphics_engine.h)

layout(location = 0) in vec3  a_position;   // model (bone-local) space
layout(location = 1) in vec4  a_color;      // BGRA (lit path only)
layout(location = 2) in vec4  a_specular;   // BGRA (CPU fog)
layout(location = 3) in vec2  a_texcoord;
layout(location = 4) in uint  a_bone;       // matrix-palette index
layout(location = 5) in vec3  a_normal;     // model-space normal (raw)

uniform vec4  u_xform[MAX_BONES * 4]; // 4 rows (_r1.._r4) per matrix
uniform vec3  u_lightdir[MAX_BONES];  // per-bone light dir (CPU [1..3])
uniform uint  u_fadetable[64];        // ULONG ramp, 0x00RRGGBB
uniform int   u_skin_unlit;           // 1 = derive color from ramp
uniform vec4  u_viewport;             // D3D viewport (x,y,w,h) as tl_vert
uniform float u_zclip;                // POLY_ZCLIP_PLANE
uniform float u_fog_view_z;           // g_mm_fog_view_z (per MM call)
uniform float u_fog_fade_start;       // POLY_FADEOUT_START (set once)
uniform float u_fog_fade_scale;       // 256/(FADE_END-FADE_START) (once)

out vec4  v_color;
out vec4  v_specular;
out vec2  v_texcoord;
out float v_view_z;

void main()
{
    int bone = int(a_bone);

    // --- Color (1B) ---
    if (u_skin_unlit != 0) {
        // Exact replica of the CPU half-Lambert ramp lookup.
        vec3  L       = u_lightdir[bone];
        float dot_raw = dot(a_normal, L);
        float cos_nl  = dot_raw * (1.0 / 251.0); // undo fNormScale=251
        float wrap    = cos_nl * 0.5 + 0.5;      // half-Lambert [0,1]
        int   idx     = int(wrap * 64.0);        // C (int) cast: truncate
        idx           = clamp(idx, 0, 63);
        uint  c       = u_fadetable[idx];        // 0x00RRGGBB
        float r = float((c >> 16) & 0xFFu);
        float g = float((c >>  8) & 0xFFu);
        float b = float( c        & 0xFFu);
        float a = float((c >> 24) & 0xFFu);      // table has A=0
        v_color = vec4(r, g, b, a) * (1.0 / 255.0);
    } else {
        v_color = a_color.zyxw; // lit path: per-vertex color (1A behaviour)
    }
    // Fog (1C): flat per-character factor from g_mm_fog_view_z, exact
    // replica of the CPU specular.a packing. a_specular carries only the
    // original RGB (lit highlight; 0 for characters); .a computed here.
    int   fe    = int((u_fog_view_z - u_fog_fade_start) * u_fog_fade_scale);
    int   fmul  = clamp(255 - fe, 0, 255);
    float fog_a = float(fmul) * (1.0 / 255.0);
    v_specular  = vec4(a_specular.z, a_specular.y, a_specular.x, fog_a);
    v_texcoord  = a_texcoord;

    // --- Transform (1A, unchanged) ---
    int b = bone * 4;
    vec4 r0 = u_xform[b + 0]; // _11 _12 _13 _14
    vec4 r1 = u_xform[b + 1]; // _21 _22 _23 _24
    vec4 r2 = u_xform[b + 2]; // _31 _32 _33 _34
    vec4 r3 = u_xform[b + 3]; // _41 _42 _43 _44

    vec3 p = a_position;
    // Exact replica of the CPU transform (column picking matches the
    // pLVertCur->x*_12 + y*_22 + z*_32 + _42 form):
    float sx = p.x * r0.y + p.y * r1.y + p.z * r2.y + r3.y;
    float sy = p.x * r0.z + p.y * r1.z + p.z * r2.z + r3.z;
    float sw = p.x * r0.w + p.y * r1.w + p.z * r2.w + r3.w;

    if (sw < 0.001) sw = 0.001;            // div/0 guard (CPU parity)
    float rhw = 1.0 / sw;
    float screen_x = sx * rhw;
    float screen_y = sy * rhw;
    float zbuf = 1.0 - u_zclip / sw;       // POLY-path inverse-z space

    // tl_vert.glsl screen→clip, 1:1 (incl. −0.5 pixel-center offset).
    float ndc_x = (screen_x - 0.5 - u_viewport.x) / u_viewport.z * 2.0 - 1.0;
    float ndc_y = 1.0 - (screen_y - 0.5 - u_viewport.y) / u_viewport.w * 2.0;
    float ndc_z = zbuf * 2.0 - 1.0;

    float w = sw; // perspective-correct W (= view-space distance)
    gl_Position = vec4(ndc_x * w, ndc_y * w, ndc_z * w, w);
    v_view_z = w;
}
