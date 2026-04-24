// Composition module — scene FBO + final upscale + FXAA pass.
// See composition.h for the per-frame flow.

#include "engine/graphics/graphics_engine/backend_opengl/postprocess/composition.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/gl_shader.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
#include "config.h"

#include <stdio.h>

#include "gl_shaders_embedded.h"

namespace {

// Scene FBO + attachments.
GLuint s_fbo        = 0;
GLuint s_color_tex  = 0;
GLuint s_depth_tex  = 0;
int32_t s_scene_w   = 0;
int32_t s_scene_h   = 0;

// Composite shader + cached uniform locations.
GLuint s_program         = 0;
GLint  s_u_scene         = -1;
GLint  s_u_inv_scene_sz  = -1;
GLint  s_u_dst_sz        = -1;
GLint  s_u_dst_offset    = -1;
GLint  s_u_fxaa_enabled  = -1;

// Last composition target rectangle on the real backbuffer (pixels,
// origin top-left for storage; composition_blit flips to bottom-left for
// glViewport). Used by input mapping (Stage 6) to translate mouse-event
// window coordinates back into FBO coordinates.
int32_t s_last_dst_x = 0;
int32_t s_last_dst_y = 0;
int32_t s_last_dst_w = 0;
int32_t s_last_dst_h = 0;
int32_t s_last_real_w = 0;
int32_t s_last_real_h = 0;

// Fullscreen quad VAO/VBO (NDC triangle strip covering [-1, +1]).
GLuint s_vao = 0;
GLuint s_vbo = 0;

bool create_quad()
{
    static const float verts[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };
    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);
    if (!s_vao || !s_vbo) return false;

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          2 * (GLsizei)sizeof(float), (void*)0);
    glBindVertexArray(0);
    return true;
}

bool create_program()
{
    if (s_program) return true;
    s_program = gl_shader_create_program(SHADER_FULLSCREEN_QUAD_VERT,
                                         SHADER_COMPOSITE_FRAG);
    if (!s_program) {
        fprintf(stderr, "Composition: failed to create composite shader program\n");
        return false;
    }
    s_u_scene        = glGetUniformLocation(s_program, "u_scene");
    s_u_inv_scene_sz = glGetUniformLocation(s_program, "u_inv_scene_size");
    s_u_dst_sz       = glGetUniformLocation(s_program, "u_dst_size");
    s_u_dst_offset   = glGetUniformLocation(s_program, "u_dst_offset");
    s_u_fxaa_enabled = glGetUniformLocation(s_program, "u_fxaa_enabled");
    return true;
}

void destroy_attachments()
{
    if (s_fbo)        { glDeleteFramebuffers(1, &s_fbo);         s_fbo = 0; }
    if (s_color_tex)  { glDeleteTextures(1, &s_color_tex);       s_color_tex = 0; }
    if (s_depth_tex)  { glDeleteTextures(1, &s_depth_tex);       s_depth_tex = 0; }
    s_scene_w = s_scene_h = 0;
}

bool create_attachments(int32_t w, int32_t h)
{
    destroy_attachments();
    if (w <= 0 || h <= 0) return false;

    // Preserve prior bindings so we don't clobber caller GL state.
    GLint prev_fbo  = 0;
    GLint prev_tex  = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prev_fbo);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,       &prev_tex);

    glGenFramebuffers(1, &s_fbo);
    glGenTextures(1, &s_color_tex);
    glGenTextures(1, &s_depth_tex);
    if (!s_fbo || !s_color_tex || !s_depth_tex) {
        destroy_attachments();
        return false;
    }

    // Color attachment: RGBA8 texture. LINEAR filtering so the composition
    // pass gets bilinear upscale for free when the scene FBO is smaller
    // than the window. CLAMP_TO_EDGE so FXAA's neighborhood taps at the
    // frame border don't wrap.
    glBindTexture(GL_TEXTURE_2D, s_color_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    // Depth attachment: 24-bit depth texture. Texture (not RBO) so future
    // post-process passes can sample depth — SSAO, soft particles, depth
    // fog etc. Sampler filtering for depth textures must be NEAREST on
    // core-profile GL when used as a plain sampler2D (not sampler2DShadow).
    glBindTexture(GL_TEXTURE_2D, s_depth_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, s_color_tex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, s_depth_tex, 0);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    bool ok = (status == GL_FRAMEBUFFER_COMPLETE);
    if (!ok) {
        fprintf(stderr, "Composition: scene FBO incomplete (status=0x%x)\n", status);
    } else {
        s_scene_w = w;
        s_scene_h = h;
    }

    // Restore.
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prev_fbo);
    glBindTexture(GL_TEXTURE_2D,      (GLuint)prev_tex);

    if (!ok) destroy_attachments();
    return ok;
}

} // namespace

bool composition_init(int32_t scene_w, int32_t scene_h)
{
    if (!create_program()) return false;
    if (!s_vao && !create_quad()) return false;
    return create_attachments(scene_w, scene_h);
}

void composition_shutdown()
{
    destroy_attachments();
    if (s_program) { glDeleteProgram(s_program);       s_program = 0; }
    if (s_vbo)     { glDeleteBuffers(1, &s_vbo);       s_vbo = 0; }
    if (s_vao)     { glDeleteVertexArrays(1, &s_vao);  s_vao = 0; }
    s_u_scene = s_u_inv_scene_sz = s_u_dst_sz = s_u_dst_offset = s_u_fxaa_enabled = -1;
}

bool composition_resize(int32_t scene_w, int32_t scene_h)
{
    if (scene_w == s_scene_w && scene_h == s_scene_h && s_fbo) return true;
    return create_attachments(scene_w, scene_h);
}

void composition_bind_scene()
{
    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
}

void composition_bind_default()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void composition_blit(int32_t window_w, int32_t window_h)
{
    if (!s_program || !s_color_tex || window_w <= 0 || window_h <= 0) return;

    // Save GL state we touch so we don't desync GERenderState's caches.
    GLint     prev_program = 0;
    GLint     prev_vao     = 0;
    GLint     prev_tex     = 0;
    GLint     prev_active  = 0;
    GLint     prev_vp[4]   = {};
    GLint     prev_sc[4]   = {};
    GLfloat   prev_clear[4] = {};
    GLboolean prev_color_mask[4] = {};
    GLboolean prev_depth_mask = GL_TRUE;
    GLboolean prev_sc_on   = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prev_cull_on = glIsEnabled(GL_CULL_FACE);
    GLboolean prev_depth_on = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prev_blend_on = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_CURRENT_PROGRAM,       &prev_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,  &prev_vao);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,    &prev_tex);
    glGetIntegerv(GL_ACTIVE_TEXTURE,        &prev_active);
    glGetIntegerv(GL_VIEWPORT,               prev_vp);
    glGetIntegerv(GL_SCISSOR_BOX,            prev_sc);
    glGetFloatv  (GL_COLOR_CLEAR_VALUE,      prev_clear);
    glGetBooleanv(GL_COLOR_WRITEMASK,        prev_color_mask);
    glGetBooleanv(GL_DEPTH_WRITEMASK,        &prev_depth_mask);

    // Aspect-fit the scene FBO into a centred rectangle on the real
    // backbuffer, leave the leftover area black. This is the only layer
    // that reconciles "real window size" with "FBO size"; everything else
    // in the engine sees the FBO as its entire screen.
    int32_t dst_x = 0, dst_y = 0, dst_w = window_w, dst_h = window_h;
    if (s_scene_w > 0 && s_scene_h > 0) {
        const float fbo_aspect  = (float)s_scene_w / (float)s_scene_h;
        const float real_aspect = (float)window_w  / (float)window_h;
        if (real_aspect > fbo_aspect) {
            // Real is wider than FBO → pillarbox (bars on left/right).
            dst_h = window_h;
            dst_w = (int32_t)((float)window_h * fbo_aspect + 0.5f);
            if (dst_w > window_w) dst_w = window_w;
            dst_x = (window_w - dst_w) / 2;
            dst_y = 0;
        } else if (real_aspect < fbo_aspect) {
            // Real is taller than FBO → letterbox (bars on top/bottom).
            dst_w = window_w;
            dst_h = (int32_t)((float)window_w / fbo_aspect + 0.5f);
            if (dst_h > window_h) dst_h = window_h;
            dst_x = 0;
            dst_y = (window_h - dst_h) / 2;
        }
    }

    // Publish for input mapping.
    s_last_dst_x  = dst_x;
    s_last_dst_y  = dst_y;
    s_last_dst_w  = dst_w;
    s_last_dst_h  = dst_h;
    s_last_real_w = window_w;
    s_last_real_h = window_h;

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    // Force full write masks before clear — prior game draws may have
    // restricted glColorMask/glDepthMask (e.g. depth-only passes), which
    // would cause glClear to leave pixels untouched and the backbuffer
    // to carry stale content from N-2 frames ago under double-buffering.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);

    // Clear the entire real backbuffer unconditionally so outer pillar/
    // letterbox bars are painted fresh every frame. Free on modern GPUs.
    glViewport(0, 0, window_w, window_h);
    // TEMP: outer bars painted red for visual verification during the
    // FBO-as-virtual-screen refactor — makes it obvious where the scene
    // FBO ends and the composition layer begins. Revert to (0,0,0,1)
    // once the refactor is done.
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Now narrow the viewport to the aspect-fit rect and draw the FBO
    // into it. The composite shader reads gl_FragCoord in real-backbuffer
    // pixels and maps it to scene UVs via (u_dst_offset, u_dst_size).
    glViewport(dst_x, dst_y, dst_w, dst_h);

    glUseProgram(s_program);
    glBindVertexArray(s_vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_color_tex);
    glUniform1i(s_u_scene, 0);

    const float inv_w = (s_scene_w > 0) ? 1.0f / (float)s_scene_w : 0.0f;
    const float inv_h = (s_scene_h > 0) ? 1.0f / (float)s_scene_h : 0.0f;
    glUniform2f(s_u_inv_scene_sz, inv_w, inv_h);
    glUniform2f(s_u_dst_sz,       (float)dst_w, (float)dst_h);
    glUniform2f(s_u_dst_offset,   (float)dst_x, (float)dst_y);
    glUniform1i(s_u_fxaa_enabled, OC_AA_ENABLE ? 1 : 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Restore everything we touched.
    glUseProgram((GLuint)prev_program);
    glBindVertexArray((GLuint)prev_vao);
    glActiveTexture((GLenum)prev_active);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    glScissor(prev_sc[0], prev_sc[1], prev_sc[2], prev_sc[3]);
    glClearColor(prev_clear[0], prev_clear[1], prev_clear[2], prev_clear[3]);
    glColorMask(prev_color_mask[0], prev_color_mask[1],
                prev_color_mask[2], prev_color_mask[3]);
    glDepthMask(prev_depth_mask);
    if (prev_sc_on)    glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    if (prev_cull_on)  glEnable(GL_CULL_FACE);    else glDisable(GL_CULL_FACE);
    if (prev_depth_on) glEnable(GL_DEPTH_TEST);   else glDisable(GL_DEPTH_TEST);
    if (prev_blend_on) glEnable(GL_BLEND);        else glDisable(GL_BLEND);
}

int32_t composition_scene_width()  { return s_scene_w; }
int32_t composition_scene_height() { return s_scene_h; }

bool composition_window_to_fbo(int win_x, int win_y, int* fbo_x, int* fbo_y)
{
    // No frame composited yet → no dst rect to map against. Treat as
    // "inside" with identity mapping so the first few events (e.g. an
    // early mouse-move before ge_flip runs) don't get dropped; callers
    // can still clamp on their end.
    if (s_last_dst_w <= 0 || s_last_dst_h <= 0 || s_scene_w <= 0 || s_scene_h <= 0) {
        if (fbo_x) *fbo_x = win_x;
        if (fbo_y) *fbo_y = win_y;
        return true;
    }

    // Reject events in the outer bar area. UI buttons can only live
    // inside the FBO, so a click outside the blit rect cannot plausibly
    // intend to hit anything — dropping is cleaner than clamp-to-edge.
    if (win_x < s_last_dst_x || win_x >= s_last_dst_x + s_last_dst_w) return false;
    if (win_y < s_last_dst_y || win_y >= s_last_dst_y + s_last_dst_h) return false;

    // Linear map: (window_px - dst_origin) × (fbo_size / dst_size).
    const float fx = (float)(win_x - s_last_dst_x) * (float)s_scene_w / (float)s_last_dst_w;
    const float fy = (float)(win_y - s_last_dst_y) * (float)s_scene_h / (float)s_last_dst_h;
    if (fbo_x) *fbo_x = (int)(fx + 0.5f);
    if (fbo_y) *fbo_y = (int)(fy + 0.5f);
    return true;
}
