#version 410 core

// Wibble post-process (issue #67 fix):
//   offset = sin(world_x × WN + phase1) × s1 + cos(world_z × WN + phase2) × s2
// World XZ is the ground-plane (Y=0) point under each pixel — POLY-aware
// ray-cast from gl_FragCoord through the inverse of MATRIX_MUL's operator
// on POLY_cam_matrix (see wibble.cpp).
//
// Why a FIXED reference plane Y=0 (not per-puddle water_height): aeng
// iterates many puddles per frame, and their screen bboxes overlap. If
// each call used its own water_height the per-pixel world_xz would
// differ per call, and as the camera rotates which puddle "wins the
// overlap" changes discretely → visible pattern JUMP every few degrees.
// One shared Y anchor gives identical world_xz across overlapping calls
// → no jumps. For UC's flat-ground street levels this is visually
// indistinguishable from anchoring to each puddle's surface.
//
// Compound-wibble fix (aeng.cpp): each wibble pass copies the current
// framebuffer to a scratch texture before applying the warp; two
// overlapping wibble calls would each see the previous warp as input
// and apply their own on top → 2×/3×/Nx warp in overlap regions. aeng
// now unions per-reflection intersections across all puddles into ONE
// WIBBLE_simple call per reflection bbox — every pixel is warped at
// most once.
//
// S_AMP_FLOOR: between splashes the puddle's s1/s2 decay to 0 (no
// active splash). Without a floor, those frames render zero-offset and
// the ripple visibly pops off during camera motion. A modest floor
// keeps the surface continuously rippling; actual splashes still bump
// the amplitude noticeably above the floor.

out vec4 FragColor;

uniform sampler2D u_source;
uniform vec2      u_source_size;
uniform vec4      u_rect;            // (min_x, min_y, max_x, max_y) in GL pixel coords

// Per-puddle ripple parameters (from PUDDLE_Info on the CPU side).
uniform float u_wibble_y1;           // spatial frequency multiplier 1 (UBYTE)
uniform float u_wibble_y2;           // spatial frequency multiplier 2 (UBYTE)
uniform float u_wibble_s1;           // amplitude 1 (UBYTE 0..255)
uniform float u_wibble_s2;           // amplitude 2 (UBYTE 0..255)
uniform float u_phase1;              // (VISUAL_TURN × wibble_g1) mod 2048
uniform float u_phase2;              // (VISUAL_TURN × wibble_g2) mod 2048

// POLY-aware ray-cast state (set by WIBBLE_simple from POLY camera globals).
uniform mat3  u_inv_poly_cam;
uniform vec3  u_poly_cam_pos;
uniform vec2  u_poly_screen_mid;
uniform vec2  u_poly_screen_mul;
uniform float u_poly_zclip;
uniform vec4  u_viewport;            // (vx, vy, vw, vh); .w used for Y-flip

const float TWO_PI          = 6.28318530717958647692;
const float ANGLE_TO_RAD    = TWO_PI / 2048.0;        // CPU folds phase to 0..2048
const float ANCHOR_Y        = 0.0;                     // shared reference plane
const float BASE_WAVELENGTH = 30.0;                    // world units; tuned by eye
const float AMP_SCALE       = 1.0 / 8.0;               // legacy >> 19 ≡ s/8 px
const float S_AMP_FLOOR     = 25.0;                    // tuned by eye; baseline ripple between splashes
const float EDGE_FADE_PX    = 6.0;                     // soft scissor border

void main()
{
    // --- Pixel → world XZ at ground plane (Y=0) ------------------------
    // POLY uses top-down screen Y; gl_FragCoord is bottom-up — flip.
    float screen_X = gl_FragCoord.x;
    float screen_Y = u_viewport.w - gl_FragCoord.y;
    // Inverse POLY_perspective: view.x / view.z = α, view.y / view.z = β.
    float alpha = (u_poly_screen_mid.x - screen_X) / (u_poly_screen_mul.x * u_poly_zclip);
    float beta  = (u_poly_screen_mid.y - screen_Y) / (u_poly_screen_mul.y * u_poly_zclip);
    // cam-relative direction (per unit view_z) = inv_poly_cam × (α, β, 1).
    vec3 r = u_inv_poly_cam * vec3(alpha, beta, 1.0);
    // Skip rays nearly parallel to the water plane (would blow up view_z).
    if (abs(r.y) < 1e-4) {
        FragColor = texture(u_source, gl_FragCoord.xy / u_source_size);
        return;
    }
    float view_z   = (ANCHOR_Y - u_poly_cam_pos.y) / r.y;
    vec2  water_xz = vec2(view_z * r.x + u_poly_cam_pos.x,
                          view_z * r.z + u_poly_cam_pos.z);

    // --- Wibble: world XZ × frequency + temporal phase -----------------
    float wn_x   = TWO_PI / BASE_WAVELENGTH * (u_wibble_y1 / 128.0);
    float wn_z   = TWO_PI / BASE_WAVELENGTH * (u_wibble_y2 / 128.0);
    float angle1 = water_xz.x * wn_x + u_phase1 * ANGLE_TO_RAD;
    float angle2 = water_xz.y * wn_z + u_phase2 * ANGLE_TO_RAD;

    // Per-pixel anti-aliasing — fade amplitude when adjacent pixels span
    // much more than a wavelength in world (extreme horizon-grazing rays
    // where sin() would otherwise alias to noise). Threshold tracks the
    // effective wavenumber so AA adapts to per-puddle frequency.
    float wn_max         = max(abs(wn_x), abs(wn_z));
    float eff_wavelength = (wn_max > 1e-6) ? TWO_PI / wn_max : BASE_WAVELENGTH;
    float dwx = fwidth(water_xz.x);
    float dwz = fwidth(water_xz.y);
    float aa  = smoothstep(eff_wavelength * 3.0, eff_wavelength * 1.0, max(dwx, dwz));

    float s1_eff = max(u_wibble_s1, S_AMP_FLOOR);
    float s2_eff = max(u_wibble_s2, S_AMP_FLOOR);

    float offset = sin(angle1) * s1_eff * AMP_SCALE * aa
                 + cos(angle2) * s2_eff * AMP_SCALE * aa;

    // Soft fade toward scissor edges so the warp doesn't leave a zigzag
    // seam where the bbox meets the un-warped surrounding scene.
    float dist_x = min(gl_FragCoord.x - u_rect.x, u_rect.z - gl_FragCoord.x);
    float dist_y = min(gl_FragCoord.y - u_rect.y, u_rect.w - gl_FragCoord.y);
    float fade   = smoothstep(0.0, EDGE_FADE_PX, min(dist_x, dist_y));
    offset *= fade;

    vec2 sample_px = vec2(gl_FragCoord.x + offset, gl_FragCoord.y);
    FragColor = texture(u_source, sample_px / u_source_size);
}
