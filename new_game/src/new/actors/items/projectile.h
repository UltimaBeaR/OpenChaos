#ifndef ACTORS_ITEMS_PROJECTILE_H
#define ACTORS_ITEMS_PROJECTILE_H

#include "core/types.h"

// uc_orig: MAX_PROJECTILES (fallen/Headers/Pjectile.h)
#define MAX_PROJECTILES 10

// uc_orig: PROJECTILE_NONE (fallen/Headers/Pjectile.h)
#define PROJECTILE_NONE 0

// uc_orig: PROJECTILE_TEST (fallen/Headers/Pjectile.h)
#define PROJECTILE_TEST 1

// The Projectile struct requires Game.h (for THING_INDEX and the COMMON macro) to be included first.
#ifdef THING_INDEX

// uc_orig: Projectile (fallen/Headers/Pjectile.h)
typedef struct
{
    COMMON(ProjectileType)

} Projectile;

// uc_orig: ProjectilePtr (fallen/Headers/Pjectile.h)
typedef Projectile* ProjectilePtr;

#endif // THING_INDEX

struct Thing;

// uc_orig: init_projectiles (fallen/Source/Pjectile.cpp)
void init_projectiles(void);

// uc_orig: alloc_projectile (fallen/Source/Pjectile.cpp)
Thing* alloc_projectile(UBYTE type);

// uc_orig: free_projectile (fallen/Source/Pjectile.cpp)
void free_projectile(Thing* proj_thing);

#endif // ACTORS_ITEMS_PROJECTILE_H
