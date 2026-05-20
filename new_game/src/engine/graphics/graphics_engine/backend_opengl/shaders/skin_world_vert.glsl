#version 410 core

// World-space skinned character vertex shader — the single GPU skinning
// path for all skeletally-animated things (people and animals/Bane/
// Balrog/bats/Gargoyle alike). See skeletal_skinning_phase2_plan.md.
//
// Inputs:
//
//   - a_position is in SHARED BIND-SPACE (mesh-build multiplied each
//     bone-local authored position by bind_palette[bMatIndex]). So a
//     single fixed reference frame holds the whole character.
//
//   - Per-bone palette (u_skin) carries the CLEAN world-space transform
//     skin[i] = current[i] * inverse(bind[i]) — same structure as the
//     shadow silhouette path: 3 vec4 per bone with the row's translation
//     packed into .w. Output of skin * bind_pos is WORLD space.
//
//   - A single u_screen_xform (per character, not per bone) carries the
//     camera*projection*viewport bake — the per-bone transform lives in
//     u_skin so this stays shared.
//
// Multi-bone blend (auto-rig at people leaf joints — 5-pair bidirectional
// in figure_build_consolidated_skin_world):
//   world_pos    = Σ w_i * (skin[bone_i] * bind_pos)
//   world_normal = Σ w_i * (skin[bone_i].rot * bind_normal)
// Same world*projection pipeline applies after the blend. Single-bone
// rigs (animals, plus people parts outside leaf-joint blend zones) use
// trivial weights w0=1, rest=0 — math collapses to a single transform.

const int MAX_BONES = 32; // = GE_SKIN_MAX_BONES (game_graphics_engine.h)

layout(location = 0) in vec3  a_position; // BIND-space
layout(location = 1) in vec4  a_color;    // BGRA — lit path only
layout(location = 2) in vec4  a_specular; // BGRA — CPU specular (only RGB used; .a built here)
layout(location = 3) in vec2  a_texcoord;
// location 4 (single-bone `a_bone`) is consumed by shadow_sil_vert.glsl,
// kept in the VBO for that path. The world-skin path uses the multi-bone
// palette at locations 6/7 below.
layout(location = 5) in vec3  a_normal;   // BIND-space normal
layout(location = 6) in uvec4 a_bones;    // multi-bone palette indices (P2-D)
layout(location = 7) in vec4  a_weights;  // multi-bone weights, normalized 0..1 (P2-D)

// Per-bone WORLD skin: 3 vec4 per bone, rotation rows with translation in .w.
//   skin[bone] = current_palette[bone] * inverse(bind_palette[bone])
//   world_pos  = (r0.xyz * bind_pos + r0.w,
//                 r1.xyz * bind_pos + r1.w,
//                 r2.xyz * bind_pos + r2.w)
// rotation already pre-divided by 32768 on the CPU (Matrix33 is fixed-point;
// CPU baker handles the divide so the shader stays branch-free, matching
// shadow_sil_vert.glsl's layout exactly).
uniform vec4 u_skin[MAX_BONES * 3];

// Camera-projection-viewport bake, shared by every bone (only depends
// on the camera + render target, not on the character's skeleton).
// Column 1 (.x lane) is unused; columns 2/3/4 carry the screen-x /
// screen-y / view-z bases. See figure_build_screen_xform_bake.
uniform vec4 u_screen_xform[4]; // 4 rows; column 1 unused

uniform vec3  u_lightdir_world; // MM_vLightDir, world space
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
    // Multi-bone skinning (P2-D). Accumulate the contribution of every bone
    // weighted by a_weights. With trivial weights (w0=1, w1..w3=0) — which
    // is what figure_build_consolidated_skin_world emits before P2-E — only
    // the i=0 iteration contributes and the math collapses to single-bone.
    // Once P2-E auto-rig populates real weights, this same loop produces
    // the soft-skinned result without any shader edit.
    //
    // Unrolled (instead of `for (int i = 0; i < 4; ++i)`) because GLSL
    // compilers may not constant-fold the indexed swizzle of uvec4/vec4,
    // and the unrolled form keeps the cost predictable and branchless on
    // the integrated GPUs we target (Steam Deck, low-end laptops).
    vec3 world_pos    = vec3(0.0);
    vec3 world_normal = vec3(0.0);

    // Bone 0 — always contributes (weight 1 for trivial, < 1 for soft).
    {
        int s = int(a_bones.x) * 3;
        vec4 sr0 = u_skin[s + 0];
        vec4 sr1 = u_skin[s + 1];
        vec4 sr2 = u_skin[s + 2];
        vec3 p = vec3(
            dot(sr0.xyz, a_position) + sr0.w,
            dot(sr1.xyz, a_position) + sr1.w,
            dot(sr2.xyz, a_position) + sr2.w);
        vec3 n = vec3(
            dot(sr0.xyz, a_normal),
            dot(sr1.xyz, a_normal),
            dot(sr2.xyz, a_normal));
        world_pos    += a_weights.x * p;
        world_normal += a_weights.x * n;
    }
    // Bones 1..3 — contribute only when their weight is non-zero. The
    // weight gate also skips work for the trivial-weights case where these
    // three lanes are always zero, keeping P2-D performance equal to P2-C.
    if (a_weights.y > 0.0) {
        int s = int(a_bones.y) * 3;
        vec4 sr0 = u_skin[s + 0];
        vec4 sr1 = u_skin[s + 1];
        vec4 sr2 = u_skin[s + 2];
        vec3 p = vec3(
            dot(sr0.xyz, a_position) + sr0.w,
            dot(sr1.xyz, a_position) + sr1.w,
            dot(sr2.xyz, a_position) + sr2.w);
        vec3 n = vec3(
            dot(sr0.xyz, a_normal),
            dot(sr1.xyz, a_normal),
            dot(sr2.xyz, a_normal));
        world_pos    += a_weights.y * p;
        world_normal += a_weights.y * n;
    }
    if (a_weights.z > 0.0) {
        int s = int(a_bones.z) * 3;
        vec4 sr0 = u_skin[s + 0];
        vec4 sr1 = u_skin[s + 1];
        vec4 sr2 = u_skin[s + 2];
        vec3 p = vec3(
            dot(sr0.xyz, a_position) + sr0.w,
            dot(sr1.xyz, a_position) + sr1.w,
            dot(sr2.xyz, a_position) + sr2.w);
        vec3 n = vec3(
            dot(sr0.xyz, a_normal),
            dot(sr1.xyz, a_normal),
            dot(sr2.xyz, a_normal));
        world_pos    += a_weights.z * p;
        world_normal += a_weights.z * n;
    }
    if (a_weights.w > 0.0) {
        int s = int(a_bones.w) * 3;
        vec4 sr0 = u_skin[s + 0];
        vec4 sr1 = u_skin[s + 1];
        vec4 sr2 = u_skin[s + 2];
        vec3 p = vec3(
            dot(sr0.xyz, a_position) + sr0.w,
            dot(sr1.xyz, a_position) + sr1.w,
            dot(sr2.xyz, a_position) + sr2.w);
        vec3 n = vec3(
            dot(sr0.xyz, a_normal),
            dot(sr1.xyz, a_normal),
            dot(sr2.xyz, a_normal));
        world_pos    += a_weights.w * p;
        world_normal += a_weights.w * n;
    }

    // --- Color (1B): half-Lambert ramp lookup, world-space dot ----------------
    if (u_skin_unlit != 0) {
        // Replica of the legacy CPU half-Lambert ramp lookup, but with
        // the dot taken in WORLD space (u_lightdir_world is uploaded
        // pre-scaled by 251 = fNormScale, matching the CPU table).
        //
        // Normalise: weighted blend of unit normals isn't unit in general
        // (Σ w_i × n_i has length ≤ 1), and integer-fixed-point matrix
        // multiplication in skin = current × inv_bind can introduce drift
        // > 1. Without this normalize, wrap can exceed [0,1] at certain
        // bone orientations and clamp to idx 0 (ambient) — visible as the
        // character going black at specific viewing angles.
        vec3  N       = normalize(world_normal);
        float dot_raw = dot(N, u_lightdir_world);
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
    // Column-pick: take columns 2/3/4 (.y/.z/.w) of the
    // camera*projection*viewport bake to get screen-x / screen-y /
    // view-z directly. Column 1 (.x lane) is unused.
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
