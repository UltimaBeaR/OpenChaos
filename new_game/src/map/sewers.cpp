// Sewer/cavern subsystem — chunk 1: init, cache init, vertex/face scratch, floors, wallstrip helpers.
// See ns.h for the full sewer system API.

#include "engine/platform/uc_common.h"
#include "map/sewers.h"
#include "map/sewers_globals.h"

#include "map/pap.h"
#include "engine/physics/collide.h"

// Returns the sewer floor height (in world Y units) at world position (x, z).
// Returns -0x200 if the position is solid rock (no sewer here).
// uc_orig: NS_calc_height_at (fallen/Source/ns.cpp)
SLONG NS_calc_height_at(SLONG x, SLONG z)
{
    NS_Hi* nh;
    SLONG ans;
    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (!WITHIN(mx, 0, PAP_SIZE_HI - 1) || !WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
        return 0;
    }

    nh = &NS_hi[mx][mz];

    if (NS_HI_TYPE(nh) == NS_HI_TYPE_ROCK) {
        return -0x200;
    } else {
        ans = (nh->bot << 5) + (-32 * 0x100);
    }

    return ans;
}

// Returns the water surface height at (x,z), or the floor height if there is no water.
// uc_orig: NS_calc_splash_height_at (fallen/Source/ns.cpp)
SLONG NS_calc_splash_height_at(SLONG x, SLONG z)
{
    NS_Hi* nh;
    SLONG ans;
    SLONG mx = x >> PAP_SHIFT_HI;
    SLONG mz = z >> PAP_SHIFT_HI;

    if (!WITHIN(mx, 0, PAP_SIZE_HI - 1) || !WITHIN(mz, 0, PAP_SIZE_HI - 1)) {
        return 0;
    }

    nh = &NS_hi[mx][mz];

    if (NS_HI_TYPE(nh) == NS_HI_TYPE_ROCK) {
        return -0x200;
    } else {
        if (nh->water) {
            ans = (nh->water << 5) + (-32 * 0x100);
        } else {
            ans = (nh->bot << 5) + (-32 * 0x100);
        }
    }

    return ans;
}

// Slides the destination position (*x2,*y2,*z2) so it does not penetrate sewer geometry.
// All coordinates in 16-bit fixed point; radius is 8-bit fixed point.
// Clamps to the playable map area first, then collides against the four adjacent hi-res cells.
// uc_orig: NS_slide_along (fallen/Source/ns.cpp)
void NS_slide_along(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG radius)
{
    SLONG i;
    SLONG height;
    SLONG mx;
    SLONG mz;
    SLONG dx;
    SLONG dz;
    SLONG vx1;
    SLONG vz1;
    SLONG vx2;
    SLONG vz2;
    SLONG sx1;
    SLONG sz1;
    SLONG sx2;
    SLONG sz2;
    SLONG sradius;
    NS_Hi* nh;

    const struct {
        SBYTE dx;
        SBYTE dz;
    } order[4] = {
        { +1, 0 },
        { -1, 0 },
        { 0, +1 },
        { 0, -1 }
    };

    SLONG collide;

#define NS_SLIDE_MIN ((1 << PAP_SHIFT_LO) << 8)
#define NS_SLIDE_MAX (((PAP_SIZE_LO - 1) << PAP_SHIFT_LO) << 8)

    if (*x2 < NS_SLIDE_MIN) {
        *x2 = NS_SLIDE_MIN;
    }
    if (*x2 > NS_SLIDE_MAX) {
        *x2 = NS_SLIDE_MAX;
    }
    if (*z2 < NS_SLIDE_MIN) {
        *z2 = NS_SLIDE_MIN;
    }
    if (*z2 > NS_SLIDE_MAX) {
        *z2 = NS_SLIDE_MAX;
    }

    radius <<= 8;

    for (i = 0; i < 4; i++) {
        mx = (x1 >> 16) + order[i].dx;
        mz = (z1 >> 16) + order[i].dz;

        ASSERT(WITHIN(mx, 0, PAP_SIZE_HI - 1));
        ASSERT(WITHIN(mz, 0, PAP_SIZE_HI - 1));

        nh = &NS_hi[mx][mz];

        switch (NS_HI_TYPE(nh)) {
        case NS_HI_TYPE_ROCK:
        case NS_HI_TYPE_CURVE:
        case NS_HI_TYPE_NOTHING:
            collide = UC_TRUE;
            break;

        default:
            height = (nh->bot << (5 + 8)) + (-32 * 0x100 * 0x100);
            // Allow stepping up a quarter of a block.
            if (*y2 + 0x4000 < height) {
                collide = UC_TRUE;
            } else {
                collide = UC_FALSE;
            }
            break;
        }

        if (collide) {
            dx = order[i].dx << 7;
            dz = order[i].dz << 7;
            mx = x1 >> 16;
            mz = z1 >> 16;

            sx1 = (mx << 8) + 0x80 + dx - dz;
            sz1 = (mz << 8) + 0x80 + dz + dx;
            sx2 = (mx << 8) + 0x80 + dx + dz;
            sz2 = (mz << 8) + 0x80 + dz - dx;

            sradius = radius >> 8;
            vx1 = x1 >> 8;
            vz1 = z1 >> 8;
            vx2 = *x2 >> 8;
            vz2 = *z2 >> 8;

            if (slide_around_sausage(sx1, sz1, sx2, sz2, sradius, vx1, vz1, &vx2, &vz2)) {
                *x2 = vx2 << 8;
                *z2 = vz2 << 8;
            }
        }
    }
}

// Returns UC_TRUE if (x,y,z) is below the sewer floor — i.e., inside the rock.
// uc_orig: NS_inside (fallen/Source/ns.cpp)
SLONG NS_inside(SLONG x, SLONG y, SLONG z)
{
    return y < NS_calc_height_at(x, z);
}

// Returns UC_TRUE if the line segment from (x1,y1,z1) to (x2,y2,z2) has clear line-of-sight
// through the sewer (no rock intersections). Sets NS_los_fail_* to the last clear point on failure.
// uc_orig: NS_there_is_a_los (fallen/Source/ns.cpp)
SLONG NS_there_is_a_los(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    SLONG i;
    SLONG x;
    SLONG y;
    SLONG z;

    SLONG dx = x2 - x1;
    SLONG dy = y2 - y1;
    SLONG dz = z2 - z1;

    SLONG len = QDIST3(abs(dx), abs(dy), abs(dz));
    SLONG steps = len >> 5;

    if (len == 0) {
        return UC_TRUE;
    }

    dx = (dx << 5) / len;
    dy = (dy << 5) / len;
    dz = (dz << 5) / len;

    x = x1 + dx;
    y = y1 + dy;
    z = z1 + dz;

    for (i = 1; i < steps; i++) {
        if (NS_inside(x, y, z)) {
            NS_los_fail_x = x - dx;
            NS_los_fail_y = y - dy;
            NS_los_fail_z = z - dz;
            return UC_FALSE;
        }
    }

    return UC_TRUE;
}
