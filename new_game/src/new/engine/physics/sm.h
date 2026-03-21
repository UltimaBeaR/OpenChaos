#ifndef ENGINE_PHYSICS_SM_H
#define ENGINE_PHYSICS_SM_H

#include "core/types.h"

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

// uc_orig: SM_CUBE_TYPE_ROCK (fallen/Headers/sm.h)
#define SM_CUBE_TYPE_ROCK 0
// uc_orig: SM_CUBE_TYPE_JELLY (fallen/Headers/sm.h)
#define SM_CUBE_TYPE_JELLY 1
// uc_orig: SM_CUBE_TYPE_CARDBOARD (fallen/Headers/sm.h)
#define SM_CUBE_TYPE_CARDBOARD 2
// uc_orig: SM_CUBE_TYPE_NUMBER (fallen/Headers/sm.h)
#define SM_CUBE_TYPE_NUMBER 3

// uc_orig: SM_init (fallen/Headers/sm.h)
void SM_init(void);

// uc_orig: SM_create_cube (fallen/Headers/sm.h)
void SM_create_cube(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2,
    SLONG amount_resolution, // 0-256
    SLONG amount_density,    // 0-256
    SLONG amount_jellyness); // 0-256

// uc_orig: SM_process (fallen/Headers/sm.h)
void SM_process(void);

// uc_orig: SM_get_start (fallen/Headers/sm.h)
void SM_get_start(void);

// uc_orig: SM_get_next (fallen/Headers/sm.h)
SM_Info* SM_get_next(void);

#endif // ENGINE_PHYSICS_SM_H
