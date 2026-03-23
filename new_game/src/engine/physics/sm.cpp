#include <platform.h>

#include "engine/physics/sm.h"
#include "engine/physics/sm_globals.h"
#include "core/fixed_math.h"
#include "world/map/pap.h"
#include "world/map/pap_globals.h"
#include "engine/input/keyboard_globals.h"
#include "engine/graphics/pipeline/aeng.h"

// uc_orig: SM_CUBE_RES_MIN (fallen/Source/sm.cpp)
#define SM_CUBE_RES_MIN 2
// uc_orig: SM_CUBE_RES_MAX (fallen/Source/sm.cpp)
#define SM_CUBE_RES_MAX 5

// uc_orig: SM_CUBE_DENSITY_MIN (fallen/Source/sm.cpp)
#define SM_CUBE_DENSITY_MIN 0x01000
// uc_orig: SM_CUBE_DENSITY_MAX (fallen/Source/sm.cpp)
#define SM_CUBE_DENSITY_MAX 0x10000

// uc_orig: SM_CUBE_JELLYNESS_MIN (fallen/Source/sm.cpp)
#define SM_CUBE_JELLYNESS_MIN 0x10000
// uc_orig: SM_CUBE_JELLYNESS_MAX (fallen/Source/sm.cpp)
#define SM_CUBE_JELLYNESS_MAX 0x00400

// uc_orig: SM_GRAVITY (fallen/Source/sm.cpp)
#define SM_GRAVITY (-0x200)

// Resets all sphere-matter state, clearing all spheres, links, and objects.
// uc_orig: SM_init (fallen/Source/sm.cpp)
void SM_init(void)
{
    SM_sphere_upto = 0;
    SM_link_upto = 0;
    SM_object_upto = 0;
}

// Creates a cube-shaped sphere-matter object from corner (x1,y1,z1) to (x2,y2,z2).
// amount_resolution (0-256) controls grid density: more spheres = more fidelity.
// amount_density (0-256) controls mass per unit volume.
// amount_jellyness (0-256) controls spring stiffness (higher = more rigid).
// Spheres are laid out in a regular grid; each adjacent pair is connected by a link.
// uc_orig: SM_create_cube (fallen/Source/sm.cpp)
void SM_create_cube(
    SLONG cx1, SLONG cy1, SLONG cz1,
    SLONG cx2, SLONG cy2, SLONG cz2,
    SLONG amount_resolution,
    SLONG amount_density,
    SLONG amount_jellyness)
{
    SLONG i;
    SLONG j;
    SLONG k;

    SLONG i2;
    SLONG j2;
    SLONG k2;

    SLONG di;
    SLONG dj;
    SLONG dk;

    SLONG sx;
    SLONG sy;
    SLONG sz;

    SLONG sx2;
    SLONG sy2;
    SLONG sz2;

    SLONG dsx;
    SLONG dsy;
    SLONG dsz;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;

    SLONG index1;
    SLONG index2;

    SLONG radius;
    SLONG volume;
    SLONG mass;

    SM_Object* so;
    SM_Sphere* ss;
    SM_Link* sl;

    // Work in 16-bit fixed point (not 8-bit).
    cx1 <<= 8;
    cy1 <<= 8;
    cz1 <<= 8;

    cx2 <<= 8;
    cy2 <<= 8;
    cz2 <<= 8;

    SLONG resolution = SM_CUBE_RES_MIN + (amount_resolution * (SM_CUBE_RES_MAX - SM_CUBE_RES_MIN) >> 8);
    SLONG density = SM_CUBE_DENSITY_MIN + (amount_density * (SM_CUBE_DENSITY_MAX - SM_CUBE_DENSITY_MIN) >> 8);
    SLONG jellyness = SM_CUBE_JELLYNESS_MIN + (amount_jellyness * (SM_CUBE_JELLYNESS_MAX - SM_CUBE_JELLYNESS_MIN) >> 8);

    resolution = amount_resolution;
    density = amount_density;
    jellyness = amount_jellyness;

    dx = cx2 - cx1;
    dy = cy2 - cy1;
    dz = cz2 - cz1;

    volume = MUL64(MUL64(dx, dy), dz);
    mass = MUL64(volume, density);
    mass /= resolution * resolution * resolution;

    dx = (cx2 - cx1) / resolution;
    dy = (cy2 - cy1) / resolution;
    dz = (cz2 - cz1) / resolution;

    radius = QDIST3(abs(dx >> 1), abs(dy >> 1), abs(dz >> 1));

    ASSERT(WITHIN(SM_object_upto, 0, SM_MAX_OBJECTS - 1));

    so = &SM_object[SM_object_upto++];

    so->sphere_index = SM_sphere_upto;
    so->sphere_num = 0;
    so->link_index = SM_link_upto;
    so->link_num = 0;
    so->jellyness = jellyness;
    so->resolution = resolution;
    so->density = density;

    for (i = 0; i < resolution; i++)
        for (j = 0; j < resolution; j++)
            for (k = 0; k < resolution; k++) {
                sx = cx1 + i * dx + (dx >> 1);
                sy = cy1 + j * dy + (dy >> 1);
                sz = cz1 + k * dx + (dx >> 1);

                ASSERT(WITHIN(SM_sphere_upto, 0, SM_MAX_SPHERES - 1));

                ss = &SM_sphere[SM_sphere_upto++];

                ss->x = sx;
                ss->y = sy;
                ss->z = sz;
                ss->dx = 0;
                ss->dy = 0;
                ss->dz = 0;
                ss->radius = radius;
                ss->mass = mass;

                so->sphere_num += 1;

                // Add spring links to all adjacent spheres with higher index.
                index1 = k + (j * resolution) + (i * resolution * resolution);

                for (di = 0; di < 2; di++)
                    for (dj = 0; dj < 2; dj++)
                        for (dk = 0; dk < 2; dk++) {
                            i2 = i + di;
                            j2 = j + dj;
                            k2 = k + dk;

                            if (i2 >= resolution || j2 >= resolution || k2 >= resolution) {
                                continue;
                            }

                            index2 = k2 + (j2 * resolution) + (i2 * resolution * resolution);

                            sx2 = cx1 + i2 * dx + (dx >> 1);
                            sy2 = cy1 + j2 * dy + (dy >> 1);
                            sz2 = cz1 + k2 * dx + (dx >> 1);

                            dsx = sx2 - sx;
                            dsy = sy2 - sy;
                            dsz = sz2 - sz;

                            dist = MUL64(dsx, dsx);
                            dist += MUL64(dsy, dsy);
                            dist += MUL64(dsz, dsz);

                            ASSERT(WITHIN(SM_link_upto, 0, SM_MAX_LINKS - 1));

                            sl = &SM_link[SM_link_upto++];

                            sl->sphere1 = index1;
                            sl->sphere2 = index2;
                            sl->dist = dist;

                            so->link_num += 1;
                        }
            }
}

// Advances sphere-matter simulation by one tick.
// Applies gravity, movement, ground collision, and friction to all spheres.
// Then applies spring forces between linked sphere pairs.
// Also responds to KB_P5 key for debug impulse on sphere 8.
// uc_orig: SM_process (fallen/Source/sm.cpp)
void SM_process()
{
    SLONG i;
    SLONG j;
    SLONG height;

    SLONG ax;
    SLONG ay;
    SLONG az;

    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG ddist;
    SLONG force;
    SLONG accel;

    SM_Object* so;
    SM_Sphere* ss;
    SM_Sphere* ss1;
    SM_Sphere* ss2;
    SM_Link* sl;

    (void)ax; (void)ay; (void)az;

    for (i = 0; i < SM_sphere_upto; i++) {
        ss = &SM_sphere[i];

        ss->dy += SM_GRAVITY;

        ss->x += ss->dx / 256;
        ss->y += ss->dy / 256;
        ss->z += ss->dz / 256;

        // Ground collision: bounce off terrain height.
        height = PAP_calc_height_at(ss->x >> 8, ss->z >> 8) << 8;

        if (ss->y - ss->radius < height) {
            ss->y = ss->radius + height;
            ss->dy = -abs(ss->dy);
            ss->dy -= ss->dy >> 9;
            ss->dx /= 2;
            ss->dz /= 2;
        }

        // Friction (damping).
        ss->dx -= ss->dx / 256;
        ss->dy -= ss->dy / 256;
        ss->dz -= ss->dz / 256;

        ss->dx -= ss->dx / 512;
        ss->dy -= ss->dy / 512;
        ss->dz -= ss->dz / 512;

        ss->dx -= SIGN(ss->dx);
        ss->dy -= SIGN(ss->dy);
        ss->dz -= SIGN(ss->dz);
    }

    for (i = 0; i < SM_object_upto; i++) {
        so = &SM_object[i];

        for (j = 0; j < SM_link_upto; j++) {
            sl = &SM_link[j];

            ASSERT(WITHIN(sl->sphere1, 0, SM_sphere_upto - 1));
            ASSERT(WITHIN(sl->sphere2, 0, SM_sphere_upto - 1));

            ss1 = &SM_sphere[sl->sphere1];
            ss2 = &SM_sphere[sl->sphere2];

            dx = ss2->x - ss1->x;
            dy = ss2->y - ss1->y;
            dz = ss2->z - ss1->z;

            dist = MUL64(dx, dx);
            dist += MUL64(dy, dy);
            dist += MUL64(dz, dz);

            ddist = dist - sl->dist;

            if (ddist) {
                force = MUL64(ddist, so->jellyness);

                // Equal and opposite spring forces on both spheres.
                accel = DIV64(force, ss1->mass);

                ss1->dx += MUL64(dx, accel);
                ss1->dy += MUL64(dy, accel);
                ss1->dz += MUL64(dz, accel);

                accel = DIV64(force, ss2->mass);

                ss2->dx -= MUL64(dx, accel);
                ss2->dy -= MUL64(dy, accel);
                ss2->dz -= MUL64(dz, accel);
            }
        }
    }

    // Debug: KB_P5 applies upward impulse to sphere 8.
    if (Keys[KB_P5]) {
        SM_sphere[8].dy -= SM_GRAVITY * 50;
    }
}

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
