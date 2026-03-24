#ifndef EFFECTS_WEATHER_DRIP_GLOBALS_H
#define EFFECTS_WEATHER_DRIP_GLOBALS_H

#include "effects/weather/drip.h"

// uc_orig: DRIP_MAX_DRIPS (fallen/Source/drip.cpp)
#define DRIP_MAX_DRIPS 1024

// Internal drip structure. Layout matches DRIP_Info so that a DRIP_Drip*
// can be cast to DRIP_Info* in DRIP_get_next (original does this).
// uc_orig: DRIP_Drip (fallen/Source/drip.cpp)
typedef struct
{
    UWORD x;
    SWORD y;
    UWORD z;
    UBYTE size;
    UBYTE fade; // 0 = no drip (inactive)
    UBYTE flags;
} DRIP_Drip;

// All drip slots. Allocation wraps around via power-of-two masking.
// uc_orig: DRIP_drip (fallen/Source/drip.cpp)
extern DRIP_Drip DRIP_drip[DRIP_MAX_DRIPS];

// Round-robin write cursor.
// uc_orig: DRIP_last (fallen/Source/drip.cpp)
extern SLONG DRIP_last;

// Iterator read cursor for DRIP_get_start / DRIP_get_next.
// uc_orig: DRIP_get_upto (fallen/Source/drip.cpp)
extern SLONG DRIP_get_upto;

#endif // EFFECTS_WEATHER_DRIP_GLOBALS_H
