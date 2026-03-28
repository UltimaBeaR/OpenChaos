// Outro graphics engine — D3D6 backend implementation.
// Implements the oge_* contract from outro_graphics_engine.h.
// Uses the shared D3D device from common/ (the_display).

#include "engine/graphics/graphics_engine/outro_graphics_engine.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/graphics_engine/backend_directx6/common/display_macros.h"
#include "engine/graphics/graphics_engine/backend_directx6/common/d3d_texture.h"
#include "engine/graphics/graphics_engine/backend_directx6/common/gd_display.h"

// Stub implementations — will be filled when migrating outro_os.cpp D3D code here.

void oge_init() {}
void oge_shutdown() {}

OGETexture oge_texture_create_from_tga(const char* fname, int32_t invert) { return nullptr; }
OGETexture oge_texture_create_blank(int32_t size, int32_t format) { return nullptr; }
void oge_texture_finished_creating() {}
int32_t oge_texture_size(OGETexture tex) { return 0; }
void oge_texture_lock(OGETexture tex) {}
void oge_texture_unlock(OGETexture tex) {}

void oge_init_renderstates() {}
void oge_calculate_pipeline() {}
void oge_change_renderstate(uint32_t draw_flags) {}
void oge_undo_renderstate_changes() {}

void oge_bind_texture(int32_t stage, OGETexture tex) {}
void oge_draw_indexed(const void* verts, int32_t num_verts,
                      const uint16_t* indices, int32_t num_indices) {}

void oge_clear_screen() { CLEAR_VIEWPORT; }
void oge_scene_begin() { ge_begin_scene(); }
void oge_scene_end() { ge_end_scene(); }
void oge_show() { ge_flip(); }
