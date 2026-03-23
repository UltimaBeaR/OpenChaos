#ifndef EFFECTS_FOG_H
#define EFFECTS_FOG_H

#include "core/types.h"

// Fog system: manages a pool of animated fog sprites around a focus point.
// Fog exists only within the active radius; particles outside are recycled
// back to the edge. Wind gusts can spin individual fog patches.

// uc_orig: FOG_TYPE_TRANS1 (fallen/Headers/fog.h)
#define FOG_TYPE_TRANS1 0
// uc_orig: FOG_TYPE_TRANS2 (fallen/Headers/fog.h)
#define FOG_TYPE_TRANS2 1
// uc_orig: FOG_TYPE_TRANS3 (fallen/Headers/fog.h)
#define FOG_TYPE_TRANS3 2
// uc_orig: FOG_TYPE_TRANS4 (fallen/Headers/fog.h)
#define FOG_TYPE_TRANS4 3
// uc_orig: FOG_TYPE_NO_MORE (fallen/Headers/fog.h)
// Returned by FOG_get_info when all fog has been iterated.
#define FOG_TYPE_NO_MORE 4
// uc_orig: FOG_TYPE_UNUSED (fallen/Headers/fog.h)
#define FOG_TYPE_UNUSED 4

// uc_orig: FOG_Info (fallen/Headers/fog.h)
// Per-fog-sprite draw descriptor returned by the iterator.
typedef struct
{
    UBYTE type;   // FOG_TYPE_* (transparency level)
    UBYTE trans;  // transparency (0 = more transparent)
    UWORD size;   // radius of the fog patch
    UWORD yaw;    // current rotation angle
    UWORD shit;   // unused padding (original field name preserved)
    SLONG x;
    SLONG y;
    SLONG z;
} FOG_Info;

// uc_orig: FOG_init (fallen/Source/fog.cpp)
// Reset the fog pool: mark all entries as unused.
void FOG_init(void);

// uc_orig: FOG_set_focus (fallen/Source/fog.cpp)
// Update the fog focus point. Fog patches outside the new radius are
// recycled and re-created at the edge. Unused slots are filled immediately.
void FOG_set_focus(SLONG x, SLONG z, SLONG radius);

// uc_orig: FOG_gust (fallen/Source/fog.cpp)
// Apply a wind gust from (gx1,gz1) toward (gx2,gz2) to nearby fog patches.
// Fog within range gets an angular velocity kick proportional to the cross product.
void FOG_gust(SLONG gx1, SLONG gz1, SLONG gx2, SLONG gz2);

// uc_orig: FOG_process (fallen/Source/fog.cpp)
// Advance fog simulation: rotate each patch and damp angular velocity toward
// its natural spin rate (derived from the patch index).
void FOG_process(void);

// uc_orig: FOG_get_start (fallen/Source/fog.cpp)
// Reset the fog draw iterator.
void FOG_get_start(void);

// uc_orig: FOG_get_info (fallen/Source/fog.cpp)
// Return draw info for the next fog patch. Returns FOG_TYPE_NO_MORE when done.
FOG_Info FOG_get_info(void);

#endif // EFFECTS_FOG_H
