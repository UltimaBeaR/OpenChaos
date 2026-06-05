// CRT-geom post-process effect — consumer TV aesthetic for dark 3D content.
//
// Called at the end of the UI pass (after all game content AND UI/HUD have
// rendered into the default FB), so the CRT filter covers the full frame
// including menus and HUD — just like a real CRT monitor would.
//
// Flow:
//   1. glCopyTexSubImage2D — GPU-side copy of the game area from the default
//      FB into a persistent scratch texture.  No CPU traffic.
//   2. CRT shader quad — reads from scratch, writes CRT-filtered result back
//      to the same game area on the default FB.

#include "engine/graphics/graphics_engine/backend_opengl/postprocess/crt_effect.h"
#include "engine/graphics/graphics_engine/backend_opengl/postprocess/composition.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/gl_shader.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
#include "engine/io/oc_config.h"
#include "engine/platform/sdl3_bridge.h"

#include <stdio.h>
#include <stdint.h>

#include "gl_shaders_embedded.h"

bool g_crt_enabled = false; // default off; overridden from config in crt_effect_init()

namespace {

// CRT scanline density: scanlines per centimetre of physical screen. Fixing
// this in real centimetres (via the display's physical pixel density) makes the
// apparent scanline pitch identical regardless of display resolution or pixel
// density. ~10 → fine modern-CRT look; lower = coarser/arcade.
constexpr float CRT_PIXELS_PER_CM = 18.0f;

// Physical display density (screen px per cm) assumed when the OS can't report a
// real panel size (non-Windows, or a driver returning a degenerate size).
constexpr float DISPLAY_PPCM_FALLBACK = 96.0f / 2.54f; // 96 DPI in px/cm

// Resolved physical px per emulated scanline. ALWAYS a whole number of physical
// pixels (see resolve_scanline_pitch) so each emulated CRT pixel maps to an
// exact NxN block of real pixels. Recomputed when the displayed size changes.
float s_scanline_pitch_px = 1.0f;

// Displayed (dst-rect) size the pitch was last resolved for. A change means a
// window resize or a move to a different-DPI monitor → re-resolve.
int32_t s_pitch_resolved_w = 0;
int32_t s_pitch_resolved_h = 0;

// Resolve the integer scanline pitch from the display's physical density.
// Rounding to a whole number of physical pixels is the key to crisp scanlines:
// a fractional pitch (e.g. 2.3) makes scanlines drift relative to the pixel
// grid and the image smears; snapping to N px aligns each CRT pixel to an exact
// NxN block of real pixels. Cheap; called only on size change, never per frame.
void resolve_scanline_pitch()
{
    float display_ppcm = 0.0f;
    const bool detected = sdl3_display_get_physical_ppcm(&display_ppcm) && display_ppcm > 0.0f;
    if (!detected)
        display_ppcm = DISPLAY_PPCM_FALLBACK;

    // Snap to nearest whole physical pixel; floor at 1 to stay a valid divisor.
    int32_t pitch = (int32_t)(display_ppcm / CRT_PIXELS_PER_CM + 0.5f);
    if (pitch < 1)
        pitch = 1;

    const float new_pitch = (float)pitch;
    if (new_pitch != s_scanline_pitch_px) {
        fprintf(stderr,
            "CRT effect: %.1f scanlines/cm, display %.1f px/cm (%s) -> %d px/scanline\n",
            (double)CRT_PIXELS_PER_CM, (double)display_ppcm,
            detected ? "OS auto-detect" : "fallback (96 DPI)", pitch);
    }
    s_scanline_pitch_px = new_pitch;
}

} // namespace

void crt_effect_init()
{
    g_crt_enabled = OC_CONFIG_get_int("video", "crt_effect", 0) != 0;

    // Initial resolve; crt_effect_apply re-resolves whenever the dst size changes.
    resolve_scanline_pitch();
}

namespace {

GLuint s_program = 0;
GLint s_u_source = -1;
GLint s_u_game_rect = -1;
GLint s_u_emu_size = -1;
GLint s_u_scanline_weight = -1;
GLint s_u_mask_dark = -1;
GLint s_u_mask_light = -1;
GLint s_u_halation_strength = -1;
GLint s_u_halation_radius = -1;
GLint s_u_vignette_strength = -1;
GLint s_u_brightness = -1;
GLint s_u_warmth = -1;

// Scratch texture — persistent copy of the game area from the default FB.
GLuint s_scratch_tex = 0;
int32_t s_scratch_w = 0;
int32_t s_scratch_h = 0;

GLuint s_vao = 0;
GLuint s_vbo = 0;

// Tuning: crt-geom beam model, flat screen, dark 3D game.
//
// CRT_SCANLINE_WEIGHT: beam width multiplier.
//   1.0 = sharp (wide dark gap), 3.0 = soft (gap almost filled).
//   At 1.5 dark areas show a clear gap, bright areas (explosions) fill in.
//
// Phosphor mask: very subtle (3D polygonal game, not pixel art).
//   1.0 / 1.0 = completely off.
//
// Halation is the main visual effect for dark 3D: streetlights, explosions,
// and gunfire bleed warm light into surrounding dark areas.
constexpr float CRT_SCANLINE_WEIGHT = 1.5f; // beam width (1=sharp gap, 3=soft)
constexpr float CRT_MASK_DARK = 0.85f; // subtle aperture-grille
constexpr float CRT_MASK_LIGHT = 1.15f;
constexpr float CRT_HALATION_STRENGTH = 0.22f; // warm glow intensity
constexpr float CRT_HALATION_RADIUS = 2.5f; // glow spread in content pixels
constexpr float CRT_VIGNETTE_STRENGTH = 0.30f;
constexpr float CRT_BRIGHTNESS = 1.15f; // compensates scanline energy loss
constexpr float CRT_WARMTH = 0.4f; // warm phosphor colour temperature

bool ensure_program()
{
    if (s_program)
        return true;
    s_program = gl_shader_create_program(SHADER_FULLSCREEN_QUAD_VERT, SHADER_CRT_FRAG);
    if (!s_program) {
        fprintf(stderr, "CRT effect: failed to create shader program\n");
        return false;
    }
    s_u_source = glGetUniformLocation(s_program, "u_source");
    s_u_game_rect = glGetUniformLocation(s_program, "u_game_rect");
    s_u_emu_size = glGetUniformLocation(s_program, "u_emu_size");
    s_u_scanline_weight = glGetUniformLocation(s_program, "u_scanline_weight");
    s_u_mask_dark = glGetUniformLocation(s_program, "u_mask_dark");
    s_u_mask_light = glGetUniformLocation(s_program, "u_mask_light");
    s_u_halation_strength = glGetUniformLocation(s_program, "u_halation_strength");
    s_u_halation_radius = glGetUniformLocation(s_program, "u_halation_radius");
    s_u_vignette_strength = glGetUniformLocation(s_program, "u_vignette_strength");
    s_u_brightness = glGetUniformLocation(s_program, "u_brightness");
    s_u_warmth = glGetUniformLocation(s_program, "u_warmth");
    return true;
}

void ensure_scratch(int32_t w, int32_t h)
{
    if (s_scratch_tex && s_scratch_w == w && s_scratch_h == h)
        return;

    if (s_scratch_tex) {
        glDeleteTextures(1, &s_scratch_tex);
        s_scratch_tex = 0;
    }
    glGenTextures(1, &s_scratch_tex);
    if (!s_scratch_tex)
        return;

    GLint prev_tex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

    glBindTexture(GL_TEXTURE_2D, s_scratch_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // LINEAR so horizontal sub-content-pixel interpolation is smooth.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);
    s_scratch_w = w;
    s_scratch_h = h;
}

void ensure_quad()
{
    if (s_vao)
        return;
    static const float verts[] = {
        -1.0f,
        -1.0f,
        1.0f,
        -1.0f,
        -1.0f,
        1.0f,
        1.0f,
        1.0f,
    };
    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);
    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * (GLsizei)sizeof(float), (void*)0);
    glBindVertexArray(0);
}

} // namespace

void crt_effect_apply()
{
    if (!g_crt_enabled)
        return;

    int32_t game_x, game_y, game_w, game_h;
    composition_get_dst_rect(&game_x, &game_y, &game_w, &game_h);
    if (game_w <= 0 || game_h <= 0)
        return;

    // Re-resolve the integer scanline pitch when the displayed size changes
    // (window resize, or a move to a different-DPI monitor). Keeps CRT pixels
    // snapped to whole real pixels in the new situation.
    if (game_w != s_pitch_resolved_w || game_h != s_pitch_resolved_h) {
        resolve_scanline_pitch();
        s_pitch_resolved_w = game_w;
        s_pitch_resolved_h = game_h;
    }

    if (!ensure_program())
        return;
    ensure_scratch(game_w, game_h);
    if (!s_scratch_tex)
        return;
    ensure_quad();

    // ---- 1. Copy the full game area from default FB → scratch -------------
    GLint prev_tex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

    glBindTexture(GL_TEXTURE_2D, s_scratch_tex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
        0, 0,
        game_x, game_y,
        game_w, game_h);

    // ---- 2. Draw CRT-filtered result back to the game area ----------------
    GLint prev_program = 0;
    GLint prev_vao = 0;
    GLint prev_active = 0;
    GLint prev_vp[4] = {};
    GLint prev_sc[4] = {};
    GLboolean prev_sc_on = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prev_depth_on = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prev_blend_on = glIsEnabled(GL_BLEND);
    GLboolean prev_cull_on = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vao);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prev_active);
    glGetIntegerv(GL_VIEWPORT, prev_vp);
    glGetIntegerv(GL_SCISSOR_BOX, prev_sc);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glViewport(game_x, game_y, game_w, game_h);

    glUseProgram(s_program);
    glBindVertexArray(s_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_scratch_tex);
    glUniform1i(s_u_source, 0);

    glUniform4f(s_u_game_rect,
        (float)game_x, (float)game_y,
        (float)game_w, (float)game_h);

    // Emulated CRT grid: physical displayed size / scanline pitch. The pitch is
    // derived from the display's physical density (see crt_effect_init), so the
    // scanline spacing is a fixed number of real centimetres regardless of
    // resolution or pixel density. Driving the shader by this instead of the
    // scene FBO / render resolution is what makes the effect density-invariant.
    // Dividing both axes by the same pitch preserves the physical aspect and
    // keeps emulated pixels square.
    glUniform2f(s_u_emu_size,
        (float)game_w / s_scanline_pitch_px,
        (float)game_h / s_scanline_pitch_px);

    glUniform1f(s_u_scanline_weight, CRT_SCANLINE_WEIGHT);
    glUniform1f(s_u_mask_dark, CRT_MASK_DARK);
    glUniform1f(s_u_mask_light, CRT_MASK_LIGHT);
    glUniform1f(s_u_halation_strength, CRT_HALATION_STRENGTH);
    glUniform1f(s_u_halation_radius, CRT_HALATION_RADIUS);
    glUniform1f(s_u_vignette_strength, CRT_VIGNETTE_STRENGTH);
    glUniform1f(s_u_brightness, CRT_BRIGHTNESS);
    glUniform1f(s_u_warmth, CRT_WARMTH);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // ---- 3. Restore everything we touched ---------------------------------
    glUseProgram((GLuint)prev_program);
    glBindVertexArray((GLuint)prev_vao);
    glActiveTexture((GLenum)prev_active);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    glScissor(prev_sc[0], prev_sc[1], prev_sc[2], prev_sc[3]);
    if (prev_sc_on)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
    if (prev_depth_on)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (prev_blend_on)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    if (prev_cull_on)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}
