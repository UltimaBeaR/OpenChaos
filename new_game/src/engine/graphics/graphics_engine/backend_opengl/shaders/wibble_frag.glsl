#version 410 core

// Wibble post-process: horizontal per-row UV shift driven by sin+cos phases.
// Port of the CPU WIBBLE_simple loop (fallen/DDEngine/Source/wibble.cpp).
// The source texture is a copy of the default framebuffer (glCopyTexSubImage2D
// output); we sample it with a per-row horizontal offset that animates via
// GAME_TURN to produce the wavy puddle-reflection look.

out vec4 FragColor;

uniform sampler2D u_source;
uniform vec2      u_source_size;   // in pixels
uniform vec4      u_rect;           // (min_x, min_y, max_x, max_y) in GL pixel coords

uniform float u_wibble_y1;          // per-row angle multiplier 1 (UBYTE 0..255)
uniform float u_wibble_y2;          // per-row angle multiplier 2
uniform float u_wibble_s1;          // amplitude 1 (UBYTE 0..255)
uniform float u_wibble_s2;          // amplitude 2
uniform float u_phase1;             // (GAME_TURN * wibble_g1) mod 2048, in 2048-unit angle
uniform float u_phase2;             // (GAME_TURN * wibble_g2) mod 2048

// 2*pi / 2048 — converts 2048-unit angle to radians.
const float ANGLE_TO_RAD = 6.28318530717958647692 / 2048.0;

// CPU: offset = SIN(angle) * s >> 19. In GL sin() returns -1..+1 (equivalent
// to sin_fixed / 65536); dividing by (1<<19 / 65536) = 8 yields the same
// pixel-space offset. Precomputed once per effect invocation.
const float AMP_SCALE = 1.0 / 8.0;

// Edge-fade width in pixels. offset(edge) = 0 → pixel on the scissor
// boundary matches the un-wibbled source, killing the zigzag seam that
// the bare CPU algorithm leaves behind (per-row offset discontinuity at
// y1/y2 + constant offset across the full row including x1/x2 edges).
const float EDGE_FADE_PX = 6.0;

void main()
{
    // Game wibble indexes rows by top-down integer y. gl_FragCoord.y is
    // bottom-up with half-integer centers — invert and floor to recover the
    // integer row index so every fragment on the same row gets the same
    // offset (matches CPU's per-row loop discretization).
    float row_y = floor(u_source_size.y - gl_FragCoord.y);

    // Phase accumulates as signed int in the CPU path (with a `& 2047` mask
    // keeping it bounded). The CPU-side phase already folds GAME_TURN * g
    // into [0, 2048), so here row_y * wibble_y + phase stays small enough for
    // float precision (row_y <= 1080, wibble_y <= 255 → product <= ~275k).
    float angle1 = mod(row_y * u_wibble_y1 + u_phase1, 2048.0);
    float angle2 = mod(row_y * u_wibble_y2 + u_phase2, 2048.0);

    float offset = sin(angle1 * ANGLE_TO_RAD) * u_wibble_s1 * AMP_SCALE;
    offset      += cos(angle2 * ANGLE_TO_RAD) * u_wibble_s2 * AMP_SCALE;

    // Fade offset toward zero near all four scissor-rect edges. Distance to
    // nearest edge → smoothstep → multiplier. On the edge (dist=0) offset=0
    // so output sample == source pixel == pre-wibble content → continuous
    // with surrounding un-wibbled scene.
    float dist_x = min(gl_FragCoord.x - u_rect.x, u_rect.z - gl_FragCoord.x);
    float dist_y = min(gl_FragCoord.y - u_rect.y, u_rect.w - gl_FragCoord.y);
    float fade   = smoothstep(0.0, EDGE_FADE_PX, min(dist_x, dist_y));
    offset *= fade;

    // Sample the source at the shifted column, same row. Out-of-bounds
    // reads clamp to the framebuffer edge (scratch tex is CLAMP_TO_EDGE),
    // matching the CPU path's lack of per-row bounds (it reads past the
    // puddle rect freely — replicating that here preserves the look).
    vec2 sample_px = vec2(gl_FragCoord.x + offset, gl_FragCoord.y);
    FragColor = texture(u_source, sample_px / u_source_size);
}
