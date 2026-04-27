#ifndef MAP_MAP_CONSTANTS_H
#define MAP_MAP_CONSTANTS_H

// Editor-side map representation — used at load time and in collision code.
// Original source: fallen/Editor/Headers/map.h (renamed to edmap.h to avoid
// conflict with standard/system headers named "map.h").

#include "engine/core/types.h"

// uc_orig: ELE_SHIFT (fallen/Editor/Headers/map.h)
#undef ELE_SIZE
#define ELE_SHIFT (8)
// uc_orig: ELE_SIZE (fallen/Editor/Headers/map.h)
#define ELE_SIZE (1 << ELE_SHIFT)
// uc_orig: HALF_ELE_SIZE (fallen/Editor/Headers/map.h)
#define HALF_ELE_SIZE ((ELE_SIZE) >> 1)
// uc_orig: ELE_AND (fallen/Editor/Headers/map.h)
#define ELE_AND (0xffffff00)

// uc_orig: EDIT_MAP_WIDTH (fallen/Editor/Headers/map.h)
#define EDIT_MAP_WIDTH 128
// uc_orig: EDIT_MAP_HEIGHT (fallen/Editor/Headers/map.h)
// Note: height is 1 (flat map), depth is the second spatial dimension.
#define EDIT_MAP_HEIGHT 1
// uc_orig: EDIT_MAP_DEPTH (fallen/Editor/Headers/map.h)
#define EDIT_MAP_DEPTH 128

// uc_orig: DepthStrip (fallen/Editor/Headers/map.h)
// One cell of the editor map, storing texture, geometry, and walkability data.
struct DepthStrip {
    UWORD MapThingIndex;
    UWORD ColVectHead;
    UWORD Texture;
    SWORD Bright;
    UBYTE Flags;
    SBYTE Y;
    SWORD Walkable;
};

// uc_orig: edit_map (fallen/Editor/Headers/map.h)
extern struct DepthStrip edit_map[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];
// uc_orig: edit_map_roof_height (fallen/Editor/Headers/map.h)
extern SBYTE edit_map_roof_height[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];
// uc_orig: tex_map (fallen/Editor/Headers/map.h)
extern UWORD tex_map[EDIT_MAP_WIDTH][EDIT_MAP_DEPTH];

#endif // MAP_MAP_CONSTANTS_H
