// Stub graphics backend — no-op implementations of all ge_* and oge_* functions.
// Used as a starting point for new backend implementations (e.g. OpenGL).

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/graphics_engine/outro_graphics_engine.h"

// ========== ge_* stubs (game graphics engine) ==========

void ge_init() {}
void ge_shutdown() {}
void ge_begin_scene() {}
void ge_end_scene() {}
void ge_clear(bool, bool) {}
void ge_flip() {}

void ge_set_blend_mode(GEBlendMode) {}
void ge_set_blend_factors(GEBlendFactor, GEBlendFactor) {}
void ge_set_blend_enabled(bool) {}
void ge_set_depth_mode(GEDepthMode) {}
void ge_set_depth_func(GECompareFunc) {}
void ge_set_cull_mode(GECullMode) {}
void ge_set_texture_filter(GETextureFilter, GETextureFilter) {}
void ge_set_texture_blend(GETextureBlend) {}
void ge_set_texture_address(GETextureAddress) {}
void ge_set_depth_bias(int32_t) {}
void ge_set_fog_enabled(bool) {}
void ge_set_color_key_enabled(bool) {}
void ge_set_alpha_test_enabled(bool) {}
void ge_set_alpha_ref(uint32_t) {}
void ge_set_alpha_func(GECompareFunc) {}
void ge_set_fog_params(uint32_t, float, float) {}
void ge_set_specular_enabled(bool) {}
void ge_set_perspective_correction(bool) {}

void ge_bind_texture(GETextureHandle) {}

void ge_draw_primitive(GEPrimitiveType, const GEVertexTL*, uint32_t) {}
void ge_draw_indexed_primitive(GEPrimitiveType, const GEVertexTL*, uint32_t, const uint16_t*, uint32_t) {}
void ge_draw_indexed_primitive_lit(GEPrimitiveType, const GEVertexLit*, uint32_t, const uint16_t*, uint32_t) {}
void ge_draw_indexed_primitive_unlit(GEPrimitiveType, const GEVertex*, uint32_t, const uint16_t*, uint32_t) {}

void ge_set_transform(GETransform, const GEMatrix*) {}
void ge_set_viewport(int32_t, int32_t, int32_t, int32_t) {}
void ge_set_viewport_3d(int32_t, int32_t, int32_t, int32_t, float, float, float, float) {}

void ge_set_background(GEBackground) {}
void ge_set_background_color(uint8_t, uint8_t, uint8_t) {}

void* ge_lock_screen() { return nullptr; }
void  ge_unlock_screen() {}
uint8_t* ge_get_screen_buffer() { return nullptr; }
int32_t  ge_get_screen_pitch() { return 0; }
int32_t  ge_get_screen_width() { return 640; }
int32_t  ge_get_screen_height() { return 480; }
int32_t  ge_get_screen_bpp() { return 16; }
void ge_plot_pixel(int32_t, int32_t, uint8_t, uint8_t, uint8_t) {}
void ge_plot_formatted_pixel(int32_t, int32_t, uint32_t) {}
void ge_get_pixel(int32_t, int32_t, uint8_t*, uint8_t*, uint8_t*) {}
uint32_t ge_get_formatted_pixel(uint8_t, uint8_t, uint8_t) { return 0; }
void ge_blit_back_buffer() {}

bool ge_supports_dest_inv_src_color() { return false; }
bool ge_supports_modulate_alpha() { return true; }
bool ge_supports_adami_lighting() { return false; }
bool ge_is_hardware() { return true; }
bool ge_is_fullscreen() { return false; }

void ge_create_background_surface(uint8_t*) {}
void ge_blit_background_surface() {}
void ge_destroy_background_surface() {}

void ge_init_back_image(const char*) {}
void ge_show_back_image(bool) {}
void ge_reset_back_image() {}

void ge_run_cutscene(int32_t) {}

bool ge_is_primary_driver() { return true; }

void ge_to_gdi() {}
void ge_from_gdi() {}
void ge_restore_all_surfaces() {}
bool ge_is_display_changed() { return false; }
void ge_clear_display_changed() {}
void ge_update_display_rect(void*, bool) {}

void ge_remove_all_loaded_textures() {}

void ge_capture_backbuffer_to_texture(int32_t, int32_t, int32_t) {}

GEScreenSurface ge_create_screen_surface(uint8_t*) { return 0; }
GEScreenSurface ge_load_screen_surface(const char*) { return 0; }
void ge_destroy_screen_surface(GEScreenSurface) {}
void ge_restore_screen_surface(GEScreenSurface) {}
void ge_set_background_override(GEScreenSurface) {}
GEScreenSurface ge_get_background_override() { return 0; }
void ge_blit_surface_to_backbuffer(GEScreenSurface, int32_t, int32_t, int32_t, int32_t) {}

void ge_reclaim_vertex_buffers() {}
void ge_dump_vpool_info(void*) {}
void* ge_vb_alloc(uint32_t, void** out_ptr, uint32_t* out_logsize) { if (out_ptr) *out_ptr = nullptr; if (out_logsize) *out_logsize = 0; return nullptr; }
void* ge_vb_expand(void*, void** out_ptr, uint32_t* out_logsize) { if (out_ptr) *out_ptr = nullptr; if (out_logsize) *out_logsize = 0; return nullptr; }
void ge_vb_release(void*) {}
void* ge_vb_get_ptr(void*) { return nullptr; }
void* ge_vb_prepare(void*) { return nullptr; }
void ge_draw_indexed_primitive_vb(void*, const uint16_t*, uint32_t) {}

bool ge_lock_texture_pixels(int32_t, uint16_t**, int32_t*) { return false; }
void ge_unlock_texture_pixels(int32_t) {}
void ge_get_texture_pixel_format(int32_t, int32_t*, int32_t*, int32_t*, int32_t*, int32_t*, int32_t*, int32_t*, int32_t*) {}
bool ge_is_texture_loaded(int32_t) { return false; }

void ge_debug_paint_block(uint32_t) {}

void ge_set_gamma(int32_t, int32_t) {}
void ge_get_gamma(int32_t*, int32_t*) {}
bool ge_is_gamma_available() { return false; }

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

void ge_set_pre_flip_callback(GEPreFlipCallback) {}
void ge_set_mode_change_callback(GEModeChangeCallback) {}
// ge_set_polys_drawn_callback defined in pipeline/polypage.cpp (common code, not backend-specific)
void ge_set_render_states_reset_callback(GERenderStatesResetCallback) {}
void ge_set_tga_load_callback(GETGALoadCallback) {}
void ge_set_texture_reload_prepare_callback(GETextureReloadPrepareCallback) {}
void ge_set_data_dir(const char*) {}
bool ge_is_ntsc() { return false; }
void ge_enumerate_drivers(GEDriverEnumCallback, void*) {}

// ========== GERenderState stubs ==========

GERenderState GERenderState::s_State;

GERenderState::GERenderState(GETextureFilter, GETextureFilter) :
    Texture(0), TexAddress(GETextureAddress::Wrap),
    TexMag(GETextureFilter::Linear), TexMin(GETextureFilter::Nearest),
    TexBlend(GETextureBlend::Modulate),
    DepthEnabled(true), DepthWrite(true), DepthFunc(GECompareFunc::LessEqual),
    AlphaTestEnabled(false), SrcBlend(GEBlendFactor::One), DstBlend(GEBlendFactor::Zero),
    AlphaBlendEnabled(false), FogEnabled(false), ColorKeyEnabled(false),
    Cull(GECullMode::CCW), DepthBias(0), Effect(GE_EFFECT_NONE), WrapOnce(false),
    TempTransparent(false), TempSrcBlend(GEBlendFactor::One), TempDstBlend(GEBlendFactor::Zero),
    TempAlphaBlendEnabled(false), TempDepthWrite(true),
    TempTexBlend(GETextureBlend::Modulate), TempEffect(GE_EFFECT_NONE) {}

void GERenderState::SetTexture(GETextureHandle t) { Texture = t; }
void GERenderState::SetTextureAddress(GETextureAddress a) { TexAddress = a; }
void GERenderState::SetTextureFilter(GETextureFilter mag, GETextureFilter min) { TexMag = mag; TexMin = min; }
void GERenderState::SetTextureMag(GETextureFilter mag) { TexMag = mag; }
void GERenderState::SetTextureMin(GETextureFilter min) { TexMin = min; }
void GERenderState::SetTextureBlend(GETextureBlend b) { TexBlend = b; }
void GERenderState::SetBlendMode(GEBlendMode) {}
void GERenderState::SetSrcBlend(GEBlendFactor s) { SrcBlend = s; }
void GERenderState::SetDstBlend(GEBlendFactor d) { DstBlend = d; }
void GERenderState::SetAlphaBlendEnabled(bool e) { AlphaBlendEnabled = e; }
void GERenderState::SetDepthEnabled(bool e) { DepthEnabled = e; }
void GERenderState::SetDepthWrite(bool w) { DepthWrite = w; }
void GERenderState::SetDepthFunc(GECompareFunc f) { DepthFunc = f; }
void GERenderState::SetDepthBias(int32_t b) { DepthBias = b; }
void GERenderState::SetFogEnabled(bool e) { FogEnabled = e; }
void GERenderState::SetAlphaTestEnabled(bool e) { AlphaTestEnabled = e; }
void GERenderState::SetColorKeyEnabled(bool e) { ColorKeyEnabled = e; }
void GERenderState::SetCullMode(GECullMode c) { Cull = c; }
void GERenderState::SetEffect(GERenderEffect e) { Effect = e; }
void GERenderState::SetTempTransparent() {}
void GERenderState::ResetTempTransparent() {}
bool GERenderState::IsSameRenderState(GERenderState*) { return false; }
void GERenderState::InitScene(uint32_t, float, float) {}
void GERenderState::SetChanged() {}

// ========== Additional backend symbols needed by game code ==========

#include "engine/graphics/pipeline/polypage.h"
#include "engine/platform/uc_common.h"

// PolyPage stubs
bool PolyPage::s_AlphaSort = false;
float PolyPage::s_XScale = 1.0f;
float PolyPage::s_YScale = 1.0f;
PolyPage::PolyPage(ULONG) {}
PolyPage::~PolyPage() {}
PolyPoly* PolyPage::PolyBufAlloc() { return nullptr; }
PolyPoint2D* PolyPage::PointAlloc(ULONG) { return nullptr; }
void PolyPage::AddToBuckets(PolyPoly** const, int) {}
void PolyPage::Clear() {}
void PolyPage::DrawSinglePoly(PolyPoly*) {}
void PolyPage::Render() {}
void PolyPage::SetTexOffset(SLONG) {}
void PolyPage::SetScaling(float, float) {}
void PolyPage::SetGreenScreen(bool) {}

// TrueType text stub
// PreFlipTT defined in text/truetype.cpp (common code)

// Bitmap text stub
void draw_centre_text_at(float, float, char*, SLONG, SLONG) {}

// GenerateMMMatrixFromStandardD3DOnes defined in pipeline/polypage.cpp (common code)
// ge_draw_multi_matrix defined in pipeline/polypage.cpp (common code, not backend-specific)

// Display globals stubs
SLONG RealDisplayWidth = 640;
SLONG RealDisplayHeight = 480;
volatile HWND hDDLibWindow = NULL;
UBYTE* image_mem = nullptr;

// Texture utility stubs
void SetLastClumpfile(char*, unsigned int) {}

SLONG OpenDisplay(ULONG, ULONG, ULONG, ULONG) { return 0; }
SLONG SetDisplay(ULONG, ULONG, ULONG) { return 0; }
SLONG CloseDisplay() { return 0; }
SLONG ClearDisplay(UBYTE, UBYTE, UBYTE) { return 0; }
void* LockWorkScreen() { return nullptr; }
void UnlockWorkScreen() {}
void ShowWorkScreen(ULONG) {}
void ClearWorkScreen(UBYTE) {}

// ========== oge_* stubs (outro graphics engine) ==========

void oge_init() {}
void oge_shutdown() {}
OGETexture oge_texture_create(const char*, int32_t, int32_t, uint32_t, const uint8_t*, int32_t) { return nullptr; }
OGETexture oge_texture_create_blank(int32_t, int32_t) { return nullptr; }
void oge_texture_finished_creating() {}
int32_t oge_texture_size(OGETexture) { return 0; }
void oge_texture_lock(OGETexture) {}
void oge_texture_unlock(OGETexture) {}
void oge_init_renderstates() {}
void oge_calculate_pipeline() {}
void oge_change_renderstate(uint32_t) {}
void oge_undo_renderstate_changes() {}
GETextSurface ge_text_surface_create(int32_t, int32_t) { return GE_TEXT_SURFACE_NONE; }
void ge_text_surface_destroy(GETextSurface) {}
bool ge_text_surface_get_dc(GETextSurface, void**) { return false; }
void ge_text_surface_release_dc(GETextSurface, void*) {}
bool ge_text_surface_lock(GETextSurface, uint8_t**, int32_t*) { return false; }
void ge_text_surface_unlock(GETextSurface) {}

void oge_bind_texture(int32_t, OGETexture) {}
void oge_draw_indexed(const void*, int32_t, const uint16_t*, int32_t) {}
void oge_clear_screen() {}
void oge_scene_begin() {}
void oge_scene_end() {}
void oge_show() {}
