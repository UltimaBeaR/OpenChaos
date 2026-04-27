#ifndef ENGINE_PHYSICS_SM_GLOBALS_H
#define ENGINE_PHYSICS_SM_GLOBALS_H

#include "engine/physics/sm.h"

// File-private struct types needed to declare global arrays.
// Originally defined as file-scope typedefs in sm.cpp.

// uc_orig: SM_Sphere (fallen/Source/sm.cpp)
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
    SLONG dx; // velocity, 8-bit fixed point
    SLONG dy;
    SLONG dz;
    SLONG radius;
    SLONG mass;

} SM_Sphere;

// uc_orig: SM_MAX_SPHERES (fallen/Source/sm.cpp)
#define SM_MAX_SPHERES 1024

// uc_orig: SM_Link (fallen/Source/sm.cpp)
typedef struct
{
    UWORD sphere1;
    UWORD sphere2;
    SLONG dist; // rest length squared

} SM_Link;

// uc_orig: SM_MAX_LINKS (fallen/Source/sm.cpp)
#define SM_MAX_LINKS 1024

// uc_orig: SM_sphere (fallen/Source/sm.cpp)
extern SM_Sphere SM_sphere[SM_MAX_SPHERES];
// uc_orig: SM_sphere_upto (fallen/Source/sm.cpp)
extern SLONG SM_sphere_upto;

// uc_orig: SM_link (fallen/Source/sm.cpp)
extern SM_Link SM_link[SM_MAX_LINKS];
// uc_orig: SM_link_upto (fallen/Source/sm.cpp)
extern SLONG SM_link_upto;

// uc_orig: SM_get_upto (fallen/Source/sm.cpp)
extern SLONG SM_get_upto;
// uc_orig: SM_get_info (fallen/Source/sm.cpp)
extern SM_Info SM_get_info;

#endif // ENGINE_PHYSICS_SM_GLOBALS_H
