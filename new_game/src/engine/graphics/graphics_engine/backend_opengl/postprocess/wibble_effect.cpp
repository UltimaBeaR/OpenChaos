// GPU implementation of the water-reflection wibble post-process.
//
// The CPU version (WIBBLE_simple in engine/graphics/postprocess/wibble.cpp)
// per-row-shifted pixels of the locked backbuffer directly, which required
// glReadPixels + writeback — the exact driver/WDDM hang pattern we are
// eliminating in Stages 1-3 of the startup-hang fix.
//
// Here we:
//   1) glCopyTexSubImage2D from the default framebuffer into a persistent
//      single-sample scratch texture (server-side GPU→GPU, no CPU traffic).
//      On an MSAA default FB this is effectively a resolve-to-sample-0;
//      acceptable because the wibble distortion hides aliasing.
//   2) Draw a warped quad back into the same region with a fragment shader
//      that does the per-row sin/cos shift. Scissor ensures we don't touch
//      pixels outside the puddle bbox.
//
// Shader sources (SHADER_FULLSCREEN_QUAD_VERT, SHADER_WIBBLE_FRAG) are
// embedded by CMake from the .glsl files in backend_opengl/shaders/.

#include "engine/graphics/graphics_engine/backend_opengl/postprocess/wibble_effect.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/gl_context.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/gl_shader.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"

#include <stdio.h>
#include <stdint.h>

#include "gl_shaders_embedded.h"

namespace {

// Shader program + cached uniform locations (lazy init).
GLuint s_program = 0;
GLint  s_u_source       = -1;
GLint  s_u_source_size  = -1;
GLint  s_u_rect         = -1;
GLint  s_u_wibble_y1    = -1;
GLint  s_u_wibble_y2    = -1;
GLint  s_u_wibble_s1    = -1;
GLint  s_u_wibble_s2    = -1;
GLint  s_u_phase1       = -1;
GLint  s_u_phase2       = -1;

// Persistent scratch color texture, resized when the framebuffer grows.
GLuint  s_scratch_tex = 0;
int32_t s_scratch_w   = 0;
int32_t s_scratch_h   = 0;

// Dedicated VAO/VBO for the NDC fullscreen quad (2D positions only).
GLuint s_vao = 0;
GLuint s_vbo = 0;

// Angle-space constants mirroring the CPU SIN/COS LUT (2048 units per turn).
constexpr int32_t ANGLE_UNITS = 2048;
constexpr int32_t ANGLE_MASK  = ANGLE_UNITS - 1;

bool ensure_program()
{
    if (s_program) return true;

    s_program = gl_shader_create_program(SHADER_FULLSCREEN_QUAD_VERT, SHADER_WIBBLE_FRAG);
    if (!s_program) {
        fprintf(stderr, "Failed to create wibble shader program\n");
        return false;
    }

    s_u_source      = glGetUniformLocation(s_program, "u_source");
    s_u_source_size = glGetUniformLocation(s_program, "u_source_size");
    s_u_rect        = glGetUniformLocation(s_program, "u_rect");
    s_u_wibble_y1   = glGetUniformLocation(s_program, "u_wibble_y1");
    s_u_wibble_y2   = glGetUniformLocation(s_program, "u_wibble_y2");
    s_u_wibble_s1   = glGetUniformLocation(s_program, "u_wibble_s1");
    s_u_wibble_s2   = glGetUniformLocation(s_program, "u_wibble_s2");
    s_u_phase1      = glGetUniformLocation(s_program, "u_phase1");
    s_u_phase2      = glGetUniformLocation(s_program, "u_phase2");
    return true;
}

void ensure_scratch(int32_t fb_w, int32_t fb_h)
{
    if (s_scratch_tex && s_scratch_w == fb_w && s_scratch_h == fb_h) return;

    if (s_scratch_tex) {
        glDeleteTextures(1, &s_scratch_tex);
        s_scratch_tex = 0;
    }

    glGenTextures(1, &s_scratch_tex);
    if (!s_scratch_tex) return;

    GLint prev_tex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

    glBindTexture(GL_TEXTURE_2D, s_scratch_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fb_w, fb_h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // Nearest filtering — wibble samples at fractional-pixel offsets but the
    // input is full-resolution and the per-row shift is what we want to
    // visualize, not a smooth interpolation between columns.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);

    s_scratch_w = fb_w;
    s_scratch_h = fb_h;
}

void ensure_quad()
{
    if (s_vao) return;

    // NDC triangle strip covering [-1, +1] — the actual affected region is
    // clipped by glScissor on every draw.
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

void ge_apply_wibble(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                     const GEWibbleParams& p)
{
    if (x2 <= x1 || y2 <= y1) return;

    int32_t fb_w = gl_context_get_width();
    int32_t fb_h = gl_context_get_height();
    if (fb_w <= 0 || fb_h <= 0) return;

    // Clamp to framebuffer — puddle bbox from aeng.cpp can extend slightly
    // off-screen at the edges (bounding box of transformed world quad).
    if (x1 < 0)    x1 = 0;
    if (y1 < 0)    y1 = 0;
    if (x2 > fb_w) x2 = fb_w;
    if (y2 > fb_h) y2 = fb_h;
    if (x2 <= x1 || y2 <= y1) return;

    if (!ensure_program()) return;
    ensure_scratch(fb_w, fb_h);
    if (!s_scratch_tex) return;
    ensure_quad();

    // Game coords are top-down; GL pixel coords are bottom-up. The scratch
    // texture uses the GL (bottom-up) orientation so dst and src share the
    // same layout; we just flip y when computing the GL rect.
    const int32_t reg_w       = x2 - x1;
    const int32_t reg_h       = y2 - y1;
    const int32_t gl_y_bottom = fb_h - y2;

    // Widen the area we copy into scratch by the maximum possible horizontal
    // shift, so the shader can sample outside the puddle rect and still read
    // fresh framebuffer content instead of stale pixels from earlier frames.
    // Max |offset| = (s1 + s2) / 8 per the AMP_SCALE constant; + 1 px safety.
    const int32_t max_shift = ((int32_t)p.wibble_s1 + (int32_t)p.wibble_s2) / 8 + 1;
    int32_t copy_x1 = x1 - max_shift;
    int32_t copy_x2 = x2 + max_shift;
    if (copy_x1 < 0)    copy_x1 = 0;
    if (copy_x2 > fb_w) copy_x2 = fb_w;
    const int32_t copy_w = copy_x2 - copy_x1;

    // ---- 1. Copy the puddle region (+ shift margin) from default FB →
    // ---- scratch texture ------------------------------------------------
    // Preserves current TEXTURE_2D binding on unit 0 so we don't desync
    // GERenderState's texture cache (which skips glBindTexture on no-change).
    GLint prev_gl_tex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_gl_tex);

    glBindTexture(GL_TEXTURE_2D, s_scratch_tex);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                        copy_x1, gl_y_bottom,   // dst offset in scratch tex
                        copy_x1, gl_y_bottom,   // src offset in read FB
                        copy_w,  reg_h);

    // ---- 2. Draw the warped rect back into the default FB ---------------
    // Save GL state we touch directly. ge_push_render_state covers the GE-
    // cached state (blend/depth/alpha-test/fog/texture-blend/bound-texture);
    // the rest we save by hand — viewport, scissor, program, VAO — because
    // those aren't part of the GE cache.
    ge_push_render_state();

    GLint     prev_program = 0;
    GLint     prev_vao     = 0;
    GLint     prev_vp[4]   = {};
    GLint     prev_sc[4]   = {};
    GLboolean prev_sc_on   = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prev_cull_on = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_CURRENT_PROGRAM,       &prev_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,  &prev_vao);
    glGetIntegerv(GL_VIEWPORT,               prev_vp);
    glGetIntegerv(GL_SCISSOR_BOX,            prev_sc);

    glViewport(0, 0, fb_w, fb_h);
    glEnable(GL_SCISSOR_TEST);
    glScissor(x1, gl_y_bottom, reg_w, reg_h);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    // Quad winding is CCW in window space but the game toggles GL_FRONT_FACE
    // per cull mode — disable culling outright so we don't render-cull to a
    // black rect under whichever setting was active.
    glDisable(GL_CULL_FACE);

    glUseProgram(s_program);
    glBindVertexArray(s_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_scratch_tex);
    glUniform1i(s_u_source, 0);
    glUniform2f(s_u_source_size, (float)fb_w, (float)fb_h);
    glUniform4f(s_u_rect,
                (float)x1,      (float)gl_y_bottom,
                (float)x2,      (float)(gl_y_bottom + reg_h));
    glUniform1f(s_u_wibble_y1, (float)p.wibble_y1);
    glUniform1f(s_u_wibble_y2, (float)p.wibble_y2);
    glUniform1f(s_u_wibble_s1, (float)p.wibble_s1);
    glUniform1f(s_u_wibble_s2, (float)p.wibble_s2);
    // Fold GAME_TURN * g into [0, 2048) on the CPU. Keeping the accumulated
    // phase small preserves float precision in the shader (GAME_TURN alone
    // grows unbounded over a session, and GAME_TURN * 255 overflows the 23-
    // bit float mantissa after ~65k frames).
    const uint32_t phase1 = ((uint32_t)p.game_turn * (uint32_t)p.wibble_g1) & ANGLE_MASK;
    const uint32_t phase2 = ((uint32_t)p.game_turn * (uint32_t)p.wibble_g2) & ANGLE_MASK;
    glUniform1f(s_u_phase1, (float)phase1);
    glUniform1f(s_u_phase2, (float)phase2);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // ---- 3. Restore every GL bit we changed ------------------------------
    glUseProgram((GLuint)prev_program);
    glBindVertexArray((GLuint)prev_vao);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    glScissor(prev_sc[0], prev_sc[1], prev_sc[2], prev_sc[3]);
    if (prev_sc_on)   glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prev_cull_on) glEnable(GL_CULL_FACE);    else glDisable(GL_CULL_FACE);

    // Texture binding on unit 0: the GERenderState uniform snapshot may skip
    // re-binding on the next draw if its cached s_bound_texture hasn't
    // changed — restore the real GL binding here so the cache and GL agree.
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_gl_tex);

    ge_pop_render_state();
}

