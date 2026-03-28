#ifndef ENGINE_GRAPHICS_GRAPHICS_ENGINE_H
#define ENGINE_GRAPHICS_GRAPHICS_ENGINE_H

// Graphics engine abstraction layer.
// Game code includes ONLY this header — never D3D/DDraw/OpenGL headers directly.
// Implementation lives in graphics_engine_d3d.cpp (or graphics_engine_opengl.cpp).

#include <cstdint>

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

// Opaque handle to a GPU texture. 0 = no texture.
using GETextureHandle = uint32_t;
constexpr GETextureHandle GE_TEXTURE_NONE = 0;

// Texture type constants.
constexpr int32_t GE_TEXTURE_TYPE_UNUSED = 0;
constexpr int32_t GE_TEXTURE_TYPE_TGA = 1;
constexpr int32_t GE_TEXTURE_TYPE_USER = 2;

enum class GEBlendMode {
    Opaque,         // no blending
    Alpha,          // src*srcA + dst*(1-srcA)
    Additive,       // src*1 + dst*1
    Modulate,       // src*dst
    InvModulate,    // src*(1-dst)
};

// Individual blend factors for custom src/dst combinations.
enum class GEBlendFactor {
    Zero,
    One,
    SrcAlpha,
    InvSrcAlpha,
    SrcColor,
    InvSrcColor,
    DstColor,
    InvDstColor,
    DstAlpha,
    InvDstAlpha,
};

enum class GEDepthMode {
    Off,            // no depth test, no depth write
    ReadOnly,       // depth test on, depth write off
    WriteOnly,      // depth test off, depth write on
    ReadWrite,      // depth test on, depth write on (default)
};

enum class GECullMode {
    None,
    CW,
    CCW,            // default
};

enum class GETextureFilter {
    Nearest,
    Linear,
};

enum class GETextureBlend {
    Modulate,       // vertex color * texture
    ModulateAlpha,  // vertex color * texture, with alpha
    Decal,          // texture only, ignore vertex color
};

enum class GETextureAddress {
    Wrap,
    Clamp,
};

enum class GECompareFunc {
    Always,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    NotEqual,
};

enum class GEPrimitiveType {
    TriangleList,
    TriangleFan,
};

// 3D vector. Replaces D3DVECTOR.
struct GEVector {
    float x, y, z;
};

// Pre-transformed, pre-lit vertex (screen space). Replaces D3DTLVERTEX.
// Union aliases (dvSX, dcColor, etc.) provided for compatibility with legacy code.
struct GEVertexTL {
    union { float x;  float sx;  float dvSX; };
    union { float y;  float sy;  float dvSY; };
    union { float z;  float sz;  float dvSZ; };
    union { float rhw; float dvRHW; };
    union { uint32_t color; uint32_t dcColor; };
    union { uint32_t specular; uint32_t dcSpecular; };
    union { float u;  float tu;  float dvTU; };
    union { float v;  float tv;  float dvTV; };
};

// Lit vertex (world space, pre-lit). Replaces D3DLVERTEX.
// Union aliases (dvX, dcColor, etc.) provided for compatibility with legacy code.
struct GEVertexLit {
    union { float x;  float dvX; };
    union { float y;  float dvY; };
    union { float z;  float dvZ; };
    union { uint32_t _reserved; uint32_t dwReserved; }; // padding to match D3DLVERTEX layout
    union { uint32_t color; uint32_t dcColor; };
    union { uint32_t specular; uint32_t dcSpecular; };
    union { float u;  float tu;  float dvTU; };
    union { float v;  float tv;  float dvTV; };
};

// Unlit vertex (world space). Replaces D3DVERTEX.
// Union aliases (dvX, dvNX, etc.) provided for compatibility with legacy code.
struct GEVertex {
    union { float x;  float dvX; };
    union { float y;  float dvY; };
    union { float z;  float dvZ; };
    union { float nx; float dvNX; };
    union { float ny; float dvNY; };
    union { float nz; float dvNZ; };
    union { float u;  float dvTU; };
    union { float v;  float dvTV; };
};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Called once at startup after window creation.
void ge_init();

// Called once at shutdown.
void ge_shutdown();

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void ge_begin_scene();
void ge_end_scene();
void ge_clear(bool color, bool depth);
void ge_flip();

// ---------------------------------------------------------------------------
// Render state
// ---------------------------------------------------------------------------

void ge_set_blend_mode(GEBlendMode mode);
void ge_set_blend_factors(GEBlendFactor src, GEBlendFactor dst);
void ge_set_blend_enabled(bool enabled);
void ge_set_depth_mode(GEDepthMode mode);
void ge_set_depth_func(GECompareFunc func);
void ge_set_cull_mode(GECullMode mode);
void ge_set_texture_filter(GETextureFilter mag, GETextureFilter min);
void ge_set_texture_blend(GETextureBlend mode);
void ge_set_texture_address(GETextureAddress mode);
void ge_set_depth_bias(int32_t bias);
void ge_set_fog_enabled(bool enabled);
void ge_set_fog_params(uint32_t color, float near_dist, float far_dist);
void ge_set_specular_enabled(bool enabled);
void ge_set_perspective_correction(bool enabled);

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

void ge_bind_texture(GETextureHandle tex);

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void ge_draw_primitive(GEPrimitiveType type, const GEVertexTL* verts, uint32_t count);
void ge_draw_indexed_primitive(GEPrimitiveType type, const GEVertexTL* verts, uint32_t vert_count,
                               const uint16_t* indices, uint32_t index_count);

// Overloads for lit vertices (world-space, pre-lit).
void ge_draw_indexed_primitive_lit(GEPrimitiveType type, const GEVertexLit* verts, uint32_t vert_count,
                                   const uint16_t* indices, uint32_t index_count);

// Overloads for unlit vertices (world-space, requires transformation).
void ge_draw_indexed_primitive_unlit(GEPrimitiveType type, const GEVertex* verts, uint32_t vert_count,
                                     const uint16_t* indices, uint32_t index_count);

// ---------------------------------------------------------------------------
// Transforms
// ---------------------------------------------------------------------------

// 4x4 matrix in row-major order (same layout as D3DMATRIX).
// Named member aliases (_11.._44) provided for compatibility with legacy code.
struct GEMatrix {
    union {
        float m[4][4];
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
    };
};

enum class GETransform {
    World,
    View,
    Projection,
};

void ge_set_transform(GETransform type, const GEMatrix* matrix);

// ---------------------------------------------------------------------------
// Viewport
// ---------------------------------------------------------------------------

// Viewport descriptor. Replaces D3DVIEWPORT2.
// Named member aliases (dwX, dwWidth, etc.) for legacy compatibility.
struct GEViewport {
    union { int32_t x; uint32_t dwX; };
    union { int32_t y; uint32_t dwY; };
    union { int32_t width; uint32_t dwWidth; };
    union { int32_t height; uint32_t dwHeight; };
    float dvClipX, dvClipY, dvClipWidth, dvClipHeight;
    float dvMinZ, dvMaxZ;
    uint32_t dwSize; // legacy: sizeof(D3DVIEWPORT2), kept for compat
};

void ge_set_viewport(int32_t x, int32_t y, int32_t w, int32_t h);

// ---------------------------------------------------------------------------
// Background
// ---------------------------------------------------------------------------

enum class GEBackground { Black, White, User };
void ge_set_background(GEBackground bg);
void ge_set_background_color(uint8_t r, uint8_t g, uint8_t b);

// ---------------------------------------------------------------------------
// Screen buffer access
// ---------------------------------------------------------------------------

// Lock the back buffer for direct pixel access. Returns pointer to pixel data, or NULL on failure.
void* ge_lock_screen();
void  ge_unlock_screen();

// Screen buffer state — valid after lock, or always valid for dimensions.
uint8_t* ge_get_screen_buffer();
int32_t  ge_get_screen_pitch();
int32_t  ge_get_screen_width();
int32_t  ge_get_screen_height();
int32_t  ge_get_screen_bpp();

// Direct pixel access — only valid when screen is locked.
void ge_plot_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b);
void ge_plot_formatted_pixel(int32_t x, int32_t y, uint32_t color);
void ge_get_pixel(int32_t x, int32_t y, uint8_t* r, uint8_t* g, uint8_t* b);

// Pack r,g,b into the current framebuffer pixel format.
uint32_t ge_get_formatted_pixel(uint8_t r, uint8_t g, uint8_t b);

// Blit back buffer to front.
void ge_blit_back_buffer();

// ---------------------------------------------------------------------------
// Device capabilities
// ---------------------------------------------------------------------------

bool ge_supports_dest_inv_src_color();
bool ge_supports_modulate_alpha();
bool ge_supports_adami_lighting();
bool ge_is_hardware();
bool ge_is_fullscreen();

// ---------------------------------------------------------------------------
// Gamma
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Background surface (loading screens)
// ---------------------------------------------------------------------------

void ge_create_background_surface(uint8_t* pixels);
void ge_blit_background_surface();
void ge_destroy_background_surface();

// Loading screen management.
void ge_init_back_image(const char* name);
void ge_show_back_image(bool in_3d_frame = true);
void ge_reset_back_image();

// ---------------------------------------------------------------------------
// Cutscene playback
// ---------------------------------------------------------------------------

void ge_run_cutscene(int32_t id);

// ---------------------------------------------------------------------------
// Driver info
// ---------------------------------------------------------------------------

bool ge_is_primary_driver();

// ---------------------------------------------------------------------------
// Display mode management
// ---------------------------------------------------------------------------

// Switch to GDI mode (for message boxes, etc.) and back.
void ge_to_gdi();
void ge_from_gdi();

// Device-lost recovery: restore all surfaces after focus regain.
void ge_restore_all_surfaces();

// Display configuration changed detection.
bool ge_is_display_changed();
void ge_clear_display_changed();

// Update the internal display rect from window position (called on WM_MOVE/WM_SIZE).
void ge_update_display_rect(void* hwnd, bool fullscreen);

// ---------------------------------------------------------------------------
// Texture management
// ---------------------------------------------------------------------------

void ge_remove_all_loaded_textures();

// ---------------------------------------------------------------------------
// Surface blitting
// ---------------------------------------------------------------------------

// Blit a texture page's DDraw surface to the back buffer (flame feedback effect).
void ge_blit_texture_to_backbuffer(int32_t texture_page, int32_t src_w, int32_t src_h);

// ---------------------------------------------------------------------------
// Screen surfaces (menu backgrounds)
// ---------------------------------------------------------------------------

// Opaque handle for offscreen surfaces (DDraw surfaces in D3D backend).
typedef uintptr_t GEScreenSurface;
#define GE_SCREEN_SURFACE_NONE ((GEScreenSurface)0)

// Create a screen-sized offscreen surface and copy image_data into it.
// image_data: raw BGR pixel data matching back buffer format.
GEScreenSurface ge_create_screen_surface(uint8_t* image_data);

// Load a raw 640x480 BGR image from file into a new screen surface.
GEScreenSurface ge_load_screen_surface(const char* filename);

// Release a screen surface.
void ge_destroy_screen_surface(GEScreenSurface surface);

// Restore a screen surface after device-lost (reload from source).
void ge_restore_screen_surface(GEScreenSurface surface);

// Set the active background override surface (shown behind 3D scene).
void ge_set_background_override(GEScreenSurface surface);
GEScreenSurface ge_get_background_override();

// Blit a screen surface to the back buffer (with optional subrect).
void ge_blit_surface_to_backbuffer(GEScreenSurface surface, int32_t x, int32_t y, int32_t w, int32_t h);

// ---------------------------------------------------------------------------
// Gamma
// ---------------------------------------------------------------------------

void ge_set_gamma(int32_t black, int32_t white);
void ge_get_gamma(int32_t* black, int32_t* white);
bool ge_is_gamma_available();

#endif // ENGINE_GRAPHICS_GRAPHICS_ENGINE_H
