#version 410 core

// CRT-Lottes scanline post-process shader.
// Adapted from Timothy Lottes' PUBLIC DOMAIN crt-lottes shader:
// https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-lottes.glsl
//
// Reads directly from the scene FBO color texture (pre-FXAA, pre-upscale),
// and writes the CRT-filtered result to the game area on the default FB,
// replacing the composition_blit output for the game image pixels.
//
// Coordinate contract:
//   gl_FragCoord    — GL window-space pixels, bottom-left origin.
//   u_game_rect     — game area (x, y, w, h) in GL pixels, bottom-left origin.
//                     Matches the aspect-fit rect from composition_blit.
//   u_source        — scene FBO color texture (size = u_content_size).
//   u_content_size  — scene FBO dimensions (virtual game resolution, e.g. 640×480).

out vec4 FragColor;

uniform sampler2D u_source;
uniform vec4  u_game_rect;      // (x, y, w, h) of game area in GL pixel coords
uniform vec2  u_content_size;   // scene FBO size (= virtual game resolution)

// CRT tuning parameters (all set from C++).
uniform float u_hard_scan;       // scanline hardness: -8.0 soft, -16.0 medium
uniform float u_hard_pix;        // horizontal pixel hardness: -3.0 soft, -4.0 hard
uniform float u_warp_x;          // barrel curvature X: 0.0 = flat, 0.031 = mild
uniform float u_warp_y;          // barrel curvature Y: 0.0 = flat, 0.041 = mild
uniform float u_mask_dark;       // phosphor mask dark channel (0.5)
uniform float u_mask_light;      // phosphor mask light channel (1.5)
uniform float u_bloom_amount;    // bloom intensity (0.15, set 0 to disable)
uniform float u_hard_bloom_pix;  // bloom horizontal softness (-1.5)
uniform float u_hard_bloom_scan; // bloom vertical softness (-2.0)
uniform float u_shape;           // Gaussian kernel shape exponent (2.0)

// ---- sRGB <-> linear -------------------------------------------------

float ToLinear1(float c) {
    return (c <= 0.04045) ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4);
}
vec3 ToLinear(vec3 c) {
    return vec3(ToLinear1(c.r), ToLinear1(c.g), ToLinear1(c.b));
}
float ToSrgb1(float c) {
    return (c < 0.0031308) ? c * 12.92 : 1.055 * pow(c, 0.41666) - 0.055;
}
vec3 ToSrgb(vec3 c) {
    return vec3(ToSrgb1(c.r), ToSrgb1(c.g), ToSrgb1(c.b));
}

// ---- sampling --------------------------------------------------------

// Sample the scene FBO at a content-pixel-aligned position.
// game_uv: position within game area, range [0, 1].
// off:     integer offset in content pixels.
vec3 Fetch(vec2 game_uv, vec2 off) {
    // Snap to content pixel grid, apply offset, renormalize to source UV.
    vec2 src_uv = (floor(game_uv * u_content_size + off) + 0.5) / u_content_size;
    if (max(abs(src_uv.x - 0.5), abs(src_uv.y - 0.5)) > 0.5)
        return vec3(0.0);
    return ToLinear(texture(u_source, src_uv).rgb);
}

// Sub-pixel distance from current position to the nearest content-pixel centre.
vec2 Dist(vec2 game_uv) {
    vec2 cp = game_uv * u_content_size;
    return -((cp - floor(cp)) - 0.5);
}

float Gaus(float pos, float scale) {
    return exp2(scale * pow(abs(pos), u_shape));
}

// ---- horizontal Gaussian taps ----------------------------------------

vec3 Horz3(vec2 uv, float off) {
    vec3 b = Fetch(uv, vec2(-1.0, off));
    vec3 c = Fetch(uv, vec2( 0.0, off));
    vec3 d = Fetch(uv, vec2( 1.0, off));
    float dst = Dist(uv).x;
    float wb = Gaus(dst - 1.0, u_hard_pix);
    float wc = Gaus(dst + 0.0, u_hard_pix);
    float wd = Gaus(dst + 1.0, u_hard_pix);
    return (b*wb + c*wc + d*wd) / (wb + wc + wd);
}

vec3 Horz5(vec2 uv, float off) {
    vec3 a = Fetch(uv, vec2(-2.0, off));
    vec3 b = Fetch(uv, vec2(-1.0, off));
    vec3 c = Fetch(uv, vec2( 0.0, off));
    vec3 d = Fetch(uv, vec2( 1.0, off));
    vec3 e = Fetch(uv, vec2( 2.0, off));
    float dst = Dist(uv).x;
    float wa = Gaus(dst - 2.0, u_hard_pix);
    float wb = Gaus(dst - 1.0, u_hard_pix);
    float wc = Gaus(dst + 0.0, u_hard_pix);
    float wd = Gaus(dst + 1.0, u_hard_pix);
    float we = Gaus(dst + 2.0, u_hard_pix);
    return (a*wa + b*wb + c*wc + d*wd + e*we) / (wa + wb + wc + wd + we);
}

vec3 Horz7(vec2 uv, float off) {
    vec3 a = Fetch(uv, vec2(-3.0, off));
    vec3 b = Fetch(uv, vec2(-2.0, off));
    vec3 c = Fetch(uv, vec2(-1.0, off));
    vec3 d = Fetch(uv, vec2( 0.0, off));
    vec3 e = Fetch(uv, vec2( 1.0, off));
    vec3 f = Fetch(uv, vec2( 2.0, off));
    vec3 g = Fetch(uv, vec2( 3.0, off));
    float dst = Dist(uv).x;
    float wa = Gaus(dst - 3.0, u_hard_bloom_pix);
    float wb = Gaus(dst - 2.0, u_hard_bloom_pix);
    float wc = Gaus(dst - 1.0, u_hard_bloom_pix);
    float wd = Gaus(dst + 0.0, u_hard_bloom_pix);
    float we = Gaus(dst + 1.0, u_hard_bloom_pix);
    float wf = Gaus(dst + 2.0, u_hard_bloom_pix);
    float wg = Gaus(dst + 3.0, u_hard_bloom_pix);
    return (a*wa + b*wb + c*wc + d*wd + e*we + f*wf + g*wg) /
           (wa + wb + wc + wd + we + wf + wg);
}

// ---- scanlines + bloom -----------------------------------------------

float Scan(vec2 uv, float off) {
    return Gaus(Dist(uv).y + off, u_hard_scan);
}
float BloomScan(vec2 uv, float off) {
    return Gaus(Dist(uv).y + off, u_hard_bloom_scan);
}

vec3 Tri(vec2 uv) {
    return Horz3(uv, -1.0) * Scan(uv, -1.0)
         + Horz5(uv,  0.0) * Scan(uv,  0.0)
         + Horz3(uv,  1.0) * Scan(uv,  1.0);
}

vec3 Bloom(vec2 uv) {
    return Horz5(uv, -2.0) * BloomScan(uv, -2.0)
         + Horz7(uv, -1.0) * BloomScan(uv, -1.0)
         + Horz7(uv,  0.0) * BloomScan(uv,  0.0)
         + Horz7(uv,  1.0) * BloomScan(uv,  1.0)
         + Horz5(uv,  2.0) * BloomScan(uv,  2.0);
}

// ---- distortion + mask -----------------------------------------------

// Barrel distortion: game_uv [0,1] → warped game_uv [0,1].
vec2 Warp(vec2 uv) {
    uv = uv * 2.0 - 1.0;
    uv *= vec2(1.0 + uv.y * uv.y * u_warp_x,
               1.0 + uv.x * uv.x * u_warp_y);
    return uv * 0.5 + 0.5;
}

// Stretched VGA shadow mask (mask style 3 from original crt-lottes).
vec3 Mask(vec2 screen_pos) {
    screen_pos.x += screen_pos.y * 3.0;
    vec3 mask = vec3(u_mask_dark);
    screen_pos.x = fract(screen_pos.x / 6.0);
    if      (screen_pos.x < 0.333) mask.r = u_mask_light;
    else if (screen_pos.x < 0.666) mask.g = u_mask_light;
    else                           mask.b = u_mask_light;
    return mask;
}

// ---- entry point -----------------------------------------------------

void main() {
    // Derive game-area UV [0, 1] from this fragment's screen position.
    vec2 game_uv = (gl_FragCoord.xy - u_game_rect.xy) / u_game_rect.zw;

    // Apply barrel distortion.
    vec2 warped = Warp(game_uv);

    // Pixels warped past the screen edge → solid black (CRT bezel).
    if (warped.x < 0.0 || warped.x > 1.0 ||
        warped.y < 0.0 || warped.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 col = Tri(warped);
    col += Bloom(warped) * u_bloom_amount;
    col *= Mask(gl_FragCoord.xy);

    FragColor = vec4(ToSrgb(col), 1.0);
}
