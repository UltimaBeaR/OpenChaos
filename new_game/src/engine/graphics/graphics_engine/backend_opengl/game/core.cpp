// Graphics engine — OpenGL 4.1 core profile implementation.
// Phase 1: lifecycle, clear, flip, render state, viewport.
// Phase 2: shaders + drawing (ge_draw_indexed_primitive, ge_draw_indexed_primitive_lit).
// Phase 3: texture loading, binding, font extraction, user pages.

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
static bool s_bound_texture_has_alpha = false;

// Texture filter and address mode (applied at draw time when binding texture).
static GETextureFilter s_tex_filter_mag = GETextureFilter::Linear;
static GETextureFilter s_tex_filter_min = GETextureFilter::Linear;
static GETextureAddress s_tex_address = GETextureAddress::Wrap;

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

// Dummy screen buffer for Phase 6 stubs (prevents null deref in wibble.cpp etc.)
static uint8_t* s_dummy_screen = nullptr;

// ===========================================================================
// Phase 3: Texture management
// ===========================================================================

// Max texture pages — matches D3D backend (22*64 standard + 160 user pages).
static constexpr int32_t GL_TEX_NUM_STANDARD = 22 * 64;
static constexpr int32_t GL_TEX_MAX = GL_TEX_NUM_STANDARD + 160;

// Pixel type for TGA data (BGRA, 4 bytes). Matches D3D backend's BGRAPixel.
struct GLBGRAPixel {
    uint8_t blue, green, red, alpha;
};

// Texture load info passed to font extraction.
struct GLTexLoadInfo {
    int32_t width, height;
    int32_t contains_alpha;
};

// Page type constants (subtexture packing layout).
static constexpr int32_t GL_PAGE_NONE    = 0;
static constexpr int32_t GL_PAGE_64_3X3  = 1;
static constexpr int32_t GL_PAGE_64_4X4  = 2;
static constexpr int32_t GL_PAGE_32_3X3  = 3;
static constexpr int32_t GL_PAGE_32_4X4  = 4;

// Font texture flags.
static constexpr int32_t GL_TEX_FLAG_FONT  = (1 << 0);
static constexpr int32_t GL_TEX_FLAG_FONT2 = (1 << 1);

struct GLTexture {
    GLuint   gl_id;           // OpenGL texture object (0 = none)
    int32_t  type;            // GE_TEXTURE_TYPE_*
    int32_t  size;            // Width (and height) in pixels (square, power of two)
    int32_t  flags;           // GL_TEX_FLAG_FONT | GL_TEX_FLAG_FONT2
    bool     contains_alpha;
    bool     greyscale;
    uint8_t  page_pos;        // Subtexture slot index within a page
    uint8_t  page_type;       // GL_PAGE_*
    Font*    font_list;       // Linked list of extracted fonts (null if not a font texture)
    char     name[256];       // TGA filename
    uint32_t file_id;         // Clump file ID
    bool     can_shrink;

    // User page pixel access (CPU staging buffer for truetype writes).
    uint8_t* staging;         // BGRA staging buffer (size * size * 4 bytes)
    int32_t  staging_pitch;   // Row pitch in bytes
    bool     staging_dirty;   // True if staging was written to and needs upload
};

static GLTexture s_textures[GL_TEX_MAX];

// Forward declarations for font extraction.
static bool gl_scan_for_baseline(GLBGRAPixel** line_ptr, GLBGRAPixel* underline,
                                  GLTexLoadInfo* info, int32_t* y_ptr);
static bool gl_create_fonts(Font** font_list, GLTexLoadInfo* info, GLBGRAPixel* data);

// Process font outline pixels: magenta (0xFF,0x00,0xFF) → black.
static void gl_process_font_outlines(GLBGRAPixel* pixels, int32_t count)
{
    for (int32_t i = 0; i < count; i++) {
        if (pixels[i].red == 0xFF && pixels[i].green == 0 && pixels[i].blue == 0xFF) {
            pixels[i].red = 0;
            pixels[i].green = 0;
            pixels[i].blue = 0;
        }
    }
}

// Process Font2 red-border pixels: red-only → transparent black.
static void gl_process_font2_red_borders(GLBGRAPixel* pixels, int32_t count)
{
    for (int32_t i = 0; i < count; i++) {
        if (pixels[i].green == 0 && pixels[i].blue == 0 && pixels[i].red > 128) {
            pixels[i].red = 0;
            pixels[i].alpha = 0;
        }
    }
}

// Apply greyscale conversion: bright = (r+g+b) * 85 >> 8.
static void gl_apply_greyscale(GLBGRAPixel* pixels, int32_t count)
{
    for (int32_t i = 0; i < count; i++) {
        int32_t bright = ((int32_t)pixels[i].red + pixels[i].green + pixels[i].blue) * 85 >> 8;
        pixels[i].red = (uint8_t)bright;
        pixels[i].green = (uint8_t)bright;
        pixels[i].blue = (uint8_t)bright;
    }
}

// Edge color bleeding: for transparent pixels with alpha content, copy RGB from nearest
// non-transparent neighbor to prevent filtering artifacts at alpha edges.
static void gl_bleed_edges(GLBGRAPixel* pixels, int32_t w, int32_t h)
{
    for (int32_t j = 0; j < h; j++) {
        for (int32_t i = 0; i < w; i++) {
            GLBGRAPixel& p = pixels[i + j * w];
            // Only process fully transparent pixels (RGB all zero).
            if (p.red || p.green || p.blue) continue;
            if (p.alpha) continue;

            // Find nearest non-transparent neighbor.
            int32_t i2 = i, j2 = j;
            #define GL_HAS_COLOR(x, y) (pixels[(x) + (y) * w].red | pixels[(x) + (y) * w].green | pixels[(x) + (y) * w].blue)
            if      (i - 1 >= 0 && GL_HAS_COLOR(i-1, j))                         { i2 = i-1; j2 = j; }
            else if (i + 1 < w  && GL_HAS_COLOR(i+1, j))                         { i2 = i+1; j2 = j; }
            else if (j - 1 >= 0 && GL_HAS_COLOR(i, j-1))                         { i2 = i;   j2 = j-1; }
            else if (j + 1 < h  && GL_HAS_COLOR(i, j+1))                         { i2 = i;   j2 = j+1; }
            else if (i - 1 >= 0 && j - 1 >= 0 && GL_HAS_COLOR(i-1, j-1))        { i2 = i-1; j2 = j-1; }
            else if (i - 1 >= 0 && j + 1 < h  && GL_HAS_COLOR(i-1, j+1))        { i2 = i-1; j2 = j+1; }
            else if (i + 1 < w  && j - 1 >= 0 && GL_HAS_COLOR(i+1, j-1))        { i2 = i+1; j2 = j-1; }
            else if (i + 1 < w  && j + 1 < h  && GL_HAS_COLOR(i+1, j+1))        { i2 = i+1; j2 = j+1; }
            else continue;
            #undef GL_HAS_COLOR

            p.red   = pixels[i2 + j2 * w].red;
            p.green = pixels[i2 + j2 * w].green;
            p.blue  = pixels[i2 + j2 * w].blue;
            // Alpha stays 0 — only RGB is bled.
        }
    }
}

// Debug counter for texture uploads.
static int32_t s_tex_upload_count = 0;

// Upload BGRA pixel data to an OpenGL texture. Creates the GL texture if needed.
static void gl_upload_texture(GLTexture& tex, GLBGRAPixel* pixels, int32_t w, int32_t h)
{
    if (!tex.gl_id) {
        glGenTextures(1, &tex.gl_id);
    }
    glBindTexture(GL_TEXTURE_2D, tex.gl_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);

    s_tex_upload_count++;

    // Default filtering: bilinear (matches D3D's default LINEAR/LINEAR).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Default address: wrap.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Destroy a single texture page.
static void gl_destroy_texture(GLTexture& tex)
{
    // Free font list.
    Font* f = tex.font_list;
    while (f) {
        Font* next = f->NextFont;
        MemFree(f);
        f = next;
    }

    // Delete GL object.
    if (tex.gl_id) {
        glDeleteTextures(1, &tex.gl_id);
    }

    // Free staging buffer.
    if (tex.staging) {
        free(tex.staging);
    }

    // Reset to defaults.
    memset(&tex, 0, sizeof(GLTexture));
}

// Load (or reload) a TGA texture into a page slot.
// Matches D3DTexture::Reload() behavior: resets render states after each load
// so POLY_init_render_states() will re-cache texture handles.
static bool gl_load_tga(GLTexture& tex)
{
    // Reset render states — matches D3D's Reload() which calls this on every texture load.
    if (s_render_states_reset_callback) s_render_states_reset_callback();

    if (!s_tga_load_callback) return false;

    uint8_t* raw_pixels = nullptr;
    int32_t load_w = 0, load_h = 0;
    bool load_alpha = false;

    bool loaded = s_tga_load_callback(tex.name, tex.file_id, tex.can_shrink,
                                       &raw_pixels, &load_w, &load_h, &load_alpha);
    if (!loaded || !raw_pixels) return false;

    // Validate: must be square, power of two.
    if (load_w != load_h || (load_w & (load_w - 1)) != 0) {
        MemFree(raw_pixels);
        return false;
    }

    GLBGRAPixel* pixels = (GLBGRAPixel*)raw_pixels;
    int32_t count = load_w * load_h;

    tex.size = load_w;
    tex.contains_alpha = load_alpha;

    // Free old font list before potential re-extraction.
    if (tex.flags & GL_TEX_FLAG_FONT) {
        Font* f = tex.font_list;
        while (f) { Font* next = f->NextFont; MemFree(f); f = next; }
        tex.font_list = nullptr;
    }

    // Font glyph extraction (before outline recoloring).
    if (tex.flags & GL_TEX_FLAG_FONT) {
        GLTexLoadInfo ti = { load_w, load_h, load_alpha ? 1 : 0 };
        gl_create_fonts(&tex.font_list, &ti, pixels);
        gl_process_font_outlines(pixels, count);
    }

    // Font2: red-border → transparent.
    if (tex.flags & GL_TEX_FLAG_FONT2) {
        gl_process_font2_red_borders(pixels, count);
    }

    // Greyscale conversion.
    if (tex.greyscale) {
        gl_apply_greyscale(pixels, count);
    }

    // Edge color bleeding for alpha textures and font2 (which creates transparent
    // pixels via red-border processing even if the source TGA had no alpha channel).
    if (load_alpha || (tex.flags & GL_TEX_FLAG_FONT2)) {
        gl_bleed_edges(pixels, load_w, load_h);
    }

    // Upload to GPU.
    gl_upload_texture(tex, pixels, load_w, load_h);

    MemFree(raw_pixels);
    return true;
}

// ===========================================================================
// Phase 2: Shader infrastructure
// ===========================================================================

// ---------------------------------------------------------------------------
// GLSL shader sources — embedded from .glsl files at build time by CMake.
// Source files: backend_opengl/shaders/{tl_vert,lit_vert,common_frag}.glsl
// ---------------------------------------------------------------------------

#include "gl_shaders_embedded.h"

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
static GLint s_tl_u_fog_near            = -1;
static GLint s_tl_u_fog_far             = -1;
static GLint s_tl_u_specular_enabled    = -1;
static GLint s_tl_u_color_key_enabled   = -1;
static GLint s_tl_u_tex_has_alpha      = -1;

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
static GLint s_lit_u_fog_near           = -1;
static GLint s_lit_u_fog_far            = -1;
static GLint s_lit_u_specular_enabled   = -1;
static GLint s_lit_u_color_key_enabled  = -1;
static GLint s_lit_u_tex_has_alpha     = -1;

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
    GLint* u_fog_enabled, GLint* u_fog_color, GLint* u_fog_near, GLint* u_fog_far,
    GLint* u_specular_enabled, GLint* u_color_key_enabled, GLint* u_tex_has_alpha)
{
    *u_has_texture        = glGetUniformLocation(prog, "u_has_texture");
    *u_texture            = glGetUniformLocation(prog, "u_texture");
    *u_texture_blend      = glGetUniformLocation(prog, "u_texture_blend");
    *u_alpha_test_enabled = glGetUniformLocation(prog, "u_alpha_test_enabled");
    *u_alpha_ref          = glGetUniformLocation(prog, "u_alpha_ref");
    *u_alpha_func         = glGetUniformLocation(prog, "u_alpha_func");
    *u_fog_enabled        = glGetUniformLocation(prog, "u_fog_enabled");
    *u_fog_color          = glGetUniformLocation(prog, "u_fog_color");
    *u_fog_near           = glGetUniformLocation(prog, "u_fog_near");
    *u_fog_far            = glGetUniformLocation(prog, "u_fog_far");
    *u_specular_enabled   = glGetUniformLocation(prog, "u_specular_enabled");
    *u_color_key_enabled  = glGetUniformLocation(prog, "u_color_key_enabled");
    *u_tex_has_alpha      = glGetUniformLocation(prog, "u_tex_has_alpha");
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
        &s_tl_u_fog_enabled, &s_tl_u_fog_color, &s_tl_u_fog_near, &s_tl_u_fog_far,
        &s_tl_u_specular_enabled, &s_tl_u_color_key_enabled, &s_tl_u_tex_has_alpha);

    // Cache Lit uniform locations.
    s_lit_u_world      = glGetUniformLocation(s_program_lit, "u_world");
    s_lit_u_view       = glGetUniformLocation(s_program_lit, "u_view");
    s_lit_u_projection = glGetUniformLocation(s_program_lit, "u_projection");
    cache_frag_uniforms(s_program_lit,
        &s_lit_u_has_texture, &s_lit_u_texture, &s_lit_u_texture_blend,
        &s_lit_u_alpha_test_enabled, &s_lit_u_alpha_ref, &s_lit_u_alpha_func,
        &s_lit_u_fog_enabled, &s_lit_u_fog_color, &s_lit_u_fog_near, &s_lit_u_fog_far,
        &s_lit_u_specular_enabled, &s_lit_u_color_key_enabled, &s_lit_u_tex_has_alpha);

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
    GLint u_fog_enabled, GLint u_fog_color, GLint u_fog_near, GLint u_fog_far,
    GLint u_specular_enabled, GLint u_color_key_enabled, GLint u_tex_has_alpha)
{
    bool has_tex = (s_bound_texture != GE_TEXTURE_NONE);
    glUniform1i(u_tex_has_alpha, s_bound_texture_has_alpha ? 1 : 0);
    glUniform1i(u_has_texture, has_tex ? 1 : 0);

    if (has_tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_bound_texture);
        glUniform1i(u_texture, 0);

        // Apply current filter state.
        GLint gl_mag = (s_tex_filter_mag == GETextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
        GLint gl_min = (s_tex_filter_min == GETextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_mag);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_min);

        // Apply current address mode.
        GLint gl_wrap = (s_tex_address == GETextureAddress::Clamp)
                        ? GL_CLAMP_TO_EDGE : GL_REPEAT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap);
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
        glUniform1f(u_fog_near, s_fog_near);
        glUniform1f(u_fog_far, s_fog_far);
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
    // Destroy all textures.
    for (int32_t i = 0; i < GL_TEX_MAX; i++) {
        if (s_textures[i].type != GE_TEXTURE_TYPE_UNUSED) {
            gl_destroy_texture(s_textures[i]);
        }
    }

    // Free dummy screen buffer.
    if (s_dummy_screen) { free(s_dummy_screen); s_dummy_screen = nullptr; }

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

void ge_set_texture_filter(GETextureFilter mag, GETextureFilter min)
{
    s_tex_filter_mag = mag;
    s_tex_filter_min = min;
}

void ge_set_texture_blend(GETextureBlend mode)
{
    s_texture_blend = mode;
}

void ge_set_texture_address(GETextureAddress mode)
{
    s_tex_address = mode;
}

void ge_set_depth_bias(int32_t bias)
{
    if (bias != 0) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        // D3D6 ZBIAS range is 0-16, directly offsets normalized Z.
        // GL polygon offset: offset = factor * slope + units * min_resolvable.
        // With 24-bit Z-buffer, min_resolvable is ~1/2^24 ≈ 6e-8, much finer
        // than D3D6's 16-bit Z, so we need large unit values to match.
        glPolygonOffset(-1.0f, (float)(-bias * 512));
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
// Textures
// ---------------------------------------------------------------------------

void ge_bind_texture(GETextureHandle tex)
{
    s_bound_texture = tex;
    s_bound_texture_has_alpha = false;
    if (tex != GE_TEXTURE_NONE) {
        GLuint gl_id = (GLuint)(uintptr_t)tex;
        for (int32_t i = 0; i < GL_TEX_MAX; i++) {
            if (s_textures[i].gl_id == gl_id) {
                s_bound_texture_has_alpha = s_textures[i].contains_alpha;
                break;
            }
        }
    }
}

bool ge_bound_texture_contains_alpha()
{
    return s_bound_texture_has_alpha;
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
        s_tl_u_fog_enabled, s_tl_u_fog_color, s_tl_u_fog_near, s_tl_u_fog_far,
        s_tl_u_specular_enabled, s_tl_u_color_key_enabled, s_tl_u_tex_has_alpha);

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
        s_tl_u_fog_enabled, s_tl_u_fog_color, s_tl_u_fog_near, s_tl_u_fog_far,
        s_tl_u_specular_enabled, s_tl_u_color_key_enabled, s_tl_u_tex_has_alpha);

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
        s_lit_u_fog_enabled, s_lit_u_fog_color, s_lit_u_fog_near, s_lit_u_fog_far,
        s_lit_u_specular_enabled, s_lit_u_color_key_enabled, s_lit_u_tex_has_alpha);

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
// Screen buffer access — dummy buffer to prevent crashes (Phase 6: real impl)
// ---------------------------------------------------------------------------
// Game code (wibble.cpp, etc.) dereferences ge_get_screen_buffer() without
// null checks. Provide a dummy buffer so writes don't crash. Effects won't
// be visible until Phase 6 implements real framebuffer access.

static constexpr int32_t DUMMY_SCREEN_W = 640;
static constexpr int32_t DUMMY_SCREEN_H = 480;
static constexpr int32_t DUMMY_SCREEN_BPP = 4; // 32bpp
static constexpr int32_t DUMMY_SCREEN_PITCH = DUMMY_SCREEN_W * DUMMY_SCREEN_BPP;

static uint8_t* ensure_dummy_screen()
{
    if (!s_dummy_screen) {
        s_dummy_screen = (uint8_t*)calloc(1, DUMMY_SCREEN_PITCH * DUMMY_SCREEN_H);
    }
    return s_dummy_screen;
}

void* ge_lock_screen()
{
    // Return NULL — framebuffer pixel access not implemented yet (Phase 6).
    // Callers (wibble.cpp puddle effect) check return value and skip if NULL.
    // ge_get_screen_buffer() still returns a dummy buffer for code that doesn't check.
    return nullptr;
}
void  ge_unlock_screen() {}
uint8_t* ge_get_screen_buffer() { return ensure_dummy_screen(); }
int32_t  ge_get_screen_pitch() { return DUMMY_SCREEN_PITCH; }
int32_t  ge_get_screen_width() { return gl_context_get_width(); }
int32_t  ge_get_screen_height() { return gl_context_get_height(); }
int32_t  ge_get_screen_bpp() { return 32; }
void ge_plot_pixel(int32_t, int32_t, uint8_t, uint8_t, uint8_t) {}
void ge_plot_formatted_pixel(int32_t, int32_t, uint32_t) {}
void ge_get_pixel(int32_t, int32_t, uint8_t*, uint8_t*, uint8_t*) {}
uint32_t ge_get_formatted_pixel(uint8_t r, uint8_t g, uint8_t b)
{
    // Pack as 0xAARRGGBB (32-bit).
    return 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
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
// Texture management
// ---------------------------------------------------------------------------

void ge_remove_all_loaded_textures()
{
    // In D3D this removes textures from the display driver's tracking list.
    // In OpenGL there's no equivalent driver tracking — no-op.
}

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

static constexpr int GL_VB_POOL_MAX = 512;
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

// Minimum VB allocation: 2^10 = 1024 vertices = 32KB. Large enough that
// ge_vb_expand is rarely needed, avoiding realloc pointer invalidation.
static constexpr uint32_t GL_VB_MIN_LOGSIZE = 10;

void* ge_vb_alloc(uint32_t logsize, void** out_ptr, uint32_t* out_logsize)
{
    vb_pool_ensure_init();

    // Clamp to minimum size to reduce expand frequency.
    if (logsize < GL_VB_MIN_LOGSIZE) logsize = GL_VB_MIN_LOGSIZE;

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

    // Use malloc + memcpy instead of realloc to avoid invalidating old pointer.
    // Old data stays valid (for any code that cached the pointer) until the
    // slot is reused in a later frame.
    uint8_t* new_data = (uint8_t*)malloc(needed);
    if (!new_data) return vb; // keep old on failure

    uint32_t old_size = buf->capacity;
    if (buf->data && old_size > 0) {
        memcpy(new_data, buf->data, old_size);
        free(buf->data);
    }

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
        s_tl_u_fog_enabled, s_tl_u_fog_color, s_tl_u_fog_near, s_tl_u_fog_far,
        s_tl_u_specular_enabled, s_tl_u_color_key_enabled, s_tl_u_tex_has_alpha);

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
// Texture pixel access (for truetype cache writes)
// ---------------------------------------------------------------------------

bool ge_lock_texture_pixels(int32_t page, uint16_t** bitmap, int32_t* pitch)
{
    if (page < 0 || page >= GL_TEX_MAX) return false;
    GLTexture& tex = s_textures[page];
    if (tex.type == GE_TEXTURE_TYPE_UNUSED || !tex.gl_id) return false;

    // Allocate staging buffer if not present (BGRA, 4 bytes per pixel).
    // The truetype code writes using 16-bit pixel format from ge_get_texture_pixel_format.
    // We provide a 16-bit staging buffer with A1R5G5B5 format.
    int32_t buf_size = tex.size * tex.size * 2; // 16bpp
    if (!tex.staging) {
        tex.staging = (uint8_t*)calloc(1, buf_size);
        tex.staging_pitch = tex.size * 2;
    }

    *bitmap = (uint16_t*)tex.staging;
    *pitch = tex.staging_pitch;
    return true;
}

void ge_unlock_texture_pixels(int32_t page)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    GLTexture& tex = s_textures[page];
    if (!tex.staging || !tex.gl_id) return;

    // Convert 16-bit A1R5G5B5 staging buffer to BGRA8 and upload.
    int32_t w = tex.size, h = tex.size;
    uint8_t* rgba = (uint8_t*)malloc(w * h * 4);
    uint16_t* src = (uint16_t*)tex.staging;

    for (int32_t i = 0; i < w * h; i++) {
        uint16_t px = src[i];
        // A1R5G5B5: bit 15 = alpha, bits 14-10 = red, 9-5 = green, 4-0 = blue
        uint8_t a = (px & 0x8000) ? 255 : 0;
        uint8_t r = (uint8_t)(((px >> 10) & 0x1F) * 255 / 31);
        uint8_t g = (uint8_t)(((px >> 5)  & 0x1F) * 255 / 31);
        uint8_t b = (uint8_t)(((px >> 0)  & 0x1F) * 255 / 31);
        rgba[i * 4 + 0] = b;  // BGRA
        rgba[i * 4 + 1] = g;
        rgba[i * 4 + 2] = r;
        rgba[i * 4 + 3] = a;
    }

    glBindTexture(GL_TEXTURE_2D, tex.gl_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, rgba);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(rgba);
}

void ge_get_texture_pixel_format(int32_t page,
    int32_t* mask_r, int32_t* mask_g, int32_t* mask_b, int32_t* mask_a,
    int32_t* shift_r, int32_t* shift_g, int32_t* shift_b, int32_t* shift_a)
{
    // Provide A1R5G5B5 format masks/shifts (matches the 16-bit staging buffer).
    // The truetype code packs pixels as: (component >> mask) << shift.
    // A1R5G5B5: A=bit15, R=bits14-10, G=bits9-5, B=bits4-0.
    (void)page;
    *mask_r = 3; *shift_r = 10;  // 5 bits red   (8-5=3)
    *mask_g = 3; *shift_g = 5;   // 5 bits green
    *mask_b = 3; *shift_b = 0;   // 5 bits blue
    *mask_a = 7; *shift_a = 15;  // 1 bit alpha  (8-1=7)
}

bool ge_is_texture_loaded(int32_t page)
{
    if (page < 0 || page >= GL_TEX_MAX) return false;
    return s_textures[page].type != GE_TEXTURE_TYPE_UNUSED;
}

void ge_debug_paint_block(uint32_t) {}

// ---------------------------------------------------------------------------
// Gamma
// ---------------------------------------------------------------------------

static int32_t s_gamma_black = 0, s_gamma_white = 255;

void ge_set_gamma(int32_t black, int32_t white) { s_gamma_black = black; s_gamma_white = white; }
void ge_get_gamma(int32_t* black, int32_t* white) { *black = s_gamma_black; *white = s_gamma_white; }
bool ge_is_gamma_available() { return false; } // TODO: SDL gamma or shader post-process

// ---------------------------------------------------------------------------
// Texture loading lifecycle
// ---------------------------------------------------------------------------

void ge_texture_loading_done()
{
    // D3D releases the fast-load scratch buffer here. No equivalent in OpenGL.
}

void ge_texture_loading_begin()
{
    // Clear any stale GL errors before loading.
    while (glGetError() != GL_NO_ERROR) {}
    // Reset render states (game needs to re-sort polygons after texture batch load).
    if (s_render_states_reset_callback) s_render_states_reset_callback();
}

void ge_texture_load_tga(int32_t page, const char* path, bool can_shrink)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    GLTexture& tex = s_textures[page];

    // Already loaded — skip (matches D3D behavior).
    if (tex.type != GE_TEXTURE_TYPE_UNUSED) return;

    strncpy(tex.name, path, sizeof(tex.name) - 1);
    tex.name[sizeof(tex.name) - 1] = '\0';
    tex.file_id = (uint32_t)page;
    tex.can_shrink = can_shrink;
    tex.type = GE_TEXTURE_TYPE_TGA;

    gl_load_tga(tex);
}

void ge_texture_create_user_page(int32_t page, int32_t size, bool alpha_fill)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    GLTexture& tex = s_textures[page];

    // Reset render states (matches D3D's CreateUserPage → Reload behavior).
    if (s_render_states_reset_callback) s_render_states_reset_callback();

    tex.type = GE_TEXTURE_TYPE_USER;
    tex.size = size;
    tex.contains_alpha = alpha_fill;

    // Create blank BGRA texture.
    int32_t buf_size = size * size * 4;
    uint8_t* blank = (uint8_t*)calloc(1, buf_size);
    if (alpha_fill) {
        // Fill with transparent black (already zeroed).
    }

    glGenTextures(1, &tex.gl_id);
    glBindTexture(GL_TEXTURE_2D, tex.gl_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, blank);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    free(blank);
}

void ge_texture_destroy(int32_t page)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    gl_destroy_texture(s_textures[page]);
}

void ge_texture_free_all()
{
    // Reset render states and sorting.
    if (s_render_states_reset_callback) s_render_states_reset_callback();
}

void ge_texture_change_tga(int32_t page, const char* path)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    GLTexture& tex = s_textures[page];

    if (tex.type == GE_TEXTURE_TYPE_UNUSED) return;

    // Destroy old GL texture but keep metadata (flags, fonts get rebuilt).
    if (tex.gl_id) { glDeleteTextures(1, &tex.gl_id); tex.gl_id = 0; }

    strncpy(tex.name, path, sizeof(tex.name) - 1);
    tex.name[sizeof(tex.name) - 1] = '\0';

    gl_load_tga(tex);
}

void ge_texture_font_on(int32_t page)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    s_textures[page].flags |= GL_TEX_FLAG_FONT;
}

void ge_texture_font2_on(int32_t page)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    s_textures[page].flags |= GL_TEX_FLAG_FONT2;
}

void ge_texture_set_greyscale(int32_t page, bool greyscale)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    GLTexture& tex = s_textures[page];

    if ((bool)tex.greyscale != greyscale) {
        tex.greyscale = greyscale;
        if (tex.type == GE_TEXTURE_TYPE_TGA) {
            // Reload to apply greyscale.
            if (tex.gl_id) { glDeleteTextures(1, &tex.gl_id); tex.gl_id = 0; }
            gl_load_tga(tex);
        }
    }
}

void ge_get_texture_offset(int32_t page, float* uScale, float* uOffset, float* vScale, float* vOffset)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    GLTexture& tex = s_textures[page];

    switch (tex.page_type) {
    case GL_PAGE_NONE:
        *uScale = 1.0f; *vScale = 1.0f;
        *uOffset = 0.0f; *vOffset = 0.0f;
        break;
    case GL_PAGE_64_3X3:
    case GL_PAGE_32_3X3:
        *uScale = 0.25f; *vScale = 0.25f;
        *uOffset = 0.375f * (float)(tex.page_pos % 3);
        *vOffset = 0.375f * (float)(tex.page_pos / 3);
        break;
    case GL_PAGE_64_4X4:
    case GL_PAGE_32_4X4:
        *uScale = 0.25f; *vScale = 0.25f;
        *uOffset = 0.25f * (float)(tex.page_pos & 0x3);
        *vOffset = 0.25f * (float)(tex.page_pos >> 2);
        break;
    default:
        *uScale = 1.0f; *vScale = 1.0f;
        *uOffset = 0.0f; *vOffset = 0.0f;
        break;
    }
}

int32_t ge_texture_get_size(int32_t page)
{
    if (page < 0 || page >= GL_TEX_MAX) return 0;
    return s_textures[page].size;
}

int32_t ge_texture_get_type(int32_t page)
{
    if (page < 0 || page >= GL_TEX_MAX) return 0;
    return s_textures[page].type;
}

void ge_texture_set_type(int32_t page, int32_t type)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    s_textures[page].type = type;
}

GETextureHandle ge_get_texture_handle(int32_t page)
{
    if (page < 0 || page >= GL_TEX_MAX) return GE_TEXTURE_NONE;
    return (GETextureHandle)s_textures[page].gl_id;
}

Font* ge_get_font(int32_t page, int32_t id)
{
    if (page < 0 || page >= GL_TEX_MAX) return nullptr;
    Font* f = s_textures[page].font_list;
    while (id && f) {
        f = f->NextFont;
        id--;
    }
    return f;
}

// ---------------------------------------------------------------------------
// Font extraction from texture pixels
// ---------------------------------------------------------------------------

// Pixel match helper for font separator detection (magenta = 0xFF,0x00,0xFF).
static bool gl_match_pixels(GLBGRAPixel* p1, GLBGRAPixel* p2)
{
    return p1->red == p2->red && p1->green == p2->green && p1->blue == p2->blue;
}

// Scan rows until a row starting with the underline color is found.
static bool gl_scan_for_baseline(GLBGRAPixel** line_ptr, GLBGRAPixel* underline,
                                  GLTexLoadInfo* info, int32_t* y_ptr)
{
    while (*y_ptr < info->height) {
        if (gl_match_pixels(*line_ptr, underline)) {
            *y_ptr += 1;
            *line_ptr += info->width;
            return true;
        }
        *y_ptr += 1;
        *line_ptr += info->width;
    }
    return false;
}

// Parse glyph geometry from a font texture using magenta separators.
// Builds a linked list of Font objects. Matches D3D CreateFonts exactly.
static bool gl_create_fonts(Font** font_list, GLTexLoadInfo* info, GLBGRAPixel* data)
{
    GLBGRAPixel underline = { 0xFF, 0x00, 0xFF, 0xFF }; // blue=0xFF, green=0, red=0xFF
    GLBGRAPixel* current_line = data;
    int32_t char_y = 0;

    if (!gl_scan_for_baseline(&current_line, &underline, info, &char_y))
        return false;

map_font:
    {
        Font* the_font = (Font*)MemAlloc(sizeof(Font));
        memset(the_font, 0, sizeof(Font));
        if (*font_list) {
            the_font->NextFont = *font_list;
        }
        *font_list = the_font;

        int32_t current_char = 0;
        int32_t char_x = 0;
        int32_t tallest_char = 1;

        while (current_char < 93) {
            // Scan across to find width.
            int32_t char_width = 0;
            GLBGRAPixel* current_pixel = current_line + char_x;
            while (!gl_match_pixels(current_pixel, &underline)) {
                current_pixel++;
                char_width++;

                if (char_x + char_width >= info->width) {
                    char_y += tallest_char + 1;
                    if (char_y >= info->height) return false;
                    current_line = data + (char_y * info->width);

                    if (!gl_scan_for_baseline(&current_line, &underline, info, &char_y))
                        return false;

                    char_x = 0;
                    tallest_char = 1;
                    char_width = 0;
                    current_pixel = current_line;
                }
            }

            // Scan down to find height.
            int32_t char_height = 0;
            current_pixel = current_line + char_x;
            while (!gl_match_pixels(current_pixel, &underline)) {
                current_pixel += info->width;
                char_height++;
                if (char_height >= info->height) return false;
            }

            the_font->CharSet[current_char].X = char_x;
            the_font->CharSet[current_char].Y = char_y;
            the_font->CharSet[current_char].Width = char_width;
            the_font->CharSet[current_char].Height = char_height;

            char_x += char_width + 1;
            if (tallest_char < char_height)
                tallest_char = char_height;

            current_char++;
        }

        // Check for another font in the same file.
        char_y += tallest_char + 1;
        if (char_y >= info->height) return true;
        current_line = data + (char_y * info->width);

        if (gl_scan_for_baseline(&current_line, &underline, info, &char_y))
            goto map_font;
    }

    return true;
}

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

// Work screen globals (D3D uses DDraw offscreen surface; we use a CPU buffer).
static UBYTE s_work_screen_buf[640 * 480 * 4];
UBYTE WorkScreenDepth = 32;
UBYTE* WorkScreen = s_work_screen_buf;
UBYTE* WorkWindow = s_work_screen_buf;
SLONG WorkScreenHeight = 480;
SLONG WorkScreenWidth = 640 * 4; // pitch in bytes (D3D convention)
SLONG WorkScreenPixelWidth = 640;
SLONG WorkWindowHeight = 480;
SLONG WorkWindowWidth = 640;
MFRect WorkWindowRect = { 0, 0, 640, 480, 640, 480 };
UBYTE CurrentPalette[256 * 3] = {};

// image_mem: used for loading screen background images (ge_init_back_image).
static UBYTE s_image_mem_buf[640 * 480 * 3];
UBYTE* image_mem = s_image_mem_buf;

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

void* LockWorkScreen() { return s_work_screen_buf; }
void UnlockWorkScreen() {}
void ShowWorkScreen(ULONG) {}
void ClearWorkScreen(UBYTE) {}
