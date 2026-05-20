#version 410 core

// World-space skinned character vertex shader — skeletal_skinning_phase2_plan.md,
// step P2-C (consolidated mesh, bind-pose + skin matrices).
//
// Differences from the baked-palette path (skin_vert.glsl):
//
//   - a_position is in SHARED BIND-SPACE (mesh-build multiplied each bone-
//     local position by bind_palette[bMatIndex]). So a single fixed reference
//     frame holds the whole character.
//
//   - Per-bone palette (u_skin) carries the CLEAN world-space transform
//     skin[i] = current[i] * bind_inv[i] — same structure as the shadow
//     silhouette path (shadow_sil_vert.glsl): 3 vec4 per bone with the row's
//     translation packed into .w.  Output of  skin * bind_pos  is WORLD space.
//
//   - A single u_screen_xform (per character, not per bone) carries the
//     camera*projection*viewport bake — exactly the matrix the existing
//     skin_vert.glsl receives PER bone with the per-bone transform baked in.
//     With world-space input we only need the shared screen-projection part.
//
// The perspective-divide + screen->NDC math is COPIED VERBATIM from
// skin_vert.glsl so the on-screen result for w0=1 single-bone matches the
// baked path pixel-for-pixel — that's the P2-C gate (skeletal_skinning_
// phase2_plan.md section 6, "Гейт: визуально 1:1").
//
// Multi-bone (P2-E) lights up naturally:
//   world_pos    = Σ w_i * (skin[bone_i] * bind_pos)
//   world_normal = Σ w_i * (skin[bone_i].rot * bind_normal)
// Same world*projection pipeline applies after the blend.

const int MAX_BONES = 32; // = GE_SKIN_MAX_BONES (game_graphics_engine.h)

layout(location = 0) in vec3 a_position; // BIND-space (was bone-local in skin_vert)
layout(location = 1) in vec4 a_color;    // BGRA — lit path only
layout(location = 2) in vec4 a_specular; // BGRA — CPU specular (only RGB used; .a built here)
layout(location = 3) in vec2 a_texcoord;
layout(location = 4) in uint a_bone;     // single-bone palette index (multi-bone via weights in P2-D)
layout(location = 5) in vec3 a_normal;   // BIND-space normal

// Per-bone WORLD skin: 3 vec4 per bone, rotation rows with translation in .w.
//   skin[bone] = current_palette[bone] * inverse(bind_palette[bone])
//   world_pos  = (r0.xyz * bind_pos + r0.w,
//                 r1.xyz * bind_pos + r1.w,
//                 r2.xyz * bind_pos + r2.w)
// rotation already pre-divided by 32768 on the CPU (Matrix33 is fixed-point;
// CPU baker handles the divide so the shader stays branch-free, matching
// shadow_sil_vert.glsl's layout exactly).
uniform vec4 u_skin[MAX_BONES * 3];

// Camera-projection-viewport bake, shared by every bone (only depends on the
// camera + render target, not on the character's skeleton). Same column
// layout as MMBodyParts_pMatrix in the baked path: row 1 (.x lane) is a
// validation pattern not used by the shader; columns 2/3/4 carry the
// screen-x / screen-y / view-z bases the same way (see figure.cpp lines
// 3497-3516 for the CPU baker, skin_vert.glsl lines 99-117 for the matching
// shader-side column-pick).
uniform vec4 u_screen_xform[4]; // 4 rows; column 1 unused (matches MMBodyParts_pMatrix bake)

uniform vec3  u_lightdir_world; // MM_vLightDir directly — world space (was bone-local in skin_vert.glsl)
uniform uint  u_fadetable[64];  // ULONG ramp, 0x00RRGGBB
uniform int   u_skin_unlit;     // 1 = derive color from ramp (character path)
uniform vec4  u_viewport;       // D3D viewport (x, y, w, h)
uniform float u_zclip;          // POLY_ZCLIP_PLANE
uniform float u_fog_view_z;     // g_mm_fog_view_z (per character)
uniform float u_fog_fade_start; // POLY_FADEOUT_START
uniform float u_fog_fade_scale; // 256 / (POLY_FADEOUT_END - POLY_FADEOUT_START)

out vec4  v_color;
out vec4  v_specular;
out vec2  v_texcoord;
out float v_view_z;

void main()
{
    int bone = int(a_bone);
    int s    = bone * 3;
    vec4 sr0 = u_skin[s + 0];
    vec4 sr1 = u_skin[s + 1];
    vec4 sr2 = u_skin[s + 2];

    // Skin -> world. With w_0 = 1 single-bone (P2-C) this is just a rigid
    // transform; P2-D/E re-add the weight sum here.
    vec3 world_pos;
    world_pos.x = dot(sr0.xyz, a_position) + sr0.w;
    world_pos.y = dot(sr1.xyz, a_position) + sr1.w;
    world_pos.z = dot(sr2.xyz, a_position) + sr2.w;

    // World-space normal: rotation part of skin only. Bind-space normal -> world.
    // Lighting (1B) is then world-space.
    vec3 world_normal;
    world_normal.x = dot(sr0.xyz, a_normal);
    world_normal.y = dot(sr1.xyz, a_normal);
    world_normal.z = dot(sr2.xyz, a_normal);

    // --- Color (1B): half-Lambert ramp lookup, world-space dot ----------------
    if (u_skin_unlit != 0) {
        // Exact replica of the CPU half-Lambert ramp lookup (skin_vert.glsl
        // lines 73-86). Single difference: dot is taken in WORLD space here,
        // and the CPU baker for u_lightdir_world skipped the per-bone
        // inverse-rotation transform that the bone-local path did.
        float dot_raw = dot(world_normal, u_lightdir_world);
        float cos_nl  = dot_raw * (1.0 / 251.0); // undo fNormScale = 251
        float wrap    = cos_nl * 0.5 + 0.5;      // half-Lambert [0,1]
        int   idx     = int(wrap * 64.0);        // C (int) cast: truncate
        idx           = clamp(idx, 0, 63);
        uint  c       = u_fadetable[idx];        // 0x00RRGGBB
        float r = float((c >> 16) & 0xFFu);
        float g = float((c >>  8) & 0xFFu);
        float b = float( c        & 0xFFu);
        float a = float((c >> 24) & 0xFFu);      // table has A = 0
        v_color = vec4(r, g, b, a) * (1.0 / 255.0);
    } else {
        v_color = a_color.zyxw; // lit path: per-vertex color (1A behaviour)
    }

    // --- Fog (1C): flat per-character factor, exact CPU replica -----------
    int   fe    = int((u_fog_view_z - u_fog_fade_start) * u_fog_fade_scale);
    int   fmul  = clamp(255 - fe, 0, 255);
    float fog_a = float(fmul) * (1.0 / 255.0);
    v_specular  = vec4(a_specular.z, a_specular.y, a_specular.x, fog_a);
    v_texcoord  = a_texcoord;

    // --- Screen transform: world_pos × u_screen_xform ---------------------
    // Same column-pick pattern as skin_vert.glsl lines 99-117: take columns
    // 2/3/4 (.y/.z/.w) of the camera*projection*viewport bake to get
    // screen-x / screen-y / view-z directly. Column 1 is the validation
    // EVAL pattern (0xe0001000) and is not consumed.
    vec4 r0 = u_screen_xform[0];
    vec4 r1 = u_screen_xform[1];
    vec4 r2 = u_screen_xform[2];
    vec4 r3 = u_screen_xform[3];

    float sx = world_pos.x * r0.y + world_pos.y * r1.y + world_pos.z * r2.y + r3.y;
    float sy = world_pos.x * r0.z + world_pos.y * r1.z + world_pos.z * r2.z + r3.z;
    float sw = world_pos.x * r0.w + world_pos.y * r1.w + world_pos.z * r2.w + r3.w;

    if (sw < 0.001) sw = 0.001;            // div/0 guard (CPU parity)
    float rhw = 1.0 / sw;
    float screen_x = sx * rhw;
    float screen_y = sy * rhw;
    float zbuf = 1.0 - u_zclip / sw;        // POLY-path inverse-z space

    // tl_vert.glsl screen → clip, 1:1 (incl. −0.5 pixel-center offset for
    // D3D6 parity — same as the baked path).
    float ndc_x = (screen_x - 0.5 - u_viewport.x) / u_viewport.z * 2.0 - 1.0;
    float ndc_y = 1.0 - (screen_y - 0.5 - u_viewport.y) / u_viewport.w * 2.0;
    float ndc_z = zbuf * 2.0 - 1.0;

    float w = sw;                          // perspective-correct W (= view-space distance)
    gl_Position = vec4(ndc_x * w, ndc_y * w, ndc_z * w, w);
    v_view_z = w;
}
