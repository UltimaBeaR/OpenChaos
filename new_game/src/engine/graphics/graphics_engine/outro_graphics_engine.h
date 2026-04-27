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

constexpr int32_t OGE_TEXTURE_FORMAT_RGB = 0;
constexpr int32_t OGE_TEXTURE_FORMAT_1555 = 1;
constexpr int32_t OGE_TEXTURE_FORMAT_4444 = 2;
constexpr int32_t OGE_TEXTURE_FORMAT_8 = 3;
constexpr int32_t OGE_TEXTURE_FORMAT_NUMBER = 4;

// ---------------------------------------------------------------------------
// Screen dimensions (set by oge_init, read by game code)
// ---------------------------------------------------------------------------

extern float OS_screen_width;
extern float OS_screen_height;

// ---------------------------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------------------------

// Initialise the outro backend: grab device from main engine,
// enumerate texture formats, compute mask/shift tables.
void oge_init();

// ---------------------------------------------------------------------------
// Texture creation flags
// ---------------------------------------------------------------------------

constexpr uint32_t OGE_TEX_HAS_ALPHA = (1 << 0);
constexpr uint32_t OGE_TEX_ONE_BIT_ALPHA = (1 << 1);
constexpr uint32_t OGE_TEX_GRAYSCALE = (1 << 2);

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

// Create a texture from raw BGRA pixel data. Returns existing if name+invert matches cache.
// pixels: array of {blue, green, red, alpha} bytes, width*height entries (caller owns, copied).
// flags: OGE_TEX_HAS_ALPHA, OGE_TEX_ONE_BIT_ALPHA, OGE_TEX_GRAYSCALE.
OGETexture oge_texture_create(const char* name, int32_t width, int32_t height,
    uint32_t flags, const uint8_t* pixels, int32_t invert);

// ---------------------------------------------------------------------------
// Draw mode flags (passed to oge_change_renderstate)
// ---------------------------------------------------------------------------

// uc_orig: OS_DRAW_* (fallen/outro/os.h)
#define OS_DRAW_NORMAL 0
#define OS_DRAW_ADD (1 << 0)
#define OS_DRAW_MULTIPLY (1 << 1)
#define OS_DRAW_CLAMP (1 << 2)
#define OS_DRAW_DECAL (1 << 3)
#define OS_DRAW_TRANSPARENT (1 << 4)
#define OS_DRAW_DOUBLESIDED (1 << 5)
#define OS_DRAW_NOZWRITE (1 << 6)
#define OS_DRAW_ALPHAREF (1 << 7)
#define OS_DRAW_ZREVERSE (1 << 8)
#define OS_DRAW_ZALWAYS (1 << 9)
#define OS_DRAW_CULLREVERSE (1 << 10)
#define OS_DRAW_NODITHER (1 << 11)
#define OS_DRAW_ALPHABLEND (1 << 12)
#define OS_DRAW_TEX_NONE (1 << 13)
#define OS_DRAW_TEX_MUL (1 << 14)
#define OS_DRAW_NOFILTER (1 << 15)
#define OS_DRAW_MULBYONE (1 << 16)

// ---------------------------------------------------------------------------
// Bitmap globals (set by oge_texture_lock, read by game code for pixel writes)
// ---------------------------------------------------------------------------

extern int32_t OS_bitmap_format;
extern uint16_t* OS_bitmap_uword_screen;
extern int32_t OS_bitmap_uword_pitch;
extern uint8_t* OS_bitmap_ubyte_screen;
extern int32_t OS_bitmap_ubyte_pitch;
extern int32_t OS_bitmap_width;
extern int32_t OS_bitmap_height;
extern int32_t OS_bitmap_mask_r, OS_bitmap_mask_g, OS_bitmap_mask_b, OS_bitmap_mask_a;
extern int32_t OS_bitmap_shift_r, OS_bitmap_shift_g, OS_bitmap_shift_b, OS_bitmap_shift_a;

// ---------------------------------------------------------------------------
// Render states
// ---------------------------------------------------------------------------

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
