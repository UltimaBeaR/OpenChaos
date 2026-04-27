#ifndef ENGINE_PHYSICS_SM_H
#define ENGINE_PHYSICS_SM_H

#include "engine/core/types.h"

// Sphere-Matter: a jelly/physics simulation using masses connected by elastic links.
// Objects are composed of spheres (SM_Sphere) connected by spring-like links (SM_Link).
// Used for jelly-like deformable objects in the game world.

// uc_orig: SM_Info (fallen/Headers/sm.h)
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
    SLONG radius;
    ULONG colour;

} SM_Info;

// uc_orig: SM_get_start (fallen/Headers/sm.h)
void SM_get_start(void);

// uc_orig: SM_get_next (fallen/Headers/sm.h)
SM_Info* SM_get_next(void);

#endif // ENGINE_PHYSICS_SM_H
