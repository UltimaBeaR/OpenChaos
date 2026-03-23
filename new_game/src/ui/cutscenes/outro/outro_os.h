#ifndef UI_CUTSCENES_OUTRO_OUTRO_OS_H
#define UI_CUTSCENES_OUTRO_OUTRO_OS_H

// uc_orig: os.h (fallen/outro/os.h)
// Platform abstraction layer for the outro/credits sequence.
// Wraps DirectDraw/Direct3D 3, keyboard input, joystick, texture management,
// render state management, camera + transform, vertex buffers, and sound.
// The main game uses its own equivalent in DDLibrary — this is the outro's
// self-contained version.

#include "ui/cutscenes/outro/outro_os_globals.h"

// ========================================================
// WINDOWS AND MISCELLANEOUS
// ========================================================

// uc_orig: OS_CARRY_ON (fallen/outro/os.h)
#define OS_CARRY_ON 0
// uc_orig: OS_QUIT_GAME (fallen/outro/os.h)
#define OS_QUIT_GAME 1

// uc_orig: OS_process_messages (fallen/outro/os.cpp)
SLONG OS_process_messages(void);

// uc_orig: OS_string (fallen/outro/os.cpp)
void OS_string(CBYTE* fmt, ...);
// uc_orig: OS_ticks (fallen/outro/os.cpp)
SLONG OS_ticks(void);
// uc_orig: OS_ticks_reset (fallen/outro/os.cpp)
void OS_ticks_reset(void);

// uc_orig: OS_mouse_get (fallen/outro/os.cpp)
void OS_mouse_get(SLONG* x, SLONG* y);
// uc_orig: OS_mouse_set (fallen/outro/os.cpp)
void OS_mouse_set(SLONG x, SLONG y);

// ========================================================
// TEXTURE TYPES AND CONSTANTS
// ========================================================

// uc_orig: os_texture (fallen/outro/os.cpp)
typedef struct os_texture OS_Texture;

// uc_orig: OS_TEXTURE_FORMAT_RGB (fallen/outro/os.h)
#define OS_TEXTURE_FORMAT_RGB    0
// uc_orig: OS_TEXTURE_FORMAT_1555 (fallen/outro/os.h)
#define OS_TEXTURE_FORMAT_1555   1
// uc_orig: OS_TEXTURE_FORMAT_4444 (fallen/outro/os.h)
#define OS_TEXTURE_FORMAT_4444   2
// uc_orig: OS_TEXTURE_FORMAT_8 (fallen/outro/os.h)
#define OS_TEXTURE_FORMAT_8      3
// uc_orig: OS_TEXTURE_FORMAT_NUMBER (fallen/outro/os.h)
#define OS_TEXTURE_FORMAT_NUMBER 4

// uc_orig: OS_texture_create (fallen/outro/os.cpp)
OS_Texture* OS_texture_create(CBYTE* fname, SLONG invert = UC_FALSE);
// uc_orig: OS_texture_create (fallen/outro/os.cpp)
OS_Texture* OS_texture_create(SLONG size, SLONG format);
// uc_orig: OS_texture_finished_creating (fallen/outro/os.cpp)
void OS_texture_finished_creating(void);
// uc_orig: OS_texture_size (fallen/outro/os.cpp)
SLONG OS_texture_size(OS_Texture* ot);
// uc_orig: OS_texture_lock (fallen/outro/os.cpp)
void OS_texture_lock(OS_Texture* ot);
// uc_orig: OS_texture_unlock (fallen/outro/os.cpp)
void OS_texture_unlock(OS_Texture* ot);

// Globals set after OS_texture_lock() — see outro_os_globals.h for declarations.
// (OS_bitmap_format, OS_bitmap_uword_screen, OS_bitmap_uword_pitch, etc.)

// Pixel helper macros for writing into a locked 16-bit texture surface.
// uc_orig: OS_BITMAP_UWORD_PIXEL (fallen/outro/os.h)
#define OS_BITMAP_UWORD_PIXEL(x, y) \
    (OS_bitmap_uword_screen + (x) + (y) * OS_bitmap_uword_pitch)
// uc_orig: OS_BITMAP_UWORD_COLOUR (fallen/outro/os.h)
#define OS_BITMAP_UWORD_COLOUR(r, g, b) \
    ((((r) >> OS_bitmap_mask_r) << OS_bitmap_shift_r) | \
     (((g) >> OS_bitmap_mask_g) << OS_bitmap_shift_g) | \
     (((b) >> OS_bitmap_mask_b) << OS_bitmap_shift_b))
// uc_orig: OS_BITMAP_UWORD_PLOT (fallen/outro/os.h)
#define OS_BITMAP_UWORD_PLOT(x, y, r, g, b) \
    { *OS_BITMAP_UWORD_PIXEL(x, y) = OS_BITMAP_UWORD_COLOUR(r, g, b); }

// uc_orig: OS_BITMAP_UBYTE_PIXEL (fallen/outro/os.h)
#define OS_BITMAP_UBYTE_PIXEL(x, y) \
    (OS_bitmap_ubyte_screen + (x) + (y) * OS_bitmap_ubyte_pitch)
// uc_orig: OS_BITMAP_UBYTE_PLOT (fallen/outro/os.h)
#define OS_BITMAP_UBYTE_PLOT(x, y, c) \
    { *OS_BITMAP_UBYTE_PIXEL(x, y) = (c); }

// ========================================================
// CAMERA AND TRANSFORM
// ========================================================

// uc_orig: OS_camera_set (fallen/outro/os.cpp)
void OS_camera_set(
    float world_x, float world_y, float world_z,
    float view_dist,
    float yaw, float pitch, float roll,
    float lens,
    float screen_x1, float screen_y1,
    float screen_x2, float screen_y2);

// Camera globals declared in outro_os_globals.h:
// OS_cam_x, OS_cam_y, OS_cam_z, OS_cam_lens, OS_cam_view_matrix, etc.

// Clipping flags — bitmask describing the clip status of a transformed point.
// uc_orig: OS_CLIP_TOP (fallen/outro/os.h)
#define OS_CLIP_TOP         (1 << 0)
// uc_orig: OS_CLIP_BOTTOM (fallen/outro/os.h)
#define OS_CLIP_BOTTOM      (1 << 1)
// uc_orig: OS_CLIP_LEFT (fallen/outro/os.h)
#define OS_CLIP_LEFT        (1 << 2)
// uc_orig: OS_CLIP_RIGHT (fallen/outro/os.h)
#define OS_CLIP_RIGHT       (1 << 3)
// uc_orig: OS_CLIP_FAR (fallen/outro/os.h)
#define OS_CLIP_FAR         (1 << 4)
// uc_orig: OS_CLIP_NEAR (fallen/outro/os.h)
#define OS_CLIP_NEAR        (1 << 5)
// uc_orig: OS_CLIP_ROTATED (fallen/outro/os.h)
#define OS_CLIP_ROTATED     (1 << 6)
// uc_orig: OS_CLIP_TRANSFORMED (fallen/outro/os.h)
#define OS_CLIP_TRANSFORMED (1 << 7)

// uc_orig: OS_CLIP_OFFSCREEN (fallen/outro/os.h)
#define OS_CLIP_OFFSCREEN (OS_CLIP_TOP | OS_CLIP_BOTTOM | OS_CLIP_LEFT | OS_CLIP_RIGHT)
// uc_orig: OS_ZCLIP_PLANE (fallen/outro/os.h)
#define OS_ZCLIP_PLANE (64.0F / 65536.0F)

// OS_Trans, OS_MAX_TRANS, OS_trans[], and OS_trans_upto are defined in outro_os_globals.h.

// uc_orig: OS_transform (fallen/outro/os.cpp)
void OS_transform(float world_x, float world_y, float world_z, OS_Trans* os);

// ========================================================
// VERTEX BUFFERS
// ========================================================

// uc_orig: os_buffer (fallen/outro/os.cpp)
typedef struct os_buffer OS_Buffer;

// A vertex for submission to OS_Buffer: references a transformed point by index,
// plus UV coordinates for up to two texture stages and ARGB colour.
// uc_orig: OS_Vert (fallen/outro/os.h)
typedef struct {
    UWORD trans;     // Index into OS_trans[]
    UWORD index;     // Flert index — set to NULL before adding to a buffer
    float u1, v1;
    float u2, v2;    // Second UV set for multitexturing
    ULONG colour;
    ULONG specular;
} OS_Vert;

// uc_orig: OS_buffer_new (fallen/outro/os.cpp)
OS_Buffer* OS_buffer_new(void);

// uc_orig: OS_buffer_add_triangle (fallen/outro/os.cpp)
void OS_buffer_add_triangle(OS_Buffer* ob, OS_Vert* ov1, OS_Vert* ov2, OS_Vert* ov3);

// Fade flags for OS_buffer_add_sprite — which edges to fade to transparent.
// uc_orig: OS_FADE_TOP (fallen/outro/os.h)
#define OS_FADE_TOP    (1 << 0)
// uc_orig: OS_FADE_BOTTOM (fallen/outro/os.h)
#define OS_FADE_BOTTOM (1 << 1)
// uc_orig: OS_FADE_LEFT (fallen/outro/os.h)
#define OS_FADE_LEFT   (1 << 2)
// uc_orig: OS_FADE_RIGHT (fallen/outro/os.h)
#define OS_FADE_RIGHT  (1 << 3)

// uc_orig: OS_buffer_add_sprite (fallen/outro/os.cpp)
void OS_buffer_add_sprite(
    OS_Buffer* ob,
    float x1, float y1,           // Normalised screen coords 0.0-1.0
    float x2, float y2,
    float u1 = 0.0F, float v1 = 0.0F,
    float u2 = 1.0F, float v2 = 1.0F,
    float z = 0.0F,
    ULONG colour = 0x00ffffff,
    ULONG specular = 0x00000000,
    ULONG fade = 0);

// uc_orig: OS_buffer_add_sprite_rot (fallen/outro/os.cpp)
void OS_buffer_add_sprite_rot(
    OS_Buffer* ob,
    float x_mid, float y_mid,
    float radius,  // Percentage of screen width
    float angle,
    float u1 = 0.0F, float v1 = 0.0F,
    float u2 = 1.0F, float v2 = 1.0F,
    float z = 0.0F,
    ULONG colour = 0x00ffffff,
    ULONG specular = 0x00000000,
    float tu1 = 0.0F, float tv1 = 0.0F,
    float tu2 = 1.0F, float tv2 = 1.0F);

// uc_orig: OS_buffer_add_sprite_arbitrary (fallen/outro/os.cpp)
void OS_buffer_add_sprite_arbitrary(
    OS_Buffer* ob,
    float x1, float y1, float x2, float y2,
    float x3, float y3, float x4, float y4,
    float u1 = 0.0F, float v0 = 0.0F,
    float u2 = 1.0F, float v2 = 0.0F,
    float u3 = 0.0F, float v3 = 1.0F,
    float u4 = 1.0F, float v4 = 1.0F,
    float z = 0.0F,
    ULONG colour = 0x00ffffff,
    ULONG specular = 0x00000000);

// uc_orig: OS_buffer_add_line_2d (fallen/outro/os.cpp)
void OS_buffer_add_line_2d(
    OS_Buffer* ob,
    float x1, float y1, float x2, float y2,
    float width = 0.01F,
    float u1 = 0.0F, float v1 = 0.0F,
    float u2 = 1.0F, float v2 = 1.0F,
    float z = 0.0F,
    ULONG colour = 0x00ffffff,
    ULONG specular = 0x00000000);

// uc_orig: OS_buffer_add_line_3d (fallen/outro/os.cpp)
void OS_buffer_add_line_3d(
    OS_Buffer* ob,
    float X1, float Y1, float X2, float Y2,  // Real screen coordinates
    float width = 0.01F,
    float u1 = 0.0F, float v1 = 0.0F,
    float u2 = 1.0F, float v2 = 1.0F,
    float z1 = 0.0F, float z2 = 0.0F,
    ULONG colour = 0x00ffffff,
    ULONG specular = 0x00000000);

// Draw mode flags for OS_buffer_draw.
// uc_orig: OS_DRAW_NORMAL (fallen/outro/os.h)
#define OS_DRAW_NORMAL      0
// uc_orig: OS_DRAW_ADD (fallen/outro/os.h)
#define OS_DRAW_ADD         (1 << 0)
// uc_orig: OS_DRAW_MULTIPLY (fallen/outro/os.h)
#define OS_DRAW_MULTIPLY    (1 << 1)
// uc_orig: OS_DRAW_CLAMP (fallen/outro/os.h)
#define OS_DRAW_CLAMP       (1 << 2)
// uc_orig: OS_DRAW_DECAL (fallen/outro/os.h)
#define OS_DRAW_DECAL       (1 << 3)
// uc_orig: OS_DRAW_TRANSPARENT (fallen/outro/os.h)
#define OS_DRAW_TRANSPARENT (1 << 4)
// uc_orig: OS_DRAW_DOUBLESIDED (fallen/outro/os.h)
#define OS_DRAW_DOUBLESIDED (1 << 5)
// uc_orig: OS_DRAW_NOZWRITE (fallen/outro/os.h)
#define OS_DRAW_NOZWRITE    (1 << 6)
// uc_orig: OS_DRAW_ALPHAREF (fallen/outro/os.h)
#define OS_DRAW_ALPHAREF    (1 << 7)
// uc_orig: OS_DRAW_ZREVERSE (fallen/outro/os.h)
#define OS_DRAW_ZREVERSE    (1 << 8)
// uc_orig: OS_DRAW_ZALWAYS (fallen/outro/os.h)
#define OS_DRAW_ZALWAYS     (1 << 9)
// uc_orig: OS_DRAW_CULLREVERSE (fallen/outro/os.h)
#define OS_DRAW_CULLREVERSE (1 << 10)
// uc_orig: OS_DRAW_NODITHER (fallen/outro/os.h)
#define OS_DRAW_NODITHER    (1 << 11)
// uc_orig: OS_DRAW_ALPHABLEND (fallen/outro/os.h)
#define OS_DRAW_ALPHABLEND  (1 << 12)
// uc_orig: OS_DRAW_TEX_NONE (fallen/outro/os.h)
#define OS_DRAW_TEX_NONE    (1 << 13)
// uc_orig: OS_DRAW_TEX_MUL (fallen/outro/os.h)
#define OS_DRAW_TEX_MUL     (1 << 14)
// uc_orig: OS_DRAW_NOFILTER (fallen/outro/os.h)
#define OS_DRAW_NOFILTER    (1 << 15)
// uc_orig: OS_DRAW_MULBYONE (fallen/outro/os.h)
#define OS_DRAW_MULBYONE    (1 << 16)

// uc_orig: OS_buffer_draw (fallen/outro/os.cpp)
void OS_buffer_draw(
    OS_Buffer* ob,
    OS_Texture* ot1,           // NULL = no texture
    OS_Texture* ot2 = NULL,
    ULONG draw = OS_DRAW_NORMAL);

// ========================================================
// RENDER LOOP
// ========================================================

// OS_screen_width and OS_screen_height declared in outro_os_globals.h.

// uc_orig: OS_scene_begin (fallen/outro/os.cpp)
void OS_scene_begin(void);
// uc_orig: OS_scene_end (fallen/outro/os.cpp)
void OS_scene_end(void);
// uc_orig: OS_clear_screen (fallen/outro/os.cpp)
void OS_clear_screen(UBYTE r = 0, UBYTE g = 0, UBYTE b = 0, float z = 1.0F);
// uc_orig: OS_fps (fallen/outro/os.cpp)
void OS_fps(void);
// uc_orig: OS_show (fallen/outro/os.cpp)
void OS_show(void);

// ========================================================
// SOUND
// ========================================================

// uc_orig: OS_sound_init (fallen/outro/os.h)
void OS_sound_init(void);
// uc_orig: OS_sound_start (fallen/outro/os.h)
void OS_sound_start(void);
// uc_orig: OS_sound_volume (fallen/outro/os.h)
void OS_sound_volume(float vol);
// uc_orig: OS_sound_loop_start (fallen/outro/os.h)
void OS_sound_loop_start();
// uc_orig: OS_sound_loop_process (fallen/outro/os.h)
void OS_sound_loop_process();

// uc_orig: OS_hack (fallen/outro/os.cpp)
void OS_hack(void);

// Joystick globals (OS_joy_x, OS_joy_y, OS_joy_button, etc.) declared in outro_os_globals.h.

#endif // UI_CUTSCENES_OUTRO_OUTRO_OS_H
