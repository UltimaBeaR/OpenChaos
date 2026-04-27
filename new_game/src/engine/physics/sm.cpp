#include "engine/platform/uc_common.h"

#include "engine/physics/sm.h"
#include "engine/physics/sm_globals.h"
#include "engine/graphics/pipeline/aeng.h"

// Resets the iteration cursor to begin enumerating all spheres via SM_get_next().
// When ControlFlag is set, also draws debug lines for all spring links.
// uc_orig: SM_get_start (fallen/Source/sm.cpp)
void SM_get_start()
{
    SM_get_upto = 0;

    if (ControlFlag) {
        SLONG i;

        SM_Link* sl;

        SM_Sphere* ss1;
        SM_Sphere* ss2;

        for (i = 0; i < SM_link_upto; i++) {
            sl = &SM_link[i];

            ASSERT(WITHIN(sl->sphere1, 0, SM_sphere_upto - 1));
            ASSERT(WITHIN(sl->sphere2, 0, SM_sphere_upto - 1));

            ss1 = &SM_sphere[sl->sphere1];
            ss2 = &SM_sphere[sl->sphere2];

            AENG_world_line(
                ss1->x >> 8, ss1->y >> 8, ss1->z >> 8, 16, 0x00ff0000,
                ss2->x >> 8, ss2->y >> 8, ss2->z >> 8, 1, 0x0000ff00,
                UC_TRUE);
        }
    }
}

// Returns the next sphere's render info (position, radius, colour) for the current frame.
// Returns NULL when all spheres have been visited.
// Colour is a hash of the sphere index (deterministic per-sphere colour for debug rendering).
// uc_orig: SM_get_next (fallen/Source/sm.cpp)
SM_Info* SM_get_next()
{
    SM_Sphere* ss;

    if (!WITHIN(SM_get_upto, 0, SM_sphere_upto - 1)) {
        return NULL;
    }

    ss = &SM_sphere[SM_get_upto++];

    SM_get_info.x = ss->x >> 8;
    SM_get_info.y = ss->y >> 8;
    SM_get_info.z = ss->z >> 8;
    SM_get_info.radius = ss->radius >> 8;
    SM_get_info.colour = 123456789 * SM_get_upto + 3141592653;

    return &SM_get_info;
}
