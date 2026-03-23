#ifndef EFFECTS_CLOTH_H
#define EFFECTS_CLOTH_H

#include "core/types.h"

// Draped cloth simulation: flags and cloaks attached to the game world.
// Cloth is a grid of CLOTH_WIDTH × CLOTH_HEIGHT simulation points.
// Some points can be locked (pinned), others move freely under physics.

// Cloth types.
// uc_orig: CLOTH_TYPE_UNUSED (fallen/Headers/cloth.h)
#define CLOTH_TYPE_UNUSED 0
// uc_orig: CLOTH_TYPE_FLAG (fallen/Headers/cloth.h)
#define CLOTH_TYPE_FLAG   1
// uc_orig: CLOTH_TYPE_CLOAK (fallen/Headers/cloth.h)
#define CLOTH_TYPE_CLOAK  2

// Grid dimensions of a cloth object.
// uc_orig: CLOTH_WIDTH (fallen/Headers/cloth.h)
#define CLOTH_WIDTH  6
// uc_orig: CLOTH_HEIGHT (fallen/Headers/cloth.h)
#define CLOTH_HEIGHT 5

// Removes all active cloth objects and resets state.
// uc_orig: CLOTH_init (fallen/Headers/cloth.h)
void CLOTH_init(void);

// Creates a new cloth object. Returns its index (non-zero) on success, or 0 on failure.
// Origin (ox, oy, oz) is the world position.
// (wdx, wdy, wdz) is the width direction vector, (hdx, hdy, hdz) is the height direction.
// dist is the segment length. colour is ARGB.
// uc_orig: CLOTH_create (fallen/Headers/cloth.h)
UBYTE CLOTH_create(
    UBYTE type,
    SLONG ox,   SLONG oy,   SLONG oz,
    SLONG wdx,  SLONG wdy,  SLONG wdz,
    SLONG hdx,  SLONG hdy,  SLONG hdz,
    SLONG dist,
    ULONG colour);

// Pins a point on the cloth so it does not move during simulation.
// uc_orig: CLOTH_point_lock (fallen/Headers/cloth.h)
void CLOTH_point_lock(UBYTE cloth, UBYTE w, UBYTE h);

// Moves the cloth point at (w, h) to the given world coordinates.
// Typically used to move locked (pinned) anchor points each frame.
// uc_orig: CLOTH_point_move (fallen/Headers/cloth.h)
void CLOTH_point_move(UBYTE cloth, UBYTE w, UBYTE h, SLONG x, SLONG y, SLONG z);

// Advances the physics simulation for all active cloth objects.
// uc_orig: CLOTH_process (fallen/Headers/cloth.h)
void CLOTH_process(void);

// Returns the first cloth index attached to the map square at (lo_map_x, lo_map_z).
// uc_orig: CLOTH_get_first (fallen/Headers/cloth.h)
UBYTE CLOTH_get_first(UBYTE lo_map_x, UBYTE lo_map_z);

// Converts (w, h) grid coordinates to a flat array index.
// uc_orig: CLOTH_INDEX (fallen/Headers/cloth.h)
#define CLOTH_INDEX(w, h) ((w) + (h) * CLOTH_WIDTH)

// Position and normal of a single cloth grid point, used for rendering.
// uc_orig: CLOTH_Drawp (fallen/Headers/cloth.h)
typedef struct
{
    float x;
    float y;
    float z;
    float nx; // Normalised normal vector.
    float ny;
    float nz;

} CLOTH_Drawp;

// Full cloth object state: type, linked-list link, colour, and all grid point data.
// uc_orig: CLOTH_Info (fallen/Headers/cloth.h)
typedef struct
{
    UBYTE type;
    UBYTE next;    // Next cloth index in the mapsquare linked list.
    UWORD padding;
    ULONG colour;

    CLOTH_Drawp p[CLOTH_WIDTH * CLOTH_HEIGHT];

} CLOTH_Info;

// Returns a pointer to the cloth data for the given cloth index.
// uc_orig: CLOTH_get_info (fallen/Headers/cloth.h)
CLOTH_Info* CLOTH_get_info(UBYTE cloth);

#endif // EFFECTS_CLOTH_H
