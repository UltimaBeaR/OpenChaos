#ifndef WORLD_MAP_AZ_H
#define WORLD_MAP_AZ_H

#include "core/types.h"

// City A-Z: produces a simplified line-based representation of the current city
// for minimap or navigation purposes. Lines are categorised as road, building,
// fence, or number. Must call AZ_create_lines() after roads are sunken.

// uc_orig: AZ_LINE_TYPE_ROAD (fallen/Headers/az.h)
#define AZ_LINE_TYPE_ROAD     0
// uc_orig: AZ_LINE_TYPE_BUILDING (fallen/Headers/az.h)
#define AZ_LINE_TYPE_BUILDING 1
// uc_orig: AZ_LINE_TYPE_FENCE (fallen/Headers/az.h)
#define AZ_LINE_TYPE_FENCE    2
// uc_orig: AZ_LINE_TYPE_NUMBER (fallen/Headers/az.h)
#define AZ_LINE_TYPE_NUMBER   3

// uc_orig: AZ_Line (fallen/Headers/az.h)
typedef struct
{
    UBYTE type;
    UBYTE x1;
    UBYTE z1;
    UBYTE x2;
    UBYTE z2;

} AZ_Line;

// uc_orig: AZ_line (fallen/Source/az.cpp)
extern AZ_Line AZ_line[];
// uc_orig: AZ_line_upto (fallen/Source/az.cpp)
extern SLONG AZ_line_upto;

// uc_orig: AZ_init (fallen/Headers/az.h)
void AZ_init(void);

// uc_orig: AZ_create_lines (fallen/Headers/az.h)
// Builds the line representation from the current map. Call after road sinking.
void AZ_create_lines(void);

#endif // WORLD_MAP_AZ_H
