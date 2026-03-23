#include <math.h>
// Temporary: game.h needed for PAP_2HI, PAP_FLAG_*, PAP_ALT_SHIFT, PAP_SHIFT_HI macros
#include "game.h"
// Temporary: mav.h needed for MAVHEIGHT macro
#include "mav.h"
#include "engine/graphics/geometry/oval.h"
#include "engine/graphics/geometry/oval_globals.h"
#include "engine/graphics/pipeline/poly.h"

// Returns the oval (u,v) at the given world position using the current oval gradient.
// uc_orig: OVAL_get_uv (fallen/DDEngine/Source/oval.cpp)
static void OVAL_get_uv(float world_x, float world_z, float* u, float* v)
{
    float dx;
    float dz;

    dx = world_x - OVAL_mid_x;
    dz = world_z - OVAL_mid_z;

    float ans_u;
    float ans_v;

    ans_u = 0.5F;
    ans_v = 0.5F;

    ans_u += dx * OVAL_dudx;
    ans_u += dz * OVAL_dudz;

    ans_v += dx * OVAL_dvdx;
    ans_v += dz * OVAL_dvdz;

    *u = ans_u;
    *v = ans_v;
}

// Projects the oval onto a single map square, computing heights at corners and adding a quad.
// Skips squares that are too far above the oval center, have roofs, or are hidden.
// uc_orig: OVAL_project_onto_mapsquare (fallen/DDEngine/Source/oval.cpp)
static void OVAL_project_onto_mapsquare(UBYTE map_x, UBYTE map_z, SLONG page)
{
    SLONG i;

    PAP_Hi* ph = &PAP_2HI(map_x, map_z);

    if (!WITHIN(map_x, 1, 126) || !WITHIN(map_z, 1, 126)) {
        return;
    }

    float world_y[4];

    if (ph->Flags & PAP_FLAG_ROOF_EXISTS) {
        world_y[0] = world_y[1] = world_y[2] = world_y[3] = MAVHEIGHT(map_x, map_z) << 6;
    } else if (ph->Flags & PAP_FLAG_HIDDEN) {
        return;
    } else {
        world_y[0] = ph[0].Alt << PAP_ALT_SHIFT;
        world_y[1] = ph[0 + PAP_SIZE_HI].Alt << PAP_ALT_SHIFT;
        world_y[2] = ph[1].Alt << PAP_ALT_SHIFT;
        world_y[3] = ph[1 + PAP_SIZE_HI].Alt << PAP_ALT_SHIFT;

        if (ph->Flags & PAP_FLAG_SINK_SQUARE) {
            world_y[0] -= 32.0F;
            world_y[1] -= 32.0F;
            world_y[2] -= 32.0F;
            world_y[3] -= 32.0F;
        }
    }

    float av_y;

    av_y = world_y[0];
    av_y += world_y[1];
    av_y += world_y[2];
    av_y += world_y[3];

    av_y *= 0.25F;

    // Fade the shadow with height above the ground.
    float dy = OVAL_mid_y - av_y;

    if (dy < -48.0F) {
        return;
    }

    float dark;

    if (dy < 128.0F) {
        dark = 1.0F;
    } else {
        dark = 1.0F - (dy - 128) * (1.0F / 128.0F);
    }

    if (dark <= 0.0F) {
        return;
    }

    SLONG colour;

    colour = ftol(dark * 128.0F);
    colour |= colour << 8;
    colour |= colour << 16;

    POLY_Point pp[4];
    POLY_Point* quad[4];

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    for (i = 0; i < 4; i++) {
        POLY_transform(
            float(map_x + (i & 1) << 8),
            world_y[i],
            float(map_z + (i >> 1) << 8),
            quad[i],
            UC_TRUE);
    }

    if (POLY_valid_quad(quad)) {
        for (i = 0; i < 4; i++) {
            OVAL_get_uv(
                float(map_x + (i & 1) << 8),
                float(map_z + (i >> 1) << 8),
                &pp[i].u,
                &pp[i].v);

            pp[i].colour = colour;
            pp[i].specular = 0xff000000;
        }

        // Reorder to match landscape winding order and avoid Z-fights.
        quad[0] = &pp[1];
        quad[1] = &pp[3];
        quad[2] = &pp[0];
        quad[3] = &pp[2];

        POLY_add_quad(quad, page, UC_FALSE);
    }
}

// uc_orig: OVAL_add (fallen/DDEngine/Source/oval.cpp)
void OVAL_add(SLONG x, SLONG y, SLONG z, SLONG size, float elongate, float angle, SLONG type)
{
    SLONG mx;
    SLONG mz;

    SLONG mx1;
    SLONG mz1;
    SLONG mx2;
    SLONG mz2;

    SLONG msize = ftol(float(size) * elongate + 8.0F);

    mx1 = x - msize >> PAP_SHIFT_HI;
    mz1 = z - msize >> PAP_SHIFT_HI;
    mx2 = x + msize >> PAP_SHIFT_HI;
    mz2 = z + msize >> PAP_SHIFT_HI;

    SATURATE(mx1, 1, 126);
    SATURATE(mz1, 1, 126);

    SATURATE(mx2, 1, 126);
    SATURATE(mz2, 1, 126);

    OVAL_mid_x = float(x);
    OVAL_mid_y = float(y);
    OVAL_mid_z = float(z);

    OVAL_dudx = (sin(angle) / float(size)) / elongate;
    OVAL_dvdx = (cos(angle) / float(size));

    OVAL_dudz = (sin(angle + PI / 2) / float(size)) / elongate;
    OVAL_dvdz = (cos(angle + PI / 2) / float(size));

    SLONG page;

    switch (type) {
    case OVAL_TYPE_OVAL:
        page = POLY_PAGE_SHADOW_OVAL;
        break;
    case OVAL_TYPE_SQUARE:
        page = POLY_PAGE_SHADOW_SQUARE;
        break;
    default:
        ASSERT(0);
        break;
    }

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            OVAL_project_onto_mapsquare(mx, mz, page);
        }
}
