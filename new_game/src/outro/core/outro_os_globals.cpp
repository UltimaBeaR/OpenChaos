// Global variable definitions for the outro OS platform layer.

#include "outro/core/outro_os_globals.h"

// uc_orig: KEY_on (fallen/outro/os.cpp)
UBYTE KEY_on[256] = {};
// uc_orig: KEY_shift (fallen/outro/os.cpp)
UBYTE KEY_shift = 0;

// uc_orig: OS_joy_x (fallen/outro/os.cpp)
float OS_joy_x = 0.0F;
// uc_orig: OS_joy_y (fallen/outro/os.cpp)
float OS_joy_y = 0.0F;
// uc_orig: OS_joy_button (fallen/outro/os.cpp)
ULONG OS_joy_button = 0;
// uc_orig: OS_joy_button_down (fallen/outro/os.cpp)
ULONG OS_joy_button_down = 0;
// uc_orig: OS_joy_button_up (fallen/outro/os.cpp)
ULONG OS_joy_button_up = 0;

// uc_orig: OS_screen_width (fallen/outro/os.cpp)
float OS_screen_width = 0.0F;
// uc_orig: OS_screen_height (fallen/outro/os.cpp)
float OS_screen_height = 0.0F;

// uc_orig: OS_game_start_tick_count (fallen/outro/os.cpp)
uint64_t OS_game_start_tick_count = 0;

// uc_orig: OS_cam_x (fallen/outro/os.cpp)
float OS_cam_x = 0.0F;
// uc_orig: OS_cam_y (fallen/outro/os.cpp)
float OS_cam_y = 0.0F;
// uc_orig: OS_cam_z (fallen/outro/os.cpp)
float OS_cam_z = 0.0F;
// uc_orig: OS_cam_aspect (fallen/outro/os.cpp)
float OS_cam_aspect = 0.0F;
// uc_orig: OS_cam_lens (fallen/outro/os.cpp)
float OS_cam_lens = 0.0F;
// uc_orig: OS_cam_view_dist (fallen/outro/os.cpp)
float OS_cam_view_dist = 0.0F;
// uc_orig: OS_cam_over_view_dist (fallen/outro/os.cpp)
float OS_cam_over_view_dist = 0.0F;
// uc_orig: OS_cam_matrix (fallen/outro/os.cpp)
float OS_cam_matrix[9] = {};
// uc_orig: OS_cam_view_matrix (fallen/outro/os.cpp)
float OS_cam_view_matrix[9] = {};

// uc_orig: OS_cam_screen_x1 (fallen/outro/os.cpp)
float OS_cam_screen_x1 = 0.0F;
// uc_orig: OS_cam_screen_y1 (fallen/outro/os.cpp)
float OS_cam_screen_y1 = 0.0F;
// uc_orig: OS_cam_screen_x2 (fallen/outro/os.cpp)
float OS_cam_screen_x2 = 0.0F;
// uc_orig: OS_cam_screen_y2 (fallen/outro/os.cpp)
float OS_cam_screen_y2 = 0.0F;
// uc_orig: OS_cam_screen_width (fallen/outro/os.cpp)
float OS_cam_screen_width = 0.0F;
// uc_orig: OS_cam_screen_height (fallen/outro/os.cpp)
float OS_cam_screen_height = 0.0F;
// uc_orig: OS_cam_screen_mid_x (fallen/outro/os.cpp)
float OS_cam_screen_mid_x = 0.0F;
// uc_orig: OS_cam_screen_mid_y (fallen/outro/os.cpp)
float OS_cam_screen_mid_y = 0.0F;
// uc_orig: OS_cam_screen_mul_x (fallen/outro/os.cpp)
float OS_cam_screen_mul_x = 0.0F;
// uc_orig: OS_cam_screen_mul_y (fallen/outro/os.cpp)
float OS_cam_screen_mul_y = 0.0F;

// uc_orig: sound (fallen/outro/os.cpp)
SLONG sound = 0;

// uc_orig: OS_buffer_free (fallen/outro/os.cpp)
struct os_buffer* OS_buffer_free = NULL;

// uc_orig: OS_trans (fallen/outro/os.cpp)
OS_Trans OS_trans[OS_MAX_TRANS] = {};
