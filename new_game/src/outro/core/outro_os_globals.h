#ifndef OUTRO_CORE_OUTRO_OS_GLOBALS_H
#define OUTRO_CORE_OUTRO_OS_GLOBALS_H

// Global state for the outro OS platform layer.
// Includes Windows handles, DirectDraw/D3D objects, screen dimensions,
// key state, joystick state, texture/buffer management lists, and transform array.

#include "outro/core/outro_always.h"

#include <stdint.h>

// KEY_on: one byte per virtual key code; non-zero while the key is held down.
// KEY_shift: non-zero if shift is held.
// uc_orig: KEY_on (fallen/outro/os.cpp)
extern UBYTE KEY_on[256];
// uc_orig: KEY_shift (fallen/outro/os.cpp)
extern UBYTE KEY_shift;

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

// Screen dimensions — declared in outro_graphics_engine.h (OS_screen_width/height).

// Tick counter base — set when the outro loop starts.
// uc_orig: OS_game_start_tick_count (fallen/outro/os.cpp)
extern uint64_t OS_game_start_tick_count;

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
    float x, y, z; // View-space position
    float X, Y, Z; // Screen-space position
    ULONG clip; // OS_CLIP_* flags
} OS_Trans;

// uc_orig: OS_MAX_TRANS (fallen/outro/os.h)
#define OS_MAX_TRANS 8192

// uc_orig: OS_trans (fallen/outro/os.cpp)
extern OS_Trans OS_trans[OS_MAX_TRANS];

#endif // OUTRO_CORE_OUTRO_OS_GLOBALS_H
