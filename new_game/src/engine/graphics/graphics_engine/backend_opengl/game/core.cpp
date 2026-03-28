// Graphics engine — OpenGL 4.1 core profile implementation.
// Phase 1: lifecycle, clear, flip, render state, viewport.
// Phase 2+: textures, drawing, framebuffer access.

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/graphics_engine/outro_graphics_engine.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/gl_context.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
#include "engine/platform/uc_common.h"
#include "engine/io/env.h"
#include "engine/io/file.h"
#include "engine/core/memory.h"
#include <string.h>
#include <stdio.h>

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

// Current viewport.
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

// Matrices (stored for shader uniforms — Phase 2).
static GEMatrix s_world_matrix;
static GEMatrix s_view_matrix;
static GEMatrix s_projection_matrix;

// Fullscreen.
static bool s_fullscreen = false;

// Display changed flag.
static bool s_display_changed = false;

// Background override.
static GEScreenSurface s_background_override = GE_SCREEN_SURFACE_NONE;

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
}

void ge_shutdown()
{
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

void ge_set_texture_blend(GETextureBlend)
{
    // D3D6 fixed-function texture blend mode.
    // In OpenGL core, this is handled by the fragment shader.
    // TODO Phase 2: pass as uniform to shader.
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
    // Fog is handled in fragment shader — uniform will be set at draw time.
}

void ge_set_color_key_enabled(bool enabled)
{
    s_color_key_enabled = enabled;
    // In OpenGL: color key is converted to alpha=0 at texture load time.
    // Runtime toggle controls discard in fragment shader.
}

void ge_set_alpha_test_enabled(bool enabled)
{
    s_alpha_test_enabled = enabled;
    // Handled in fragment shader via discard.
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

void ge_bind_texture(GETextureHandle) {}

// ---------------------------------------------------------------------------
// Drawing — TODO Phase 2
// ---------------------------------------------------------------------------

void ge_draw_primitive(GEPrimitiveType, const GEVertexTL*, uint32_t) {}
void ge_draw_indexed_primitive(GEPrimitiveType, const GEVertexTL*, uint32_t, const uint16_t*, uint32_t) {}
void ge_draw_indexed_primitive_lit(GEPrimitiveType, const GEVertexLit*, uint32_t, const uint16_t*, uint32_t) {}
void ge_draw_indexed_primitive_unlit(GEPrimitiveType, const GEVertex*, uint32_t, const uint16_t*, uint32_t) {}

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
    // Matrices uploaded to shader uniforms at draw time.
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
// Vertex buffer pool — TODO Phase 7
// ---------------------------------------------------------------------------

void ge_reclaim_vertex_buffers() {}
void ge_dump_vpool_info(void*) {}
void* ge_vb_alloc(uint32_t, void** out_ptr, uint32_t* out_logsize) { if (out_ptr) *out_ptr = nullptr; if (out_logsize) *out_logsize = 0; return nullptr; }
void* ge_vb_expand(void*, void** out_ptr, uint32_t* out_logsize) { if (out_ptr) *out_ptr = nullptr; if (out_logsize) *out_logsize = 0; return nullptr; }
void ge_vb_release(void*) {}
void* ge_vb_get_ptr(void*) { return nullptr; }
void* ge_vb_prepare(void*) { return nullptr; }
void ge_draw_indexed_primitive_vb(void*, const uint16_t*, uint32_t) {}

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
