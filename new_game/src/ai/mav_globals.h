#ifndef AI_MAV_GLOBALS_H
#define AI_MAV_GLOBALS_H

#include "ai/mav_action.h"
#include "map/map.h"  // MAP_HEIGHT, MAP_WIDTH for MAV_flag/MAV_dir array dimensions

// Navigation option record: one entry per unique combination of per-direction movement caps.
// MAV_NAV(x,z) indexes into MAV_opt[]; there are at most MAV_MAX_OPTS unique entries.
// uc_orig: MAV_Opt (fallen/Headers/mav.h)
typedef struct
{
    UBYTE opt[4]; // Movement options for each of the 4 cardinal directions.
} MAV_Opt;

// Maximum number of unique MAV_Opt entries in the pool.
// uc_orig: MAV_MAX_OPTS (fallen/Headers/mav.h)
#define MAV_MAX_OPTS 1024

// uc_orig: MAV_opt (fallen/Source/mav.cpp)
extern MAV_Opt* MAV_opt;

// uc_orig: MAV_opt_upto (fallen/Source/mav.cpp)
extern SLONG MAV_opt_upto;

// Navigation grid: 2D UWORD array, pitch = MAV_nav_pitch.
// Each cell: bits 0-9 = opt index, bits 10-13 = car flags, bits 14-15 = spare.
// uc_orig: MAV_nav (fallen/Source/mav.cpp)
extern UWORD* MAV_nav;

// uc_orig: MAV_nav_pitch (fallen/Source/mav.cpp)
extern SLONG MAV_nav_pitch;

// Last position reached by MAV_can_i_walk before hitting a wall.
// uc_orig: MAV_last_mx (fallen/Source/mav.cpp)
extern SLONG MAV_last_mx;

// uc_orig: MAV_last_mz (fallen/Source/mav.cpp)
extern SLONG MAV_last_mz;

// Direction attempted when MAV_can_i_walk hit a wall.
// uc_orig: MAV_dmx (fallen/Source/mav.cpp)
extern SLONG MAV_dmx;

// uc_orig: MAV_dmz (fallen/Source/mav.cpp)
extern SLONG MAV_dmz;

// Agent start position for MAV_do / MAV_get_first_action_from_nodelist.
// uc_orig: MAV_start_x (fallen/Source/mav.cpp)
extern UBYTE MAV_start_x;

// uc_orig: MAV_start_z (fallen/Source/mav.cpp)
extern UBYTE MAV_start_z;

// Destination for MAV_do / MAV_score_pos.
// uc_orig: MAV_dest_x (fallen/Source/mav.cpp)
extern UBYTE MAV_dest_x;

// uc_orig: MAV_dest_z (fallen/Source/mav.cpp)
extern UBYTE MAV_dest_z;

// MAV_LOOKAHEAD: A* searches at most this many grid cells from the agent.
#define MAV_LOOKAHEAD 32

// Path waypoints recovered after A* terminates. Index 0 = far end, [node_upto-1] = first step.
// uc_orig: MAV_node (fallen/Source/mav.cpp)
extern MAV_Action MAV_node[MAV_LOOKAHEAD];

// Number of valid entries in MAV_node[].
// uc_orig: MAV_node_upto (fallen/Source/mav.cpp)
extern SLONG MAV_node_upto;

// Packed A* visited/action state: 2 cells per byte, 4 bits per cell.
// Bits [2:0] = action taken to reach this cell; bit [3] = visited flag.
// uc_orig: MAV_flag (fallen/Source/mav.cpp)
extern UBYTE MAV_flag[MAP_HEIGHT][MAP_WIDTH / 2];

// Direction we came from for each cell: 2 bits per cell, 4 cells per byte.
// uc_orig: MAV_dir (fallen/Source/mav.cpp)
extern UBYTE MAV_dir[MAP_HEIGHT][MAP_WIDTH / 4];

// Set to UC_TRUE by MAV_do if the exact destination was reached during search.
// uc_orig: MAV_do_found_dest (fallen/Source/mav.cpp)
extern UBYTE MAV_do_found_dest;

// Last point before terrain LOS failure in MAV_height_los_fast/slow.
// uc_orig: MAV_height_los_fail_x (fallen/Source/mav.cpp)
extern SLONG MAV_height_los_fail_x;

// uc_orig: MAV_height_los_fail_y (fallen/Source/mav.cpp)
extern SLONG MAV_height_los_fail_y;

// uc_orig: MAV_height_los_fail_z (fallen/Source/mav.cpp)
extern SLONG MAV_height_los_fail_z;

#endif // AI_MAV_GLOBALS_H
