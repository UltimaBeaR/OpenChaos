#ifndef NAVIGATION_INSIDE2_H
#define NAVIGATION_INSIDE2_H

#include "engine/core/types.h"

// Forward declaration — Thing is defined in fallen/Headers/Game.h (not yet migrated).
struct Thing;

// Inside building navigation: storey/room detection, collision sliding, stair traversal.
// "Inside" = a building's interior floor plan described by InsideStorey + inside_block grid.
// Each InsideStorey is a bounding rectangle tiling of 1-byte room IDs. Bit fields in
// each cell encode doors (FLAG_DOOR_LEFT/UP) and stair presence (FLAG_INSIDE_STAIR).

// ---- Constants ----

// uc_orig: MAX_INSIDE_RECT (fallen/Headers/inside2.h)
#define MAX_INSIDE_RECT 2000

// uc_orig: MAX_INSIDE_MEM (fallen/Headers/inside2.h)
#define MAX_INSIDE_MEM 64000

// uc_orig: MAX_INSIDE_STAIRS (fallen/Headers/inside2.h)
#define MAX_INSIDE_STAIRS (MAX_INSIDE_RECT * 4)

// uc_orig: START_PAGE_FOR_FLOOR (fallen/Headers/inside2.h)
#define START_PAGE_FOR_FLOOR 8

// Cell flag: door opening on the left (west) wall of this cell.
// uc_orig: FLAG_DOOR_LEFT (fallen/Headers/inside2.h)
#define FLAG_DOOR_LEFT (1 << 4)

// Cell flag: door opening on the top (north) wall of this cell.
// uc_orig: FLAG_DOOR_UP (fallen/Headers/inside2.h)
#define FLAG_DOOR_UP (1 << 5)

// Cell flags: staircase triggers (up/down/both).
// uc_orig: FLAG_INSIDE_STAIR_UP (fallen/Headers/inside2.h)
#define FLAG_INSIDE_STAIR_UP (1 << 6)
// uc_orig: FLAG_INSIDE_STAIR_DOWN (fallen/Headers/inside2.h)
#define FLAG_INSIDE_STAIR_DOWN (1 << 7)
// uc_orig: FLAG_INSIDE_STAIR (fallen/Headers/inside2.h)
#define FLAG_INSIDE_STAIR (3 << 6)

// ---- Structures ----

// Binary-file format structures: 1-byte packed for level file compatibility.
#pragma pack(push, 1)

// One floor of a building interior: bounding rect in map coords, room grid, stair list.
// uc_orig: InsideStorey (fallen/Headers/inside2.h)
struct InsideStorey {
    UBYTE MinX;         // bounding rect (map squares)
    UBYTE MinZ;
    UBYTE MaxX;
    UBYTE MaxZ;
    UWORD InsideBlock;  // index into inside_block[] — packed array of 1-byte room IDs
    UWORD StairCaseHead; // linked list head into inside_stairs[]
    UWORD TexType;      // interior style index
    UWORD FacetStart;   // dfacet range for this interior
    UWORD FacetEnd;
    SWORD StoreyY;      // Y coordinate of this floor
    UWORD Building;
    UWORD Dummy[2];
};

// Connection between two InsideStoreys — a staircase or vertical passage.
// uc_orig: Staircase (fallen/Headers/inside2.h)
struct Staircase {
    UBYTE X, Z;         // map position of staircase
    UBYTE Flags;        // direction (GET_STAIR_DIR) and up/down bits
    UBYTE ID;
    SWORD NextStairs;   // linked list: next staircase in same InsideStorey
    SWORD DownInside;   // InsideStorey index for going downstairs (0 = none)
    SWORD UpInside;     // InsideStorey index for going upstairs (0 = none)
};

#pragma pack(pop)

static_assert(sizeof(InsideStorey) == 22, "InsideStorey: binary file layout");
static_assert(sizeof(Staircase) == 10, "Staircase: binary file layout");

// ---- Functions ----

// Returns the Y altitude of the given inside storey floor.
// uc_orig: get_inside_alt (fallen/Source/inside2.cpp)
SLONG get_inside_alt(SLONG inside);

// Returns the room ID (1-15) at map cell (x, z) within the given inside storey, or 0.
// uc_orig: find_inside_room (fallen/Source/inside2.cpp)
SLONG find_inside_room(SLONG inside, SLONG x, SLONG z);

// Returns the full cell byte (room ID + door/stair flags) at (x, z), or 0.
// uc_orig: find_inside_flags (fallen/Source/inside2.cpp)
SLONG find_inside_flags(SLONG inside, SLONG x, SLONG z);

// Slide-collision for a person moving inside a building. Clamps the destination
// position (x2, y2, z2) against room walls and door frames. Returns 1 if the
// person is outside their permitted room (may have exited a door).
// uc_orig: person_slide_inside (fallen/Source/inside2.cpp)
SLONG person_slide_inside(SLONG inside, SLONG x1, SLONG y1, SLONG z1, SLONG* x2, SLONG* y2, SLONG* z2);

#endif // NAVIGATION_INSIDE2_H
