#ifndef EFFECTS_WEATHER_FOG_GLOBALS_H
#define EFFECTS_WEATHER_FOG_GLOBALS_H

#include "engine/core/types.h"
#include "effects/weather/fog.h"

// uc_orig: FOG_MAX_FOG (fallen/Source/fog.cpp)
#define FOG_MAX_FOG 2048

// uc_orig: FOG_Fog (fallen/Source/fog.cpp)
// Internal fog patch state (not exposed in the public API; FOG_Info is the draw-side view).
typedef struct
{
    UBYTE type;    // FOG_TYPE_* or FOG_TYPE_UNUSED
    SBYTE dyaw;    // current angular velocity (signed, clamped ±127)
    SWORD yaw;     // current orientation (0–2047)
    UWORD size;    // patch radius
    UWORD shit;    // unused padding (original field name preserved)
    SLONG x;
    SLONG y;
    SLONG z;
} FOG_Fog;

// uc_orig: FOG_fog (fallen/Source/fog.cpp)
extern FOG_Fog FOG_fog[FOG_MAX_FOG];


#endif // EFFECTS_WEATHER_FOG_GLOBALS_H
