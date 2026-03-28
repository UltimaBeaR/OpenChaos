// Outro graphics engine — OpenGL 4.1 backend.
// Stub implementation for now — real outro rendering in a later phase.
// Globals (OS_screen_*, OS_bitmap_*) are defined in outro_os_globals.cpp (game code).

#include "engine/graphics/graphics_engine/outro_graphics_engine.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"

void oge_init()
{
    OS_screen_width = (float)ge_get_screen_width();
    OS_screen_height = (float)ge_get_screen_height();
}

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

void oge_bind_texture(int32_t, OGETexture) {}
void oge_draw_indexed(const void*, int32_t, const uint16_t*, int32_t) {}

void oge_clear_screen()
{
    ge_clear(true, true);
}

void oge_scene_begin()
{
    ge_begin_scene();
}

void oge_scene_end()
{
    ge_end_scene();
}

void oge_show()
{
    ge_flip();
}
