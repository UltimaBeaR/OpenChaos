#ifndef ENGINE_GRAPHICS_GAME_GRAPHICS_ENGINE_H
#define ENGINE_GRAPHICS_GAME_GRAPHICS_ENGINE_H

// Graphics engine abstraction layer.
// Game code includes ONLY this header — never D3D/DDraw/OpenGL headers directly.
// Implementation lives in graphics_engine_d3d.cpp (or graphics_engine_opengl.cpp).

#include <cstddef>
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

// 3D vector.
struct GEVector {
    float x, y, z;
};

// Pre-transformed, pre-lit vertex (screen space).
struct GEVertexTL {
    union { float x;  float sx; };
    union { float y;  float sy; };
    union { float z;  float sz; };
    float rhw;
    uint32_t color;
    uint32_t specular;
    union { float u;  float tu; };
    union { float v;  float tv; };
};

// Lit vertex (world space, pre-lit).
struct GEVertexLit {
    float x;
    float y;
    float z;
    uint32_t _reserved; // padding to match D3DLVERTEX layout
    uint32_t color;
    uint32_t specular;
    union { float u;  float tu; };
    union { float v;  float tv; };
};

// Unlit vertex (world space, needs lighting).
struct GEVertex {
    float x;
    float y;
    float z;
    float nx;
    float ny;
    float nz;
    union { float u;  float tu; };
    union { float v;  float tv; };
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
void ge_set_color_key_enabled(bool enabled);
void ge_set_alpha_test_enabled(bool enabled);
void ge_set_alpha_ref(uint32_t ref);
void ge_set_alpha_func(GECompareFunc func);
void ge_set_fog_params(uint32_t color, float near_dist, float far_dist);
void ge_set_specular_enabled(bool enabled);
void ge_set_perspective_correction(bool enabled);

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

void ge_bind_texture(GETextureHandle tex);
// Returns true if the currently bound texture has an alpha channel.
// Used by the shader to select correct ALPHAOP for D3DTBLEND_MODULATE.
bool ge_bound_texture_contains_alpha();
// Override the alpha flag for the currently bound texture.
// Used by outro which creates GL textures outside the game texture pool.
void ge_set_bound_texture_has_alpha(bool has_alpha);

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

// 4x4 matrix in row-major order.
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

// Viewport descriptor.
// Named member aliases (dwX, dwWidth, etc.) for legacy compatibility.
// RGBA color value (float, 0-1 range).
struct GEColorValue { float r, g, b, a; };

struct GEViewport {
    union { int32_t x; uint32_t dwX; };
    union { int32_t y; uint32_t dwY; };
    union { int32_t width; uint32_t dwWidth; };
    union { int32_t height; uint32_t dwHeight; };
    float dvClipX, dvClipY, dvClipWidth, dvClipHeight;
    float dvMinZ, dvMaxZ;
    uint32_t dwSize; // legacy, kept for compat
};

// Set viewport with screen-space clip values (for 2D / TL vertex rendering).
void ge_set_viewport(int32_t x, int32_t y, int32_t w, int32_t h);

// Set viewport with explicit clip volume (for 3D projection rendering).
void ge_set_viewport_3d(int32_t x, int32_t y, int32_t w, int32_t h,
                        float clip_x, float clip_y, float clip_w, float clip_h);

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
// Video rendering (used by engine/video/video_player.cpp)
// ---------------------------------------------------------------------------

// Opaque handle to a video frame texture.
typedef uintptr_t GEVideoTexture;

// Create a texture for video frame upload. Returns 0 on failure.
GEVideoTexture ge_video_texture_create(int width, int height);

// Upload RGB24 pixel data to the video texture.
void ge_video_texture_upload(GEVideoTexture tex, int width, int height,
                             const uint8_t* rgb_data, int row_stride);

// Draw the video texture as a fullscreen letterboxed quad and swap.
void ge_video_draw_and_swap(GEVideoTexture tex, int video_w, int video_h);

// Destroy the video texture.
void ge_video_texture_destroy(GEVideoTexture tex);

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


// ---------------------------------------------------------------------------
// Screen surfaces (menu backgrounds)
// ---------------------------------------------------------------------------

// Opaque handle for offscreen surfaces.
using GEScreenSurface = uintptr_t;
constexpr GEScreenSurface GE_SCREEN_SURFACE_NONE = 0;

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
// Vertex buffer pool
// ---------------------------------------------------------------------------

// Reclaim all vertex buffer allocations (end of frame).
void ge_reclaim_vertex_buffers();

// Dump vertex buffer pool info to a file descriptor (debug).
void ge_dump_vpool_info(void* fd);

// Allocate a vertex buffer from the pool with 2^logsize capacity.
// Returns opaque handle (NULL on failure). Writes locked vertex pointer and actual logsize.
void* ge_vb_alloc(uint32_t logsize, void** out_ptr, uint32_t* out_logsize);

// Expand a vertex buffer to the next size class. Returns new handle.
// Updates locked vertex pointer and logsize.
void* ge_vb_expand(void* vb, void** out_ptr, uint32_t* out_logsize);

// Release a vertex buffer back to the pool.
void ge_vb_release(void* vb);

// Get the locked vertex write pointer for a vertex buffer.
void* ge_vb_get_ptr(void* vb);

// Prepare a vertex buffer for rendering (unlock). Returns opaque drawable handle.
void* ge_vb_prepare(void* vb);

// Draw indexed triangles from a prepared vertex buffer.
void ge_draw_indexed_primitive_vb(void* prepared_vb, const uint16_t* indices, uint32_t index_count);

// ---------------------------------------------------------------------------
// Texture pixel access
// ---------------------------------------------------------------------------

// Lock a texture page for direct pixel write. Returns locked bitmap pointer and pitch.
bool ge_lock_texture_pixels(int32_t page, uint16_t** bitmap, int32_t* pitch);
void ge_unlock_texture_pixels(int32_t page);

// Get the pixel format masks/shifts for a texture page (for manual pixel packing).
void ge_get_texture_pixel_format(int32_t page,
    int32_t* mask_r, int32_t* mask_g, int32_t* mask_b, int32_t* mask_a,
    int32_t* shift_r, int32_t* shift_g, int32_t* shift_b, int32_t* shift_a);


// Check if a texture page is loaded.
bool ge_is_texture_loaded(int32_t page);

// Debug: paint a solid color block on the front surface (crash diagnostics).
void ge_debug_paint_block(uint32_t color);

// ---------------------------------------------------------------------------
// Gamma
// ---------------------------------------------------------------------------

void ge_set_gamma(int32_t black, int32_t white);
void ge_get_gamma(int32_t* black, int32_t* white);
bool ge_is_gamma_available();

// ---------------------------------------------------------------------------
// Font glyph metadata (extracted from font texture pages)
// ---------------------------------------------------------------------------

// Bounding rectangle of a single glyph within a font texture page.
struct GEFontChar {
    int32_t X, Y, Height, Width;
};

// A 96-character font extracted from a texture page, stored as a linked list of font sets.
struct GEFont {
    int32_t StartLine;
    GEFontChar CharSet[96];
    GEFont* NextFont;
};

// Legacy aliases — used throughout game code.
using Char = GEFontChar;
using Font = GEFont;

// Get the font at position id (0 = first) from a texture page's font list.
Font* ge_get_font(int32_t page, int32_t id);

// ---------------------------------------------------------------------------
// Texture loading lifecycle
// ---------------------------------------------------------------------------

// Signal that bulk texture loading is done — backend may release temp buffers.
void ge_texture_loading_done();

// Signal that bulk texture loading is about to begin — backend may allocate temp buffers.
void ge_texture_loading_begin();

// ---------------------------------------------------------------------------
// Texture page management
// ---------------------------------------------------------------------------

// Load a TGA file into a texture page. can_shrink: allow detail reduction.
void ge_texture_load_tga(int32_t page, const char* path, bool can_shrink = true);

// Mark a texture page to skip mipmap generation (call before texture is uploaded).
void ge_texture_set_no_mipmaps(int32_t page);

// Create a user-writable texture page of given size. If alpha_fill: fill with alpha.
void ge_texture_create_user_page(int32_t page, int32_t size, bool alpha_fill);

// Destroy a texture page and free its GPU resources.
void ge_texture_destroy(int32_t page);

// Free all texture pages and release internal GPU memory pools.
void ge_texture_free_all();

// Replace a texture page's contents from a new TGA file.
void ge_texture_change_tga(int32_t page, const char* path);

// Enable font-glyph mode on a texture page (for bitmap font rendering).
void ge_texture_font_on(int32_t page);

// Enable secondary font mode on a texture page (for LCD-style font).
void ge_texture_font2_on(int32_t page);

// Set greyscale mode on a texture page.
void ge_texture_set_greyscale(int32_t page, bool greyscale);

// Get UV offset/scale for a texture page (for sub-page addressing).
void ge_get_texture_offset(int32_t page, float* uScale, float* uOffset, float* vScale, float* vOffset);

// Get the pixel size of a texture page.
int32_t ge_texture_get_size(int32_t page);

// Get/set the texture type (GE_TEXTURE_TYPE_* constants).
int32_t ge_texture_get_type(int32_t page);
void ge_texture_set_type(int32_t page, int32_t type);

// Get the opaque texture handle for a page (for binding).
GETextureHandle ge_get_texture_handle(int32_t page);

// ---------------------------------------------------------------------------
// Callbacks (game code hooks into backend lifecycle)
// ---------------------------------------------------------------------------

// Pre-flip callback: called by the backend just before the frame flip.
// Game code registers this to run end-of-frame logic (TrueType flush, UI overlay, etc.).
using GEPreFlipCallback = void (*)();
void ge_set_pre_flip_callback(GEPreFlipCallback callback);

// Mode change callback: called after display resolution changes.
// Game code registers this to adjust scaling, viewport, etc.
using GEModeChangeCallback = void (*)(int32_t width, int32_t height);
void ge_set_mode_change_callback(GEModeChangeCallback callback);

// Polys-drawn counter callback: backend calls this to report drawn poly count.
using GEPolysDrawnCallback = void (*)(int32_t count);
void ge_set_polys_drawn_callback(GEPolysDrawnCallback callback);

// Render state reset callback: backend calls this after texture reload to reset pipeline state.
using GERenderStatesResetCallback = void (*)();
void ge_set_render_states_reset_callback(GERenderStatesResetCallback callback);

// TGA load callback: backend calls this to load texture pixels from a TGA file.
// Game code implements the actual TGA loading (format knowledge stays outside backend).
// Callee allocates out_pixels (BGRA, 4 bytes/pixel); caller frees via MemFree.
// Returns true on success.
using GETGALoadCallback = bool (*)(const char* name, uint32_t id, bool can_shrink,
                                    uint8_t** out_pixels, int32_t* out_width, int32_t* out_height,
                                    bool* out_contains_alpha);
void ge_set_tga_load_callback(GETGALoadCallback callback);

// Texture reload prepare callback: called before/after batch texture reload (device-lost recovery).
// Game code opens/closes the TGA clump archive.
// begin=true: about to reload (clump_file/clump_size provided). begin=false: reload complete.
using GETextureReloadPrepareCallback = void (*)(bool begin, const char* clump_file, size_t clump_size);
void ge_set_texture_reload_prepare_callback(GETextureReloadPrepareCallback callback);

// Set the game data root directory (backend uses for file path construction).
void ge_set_data_dir(const char* dir);

// Check if the display is NTSC mode (affects vertical position of some UI elements).
bool ge_is_ntsc();

// ---------------------------------------------------------------------------
// Display driver enumeration (video settings menu)
// ---------------------------------------------------------------------------

// Callback for ge_enumerate_drivers: called once per available driver.
// name: driver display name, is_primary: whether it's the primary adapter,
// is_current: whether it's the currently selected driver.
using GEDriverEnumCallback = void (*)(const char* name, bool is_primary, bool is_current, void* ctx);

// Enumerate available display drivers. Calls callback for each driver.
void ge_enumerate_drivers(GEDriverEnumCallback callback, void* ctx);

// ===========================================================================
// Render state cache (higher-level batching layer over ge_* functions)
// ===========================================================================

// Special per-polygon rendering effects applied during vertex massaging.
// uc_orig: SpecialEffect (fallen/DDEngine/Headers/renderstate.h)
enum GERenderEffect {
    GE_EFFECT_NONE,            // no special effect
    GE_EFFECT_ALPHA_PREMULT,   // premultiply vertex colours by alpha, set alpha to 0
    GE_EFFECT_BLACK_WITH_ALPHA,// set vertex R,G,B to (0,0,0)
    GE_EFFECT_INV_ALPHA_PREMULT, // premultiply vertex colours by 1-alpha, set alpha to 0
    GE_EFFECT_DECAL_MODE       // set vertex A,R,G,B to (255,255,255,255)
};

// Legacy aliases for SpecialEffect names used throughout the codebase.
inline constexpr GERenderEffect RS_None           = GE_EFFECT_NONE;
inline constexpr GERenderEffect RS_AlphaPremult   = GE_EFFECT_ALPHA_PREMULT;
inline constexpr GERenderEffect RS_BlackWithAlpha = GE_EFFECT_BLACK_WITH_ALPHA;
inline constexpr GERenderEffect RS_InvAlphaPremult = GE_EFFECT_INV_ALPHA_PREMULT;
inline constexpr GERenderEffect RS_DecalMode      = GE_EFFECT_DECAL_MODE;

#pragma pack(push, 4)
struct GERenderState {
    GERenderState(GETextureFilter mag = GETextureFilter::Linear,
                  GETextureFilter min = GETextureFilter::Nearest);

    // Typed setters (replace generic SetRenderState(DWORD, DWORD))
    void SetTexture(GETextureHandle texture);
    void SetTextureAddress(GETextureAddress addr);
    void SetTextureFilter(GETextureFilter mag, GETextureFilter min);
    void SetTextureMag(GETextureFilter mag);
    void SetTextureMin(GETextureFilter min);
    void SetTextureBlend(GETextureBlend blend);
    void SetBlendMode(GEBlendMode mode);
    void SetSrcBlend(GEBlendFactor src);
    void SetDstBlend(GEBlendFactor dst);
    void SetAlphaBlendEnabled(bool enabled);
    void SetDepthEnabled(bool enabled);
    void SetDepthWrite(bool enabled);
    void SetDepthFunc(GECompareFunc func);
    void SetDepthBias(int32_t bias);
    void SetFogEnabled(bool enabled);
    void SetAlphaTestEnabled(bool enabled);
    void SetColorKeyEnabled(bool enabled);
    void SetCullMode(GECullMode mode);
    void SetEffect(GERenderEffect effect);

    void SetTempTransparent();
    void ResetTempTransparent();

    GETextureHandle GetTexture() { return Texture; }
    GERenderEffect GetEffect() { return Effect; }
    bool NeedsSorting() { return !DepthWrite; }
    int32_t ZLift() { return DepthBias; }
    void WrapJustOnce() { WrapOnce = true; }
    bool IsAlphaBlendEnabled() { return AlphaBlendEnabled; }
    bool IsAdditiveBlend() { return AlphaBlendEnabled && DstBlend == GEBlendFactor::One; }

    bool IsSameRenderState(GERenderState* pRS);

    // Flush all states to ge_* (full).
    // fog_near/fog_far: pre-computed fog distances (caller computes from CurDrawDistance).
    void InitScene(uint32_t fog_colour, float fog_near, float fog_far);
    // Flush only changed states to ge_* (delta).
    void SetChanged();

    static GERenderState s_State;

private:
    GETextureHandle Texture;
    GETextureAddress TexAddress;
    GETextureFilter TexMag;
    GETextureFilter TexMin;
    GETextureBlend TexBlend;

    bool DepthEnabled;
    bool DepthWrite;
    GECompareFunc DepthFunc;
    bool AlphaTestEnabled;
    GEBlendFactor SrcBlend;
    GEBlendFactor DstBlend;
    bool AlphaBlendEnabled;
    bool FogEnabled;
    bool ColorKeyEnabled;
    GECullMode Cull;
    int32_t DepthBias;
    GERenderEffect Effect;
    bool WrapOnce;

    bool TempTransparent;
    GEBlendFactor TempSrcBlend;
    GEBlendFactor TempDstBlend;
    bool TempAlphaBlendEnabled;
    bool TempDepthWrite;
    GETextureBlend TempTexBlend;
    GERenderEffect TempEffect;
};
#pragma pack(pop)

// Legacy alias — old code uses "RenderState", new code should prefer "GERenderState".
using RenderState = GERenderState;
// Legacy alias — old code uses "SpecialEffect".
using SpecialEffect = GERenderEffect;

#endif // ENGINE_GRAPHICS_GAME_GRAPHICS_ENGINE_H
