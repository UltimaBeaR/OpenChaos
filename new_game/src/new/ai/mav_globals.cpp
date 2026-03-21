#include "ai/mav_globals.h"

// uc_orig: MAV_opt (fallen/Source/mav.cpp)
MAV_Opt* MAV_opt;

// uc_orig: MAV_opt_upto (fallen/Source/mav.cpp)
SLONG MAV_opt_upto;

// uc_orig: MAV_nav (fallen/Source/mav.cpp)
UWORD* MAV_nav;

// uc_orig: MAV_nav_pitch (fallen/Source/mav.cpp)
SLONG MAV_nav_pitch = 128;

// uc_orig: MAV_last_mx (fallen/Source/mav.cpp)
SLONG MAV_last_mx;

// uc_orig: MAV_last_mz (fallen/Source/mav.cpp)
SLONG MAV_last_mz;

// uc_orig: MAV_dmx (fallen/Source/mav.cpp)
SLONG MAV_dmx;

// uc_orig: MAV_dmz (fallen/Source/mav.cpp)
SLONG MAV_dmz;

// uc_orig: MAV_start_x (fallen/Source/mav.cpp)
UBYTE MAV_start_x;

// uc_orig: MAV_start_z (fallen/Source/mav.cpp)
UBYTE MAV_start_z;

// uc_orig: MAV_dest_x (fallen/Source/mav.cpp)
UBYTE MAV_dest_x;

// uc_orig: MAV_dest_z (fallen/Source/mav.cpp)
UBYTE MAV_dest_z;

// uc_orig: MAV_node (fallen/Source/mav.cpp)
MAV_Action MAV_node[MAV_LOOKAHEAD];

// uc_orig: MAV_node_upto (fallen/Source/mav.cpp)
SLONG MAV_node_upto;

// uc_orig: MAV_flag (fallen/Source/mav.cpp)
UBYTE MAV_flag[MAP_HEIGHT][MAP_WIDTH / 2];

// uc_orig: MAV_dir (fallen/Source/mav.cpp)
UBYTE MAV_dir[MAP_HEIGHT][MAP_WIDTH / 4];

// uc_orig: MAV_do_found_dest (fallen/Source/mav.cpp)
UBYTE MAV_do_found_dest;

// uc_orig: MAV_height_los_fail_x (fallen/Source/mav.cpp)
SLONG MAV_height_los_fail_x;

// uc_orig: MAV_height_los_fail_y (fallen/Source/mav.cpp)
SLONG MAV_height_los_fail_y;

// uc_orig: MAV_height_los_fail_z (fallen/Source/mav.cpp)
SLONG MAV_height_los_fail_z;
