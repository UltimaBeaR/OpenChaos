#include <string.h>
#include "game/game_types.h"
#include "shooting/projectile.h"
#include "things/core/thing.h"

// uc_orig: init_projectiles (fallen/Source/Pjectile.cpp)
void init_projectiles(void)
{
    memset((UBYTE*)PROJECTILES, 0, sizeof(PROJECTILES));
    PROJECTILE_COUNT = 0;
}

// uc_orig: free_projectile (fallen/Source/Pjectile.cpp)
void free_projectile(Thing* proj_thing)
{
    proj_thing->Genus.Projectile->ProjectileType = PROJECTILE_NONE;
    free_thing(proj_thing);
}
