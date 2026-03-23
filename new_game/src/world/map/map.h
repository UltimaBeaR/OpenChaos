#ifndef WORLD_MAP_MAP_H
#define WORLD_MAP_MAP_H

#include "core/types.h"
#include "core/vector.h"

#include "engine/lighting/light.h" // light.h now includes prim_types.h for RMAX_PRIM_POINTS

// Map dimensions and indexing.
// uc_orig: MAP_HEIGHT_SHIFT (fallen/Headers/Map.h)
#define MAP_HEIGHT_SHIFT (7)
// uc_orig: MAP_WIDTH_SHIFT (fallen/Headers/Map.h)
#define MAP_WIDTH_SHIFT (7)
// uc_orig: MAP_HEIGHT (fallen/Headers/Map.h)
#define MAP_HEIGHT (128)
// uc_orig: MAP_WIDTH (fallen/Headers/Map.h)
#define MAP_WIDTH (128)
// uc_orig: MAP_SIZE (fallen/Headers/Map.h)
#define MAP_SIZE (MAP_HEIGHT * MAP_WIDTH)
// uc_orig: MAP_INDEX (fallen/Headers/Map.h)
#define MAP_INDEX(x, y) (((x) << MAP_WIDTH_SHIFT) | (y))
// uc_orig: ELE_SHIFT (fallen/Headers/Map.h)
#define ELE_SHIFT (8)
// uc_orig: ELE_SIZE (fallen/Headers/Map.h)
#define ELE_SIZE (256)

// Floor flags.
// uc_orig: FLOOR_SHADOW_TYPE (fallen/Headers/Map.h)
#define FLOOR_SHADOW_TYPE 7
// uc_orig: FLOOR_HIDDEN (fallen/Headers/Map.h)
#define FLOOR_HIDDEN (1 << 4)
// uc_orig: FLOOR_TRENCH (fallen/Headers/Map.h)
#define FLOOR_TRENCH (1 << 7)
// uc_orig: FLOOR_ANIM_TMAP (fallen/Headers/Map.h)
#define FLOOR_ANIM_TMAP (1 << 8)
// uc_orig: FLOOR_IS_ROOF (fallen/Headers/Map.h)
#define FLOOR_IS_ROOF (1 << 6)
// uc_orig: FLOOR_HEIGHT_SHIFT (fallen/Headers/Map.h)
#define FLOOR_HEIGHT_SHIFT (3)

// One cell in the 128x128 world map.
// Note: on PC/DDEngine, only the Colour field is actively used at runtime.
// MapWho and ColVectHead are maintained separately in the PAP high/low resolution grids.
// uc_orig: MapElement (fallen/Headers/Map.h)
typedef struct
{
    LIGHT_Colour Colour;
    SBYTE AltNotUsedAnyMore;
    UWORD FlagsNotUsedAnyMore;
    UWORD ColVectHead,
        TextureNotUsedAnyMore;
    SWORD Walkable;
    UWORD MapWho;   // THING_INDEX of first thing in this cell (linked list via Thing.NextLink)
} MapElement; // 14 bytes for pc

// Compact UV/page/rotation encoding for a floor texture.
// uc_orig: CodedTexture (fallen/Headers/Map.h)
typedef struct
{
    UWORD X : 3;
    UWORD Y : 3;
    UWORD Page : 4;
    UWORD Rot : 2;
    UWORD Flip : 2;
    UWORD Size : 2;
} CodedTexture;

// Initialise the map to zero (called at level load).
// uc_orig: init_map (fallen/Headers/Map.h)
void init_map(void);

// Lighting interface callbacks for the LIGHT subsystem.
// uc_orig: MAP_light_get_height (fallen/Headers/Map.h)
SLONG MAP_light_get_height(SLONG x, SLONG z);
// uc_orig: MAP_light_get_light (fallen/Headers/Map.h)
LIGHT_Colour MAP_light_get_light(SLONG x, SLONG z);
// uc_orig: MAP_light_set_light (fallen/Headers/Map.h)
void MAP_light_set_light(SLONG x, SLONG z, LIGHT_Colour colour);

#endif // WORLD_MAP_MAP_H
