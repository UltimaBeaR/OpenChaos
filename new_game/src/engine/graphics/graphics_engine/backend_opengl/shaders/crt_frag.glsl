#version 410 core

// CRT-geom adaptation for OpenChaos — PUBLIC DOMAIN
// Based on crt-geom by cgwg (Christian Gosch):
//   https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-geom.glsl
//
// Modifications vs original:
//   - Removed barrel curvature and corner rounding (flat screen)
//   - Adapted coordinate system: u_source = display-res scratch texture,
//     u_content_size = virtual game resolution (e.g. 640x480)
//   - Added explicit halation pass (warm phosphor glow for dark 3D content)
//   - Mask tuned to very subtle (3D polygonal game, not pixel art)
//
// Algorithm:
//   For each output pixel, reconstruct colour from the two nearest content
//   scanlines using a 4th-power Gaussian beam profile (crt-geom characteristic:
//   sharper falloff than 2nd-power → real dark gap between lines).
//   Beam width scales with local luminance: bright phosphors spread more,
//   dark areas show a fully black gap — correct CRT physics.

out vec4 FragColor;

uniform sampler2D u_source;
uniform vec4  u_game_rect;
uniform vec2  u_content_size;

uniform float u_scanline_weight;    // beam width: 1.0 = sharp, 3.0 = soft/bloomy
uniform float u_mask_dark;          // phosphor mask dark value  (1.0 = mask off)
uniform float u_mask_light;         // phosphor mask bright value (1.0 = mask off)
uniform float u_halation_strength;  // warm glow intensity around bright objects
uniform float u_halation_radius;    // glow spread in content pixels
uniform float u_vignette_strength;
uniform float u_brightness;
uniform float u_warmth;

// ---- sRGB <-> linear -----------------------------------------------

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

// ---- sampling -------------------------------------------------------

// Sample scratch texture at an exact content-row centre (Y snapped, X free).
// The scratch texture is display-resolution; snapping Y to content row centres
// is the key to making scanlines reconstruct correctly.
vec3 FetchRow(float x_uv, float row) {
    vec2 uv = vec2(x_uv, (row + 0.5) / u_content_size.y);
    return ToLinear(texture(u_source, clamp(uv, vec2(0.0), vec2(1.0))).rgb);
}

// Sample with a content-pixel offset (used for halation wide kernel).
vec3 FetchOffset(vec2 uv, vec2 cp_off) {
    return ToLinear(texture(u_source,
        clamp(uv + cp_off / u_content_size, vec2(0.0), vec2(1.0))).rgb);
}

// ---- crt-geom scanline beam ----------------------------------------

// 4th-power Gaussian profile from crt-geom (cgwg).
// Compared to 2nd-power: sharper falloff → darker gap at midpoint between lines.
// Width scales with luminance: bright phosphors spread, dark areas stay sharp.
float ScanlineWeight(float dist, float lum) {
    float wid = u_scanline_weight * (0.3 + 0.2 * lum * lum * lum);
    float d   = dist / wid;
    return (0.4 / wid) * exp(-d * d * d * d);
}

// Reconstruct output pixel from 4 nearest content scanlines.
vec3 CrtScan(vec2 uv) {
    float cy = uv.y * u_content_size.y;

    // r: index of the content row whose centre is just below the current position.
    // t: fractional distance past r's centre (0 = at r centre, 1 = at r+1 centre).
    float r = floor(cy - 0.5);
    float t = cy - (r + 0.5);

    vec3 c0 = FetchRow(uv.x, r - 1.0);
    vec3 c1 = FetchRow(uv.x, r);
    vec3 c2 = FetchRow(uv.x, r + 1.0);
    vec3 c3 = FetchRow(uv.x, r + 2.0);

    // Luminance of the two primary rows drives their beam width.
    float l1 = dot(c1, vec3(0.2126, 0.7152, 0.0722));
    float l2 = dot(c2, vec3(0.2126, 0.7152, 0.0722));

    float w0 = ScanlineWeight(t + 1.0, l1);  // r-1: far above
    float w1 = ScanlineWeight(t,       l1);  // r:   near above
    float w2 = ScanlineWeight(1.0 - t, l2);  // r+1: near below
    float w3 = ScanlineWeight(2.0 - t, l2);  // r+2: far below

    return w0*c0 + w1*c1 + w2*c2 + w3*c3;
}

// ---- phosphor mask -------------------------------------------------

// Subtle aperture-grille: R / G / B columns, 3-pixel repeat.
// With u_mask_dark=0.85 / u_mask_light=1.15 the effect is barely perceptible
// on 3D content but adds a faint screen texture.
vec3 PhosphorMask(vec2 screen_pos) {
    float x    = mod(floor(screen_pos.x), 3.0);
    vec3  mask = vec3(u_mask_dark);
    if      (x < 1.0) mask.r = u_mask_light;
    else if (x < 2.0) mask.g = u_mask_light;
    else              mask.b = u_mask_light;
    return mask;
}

// ---- halation ------------------------------------------------------

// Warm phosphor glow: bright objects bleed light into surrounding dark areas.
// Main visual effect for dark 3D scenes (streetlights, explosions, gunfire).
vec3 Halation(vec2 uv) {
    vec3  glow  = vec3(0.0);
    float total = 0.0;
    for (float dy = -1.0; dy <= 1.0; dy += 1.0) {
        for (float dx = -1.0; dx <= 1.0; dx += 1.0) {
            float w  = exp(-(dx*dx + dy*dy));
            glow    += FetchOffset(uv, vec2(dx, dy) * u_halation_radius) * w;
            total   += w;
        }
    }
    glow /= total;
    float lum = dot(glow, vec3(0.2126, 0.7152, 0.0722));
    // Warm phosphor tint: red-heavy, low blue.
    const vec3 PHOSPHOR_WARM = vec3(1.1, 0.75, 0.3);
    return glow * PHOSPHOR_WARM * lum * u_halation_strength;
}

// ---- vignette ------------------------------------------------------

float Vignette(vec2 uv) {
    float aspect = u_content_size.x / u_content_size.y;
    vec2  d = (uv - 0.5) * vec2(aspect, 1.0);
    float r = dot(d, d);
    return 1.0 - u_vignette_strength * smoothstep(0.0, 0.75, r * 4.0);
}

// ---- entry point ---------------------------------------------------

void main() {
    vec2 game_uv = (gl_FragCoord.xy - u_game_rect.xy) / u_game_rect.zw;

    // Scanline reconstruction (crt-geom 4th-power Gaussian beam model).
    vec3 col = CrtScan(game_uv);

    // Phosphor mask (very subtle for 3D content).
    col *= PhosphorMask(gl_FragCoord.xy);

    // Halation: additive warm glow around bright areas.
    col += Halation(game_uv);

    // Vignette.
    col *= Vignette(game_uv);

    // Warm phosphor colour temperature.
    col.r *= 1.0 + u_warmth * 0.07;
    col.b *= 1.0 - u_warmth * 0.07;

    col *= u_brightness;

    // Clamp before gamma to avoid artefacts from scanline energy overshoot.
    col = min(col, vec3(1.5));

    FragColor = vec4(ToSrgb(col), 1.0);
}
