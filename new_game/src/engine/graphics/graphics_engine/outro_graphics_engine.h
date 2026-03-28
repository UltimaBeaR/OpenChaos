#ifndef ENGINE_GRAPHICS_OUTRO_GRAPHICS_ENGINE_H
#define ENGINE_GRAPHICS_OUTRO_GRAPHICS_ENGINE_H

// Outro graphics engine abstraction layer.
// The outro has its own texture system, render state management, and draw calls.
// For shared operations (begin/end scene, clear, flip) the outro calls
// the game graphics engine (game_graphics_engine.h) directly.

#include <cstdint>

// ---------------------------------------------------------------------------
// Opaque texture handle
// ---------------------------------------------------------------------------

// Forward declaration — full definition is internal to the backend.
struct os_texture;
using OGETexture = struct os_texture*;

// ---------------------------------------------------------------------------
// Texture formats (matching original OS_TEXTURE_FORMAT_* constants)
// ---------------------------------------------------------------------------

constexpr int32_t OGE_TEXTURE_FORMAT_RGB  = 0;
constexpr int32_t OGE_TEXTURE_FORMAT_1555 = 1;
constexpr int32_t OGE_TEXTURE_FORMAT_4444 = 2;
constexpr int32_t OGE_TEXTURE_FORMAT_8    = 3;
constexpr int32_t OGE_TEXTURE_FORMAT_NUMBER = 4;

// ---------------------------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------------------------

// Initialise the outro backend: grab device from main engine,
// enumerate texture formats, compute mask/shift tables.
void oge_init();

// Release all outro textures.
void oge_shutdown();

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

// Create a texture from a TGA file. Returns existing if already loaded.
// invert: 1 = invert all channels (used for glow effects).
OGETexture oge_texture_create_from_tga(const char* fname, int32_t invert);

// Create a blank texture of given size and format (for dynamic writing).
OGETexture oge_texture_create_blank(int32_t size, int32_t format);

// Called after all textures are loaded (hint to driver, currently a no-op).
void oge_texture_finished_creating();

// Get texture dimensions.
int32_t oge_texture_size(OGETexture tex);

// Lock texture for CPU write. Populates the OS_bitmap_* globals.
void oge_texture_lock(OGETexture tex);
void oge_texture_unlock(OGETexture tex);

// ---------------------------------------------------------------------------
// Render states
// ---------------------------------------------------------------------------

// Reset all render states to outro defaults (Gouraud, zbuffer, CCW cull, etc.).
void oge_init_renderstates();

// Probe which multitexture blending method the device supports.
void oge_calculate_pipeline();

// Apply render state overrides for a given OS_DRAW_* flag combination.
void oge_change_renderstate(uint32_t draw_flags);

// Restore default render states after a custom draw call.
void oge_undo_renderstate_changes();

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

// Bind a texture to a texture stage (0 or 1). NULL = unbind.
void oge_bind_texture(int32_t stage, OGETexture tex);

// Submit a vertex buffer for rendering.
// Vertices are in outro's own pre-transformed format (OS_Flert).
// verts: array of vertices, indices: array of triangle indices.
void oge_draw_indexed(const void* verts, int32_t num_verts,
                      const uint16_t* indices, int32_t num_indices);

// ---------------------------------------------------------------------------
// Scene (delegates to game graphics engine internally)
// ---------------------------------------------------------------------------

void oge_clear_screen();
void oge_scene_begin();
void oge_scene_end();
void oge_show();

#endif // ENGINE_GRAPHICS_OUTRO_GRAPHICS_ENGINE_H
