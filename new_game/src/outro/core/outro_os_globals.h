#ifndef OUTRO_CORE_OUTRO_OS_GLOBALS_H
#define OUTRO_CORE_OUTRO_OS_GLOBALS_H

// Global state for the outro OS platform layer.
// Includes Windows handles, DirectDraw/D3D objects, screen dimensions,
// key state, joystick state, texture/buffer management lists, and transform array.

#include "outro/core/outro_always.h"

#include <windows.h>

// uc_orig: OS_this_instance (fallen/outro/os.cpp)
extern HINSTANCE OS_this_instance;
// uc_orig: OS_last_instance (fallen/outro/os.cpp)
extern HINSTANCE OS_last_instance;
// uc_orig: OS_command_line (fallen/outro/os.cpp)
extern LPSTR OS_command_line;
// uc_orig: OS_start_show_state (fallen/outro/os.cpp)
extern int OS_start_show_state;

// uc_orig: OS_application_name (fallen/outro/os.cpp)
extern CBYTE* OS_application_name;

// uc_orig: OS_wcl (fallen/outro/os.cpp)
extern WNDCLASSEX OS_wcl;
// uc_orig: OS_window_handle (fallen/outro/os.cpp)
extern HWND OS_window_handle;

// uc_orig: OS_frame_is_fullscreen (fallen/outro/os.cpp)
extern UBYTE OS_frame_is_fullscreen;
// uc_orig: OS_frame_is_hardware (fallen/outro/os.cpp)
extern UBYTE OS_frame_is_hardware;

// KEY_on: one byte per virtual key code; non-zero while the key is held down.
// KEY_inkey: last pressed key character.
// KEY_shift: non-zero if shift is held.
// uc_orig: KEY_on (fallen/outro/os.cpp)
extern UBYTE KEY_on[256];
// uc_orig: KEY_inkey (fallen/outro/os.cpp)
extern UBYTE KEY_inkey;
// uc_orig: KEY_shift (fallen/outro/os.cpp)
extern UBYTE KEY_shift;

// uc_orig: OS_midas_ok (fallen/outro/os.cpp)
extern SLONG OS_midas_ok;

// Joystick state — updated by OS_joy_poll() called from OS_process_messages().
// uc_orig: OS_joy_x (fallen/outro/os.cpp)
extern float OS_joy_x;
// uc_orig: OS_joy_y (fallen/outro/os.cpp)
extern float OS_joy_y;
// uc_orig: OS_joy_button (fallen/outro/os.cpp)
extern ULONG OS_joy_button;
// uc_orig: OS_joy_button_down (fallen/outro/os.cpp)
extern ULONG OS_joy_button_down;
// uc_orig: OS_joy_button_up (fallen/outro/os.cpp)
extern ULONG OS_joy_button_up;


// Screen dimensions — set from RealDisplayWidth/Height during OS_hack().
// uc_orig: OS_screen_width (fallen/outro/os.cpp)
extern float OS_screen_width;
// uc_orig: OS_screen_height (fallen/outro/os.cpp)
extern float OS_screen_height;

// Tick counter base — set when the outro loop starts.
// uc_orig: OS_game_start_tick_count (fallen/outro/os.cpp)
extern DWORD OS_game_start_tick_count;

// Camera globals — set by OS_camera_set().
// uc_orig: OS_cam_x (fallen/outro/os.cpp)
extern float OS_cam_x;
// uc_orig: OS_cam_y (fallen/outro/os.cpp)
extern float OS_cam_y;
// uc_orig: OS_cam_z (fallen/outro/os.cpp)
extern float OS_cam_z;
// uc_orig: OS_cam_aspect (fallen/outro/os.cpp)
extern float OS_cam_aspect;
// uc_orig: OS_cam_lens (fallen/outro/os.cpp)
extern float OS_cam_lens;
// uc_orig: OS_cam_view_dist (fallen/outro/os.cpp)
extern float OS_cam_view_dist;
// uc_orig: OS_cam_over_view_dist (fallen/outro/os.cpp)
extern float OS_cam_over_view_dist;
// uc_orig: OS_cam_matrix (fallen/outro/os.cpp)
extern float OS_cam_matrix[9];
// uc_orig: OS_cam_view_matrix (fallen/outro/os.cpp)
extern float OS_cam_view_matrix[9];

// Screen viewport rect in pixels — derived from normalised [0..1] coords * resolution.
// uc_orig: OS_cam_screen_x1 (fallen/outro/os.cpp)
extern float OS_cam_screen_x1;
// uc_orig: OS_cam_screen_y1 (fallen/outro/os.cpp)
extern float OS_cam_screen_y1;
// uc_orig: OS_cam_screen_x2 (fallen/outro/os.cpp)
extern float OS_cam_screen_x2;
// uc_orig: OS_cam_screen_y2 (fallen/outro/os.cpp)
extern float OS_cam_screen_y2;
// uc_orig: OS_cam_screen_width (fallen/outro/os.cpp)
extern float OS_cam_screen_width;
// uc_orig: OS_cam_screen_height (fallen/outro/os.cpp)
extern float OS_cam_screen_height;
// uc_orig: OS_cam_screen_mid_x (fallen/outro/os.cpp)
extern float OS_cam_screen_mid_x;
// uc_orig: OS_cam_screen_mid_y (fallen/outro/os.cpp)
extern float OS_cam_screen_mid_y;
// uc_orig: OS_cam_screen_mul_x (fallen/outro/os.cpp)
extern float OS_cam_screen_mul_x;
// uc_orig: OS_cam_screen_mul_y (fallen/outro/os.cpp)
extern float OS_cam_screen_mul_y;

// Bitmap lock state — set by OS_texture_lock(), cleared by OS_texture_unlock().
// uc_orig: OS_bitmap_format (fallen/outro/os.cpp)
extern SLONG OS_bitmap_format;
// uc_orig: OS_bitmap_uword_screen (fallen/outro/os.cpp)
extern UWORD* OS_bitmap_uword_screen;
// uc_orig: OS_bitmap_uword_pitch (fallen/outro/os.cpp)
extern SLONG OS_bitmap_uword_pitch;
// uc_orig: OS_bitmap_ubyte_screen (fallen/outro/os.cpp)
extern UBYTE* OS_bitmap_ubyte_screen;
// uc_orig: OS_bitmap_ubyte_pitch (fallen/outro/os.cpp)
extern SLONG OS_bitmap_ubyte_pitch;
// uc_orig: OS_bitmap_width (fallen/outro/os.cpp)
extern SLONG OS_bitmap_width;
// uc_orig: OS_bitmap_height (fallen/outro/os.cpp)
extern SLONG OS_bitmap_height;
// uc_orig: OS_bitmap_mask_r (fallen/outro/os.cpp)
extern SLONG OS_bitmap_mask_r;
// uc_orig: OS_bitmap_mask_g (fallen/outro/os.cpp)
extern SLONG OS_bitmap_mask_g;
// uc_orig: OS_bitmap_mask_b (fallen/outro/os.cpp)
extern SLONG OS_bitmap_mask_b;
// uc_orig: OS_bitmap_mask_a (fallen/outro/os.cpp)
extern SLONG OS_bitmap_mask_a;
// uc_orig: OS_bitmap_shift_r (fallen/outro/os.cpp)
extern SLONG OS_bitmap_shift_r;
// uc_orig: OS_bitmap_shift_g (fallen/outro/os.cpp)
extern SLONG OS_bitmap_shift_g;
// uc_orig: OS_bitmap_shift_b (fallen/outro/os.cpp)
extern SLONG OS_bitmap_shift_b;
// uc_orig: OS_bitmap_shift_a (fallen/outro/os.cpp)
extern SLONG OS_bitmap_shift_a;

// sound global — the currently playing music track ID.
// uc_orig: sound (fallen/outro/os.cpp)
extern SLONG sound;

// ========================================================
// Buffer list head — type is opaque (defined in outro_os.cpp).
// ========================================================

struct os_buffer;

// uc_orig: OS_buffer_free (fallen/outro/os.cpp)
extern struct os_buffer* OS_buffer_free;

// ========================================================
// OS_Trans — projected world point.
// Defined here so outro_os_globals.h is self-contained.
// The identical definition also appears as OS_Trans in outro_os.h.
// ========================================================

// uc_orig: OS_Trans (fallen/outro/os.h)
typedef struct {
    float x, y, z;   // View-space position
    float X, Y, Z;   // Screen-space position
    ULONG clip;       // OS_CLIP_* flags
} OS_Trans;

// uc_orig: OS_MAX_TRANS (fallen/outro/os.h)
#define OS_MAX_TRANS 8192

// uc_orig: OS_trans (fallen/outro/os.cpp)
extern OS_Trans OS_trans[OS_MAX_TRANS];
// uc_orig: OS_trans_upto (fallen/outro/os.cpp)
extern SLONG OS_trans_upto;

#endif // OUTRO_CORE_OUTRO_OS_GLOBALS_H
