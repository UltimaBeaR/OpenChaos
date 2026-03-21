// Partially migrated to new/engine/lighting/night.cpp + new/engine/lighting/night_globals.cpp
// Remaining: NIGHT_dfcache_recalc, NIGHT_dfcache_create, NIGHT_dfcache_destroy,
//   NIGHT_get_light_at, NIGHT_find, NIGHT_init,
//   calc_lighting__for_point, NIGHT_generate_roof_walkable, NIGHT_generate_walkable_lighting,
//   NIGHT_destroy_all_cached_info, NIGHT_load_ed_file

// Game.h first: brings in MFStdLib.h and string.h via the anim.h chain.
#include "fallen/Headers/Game.h"
#include "fallen/Headers/heap.h"
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"
#include "fallen/Headers/ob.h"
#include "fallen/Headers/pap.h"
#include "core/fmatrix.h"
#include "fallen/Headers/supermap.h"
#include "fallen/Headers/ed.h"
#include "fallen/Headers/memory.h"
#include "fallen/Headers/ware.h"
#include "fallen/Headers/mav.h"
#include "fallen/Headers/prim.h"
#include "engine/io/file.h"

void NIGHT_dfcache_recalc()
{
    SLONG dfcache;
    SLONG next;
    SLONG dfacet;

    for (dfcache = NIGHT_dfcache_used; dfcache; dfcache = next) {
        ASSERT(WITHIN(dfcache, 1, NIGHT_MAX_DFCACHES - 1));

        dfacet = NIGHT_dfcache[dfcache].dfacet;
        next = NIGHT_dfcache[dfcache].next;

        ASSERT(WITHIN(dfacet, 1, next_dfacet - 1));

        NIGHT_dfcache_destroy(dfcache);

        dfacets[dfacet].Dfcache = NIGHT_dfcache_create(dfacet);
    }
}

UBYTE NIGHT_dfcache_create(UWORD dfacet_index)
{
    SLONG i;
    SLONG j;
    SLONG k;

    SLONG lx;
    SLONG ly;
    SLONG lz;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG nx;
    SLONG ny;
    SLONG nz;

    SLONG x;
    SLONG y;
    SLONG z;

    SLONG dsx1;
    SLONG dsy1;
    SLONG dsz1;

    SLONG dsx2;
    SLONG dsy2;
    SLONG dsz2;

    SLONG dlx;
    SLONG dly;
    SLONG dlz;

    SLONG dprod;
    SLONG length;
    SLONG height;
    SLONG num_points;
    SLONG num_faces;
    SLONG num_bytes;

    SLONG base_red;
    SLONG base_green;
    SLONG base_blue;

    SLONG red;
    SLONG green;
    SLONG blue;

    SLONG dist;
    SLONG bright;

    UBYTE dfcache_index;
    UBYTE darken;

    NIGHT_Dfcache* nd;
    NIGHT_Colour* nc;
    NIGHT_Smap* ns;
    NIGHT_Slight* nsl;

    UBYTE inside = 0;
    ULONG flags;

#define NIGHT_MAX_SLIGHTS_PER_FACET 16

    struct
    {
        UBYTE index;
        UBYTE padding;
        UWORD x;
        UWORD z;

    } slight[16];
    SLONG slight_upto = 0;

    DFacet* df;

    ASSERT(WITHIN(dfacet_index, 1, next_dfacet - 1));

    df = &dfacets[dfacet_index];

    NIGHT_get_facet_info(
        dfacet_index,
        &length,
        &height,
        &dx,
        &dy,
        &dz,
        &flags);

    ASSERT(height >= 2);

    num_points = length * height;
    num_faces = num_points - length - height + 1;

    if (!WITHIN(NIGHT_dfcache_free, 1, NIGHT_MAX_DFCACHES - 1)) {
        return NULL;
    }

    dfcache_index = NIGHT_dfcache_free;
    nd = &NIGHT_dfcache[dfcache_index];
    NIGHT_dfcache_free = nd->next;

    nd->next = NULL;
    nd->dfacet = dfacet_index;

    nd->next = NIGHT_dfcache_used;
    NIGHT_dfcache_used = dfcache_index;

    num_bytes = num_points * sizeof(NIGHT_Colour);
    num_bytes += num_faces + 3 >> 2;

    nd->colour = (NIGHT_Colour*)HEAP_get(num_bytes);
    nd->sizeof_colour = num_bytes;

    ASSERT(nd->colour != NULL);

    nx = -dz;
    ny = 0;
    nz = dx;

    dprod = NIGHT_amb_norm_x * nx + NIGHT_amb_norm_z * nz >> 8;

    if (WARE_in) {
        dprod >>= 3;
        dprod += 128 + 64 + 32;

        nx = -nx;
        nz = -nz;
    } else {
        dprod >>= 2;
        dprod += 192;
    }

    base_red = NIGHT_amb_red * dprod >> 8;
    base_green = NIGHT_amb_green * dprod >> 8;
    base_blue = NIGHT_amb_blue * dprod >> 8;

    {
        SLONG mx;
        SLONG mz;
        SLONG mx1;
        SLONG mz1;
        SLONG mx2;
        SLONG mz2;
        SLONG map_x;
        SLONG map_z;

        mx1 = df->x[0] << 8;
        mx2 = df->x[1] << 8;

        mz1 = df->z[0] << 8;
        mz2 = df->z[1] << 8;

        if (mx1 > mx2) {
            SWAP(mx1, mx2);
        }
        if (mz1 > mz2) {
            SWAP(mz1, mz2);
        }

        mx1 -= 0x400;
        mz1 -= 0x400;

        mx2 += 0x400;
        mz2 += 0x400;

        mx1 >>= PAP_SHIFT_LO;
        mz1 >>= PAP_SHIFT_LO;
        mx2 >>= PAP_SHIFT_LO;
        mz2 >>= PAP_SHIFT_LO;

        SATURATE(mx1, 0, PAP_SIZE_LO - 1);
        SATURATE(mz1, 0, PAP_SIZE_LO - 1);
        SATURATE(mx2, 0, PAP_SIZE_LO - 1);
        SATURATE(mz2, 0, PAP_SIZE_LO - 1);

        for (mx = mx1; mx <= mx2; mx++)
            for (mz = mz1; mz <= mz2; mz++) {
                map_x = mx << PAP_SHIFT_LO;
                map_z = mz << PAP_SHIFT_LO;

                ASSERT(WITHIN(mx, 0, PAP_SIZE_LO - 1));
                ASSERT(WITHIN(mz, 0, PAP_SIZE_LO - 1));

                ns = &NIGHT_smap[mx][mz];

                for (i = 0; i < ns->number; i++) {
                    ASSERT(WITHIN(ns->index + i, 0, NIGHT_slight_upto - 1));

                    nsl = &NIGHT_slight[ns->index + i];

                    lx = (nsl->x << 2) + map_x;
                    lz = (nsl->z << 2) + map_z;

                    dlx = (df->x[0] << 8) - lx;
                    dlz = (df->z[0] << 8) - lz;

                    dprod = dlx * nx + dlz * nz;

                    if (dprod > 0) {
                        if (WITHIN(slight_upto, 0, NIGHT_MAX_SLIGHTS_PER_FACET - 1)) {
                            slight[slight_upto].index = ns->index + i;
                            slight[slight_upto].x = lx;
                            slight[slight_upto].z = lz;

                            slight_upto += 1;
                        }
                    }
                }
            }
    }

    lx = df->x[0] << 8;
    ly = df->Y[0];
    lz = df->z[0] << 8;

    nc = nd->colour;

    for (i = 0; i < height; i++) {
        x = lx;
        y = ly;
        z = lz;

        darken = (NIGHT_flag & NIGHT_FLAG_DARKEN_BUILDING_POINTS) && (ly == PAP_calc_height_at_point(lx >> 8, ly >> 8));

        for (j = 0; j < length; j++) {
            red = base_red;
            green = base_green;
            blue = base_blue;

            if (darken) {
#define NIGHT_DARKEN_FACET 16

                red -= NIGHT_DARKEN_FACET;
                green -= NIGHT_DARKEN_FACET;
                blue -= NIGHT_DARKEN_FACET;
            }

            for (k = 0; k < slight_upto; k++) {
                ASSERT(WITHIN(slight[k].index, 0, NIGHT_slight_upto - 1));

                nsl = &NIGHT_slight[slight[k].index];

                if ((WARE_in && (nsl->blue & 1)) || ((flags & NIGHT_FLAG_INSIDE) == (unsigned)(nsl->blue & 1))) {
                    dlx = x - slight[k].x;
                    dlz = z - slight[k].z;
                    dly = y - nsl->y;

                    dist = QDIST3(abs(dlx), abs(dly), abs(dlz)) + 1;

                    if (dist < (nsl->radius << 2)) {
                        dprod = dlx * nx + dlz * nz;

                        ASSERT(dprod >= 0);

                        dprod /= dist;
                        bright = 256 - (dist << 8) / (nsl->radius << 2);
                        bright = (bright * dprod >> 8) * NIGHT_LIGHT_MULTIPLIER;

                        red += nsl->red * bright >> 8;
                        green += nsl->green * bright >> 8;
                        blue += nsl->blue * bright >> 8;
                    }
                }
            }

            SATURATE(red, 0, 255);
            SATURATE(green, 0, 255);
            SATURATE(blue, 0, 255);

            nc->red = red;
            nc->green = green;
            nc->blue = blue;

            x += dx;
            z += dz;

            nc += 1;
        }

        ly += dy;
    }

    ASSERT(nc == nd->colour + num_points);

    return dfcache_index;
}

void NIGHT_dfcache_destroy(UBYTE dfcache_index)
{
    NIGHT_Dfcache* nd;

    ASSERT(WITHIN(dfcache_index, 1, NIGHT_MAX_DFCACHES - 1));

    UBYTE next = NIGHT_dfcache_used;
    UBYTE* prev = &NIGHT_dfcache_used;

    while (1) {
        if (next == NULL) {
            ASSERT(0);

            break;
        }

        ASSERT(WITHIN(next, 1, NIGHT_MAX_DFCACHES - 1));

        nd = &NIGHT_dfcache[next];

        if (next == dfcache_index) {
            *prev = nd->next;

            break;
        }

        prev = &nd->next;
        next = nd->next;
    }

    nd = &NIGHT_dfcache[dfcache_index];

    HEAP_give(nd->colour, nd->sizeof_colour);

    nd->next = NIGHT_dfcache_free;
    NIGHT_dfcache_free = dfcache_index;
    nd->dfacet = NULL;
}

NIGHT_Colour NIGHT_get_light_at(
    SLONG x,
    SLONG y,
    SLONG z)
{
    SLONG i;

    SLONG mx;
    SLONG mz;

    SLONG xmid;
    SLONG zmid;

    SLONG xmin;
    SLONG zmin;

    SLONG xmax;
    SLONG zmax;

    SLONG map_x;
    SLONG map_z;

    SLONG lx;
    SLONG ly;
    SLONG lz;
    SLONG lradius;

    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG bright;

    SLONG red;
    SLONG green;
    SLONG blue;

    SLONG vector[3];

    NIGHT_Smap* ns;
    NIGHT_Slight* nsl;

    NIGHT_Colour ans;

    OB_Info* oi;

    red = (NIGHT_amb_red * 3 >> 2) * NIGHT_LIGHT_MULTIPLIER;
    green = (NIGHT_amb_green * 3 >> 2) * NIGHT_LIGHT_MULTIPLIER;
    blue = (NIGHT_amb_blue * 3 >> 2) * NIGHT_LIGHT_MULTIPLIER;

    xmid = x >> PAP_SHIFT_LO;
    zmid = z >> PAP_SHIFT_LO;

    xmin = xmid - 1;
    zmin = zmid - 1;

    xmax = xmid + 1;
    zmax = zmid + 1;

    SATURATE(xmin, 0, PAP_SIZE_LO - 1);
    SATURATE(zmin, 0, PAP_SIZE_LO - 1);
    SATURATE(xmax, 0, PAP_SIZE_LO - 1);
    SATURATE(zmax, 0, PAP_SIZE_LO - 1);

    for (mz = zmin; mz <= zmax; mz++)
        for (mx = xmin; mx <= xmax; mx++) {
            ns = &NIGHT_smap[mx][mz];

            map_x = mx << PAP_SHIFT_LO;
            map_z = mz << PAP_SHIFT_LO;

            for (i = 0; i < ns->number; i++) {
                nsl = &NIGHT_slight[ns->index + i];

                lx = (nsl->x << 2) + map_x;
                ly = nsl->y;
                lz = (nsl->z << 2) + map_z;

                dx = abs(lx - x);
                dy = abs(ly - y);
                dz = abs(lz - z);

                lradius = nsl->radius << 2;

                dist = QDIST3(abs(dx), abs(dy), abs(dz));

                if (dist > lradius) {
                } else {
                    bright = (128 - (dist * 128) / lradius) * NIGHT_LIGHT_MULTIPLIER;

                    red += nsl->red * bright >> 8;
                    green += nsl->green * bright >> 8;
                    blue += nsl->blue * bright >> 8;
                }
            }

            if (NIGHT_flag & NIGHT_FLAG_LIGHTS_UNDER_LAMPOSTS) {
                for (oi = OB_find(mx, mz); oi->prim; oi++) {
                    if (oi->prim < 5) {
                        FMATRIX_vector(
                            vector,
                            oi->yaw,
                            oi->pitch);

                        lx = (-vector[0] >> 9) + oi->x;
                        ly = (-vector[1] >> 9) + oi->y + 0x100;
                        lz = (-vector[2] >> 9) + oi->z;

                        dx = abs(lx - x);
                        dy = abs(ly - y);
                        dz = abs(lz - z);

                        lradius = NIGHT_lampost_radius << 2;

                        dist = QDIST3(abs(dx), abs(dy), abs(dz));

                        if (dist > lradius) {
                        } else {
                            bright = (128 - (dist * 128) / lradius) * NIGHT_LIGHT_MULTIPLIER;

                            red += NIGHT_lampost_red * bright >> 8;
                            green += NIGHT_lampost_green * bright >> 8;
                            blue += NIGHT_lampost_blue * bright >> 8;
                        }
                    }
                }
            }
        }

    if (x < (4 << 8) || x > (124 << 8) || z < (4 << 8) || z > (124 << 8)) {
        SLONG mul;
        SLONG mulx;
        SLONG mulz;

        mulx = MIN(x, (128 << 8) - x);
        mulz = MIN(z, (128 << 8) - z);
        mul = MIN(mulx, mulz);

        red = red * mul >> 10;
        green = green * mul >> 10;
        blue = blue * mul >> 10;
    }

    SATURATE(red, 0, 255);
    SATURATE(green, 0, 255);
    SATURATE(blue, 0, 255);

    ans.red = red;
    ans.green = green;
    ans.blue = blue;

    return ans;
}

void NIGHT_find(SLONG x, SLONG y, SLONG z)
{
    SLONG i;

    SLONG mx;
    SLONG mz;

    SLONG xmid;
    SLONG zmid;

    SLONG xmin;
    SLONG zmin;

    SLONG xmax;
    SLONG zmax;

    SLONG map_x;
    SLONG map_z;

    SLONG lx;
    SLONG ly;
    SLONG lz;
    SLONG lradius;

    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG dist;
    SLONG bright;

    SLONG vector[3];

    NIGHT_Smap* ns;
    NIGHT_Slight* nsl;

    NIGHT_Colour ans;

    OB_Info* oi;

    NIGHT_found_upto = 0;

    NIGHT_found[NIGHT_found_upto].dx = 0;
    NIGHT_found[NIGHT_found_upto].dy = -256;
    NIGHT_found[NIGHT_found_upto].dz = 0;
    NIGHT_found[NIGHT_found_upto].r = 5.0f * NIGHT_LIGHT_MULTIPLIER;
    NIGHT_found[NIGHT_found_upto].g = 5.0f * NIGHT_LIGHT_MULTIPLIER;
    NIGHT_found[NIGHT_found_upto].b = 15.0f * NIGHT_LIGHT_MULTIPLIER;

    NIGHT_found_upto = 1;

    xmid = x >> PAP_SHIFT_LO;
    zmid = z >> PAP_SHIFT_LO;

    xmin = xmid - 1;
    zmin = zmid - 1;

    xmax = xmid + 1;
    zmax = zmid + 1;

    SATURATE(xmin, 0, PAP_SIZE_LO - 1);
    SATURATE(zmin, 0, PAP_SIZE_LO - 1);
    SATURATE(xmax, 0, PAP_SIZE_LO - 1);
    SATURATE(zmax, 0, PAP_SIZE_LO - 1);

    for (mz = zmin; mz <= zmax; mz++)
        for (mx = xmin; mx <= xmax; mx++) {
            ns = &NIGHT_smap[mx][mz];

            map_x = mx << PAP_SHIFT_LO;
            map_z = mz << PAP_SHIFT_LO;

            for (i = 0; i < ns->number; i++) {
                nsl = &NIGHT_slight[ns->index + i];

                lx = (nsl->x << 2) + map_x;
                ly = nsl->y + 0x20;
                lz = (nsl->z << 2) + map_z;

                dx = lx - x;
                dy = ly - y;
                dz = lz - z;

                lradius = nsl->radius << 2;

                dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

                if (dist > lradius) {
                } else {
                    bright = (256 - (dist * 256) / lradius) * NIGHT_LIGHT_MULTIPLIER;

                    ASSERT(WITHIN(NIGHT_found_upto, 0, NIGHT_MAX_FOUND - 1));

                    NIGHT_found[NIGHT_found_upto].dx = dx * 256 / dist;
                    NIGHT_found[NIGHT_found_upto].dy = dy * 256 / dist;
                    NIGHT_found[NIGHT_found_upto].dz = dz * 256 / dist;
                    NIGHT_found[NIGHT_found_upto].r = nsl->red * bright >> 8;
                    NIGHT_found[NIGHT_found_upto].g = nsl->green * bright >> 8;
                    NIGHT_found[NIGHT_found_upto].b = nsl->blue * bright >> 8;

                    NIGHT_found_upto += 1;

                    if (NIGHT_found_upto >= NIGHT_MAX_FOUND) {
                        return;
                    }
                }
            }

            if (NIGHT_flag & NIGHT_FLAG_LIGHTS_UNDER_LAMPOSTS) {
                for (oi = OB_find(mx, mz); oi->prim; oi++) {
                    if (oi->prim < 5) {
                        FMATRIX_vector(
                            vector,
                            oi->yaw,
                            oi->pitch);

                        lx = (-vector[0] >> 9) + oi->x;
                        ly = (-vector[1] >> 9) + oi->y + 0x120;
                        lz = (-vector[2] >> 9) + oi->z;

                        dx = lx - x;
                        dy = ly - y;
                        dz = lz - z;

                        lradius = NIGHT_lampost_radius << 2;

                        dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

                        if (dist > lradius) {
                        } else {
                            ASSERT(WITHIN(NIGHT_found_upto, 0, NIGHT_MAX_FOUND - 1));

                            bright = (256 - (dist * 256) / lradius) * NIGHT_LIGHT_MULTIPLIER;

                            NIGHT_found[NIGHT_found_upto].dx = dx * 256 / dist;
                            NIGHT_found[NIGHT_found_upto].dy = dy * 256 / dist;
                            NIGHT_found[NIGHT_found_upto].dz = dz * 256 / dist;
                            NIGHT_found[NIGHT_found_upto].r = NIGHT_lampost_red * bright >> 8;
                            NIGHT_found[NIGHT_found_upto].g = NIGHT_lampost_green * bright >> 8;
                            NIGHT_found[NIGHT_found_upto].b = NIGHT_lampost_blue * bright >> 8;

                            NIGHT_found_upto += 1;

                            if (NIGHT_found_upto >= NIGHT_MAX_FOUND) {
                                return;
                            }
                        }
                    }
                }
            }
        }

    NIGHT_Dlight* ndl;

    for (i = NIGHT_dlight_used; i; i = ndl->next) {
        ASSERT(WITHIN(i, 1, NIGHT_MAX_DLIGHTS - 1));

        ndl = &NIGHT_dlight[i];

        lx = ndl->x;
        ly = ndl->y + 0x20;
        lz = ndl->z;

        dx = lx - x;
        dy = ly - y;
        dz = lz - z;

        lradius = ndl->radius << 2;

        dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

        if (dist > lradius) {
        } else {
            ASSERT(WITHIN(NIGHT_found_upto, 0, NIGHT_MAX_FOUND - 1));

            bright = (256 - (dist * 256) / lradius) * NIGHT_LIGHT_MULTIPLIER;

            NIGHT_found[NIGHT_found_upto].dx = dx * 256 / dist;
            NIGHT_found[NIGHT_found_upto].dy = dy * 256 / dist;
            NIGHT_found[NIGHT_found_upto].dz = dz * 256 / dist;
            NIGHT_found[NIGHT_found_upto].r = ndl->red * bright >> 8;
            NIGHT_found[NIGHT_found_upto].g = ndl->green * bright >> 8;
            NIGHT_found[NIGHT_found_upto].b = ndl->blue * bright >> 8;

            NIGHT_found_upto += 1;

            if (NIGHT_found_upto >= NIGHT_MAX_FOUND) {
                return;
            }
        }
    }
}

void NIGHT_init()
{
    HEAP_init();

    NIGHT_cache_init();
    NIGHT_slight_init();
    NIGHT_dlight_init();
    NIGHT_dfcache_init();

    NIGHT_sky_colour.red = 210;
    NIGHT_sky_colour.green = 200;
    NIGHT_sky_colour.blue = 240;

    NIGHT_lampost_red = 70;
    NIGHT_lampost_green = 70;
    NIGHT_lampost_blue = 36;

    NIGHT_lampost_radius = 255;

    NIGHT_flag = NIGHT_FLAG_DAYTIME | NIGHT_FLAG_DARKEN_BUILDING_POINTS;

    NIGHT_ambient(255, 255, 255, 110, -148, -177);
}

UWORD hidden_roof_index[128][128];

static void calc_lighting__for_point(SLONG prim_x, SLONG prim_y, SLONG prim_z, NIGHT_Colour* nc)
{
    SLONG i;
    SLONG j;

    SLONG mx, mz;
    SLONG red;
    SLONG green;
    SLONG blue;

    SLONG dist;
    SLONG dprod;
    SLONG bright;

    NIGHT_Smap* ns;
    NIGHT_Slight* nsl;

    SLONG map_x;
    SLONG map_z;

    SLONG mx1;
    SLONG mz1;
    SLONG mx2;
    SLONG mz2;

    SLONG lradius;
    SLONG lx;
    SLONG ly;
    SLONG lz;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    mx1 = prim_x - 0x400 >> PAP_SHIFT_LO;
    mz1 = prim_z - 0x400 >> PAP_SHIFT_LO;

    mx2 = prim_x + 0x400 >> PAP_SHIFT_LO;
    mz2 = prim_z + 0x400 >> PAP_SHIFT_LO;

    SATURATE(mx1, 0, PAP_SIZE_LO - 1);
    SATURATE(mz1, 0, PAP_SIZE_LO - 1);

    SATURATE(mx2, 0, PAP_SIZE_LO - 1);
    SATURATE(mz2, 0, PAP_SIZE_LO - 1);

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            map_x = mx << PAP_SHIFT_LO;
            map_z = mz << PAP_SHIFT_LO;

            ASSERT(WITHIN(mx, 0, PAP_SIZE_LO - 1));
            ASSERT(WITHIN(mz, 0, PAP_SIZE_LO - 1));

            ns = &NIGHT_smap[mx][mz];

            for (j = 0; j < ns->number; j++) {
                ASSERT(WITHIN(ns->index + j, 0, NIGHT_MAX_SLIGHTS - 1));

                nsl = &NIGHT_slight[ns->index + j];

                lx = (nsl->x << 2) + map_x;
                ly = nsl->y;
                lz = (nsl->z << 2) + map_z;

                lradius = nsl->radius << 2;

                if (ly <= prim_y) {
                    continue;
                }

                dx = prim_x - lx;
                dy = prim_y - ly;
                dz = prim_z - lz;

                dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

                if (dist > lradius) {
                    continue;
                }

                dprod = -256 * dy;
                dprod /= dist;
                bright = 256 - (dist << 8) / lradius;
                bright = (bright * dprod >> 8) * NIGHT_LIGHT_MULTIPLIER;

                red = nc->red;
                green = nc->green;
                blue = nc->blue;

                red += nsl->red * bright >> 8;
                green += nsl->green * bright >> 8;
                blue += nsl->blue * bright >> 8;

                SATURATE(red, 0, 255);
                SATURATE(green, 0, 255);
                SATURATE(blue, 0, 255);

                nc->red = red;
                nc->green = green;
                nc->blue = blue;
            }
        }
}

void NIGHT_generate_roof_walkable()
{
    SLONG i;
    SLONG j;
    SLONG b;

    SLONG mx;
    SLONG mz;

    SLONG mx1;
    SLONG mz1;
    SLONG mx2;
    SLONG mz2;

    SLONG map_x;
    SLONG map_z;

    SLONG lradius;
    SLONG lx;
    SLONG ly;
    SLONG lz;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;
    SLONG dprod;
    SLONG bright;
    SLONG walk;

    SLONG amb_amount;
    SLONG amb_red;
    SLONG amb_green;
    SLONG amb_blue;

    SLONG red;
    SLONG green;
    SLONG blue;

    NIGHT_Smap* ns;
    NIGHT_Slight* nsl;
    NIGHT_Colour* nc;

    DBuilding* db;
    DWalkable* dw;
    SLONG max_face = 0;

    NIGHT_first_walkable_prim_point = first_walkable_prim_point;

    for (b = 1; b < next_dbuilding; b++) {
        db = &dbuildings[b];

        if (db->Type != BUILDING_TYPE_CRATE_IN) {
            amb_amount = 128;
            amb_amount += -NIGHT_amb_norm_y >> 1;
            amb_amount *= NIGHT_LIGHT_MULTIPLIER;

            amb_red = NIGHT_amb_red * amb_amount >> 8;
            amb_green = NIGHT_amb_green * amb_amount >> 8;
            amb_blue = NIGHT_amb_blue * amb_amount >> 8;
        } else {
            amb_red = 30;
            amb_green = 30;
            amb_blue = 30;
        }

        for (walk = db->Walkable; walk; walk = dw->Next) {
            dw = &dwalkables[walk];

            for (i = dw->StartFace4; i < dw->EndFace4; i++) {
                struct RoofFace4* rf;
                SLONG prim_x, prim_y, prim_z, point;
                SLONG mx, mz;
                SLONG roof_face_x, roof_face_z;

                rf = &roof_faces4[i];
                roof_face_x = (rf->RX & 127) << 8;
                roof_face_z = (rf->RZ & 127) << 8;

                if (i > max_face) {
                    max_face = i;
                }

                for (point = 0; point < 4; point++) {
                    switch (point) {
                    case 0:
                        prim_x = roof_face_x;
                        prim_y = rf->Y;
                        prim_z = roof_face_z;
                        break;
                    case 1:
                        prim_x = roof_face_x + 256;
                        prim_y = (rf->Y) + (rf->DY[0] << ROOF_SHIFT);
                        prim_z = roof_face_z;
                        break;
                    case 3:
                        prim_x = roof_face_x + 256;
                        prim_y = (rf->Y) + (rf->DY[1] << ROOF_SHIFT);
                        prim_z = roof_face_z + 256;
                        break;
                    case 2:
                        prim_x = roof_face_x;
                        prim_y = (rf->Y) + (rf->DY[2] << ROOF_SHIFT);
                        prim_z = roof_face_z + 256;
                        break;
                    }

                    nc = &NIGHT_roof_walkable[i * 4 + point];

                    nc->red = amb_red;
                    nc->green = amb_green;
                    nc->blue = amb_blue;

                    calc_lighting__for_point(prim_x, prim_y, prim_z, nc);
                }
            }
        }
    }

    {
        SLONG x, z, point, prim_x, prim_y, prim_z;

        amb_amount = 128;
        amb_amount += -NIGHT_amb_norm_y >> 1;
        amb_amount *= NIGHT_LIGHT_MULTIPLIER;

        amb_red = NIGHT_amb_red * amb_amount >> 8;
        amb_green = NIGHT_amb_green * amb_amount >> 8;
        amb_blue = NIGHT_amb_blue * amb_amount >> 8;

        for (x = 0; x < 128; x++) {
            for (z = 0; z < 128; z++) {
                if (PAP_hi[x][z].Flags & PAP_FLAG_ROOF_EXISTS) {
                    max_face++;

                    hidden_roof_index[x][z] = max_face;

                    for (point = 0; point < 4; point++) {
                        switch (point) {
                        case 0:
                            prim_x = (x) << 8;
                            prim_y = MAVHEIGHT(x, z) << 6;
                            prim_z = (z) << 8;
                            break;
                        case 1:
                            prim_x = (x + 1) << 8;
                            prim_y = MAVHEIGHT(x, z) << 6;
                            prim_z = (z) << 8;
                            break;
                        case 2:
                            prim_x = (x + 1) << 8;
                            prim_y = MAVHEIGHT(x, z) << 6;
                            prim_z = (z + 1) << 8;
                            break;
                        case 3:
                            prim_x = (x) << 8;
                            prim_y = MAVHEIGHT(x, z) << 6;
                            prim_z = (z + 1) << 8;
                            break;
                        }

                        nc = &NIGHT_roof_walkable[max_face * 4 + point];

                        nc->red = amb_red;
                        nc->green = amb_green;
                        nc->blue = amb_blue;

                        calc_lighting__for_point(prim_x, prim_y, prim_z, nc);
                    }
                }
            }
        }
    }
}

void NIGHT_generate_walkable_lighting()
{
    SLONG i;
    SLONG j;

    SLONG mx;
    SLONG mz;

    SLONG mx1;
    SLONG mz1;
    SLONG mx2;
    SLONG mz2;

    SLONG map_x;
    SLONG map_z;

    SLONG lradius;
    SLONG lx;
    SLONG ly;
    SLONG lz;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dist;
    SLONG dprod;
    SLONG bright;

    SLONG amb_amount;
    SLONG amb_red;
    SLONG amb_green;
    SLONG amb_blue;

    SLONG red;
    SLONG green;
    SLONG blue;

    PrimPoint* pp;
    NIGHT_Smap* ns;
    NIGHT_Slight* nsl;
    NIGHT_Colour* nc;

    NIGHT_generate_roof_walkable();

    NIGHT_first_walkable_prim_point = first_walkable_prim_point;

    return;

    amb_amount = 128;
    amb_amount += -NIGHT_amb_norm_y >> 1;
    amb_amount *= NIGHT_LIGHT_MULTIPLIER;

    amb_red = NIGHT_amb_red * amb_amount >> 8;
    amb_green = NIGHT_amb_green * amb_amount >> 8;
    amb_blue = NIGHT_amb_blue * amb_amount >> 8;

    for (i = 0; i < number_of_walkable_prim_points; i++) {
        ASSERT(WITHIN(i, 0, NIGHT_MAX_WALKABLE));
        ASSERT(WITHIN(first_walkable_prim_point + i, 1, next_prim_point - 1));

        pp = &prim_points[first_walkable_prim_point + i];

        nc = &NIGHT_walkable[i];

        nc->red = amb_red;
        nc->green = amb_green;
        nc->blue = amb_blue;

        mx1 = pp->X - 0x400 >> PAP_SHIFT_LO;
        mz1 = pp->Z - 0x400 >> PAP_SHIFT_LO;

        mx2 = pp->X + 0x400 >> PAP_SHIFT_LO;
        mz2 = pp->Z + 0x400 >> PAP_SHIFT_LO;

        SATURATE(mx1, 0, PAP_SIZE_LO - 1);
        SATURATE(mz1, 0, PAP_SIZE_LO - 1);

        SATURATE(mx2, 0, PAP_SIZE_LO - 1);
        SATURATE(mz2, 0, PAP_SIZE_LO - 1);

        for (mx = mx1; mx <= mx2; mx++)
            for (mz = mz1; mz <= mz2; mz++) {
                map_x = mx << PAP_SHIFT_LO;
                map_z = mz << PAP_SHIFT_LO;

                ASSERT(WITHIN(mx, 0, PAP_SIZE_LO - 1));
                ASSERT(WITHIN(mz, 0, PAP_SIZE_LO - 1));

                ns = &NIGHT_smap[mx][mz];

                for (j = 0; j < ns->number; j++) {
                    ASSERT(WITHIN(ns->index + j, 0, NIGHT_MAX_SLIGHTS - 1));

                    nsl = &NIGHT_slight[ns->index + j];

                    lx = (nsl->x << 2) + map_x;
                    ly = nsl->y;
                    lz = (nsl->z << 2) + map_z;

                    lradius = nsl->radius << 2;

                    if (ly <= pp->Y) {
                        continue;
                    }

                    dx = pp->X - lx;
                    dy = pp->Y - ly;
                    dz = pp->Z - lz;

                    dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

                    if (dist > lradius) {
                        continue;
                    }

                    dprod = -256 * dy;
                    dprod /= dist;
                    bright = 256 - (dist << 8) / lradius;
                    bright = (bright * dprod >> 8) * NIGHT_LIGHT_MULTIPLIER;

                    red = nc->red;
                    green = nc->green;
                    blue = nc->blue;

                    red += nsl->red * bright >> 8;
                    green += nsl->green * bright >> 8;
                    blue += nsl->blue * bright >> 8;

                    SATURATE(red, 0, 255);
                    SATURATE(green, 0, 255);
                    SATURATE(blue, 0, 255);

                    nc->red = red;
                    nc->green = green;
                    nc->blue = blue;
                }
            }
    }
}

void NIGHT_destroy_all_cached_info()
{
    HEAP_init();
    NIGHT_cache_init();
    NIGHT_dfcache_init();
}

SLONG NIGHT_load_ed_file(CBYTE* name)
{
    SLONG i;

    SLONG sizeof_ed_light;
    SLONG ed_max_lights;
    SLONG sizeof_night_colour;
    SLONG ed_light_free;
    SLONG data_left;
    UBYTE version = 0;

    ED_Light el;

    NIGHT_Colour col;

    MFFileHandle handle;

    handle = FileOpen(name);

    if ((!handle) || (handle == FILE_OPEN_ERROR)) {
        return FALSE;
    }

    if (FileRead(handle, &sizeof_ed_light, sizeof(SLONG)) < 0)
        goto file_error;
    if (FileRead(handle, &ed_max_lights, sizeof(SLONG)) < 0)
        goto file_error;
    if (FileRead(handle, &sizeof_night_colour, sizeof(SLONG)) < 0)
        goto file_error;

    version = sizeof_ed_light >> 16;
    sizeof_ed_light &= 0xffff;

    if (sizeof_ed_light != sizeof(ED_Light) || sizeof_night_colour != sizeof(NIGHT_Colour)) {
        FileClose(handle);

        return FALSE;
    }

    for (i = 0; i < ed_max_lights; i++) {
        SLONG count = 0;
        if (FileRead(handle, &el, sizeof(ED_Light)) < 0)
            goto file_error;

        if (el.used) {
            count++;

            NIGHT_slight_create(
                el.x,
                el.y,
                el.z,
                el.range,
                el.red,
                el.green,
                el.blue);
        }
    }

    if (FileRead(handle, &ed_light_free, sizeof(SLONG)) < 0)
        goto file_error;

    ULONG flag;
    ULONG amb_d3d_colour;
    ULONG amb_d3d_specular;
    SLONG amb_red;
    SLONG amb_green;
    SLONG amb_blue;
    SBYTE lampost_red;
    SBYTE lampost_green;
    SBYTE lampost_blue;
    UBYTE padding;
    SLONG lampost_radius;
    NIGHT_Colour sky_colour;

    if (FileRead(handle, &flag, sizeof(ULONG)) < 0)
        goto file_error;
    if (FileRead(handle, &amb_d3d_colour, sizeof(ULONG)) < 0)
        goto file_error;
    if (FileRead(handle, &amb_d3d_specular, sizeof(ULONG)) < 0)
        goto file_error;
    if (FileRead(handle, &amb_red, sizeof(ULONG)) < 0)
        goto file_error;
    if (FileRead(handle, &amb_green, sizeof(ULONG)) < 0)
        goto file_error;
    if (FileRead(handle, &amb_blue, sizeof(ULONG)) < 0)
        goto file_error;
    if (FileRead(handle, &lampost_red, sizeof(SBYTE)) < 0)
        goto file_error;
    if (FileRead(handle, &lampost_green, sizeof(SBYTE)) < 0)
        goto file_error;
    if (FileRead(handle, &lampost_blue, sizeof(SBYTE)) < 0)
        goto file_error;
    if (FileRead(handle, &padding, sizeof(UBYTE)) < 0)
        goto file_error;
    if (FileRead(handle, &lampost_radius, sizeof(SLONG)) < 0)
        goto file_error;
    if (FileRead(handle, &sky_colour, sizeof(NIGHT_Colour)) < 0)
        goto file_error;

    NIGHT_ambient(
        amb_red * 820 >> 8,
        amb_green * 820 >> 8,
        amb_blue * 820 >> 8,
        110, -148, -177);

    NIGHT_flag = flag;
    NIGHT_sky_colour = sky_colour;
    NIGHT_lampost_radius = lampost_radius;
    NIGHT_lampost_red = lampost_red;
    NIGHT_lampost_green = lampost_green;
    NIGHT_lampost_blue = lampost_blue;

    FileClose(handle);

    return TRUE;

file_error:;

    FileClose(handle);

    return FALSE;
}
