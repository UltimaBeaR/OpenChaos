#ifndef EFFECTS_FOG_GLOBALS_H
#define EFFECTS_FOG_GLOBALS_H

#include "effects/fog.h"

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

// uc_orig: FOG_focus_x (fallen/Source/fog.cpp)
extern SLONG FOG_focus_x;
// uc_orig: FOG_focus_z (fallen/Source/fog.cpp)
extern SLONG FOG_focus_z;
// uc_orig: FOG_focus_radius (fallen/Source/fog.cpp)
extern SLONG FOG_focus_radius;
// uc_orig: FOG_get_upto (fallen/Source/fog.cpp)
extern SLONG FOG_get_upto;

#endif // EFFECTS_FOG_GLOBALS_H
