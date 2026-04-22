#version 410 core

// Composition pass: reads the scene FBO (rendered at physical × OC_RENDER_SCALE)
// and writes to the default framebuffer at full window size. Upscale is
// implicit — the scene texture sampler uses LINEAR filtering, so reading it
// over a larger viewport gives bilinear scaling for free.
//
// Optional shader AA: a simplified FXAA pass (Lottes 2009, algorithm close to
// FXAA 3.11 PC Quality but single-sample offset instead of full span search —
// cheap, works on alpha-test edges which MSAA never covered, slightly blurs
// thin 1-pixel details). Controlled by u_fxaa_enabled.

out vec4 FragColor;

uniform sampler2D u_scene;
uniform vec2      u_inv_scene_size;  // 1.0 / scene_pixel_size
uniform vec2      u_window_size;     // output framebuffer size (pixels)
uniform int       u_fxaa_enabled;    // 0 = plain upscale, 1 = FXAA + upscale

// FXAA tuning — close to Lottes's "PC Quality 12" defaults.
const float FXAA_EDGE_THRESHOLD     = 0.166;   // 1/6 — contrast ratio to flag an edge
const float FXAA_EDGE_THRESHOLD_MIN = 0.0833;  // 1/12 — absolute floor on dark edges
const float FXAA_SUBPIX_QUALITY     = 0.75;    // 0 = sharpest, 1 = most blur

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

vec3 fxaa(vec2 uv)
{
    vec3  rgbM  = texture(u_scene, uv).rgb;
    float lumaM = luma(rgbM);

    // 4-neighborhood.
    vec3 rgbN = textureOffset(u_scene, uv, ivec2( 0,  1)).rgb;
    vec3 rgbS = textureOffset(u_scene, uv, ivec2( 0, -1)).rgb;
    vec3 rgbE = textureOffset(u_scene, uv, ivec2( 1,  0)).rgb;
    vec3 rgbW = textureOffset(u_scene, uv, ivec2(-1,  0)).rgb;
    float lumaN = luma(rgbN);
    float lumaS = luma(rgbS);
    float lumaE = luma(rgbE);
    float lumaW = luma(rgbW);

    float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
    float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
    float range   = lumaMax - lumaMin;

    // Not an edge — return center unchanged.
    if (range < max(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD)) {
        return rgbM;
    }

    // Diagonal neighbors for edge-direction test and subpixel average.
    vec3 rgbNW = textureOffset(u_scene, uv, ivec2(-1,  1)).rgb;
    vec3 rgbNE = textureOffset(u_scene, uv, ivec2( 1,  1)).rgb;
    vec3 rgbSW = textureOffset(u_scene, uv, ivec2(-1, -1)).rgb;
    vec3 rgbSE = textureOffset(u_scene, uv, ivec2( 1, -1)).rgb;
    float lumaNW = luma(rgbNW);
    float lumaNE = luma(rgbNE);
    float lumaSW = luma(rgbSW);
    float lumaSE = luma(rgbSE);

    // Edge orientation via second-derivative-like sums.
    float edgeH = abs((lumaNW + lumaNE) - 2.0 * lumaN)
                + 2.0 * abs((lumaW  + lumaE ) - 2.0 * lumaM)
                + abs((lumaSW + lumaSE) - 2.0 * lumaS);
    float edgeV = abs((lumaNW + lumaSW) - 2.0 * lumaW)
                + 2.0 * abs((lumaN  + lumaS ) - 2.0 * lumaM)
                + abs((lumaNE + lumaSE) - 2.0 * lumaE);
    bool horzSpan = edgeH >= edgeV;

    // Pick the brighter side perpendicular to the span; step toward it.
    float luma1 = horzSpan ? lumaN : lumaW;
    float luma2 = horzSpan ? lumaS : lumaE;
    float grad1 = abs(luma1 - lumaM);
    float grad2 = abs(luma2 - lumaM);
    float stepLen;
    if (grad1 >= grad2) {
        stepLen = horzSpan ?  u_inv_scene_size.y : -u_inv_scene_size.x;
    } else {
        stepLen = horzSpan ? -u_inv_scene_size.y :  u_inv_scene_size.x;
    }

    // Subpixel blend factor: how much of a "sub-pixel-sized" detail is in the
    // neighborhood average → how strongly to blend toward the offset sample.
    float lumaAvg  = (1.0 / 12.0) *
                     (2.0 * (lumaN + lumaS + lumaE + lumaW)
                      + (lumaNW + lumaNE + lumaSW + lumaSE));
    float subpix   = clamp(abs(lumaAvg - lumaM) / range, 0.0, 1.0);
    subpix = smoothstep(0.0, 1.0, subpix) * FXAA_SUBPIX_QUALITY;

    vec2 offset = horzSpan ? vec2(0.0, stepLen) : vec2(stepLen, 0.0);
    vec3 rgbOff = texture(u_scene, uv + offset).rgb;

    return mix(rgbM, rgbOff, subpix);
}

void main()
{
    // Output viewport covers the whole window; scene UVs span [0, 1].
    vec2 uv = gl_FragCoord.xy / u_window_size;

    vec3 rgb;
    if (u_fxaa_enabled != 0) {
        rgb = fxaa(uv);
    } else {
        rgb = texture(u_scene, uv).rgb;
    }
    FragColor = vec4(rgb, 1.0);
}
