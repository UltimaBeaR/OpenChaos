#include <string.h>
#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "shooting/projectile.h"
#include "things/core/thing.h"
#include "things/core/statedef.h"

// uc_orig: init_projectiles (fallen/Source/Pjectile.cpp)
void init_projectiles(void)
{
    memset((UBYTE*)PROJECTILES, 0, sizeof(PROJECTILES));
    PROJECTILE_COUNT = 0;
}

// uc_orig: alloc_projectile (fallen/Source/Pjectile.cpp)
Thing* alloc_projectile(UBYTE type)
{
    SLONG c0;
    Projectile* new_proj;
    Thing* proj_thing = NULL;

    // Scan the pool for a free slot. O(n) but pool is small (MAX_PROJECTILES=10).
    for (c0 = 0; c0 < MAX_PROJECTILES; c0++) {
        if (PROJECTILES[c0].ProjectileType == PROJECTILE_NONE) {
            proj_thing = alloc_thing(CLASS_PROJECTILE);
            if (proj_thing) {
                new_proj = TO_PROJECTILE(c0);
                new_proj->ProjectileType = type;
                new_proj->Thing = THING_NUMBER(proj_thing);

                proj_thing->Genus.Projectile = new_proj;

                set_state_function(proj_thing, STATE_INIT);
            }
            break;
        }
    }
    return proj_thing;
}

// uc_orig: free_projectile (fallen/Source/Pjectile.cpp)
void free_projectile(Thing* proj_thing)
{
    proj_thing->Genus.Projectile->ProjectileType = PROJECTILE_NONE;
    free_thing(proj_thing);
}
