#include "navigation/inside2.h"
#include "navigation/inside2_globals.h"
#include "buildings/building_types.h" // STOREY_TYPE_*, FACET_FLAG_*, FBuilding, FStorey, FWall, STAIR_FLAG_*, etc.
#include "map/level_pools.h"

// Forward declaration of slide_door (defined in door.cpp, not yet fully migrated here).
extern SLONG slide_door;

// uc_orig: find_inside_room (fallen/Source/inside2.cpp)
SLONG find_inside_room(SLONG inside, SLONG x, SLONG z)
{
    UBYTE* rooms;
    SWORD width;

    if (x >= inside_storeys[inside].MaxX || z >= inside_storeys[inside].MaxZ)
        return 0;
    if (x < inside_storeys[inside].MinX || z < inside_storeys[inside].MinZ)
        return 0;

    width = inside_storeys[inside].MaxX - inside_storeys[inside].MinX;
    rooms = &inside_block[inside_storeys[inside].InsideBlock];

    x -= inside_storeys[inside].MinX;
    z -= inside_storeys[inside].MinZ;

    z = z * width;
    return (rooms[x + z] & 0xf);
}

// uc_orig: find_inside_flags (fallen/Source/inside2.cpp)
SLONG find_inside_flags(SLONG inside, SLONG x, SLONG z)
{
    UBYTE* rooms;
    SWORD width;

    if (x >= inside_storeys[inside].MaxX || z >= inside_storeys[inside].MaxZ)
        return 0;
    if (x < inside_storeys[inside].MinX || z < inside_storeys[inside].MinZ)
        return 0;

    width = inside_storeys[inside].MaxX - inside_storeys[inside].MinX;
    rooms = &inside_block[inside_storeys[inside].InsideBlock];

    x -= inside_storeys[inside].MinX;
    z -= inside_storeys[inside].MinZ;

    z = z * width;
    return rooms[x + z];
}

// uc_orig: get_inside_alt (fallen/Source/inside2.cpp)
SLONG get_inside_alt(SLONG inside)
{
    return inside_storeys[inside].StoreyY;
}

// uc_orig: person_slide_inside (fallen/Source/inside2.cpp)
SLONG person_slide_inside(
    SLONG inside,
    SLONG x1,
    SLONG y1,
    SLONG z1,
    SLONG* x2,
    SLONG* y2,
    SLONG* z2)
{
    SLONG dx;
    SLONG dz;
    SLONG dist;

    SWORD mx = *x2 >> 16;
    SWORD mz = *z2 >> 16;
    SLONG ret_val = 0;

    slide_door = 0;
    slide_inside_stair = 0;

    // Read all neighbouring cell flags.
    UBYTE id_here = find_inside_flags(inside, mx, mz);
    UBYTE id_xs = find_inside_flags(inside, mx - 1, mz);
    UBYTE id_xl = find_inside_flags(inside, mx + 1, mz);
    UBYTE id_zs = find_inside_flags(inside, mx, mz - 1);
    UBYTE id_zl = find_inside_flags(inside, mx, mz + 1);

    if ((id_here & 0xf) == 0) {
        // Person is outside a permitted room area — may have exited a door.
        ret_val = 1;
    }

    // Half-width of the collision cylinder at each wall (sub-pixel units).
#define INSIDE_RADIUS (50 << 8)

    // Bitmask of walls the person is colliding against this tick.
    UBYTE col = 0;

#define INSIDE_COL_XS (1 << 0)
#define INSIDE_COL_XL (1 << 1)
#define INSIDE_COL_ZS (1 << 2)
#define INSIDE_COL_ZL (1 << 3)

    if ((id_here & 0xf) != (id_xs & 0xf))
        col |= INSIDE_COL_XS;
    if ((id_here & 0xf) != (id_xl & 0xf))
        col |= INSIDE_COL_XL;
    if ((id_here & 0xf) != (id_zs & 0xf))
        col |= INSIDE_COL_ZS;
    if ((id_here & 0xf) != (id_zl & 0xf))
        col |= INSIDE_COL_ZL;

    // Slide against each wall boundary, respecting door-frame ellipses.

    if (col & INSIDE_COL_XS) {
        if ((*x2 & 0xffff) < INSIDE_RADIUS) {
            if (id_here & FLAG_DOOR_LEFT) {
                // Door frame: ellipse at each end of the mapsquare.
                if ((*z2 & 0xffff) > 0x8000) {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dz = 0x10000 - dz;
                    dz >>= 1;
                    dist = QDIST2(dx, dz);
                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }
                    dz <<= 1;
                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;
                    *x2 |= dx;
                    *z2 |= 0x10000 - dz;
                } else {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dz >>= 1;
                    dist = QDIST2(dx, dz);
                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }
                    dz <<= 1;
                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;
                    *x2 |= dx;
                    *z2 |= dz;
                }
            } else {
                *x2 &= ~0xffff;
                *x2 |= INSIDE_RADIUS;
            }
        }
    }

    if (col & INSIDE_COL_XL) {
        if ((*x2 & 0xffff) > 0x10000 - INSIDE_RADIUS) {
            if (id_xl & FLAG_DOOR_LEFT) {
                if ((*z2 & 0xffff) > 0x8000) {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dx = 0x10000 - dx;
                    dz = 0x10000 - dz;
                    dz >>= 1;
                    dist = QDIST2(dx, dz);
                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }
                    dz <<= 1;
                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;
                    *x2 |= 0x10000 - dx;
                    *z2 |= 0x10000 - dz;
                } else {
                    dx = *x2 & 0xffff;
                    dx = 0x10000 - dx;
                    dz = *z2 & 0xffff;
                    dz >>= 1;
                    dist = QDIST2(dx, dz);
                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }
                    dz <<= 1;
                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;
                    *x2 |= 0x10000 - dx;
                    *z2 |= dz;
                }
            } else {
                *x2 &= ~0xffff;
                *x2 |= 0x10000 - INSIDE_RADIUS;
            }
        }
    }

    if (col & INSIDE_COL_ZS) {
        if ((*z2 & 0xffff) < INSIDE_RADIUS) {
            if (id_here & FLAG_DOOR_UP) {
                if ((*x2 & 0xffff) > 0x8000) {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dx = 0x10000 - dx;
                    dx >>= 1;
                    dist = QDIST2(dx, dz);
                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }
                    dx <<= 1;
                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;
                    *x2 |= 0x10000 - dx;
                    *z2 |= dz;
                } else {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dx >>= 1;
                    dist = QDIST2(dx, dz);
                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }
                    dx <<= 1;
                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;
                    *x2 |= dx;
                    *z2 |= dz;
                }
            } else {
                *z2 &= ~0xffff;
                *z2 |= INSIDE_RADIUS;
            }
        }
    }

    if (col & INSIDE_COL_ZL) {
        if ((*z2 & 0xffff) > 0x10000 - INSIDE_RADIUS) {
            if (id_zl & FLAG_DOOR_UP) {
                if ((*x2 & 0xffff) > 0x8000) {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dx = 0x10000 - dx;
                    dz = 0x10000 - dz;
                    dx >>= 1;
                    dist = QDIST2(dx, dz);
                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }
                    dx <<= 1;
                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;
                    *x2 |= 0x10000 - dx;
                    *z2 |= 0x10000 - dz;
                } else {
                    dx = *x2 & 0xffff;
                    dz = *z2 & 0xffff;
                    dz = 0x10000 - dz;
                    dx >>= 1;
                    dist = QDIST2(dx, dz);
                    if (dist < INSIDE_RADIUS) {
                        dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                        dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                    }
                    dx <<= 1;
                    *x2 &= ~0xffff;
                    *z2 &= ~0xffff;
                    *x2 |= dx;
                    *z2 |= 0x10000 - dz;
                }
            } else {
                *z2 &= ~0xffff;
                *z2 |= 0x10000 - INSIDE_RADIUS;
            }
        }
    }

    // Corner convex-corner sliding (sticky-outy bits).

    if (!(col & (INSIDE_COL_XS | INSIDE_COL_ZS))) {
        if ((id_here & 0xf) != (find_inside_flags(inside, mx - 1, mz - 1) & 0xf)) {
            dx = *x2 & 0xffff;
            dz = *z2 & 0xffff;
            dist = QDIST2(dx, dz);
            if (dist < INSIDE_RADIUS) {
                dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
            }
            *x2 &= ~0xffff;
            *z2 &= ~0xffff;
            *x2 |= dx;
            *z2 |= dz;
        }
    }

    if (!(col & (INSIDE_COL_XS | INSIDE_COL_ZL))) {
        if ((id_here & 0xf) != (find_inside_flags(inside, mx - 1, mz + 1) & 0xf)) {
            dx = *x2 & 0xffff;
            dz = *z2 & 0xffff;
            dz = 0x10000 - dz;
            dist = QDIST2(dx, dz);
            if (dist < INSIDE_RADIUS) {
                dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
            }
            *x2 &= ~0xffff;
            *z2 &= ~0xffff;
            *x2 |= dx;
            *z2 |= 0x10000 - dz;
        }
    }

    if (!(col & (INSIDE_COL_XL | INSIDE_COL_ZS))) {
        if ((id_here & 0xf) != (find_inside_flags(inside, mx + 1, mz - 1) & 0xf)) {
            dx = *x2 & 0xffff;
            dz = *z2 & 0xffff;
            dx = 0x10000 - dx;
            dist = QDIST2(dx, dz);
            if (dist < INSIDE_RADIUS) {
                dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
            }
            *x2 &= ~0xffff;
            *z2 &= ~0xffff;
            *x2 |= 0x10000 - dx;
            *z2 |= dz;
        }
    }

    if (!(col & (INSIDE_COL_XL | INSIDE_COL_ZL))) {
        if ((id_here & 0xf) != (find_inside_flags(inside, mx + 1, mz + 1) & 0xf)) {
            dx = *x2 & 0xffff;
            dz = *z2 & 0xffff;
            dx = 0x10000 - dx;
            dz = 0x10000 - dz;
            dist = QDIST2(dx, dz);
            if (dist < INSIDE_RADIUS) {
                dx = dx * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
                dz = dz * (INSIDE_RADIUS >> 8) / ((dist >> 8) + 1);
            }
            *x2 &= ~0xffff;
            *z2 &= ~0xffff;
            *x2 |= 0x10000 - dx;
            *z2 |= 0x10000 - dz;
        }
    }

    // Check if the final position is on a stair tile.
    {
        UBYTE id_final = find_inside_flags(inside, *x2 >> 16, *z2 >> 16);
        if (id_final & FLAG_INSIDE_STAIR)
            slide_inside_stair = id_final & FLAG_INSIDE_STAIR;
    }

    return ret_val;
}

