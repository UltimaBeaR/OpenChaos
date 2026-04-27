#ifndef THINGS_ITEMS_BARREL_H
#define THINGS_ITEMS_BARREL_H

#include "engine/core/types.h"
#include "engine/core/vector.h"

struct Thing;

// uc_orig: BARREL_MAX_SPHERES (fallen/Headers/barrel.h)
#define BARREL_MAX_SPHERES 80

// uc_orig: BARREL_MAX_BARRELS (fallen/Headers/barrel.h)
#define BARREL_MAX_BARRELS 300

// uc_orig: TO_BARREL (fallen/Headers/barrel.h)
#define TO_BARREL(t) (&BARREL_barrel[t])

// uc_orig: BARREL_NUMBER (fallen/Headers/barrel.h)
#define BARREL_NUMBER(t) (COMMON_INDEX)(t - TO_BARREL(0))

// uc_orig: BARREL_Sphere (fallen/Headers/barrel.h)
// One physics sphere (each barrel uses two, one per end).
typedef struct
{
    SLONG x; // -UC_INFINITY means unused
    SLONG y;
    SLONG z;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SWORD still; // Number of gameturns below movement threshold.
    UWORD radius; // In fine coordinate space (note: stored as UWORD).
} BARREL_Sphere;

// uc_orig: Barrel (fallen/Headers/barrel.h)
struct Barrel {
    UBYTE type;
    UBYTE flag;
    UWORD on; // For stacked barrels: index of the barrel we rest on (NULL = on ground).
    UWORD bs; // For moving barrels: index into BARREL_sphere[] for this barrel's 2 spheres.
};

// uc_orig: BarrelPtr (fallen/Headers/barrel.h)
typedef Barrel* BarrelPtr;

// uc_orig: BARREL_TYPE_NORMAL (fallen/Headers/barrel.h)
#define BARREL_TYPE_NORMAL 0
// uc_orig: BARREL_TYPE_CONE (fallen/Headers/barrel.h)
#define BARREL_TYPE_CONE 1
// uc_orig: BARREL_TYPE_BURNING (fallen/Headers/barrel.h)
#define BARREL_TYPE_BURNING 2
// uc_orig: BARREL_TYPE_BIN (fallen/Headers/barrel.h)
#define BARREL_TYPE_BIN 3

// uc_orig: BARREL_sphere (fallen/Headers/barrel.h)
extern BARREL_Sphere* BARREL_sphere;
// uc_orig: BARREL_barrel (fallen/Headers/barrel.h)
extern Barrel* BARREL_barrel;
// uc_orig: BARREL_sphere_last (fallen/Headers/barrel.h)
extern SLONG BARREL_sphere_last;
// uc_orig: BARREL_barrel_upto (fallen/Headers/barrel.h)
extern SLONG BARREL_barrel_upto;

// uc_orig: BARREL_init (fallen/Headers/barrel.h)
// Clears all barrel and sphere data, resets counters.
void BARREL_init(void);

// uc_orig: BARREL_alloc (fallen/Headers/barrel.h)
// Creates a new barrel at (x, z) with the given prim mesh.
// Stacks on top of an existing barrel if one is nearby.
// waypoint is the eway index that spawned this barrel, or NULL.
UWORD BARREL_alloc(SLONG type, SLONG prim, SLONG x, SLONG z, SLONG waypoint);

// uc_orig: BARREL_fire_pos (fallen/Headers/barrel.h)
// Returns position of a burning barrel's base sphere (for flame attachment).
GameCoord BARREL_fire_pos(Thing* p_barrel);

// uc_orig: BARREL_hit_with_prim (fallen/Headers/barrel.h)
// Knocks barrels inside a rotating prim's bounding box.
void BARREL_hit_with_prim(SLONG prim, SLONG x, SLONG y, SLONG z, SLONG yaw);

// uc_orig: BARREL_hit_with_sphere (fallen/Headers/barrel.h)
// Knocks all barrels overlapping the given sphere. Converts stacked barrels to moving.
void BARREL_hit_with_sphere(SLONG x, SLONG y, SLONG z, SLONG radius);

// uc_orig: BARREL_shoot (fallen/Headers/barrel.h)
// Handles a barrel being shot: plays sound, triggers explosion or cone bounce.
void BARREL_shoot(Thing* p_barrel, Thing* p_shooter);

// uc_orig: BARREL_dissapear (fallen/Source/barrel.cpp)
// Removes a barrel from the map and frees its Thing/DrawMesh.
// Cascades to barrels stacked on top of this one.
void BARREL_dissapear(Thing* p_barrel);

// uc_orig: BARREL_is_stacked (fallen/Source/barrel.cpp)
// Returns nonzero if the barrel is in STACKED state (on top of another barrel).
SLONG BARREL_is_stacked(Thing* p_barrel);

// Note: BARREL_position_on_hands and BARREL_throw are declared in the original barrel.h
// but their implementations are in a /* */ dead-code block in barrel.cpp (pre-release).
// uc_orig: BARREL_position_on_hands (fallen/Headers/barrel.h)
void BARREL_position_on_hands(Thing* p_barrel, Thing* p_person);
// uc_orig: BARREL_throw (fallen/Headers/barrel.h)
void BARREL_throw(Thing* p_barrel);

#endif // THINGS_ITEMS_BARREL_H
