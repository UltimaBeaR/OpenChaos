#ifndef UI_HUD_ENG_MAP_GLOBALS_H
#define UI_HUD_ENG_MAP_GLOBALS_H

// Global state for the full-screen minimap (engineMap.cpp).

#include "core/types.h"

// MAP_Beacon struct and MAP_MAX_BEACONS defined in fallen/Headers/memory.h.
// MFStdLib.h must come first (memory.h → supermap.h needs MFFileHandle).
// string.h must come first (memory.h → structs.h → anim.h uses strcpy).
#include <string.h>
#include <MFStdLib.h>
#include "fallen/Headers/memory.h"

// --- screen layout ---

// Physical screen dimensions (set at draw time from DisplayWidth/DisplayHeight).
// uc_orig: MAP_screen_size_x (fallen/DDEngine/Source/engineMap.cpp)
extern float MAP_screen_size_x;
// uc_orig: MAP_screen_size_y (fallen/DDEngine/Source/engineMap.cpp)
extern float MAP_screen_size_y;

// Virtual screen centre (0..1 range).
// uc_orig: MAP_screen_x (fallen/DDEngine/Source/engineMap.cpp)
extern float MAP_screen_x;
// uc_orig: MAP_screen_y (fallen/DDEngine/Source/engineMap.cpp)
extern float MAP_screen_y;

// World coordinates at the screen centre (map squares).
// uc_orig: MAP_world_x (fallen/DDEngine/Source/engineMap.cpp)
extern float MAP_world_x;
// uc_orig: MAP_world_z (fallen/DDEngine/Source/engineMap.cpp)
extern float MAP_world_z;

// Virtual pixels per map square.
// uc_orig: MAP_scale_x (fallen/DDEngine/Source/engineMap.cpp)
extern float MAP_scale_x;
// uc_orig: MAP_scale_y (fallen/DDEngine/Source/engineMap.cpp)
extern float MAP_scale_y;

// --- pulses ---

// uc_orig: MAP_Pulse (fallen/DDEngine/Source/engineMap.cpp)
typedef struct
{
    SLONG life;   // countdown; 0 = unused
    ULONG colour;
    float wx;
    float wz;
    float radius;
} MAP_Pulse;

// uc_orig: MAP_MAX_PULSES (fallen/DDEngine/Source/engineMap.cpp)
#define MAP_MAX_PULSES 16

// uc_orig: MAP_pulse (fallen/DDEngine/Source/engineMap.cpp)
extern MAP_Pulse MAP_pulse[MAP_MAX_PULSES];

// --- beacon colours ---

// uc_orig: MAP_MAX_BEACON_COLOURS (fallen/DDEngine/Source/engineMap.cpp)
#define MAP_MAX_BEACON_COLOURS 6

// uc_orig: MAP_beacon_colour (fallen/DDEngine/Source/engineMap.cpp)
extern ULONG MAP_beacon_colour[MAP_MAX_BEACON_COLOURS];

#endif // UI_HUD_ENG_MAP_GLOBALS_H
