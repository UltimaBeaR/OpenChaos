// Outro OS platform layer.
// Covers: joystick, texture wrappers, camera+transform, vertex buffers,
// Windows helpers, render loop wrappers, and sound.
// All D3D code has been moved to the outro graphics engine backend
// (engine/graphics/graphics_engine/backend_directx6/outro/core.cpp).

#include <stdarg.h>
#include <string.h>

#include "engine/input/gamepad.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/input/gamepad_globals.h"
#include "outro/core/outro_os.h"
#include "outro/core/outro_os_globals.h"
#include "outro/core/outro_key.h"
#include "outro/core/outro_matrix.h"
#include "outro/core/outro_tga.h"
#include "engine/platform/uc_common.h"
#include "engine/graphics/graphics_engine/outro_graphics_engine.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/keyboard.h"
#include "engine/audio/mfx.h"
#include "engine/audio/music.h"
#include "assets/sound_id.h"

// Entry point from outro system.
extern void MAIN_main(void);

// ========================================================
// JOYSTICK
// ========================================================

// uc_orig: OS_joy_poll (fallen/outro/os.cpp)
// Rewritten: reads from gamepad_state (filled by gamepad_poll) instead of DirectInput.
void OS_joy_poll(void)
{
    if (!gamepad_state.connected) {
        OS_joy_x           = 0.0F;
        OS_joy_y           = 0.0F;
        OS_joy_button      = 0;
        OS_joy_button_down = 0;
        OS_joy_button_up   = 0;
        return;
    }

    // gamepad_state.lX/lY are 0..65535 (centre 32768). Convert to -1..+1.
    OS_joy_x = (float(gamepad_state.lX) / 32768.0F) - 1.0F;
    OS_joy_y = (float(gamepad_state.lY) / 32768.0F) - 1.0F;

    ULONG last = OS_joy_button;
    ULONG now  = 0;

    for (SLONG i = 0; i < 32; i++) {
        if (gamepad_state.rgbButtons[i] & 0x80) {
            now |= 1 << i;
        }
    }

    OS_joy_button      = now;
    OS_joy_button_down = now & ~last;
    OS_joy_button_up   = last & ~now;
}

// ========================================================
// TEXTURE WRAPPERS (delegate to outro graphics engine)
// ========================================================

// Texture directory prefix.
#define OS_TEXTURE_DIR "Textures/"

// uc_orig: OS_texture_create (fallen/outro/os.cpp)
OS_Texture* OS_texture_create(CBYTE* fname, SLONG invert)
{
    OUTRO_TGA_Pixel* data;
    OUTRO_TGA_Info   ti;
    CBYTE            fullpath[_MAX_PATH];

    data = (OUTRO_TGA_Pixel*)malloc(256 * 256 * sizeof(OUTRO_TGA_Pixel));
    if (data == NULL) return NULL;

    sprintf(fullpath, OS_TEXTURE_DIR "%s", fname);
    ti = OUTRO_TGA_load(fullpath, 256, 256, data);

    if (!ti.valid || ti.width != ti.height) {
        free(data);
        return NULL;
    }

    uint32_t flags = 0;
    if (ti.flag & TGA_FLAG_CONTAINS_ALPHA) {
        flags |= OGE_TEX_HAS_ALPHA;
        if (ti.flag & TGA_FLAG_ONE_BIT_ALPHA) {
            flags |= OGE_TEX_ONE_BIT_ALPHA;
        }
    }
    if (ti.flag & TGA_FLAG_GRAYSCALE) {
        flags |= OGE_TEX_GRAYSCALE;
    }

    OGETexture tex = oge_texture_create(fname, ti.width, ti.height,
                                         flags, (const uint8_t*)data, invert);
    free(data);
    return tex;
}

// uc_orig: OS_texture_create (fallen/outro/os.cpp)
OS_Texture* OS_texture_create(SLONG size, SLONG format)
{
    return oge_texture_create_blank(size, format);
}

// uc_orig: OS_texture_finished_creating (fallen/outro/os.cpp)
void OS_texture_finished_creating()
{
    oge_texture_finished_creating();
}

// uc_orig: OS_texture_size (fallen/outro/os.cpp)
SLONG OS_texture_size(OS_Texture* ot)
{
    return oge_texture_size(ot);
}

// uc_orig: OS_texture_lock (fallen/outro/os.cpp)
void OS_texture_lock(OS_Texture* ot)
{
    oge_texture_lock(ot);
}

// uc_orig: OS_texture_unlock (fallen/outro/os.cpp)
void OS_texture_unlock(OS_Texture* ot)
{
    oge_texture_unlock(ot);
}

// ========================================================
// WINDOWS UTILITIES
// ========================================================

// uc_orig: OS_string (fallen/outro/os.cpp)
void OS_string(CBYTE* fmt, ...)
{
    CBYTE message[512];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);
    OutputDebugString(message);
}

// uc_orig: OS_ticks (fallen/outro/os.cpp)
SLONG OS_ticks(void)
{
    return sdl3_get_ticks() - OS_game_start_tick_count;
}

// uc_orig: OS_ticks_reset (fallen/outro/os.cpp)
void OS_ticks_reset()
{
    OS_game_start_tick_count = sdl3_get_ticks();
}

// ========================================================
// MOUSE
// ========================================================

// uc_orig: OS_mouse_get (fallen/outro/os.cpp)
void OS_mouse_get(SLONG* x, SLONG* y)
{
    POINT point;
    GetCursorPos(&point);
    *x = point.x;
    *y = point.y;
}

// uc_orig: OS_mouse_set (fallen/outro/os.cpp)
void OS_mouse_set(SLONG x, SLONG y)
{
    SetCursorPos(x, y);
}

// Processes Windows messages and maps keyboard/joystick input.
// Note: the function returns early at OS_CARRY_ON before the message pump —
// the message pump code below is unreachable dead code from the original.
// uc_orig: OS_process_messages (fallen/outro/os.cpp)
SLONG OS_process_messages()
{
    SHELL_ACTIVE;

    gamepad_poll();
    OS_joy_poll();

    if (Keys[KB_ESC]) {
        Keys[KB_ESC] = 0;
        KEY_on[KEY_ESCAPE] = UC_TRUE;
    }

    return OS_CARRY_ON;
}

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
    float screen_x2, float screen_y2)
{
    OS_cam_screen_x1 = screen_x1 * OS_screen_width;
    OS_cam_screen_y1 = screen_y1 * OS_screen_height;
    OS_cam_screen_x2 = screen_x2 * OS_screen_width;
    OS_cam_screen_y2 = screen_y2 * OS_screen_height;

    OS_cam_screen_width  = OS_cam_screen_x2 - OS_cam_screen_x1;
    OS_cam_screen_height = OS_cam_screen_y2 - OS_cam_screen_y1;
    OS_cam_screen_mid_x  = OS_cam_screen_x1 + OS_cam_screen_width  * 0.5F;
    OS_cam_screen_mid_y  = OS_cam_screen_y1 + OS_cam_screen_height * 0.5F;
    OS_cam_screen_mul_x  = OS_cam_screen_width  * 0.5F / OS_ZCLIP_PLANE;
    OS_cam_screen_mul_y  = OS_cam_screen_height * 0.5F / OS_ZCLIP_PLANE;

    OS_cam_x = world_x;
    OS_cam_y = world_y;
    OS_cam_z = world_z;

    OS_cam_lens           = lens;
    OS_cam_view_dist      = view_dist;
    OS_cam_over_view_dist = 1.0F / view_dist;
    OS_cam_aspect         = OS_cam_screen_height / OS_cam_screen_width;

    MATRIX_calc(OS_cam_matrix, yaw, pitch, roll);

    memcpy(OS_cam_view_matrix, OS_cam_matrix, sizeof(OS_cam_view_matrix));

    MATRIX_skew(
        OS_cam_matrix,
        OS_cam_aspect,
        OS_cam_lens,
        OS_cam_over_view_dist);
}

// Projects a world-space point into screen-space and stores it in an OS_Trans slot.
// uc_orig: OS_transform (fallen/outro/os.cpp)
void OS_transform(float world_x, float world_y, float world_z, OS_Trans* os)
{
    os->x = world_x - OS_cam_x;
    os->y = world_y - OS_cam_y;
    os->z = world_z - OS_cam_z;

    MATRIX_MUL(OS_cam_matrix, os->x, os->y, os->z);

    os->clip = OS_CLIP_ROTATED;

    if (os->z < OS_ZCLIP_PLANE) {
        os->clip |= OS_CLIP_NEAR;
        return;
    } else if (os->z > 1.0F) {
        os->clip |= OS_CLIP_FAR;
        return;
    } else {
        os->Z = OS_ZCLIP_PLANE / os->z;
        os->X = OS_cam_screen_mid_x + OS_cam_screen_mul_x * os->x * os->Z;
        os->Y = OS_cam_screen_mid_y - OS_cam_screen_mul_y * os->y * os->Z;

        os->clip |= OS_CLIP_TRANSFORMED;

        if (os->X < 0.0F) {
            os->clip |= OS_CLIP_LEFT;
        } else if (os->X > OS_screen_width) {
            os->clip |= OS_CLIP_RIGHT;
        }

        if (os->Y < 0.0F) {
            os->clip |= OS_CLIP_TOP;
        } else if (os->Y > OS_screen_height) {
            os->clip |= OS_CLIP_BOTTOM;
        }

        return;
    }
}

// ========================================================
// RENDER LOOP (wrappers)
// ========================================================

// uc_orig: OS_clear_screen (fallen/outro/os.cpp)
void OS_clear_screen(UBYTE r, UBYTE g, UBYTE b, float z)
{
    oge_clear_screen();
}

// uc_orig: OS_scene_begin (fallen/outro/os.cpp)
void OS_scene_begin()
{
    oge_scene_begin();
}

// uc_orig: OS_scene_end (fallen/outro/os.cpp)
void OS_scene_end()
{
    oge_scene_end();
}

// Draws a framerate indicator as a row of ticks at the top of the screen.
// uc_orig: OS_fps (fallen/outro/os.cpp)
void OS_fps()
{
    SLONG i;
    static SLONG fps = 0;
    static SLONG last_time = 0;
    static SLONG last_frame_count = 0;
    static SLONG frame_count = 0;

    float x1;
    float y1;
    float x2;
    float y2;
    float tick;
    SLONG now;

    now = OS_ticks();
    frame_count += 1;

    if (now >= last_time + 1000) {
        fps = frame_count - last_frame_count;
        last_frame_count = frame_count;
        last_time = now;
    }

    OS_Buffer* ob = OS_buffer_new();

    for (i = 0; i < fps; i++) {
        switch ((i + 1) % 10) {
        case 0:
            tick = 8.0F;
            break;
        case 5:
            tick = 5.0F;
            break;
        default:
            tick = 3.0F;
            break;
        }

        x1 = float(i)       / 100.0F;
        x2 = float(i + 1)   / 100.0F - 0.001F;
        y1 = 1.0F - tick    / OS_screen_height;
        y2 = 1.0F;

        OS_buffer_add_sprite(
            ob,
            x1, y1,
            x2, y2,
            0.0F, 1.0F,
            0.0F, 1.0F,
            0.0F,
            0x00ffffff,
            0x00000000,
            OS_FADE_BOTTOM);
    }

    OS_buffer_draw(ob, NULL);
}

// uc_orig: OS_show (fallen/outro/os.cpp)
void OS_show()
{
    oge_show();
}

// ========================================================
// VERTEX BUFFERS
// ========================================================

// Pre-transformed + lit vertex format for DrawIndexedPrimitive.
// Layout must match the backend's OGE_Flert exactly.
// uc_orig: OS_Flert (fallen/outro/os.cpp)
typedef struct {
    float sx, sy, sz;
    float rhw;
    ULONG colour;
    ULONG specular;
    float tu1, tv1;
    float tu2, tv2;
} OS_Flert;

// Full definition of OS_Buffer (opaque in the header).
// uc_orig: os_buffer (fallen/outro/os.cpp)
typedef struct os_buffer {
    SLONG       num_flerts;
    SLONG       num_indices;
    SLONG       max_flerts;
    SLONG       max_indices;
    OS_Flert*   flert;
    UWORD*      index;
    OS_Buffer*  next;
} OS_Buffer;

// uc_orig: OS_buffer_create (fallen/outro/os.cpp)
OS_Buffer* OS_buffer_create(void)
{
    OS_Buffer* ob = (OS_Buffer*)malloc(sizeof(OS_Buffer));

    ob->max_flerts  = 256;
    ob->max_indices = 1024;
    ob->num_flerts  = 0;
    ob->num_indices = 1;
    ob->flert       = (OS_Flert*)malloc(sizeof(OS_Flert) * ob->max_flerts);
    ob->index       = (UWORD*)malloc(sizeof(UWORD)      * ob->max_indices);

    memset(ob->flert,  0, sizeof(OS_Flert) * ob->max_flerts);
    memset(ob->index,  0, sizeof(UWORD)    * ob->max_indices);

    ob->next = NULL;

    return ob;
}

// uc_orig: OS_buffer_get (fallen/outro/os.cpp)
OS_Buffer* OS_buffer_get(void)
{
    OS_Buffer* ans;

    if (OS_buffer_free) {
        ans              = OS_buffer_free;
        OS_buffer_free   = OS_buffer_free->next;
        ans->next        = NULL;
    } else {
        ans = OS_buffer_create();
    }

    return ans;
}

// uc_orig: OS_buffer_give (fallen/outro/os.cpp)
void OS_buffer_give(OS_Buffer* ob)
{
    ob->next       = OS_buffer_free;
    OS_buffer_free = ob;
}

// uc_orig: OS_buffer_new (fallen/outro/os.cpp)
OS_Buffer* OS_buffer_new(void)
{
    OS_Buffer* ob = OS_buffer_get();
    ob->num_indices = 0;
    ob->num_flerts  = 1;
    return ob;
}

// uc_orig: OS_buffer_grow_flerts (fallen/outro/os.cpp)
void OS_buffer_grow_flerts(OS_Buffer* ob)
{
    ob->max_flerts *= 2;
    ob->flert = (OS_Flert*)realloc(ob->flert, ob->max_flerts * sizeof(OS_Flert));
}

// uc_orig: OS_buffer_grow_indices (fallen/outro/os.cpp)
void OS_buffer_grow_indices(OS_Buffer* ob)
{
    ob->max_indices *= 2;
    ob->index = (UWORD*)realloc(ob->index, ob->max_indices * sizeof(UWORD));
}

// uc_orig: OS_buffer_add_vert (fallen/outro/os.cpp)
void OS_buffer_add_vert(OS_Buffer* ob, OS_Vert* ov)
{
    OS_Trans* ot;
    OS_Flert* of;

    if (ob->num_flerts >= ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    ASSERT(WITHIN(ov->trans, 0, OS_MAX_TRANS - 1));

    of = &ob->flert[ob->num_flerts];
    ot = &OS_trans[ov->trans];

    of->sx       = ot->X;
    of->sy       = ot->Y;
    of->sz       = 1.0F - ot->Z;
    of->rhw      = ot->Z;
    of->colour   = ov->colour;
    of->specular = ov->specular;
    of->tu1      = ov->u1;
    of->tv1      = ov->v1;
    of->tu2      = ov->u2;
    of->tv2      = ov->v2;

    ov->index = ob->num_flerts++;
}

// uc_orig: OS_buffer_add_triangle (fallen/outro/os.cpp)
void OS_buffer_add_triangle(OS_Buffer* ob, OS_Vert* ov1, OS_Vert* ov2, OS_Vert* ov3)
{
    ULONG clip_and = OS_trans[ov1->trans].clip & OS_trans[ov2->trans].clip & OS_trans[ov3->trans].clip;

    if (clip_and & OS_CLIP_TRANSFORMED) {
        if (clip_and & OS_CLIP_OFFSCREEN) {
            return;
        } else {
            if (ov1->index == NULL) {
                OS_buffer_add_vert(ob, ov1);
            }
            if (ov2->index == NULL) {
                OS_buffer_add_vert(ob, ov2);
            }
            if (ov3->index == NULL) {
                OS_buffer_add_vert(ob, ov3);
            }

            if (ob->num_indices + 3 > ob->max_indices) {
                OS_buffer_grow_indices(ob);
            }

            ob->index[ob->num_indices++] = ov1->index;
            ob->index[ob->num_indices++] = ov2->index;
            ob->index[ob->num_indices++] = ov3->index;

            return;
        }
    } else {
        ULONG clip_or = OS_trans[ov1->trans].clip | OS_trans[ov2->trans].clip | OS_trans[ov3->trans].clip;

        if (clip_or & OS_CLIP_TRANSFORMED) {
            return;
        } else {
            return;
        }
    }
}

// uc_orig: OS_buffer_add_sprite (fallen/outro/os.cpp)
void OS_buffer_add_sprite(
    OS_Buffer* ob,
    float x1, float y1,
    float x2, float y2,
    float u1, float v1,
    float u2, float v2,
    float z,
    ULONG colour,
    ULONG specular,
    ULONG fade)
{
    SLONG i;
    OS_Flert* of;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        of->sx       = ((i & 1) ? x1 : x2) * OS_screen_width;
        of->sy       = ((i & 2) ? y1 : y2) * OS_screen_height;
        of->sz       = z;
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = ((i & 1) ? u1 : u2);
        of->tv1      = ((i & 2) ? v1 : v2);
        of->tu2      = ((i & 1) ? u1 : u2);
        of->tv2      = ((i & 2) ? v1 : v2);
    }

    if (fade) {
        if (fade & OS_FADE_TOP) {
            ob->flert[ob->num_flerts + 2].colour = 0x00000000;
            ob->flert[ob->num_flerts + 3].colour = 0x00000000;
        }
        if (fade & OS_FADE_BOTTOM) {
            ob->flert[ob->num_flerts + 0].colour = 0x00000000;
            ob->flert[ob->num_flerts + 1].colour = 0x00000000;
        }
        if (fade & OS_FADE_LEFT) {
            ob->flert[ob->num_flerts + 1].colour = 0x00000000;
            ob->flert[ob->num_flerts + 3].colour = 0x00000000;
        }
        if (fade & OS_FADE_RIGHT) {
            ob->flert[ob->num_flerts + 0].colour = 0x00000000;
            ob->flert[ob->num_flerts + 2].colour = 0x00000000;
        }
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// uc_orig: OS_buffer_add_sprite_rot (fallen/outro/os.cpp)
void OS_buffer_add_sprite_rot(
    OS_Buffer* ob,
    float x_mid, float y_mid,
    float size,
    float angle,
    float u1, float v1,
    float u2, float v2,
    float z,
    ULONG colour,
    ULONG specular,
    float tu1, float tv1,
    float tu2, float tv2)
{
    SLONG i;
    OS_Flert* of;

    float dx = sin(angle) * size;
    float dy = cos(angle) * size;
    float x;
    float y;

    x_mid *= OS_screen_width;
    y_mid *= OS_screen_height;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        x = 0.0F;
        y = 0.0F;

        if (i & 1) { x -= dx; y -= dy; }
        else        { x += dx; y += dy; }

        if (i & 2) { x += -dy; y += +dx; }
        else        { x -= -dy; y -= +dx; }

        x *= OS_screen_width;
        y *= OS_screen_height * 1.33F;

        x += x_mid;
        y += y_mid;

        of->sx       = x;
        of->sy       = y;
        of->sz       = z;
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = ((i & 1) ? u1  : u2);
        of->tv1      = ((i & 2) ? v1  : v2);
        of->tu2      = ((i & 1) ? tu1 : tu2);
        of->tv2      = ((i & 2) ? tv1 : tv2);
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// uc_orig: OS_buffer_add_sprite_arbitrary (fallen/outro/os.cpp)
void OS_buffer_add_sprite_arbitrary(
    OS_Buffer* ob,
    float x1, float y1,
    float x2, float y2,
    float x3, float y3,
    float x4, float y4,
    float u1, float v0,
    float u2, float v2,
    float u3, float v3,
    float u4, float v4,
    float z,
    ULONG colour,
    ULONG specular)
{
    SLONG i;
    float x, y, u, v;
    OS_Flert* of;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        switch (i) {
        case 0: x = x1; y = y1; u = u1; v = v0; break;
        case 1: x = x2; y = y2; u = u2; v = v2; break;
        case 2: x = x3; y = y3; u = u3; v = v3; break;
        case 3: x = x4; y = y4; u = u4; v = v4; break;
        default: ASSERT(0); break;
        }

        x *= OS_screen_width;
        y *= OS_screen_height;

        of->sx       = x;
        of->sy       = y;
        of->sz       = z;
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = u;
        of->tv1      = v;
        of->tu2      = u;
        of->tv2      = v;
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// uc_orig: OS_buffer_add_line_3d (fallen/outro/os.cpp)
void OS_buffer_add_line_3d(
    OS_Buffer* ob,
    float X1, float Y1,
    float X2, float Y2,
    float width,
    float u1, float v1,
    float u2, float v2,
    float z1, float z2,
    ULONG colour,
    ULONG specular)
{
    SLONG i;
    OS_Flert* of;
    float dx, dy, len, overlen;
    float x, y;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    dx = X2 - X1;
    dy = Y2 - Y1;

    len = qdist2(fabs(dx), fabs(dy));
    overlen = width * OS_screen_width / len;

    dx *= overlen;
    dy *= overlen;

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        x = 0.0F;
        y = 0.0F;

        if (i & 1) { x += -dy; y += +dx; }
        else        { x -= -dy; y -= +dx; }

        if (i & 2) { x += X1;  y += Y1;  }
        else        { x += X2;  y += Y2;  }

        of->sx       = x;
        of->sy       = y;
        of->sz       = ((i & 2) ? z1 : z2);
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = ((i & 1) ? u1 : u2);
        of->tv1      = ((i & 2) ? v1 : v2);
        of->tu2      = ((i & 1) ? u1 : u2);
        of->tv2      = ((i & 2) ? v1 : v2);
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// uc_orig: OS_buffer_add_line_2d (fallen/outro/os.cpp)
void OS_buffer_add_line_2d(
    OS_Buffer* ob,
    float x1, float y1,
    float x2, float y2,
    float width,
    float u1, float v1,
    float u2, float v2,
    float z,
    ULONG colour,
    ULONG specular)
{
    SLONG i;
    OS_Flert* of;
    float dx, dy, len, overlen;
    float x, y;

    if (ob->num_indices + 6 > ob->max_indices) {
        OS_buffer_grow_indices(ob);
    }
    if (ob->num_flerts + 4 > ob->max_flerts) {
        OS_buffer_grow_flerts(ob);
    }

    x1 *= OS_screen_width;
    y1 *= OS_screen_height;
    x2 *= OS_screen_width;
    y2 *= OS_screen_height;

    dx = x2 - x1;
    dy = y2 - y1;

    len = qdist2(fabs(dx), fabs(dy));
    overlen = width * OS_screen_width / len;

    dx *= overlen;
    dy *= overlen;

    for (i = 0; i < 4; i++) {
        of = &ob->flert[ob->num_flerts + i];

        x = 0.0F;
        y = 0.0F;

        if (i & 1) { x += -dy; y += +dx; }
        else        { x -= -dy; y -= +dx; }

        if (i & 2) { x += x1; y += y1; }
        else        { x += x2; y += y2; }

        of->sx       = x;
        of->sy       = y;
        of->sz       = z;
        of->rhw      = 0.5F;
        of->colour   = colour;
        of->specular = specular;
        of->tu1      = ((i & 1) ? u1 : u2);
        of->tv1      = ((i & 2) ? v1 : v2);
        of->tu2      = ((i & 1) ? u1 : u2);
        of->tv2      = ((i & 2) ? v1 : v2);
    }

    ob->index[ob->num_indices + 0] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 1] = ob->num_flerts + 1;
    ob->index[ob->num_indices + 2] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 3] = ob->num_flerts + 0;
    ob->index[ob->num_indices + 4] = ob->num_flerts + 3;
    ob->index[ob->num_indices + 5] = ob->num_flerts + 2;

    ob->num_indices += 6;
    ob->num_flerts  += 4;
}

// Submits the buffer for rendering then returns it to the free pool.
// uc_orig: OS_buffer_draw (fallen/outro/os.cpp)
void OS_buffer_draw(OS_Buffer* ob, OS_Texture* ot1, OS_Texture* ot2, ULONG draw)
{
    if (ob->num_flerts == 0) {
        OS_buffer_give(ob);
        return;
    }

    if (ot1 == NULL) {
        draw |= OS_DRAW_TEX_NONE;
    } else {
        oge_bind_texture(0, ot1);
    }

    if (ot2) {
        oge_bind_texture(1, ot2);
    }

    if (draw != OS_DRAW_NORMAL) {
        oge_change_renderstate(draw);
    }

    oge_draw_indexed(ob->flert, ob->num_flerts, ob->index, ob->num_indices);

    if (draw != OS_DRAW_NORMAL) {
        oge_undo_renderstate_changes();
    }

    OS_buffer_give(ob);
}

// ========================================================
// SOUND
// ========================================================

// uc_orig: OS_sound_init (fallen/outro/os.cpp)
void OS_sound_init()
{
    // Stub — MIDAS implementation was removed.
}

// uc_orig: OS_sound_start (fallen/outro/os.cpp)
void OS_sound_start(void)
{
    // Stub — MIDAS implementation was removed.
}

// uc_orig: OS_sound_volume (fallen/outro/os.cpp)
void OS_sound_volume(float vol)
{
    // Stub — MIDAS implementation was removed.
}

// Entry point from the main game. Initialises graphics, runs the outro loop, cleans up.
// uc_orig: OS_hack (fallen/outro/os.cpp)
void OS_hack(void)
{
    MUSIC_mode(0);
    MUSIC_mode_process();

    switch (rand() % 5) {
    case 0: sound = S_TUNE_COMBAT_TRAINING;   break;
    case 1: sound = S_TUNE_DRIVING_TRAINING;  break;
    case 2: sound = S_TUNE_CLUB_START;        break;
    case 3: sound = S_TUNE_CLUB_END;          break;
    case 4: sound = S_TUNE_ASSAULT_TRAINING;  break;
    }

    sound = S_CREDITS_LOOP;

    oge_init();

    OS_game_start_tick_count = sdl3_get_ticks();

    KEY_on[KEY_ESCAPE] = 0;

    MAIN_main();

    // DO NO CLEANUP!

    MFX_stop(0, sound);
}

// uc_orig: OS_sound_loop_start (fallen/outro/os.cpp)
void OS_sound_loop_start()
{
    MFX_play_stereo(0, sound, MFX_LOOPED);
}

// uc_orig: OS_sound_loop_process (fallen/outro/os.cpp)
void OS_sound_loop_process()
{
    MUSIC_mode_process();
    MFX_update();
}
