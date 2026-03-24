#ifndef OUTRO_CORE_OUTRO_OS_GLOBALS_H
#define OUTRO_CORE_OUTRO_OS_GLOBALS_H

// Global state for the outro OS platform layer.
// Includes Windows handles, DirectDraw/D3D objects, screen dimensions,
// key state, joystick state, texture/buffer management lists, and transform array.

#include "outro/core/outro_always.h"

#include <windows.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0700
#endif
#include <dinput.h>
#include <ddraw.h>
#include <d3d.h>

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

// Joystick DirectInput objects — extern because they are defined in the main game's DIManager.
// uc_orig: OS_joy_direct_input (fallen/outro/os.cpp)
extern IDirectInput* OS_joy_direct_input;
// uc_orig: OS_joy_input_device (fallen/outro/os.cpp)
extern IDirectInputDevice* OS_joy_input_device;
// uc_orig: OS_joy_input_device2 (fallen/outro/os.cpp)
extern IDirectInputDevice2* OS_joy_input_device2;
// uc_orig: OS_joy_x_range_min (fallen/outro/os.cpp)
extern SLONG OS_joy_x_range_min;
// uc_orig: OS_joy_x_range_max (fallen/outro/os.cpp)
extern SLONG OS_joy_x_range_max;
// uc_orig: OS_joy_y_range_min (fallen/outro/os.cpp)
extern SLONG OS_joy_y_range_min;
// uc_orig: OS_joy_y_range_max (fallen/outro/os.cpp)
extern SLONG OS_joy_y_range_max;

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

// Multi-texture pipeline selection — 0 or 1 depending on which D3D method validated.
// uc_orig: OS_pipeline_method_mul (fallen/outro/os.cpp)
extern SLONG OS_pipeline_method_mul;

// sound global — the currently playing music track ID.
// uc_orig: sound (fallen/outro/os.cpp)
extern SLONG sound;

// ========================================================
// OS_Framework — DirectDraw4/D3D3 device pair wrapper.
// Defined here (globals header) so the global OS_frame can be declared extern.
// ========================================================

// uc_orig: OS_Framework (fallen/outro/os.cpp)
typedef class {
public:
    LPDIRECTDRAW4      direct_draw;
    LPDIRECT3DDEVICE3  direct_3d;

    LPDIRECTDRAW4     GetDirectDraw() { return direct_draw; }
    LPDIRECT3DDEVICE3 GetD3DDevice()  { return direct_3d; }
} OS_Framework;

// uc_orig: OS_frame (fallen/outro/os.cpp)
extern OS_Framework OS_frame;

// ========================================================
// OS_Tformat — per pixel-format info populated during texture format enumeration.
// ========================================================

// uc_orig: OS_Tformat (fallen/outro/os.cpp)
typedef struct {
    SLONG valid;
    DDPIXELFORMAT ddpf;
    SLONG mask_r, mask_g, mask_b, mask_a;
    SLONG shift_r, shift_g, shift_b, shift_a;
} OS_Tformat;

// uc_orig: OS_tformat (fallen/outro/os.cpp)
extern OS_Tformat OS_tformat[4];

// ========================================================
// Texture and buffer list heads — types are opaque (defined in outro_os.cpp).
// ========================================================

struct os_texture;
struct os_buffer;

// uc_orig: OS_texture_head (fallen/outro/os.cpp)
extern struct os_texture* OS_texture_head;

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
