#include "ui/hud/eng_map_globals.h"

// uc_orig: MAP_screen_size_x (fallen/DDEngine/Source/engineMap.cpp)
float MAP_screen_size_x;
// uc_orig: MAP_screen_size_y (fallen/DDEngine/Source/engineMap.cpp)
float MAP_screen_size_y;

// uc_orig: MAP_screen_x (fallen/DDEngine/Source/engineMap.cpp)
float MAP_screen_x = 0.60F;
// uc_orig: MAP_screen_y (fallen/DDEngine/Source/engineMap.cpp)
float MAP_screen_y = 0.55F;
// uc_orig: MAP_world_x (fallen/DDEngine/Source/engineMap.cpp)
float MAP_world_x;
// uc_orig: MAP_world_z (fallen/DDEngine/Source/engineMap.cpp)
float MAP_world_z;
// uc_orig: MAP_scale_x (fallen/DDEngine/Source/engineMap.cpp)
float MAP_scale_x = 0.03F;
// uc_orig: MAP_scale_y (fallen/DDEngine/Source/engineMap.cpp)
float MAP_scale_y = 0.03F * 1.33F;

// uc_orig: MAP_pulse (fallen/DDEngine/Source/engineMap.cpp)
MAP_Pulse MAP_pulse[MAP_MAX_PULSES];

// uc_orig: MAP_beacon_colour (fallen/DDEngine/Source/engineMap.cpp)
ULONG MAP_beacon_colour[MAP_MAX_BEACON_COLOURS] = {
    0xffff00,
    0xff0000,
    0x00ff00,
    0x0000ff,
    0xff00ff,
    0x00ffff
};
