// Global variable definitions for the outro OS platform layer.

#include "outro/core/outro_os_globals.h"

// uc_orig: OS_this_instance (fallen/outro/os.cpp)
HINSTANCE OS_this_instance = NULL;
// uc_orig: OS_last_instance (fallen/outro/os.cpp)
HINSTANCE OS_last_instance = NULL;
// uc_orig: OS_command_line (fallen/outro/os.cpp)
LPSTR OS_command_line = NULL;
// uc_orig: OS_start_show_state (fallen/outro/os.cpp)
int OS_start_show_state = 0;

// uc_orig: OS_application_name (fallen/outro/os.cpp)
CBYTE* OS_application_name = "Urban Chaos credits";

// uc_orig: OS_wcl (fallen/outro/os.cpp)
WNDCLASSEX OS_wcl = {};
// uc_orig: OS_window_handle (fallen/outro/os.cpp)
HWND OS_window_handle = NULL;

// uc_orig: OS_frame_is_fullscreen (fallen/outro/os.cpp)
UBYTE OS_frame_is_fullscreen = 0;
// uc_orig: OS_frame_is_hardware (fallen/outro/os.cpp)
UBYTE OS_frame_is_hardware = 0;

// uc_orig: KEY_on (fallen/outro/os.cpp)
UBYTE KEY_on[256] = {};
// uc_orig: KEY_inkey (fallen/outro/os.cpp)
UBYTE KEY_inkey = 0;
// uc_orig: KEY_shift (fallen/outro/os.cpp)
UBYTE KEY_shift = 0;

// uc_orig: OS_midas_ok (fallen/outro/os.cpp)
SLONG OS_midas_ok = 0;

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
DWORD OS_game_start_tick_count = 0;

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

// uc_orig: OS_bitmap_format (fallen/outro/os.cpp)
SLONG OS_bitmap_format = 0;
// uc_orig: OS_bitmap_uword_screen (fallen/outro/os.cpp)
UWORD* OS_bitmap_uword_screen = NULL;
// uc_orig: OS_bitmap_uword_pitch (fallen/outro/os.cpp)
SLONG OS_bitmap_uword_pitch = 0;
// uc_orig: OS_bitmap_ubyte_screen (fallen/outro/os.cpp)
UBYTE* OS_bitmap_ubyte_screen = NULL;
// uc_orig: OS_bitmap_ubyte_pitch (fallen/outro/os.cpp)
SLONG OS_bitmap_ubyte_pitch = 0;
// uc_orig: OS_bitmap_width (fallen/outro/os.cpp)
SLONG OS_bitmap_width = 0;
// uc_orig: OS_bitmap_height (fallen/outro/os.cpp)
SLONG OS_bitmap_height = 0;
// uc_orig: OS_bitmap_mask_r (fallen/outro/os.cpp)
SLONG OS_bitmap_mask_r = 0;
// uc_orig: OS_bitmap_mask_g (fallen/outro/os.cpp)
SLONG OS_bitmap_mask_g = 0;
// uc_orig: OS_bitmap_mask_b (fallen/outro/os.cpp)
SLONG OS_bitmap_mask_b = 0;
// uc_orig: OS_bitmap_mask_a (fallen/outro/os.cpp)
SLONG OS_bitmap_mask_a = 0;
// uc_orig: OS_bitmap_shift_r (fallen/outro/os.cpp)
SLONG OS_bitmap_shift_r = 0;
// uc_orig: OS_bitmap_shift_g (fallen/outro/os.cpp)
SLONG OS_bitmap_shift_g = 0;
// uc_orig: OS_bitmap_shift_b (fallen/outro/os.cpp)
SLONG OS_bitmap_shift_b = 0;
// uc_orig: OS_bitmap_shift_a (fallen/outro/os.cpp)
SLONG OS_bitmap_shift_a = 0;

// uc_orig: OS_pipeline_method_mul (fallen/outro/os.cpp)
SLONG OS_pipeline_method_mul = 0;

// uc_orig: sound (fallen/outro/os.cpp)
SLONG sound = 0;

// uc_orig: OS_frame (fallen/outro/os.cpp)
OS_Framework OS_frame;

// uc_orig: OS_tformat (fallen/outro/os.cpp)
OS_Tformat OS_tformat[4] = {};

// uc_orig: OS_texture_head (fallen/outro/os.cpp)
struct os_texture* OS_texture_head = NULL;

// uc_orig: OS_buffer_free (fallen/outro/os.cpp)
struct os_buffer* OS_buffer_free = NULL;

// uc_orig: OS_trans (fallen/outro/os.cpp)
OS_Trans OS_trans[OS_MAX_TRANS] = {};
// uc_orig: OS_trans_upto (fallen/outro/os.cpp)
SLONG OS_trans_upto = 0;
