// CRT-Lottes scanline post-process effect.
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
#include "config.h"

#include <stdio.h>
#include <stdint.h>

#include "gl_shaders_embedded.h"

bool g_crt_enabled = OC_CRT_ENABLE;

namespace {

GLuint s_program            = 0;
GLint  s_u_source           = -1;
GLint  s_u_game_rect        = -1;
GLint  s_u_content_size     = -1;
GLint  s_u_hard_scan        = -1;
GLint  s_u_hard_pix         = -1;
GLint  s_u_warp_x           = -1;
GLint  s_u_warp_y           = -1;
GLint  s_u_mask_dark        = -1;
GLint  s_u_mask_light       = -1;
GLint  s_u_bloom_amount     = -1;
GLint  s_u_hard_bloom_pix   = -1;
GLint  s_u_hard_bloom_scan  = -1;
GLint  s_u_shape            = -1;

// Scratch texture — persistent copy of the game area from the default FB.
// Resized when the game area changes.  Sized to game area (not full window)
// so the shader can address it directly with game_uv [0, 1].
GLuint  s_scratch_tex = 0;
int32_t s_scratch_w   = 0;
int32_t s_scratch_h   = 0;

GLuint s_vao = 0;
GLuint s_vbo = 0;

// Default tuning constants matching crt-lottes original values.
constexpr float CRT_HARD_SCAN       = -8.0f;   // soft scanlines
constexpr float CRT_HARD_PIX        = -3.0f;   // soft horizontal pixels
constexpr float CRT_WARP_X          = 0.031f;  // mild barrel curvature X
constexpr float CRT_WARP_Y          = 0.041f;  // mild barrel curvature Y
constexpr float CRT_MASK_DARK       = 0.5f;
constexpr float CRT_MASK_LIGHT      = 1.5f;
constexpr float CRT_BLOOM_AMOUNT    = 0.15f;
constexpr float CRT_HARD_BLOOM_PIX  = -1.5f;
constexpr float CRT_HARD_BLOOM_SCAN = -2.0f;
constexpr float CRT_SHAPE           = 2.0f;    // Gaussian kernel shape

bool ensure_program()
{
    if (s_program) return true;
    s_program = gl_shader_create_program(SHADER_FULLSCREEN_QUAD_VERT, SHADER_CRT_FRAG);
    if (!s_program) {
        fprintf(stderr, "CRT effect: failed to create shader program\n");
        return false;
    }
    s_u_source          = glGetUniformLocation(s_program, "u_source");
    s_u_game_rect       = glGetUniformLocation(s_program, "u_game_rect");
    s_u_content_size    = glGetUniformLocation(s_program, "u_content_size");
    s_u_hard_scan       = glGetUniformLocation(s_program, "u_hard_scan");
    s_u_hard_pix        = glGetUniformLocation(s_program, "u_hard_pix");
    s_u_warp_x          = glGetUniformLocation(s_program, "u_warp_x");
    s_u_warp_y          = glGetUniformLocation(s_program, "u_warp_y");
    s_u_mask_dark       = glGetUniformLocation(s_program, "u_mask_dark");
    s_u_mask_light      = glGetUniformLocation(s_program, "u_mask_light");
    s_u_bloom_amount    = glGetUniformLocation(s_program, "u_bloom_amount");
    s_u_hard_bloom_pix  = glGetUniformLocation(s_program, "u_hard_bloom_pix");
    s_u_hard_bloom_scan = glGetUniformLocation(s_program, "u_hard_bloom_scan");
    s_u_shape           = glGetUniformLocation(s_program, "u_shape");
    return true;
}

void ensure_scratch(int32_t w, int32_t h)
{
    if (s_scratch_tex && s_scratch_w == w && s_scratch_h == h) return;

    if (s_scratch_tex) {
        glDeleteTextures(1, &s_scratch_tex);
        s_scratch_tex = 0;
    }
    glGenTextures(1, &s_scratch_tex);
    if (!s_scratch_tex) return;

    GLint prev_tex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

    glBindTexture(GL_TEXTURE_2D, s_scratch_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // LINEAR so the Gaussian filter gets proper sub-pixel interpolation.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);
    s_scratch_w = w;
    s_scratch_h = h;
}

void ensure_quad()
{
    if (s_vao) return;
    static const float verts[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
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
    if (!g_crt_enabled) return;

    int32_t game_x, game_y, game_w, game_h;
    composition_get_dst_rect(&game_x, &game_y, &game_w, &game_h);
    if (game_w <= 0 || game_h <= 0) return;

    if (!ensure_program()) return;
    ensure_scratch(game_w, game_h);
    if (!s_scratch_tex) return;
    ensure_quad();

    // ---- 1. Copy the full game area (game + UI) from default FB → scratch --
    // Preserves current texture binding so we don't desync the engine cache.
    GLint prev_tex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

    glBindTexture(GL_TEXTURE_2D, s_scratch_tex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                        0, 0,           // dst offset in scratch (origin)
                        game_x, game_y, // src offset in default FB (GL coords)
                        game_w, game_h);

    // ---- 2. Draw CRT-filtered result back to the game area ----------------
    GLint     prev_program  = 0;
    GLint     prev_vao      = 0;
    GLint     prev_active   = 0;
    GLint     prev_vp[4]    = {};
    GLint     prev_sc[4]    = {};
    GLboolean prev_sc_on    = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prev_depth_on = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prev_blend_on = glIsEnabled(GL_BLEND);
    GLboolean prev_cull_on  = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_CURRENT_PROGRAM,      &prev_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vao);
    glGetIntegerv(GL_ACTIVE_TEXTURE,       &prev_active);
    glGetIntegerv(GL_VIEWPORT,              prev_vp);
    glGetIntegerv(GL_SCISSOR_BOX,           prev_sc);

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

    // u_game_rect tells the shader how to compute game_uv from gl_FragCoord.
    glUniform4f(s_u_game_rect,
                (float)game_x, (float)game_y,
                (float)game_w, (float)game_h);
    // u_content_size drives the scanline grid — matches the virtual game
    // resolution (scene FBO size, e.g. 640×480), not the scratch texture size.
    glUniform2f(s_u_content_size,
                (float)composition_scene_width(),
                (float)composition_scene_height());

    glUniform1f(s_u_hard_scan,       CRT_HARD_SCAN);
    glUniform1f(s_u_hard_pix,        CRT_HARD_PIX);
    glUniform1f(s_u_warp_x,          CRT_WARP_X);
    glUniform1f(s_u_warp_y,          CRT_WARP_Y);
    glUniform1f(s_u_mask_dark,       CRT_MASK_DARK);
    glUniform1f(s_u_mask_light,      CRT_MASK_LIGHT);
    glUniform1f(s_u_bloom_amount,    CRT_BLOOM_AMOUNT);
    glUniform1f(s_u_hard_bloom_pix,  CRT_HARD_BLOOM_PIX);
    glUniform1f(s_u_hard_bloom_scan, CRT_HARD_BLOOM_SCAN);
    glUniform1f(s_u_shape,           CRT_SHAPE);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // ---- 3. Restore everything we touched ---------------------------------
    glUseProgram((GLuint)prev_program);
    glBindVertexArray((GLuint)prev_vao);
    glActiveTexture((GLenum)prev_active);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    glScissor(prev_sc[0], prev_sc[1], prev_sc[2], prev_sc[3]);
    if (prev_sc_on)    glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prev_depth_on) glEnable(GL_DEPTH_TEST);   else glDisable(GL_DEPTH_TEST);
    if (prev_blend_on) glEnable(GL_BLEND);        else glDisable(GL_BLEND);
    if (prev_cull_on)  glEnable(GL_CULL_FACE);    else glDisable(GL_CULL_FACE);
}

void crt_effect_shutdown()
{
    if (s_program)     { glDeleteProgram(s_program);          s_program = 0; }
    if (s_scratch_tex) { glDeleteTextures(1, &s_scratch_tex); s_scratch_tex = 0; }
    if (s_vao)         { glDeleteVertexArrays(1, &s_vao);     s_vao = 0; }
    if (s_vbo)         { glDeleteBuffers(1, &s_vbo);          s_vbo = 0; }
    s_scratch_w = s_scratch_h = 0;
    s_u_source = s_u_game_rect = s_u_content_size = -1;
    s_u_hard_scan = s_u_hard_pix = s_u_warp_x = s_u_warp_y = -1;
    s_u_mask_dark = s_u_mask_light = s_u_bloom_amount = -1;
    s_u_hard_bloom_pix = s_u_hard_bloom_scan = s_u_shape = -1;
}
