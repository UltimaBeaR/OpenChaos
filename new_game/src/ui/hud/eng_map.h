#ifndef UI_HUD_ENG_MAP_H
#define UI_HUD_ENG_MAP_H

// Full-screen minimap overlay: draws the world map with terrain tiles, beacons,
// people and vehicle dots, and pulsing nav markers.
// Corresponds to fallen/DDEngine/Source/engineMap.cpp

#include "core/types.h"

// Initialises beacon and pulse arrays. Called from ELEVATOR_reset (elev.cpp) on level load.
// uc_orig: MAP_beacon_init (fallen/DDEngine/Source/engineMap.cpp)
void MAP_beacon_init(void);
// uc_orig: MAP_pulse_init (fallen/DDEngine/Source/engineMap.cpp)
void MAP_pulse_init(void);

// Drawn every frame while the map screen is active.
// uc_orig: MAP_draw (fallen/DDEngine/Headers/map.h)
void MAP_draw(void);

// Per-frame beacon and pulse animation update.
// uc_orig: MAP_process (fallen/DDEngine/Headers/map.h)
void MAP_process(void);

// Creates a nav beacon at world position (x,z) with an associated message index.
// If track_my_position is nonzero, the beacon follows that thing.
// Returns the beacon slot index (0 = failure).
// uc_orig: MAP_beacon_create (fallen/DDEngine/Headers/map.h)
UBYTE MAP_beacon_create(SLONG x, SLONG z, SLONG index, UWORD track_my_position);

// Removes the beacon at the given slot.
// uc_orig: MAP_beacon_remove (fallen/DDEngine/Headers/map.h)
void MAP_beacon_remove(UBYTE beacon);

#endif // UI_HUD_ENG_MAP_H
