// Graphics engine — OpenGL 4.1 core profile implementation.
// Phase 1: lifecycle, clear, flip, render state, viewport.
// Phase 2: shaders + drawing (ge_draw_indexed_primitive, ge_draw_indexed_primitive_lit).

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/graphics_engine/outro_graphics_engine.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/gl_context.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/gl_shader.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
#include "engine/platform/uc_common.h"
#include "engine/io/env.h"
#include "engine/io/file.h"
#include "engine/core/memory.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Backend-local state
// ---------------------------------------------------------------------------

static char s_data_dir[256] = "";

// Callbacks registered by game code.
static GEPreFlipCallback              s_pre_flip_callback = nullptr;
static GEModeChangeCallback           s_mode_change_callback = nullptr;
static GERenderStatesResetCallback    s_render_states_reset_callback = nullptr;
static GETGALoadCallback              s_tga_load_callback = nullptr;
static GETextureReloadPrepareCallback s_texture_reload_prepare_callback = nullptr;

// Background color for ge_clear.
static float s_clear_r = 0.0f, s_clear_g = 0.0f, s_clear_b = 0.0f;

// Current viewport (D3D screen coordinates: top-left origin).
static int32_t s_vp_x = 0, s_vp_y = 0, s_vp_w = 640, s_vp_h = 480;

// Fog state.
static bool     s_fog_enabled = false;
static uint32_t s_fog_color = 0;
static float    s_fog_near = 0.0f, s_fog_far = 1.0f;

// Alpha test state.
static bool     s_alpha_test_enabled = false;
static uint32_t s_alpha_ref = 0;
static GECompareFunc s_alpha_func = GECompareFunc::Always;

// Specular.
static bool s_specular_enabled = false;

// Color key.
static bool s_color_key_enabled = false;

// Texture blend mode (tracked for shader uniform).
static GETextureBlend s_texture_blend = GETextureBlend::Modulate;

// Currently bound texture (0 = none).
static GETextureHandle s_bound_texture = GE_TEXTURE_NONE;

// Matrices (for shader uniforms).
static GEMatrix s_world_matrix;
static GEMatrix s_view_matrix;
static GEMatrix s_projection_matrix;

// Fullscreen.
static bool s_fullscreen = false;

// Display changed flag.
static bool s_display_changed = false;

// Background override.
static GEScreenSurface s_background_override = GE_SCREEN_SURFACE_NONE;

// ===========================================================================
// Phase 2: Shader infrastructure
// ===========================================================================

// ---------------------------------------------------------------------------
// GLSL shader sources
// ---------------------------------------------------------------------------

// Vertex shader for pre-transformed vertices (GEVertexTL, screen-space).
// Converts D3D screen coordinates to OpenGL clip coordinates.
// D3D color format: 0xAARRGGBB — bytes in memory (little-endian): B, G, R, A.
// We read as GL_UNSIGNED_BYTE normalized, getting vec4(B, G, R, A), then swizzle.
static const char* const SHADER_TL_VERT = R"glsl(
#version 410 core

layout(location = 0) in vec3  a_position;  // screen x, y, z
layout(location = 1) in float a_rhw;       // reciprocal homogeneous W
layout(location = 2) in vec4  a_color;     // BGRA (normalized from uint32)
layout(location = 3) in vec4  a_specular;  // BGRA (normalized from uint32)
layout(location = 4) in vec2  a_texcoord;

// D3D viewport in screen coordinates (x, y, w, h).
uniform vec4 u_viewport;

out vec4 v_color;
out vec4 v_specular;
out vec2 v_texcoord;

void main()
{
    // Swizzle BGRA → RGBA.
    v_color    = a_color.zyxw;
    v_specular = a_specular.zyxw;
    v_texcoord = a_texcoord;

    // Screen space → NDC relative to viewport.
    float ndc_x = (a_position.x - u_viewport.x) / u_viewport.z * 2.0 - 1.0;
    float ndc_y = 1.0 - (a_position.y - u_viewport.y) / u_viewport.w * 2.0;
    float ndc_z = a_position.z * 2.0 - 1.0;

    // Preserve perspective-correct interpolation via W = 1/rhw.
    float w = 1.0 / a_rhw;
    gl_Position = vec4(ndc_x * w, ndc_y * w, ndc_z * w, w);
}
)glsl";

// Vertex shader for pre-lit world-space vertices (GEVertexLit).
// Applies World * View * Projection transform.
static const char* const SHADER_LIT_VERT = R"glsl(
#version 410 core

layout(location = 0) in vec3  a_position;  // world x, y, z
layout(location = 1) in float a_pad;       // _reserved padding
layout(location = 2) in vec4  a_color;     // BGRA (normalized from uint32)
layout(location = 3) in vec4  a_specular;  // BGRA (normalized from uint32)
layout(location = 4) in vec2  a_texcoord;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec4 v_color;
out vec4 v_specular;
out vec2 v_texcoord;

void main()
{
    v_color    = a_color.zyxw;
    v_specular = a_specular.zyxw;
    v_texcoord = a_texcoord;

    vec4 world_pos = u_world * vec4(a_position, 1.0);
    vec4 view_pos  = u_view * world_pos;
    gl_Position    = u_projection * view_pos;
}
)glsl";

// Shared fragment shader for both TL and Lit vertices.
// Handles: texture sampling, texture blend modes, alpha test, color key,
// fog (from specular alpha), and specular highlight addition.
static const char* const SHADER_FRAG = R"glsl(
#version 410 core

in vec4 v_color;
in vec4 v_specular;
in vec2 v_texcoord;

uniform int       u_has_texture;
uniform sampler2D u_texture;
uniform int       u_texture_blend;       // 0=Modulate, 1=ModulateAlpha, 2=Decal
uniform int       u_alpha_test_enabled;
uniform float     u_alpha_ref;           // normalized 0-1
uniform int       u_alpha_func;          // GECompareFunc as int
uniform int       u_fog_enabled;
uniform vec3      u_fog_color;
uniform int       u_specular_enabled;
uniform int       u_color_key_enabled;

out vec4 frag_color;

void main()
{
    vec4 color = v_color;

    if (u_has_texture != 0) {
        vec4 tex = texture(u_texture, v_texcoord);

        if (u_texture_blend == 0) {
            // Modulate: vertex RGB * texture RGB, alpha from vertex.
            color.rgb = color.rgb * tex.rgb;
        } else if (u_texture_blend == 1) {
            // ModulateAlpha: vertex * texture (all channels).
            color = color * tex;
        } else {
            // Decal: texture only, ignore vertex color.
            color = tex;
        }
    }

    // Color key: discard fully transparent pixels.
    if (u_color_key_enabled != 0 && color.a < 0.004)
        discard;

    // Alpha test.
    if (u_alpha_test_enabled != 0) {
        bool pass = true;
        // GECompareFunc: 0=Always, 1=Less, 2=LessEqual, 3=Greater, 4=GreaterEqual, 5=NotEqual
        if      (u_alpha_func == 1) pass = (color.a < u_alpha_ref);
        else if (u_alpha_func == 2) pass = (color.a <= u_alpha_ref);
        else if (u_alpha_func == 3) pass = (color.a > u_alpha_ref);
        else if (u_alpha_func == 4) pass = (color.a >= u_alpha_ref);
        else if (u_alpha_func == 5) pass = (abs(color.a - u_alpha_ref) > 0.001);
        if (!pass) discard;
    }

    // Specular highlight: add specular RGB to final color.
    if (u_specular_enabled != 0) {
        color.rgb += v_specular.rgb;
    }

    // Fog: linear blend using specular alpha as fog factor.
    // D3D6 convention: specular.a = 1.0 → no fog, 0.0 → fully fogged.
    if (u_fog_enabled != 0) {
        float fog_factor = v_specular.a;
        color.rgb = mix(u_fog_color, color.rgb, fog_factor);
    }

    frag_color = color;
}
)glsl";

// ---------------------------------------------------------------------------
// Shader programs and GL objects
// ---------------------------------------------------------------------------

static GLuint s_program_tl  = 0;  // TL vertex shader program
static GLuint s_program_lit = 0;  // Lit vertex shader program

// Uniform locations for TL program.
static GLint s_tl_u_viewport            = -1;
static GLint s_tl_u_has_texture         = -1;
static GLint s_tl_u_texture             = -1;
static GLint s_tl_u_texture_blend       = -1;
static GLint s_tl_u_alpha_test_enabled  = -1;
static GLint s_tl_u_alpha_ref           = -1;
static GLint s_tl_u_alpha_func          = -1;
static GLint s_tl_u_fog_enabled         = -1;
static GLint s_tl_u_fog_color           = -1;
static GLint s_tl_u_specular_enabled    = -1;
static GLint s_tl_u_color_key_enabled   = -1;

// Uniform locations for Lit program.
static GLint s_lit_u_world              = -1;
static GLint s_lit_u_view               = -1;
static GLint s_lit_u_projection         = -1;
static GLint s_lit_u_has_texture        = -1;
static GLint s_lit_u_texture            = -1;
static GLint s_lit_u_texture_blend      = -1;
static GLint s_lit_u_alpha_test_enabled = -1;
static GLint s_lit_u_alpha_ref          = -1;
static GLint s_lit_u_alpha_func         = -1;
static GLint s_lit_u_fog_enabled        = -1;
static GLint s_lit_u_fog_color          = -1;
static GLint s_lit_u_specular_enabled   = -1;
static GLint s_lit_u_color_key_enabled  = -1;

// VAO for each vertex format. VBO/EBO are shared (streaming).
static GLuint s_vao_tl  = 0;
static GLuint s_vao_lit = 0;
static GLuint s_vbo     = 0;  // streaming vertex buffer
static GLuint s_ebo     = 0;  // streaming index buffer

static bool s_shaders_ready = false;

// Cache uniform locations for a program's fragment shader uniforms.
static void cache_frag_uniforms(GLuint prog,
    GLint* u_has_texture, GLint* u_texture, GLint* u_texture_blend,
    GLint* u_alpha_test_enabled, GLint* u_alpha_ref, GLint* u_alpha_func,
    GLint* u_fog_enabled, GLint* u_fog_color,
    GLint* u_specular_enabled, GLint* u_color_key_enabled)
{
    *u_has_texture        = glGetUniformLocation(prog, "u_has_texture");
    *u_texture            = glGetUniformLocation(prog, "u_texture");
    *u_texture_blend      = glGetUniformLocation(prog, "u_texture_blend");
    *u_alpha_test_enabled = glGetUniformLocation(prog, "u_alpha_test_enabled");
    *u_alpha_ref          = glGetUniformLocation(prog, "u_alpha_ref");
    *u_alpha_func         = glGetUniformLocation(prog, "u_alpha_func");
    *u_fog_enabled        = glGetUniformLocation(prog, "u_fog_enabled");
    *u_fog_color          = glGetUniformLocation(prog, "u_fog_color");
    *u_specular_enabled   = glGetUniformLocation(prog, "u_specular_enabled");
    *u_color_key_enabled  = glGetUniformLocation(prog, "u_color_key_enabled");
}

// Set up VAO for GEVertexTL layout: 32 bytes per vertex.
// [0..11] vec3 position (x,y,z)  [12..15] float rhw
// [16..19] u8x4 color (BGRA)    [20..23] u8x4 specular (BGRA)
// [24..31] vec2 texcoord (u,v)
static void setup_vao_tl(GLuint vao, GLuint vbo, GLuint ebo)
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    const GLsizei stride = 32; // sizeof(GEVertexTL)

    // location 0: position (3 floats, offset 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    // location 1: rhw (1 float, offset 12)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride, (void*)12);

    // location 2: color (4 unsigned bytes, normalized, offset 16)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)16);

    // location 3: specular (4 unsigned bytes, normalized, offset 20)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)20);

    // location 4: texcoord (2 floats, offset 24)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (void*)24);

    glBindVertexArray(0);
}

// Set up VAO for GEVertexLit layout: 32 bytes per vertex.
// [0..11] vec3 position (x,y,z)  [12..15] uint32 _reserved
// [16..19] u8x4 color (BGRA)    [20..23] u8x4 specular (BGRA)
// [24..31] vec2 texcoord (u,v)
// Same memory layout as GEVertexTL but different semantic for position slot.
static void setup_vao_lit(GLuint vao, GLuint vbo, GLuint ebo)
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    const GLsizei stride = 32; // sizeof(GEVertexLit)

    // location 0: position (3 floats, offset 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    // location 1: _reserved padding (1 float, offset 12) — not used in shader but declared
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, stride, (void*)12);

    // location 2: color (4 unsigned bytes, normalized, offset 16)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)16);

    // location 3: specular (4 unsigned bytes, normalized, offset 20)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)20);

    // location 4: texcoord (2 floats, offset 24)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, stride, (void*)24);

    glBindVertexArray(0);
}

// Initialize shaders, VAOs, VBO, EBO. Called lazily on first draw.
static bool init_shaders()
{
    if (s_shaders_ready) return true;

    // Compile shader programs.
    s_program_tl = gl_shader_create_program(SHADER_TL_VERT, SHADER_FRAG);
    if (!s_program_tl) {
        fprintf(stderr, "Failed to create TL shader program\n");
        return false;
    }

    s_program_lit = gl_shader_create_program(SHADER_LIT_VERT, SHADER_FRAG);
    if (!s_program_lit) {
        fprintf(stderr, "Failed to create Lit shader program\n");
        return false;
    }

    // Cache TL uniform locations.
    s_tl_u_viewport = glGetUniformLocation(s_program_tl, "u_viewport");
    cache_frag_uniforms(s_program_tl,
        &s_tl_u_has_texture, &s_tl_u_texture, &s_tl_u_texture_blend,
        &s_tl_u_alpha_test_enabled, &s_tl_u_alpha_ref, &s_tl_u_alpha_func,
        &s_tl_u_fog_enabled, &s_tl_u_fog_color,
        &s_tl_u_specular_enabled, &s_tl_u_color_key_enabled);

    // Cache Lit uniform locations.
    s_lit_u_world      = glGetUniformLocation(s_program_lit, "u_world");
    s_lit_u_view       = glGetUniformLocation(s_program_lit, "u_view");
    s_lit_u_projection = glGetUniformLocation(s_program_lit, "u_projection");
    cache_frag_uniforms(s_program_lit,
        &s_lit_u_has_texture, &s_lit_u_texture, &s_lit_u_texture_blend,
        &s_lit_u_alpha_test_enabled, &s_lit_u_alpha_ref, &s_lit_u_alpha_func,
        &s_lit_u_fog_enabled, &s_lit_u_fog_color,
        &s_lit_u_specular_enabled, &s_lit_u_color_key_enabled);

    // Create shared VBO and EBO (streaming, orphaned each draw).
    glGenBuffers(1, &s_vbo);
    glGenBuffers(1, &s_ebo);

    // Create VAOs.
    glGenVertexArrays(1, &s_vao_tl);
    glGenVertexArrays(1, &s_vao_lit);

    setup_vao_tl(s_vao_tl, s_vbo, s_ebo);
    setup_vao_lit(s_vao_lit, s_vbo, s_ebo);

    s_shaders_ready = true;
    fprintf(stderr, "OpenGL shaders initialized (TL + Lit programs)\n");
    return true;
}

static void destroy_shaders()
{
    if (s_program_tl)  { glDeleteProgram(s_program_tl);  s_program_tl = 0; }
    if (s_program_lit) { glDeleteProgram(s_program_lit); s_program_lit = 0; }
    if (s_vao_tl)  { glDeleteVertexArrays(1, &s_vao_tl);  s_vao_tl = 0; }
    if (s_vao_lit) { glDeleteVertexArrays(1, &s_vao_lit); s_vao_lit = 0; }
    if (s_vbo) { glDeleteBuffers(1, &s_vbo); s_vbo = 0; }
    if (s_ebo) { glDeleteBuffers(1, &s_ebo); s_ebo = 0; }
    s_shaders_ready = false;
}

// Upload current fragment shader uniforms to the active program.
static void set_frag_uniforms(
    GLint u_has_texture, GLint u_texture, GLint u_texture_blend,
    GLint u_alpha_test_enabled, GLint u_alpha_ref, GLint u_alpha_func,
    GLint u_fog_enabled, GLint u_fog_color,
    GLint u_specular_enabled, GLint u_color_key_enabled)
{
    bool has_tex = (s_bound_texture != GE_TEXTURE_NONE);
    glUniform1i(u_has_texture, has_tex ? 1 : 0);

    if (has_tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_bound_texture);
        glUniform1i(u_texture, 0);
    }

    glUniform1i(u_texture_blend, static_cast<int>(s_texture_blend));

    glUniform1i(u_alpha_test_enabled, s_alpha_test_enabled ? 1 : 0);
    glUniform1f(u_alpha_ref, s_alpha_ref / 255.0f);
    glUniform1i(u_alpha_func, static_cast<int>(s_alpha_func));

    glUniform1i(u_fog_enabled, s_fog_enabled ? 1 : 0);
    if (s_fog_enabled) {
        float fr = ((s_fog_color >> 16) & 0xFF) / 255.0f;
        float fg = ((s_fog_color >> 8)  & 0xFF) / 255.0f;
        float fb = ((s_fog_color >> 0)  & 0xFF) / 255.0f;
        glUniform3f(u_fog_color, fr, fg, fb);
    }

    glUniform1i(u_specular_enabled, s_specular_enabled ? 1 : 0);
    glUniform1i(u_color_key_enabled, s_color_key_enabled ? 1 : 0);
}

// ---------------------------------------------------------------------------
// Callback registration
// ---------------------------------------------------------------------------

void ge_set_pre_flip_callback(GEPreFlipCallback callback)              { s_pre_flip_callback = callback; }
void ge_set_mode_change_callback(GEModeChangeCallback callback)        { s_mode_change_callback = callback; }
void ge_set_render_states_reset_callback(GERenderStatesResetCallback callback) { s_render_states_reset_callback = callback; }
void ge_set_tga_load_callback(GETGALoadCallback callback)              { s_tga_load_callback = callback; }
void ge_set_texture_reload_prepare_callback(GETextureReloadPrepareCallback callback) { s_texture_reload_prepare_callback = callback; }
void ge_set_data_dir(const char* dir) { strncpy(s_data_dir, dir, sizeof(s_data_dir) - 1); s_data_dir[sizeof(s_data_dir) - 1] = '\0'; }

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ge_init()
{
    // Context is created by OpenDisplay(), ge_init() is called after.
    // Shaders are initialized lazily on first draw call.
}

void ge_shutdown()
{
    destroy_shaders();
    gl_context_destroy();
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void ge_begin_scene()
{
    // OpenGL doesn't have explicit begin/end scene — no-op.
}

void ge_end_scene()
{
    // No-op.
}

void ge_clear(bool color, bool depth)
{
    GLbitfield mask = 0;
    if (color) mask |= GL_COLOR_BUFFER_BIT;
    if (depth) mask |= GL_DEPTH_BUFFER_BIT;
    if (mask) {
        if (color) glClearColor(s_clear_r, s_clear_g, s_clear_b, 1.0f);
        if (depth) glDepthMask(GL_TRUE); // ensure depth writes for clear
        glClear(mask);
    }
}

void ge_flip()
{
    if (s_pre_flip_callback) s_pre_flip_callback();
    gl_context_swap();
}

// ---------------------------------------------------------------------------
// Render state
// ---------------------------------------------------------------------------

static GLenum gl_blend_factor(GEBlendFactor f)
{
    switch (f) {
    case GEBlendFactor::Zero:        return GL_ZERO;
    case GEBlendFactor::One:         return GL_ONE;
    case GEBlendFactor::SrcAlpha:    return GL_SRC_ALPHA;
    case GEBlendFactor::InvSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
    case GEBlendFactor::SrcColor:    return GL_SRC_COLOR;
    case GEBlendFactor::InvSrcColor: return GL_ONE_MINUS_SRC_COLOR;
    case GEBlendFactor::DstColor:    return GL_DST_COLOR;
    case GEBlendFactor::InvDstColor: return GL_ONE_MINUS_DST_COLOR;
    case GEBlendFactor::DstAlpha:    return GL_DST_ALPHA;
    case GEBlendFactor::InvDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
    default:                         return GL_ONE;
    }
}

void ge_set_blend_mode(GEBlendMode mode)
{
    switch (mode) {
    case GEBlendMode::Opaque:
        glDisable(GL_BLEND);
        break;
    case GEBlendMode::Alpha:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case GEBlendMode::Additive:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        break;
    case GEBlendMode::Modulate:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        break;
    case GEBlendMode::InvModulate:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
        break;
    }
}

void ge_set_blend_factors(GEBlendFactor src, GEBlendFactor dst)
{
    glBlendFunc(gl_blend_factor(src), gl_blend_factor(dst));
    glEnable(GL_BLEND);
}

void ge_set_blend_enabled(bool enabled)
{
    if (enabled) glEnable(GL_BLEND);
    else         glDisable(GL_BLEND);
}

void ge_set_depth_mode(GEDepthMode mode)
{
    switch (mode) {
    case GEDepthMode::Off:
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        break;
    case GEDepthMode::ReadOnly:
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        break;
    case GEDepthMode::WriteOnly:
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        break;
    case GEDepthMode::ReadWrite:
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        break;
    }
}

static GLenum gl_compare_func(GECompareFunc func)
{
    switch (func) {
    case GECompareFunc::Always:       return GL_ALWAYS;
    case GECompareFunc::Less:         return GL_LESS;
    case GECompareFunc::LessEqual:    return GL_LEQUAL;
    case GECompareFunc::Greater:      return GL_GREATER;
    case GECompareFunc::GreaterEqual: return GL_GEQUAL;
    case GECompareFunc::NotEqual:     return GL_NOTEQUAL;
    default:                          return GL_LEQUAL;
    }
}

void ge_set_depth_func(GECompareFunc func)
{
    glDepthFunc(gl_compare_func(func));
}

void ge_set_cull_mode(GECullMode mode)
{
    switch (mode) {
    case GECullMode::None:
        glDisable(GL_CULL_FACE);
        break;
    case GECullMode::CW:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        break;
    case GECullMode::CCW:
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);
        break;
    }
}

void ge_set_texture_filter(GETextureFilter, GETextureFilter)
{
    // Applied per-texture via glTexParameteri — stored for later use.
    // TODO Phase 3: apply to currently bound texture.
}

void ge_set_texture_blend(GETextureBlend mode)
{
    s_texture_blend = mode;
}

void ge_set_texture_address(GETextureAddress mode)
{
    // TODO Phase 3: apply to currently bound texture via glTexParameteri.
    (void)mode;
}

void ge_set_depth_bias(int32_t bias)
{
    if (bias != 0) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        // D3D6 ZBIAS range is 0-16, map to reasonable GL polygon offset.
        glPolygonOffset(0.0f, (float)-bias);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

void ge_set_fog_enabled(bool enabled)
{
    s_fog_enabled = enabled;
}

void ge_set_color_key_enabled(bool enabled)
{
    s_color_key_enabled = enabled;
}

void ge_set_alpha_test_enabled(bool enabled)
{
    s_alpha_test_enabled = enabled;
}

void ge_set_alpha_ref(uint32_t ref)
{
    s_alpha_ref = ref;
}

void ge_set_alpha_func(GECompareFunc func)
{
    s_alpha_func = func;
}

void ge_set_fog_params(uint32_t color, float near_dist, float far_dist)
{
    s_fog_color = color;
    s_fog_near = near_dist;
    s_fog_far = far_dist;
}

void ge_set_specular_enabled(bool enabled)
{
    s_specular_enabled = enabled;
}

void ge_set_perspective_correction(bool)
{
    // Always perspective-correct on modern GPUs — no-op.
}

// ---------------------------------------------------------------------------
// Textures — TODO Phase 3
// ---------------------------------------------------------------------------

void ge_bind_texture(GETextureHandle tex)
{
    s_bound_texture = tex;
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void ge_draw_primitive(GEPrimitiveType type, const GEVertexTL* verts, uint32_t count)
{
    if (!count) return;
    if (!init_shaders()) return;

    glUseProgram(s_program_tl);

    // Upload viewport uniform.
    glUniform4f(s_tl_u_viewport,
        (float)s_vp_x, (float)s_vp_y, (float)s_vp_w, (float)s_vp_h);

    // Upload fragment uniforms.
    set_frag_uniforms(
        s_tl_u_has_texture, s_tl_u_texture, s_tl_u_texture_blend,
        s_tl_u_alpha_test_enabled, s_tl_u_alpha_ref, s_tl_u_alpha_func,
        s_tl_u_fog_enabled, s_tl_u_fog_color,
        s_tl_u_specular_enabled, s_tl_u_color_key_enabled);

    // Upload vertex data.
    glBindVertexArray(s_vao_tl);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(GEVertexTL), verts, GL_STREAM_DRAW);

    GLenum gl_mode = (type == GEPrimitiveType::TriangleFan) ? GL_TRIANGLE_FAN : GL_TRIANGLES;
    glDrawArrays(gl_mode, 0, count);

    glBindVertexArray(0);
    glUseProgram(0);
}

void ge_draw_indexed_primitive(GEPrimitiveType type, const GEVertexTL* verts, uint32_t vert_count,
                               const uint16_t* indices, uint32_t index_count)
{
    if (!index_count || !vert_count) return;
    if (!init_shaders()) return;

    glUseProgram(s_program_tl);

    // Upload viewport uniform.
    glUniform4f(s_tl_u_viewport,
        (float)s_vp_x, (float)s_vp_y, (float)s_vp_w, (float)s_vp_h);

    // Upload fragment uniforms.
    set_frag_uniforms(
        s_tl_u_has_texture, s_tl_u_texture, s_tl_u_texture_blend,
        s_tl_u_alpha_test_enabled, s_tl_u_alpha_ref, s_tl_u_alpha_func,
        s_tl_u_fog_enabled, s_tl_u_fog_color,
        s_tl_u_specular_enabled, s_tl_u_color_key_enabled);

    // Upload vertex and index data.
    glBindVertexArray(s_vao_tl);

    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_count * sizeof(GEVertexTL), verts, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint16_t), indices, GL_STREAM_DRAW);

    GLenum gl_mode = (type == GEPrimitiveType::TriangleFan) ? GL_TRIANGLE_FAN : GL_TRIANGLES;
    glDrawElements(gl_mode, index_count, GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);
    glUseProgram(0);
}

// Transpose a row-major GEMatrix to column-major for OpenGL.
static void transpose_matrix(const GEMatrix* src, float dst[16])
{
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            dst[c * 4 + r] = src->m[r][c];
}

void ge_draw_indexed_primitive_lit(GEPrimitiveType type, const GEVertexLit* verts, uint32_t vert_count,
                                   const uint16_t* indices, uint32_t index_count)
{
    if (!index_count || !vert_count) return;
    if (!init_shaders()) return;

    glUseProgram(s_program_lit);

    // Upload transform matrices (transposed: row-major → column-major).
    float mat[16];
    transpose_matrix(&s_world_matrix, mat);
    glUniformMatrix4fv(s_lit_u_world, 1, GL_FALSE, mat);

    transpose_matrix(&s_view_matrix, mat);
    glUniformMatrix4fv(s_lit_u_view, 1, GL_FALSE, mat);

    transpose_matrix(&s_projection_matrix, mat);
    glUniformMatrix4fv(s_lit_u_projection, 1, GL_FALSE, mat);

    // Upload fragment uniforms.
    set_frag_uniforms(
        s_lit_u_has_texture, s_lit_u_texture, s_lit_u_texture_blend,
        s_lit_u_alpha_test_enabled, s_lit_u_alpha_ref, s_lit_u_alpha_func,
        s_lit_u_fog_enabled, s_lit_u_fog_color,
        s_lit_u_specular_enabled, s_lit_u_color_key_enabled);

    // Upload vertex and index data.
    glBindVertexArray(s_vao_lit);

    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_count * sizeof(GEVertexLit), verts, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint16_t), indices, GL_STREAM_DRAW);

    GLenum gl_mode = (type == GEPrimitiveType::TriangleFan) ? GL_TRIANGLE_FAN : GL_TRIANGLES;
    glDrawElements(gl_mode, index_count, GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);
    glUseProgram(0);
}

void ge_draw_indexed_primitive_unlit(GEPrimitiveType type, const GEVertex* verts, uint32_t vert_count,
                                     const uint16_t* indices, uint32_t index_count)
{
    // Never called in the codebase (0 call sites). Stub for now.
    (void)type; (void)verts; (void)vert_count; (void)indices; (void)index_count;
}

// ---------------------------------------------------------------------------
// Transforms
// ---------------------------------------------------------------------------

void ge_set_transform(GETransform type, const GEMatrix* matrix)
{
    switch (type) {
    case GETransform::World:      memcpy(&s_world_matrix, matrix, sizeof(GEMatrix)); break;
    case GETransform::View:       memcpy(&s_view_matrix, matrix, sizeof(GEMatrix)); break;
    case GETransform::Projection: memcpy(&s_projection_matrix, matrix, sizeof(GEMatrix)); break;
    }
}

// ---------------------------------------------------------------------------
// Viewport
// ---------------------------------------------------------------------------

void ge_set_viewport(int32_t x, int32_t y, int32_t w, int32_t h)
{
    s_vp_x = x; s_vp_y = y; s_vp_w = w; s_vp_h = h;
    // OpenGL viewport Y is bottom-up, D3D is top-down.
    int32_t screen_h = gl_context_get_height();
    glViewport(x, screen_h - y - h, w, h);
    glScissor(x, screen_h - y - h, w, h);
    glEnable(GL_SCISSOR_TEST);
}

void ge_set_viewport_3d(int32_t x, int32_t y, int32_t w, int32_t h,
                        float, float, float, float)
{
    // Clip parameters are D3D6-specific (D3DVIEWPORT2 clipping volume).
    // In OpenGL, clipping is done by the projection matrix.
    ge_set_viewport(x, y, w, h);
}

// ---------------------------------------------------------------------------
// Background
// ---------------------------------------------------------------------------

void ge_set_background(GEBackground bg)
{
    switch (bg) {
    case GEBackground::Black: s_clear_r = s_clear_g = s_clear_b = 0.0f; break;
    case GEBackground::White: s_clear_r = s_clear_g = s_clear_b = 1.0f; break;
    case GEBackground::User:  break; // uses current s_clear_r/g/b
    }
}

void ge_set_background_color(uint8_t r, uint8_t g, uint8_t b)
{
    s_clear_r = r / 255.0f;
    s_clear_g = g / 255.0f;
    s_clear_b = b / 255.0f;
}

// ---------------------------------------------------------------------------
// Screen buffer access — TODO Phase 6
// ---------------------------------------------------------------------------

void* ge_lock_screen() { return nullptr; }
void  ge_unlock_screen() {}
uint8_t* ge_get_screen_buffer() { return nullptr; }
int32_t  ge_get_screen_pitch() { return 0; }
int32_t  ge_get_screen_width() { return gl_context_get_width(); }
int32_t  ge_get_screen_height() { return gl_context_get_height(); }
int32_t  ge_get_screen_bpp() { return 32; }
void ge_plot_pixel(int32_t, int32_t, uint8_t, uint8_t, uint8_t) {}
void ge_plot_formatted_pixel(int32_t, int32_t, uint32_t) {}
void ge_get_pixel(int32_t, int32_t, uint8_t*, uint8_t*, uint8_t*) {}
uint32_t ge_get_formatted_pixel(uint8_t, uint8_t, uint8_t) { return 0; }
void ge_blit_back_buffer() {}

// ---------------------------------------------------------------------------
// Device capabilities
// ---------------------------------------------------------------------------

bool ge_supports_dest_inv_src_color() { return true; }
bool ge_supports_modulate_alpha()     { return true; }
bool ge_supports_adami_lighting()     { return true; }
bool ge_is_hardware()                 { return true; }
bool ge_is_fullscreen()               { return s_fullscreen; }

// ---------------------------------------------------------------------------
// Background surface — TODO Phase 6
// ---------------------------------------------------------------------------

void ge_create_background_surface(uint8_t*) {}
void ge_blit_background_surface() {}
void ge_destroy_background_surface() {}

void ge_init_back_image(const char*) {}
void ge_show_back_image(bool) {}
void ge_reset_back_image() {}

// ---------------------------------------------------------------------------
// Cutscene
// ---------------------------------------------------------------------------

void ge_run_cutscene(int32_t) {}

// ---------------------------------------------------------------------------
// Driver info
// ---------------------------------------------------------------------------

bool ge_is_primary_driver() { return true; }

// ---------------------------------------------------------------------------
// Display mode management
// ---------------------------------------------------------------------------

void ge_to_gdi()
{
    // No exclusive display mode in OpenGL windowed — no-op.
}

void ge_from_gdi()
{
    // No-op.
}

void ge_restore_all_surfaces()
{
    // No device-lost in OpenGL — no-op.
}

bool ge_is_display_changed() { return s_display_changed; }
void ge_clear_display_changed() { s_display_changed = false; }

void ge_update_display_rect(void*, bool) {}

// ---------------------------------------------------------------------------
// Texture management — TODO Phase 3
// ---------------------------------------------------------------------------

void ge_remove_all_loaded_textures() {}

// ---------------------------------------------------------------------------
// Surface blitting — TODO Phase 6
// ---------------------------------------------------------------------------

void ge_capture_backbuffer_to_texture(int32_t, int32_t, int32_t) {}

// ---------------------------------------------------------------------------
// Screen surfaces — TODO Phase 6
// ---------------------------------------------------------------------------

GEScreenSurface ge_create_screen_surface(uint8_t*) { return GE_SCREEN_SURFACE_NONE; }
GEScreenSurface ge_load_screen_surface(const char*) { return GE_SCREEN_SURFACE_NONE; }
void ge_destroy_screen_surface(GEScreenSurface) {}
void ge_restore_screen_surface(GEScreenSurface) {}
void ge_set_background_override(GEScreenSurface surface) { s_background_override = surface; }
GEScreenSurface ge_get_background_override() { return s_background_override; }
void ge_blit_surface_to_backbuffer(GEScreenSurface, int32_t, int32_t, int32_t, int32_t) {}

// ---------------------------------------------------------------------------
// Vertex buffer pool — CPU-side implementation for OpenGL
// ---------------------------------------------------------------------------
// D3D backend uses IDirect3DVertexBuffer objects in a pool.
// OpenGL backend: simple CPU malloc buffers. polypage.cpp writes vertices
// via pointer, then calls prepare + draw. We upload to VBO at draw time.

struct GLVertexBuf {
    uint8_t* data;       // CPU vertex data (GEVertexTL layout, 32 bytes per vertex)
    uint32_t capacity;   // in bytes
    uint32_t logsize;    // log2 of vertex count
    bool     in_use;     // allocated and not yet released
};

static constexpr int GL_VB_POOL_MAX = 64;
static GLVertexBuf s_vb_pool[GL_VB_POOL_MAX];
static bool s_vb_pool_init = false;

static void vb_pool_ensure_init()
{
    if (!s_vb_pool_init) {
        memset(s_vb_pool, 0, sizeof(s_vb_pool));
        s_vb_pool_init = true;
    }
}

void ge_reclaim_vertex_buffers()
{
    // Mark all buffers as available (end of frame).
    for (int i = 0; i < GL_VB_POOL_MAX; i++) {
        s_vb_pool[i].in_use = false;
    }
}

void ge_dump_vpool_info(void*) {}

void* ge_vb_alloc(uint32_t logsize, void** out_ptr, uint32_t* out_logsize)
{
    vb_pool_ensure_init();

    // Find a free slot (prefer one with matching or larger capacity).
    GLVertexBuf* best = nullptr;
    for (int i = 0; i < GL_VB_POOL_MAX; i++) {
        if (!s_vb_pool[i].in_use) {
            if (!best || (s_vb_pool[i].data && s_vb_pool[i].logsize >= logsize)) {
                best = &s_vb_pool[i];
                if (best->data && best->logsize >= logsize) break;
            }
        }
    }

    if (!best) return nullptr;

    best->in_use = true;
    best->logsize = logsize;

    uint32_t vertex_count = 1u << logsize;
    uint32_t needed = vertex_count * 32; // sizeof(GEVertexTL) = 32

    if (!best->data || best->capacity < needed) {
        free(best->data);
        best->data = (uint8_t*)malloc(needed);
        best->capacity = needed;
    }

    if (out_ptr) *out_ptr = best->data;
    if (out_logsize) *out_logsize = logsize;
    return best;
}

void* ge_vb_expand(void* vb, void** out_ptr, uint32_t* out_logsize)
{
    if (!vb) return nullptr;
    GLVertexBuf* buf = (GLVertexBuf*)vb;

    uint32_t new_logsize = buf->logsize + 1;
    uint32_t new_count = 1u << new_logsize;
    uint32_t needed = new_count * 32;

    uint8_t* new_data = (uint8_t*)realloc(buf->data, needed);
    if (!new_data) return vb; // keep old on failure

    buf->data = new_data;
    buf->capacity = needed;
    buf->logsize = new_logsize;

    if (out_ptr) *out_ptr = buf->data;
    if (out_logsize) *out_logsize = new_logsize;
    return buf;
}

void ge_vb_release(void* vb)
{
    if (!vb) return;
    GLVertexBuf* buf = (GLVertexBuf*)vb;
    buf->in_use = false;
}

void* ge_vb_get_ptr(void* vb)
{
    if (!vb) return nullptr;
    return ((GLVertexBuf*)vb)->data;
}

void* ge_vb_prepare(void* vb)
{
    // In D3D, this unlocks the VB and moves it to "busy list" for GPU use.
    // In OpenGL, the data is CPU memory — just return the handle.
    // Buffer stays in_use until ge_reclaim_vertex_buffers or ge_draw_indexed_primitive_vb.
    return vb;
}

void ge_draw_indexed_primitive_vb(void* prepared_vb, const uint16_t* indices, uint32_t index_count)
{
    if (!prepared_vb || !indices || !index_count) return;
    if (!init_shaders()) return;

    GLVertexBuf* buf = (GLVertexBuf*)prepared_vb;
    uint32_t vert_count = 1u << buf->logsize;

    // VB data is GEVertexTL layout (screen-space, pre-transformed).
    glUseProgram(s_program_tl);

    glUniform4f(s_tl_u_viewport,
        (float)s_vp_x, (float)s_vp_y, (float)s_vp_w, (float)s_vp_h);

    set_frag_uniforms(
        s_tl_u_has_texture, s_tl_u_texture, s_tl_u_texture_blend,
        s_tl_u_alpha_test_enabled, s_tl_u_alpha_ref, s_tl_u_alpha_func,
        s_tl_u_fog_enabled, s_tl_u_fog_color,
        s_tl_u_specular_enabled, s_tl_u_color_key_enabled);

    glBindVertexArray(s_vao_tl);

    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_count * 32, buf->data, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint16_t), indices, GL_STREAM_DRAW);

    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);
    glUseProgram(0);

    // Data uploaded to GPU — release the CPU buffer back to the pool.
    buf->in_use = false;
}

// ---------------------------------------------------------------------------
// Texture pixel access — TODO Phase 3
// ---------------------------------------------------------------------------

bool ge_lock_texture_pixels(int32_t, uint16_t**, int32_t*) { return false; }
void ge_unlock_texture_pixels(int32_t) {}
void ge_get_texture_pixel_format(int32_t, int32_t*, int32_t*, int32_t*, int32_t*, int32_t*, int32_t*, int32_t*, int32_t*) {}
bool ge_is_texture_loaded(int32_t) { return false; }

void ge_debug_paint_block(uint32_t) {}

// ---------------------------------------------------------------------------
// Gamma
// ---------------------------------------------------------------------------

static int32_t s_gamma_black = 0, s_gamma_white = 255;

void ge_set_gamma(int32_t black, int32_t white) { s_gamma_black = black; s_gamma_white = white; }
void ge_get_gamma(int32_t* black, int32_t* white) { *black = s_gamma_black; *white = s_gamma_white; }
bool ge_is_gamma_available() { return false; } // TODO: SDL gamma or shader post-process

// ---------------------------------------------------------------------------
// Texture loading lifecycle — TODO Phase 3
// ---------------------------------------------------------------------------

void ge_texture_loading_done() {}
void ge_texture_loading_begin() {}
void ge_texture_load_tga(int32_t, const char*, bool) {}
void ge_texture_create_user_page(int32_t, int32_t, bool) {}
void ge_texture_destroy(int32_t) {}
void ge_texture_free_all() {}
void ge_texture_change_tga(int32_t, const char*) {}
void ge_texture_font_on(int32_t) {}
void ge_texture_font2_on(int32_t) {}
void ge_texture_set_greyscale(int32_t, bool) {}
void ge_get_texture_offset(int32_t, float*, float*, float*, float*) {}
int32_t ge_texture_get_size(int32_t) { return 0; }
int32_t ge_texture_get_type(int32_t) { return 0; }
void ge_texture_set_type(int32_t, int32_t) {}
GETextureHandle ge_get_texture_handle(int32_t) { return 0; }
Font* ge_get_font(int32_t, int32_t) { return nullptr; }

// ---------------------------------------------------------------------------
// Text surface — TODO Phase 8
// ---------------------------------------------------------------------------

GETextSurface ge_text_surface_create(int32_t, int32_t) { return GE_TEXT_SURFACE_NONE; }
void ge_text_surface_destroy(GETextSurface) {}
bool ge_text_surface_get_dc(GETextSurface, void**) { return false; }
void ge_text_surface_release_dc(GETextSurface, void*) {}
bool ge_text_surface_lock(GETextSurface, uint8_t**, int32_t*) { return false; }
void ge_text_surface_unlock(GETextSurface) {}

// ---------------------------------------------------------------------------
// NTSC / Driver enumeration
// ---------------------------------------------------------------------------

bool ge_is_ntsc() { return false; }
void ge_enumerate_drivers(GEDriverEnumCallback, void*) {}

// ===========================================================================
// GERenderState — same as game_graphics_engine.cpp (shared)
// ===========================================================================
// Shared implementation is in game_graphics_engine.cpp, compiled separately.

// ===========================================================================
// Display globals (expected by game code)
// ===========================================================================

SLONG RealDisplayWidth = 640;
SLONG RealDisplayHeight = 480;
SLONG DisplayBPP = 32;
volatile HWND hDDLibWindow = NULL;
UBYTE* image_mem = nullptr;

// ---------------------------------------------------------------------------
// Display functions (called from game.cpp)
// ---------------------------------------------------------------------------

SLONG OpenDisplay(ULONG width, ULONG height, ULONG depth, ULONG flags)
{
    // Read video resolution from .env (same logic as D3D backend).
    SLONG VideoRes = ENV_get_value_number((CBYTE*)"video_res", -1, (CBYTE*)"Render");
    if (VideoRes < 0) VideoRes = 2; // default 640x480
    if (VideoRes > 4) VideoRes = 4;

    depth = 32; // OpenGL always 32bpp
    switch (VideoRes) {
    case 0: width = 320;  height = 240; break;
    case 1: width = 512;  height = 384; break;
    case 2: width = 640;  height = 480; break;
    case 3: width = 800;  height = 600; break;
    case 4: width = 1024; height = 768; break;
    }

    RealDisplayWidth = width;
    RealDisplayHeight = height;
    DisplayBPP = depth;

    if (!gl_context_create(width, height)) {
        return -1;
    }

    if (s_mode_change_callback) {
        s_mode_change_callback(width, height);
    }

    return 0; // success (D3D backend returns HRESULT, 0 = S_OK)
}

SLONG SetDisplay(ULONG width, ULONG height, ULONG depth)
{
    // TODO: resize GL context.
    RealDisplayWidth = width;
    RealDisplayHeight = height;
    DisplayBPP = depth;
    return 0;
}

SLONG CloseDisplay()
{
    gl_context_destroy();
    return 1;
}

SLONG ClearDisplay(UBYTE r, UBYTE g, UBYTE b)
{
    glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return 0;
}

void SetLastClumpfile(char*, unsigned int) {}

void* LockWorkScreen() { return nullptr; }
void UnlockWorkScreen() {}
void ShowWorkScreen(ULONG) {}
void ClearWorkScreen(UBYTE) {}
