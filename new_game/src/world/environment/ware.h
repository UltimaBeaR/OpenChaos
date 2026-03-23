#ifndef WORLD_ENVIRONMENT_WARE_H
#define WORLD_ENVIRONMENT_WARE_H

#include "core/types.h"
#include "ai/mav.h"

// Forward declaration to avoid pulling in Thing.h here.
struct Thing;

// One door of a warehouse: outside mapsquare coords and inside mapsquare coords.
// Extracted from the anonymous nested struct inside WARE_Ware in the original.
// uc_orig: WARE_Ware (fallen/Headers/ware.h)
struct WARE_Door {
    UBYTE out_x;
    UBYTE out_z;
    UBYTE in_x;
    UBYTE in_z;
};

#define WARE_MAX_DOORS 4

// Per-warehouse data: door locations, bounding box, navigation offsets, ambience.
// uc_orig: WARE_Ware (fallen/Headers/ware.h)
typedef struct {
    WARE_Door door[WARE_MAX_DOORS]; // Up to four entrance/exit pairs.
    UBYTE door_upto;

    UBYTE minx;
    UBYTE minz;
    UBYTE maxx; // Inclusive.
    UBYTE maxz; // Inclusive.

    UBYTE nav_pitch; // Stride for WARE_nav: index = (x - minx) * nav_pitch + (z - minz).
    UWORD nav;       // Start index into the global WARE_nav pool for this warehouse.
    UWORD building;  // DBuilding index this warehouse corresponds to.
    UWORD height;    // Start index into the global WARE_height pool.
    UWORD rooftex;   // Start index into the global WARE_rooftex pool.
    UBYTE ambience;  // Ambient sound preset to use while inside.
    UBYTE padding;
} WARE_Ware;

#define WARE_MAX_WARES 32
#define WARE_MAX_NAVS 4096
#define WARE_MAX_HEIGHTS 8192
#define WARE_MAX_ROOFTEXES 4096

// Scans the map for BUILDING_TYPE_WAREHOUSE buildings, builds navigation grids,
// height arrays, roof textures, and marks PAP_lo squares. Call after buildings are compiled.
// uc_orig: WARE_init (fallen/Source/ware.cpp)
void WARE_init(void);

// Returns UC_TRUE if mapsquare (x, z) is inside warehouse ware's floor plan.
// uc_orig: WARE_in_floorplan (fallen/Source/ware.cpp)
SLONG WARE_in_floorplan(UBYTE ware, UBYTE x, UBYTE z);

// Returns the warehouse index (1-based) that contains mapsquare (x, z), or NULL if none.
// uc_orig: WARE_which_contains (fallen/Source/ware.cpp)
SLONG WARE_which_contains(UBYTE x, UBYTE z);

// Returns UC_TRUE if world-space point (x, y, z) is below the warehouse floor/ceiling
// (i.e. underground inside crates). Returns UC_TRUE if outside the warehouse entirely.
// uc_orig: WARE_inside (fallen/Source/ware.cpp)
SLONG WARE_inside(UBYTE ware, SLONG x, SLONG y, SLONG z);

// Returns the navigation caps byte for movement in dir from mapsquare (x, z) inside ware.
// uc_orig: WARE_get_caps (fallen/Source/ware.cpp)
UBYTE WARE_get_caps(UBYTE ware, UBYTE x, UBYTE z, UBYTE dir);

// Returns the floor height at world-space (x, z) inside warehouse ware.
// uc_orig: WARE_calc_height_at (fallen/Source/ware.cpp)
SLONG WARE_calc_height_at(UBYTE ware, SLONG x, SLONG z);

// Sets WARE_in and invalidates lighting caches when the player enters a warehouse.
// uc_orig: WARE_enter (fallen/Source/ware.cpp)
void WARE_enter(SLONG building);

// Clears WARE_in and invalidates lighting caches when the player exits a warehouse.
// uc_orig: WARE_exit (fallen/Source/ware.cpp)
void WARE_exit(void);

// MAV navigation for a person approaching a warehouse from outside.
// uc_orig: WARE_mav_enter (fallen/Source/ware.cpp)
MAV_Action WARE_mav_enter(Thing* p_person, UBYTE ware, UBYTE caps);

// MAV navigation for a person moving within a warehouse to (dest_x, dest_z).
// uc_orig: WARE_mav_inside (fallen/Source/ware.cpp)
MAV_Action WARE_mav_inside(Thing* p_person, UBYTE dest_x, UBYTE dest_z, UBYTE caps);

// MAV navigation for a person inside a warehouse heading to the nearest exit.
// uc_orig: WARE_mav_exit (fallen/Source/ware.cpp)
MAV_Action WARE_mav_exit(Thing* p_person, UBYTE caps);

// Draws debug lines for all warehouse doors and navigation grids.
// uc_orig: WARE_debug (fallen/Source/ware.cpp)
void WARE_debug(void);

#endif // WORLD_ENVIRONMENT_WARE_H
