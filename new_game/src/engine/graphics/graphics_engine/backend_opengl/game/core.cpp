// Graphics engine — OpenGL 4.1 core profile implementation.
// Phase 1: lifecycle, clear, flip, render state, viewport.
// Phase 2: shaders + drawing (ge_draw_indexed_primitive, ge_draw_indexed_primitive_lit).
// Phase 3: texture loading, binding, font extraction, user pages.

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/gl_context.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/gl_shader.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
#include "engine/graphics/graphics_engine/backend_opengl/postprocess/composition.h"
#include "engine/graphics/graphics_engine/backend_opengl/postprocess/crt_effect.h"
#include "engine/platform/uc_common.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/io/file.h"
#include "engine/io/oc_config.h"
#include "engine/graphics/aspect_clamp.h"  // FOV_CAP_ASPECT / FOV_MIN_ASPECT
#include "engine/core/memory.h"
#include "engine/graphics/pipeline/polypage.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Backend-local state
// ---------------------------------------------------------------------------

static char s_data_dir[256] = "";

// GL state cache — avoid redundant GL calls on macOS GL→Metal translation layer.
static GLuint   s_cached_program = 0;          // currently bound program

// Uniform snapshot — last uploaded values, skip re-upload if unchanged.
struct UniformSnapshot {
    GETextureHandle texture;
    bool            texture_has_alpha;
    GETextureBlend  texture_blend;
    GETextureFilter tex_mag, tex_min;
    GETextureAddress tex_address;
    bool            alpha_test_enabled;
    uint32_t        alpha_ref;
    GECompareFunc   alpha_func;
    bool            fog_enabled;
    uint32_t        fog_color;
    float           fog_near, fog_far;
    bool            specular_enabled;
    bool            color_key_enabled;
    int32_t         vp_x, vp_y, vp_w, vp_h;
    int             farfacet_mode;
};
static UniformSnapshot s_last_uniforms = {};
static bool s_uniforms_ever_uploaded = false;

// Callbacks registered by game code.
static GEPreFlipCallback              s_pre_flip_callback = nullptr;
static GEPostCompositionCallback      s_post_composition_callback = nullptr;
static GEModeChangeCallback           s_mode_change_callback = nullptr;
static GERenderStatesResetCallback    s_render_states_reset_callback = nullptr;
static GETGALoadCallback              s_tga_load_callback = nullptr;
static GETextureReloadPrepareCallback s_texture_reload_prepare_callback = nullptr;

// UI mode flag — redirects ge_begin_scene() to default FB. See
// game_graphics_engine.h for rationale.
static bool s_ui_mode_active = false;

// UI pass viewport size (set by ge_enter_ui_viewport). When ui_mode is
// active, ge_get_screen_width/height report these instead of the scene
// FBO size, so pixel-bounds checks inside UI draw code (e.g.
// FONT_draw_coloured_char clipping glyphs outside screen) compare against
// the actual UI target dimensions.
static int32_t s_ui_vp_w = 0;
static int32_t s_ui_vp_h = 0;

// UI pass viewport ORIGIN on the default framebuffer (set by
// ge_enter_ui_viewport). Needed by ge_set_scissor / ge_disable_scissor
// because glScissor takes ABSOLUTE framebuffer coords — push_ui_mode hands
// us scissor coords relative to the dst rect (assumes (0, 0) origin like
// the shader uses for u_viewport), but the actual viewport may be offset
// inside the window when the FBO aspect doesn't match the window aspect
// (composition pillar/letterbox bars). Without adding this offset the
// scissor lands at window-(0..dst_w) while the viewport draws at
// window-(dst_x..dst_x+dst_w), and their intersection is the visible band
// — which clipped pause-menu / cutscene-dialog UI to a narrow strip on
// wider-than-cap-aspect windows. Stays 0 when ui_mode isn't active so
// scene-pass scissor is unaffected.
static int32_t s_ui_vp_x = 0;
static int32_t s_ui_vp_y = 0;

// Background color for ge_clear.
static float s_clear_r = 0.0f, s_clear_g = 0.0f, s_clear_b = 0.0f;

// Current viewport (D3D screen coordinates: top-left origin).
static int32_t s_vp_x = 0, s_vp_y = 0, s_vp_w = 640, s_vp_h = 480;

// Fog state.
static bool     s_fog_enabled = false;
static uint32_t s_fog_color = 0;
static float    s_fog_near = 0.0f, s_fog_far = 1.0f;

// Far-facet silhouette mode: 0 = off, 1 = production (fog-colour
// silhouettes, alpha fade tied to fog density), 2 = debug split
// (opaque pixels painted purple, semi-transparent pixels green).
static int      s_farfacet_mode = 0;

float g_mm_fog_view_z = 0.0f;

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
static bool s_bound_texture_has_mipmaps = false;

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

// Background override.
static GEScreenSurface s_background_override = GE_SCREEN_SURFACE_NONE;

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
    bool     has_mipmaps;     // True if glGenerateMipmap was called
    bool     no_mipmaps;      // True = skip mipmap generation (set before upload)
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

// Edge color bleeding: for fully transparent pixels (alpha=0, RGB=0), copy RGB from the
// nearest non-black neighbor to prevent bilinear filtering artifacts at alpha edges.
//
// Uses a read-only copy of the original pixel data for neighbor lookups to prevent
// chain propagation.  In D3D6, edge bleeding reads from tga[] (source) and writes to
// the D3D surface (destination) — two separate buffers, no chain reaction.  Our original
// implementation read and wrote the same array, causing a chain reaction where bled pixels
// became valid neighbors for subsequent pixels, filling the entire black background with
// ghost RGB.  This was the root cause of the ghost RGB issue on HUD icons.
static void gl_bleed_edges(GLBGRAPixel* pixels, int32_t w, int32_t h)
{
    int32_t count = w * h;
    GLBGRAPixel* source = (GLBGRAPixel*)MemAlloc(count * sizeof(GLBGRAPixel));
    memcpy(source, pixels, count * sizeof(GLBGRAPixel));

    for (int32_t j = 0; j < h; j++) {
        for (int32_t i = 0; i < w; i++) {
            GLBGRAPixel& p = pixels[i + j * w];
            // Only process fully transparent pixels (RGB all zero).
            if (p.red || p.green || p.blue) continue;
            if (p.alpha) continue;

            // Find nearest non-black neighbor in the ORIGINAL (unmodified) data.
            int32_t i2 = i, j2 = j;
            #define GL_HAS_COLOR(x, y) (source[(x) + (y) * w].red | source[(x) + (y) * w].green | source[(x) + (y) * w].blue)
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

            // Copy RGB from original data, not from potentially bled neighbors.
            p.red   = source[i2 + j2 * w].red;
            p.green = source[i2 + j2 * w].green;
            p.blue  = source[i2 + j2 * w].blue;
            // Alpha stays 0 — only RGB is bled.
        }
    }

    MemFree(source);
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

    // Generate mipmaps to eliminate texture shimmer on distant surfaces.
    // Caller can set tex.no_mipmaps = true to skip (e.g. dirt system textures
    // where mipmap averaging bleeds dark outlines into the color).
    if (tex.no_mipmaps) {
        // Dirt textures (leaves, rubbish, snowflakes): no mipmaps.
        // Black outlines bleed into mip levels and darken sprites.
        tex.has_mipmaps = false;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glGenerateMipmap(GL_TEXTURE_2D);
        tex.has_mipmaps = true;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1.7f);
        // GL_TEXTURE_MAX_ANISOTROPY_EXT (0x84FF)
        glTexParameterf(GL_TEXTURE_2D, 0x84FF, 16.0f);
    }
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
// Source files: backend_opengl/shaders/{tl_vert,common_frag}.glsl
// ---------------------------------------------------------------------------

#include "gl_shaders_embedded.h"

// ---------------------------------------------------------------------------
// Shader programs and GL objects
// ---------------------------------------------------------------------------

static GLuint s_program_tl  = 0;  // TL vertex shader program
// Lit vertex shader removed — all geometry uses CPU-transform → TL path.
// See tl_vert.glsl header and known_issues_and_bugs.md for details.

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
static GLint s_tl_u_farfacet_mode      = -1;


// VAO for each vertex format. VBO/EBO are shared (streaming).
static GLuint s_vao_tl  = 0;
static GLuint s_vbo     = 0;  // streaming vertex buffer
static GLuint s_ebo     = 0;  // streaming index buffer

static bool s_shaders_ready = false;

// Cache uniform locations for a program's fragment shader uniforms.
static void cache_frag_uniforms(GLuint prog,
    GLint* u_has_texture, GLint* u_texture, GLint* u_texture_blend,
    GLint* u_alpha_test_enabled, GLint* u_alpha_ref, GLint* u_alpha_func,
    GLint* u_fog_enabled, GLint* u_fog_color, GLint* u_fog_near, GLint* u_fog_far,
    GLint* u_specular_enabled, GLint* u_color_key_enabled, GLint* u_tex_has_alpha,
    GLint* u_farfacet_mode)
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
    *u_farfacet_mode      = glGetUniformLocation(prog, "u_farfacet_mode");
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

    // Cache TL uniform locations.
    s_tl_u_viewport = glGetUniformLocation(s_program_tl, "u_viewport");
    cache_frag_uniforms(s_program_tl,
        &s_tl_u_has_texture, &s_tl_u_texture, &s_tl_u_texture_blend,
        &s_tl_u_alpha_test_enabled, &s_tl_u_alpha_ref, &s_tl_u_alpha_func,
        &s_tl_u_fog_enabled, &s_tl_u_fog_color, &s_tl_u_fog_near, &s_tl_u_fog_far,
        &s_tl_u_specular_enabled, &s_tl_u_color_key_enabled, &s_tl_u_tex_has_alpha,
        &s_tl_u_farfacet_mode);

    // Create shared VBO and EBO. Pre-allocate to avoid repeated orphaning
    // (glBufferData with different sizes) which can trigger NVIDIA driver bugs.
    glGenBuffers(1, &s_vbo);
    glGenBuffers(1, &s_ebo);
    // Pre-allocate: 8192 verts * 32 bytes = 256KB VBO, 32K indices * 2 = 64KB EBO.
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, 8192 * 32, nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 32768 * sizeof(uint16_t), nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Create VAO.
    glGenVertexArrays(1, &s_vao_tl);
    setup_vao_tl(s_vao_tl, s_vbo, s_ebo);

    s_shaders_ready = true;
    fprintf(stderr, "OpenGL shaders initialized (TL program)\n");
    return true;
}

static void destroy_shaders()
{
    if (s_program_tl)  { glDeleteProgram(s_program_tl);  s_program_tl = 0; }
    if (s_vao_tl)  { glDeleteVertexArrays(1, &s_vao_tl);  s_vao_tl = 0; }
    if (s_vbo) { glDeleteBuffers(1, &s_vbo); s_vbo = 0; }
    if (s_ebo) { glDeleteBuffers(1, &s_ebo); s_ebo = 0; }
    s_shaders_ready = false;
}

// Upload current fragment shader uniforms to the active program.
// Uses snapshot comparison to skip re-upload when nothing changed.
static void set_frag_uniforms(
    GLint u_has_texture, GLint u_texture, GLint u_texture_blend,
    GLint u_alpha_test_enabled, GLint u_alpha_ref, GLint u_alpha_func,
    GLint u_fog_enabled, GLint u_fog_color, GLint u_fog_near, GLint u_fog_far,
    GLint u_specular_enabled, GLint u_color_key_enabled, GLint u_tex_has_alpha,
    GLint u_farfacet_mode)
{
    // Check if anything changed since last upload.
    UniformSnapshot cur = {
        s_bound_texture, s_bound_texture_has_alpha, s_texture_blend,
        s_tex_filter_mag, s_tex_filter_min, s_tex_address,
        s_alpha_test_enabled, s_alpha_ref, s_alpha_func,
        s_fog_enabled, s_fog_color, s_fog_near, s_fog_far,
        s_specular_enabled, s_color_key_enabled,
        s_vp_x, s_vp_y, s_vp_w, s_vp_h,
        s_farfacet_mode
    };

    if (s_uniforms_ever_uploaded && memcmp(&cur, &s_last_uniforms, sizeof(cur)) == 0)
        return;

    s_last_uniforms = cur;
    s_uniforms_ever_uploaded = true;

    // Viewport uniform.
    glUniform4f(s_tl_u_viewport,
        (float)s_vp_x, (float)s_vp_y, (float)s_vp_w, (float)s_vp_h);

    bool has_tex = (s_bound_texture != GE_TEXTURE_NONE);
    glUniform1i(u_tex_has_alpha, s_bound_texture_has_alpha ? 1 : 0);
    glUniform1i(u_has_texture, has_tex ? 1 : 0);

    if (has_tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, s_bound_texture);
        glUniform1i(u_texture, 0);

        // Apply current filter state. Use mipmap variants only for textures
        // that have mipmaps — others (screen surfaces, shadows) would be
        // incomplete and render black with mipmap filters.
        GLint gl_mag = (s_tex_filter_mag == GETextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
        GLint gl_min;
        if (s_bound_texture_has_mipmaps) {
            gl_min = (s_tex_filter_min == GETextureFilter::Nearest) ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
        } else {
            gl_min = (s_tex_filter_min == GETextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
        }
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
    glUniform1i(u_farfacet_mode, s_farfacet_mode);
}

// ---------------------------------------------------------------------------
// Callback registration
// ---------------------------------------------------------------------------

void ge_set_pre_flip_callback(GEPreFlipCallback callback)              { s_pre_flip_callback = callback; }
void ge_set_post_composition_callback(GEPostCompositionCallback callback) { s_post_composition_callback = callback; }
void ge_set_ui_mode(bool active) { s_ui_mode_active = active; }

void ge_enter_ui_viewport(int32_t x, int32_t y, int32_t w, int32_t h)
{
    // GL bottom-left viewport directly — no Y flip. Caller (UI pass) passes
    // the composition dst rect, which is already in GL bottom-left form.
    glViewport(x, y, w, h);

    // Reset the tracked viewport to a 0-origin rect the size of the UI
    // pass target. The TL vertex shader normalises incoming pixel coords
    // by (u_viewport.x/y, u_viewport.z/w); with origin (0, 0) and the UI
    // rect's width/height, literal pixel coords map directly onto the UI
    // rect pixels on the default FB.
    s_vp_x = 0;
    s_vp_y = 0;
    s_vp_w = w;
    s_vp_h = h;

    // Record the UI viewport size so ge_get_screen_width/height report
    // it instead of the scene FBO size during UI draws. Game code uses
    // those to clip off-screen glyphs / rects; clipping against the
    // scene FBO size when we're actually drawing to a larger UI rect
    // would truncate legitimate text (symptom before this fix: F1 legend
    // at OC_RENDER_SCALE=0.25 cut off past the scene-FBO width).
    s_ui_vp_w = w;
    s_ui_vp_h = h;
    // Also remember the actual viewport ORIGIN on the default FB. Used
    // by ge_set_scissor / ge_disable_scissor to convert UI-relative
    // scissor coords (which assume 0,0 origin like the shader's
    // u_viewport) into absolute FB coords that glScissor expects. Without
    // this, scissor lands at FB(0..w) while viewport draws at
    // FB(x..x+w), so when x ≠ 0 (composition pillarbox bars present),
    // the visible band gets clipped to their intersection.
    s_ui_vp_x = x;
    s_ui_vp_y = y;

    // Force the next draw call to re-upload the uniform (the cache was
    // populated with scene-FBO viewport values).
    s_uniforms_ever_uploaded = false;
}

void ge_invalidate_uniform_cache()
{
    s_uniforms_ever_uploaded = false;
}
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

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void ge_begin_scene()
{
    // Redirect the frame into the scene FBO. All subsequent game draw calls
    // (3D, HUD, overlays, pre-flip callback) land there. The composition
    // pass in ge_flip() upscales it to the default framebuffer.
    //
    // Exception: during the post-composition UI pass, ui_mode is on and we
    // bind the default FB instead so UI draws land on top of the composed
    // scene (at native resolution, untouched by FXAA/upscale). POLY_frame_init
    // calls ge_begin_scene implicitly — without this redirect, UI code would
    // silently send draws back into the scene FBO after composition already
    // ran, and the results would be invisible until next frame.
    if (s_ui_mode_active)
        composition_bind_default();
    else
        composition_bind_scene();
}

void ge_end_scene()
{
    // Scene FBO stays bound — pre-flip callback (game_pre_flip) still needs
    // to draw into it (screensaver / depth bodge reset). Unbind happens in
    // ge_flip() right before the composition blit.
}

void ge_clear(bool color, bool depth)
{
    GLbitfield mask = 0;
    if (color) mask |= GL_COLOR_BUFFER_BIT;
    if (depth) mask |= GL_DEPTH_BUFFER_BIT;
    if (mask) {
        if (color) glClearColor(s_clear_r, s_clear_g, s_clear_b, 1.0f);
        if (depth) glDepthMask(GL_TRUE); // ensure depth writes for clear

        // glClear honours the scissor rectangle. In SCENE mode the scissor
        // matches the viewport (= full FBO), so disabling it makes no
        // visible difference; we keep the disable to wipe the entire FBO
        // even if some caller narrowed scissor before calling us.
        //
        // In UI mode, however, scissor is bounded to the UI viewport
        // (s_ui_vp_x/y/w/h) — disabling it would wipe the entire default
        // FB, taking out the composition layer's outer pillar/letterbox
        // bars and any UI already drawn earlier in this pass. Keep
        // scissor enabled in UI mode so the clear stays inside the UI
        // rect (which is exactly what a frontend-style "clear and redraw
        // background" wants).
        GLboolean scissor_was_enabled = GL_FALSE;
        glGetBooleanv(GL_SCISSOR_TEST, &scissor_was_enabled);
        const bool keep_scissor = s_ui_mode_active;
        if (scissor_was_enabled && !keep_scissor) glDisable(GL_SCISSOR_TEST);
        glClear(mask);
        if (scissor_was_enabled) glEnable(GL_SCISSOR_TEST);
    }
}

// Capture the final default-FB content into the snapshot texture and
// then swap. Used by every "this frame is finished, present it"
// path (ge_flip, ge_blit_back_buffer, ge_video_draw_and_swap) so the
// drag-resize present always has a fresh snapshot to stretch. The
// drag-resize present itself (ge_present_for_drag) does NOT go through
// here — it intentionally skips capture (it presents the previous
// snapshot) and does its own swap.
//
// Wraps capture+swap in one place so adding a new flip path can't
// accidentally skip the snapshot (was the failure mode that left the
// loadscreen visible during drag while a video was playing — video
// had its own swap site that I forgot to update).
static void present_and_swap(int win_w, int win_h)
{
    // Snapshot the finished default-FB into a backing texture so an
    // OS-level drag-resize (which pauses the game's main loop and
    // therefore can't refresh per-tick UI state) has a frame to scale
    // visually. See composition.h's "Last-frame snapshot" section.
    composition_capture_last_frame(win_w, win_h);
    gl_context_swap();
}

void ge_flip()
{
    // Reset GL state cache at frame boundary.
    if (s_cached_program) {
        glUseProgram(0);
        s_cached_program = 0;
    }
    glBindVertexArray(0);
    s_uniforms_ever_uploaded = false;

    // Last chance for game code to draw into the scene FBO (screensaver,
    // depth bodge reset).
    if (s_pre_flip_callback) s_pre_flip_callback();

    // Scene is complete — run composition into the default framebuffer at
    // full window size. This does the upscale from scene-FBO → window and
    // applies FXAA when OC_AA_ENABLE is on.
    int win_w = 0, win_h = 0;
    sdl3_window_get_drawable_size(&win_w, &win_h);
    composition_bind_default();
    composition_blit(win_w, win_h);

    // Default FB is bound and contains the composed scene. UI pass draws
    // on top at native resolution, untouched by the composition shader.
    if (s_post_composition_callback) s_post_composition_callback();

    present_and_swap(win_w, win_h);

    // Re-bind the scene FBO for the next frame — see OpenDisplay for why
    // the scene FBO is kept bound between frames (some clears fire before
    // the next ge_begin_scene).
    composition_bind_scene();
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

void ge_set_farfacet_mode(int mode)
{
    s_farfacet_mode = mode;
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
    s_bound_texture_has_mipmaps = false;
    if (tex != GE_TEXTURE_NONE) {
        GLuint gl_id = (GLuint)(uintptr_t)tex;
        for (int32_t i = 0; i < GL_TEX_MAX; i++) {
            if (s_textures[i].gl_id == gl_id) {
                s_bound_texture_has_alpha = s_textures[i].contains_alpha;
                s_bound_texture_has_mipmaps = s_textures[i].has_mipmaps;
                break;
            }
        }
    }
}

void ge_set_bound_texture_has_alpha(bool has_alpha)
{
    s_bound_texture_has_alpha = has_alpha;
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void ge_draw_indexed_primitive(GEPrimitiveType type, const GEVertexTL* verts, uint32_t vert_count,
                               const uint16_t* indices, uint32_t index_count)
{
    if (!index_count || !vert_count) return;
    if (!init_shaders()) return;

    // Validate index bounds.
    for (uint32_t i = 0; i < index_count; i++) {
        if (indices[i] >= vert_count) {
            fprintf(stderr, "DIP OVERRUN: idx[%u]=%u >= vert_count=%u\n", i, indices[i], vert_count);
            return;
        }
    }

    if (s_cached_program != s_program_tl) {
        glUseProgram(s_program_tl);
        s_cached_program = s_program_tl;
        s_uniforms_ever_uploaded = false;
    }

    set_frag_uniforms(
        s_tl_u_has_texture, s_tl_u_texture, s_tl_u_texture_blend,
        s_tl_u_alpha_test_enabled, s_tl_u_alpha_ref, s_tl_u_alpha_func,
        s_tl_u_fog_enabled, s_tl_u_fog_color, s_tl_u_fog_near, s_tl_u_fog_far,
        s_tl_u_specular_enabled, s_tl_u_color_key_enabled, s_tl_u_tex_has_alpha,
        s_tl_u_farfacet_mode);

    glBindVertexArray(s_vao_tl);

    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_count * sizeof(GEVertexTL), verts, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint16_t), indices, GL_STREAM_DRAW);

    GLenum gl_mode = (type == GEPrimitiveType::TriangleFan) ? GL_TRIANGLE_FAN : GL_TRIANGLES;
    glDrawElements(gl_mode, index_count, GL_UNSIGNED_SHORT, nullptr);

}


void ge_draw_indexed_primitive_lit(GEPrimitiveType type, const GEVertexLit* verts, uint32_t vert_count,
                                   const uint16_t* indices, uint32_t index_count)
{
    if (!index_count || !vert_count) return;

    // TECH DEBT: CPU-transform. A GPU lit vertex shader would be faster but D3D
    // matrices use different clip space conventions than OpenGL (Z [0,1] vs [-1,1],
    // viewport Y). All other geometry already uses CPU-transform → TL path
    // (ge_draw_multi_matrix), so this is consistent. Performance is fine for ~200
    // leaf/dirt vertices per frame. See known_issues_and_bugs.md.

    // Compute combined WVP matrix (row-major, D3D convention: v' = v * WVP).
    // View is identity in this engine, so WVP = World * Projection.
    GEMatrix wvp;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            wvp.m[r][c] = s_world_matrix.m[r][0] * s_projection_matrix.m[0][c]
                        + s_world_matrix.m[r][1] * s_projection_matrix.m[1][c]
                        + s_world_matrix.m[r][2] * s_projection_matrix.m[2][c]
                        + s_world_matrix.m[r][3] * s_projection_matrix.m[3][c];
        }
    }

    float vp_x = (float)s_vp_x;
    float vp_y = (float)s_vp_y;
    float vp_w = (float)s_vp_w;
    float vp_h = (float)s_vp_h;

    // Transform into static TL buffer (single-threaded rendering, safe to reuse).
    static GEVertexTL tl[8192];
    ASSERT(vert_count <= 8192);

    for (uint32_t i = 0; i < vert_count; i++) {
        float x = verts[i].x, y = verts[i].y, z = verts[i].z;

        // v' = (x,y,z,1) * WVP  (D3D row-vector convention)
        float cx = x * wvp.m[0][0] + y * wvp.m[1][0] + z * wvp.m[2][0] + wvp.m[3][0];
        float cy = x * wvp.m[0][1] + y * wvp.m[1][1] + z * wvp.m[2][1] + wvp.m[3][1];
        float cz = x * wvp.m[0][2] + y * wvp.m[1][2] + z * wvp.m[2][2] + wvp.m[3][2];
        float cw = x * wvp.m[0][3] + y * wvp.m[1][3] + z * wvp.m[2][3] + wvp.m[3][3];

        // Perspective divide → NDC.
        float rhw = (cw != 0.0f) ? (1.0f / cw) : 1.0f;
        float ndc_x = cx * rhw;  // [-1,1] in D3D clip space (but X is flipped by proj._11=-1)
        float ndc_y = cy * rhw;

        // NDC → screen coords (matching D3D viewport transform).
        tl[i].x = (ndc_x + 1.0f) * 0.5f * vp_w + vp_x;  // but proj flips X, so ndc_x is already correct
        tl[i].y = (1.0f - ndc_y) * 0.5f * vp_h + vp_y;
        tl[i].z = cz * rhw;  // D3D Z [0,1]
        tl[i].rhw = rhw;

        tl[i].u = verts[i].u;
        tl[i].v = verts[i].v;
        tl[i].color = verts[i].color;
        tl[i].specular = verts[i].specular;
    }

    // Draw as TL (screen-space) triangles.
    ge_draw_indexed_primitive(type, tl, vert_count, indices, index_count);
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
    // OpenGL viewport Y is bottom-up, D3D is top-down. The height used
    // for the flip is the current render target height: scene FBO
    // normally, but the UI viewport height during the post-composition
    // UI pass (the default FB is typically larger than the scene FBO
    // when OC_RENDER_SCALE < 1).
    int32_t screen_h = (s_ui_mode_active && s_ui_vp_h > 0) ? s_ui_vp_h
                                                           : gl_context_get_height();
    // In UI mode also offset by the actual viewport origin on the default
    // FB. Caller passes UI-rect-relative coords (0..s_ui_vp_w, 0..s_ui_vp_h)
    // mirroring how scissor is specified, but glViewport takes absolute FB
    // coords. Without the offset, calling e.g. ge_set_viewport(0, 0, w, h)
    // from inside the UI pass would land the viewport at FB origin (0, 0)
    // rather than at the dst rect (s_ui_vp_x, s_ui_vp_y), yanking all
    // subsequent draws into the upper-left corner on windows wider than
    // the FBO aspect (composition pillarbox bars present).
    int32_t off_x = s_ui_mode_active ? s_ui_vp_x : 0;
    int32_t off_y = s_ui_mode_active ? s_ui_vp_y : 0;
    glViewport(off_x + x, off_y + screen_h - y - h, w, h);
    if (!PolyPage::in_ui_mode()) {
        // Default behavior — scissor matches the new viewport.
        glScissor(x, screen_h - y - h, w, h);
    }
    // In UI mode, the pipeline already owns scissor (set by push_ui_mode
    // to the framed UI region); leave it alone so widening the viewport
    // here doesn't break UI clipping.
    glEnable(GL_SCISSOR_TEST);
}

void ge_set_viewport_3d(int32_t x, int32_t y, int32_t w, int32_t h,
                        float, float, float, float)
{
    // Clip parameters are D3D6-specific (D3DVIEWPORT2 clipping volume).
    // In OpenGL, clipping is done by the projection matrix.
    ge_set_viewport(x, y, w, h);
}

void ge_set_scissor(int32_t x, int32_t y, int32_t w, int32_t h)
{
    // Top-left coords (D3D-style); flip for OpenGL bottom-up scissor.
    // Same render-target-height logic as ge_set_viewport — UI pass may
    // target a larger FB than the scene FBO. In UI mode also offset the
    // scissor by the actual viewport origin on the default FB: caller
    // passes coords relative to the UI viewport (0..w, 0..h), but
    // glScissor takes absolute FB coords. Without the offset, scissor
    // lands at the wrong x/y when composition centres the FBO with
    // pillar/letterbox bars.
    int32_t screen_h = (s_ui_mode_active && s_ui_vp_h > 0) ? s_ui_vp_h
                                                           : gl_context_get_height();
    int32_t off_x = s_ui_mode_active ? s_ui_vp_x : 0;
    int32_t off_y = s_ui_mode_active ? s_ui_vp_y : 0;
    glScissor(off_x + x, off_y + screen_h - y - h, w, h);
    glEnable(GL_SCISSOR_TEST);
}

void ge_disable_scissor()
{
    // ge_set_viewport leaves GL_SCISSOR_TEST enabled with scissor == viewport.
    // "Disable" here means restore that wide-as-viewport state, so subsequent
    // draws are not clipped by a tighter UI scissor.
    if (s_ui_mode_active && s_ui_vp_h > 0) {
        // UI pass: viewport is at (s_ui_vp_x, s_ui_vp_y) on the default
        // FB sized (s_ui_vp_w × s_ui_vp_h). Scissor must match that
        // absolute rect, not s_vp_* (which is reset to 0-origin for
        // shader normalisation).
        glScissor(s_ui_vp_x, s_ui_vp_y, s_ui_vp_w, s_ui_vp_h);
    } else {
        int32_t screen_h = gl_context_get_height();
        glScissor(s_vp_x, screen_h - s_vp_y - s_vp_h, s_vp_w, s_vp_h);
    }
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
// Screen dimensions + one-shot framebuffer read (screenshots)
// ---------------------------------------------------------------------------
// The old ge_lock_screen / ge_unlock_screen / ge_plot_pixel path — glReadPixels
// the whole backbuffer into a CPU buffer, let game code poke individual pixels,
// writeback via a fullscreen textured quad — is gone. Its per-frame readback
// triggered the WDDM/DWM hang pattern on affected hardware, and all of its
// game-side clients (FONT_buffer_draw, SKY_draw_stars, WIBBLE_simple) now
// render through the TL/post-process GPU paths in Stages 1-3.
//
// What remains is a strictly one-shot read-only helper used by screenshot
// code: glReadPixels → caller-owned buffer → caller saves to disk → done.
// No CPU-side buffer is retained, no writeback ever happens.

int32_t ge_get_screen_width()
{
    // During the UI pass we draw into the default FB at the composition
    // dst rect (which may differ from the scene FBO size when
    // OC_RENDER_SCALE < 1.0 or when the window aspect is outside the
    // FOV clamp). UI code that clips against ge_get_screen_width must
    // compare against the UI target, not the scene FBO.
    if (s_ui_mode_active && s_ui_vp_w > 0) return s_ui_vp_w;
    return gl_context_get_width();
}
int32_t ge_get_screen_height()
{
    if (s_ui_mode_active && s_ui_vp_h > 0) return s_ui_vp_h;
    return gl_context_get_height();
}
void ge_read_framebuffer_rgba(uint8_t* out, int32_t w, int32_t h)
{
    if (!out || w <= 0 || h <= 0) return;

    // glReadPixels hands back bottom-up rows; the callers (TGA screenshot)
    // want top-down. Read into the caller buffer then flip in place with a
    // 16-byte chunked swap.
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, out);

    const int32_t pitch = w * 4;
    uint8_t* top = out;
    uint8_t* bot = out + (h - 1) * pitch;
    while (top < bot) {
        for (int32_t x = 0; x < pitch; x += 16) {
            int32_t chunk = (pitch - x < 16) ? (pitch - x) : 16;
            uint8_t tmp[16];
            memcpy(tmp, top + x, chunk);
            memcpy(top + x, bot + x, chunk);
            memcpy(bot + x, tmp, chunk);
        }
        top += pitch;
        bot -= pitch;
    }
}

// ---------------------------------------------------------------------------
// Render state scratch save/restore
// ---------------------------------------------------------------------------

namespace {
struct RenderStateSnapshot {
    bool             alpha_test_enabled;
    uint32_t         alpha_ref;
    GECompareFunc    alpha_func;
    bool             specular_enabled;
    bool             color_key_enabled;
    bool             fog_enabled;
    GETextureBlend   texture_blend;
    GETextureHandle  bound_texture;
    bool             bound_texture_has_alpha;
    bool             bound_texture_has_mipmaps;
    GLboolean        gl_blend_enabled;
    GLboolean        gl_depth_test;
    GLboolean        gl_depth_mask;
};
// Only the outermost push actually snapshots. Nested push/pop pairs increment
// the depth counter so control flow like speech_bubble_text (measure pass +
// draw pass, both push) doesn't clobber the saved state.
static RenderStateSnapshot s_state_snapshot{};
static int32_t             s_state_depth = 0;
}

void ge_push_render_state()
{
    if (s_state_depth++ > 0) return;

    s_state_snapshot.alpha_test_enabled        = s_alpha_test_enabled;
    s_state_snapshot.alpha_ref                 = s_alpha_ref;
    s_state_snapshot.alpha_func                = s_alpha_func;
    s_state_snapshot.specular_enabled          = s_specular_enabled;
    s_state_snapshot.color_key_enabled         = s_color_key_enabled;
    s_state_snapshot.fog_enabled               = s_fog_enabled;
    s_state_snapshot.texture_blend             = s_texture_blend;
    s_state_snapshot.bound_texture             = s_bound_texture;
    s_state_snapshot.bound_texture_has_alpha   = s_bound_texture_has_alpha;
    s_state_snapshot.bound_texture_has_mipmaps = s_bound_texture_has_mipmaps;
    s_state_snapshot.gl_blend_enabled          = glIsEnabled(GL_BLEND);
    s_state_snapshot.gl_depth_test             = glIsEnabled(GL_DEPTH_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &s_state_snapshot.gl_depth_mask);
}

void ge_pop_render_state()
{
    ASSERT(s_state_depth > 0);
    if (--s_state_depth > 0) return;

    s_alpha_test_enabled        = s_state_snapshot.alpha_test_enabled;
    s_alpha_ref                 = s_state_snapshot.alpha_ref;
    s_alpha_func                = s_state_snapshot.alpha_func;
    s_specular_enabled          = s_state_snapshot.specular_enabled;
    s_color_key_enabled         = s_state_snapshot.color_key_enabled;
    s_fog_enabled               = s_state_snapshot.fog_enabled;
    s_texture_blend             = s_state_snapshot.texture_blend;
    s_bound_texture             = s_state_snapshot.bound_texture;
    s_bound_texture_has_alpha   = s_state_snapshot.bound_texture_has_alpha;
    s_bound_texture_has_mipmaps = s_state_snapshot.bound_texture_has_mipmaps;

    if (s_state_snapshot.gl_blend_enabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (s_state_snapshot.gl_depth_test)    glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthMask(s_state_snapshot.gl_depth_mask);
}

// ---------------------------------------------------------------------------
// User textures — ad-hoc, outside the page system
// ---------------------------------------------------------------------------

GETextureHandle ge_create_user_texture_r8_rrrr(int32_t w, int32_t h, const uint8_t* pixels)
{
    if (w <= 0 || h <= 0 || !pixels) return GE_TEXTURE_NONE;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (!tex) return GE_TEXTURE_NONE;

    // Save any texture currently bound on TEXTURE_2D so we don't disturb the
    // binding the caller (or the state cache) expects.
    GLint prev_tex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

    glBindTexture(GL_TEXTURE_2D, tex);

    // Byte-per-pixel source; defeat the default 4-byte row alignment.
    GLint prev_unpack = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Sample as (R,R,R,R): a 1-bit glyph mask modulates vertex color cleanly
    // through the existing Modulate path, with color-key discard on zeros.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);

    // Pixel-perfect mapping for bitmap fonts — nearest + clamp.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);

    glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);

    return (GETextureHandle)(uintptr_t)tex;
}

void ge_blit_back_buffer()
{
    // Legacy D3D "non-flipping present" path. In GL both flip and blit
    // reduce to the same thing — run composition into the default FB,
    // then swap. Main game loop (game.cpp:681) uses this, not ge_flip.
    int win_w = 0, win_h = 0;
    sdl3_window_get_drawable_size(&win_w, &win_h);
    composition_bind_default();
    composition_blit(win_w, win_h);

    // Default FB is bound and contains the composed scene. UI pass draws
    // on top at native resolution, untouched by the composition shader.
    if (s_post_composition_callback) s_post_composition_callback();

    present_and_swap(win_w, win_h);

    // Re-bind scene FBO for the next frame — see OpenDisplay for rationale.
    composition_bind_scene();
}

// ---------------------------------------------------------------------------
// Device capabilities
// ---------------------------------------------------------------------------

bool ge_supports_dest_inv_src_color() { return true; }
bool ge_supports_modulate_alpha()     { return true; }
bool ge_supports_adami_lighting()     { return true; }
bool ge_is_hardware()                 { return true; }
bool ge_is_fullscreen()               { return s_fullscreen; }

// ---------------------------------------------------------------------------
// Background surface — fullscreen background image (loading screen, menus)
// ---------------------------------------------------------------------------

static GLuint s_bg_texture = 0;
static uint8_t* s_bg_pixels = nullptr;
static GLuint gl_create_bgr_texture(uint8_t* bgr_pixels);  // forward decl

void ge_create_background_surface(uint8_t* pixels)
{
    if (s_bg_texture) { glDeleteTextures(1, &s_bg_texture); s_bg_texture = 0; }
    if (!pixels) return;
    s_bg_pixels = pixels;
    s_bg_texture = gl_create_bgr_texture(pixels);
}

// Helper: draw a textured quad covering [x..x+w] x [y..y+h] in screen pixels.
static void gl_blit_textured_quad(GLuint tex, float x, float y, float w, float h)
{
    if (!tex) return;

    // Offset by +0.5 to compensate the -0.5 D3D6 pixel center adjustment in
    // tl_vert.glsl — without this the quad lands at -0.5..w-0.5, leaving a
    // 1-pixel undrawn strip on the right and bottom edges.
    const float ox = 0.5f, oy = 0.5f;

    GEVertexTL verts[4];
    verts[0].x = x + ox;     verts[0].y = y + oy;     verts[0].z = 0.5f; verts[0].rhw = 1.0f;
    verts[0].color = 0xFFFFFFFF; verts[0].specular = 0xFF000000;
    verts[0].u = 0.0f; verts[0].v = 0.0f;
    verts[1].x = x + w + ox; verts[1].y = y + oy;     verts[1].z = 0.5f; verts[1].rhw = 1.0f;
    verts[1].color = 0xFFFFFFFF; verts[1].specular = 0xFF000000;
    verts[1].u = 1.0f; verts[1].v = 0.0f;
    verts[2].x = x + ox;     verts[2].y = y + h + oy; verts[2].z = 0.5f; verts[2].rhw = 1.0f;
    verts[2].color = 0xFFFFFFFF; verts[2].specular = 0xFF000000;
    verts[2].u = 0.0f; verts[2].v = 1.0f;
    verts[3].x = x + w + ox; verts[3].y = y + h + oy; verts[3].z = 0.5f; verts[3].rhw = 1.0f;
    verts[3].color = 0xFFFFFFFF; verts[3].specular = 0xFF000000;
    verts[3].u = 1.0f; verts[3].v = 1.0f;

    uint16_t indices[6] = { 0, 1, 2, 2, 1, 3 };

    // Ensure opaque draw — disable blending that may be left over from game rendering.
    ge_set_blend_enabled(false);
    ge_set_alpha_test_enabled(false);
    ge_set_depth_mode(GEDepthMode::Off);

    // Bind texture directly (bypass ge_bind_texture which uses the game texture pool).
    s_bound_texture = (GETextureHandle)(uintptr_t)tex;
    s_bound_texture_has_alpha = false;
    s_bound_texture_has_mipmaps = false;
    s_texture_blend = GETextureBlend::Decal;

    ge_draw_indexed_primitive(GEPrimitiveType::TriangleList, verts, 4, indices, 6);
}

// Helper: draw a fullscreen textured quad with the given GL texture ID.
static void gl_blit_fullscreen_texture(GLuint tex)
{
    gl_blit_textured_quad(tex, 0.0f, 0.0f, (float)s_vp_w, (float)s_vp_h);
}

void ge_blit_background_surface()
{
    // Override takes priority (frontend theme surfaces).
    GLuint tex = (s_background_override != GE_SCREEN_SURFACE_NONE)
        ? (GLuint)(uintptr_t)s_background_override
        : s_bg_texture;
    if (!tex) return;

    // Defensive: keep ui_coords in sync with the actual render target
    // size on every blit. Covers the case where the mode-change callback
    // hasn't run yet, or the window / FBO resized without a callback.
    ui_coords::recompute(gl_context_get_width(), gl_context_get_height());

    // If the GL context isn't fully sized yet, skip the blit entirely.
    if (ui_coords::g_screen_w_px <= 0.0f || ui_coords::g_screen_h_px <= 0.0f) return;

    // The TL vertex shader maps pixel coords -> NDC through u_viewport,
    // which tracks the most-recent ge_set_viewport call. Game init code
    // commonly leaves it at 640x480 (the virtual game viewport) before
    // we get here, which would shrink the quad to a 640x480 patch in
    // the corner. Snapshot, expand to the full render target for the
    // blit, then restore so the next draw still maps as the caller
    // expected.
    int32_t saved_vx = s_vp_x, saved_vy = s_vp_y, saved_vw = s_vp_w, saved_vh = s_vp_h;
    ge_set_viewport(0, 0, gl_context_get_width(), gl_context_get_height());

    // Background images are 640x480 BMPs — always render into the 4:3
    // framed region with black bars around (independent of any active
    // UI scope, since this is also called from non-UI loaders like
    // ATTRACT_loadscreen_init). Clear first so the bars are clean
    // rather than carrying garbage from the previous frame.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    GLboolean scissor_was_enabled = GL_FALSE;
    glGetBooleanv(GL_SCISSOR_TEST, &scissor_was_enabled);
    if (scissor_was_enabled) glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    if (scissor_was_enabled) glEnable(GL_SCISSOR_TEST);

    gl_blit_textured_quad(tex,
                          ui_coords::g_screen_w_px * 0.5f - ui_coords::g_frame_w_px * 0.5f,
                          ui_coords::g_screen_h_px * 0.5f - ui_coords::g_frame_h_px * 0.5f,
                          ui_coords::g_frame_w_px,
                          ui_coords::g_frame_h_px);

    ge_set_viewport(saved_vx, saved_vy, saved_vw, saved_vh);
}

void ge_destroy_background_surface()
{
    if (s_bg_texture) { glDeleteTextures(1, &s_bg_texture); s_bg_texture = 0; }
    s_bg_pixels = nullptr;
}

void ge_init_back_image(const char* name)
{
    char fname[256];
    sprintf(fname, "%sdata/%s", s_data_dir, name);

    if (!s_bg_pixels) {
        s_bg_pixels = (uint8_t*)MemAlloc(640 * 480 * 3);
    }
    if (!s_bg_pixels) return;

    // Load 640x480x24 BMP: skip 18-byte header, read rows bottom-up (BMP convention).
    MFFileHandle f = FileOpen((CBYTE*)fname);
    if (f == FILE_OPEN_ERROR) return;
    FileSeek(f, SEEK_MODE_BEGINNING, 18);
    uint8_t* row = s_bg_pixels + (640 * 479 * 3);
    for (int y = 480; y > 0; y--, row -= (640 * 3)) {
        FileRead(f, row, 640 * 3);
    }
    FileClose(f);

    ge_create_background_surface(s_bg_pixels);
}

void ge_show_back_image(bool)
{
    ge_blit_background_surface();
}

void ge_reset_back_image()
{
    ge_destroy_background_surface();
    if (s_bg_pixels) {
        MemFree(s_bg_pixels);
        s_bg_pixels = nullptr;
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Video rendering
// ---------------------------------------------------------------------------

// Shader sources for video fullscreen quad (embedded from .glsl files).
// SHADER_VIDEO_VERT and SHADER_VIDEO_FRAG are defined in gl_shaders_embedded.h.

static GLuint s_vid_program = 0;
static GLuint s_vid_vao = 0;
static GLuint s_vid_vbo = 0;

static void vid_ensure_resources()
{
    if (s_vid_program) return;

    // Compile shader
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { glDeleteShader(s); return 0; }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, SHADER_VIDEO_VERT);
    GLuint fs = compile(GL_FRAGMENT_SHADER, SHADER_VIDEO_FRAG);
    if (!vs || !fs) return;

    s_vid_program = glCreateProgram();
    glAttachShader(s_vid_program, vs);
    glAttachShader(s_vid_program, fs);
    glLinkProgram(s_vid_program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // VAO/VBO
    glGenVertexArrays(1, &s_vid_vao);
    glGenBuffers(1, &s_vid_vbo);
    glBindVertexArray(s_vid_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vid_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

GEVideoTexture ge_video_texture_create(int width, int height)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    return (GEVideoTexture)tex;
}

void ge_video_texture_upload(GEVideoTexture tex, int width, int height,
                             const uint8_t* rgb_data, int row_stride)
{
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row_stride / 3);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                    GL_RGB, GL_UNSIGNED_BYTE, rgb_data);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void ge_video_draw_and_swap(GEVideoTexture tex, int video_w, int video_h)
{
    vid_ensure_resources();
    if (!s_vid_program) return;

    // Render the video into the scene FBO, exactly like every other game
    // draw. The composition pass at frame end then aspect-fits the FBO into
    // the real backbuffer and paints outer bars — video inherits that for
    // free. The scene FBO is assumed bound (invariant maintained across
    // frames); no explicit bind needed here.
    const int fbo_w = gl_context_get_width();
    const int fbo_h = gl_context_get_height();
    glViewport(0, 0, fbo_w, fbo_h);

    // Snapshot+disable scissor: a UI scope above us may have set it tight
    // around the framed UI region, which would crop the video into a small
    // square. Restore on the way out so the next UI frame still clips.
    GLboolean scissor_was_enabled = GL_FALSE;
    GLint saved_scissor[4] = { 0, 0, 0, 0 };
    glGetBooleanv(GL_SCISSOR_TEST, &scissor_was_enabled);
    glGetIntegerv(GL_SCISSOR_BOX, saved_scissor);
    glDisable(GL_SCISSOR_TEST);

    // Aspect-fit the video into the framed 4:3 region inside the FBO (NOT
    // the full FBO). Keeps videos stylistically consistent with the framed
    // UI (menus, loading screens) — a 4:3 video gets pillarbox bars
    // exactly matching the UI, and a wider video gets letterbox inside
    // the framed region. ui_coords::g_frame_* is recomputed defensively
    // in case the mode-change callback hasn't fired yet.
    ui_coords::recompute(fbo_w, fbo_h);
    const float frame_w = (ui_coords::g_frame_w_px > 0.0f)
        ? ui_coords::g_frame_w_px : (float)fbo_w;
    const float frame_h = (ui_coords::g_frame_h_px > 0.0f)
        ? ui_coords::g_frame_h_px : (float)fbo_h;
    const float frame_w_ndc = frame_w / (float)fbo_w;  // half-extent of framed region in NDC X
    const float frame_h_ndc = frame_h / (float)fbo_h;
    const float va = (float)video_w / (float)video_h;
    const float fa = frame_w / frame_h;
    const float aspect_sx = (va > fa) ? 1.0f : va / fa;
    const float aspect_sy = (va > fa) ? fa / va : 1.0f;
    const float sx = aspect_sx * frame_w_ndc;
    const float sy = aspect_sy * frame_h_ndc;

    float verts[] = {
        -sx, -sy, 0.0f, 1.0f,
         sx, -sy, 1.0f, 1.0f,
        -sx,  sy, 0.0f, 0.0f,
         sx,  sy, 1.0f, 0.0f,
    };
    glBindBuffer(GL_ARRAY_BUFFER, s_vid_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);

    // Render
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(s_vid_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)tex);
    glUniform1i(glGetUniformLocation(s_vid_program, "uTexture"), 0);

    glBindVertexArray(s_vid_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Present through composition — same path as ge_flip. Composition
    // handles aspect-fit of the FBO into the real backbuffer and paints
    // outer bars; video automatically gets that treatment.
    int win_w = 0, win_h = 0;
    sdl3_window_get_drawable_size(&win_w, &win_h);
    composition_bind_default();
    composition_blit(win_w, win_h);
    crt_effect_apply();

    present_and_swap(win_w, win_h);

    // Restore scissor state captured before the video draw.
    glScissor(saved_scissor[0], saved_scissor[1], saved_scissor[2], saved_scissor[3]);
    if (scissor_was_enabled) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);

    // Re-bind scene FBO for the next frame — keeps the "scene FBO is
    // always the active draw target between presents" invariant.
    composition_bind_scene();
}

void ge_video_texture_destroy(GEVideoTexture tex)
{
    GLuint t = (GLuint)tex;
    if (t) glDeleteTextures(1, &t);
}

// ---------------------------------------------------------------------------
// Driver info
// ---------------------------------------------------------------------------

bool ge_is_primary_driver() { return true; }

// ---------------------------------------------------------------------------
// Display mode management
// ---------------------------------------------------------------------------

void ge_restore_all_surfaces()
{
    // No device-lost in OpenGL — no-op.
}

void ge_to_gdi() {}
void ge_from_gdi() {}
bool ge_is_display_changed() { return false; }
void ge_clear_display_changed() {}

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
// Surface blitting
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Screen surfaces — GL textures used as background images (frontend themes)
// ---------------------------------------------------------------------------

// Helper: create GL texture from 640x480 BGR24 pixel data.
static GLuint gl_create_bgr_texture(uint8_t* bgr_pixels)
{
    if (!bgr_pixels) return 0;

    uint8_t* rgba = (uint8_t*)MemAlloc(640 * 480 * 4);
    if (!rgba) return 0;
    for (int i = 0; i < 640 * 480; i++) {
        rgba[i * 4 + 0] = bgr_pixels[i * 3 + 2]; // R
        rgba[i * 4 + 1] = bgr_pixels[i * 3 + 1]; // G
        rgba[i * 4 + 2] = bgr_pixels[i * 3 + 0]; // B
        rgba[i * 4 + 3] = 255;                    // A
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 640, 480, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    MemFree(rgba);
    return tex;
}

GEScreenSurface ge_create_screen_surface(uint8_t* pixels)
{
    GLuint tex = gl_create_bgr_texture(pixels);
    if (!tex) return GE_SCREEN_SURFACE_NONE;
    return (GEScreenSurface)(uintptr_t)tex;
}

GEScreenSurface ge_load_screen_surface(const char* filename)
{
    char fname[256];
    sprintf(fname, "%sdata/%s", s_data_dir, filename);

    uint8_t* image_data = (uint8_t*)MemAlloc(640 * 480 * 3);
    if (!image_data) return GE_SCREEN_SURFACE_NONE;

    MFFileHandle image_file = FileOpen((CBYTE*)fname);
    if (image_file != FILE_OPEN_ERROR) {
        FileSeek(image_file, SEEK_MODE_BEGINNING, 18);
        uint8_t* row = image_data + (640 * 479 * 3);
        for (int h = 480; h > 0; h--, row -= (640 * 3)) {
            FileRead(image_file, row, 640 * 3);
        }
        FileClose(image_file);
    }

    GEScreenSurface surface = ge_create_screen_surface(image_data);
    MemFree(image_data);
    return surface;
}

void ge_destroy_screen_surface(GEScreenSurface surface)
{
    if (surface != GE_SCREEN_SURFACE_NONE) {
        GLuint tex = (GLuint)(uintptr_t)surface;
        glDeleteTextures(1, &tex);
    }
}

void ge_set_background_override(GEScreenSurface surface) { s_background_override = surface; }
GEScreenSurface ge_get_background_override() { return s_background_override; }

void ge_blit_surface_to_backbuffer(GEScreenSurface surface, int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (surface == GE_SCREEN_SURFACE_NONE || w <= 0 || h <= 0) return;


    GLuint tex = (GLuint)(uintptr_t)surface;

    // Input (x,y,w,h) is in framed-area pixel coordinates (origin at the
    // top-left of the 4:3 framed UI region, extending to g_frame_w_px x
    // g_frame_h_px). Map both source UVs and destination pixels through
    // the framed origin so the texture stretches across the framed area
    // and the rect lands inside it. Used by frontend transitions only.
    float fw = ui_coords::g_frame_w_px;
    float fh = ui_coords::g_frame_h_px;
    float fox = ui_coords::g_screen_w_px * 0.5f - fw * 0.5f;
    float foy = ui_coords::g_screen_h_px * 0.5f - fh * 0.5f;

    float u0 = (float)x       / fw;
    float v0 = (float)y       / fh;
    float u1 = (float)(x + w) / fw;
    float v1 = (float)(y + h) / fh;

    // Offset by +0.5 to compensate the -0.5 D3D6 pixel center adjustment in
    // tl_vert.glsl (see gl_blit_fullscreen_texture comment).
    float sx  = fox + (float)x       + 0.5f;
    float sy  = foy + (float)y       + 0.5f;
    float sx1 = fox + (float)(x + w) + 0.5f;
    float sy1 = foy + (float)(y + h) + 0.5f;

    GEVertexTL verts[4];
    verts[0].x = sx;  verts[0].y = sy;  verts[0].z = 0.5f; verts[0].rhw = 1.0f;
    verts[0].color = 0xFFFFFFFF; verts[0].specular = 0xFF000000;
    verts[0].u = u0; verts[0].v = v0;
    verts[1].x = sx1; verts[1].y = sy;  verts[1].z = 0.5f; verts[1].rhw = 1.0f;
    verts[1].color = 0xFFFFFFFF; verts[1].specular = 0xFF000000;
    verts[1].u = u1; verts[1].v = v0;
    verts[2].x = sx;  verts[2].y = sy1; verts[2].z = 0.5f; verts[2].rhw = 1.0f;
    verts[2].color = 0xFFFFFFFF; verts[2].specular = 0xFF000000;
    verts[2].u = u0; verts[2].v = v1;
    verts[3].x = sx1; verts[3].y = sy1; verts[3].z = 0.5f; verts[3].rhw = 1.0f;
    verts[3].color = 0xFFFFFFFF; verts[3].specular = 0xFF000000;
    verts[3].u = u1; verts[3].v = v1;

    uint16_t indices[6] = { 0, 1, 2, 2, 1, 3 };

    ge_set_blend_enabled(false);
    ge_set_alpha_test_enabled(false);
    ge_set_depth_mode(GEDepthMode::Off);

    s_bound_texture = (GETextureHandle)(uintptr_t)tex;
    s_bound_texture_has_alpha = false;
    s_bound_texture_has_mipmaps = false;
    s_texture_blend = GETextureBlend::Decal;

    ge_draw_indexed_primitive(GEPrimitiveType::TriangleList, verts, 4, indices, 6);
}

// ---------------------------------------------------------------------------
// Vertex buffer pool — CPU-side implementation for OpenGL
// ---------------------------------------------------------------------------
// D3D backend uses IDirect3DVertexBuffer objects in a pool.
// OpenGL backend: simple CPU malloc buffers. polypage.cpp writes vertices
// via pointer, then calls prepare + draw. We upload to VBO at draw time.

struct GLVertexBuf {
    uint32_t magic;      // canary: 0xVBUF1234
    uint8_t* data;       // CPU vertex data (GEVertexTL layout, 32 bytes per vertex)
    uint32_t capacity;   // in bytes
    uint32_t logsize;    // log2 of vertex count
    bool     in_use;     // allocated and not yet released
    GLuint   vbo;        // Per-slot VBO (avoids hammering one shared VBO with glBufferData)
    GLuint   ebo;        // Per-slot EBO
    GLuint   vao;        // Per-slot VAO (bound to per-slot VBO/EBO)
    uint32_t vbo_size;   // Current VBO allocation size (bytes) — avoid needless realloc
};
static constexpr uint32_t GLVB_MAGIC = 0xAB123456;

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

    best->magic = GLVB_MAGIC;
    best->in_use = true;
    best->logsize = logsize;

    // Create per-slot VBO/EBO/VAO on first use.
    if (!best->vbo) {
        glGenBuffers(1, &best->vbo);
        glGenBuffers(1, &best->ebo);
        glGenVertexArrays(1, &best->vao);
        setup_vao_tl(best->vao, best->vbo, best->ebo);
        best->vbo_size = 0;
    }

    uint32_t vertex_count = 1u << logsize;
    uint32_t needed = vertex_count * 32; // sizeof(GEVertexTL) = 32

    if (!best->data || best->capacity < needed) {
        // Don't free old data — it may still be referenced by DrawSinglePoly
        // (via m_VB cached during AddToBuckets, before reclaim releases the slot).
        best->data = (uint8_t*)calloc(1, needed);
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

    // If the buffer is already large enough (reused slot from a previous larger
    // allocation), just bump logsize — no reallocation needed.
    if (buf->data && buf->capacity >= needed) {
        buf->logsize = new_logsize;
        if (out_ptr) *out_ptr = buf->data;
        if (out_logsize) *out_logsize = new_logsize;
        return buf;
    }

    // Use malloc + memcpy instead of realloc to avoid invalidating old pointer.
    // Old data stays valid (for any code that cached the pointer) until the
    // slot is reused in a later frame.
    uint8_t* new_data = (uint8_t*)calloc(1, needed);
    if (!new_data) return vb; // keep old on failure

    // Don't free old data — it may still be cached by DrawSinglePoly.
    uint32_t old_size = buf->capacity;
    if (buf->data && old_size > 0) {
        memcpy(new_data, buf->data, old_size);
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
    ASSERT(buf->magic == GLVB_MAGIC);
    ASSERT(buf->data);

    // Bind program once — skip if already bound.
    if (s_cached_program != s_program_tl) {
        glUseProgram(s_program_tl);
        s_cached_program = s_program_tl;
        s_uniforms_ever_uploaded = false; // force re-upload after program switch
    }

    // Upload uniforms only if state changed.
    set_frag_uniforms(
        s_tl_u_has_texture, s_tl_u_texture, s_tl_u_texture_blend,
        s_tl_u_alpha_test_enabled, s_tl_u_alpha_ref, s_tl_u_alpha_func,
        s_tl_u_fog_enabled, s_tl_u_fog_color, s_tl_u_fog_near, s_tl_u_fog_far,
        s_tl_u_specular_enabled, s_tl_u_color_key_enabled, s_tl_u_tex_has_alpha,
        s_tl_u_farfacet_mode);

    // Use buffer's allocated capacity — avoids scanning all indices for max.
    uint32_t vert_count = 1u << buf->logsize;
    uint32_t upload_size = vert_count * 32;

    glBindVertexArray(buf->vao);

    glBindBuffer(GL_ARRAY_BUFFER, buf->vbo);
    if (buf->vbo_size < upload_size) {
        glBufferData(GL_ARRAY_BUFFER, upload_size, nullptr, GL_STREAM_DRAW);
        buf->vbo_size = upload_size;
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, upload_size, buf->data);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint16_t), indices, GL_STREAM_DRAW);

    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr);


    // No unbind — next draw will rebind its own VAO.

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

void ge_texture_set_no_mipmaps(int32_t page)
{
    if (page < 0 || page >= GL_TEX_MAX) return;
    s_textures[page].no_mipmaps = true;
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

// Dead in DEAD_CODE_REPORT builds (all callers are dead); stubs needed for
// regular builds where dead callers share a .o with live code.
void ge_texture_free_all() {}
void ge_texture_change_tga(int32_t, const char*) {}
void ge_texture_set_greyscale(int32_t, bool) {}
int32_t ge_texture_get_size(int32_t) { return 0; }
Font* ge_get_font(int32_t, int32_t) { return nullptr; }
int32_t ge_get_screen_bpp() { return 32; }
void ge_enumerate_drivers(GEDriverEnumCallback, void*) {}

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
// NTSC / Driver enumeration
// ---------------------------------------------------------------------------

bool ge_is_ntsc() { return false; }

// ===========================================================================
// GERenderState — same as game_graphics_engine.cpp (shared)
// ===========================================================================
// Shared implementation is in game_graphics_engine.cpp, compiled separately.

// ===========================================================================
// Display globals (expected by game code)
// ===========================================================================

SLONG ScreenWidth = 640;
SLONG ScreenHeight = 480;
SLONG DisplayBPP = 32;

// ---------------------------------------------------------------------------
// Display functions (called from game.cpp)
// ---------------------------------------------------------------------------

// Pure function: physical window size → scene-FBO size. Applies the
// OC_RENDER_SCALE factor and clamps the resulting aspect into
// [FOV_MIN_ASPECT, FOV_CAP_ASPECT]. Outputs also expose the
// intermediate "scaled" values and whether a clamp fired — used by
// OpenDisplay for one-time logging and nothing else.
static void compute_fbo_size(int phys_w, int phys_h,
                             int* out_fbo_w, int* out_fbo_h,
                             int* out_scaled_w, int* out_scaled_h,
                             float* out_scale, bool* out_clamped)
{
    // Hard floor on render scale: below this the scene becomes
    // unreadable and GL driver texture-size math gets wobbly on some
    // platforms. Internal limit, not user-tuneable.
    static constexpr float RENDER_SCALE_MIN = 0.25f;

    float scale = OC_CONFIG_get_float("video", "render_scale", 1.0f);
    if (scale < RENDER_SCALE_MIN) scale = RENDER_SCALE_MIN;
    if (scale > 1.0f)             scale = 1.0f;

    int scaled_w = (int)((float)phys_w * scale + 0.5f);
    int scaled_h = (int)((float)phys_h * scale + 0.5f);
    if (scaled_w < 1) scaled_w = 1;
    if (scaled_h < 1) scaled_h = 1;

    const float a = (float)scaled_w / (float)scaled_h;
    int fbo_w, fbo_h;
    bool clamped = false;
    if (a > FOV_CAP_ASPECT) {
        fbo_h = scaled_h;
        fbo_w = (int)((float)scaled_h * FOV_CAP_ASPECT + 0.5f);
        clamped = true;
    } else if (a < FOV_MIN_ASPECT) {
        fbo_w = scaled_w;
        fbo_h = (int)((float)scaled_w / FOV_MIN_ASPECT + 0.5f);
        clamped = true;
    } else {
        fbo_w = scaled_w;
        fbo_h = scaled_h;
    }
    if (fbo_w < 1) fbo_w = 1;
    if (fbo_h < 1) fbo_h = 1;

    *out_fbo_w    = fbo_w;
    *out_fbo_h    = fbo_h;
    *out_scaled_w = scaled_w;
    *out_scaled_h = scaled_h;
    *out_scale    = scale;
    *out_clamped  = clamped;
}

// Apply an already-computed FBO size: reallocate scene attachments,
// publish the new dimensions to all downstream consumers (ScreenWidth/
// Height, gl_context cached size), rebind the scene FBO as the default
// draw target, and notify the game via the mode-change callback so
// PolyPage::SetScaling + ui_coords::recompute refresh in lockstep.
//
// Shared tail of OpenDisplay (first init) and ge_resize_display (runtime
// window resize). Returns false if the GPU allocation fails.
static bool apply_fbo_size(int fbo_w, int fbo_h)
{
    if (!composition_resize(fbo_w, fbo_h)) return false;

    ScreenWidth  = fbo_w;
    ScreenHeight = fbo_h;
    gl_context_set_size(fbo_w, fbo_h);
    composition_bind_scene();

    if (s_mode_change_callback) {
        s_mode_change_callback(fbo_w, fbo_h);
    }
    return true;
}

SLONG OpenDisplay(ULONG width, ULONG height, ULONG depth, ULONG flags)
{
    // Window was already created with the requested mode/size by SetupHost
    // (driven by config.h). Query the actual drawable size — in fullscreen
    // that's the monitor's native resolution, in windowed it's OC_WINDOWED_*,
    // in either case it also accounts for HiDPI. Incoming args are ignored
    // during the transition away from the dead INI-based path; they will
    // disappear once the call site is updated.
    (void)width; (void)height; (void)depth; (void)flags;

    int phys_w = 0, phys_h = 0;
    sdl3_window_get_drawable_size(&phys_w, &phys_h);
    int logical_w = 0, logical_h = 0;
    sdl3_window_get_size(&logical_w, &logical_h);

    // Compute the scene FBO size — the render target the game treats as
    // "the screen". Two shaping steps inside compute_fbo_size:
    //   1. OC_RENDER_SCALE — scales the physical window size uniformly.
    //      1.0 = pixel-perfect, lower = cheaper/blurrier.
    //   2. Aspect clamp to [FOV_MIN_ASPECT, FOV_CAP_ASPECT] — the
    //      range the rectilinear 3D projection can render at without
    //      fish-eye (too wide) or tiny-character (too tall) artefacts.
    //      When the physical window aspect lies outside this range, the
    //      FBO is made narrower/shorter than scaled_w/h in one dimension;
    //      the composition pass paints outer pillar/letterbox bars over
    //      the leftover real-backbuffer area. The game itself never sees
    //      those bars — from its point of view the FBO *is* the screen.
    int fbo_w = 0, fbo_h = 0, scaled_w = 0, scaled_h = 0;
    float scale = 1.0f;
    bool clamped = false;
    compute_fbo_size(phys_w, phys_h,
                     &fbo_w, &fbo_h, &scaled_w, &scaled_h,
                     &scale, &clamped);
    const float scaled_aspect = (float)scaled_w / (float)scaled_h;

    DisplayBPP = 32;  // OpenGL is always 32bpp.

    // Print the resolved dimensions so it's obvious what the game is
    // actually rendering at vs. what the window presents. Useful when
    // HiDPI / render-scale / aspect-clamp math starts behaving unexpectedly.
    fprintf(stderr,
            "Display init:\n"
            "  config request:     %dx%d physical px (fullscreen=%d)\n"
            "  window (logical):   %dx%d points\n"
            "  window (physical):  %dx%d pixels  [HiDPI ratio %.2fx]\n"
            "  render scale:       %.2f\n"
            "  after scale:        %dx%d pixels  [aspect %.3f]\n"
            "  aspect clamp:       [%.3f .. %.3f]  %s\n"
            "  scene FBO (game):   %dx%d pixels  [aspect %.3f]\n",
            OC_CONFIG_get_int("video", "windowed_width", 640),
            OC_CONFIG_get_int("video", "windowed_height", 480),
            OC_CONFIG_get_int("video", "fullscreen", 0),
            logical_w, logical_h,
            phys_w, phys_h,
            (logical_w > 0) ? (float)phys_w / (float)logical_w : 0.0f,
            (double)scale,
            scaled_w, scaled_h, (double)scaled_aspect,
            (double)FOV_MIN_ASPECT, (double)FOV_CAP_ASPECT,
            clamped ? "APPLIED (outer bars)" : "within range",
            fbo_w, fbo_h, (double)((float)fbo_w / (float)fbo_h));

    s_fullscreen = OC_CONFIG_get_int("video", "fullscreen", 0) != 0;

    // gl_context_get_width/height must return the render-target size
    // (wibble effect and ge_set_viewport's y-flip both rely on it).
    if (!gl_context_create(fbo_w, fbo_h, OC_CONFIG_get_int("video", "vsync", 1) != 0)) {
        return -1;
    }

    if (!composition_init(fbo_w, fbo_h)) {
        fprintf(stderr, "OpenDisplay: composition_init failed\n");
        return -1;
    }

    // Wipe the freshly-allocated scene FBO so the first present can't briefly
    // flicker uninitialized GPU memory in the bars around the framed UI
    // (e.g. before the intro video starts, ATTRACT_loadscreen_init blits a
    // 4:3 background and AENG_flip presents — without this clear, the area
    // outside the framed region carried garbage on the first frame).
    composition_bind_scene();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Publish dimensions + fire mode-change callback. composition_resize
    // inside apply_fbo_size is a no-op here (same size that composition_init
    // just used); the call exists to share the "tail" with ge_resize_display.
    if (!apply_fbo_size(fbo_w, fbo_h)) {
        fprintf(stderr, "OpenDisplay: apply_fbo_size failed\n");
        return -1;
    }

    return 0; // success (D3D backend returns HRESULT, 0 = S_OK)
}

// Live repaint during window-edge drag. The SDL event watcher fires
// this synchronously while Windows is in its modal resize loop (main
// game loop is frozen at that point); without it, the OS reveals each
// new strip of client area filled with black and never repaints until
// the user releases the mouse.
//
// Single uniform path for all game modes (gameplay, menu, video,
// outro): non-uniform stretch of the OLD scene FBO into the dst rect
// the POST-resize composition would produce. The window aspect is
// clamped into [FOV_MIN_ASPECT, FOV_CAP_ASPECT] and that clamped
// target is aspect-fit into the window. Inside the range → dst fills
// window, FBO stretches freely. Outside → centred outer pillar/letter-
// box bars (the same bars the post-resize composition_blit will draw).
//
// Drag-time render = stretched re-present of the LAST captured real
// frame. Architecturally cleaner than re-running the rendering pipeline
// without a fresh game tick: timer queues, dirty-bit-driven UI, etc.
// would all see stale / consumed state and individual UI elements would
// flicker or vanish until the drag ends. Replaying the last frame is
// universally correct — whatever the user saw a moment ago, they keep
// seeing, smoothly rescaled. As soon as drag releases, the regular
// game-loop flip resumes and re-renders fresh content at the new
// resolution.
void ge_present_for_drag()
{
    // Skip if GL / composition isn't up yet (very early window events
    // before OpenDisplay).
    if (composition_scene_width() <= 0 || composition_scene_height() <= 0) return;

    int win_w = 0, win_h = 0;
    sdl3_window_get_drawable_size(&win_w, &win_h);
    if (win_w <= 0 || win_h <= 0) return;

    composition_present_last_frame(win_w, win_h);
    gl_context_swap();

    // Restore the scene-FBO-as-default-target invariant — see OpenDisplay
    // for why the engine keeps it bound between presents.
    composition_bind_scene();
}

// Public entry point for runtime window resize. Called from the host
// event loop (host.cpp) after the OS reports a window size change. Safe
// no-op if the FBO is already at the current drawable size.
void ge_resize_display()
{
    int phys_w = 0, phys_h = 0;
    sdl3_window_get_drawable_size(&phys_w, &phys_h);
    if (phys_w <= 0 || phys_h <= 0) return;

    int fbo_w = 0, fbo_h = 0, scaled_w = 0, scaled_h = 0;
    float scale = 1.0f;
    bool clamped = false;
    compute_fbo_size(phys_w, phys_h,
                     &fbo_w, &fbo_h, &scaled_w, &scaled_h,
                     &scale, &clamped);

    if (!apply_fbo_size(fbo_w, fbo_h)) {
        fprintf(stderr, "ge_resize_display: apply_fbo_size failed (%dx%d)\n",
                fbo_w, fbo_h);
    }
}

SLONG CloseDisplay()
{
    composition_shutdown();
    gl_context_destroy();
    return 1;
}

void SetLastClumpfile(char*, size_t) {}
