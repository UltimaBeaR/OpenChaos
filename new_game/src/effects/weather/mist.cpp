#include "engine/platform/uc_common.h"
#include <math.h>
#include "game/game_types.h" // for TICK_INV_RATIO, TICK_SHIFT macros
#include "effects/weather/mist.h"
#include "effects/weather/mist_globals.h"

// uc_orig: MIST_init (fallen/Source/mist.cpp)
// Resets both the point pool and layer array to empty.
void MIST_init()
{
    MIST_point_upto = 0;
    MIST_mist_upto = 0;
}

// uc_orig: MIST_create (fallen/Source/mist.cpp)
// Allocates a new mist layer and reserves detail*detail points from the shared pool.
// detail is clamped to [3..255]; type cycles 0..3 to vary the UV scroll pattern.
void MIST_create(
    SLONG detail,
    SLONG height,
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2)
{
    SLONG i;
    static SLONG type_cycle = 0;

    ASSERT(WITHIN(MIST_mist_upto, 0, MIST_MAX_MIST - 1));

    MIST_Mist* mm = &MIST_mist[MIST_mist_upto++];

    SATURATE(detail, 3, 255);

    ASSERT(MIST_point_upto + detail * detail <= MIST_MAX_POINTS);

    mm->type = type_cycle++ & 0x3;
    mm->height = height;
    mm->detail = detail;
    mm->p_index = MIST_point_upto;
    mm->x1 = x1;
    mm->z1 = z1;
    mm->x2 = x2;
    mm->z2 = z2;

    for (i = 0; i < detail * detail; i++) {
        MIST_point[MIST_point_upto].u = 0;
        MIST_point[MIST_point_upto].v = 0;
        MIST_point[MIST_point_upto].du = 0;
        MIST_point[MIST_point_upto].dv = 0;

        MIST_point_upto += 1;
    }
}

// uc_orig: MIST_gust (fallen/Source/mist.cpp)
// Applies a wind gust vector to mist points within range.
// Gust strength is scaled by TICK_INV_RATIO so fast frame rates don't stall the effect.
// Points in front of the gust are pushed back; points behind are pulled forward.
void MIST_gust(
    SLONG gx1, SLONG gz1,
    SLONG gx2, SLONG gz2)
{
    SLONG i;

    SLONG sx;
    SLONG sz;

    SLONG sizex;
    SLONG sizez;

    SLONG x1, z1;
    SLONG x2, z2;

    SLONG x;
    SLONG z;

    SLONG wx;
    SLONG wz;

    SLONG dx;
    SLONG dz;

    SLONG dgx;
    SLONG dgz;

    SLONG dist;
    SLONG push;
    SLONG dprod;
    SLONG strength;

    SLONG p_index;

    float ddu;
    float ddv;

    MIST_Mist* mm;
    MIST_Point* mp;

    // Scale gust vector by inverse tick ratio so effect is frame-rate independent
    dgx = gx2 - gx1;
    dgz = gz2 - gz1;

    dgx = dgx * TICK_INV_RATIO >> TICK_SHIFT;
    dgz = dgz * TICK_INV_RATIO >> TICK_SHIFT;

    gx2 = gx1 + dgx;
    gz2 = gz1 + dgz;

    strength = QDIST2(abs(dgx), abs(dgz));

    // Cap gust strength so fast vehicles don't dominate
    if (strength > MIST_MAX_STRENGTH) {
        gx2 = gx1 + (dgx * MIST_MAX_STRENGTH) / strength;
        gz2 = gz1 + (dgz * MIST_MAX_STRENGTH) / strength;

        strength = MIST_MAX_STRENGTH;
    }

    strength <<= 4;

    if (strength == 0) {
        // Ignore this gust.
    }

    for (i = 0; i < MIST_mist_upto; i++) {
        mm = &MIST_mist[i];

        sizex = (mm->x2 - mm->x1) / (mm->detail - 1);
        sizez = (mm->z2 - mm->z1) / (mm->detail - 1);

        // Which square of the mist is the gust located in?
        sx = (gx1 - mm->x1) / sizex;
        sz = (gz2 - mm->z1) / sizez;

        if (sx < -1 || sx >= mm->detail || sz < -1 || sz >= mm->detail) {
            // The gust is outside this mist layer.
        } else {
            // Bounding box of points to consider
            x1 = sx - 1;
            z1 = sz - 1;
            x2 = sx + 2;
            z2 = sz + 2;

            if (x1 < 0) {
                x1 = 0;
            }
            if (z1 < 0) {
                z1 = 0;
            }

            if (x2 > mm->detail - 1) {
                x2 = mm->detail - 1;
            }
            if (z2 > mm->detail - 1) {
                z2 = mm->detail - 1;
            }

            for (x = x1; x <= x2; x++)
                for (z = z1; z <= z2; z++) {
                    wx = mm->x1 + x * sizex;
                    wz = mm->z1 + z * sizez;

                    dx = wx - gx1;
                    dz = wz - gz1;

                    dist = QDIST2(abs(dx), abs(dz));

                    if (dist == 0) {
                        continue;
                    }

                    if (strength > dist) {
                        // Dot product: positive = point is behind gust origin, gets pushed
                        dprod = dx * dgx + dz * dgz;

                        push = -dprod;
                        push /= dist;
                        push *= strength - dist;
                        push /= strength;

                        ddu = float(dx) * float(push) * 0.0000005F;
                        ddv = float(dz) * float(push) * 0.0000005F;

                        if (push > 0) {
                            // Points being pushed follow the gust more strongly
                            ddu += ddu;
                            ddv += ddv;
                        }

                        p_index = mm->p_index;
                        p_index += x + z * mm->detail;

                        ASSERT(WITHIN(p_index, 0, MIST_point_upto - 1));

                        mp = &MIST_point[p_index];

                        mp->du += ddu;
                        mp->dv += ddv;
                    }
                }
        }
    }
}

// uc_orig: MIST_process (fallen/Source/mist.cpp)
// Integrates velocities and applies 25% damping + 0.5% spring each tick.
void MIST_process()
{
    SLONG i;
    MIST_Point* mp;

    for (i = 0; i < MIST_point_upto; i++) {
        mp = &MIST_point[i];

        mp->u += mp->du;
        mp->v += mp->dv;

        mp->dv -= mp->dv * 0.25F;
        mp->du -= mp->du * 0.25F;

        mp->du -= mp->u * 0.005F;
        mp->dv -= mp->v * 0.005F;
    }
}

// uc_orig: MIST_get_start (fallen/Source/mist.cpp)
// Initialise renderer iterator; increments turn counter for animated UV wibble.
void MIST_get_start()
{
    MIST_get_upto = 0;
    MIST_get_turn += 1;
}

// uc_orig: MIST_get_detail (fallen/Source/mist.cpp)
// Advances to the next mist layer. Returns the layer's detail (grid resolution),
// or NULL if all layers have been visited.
// Pre-computes per-layer cached values used by MIST_get_point / MIST_get_texture.
SLONG MIST_get_detail()
{
    float radius;
    float yaw_odd;
    float yaw_even;

    MIST_Mist* mm;

    if (!WITHIN(MIST_get_upto, 0, MIST_mist_upto - 1)) {
        return NULL;
    } else {
        mm = &MIST_mist[MIST_get_upto++];

        MIST_get_mist = mm;

        MIST_get_dx = (mm->x2 - mm->x1) / (mm->detail - 1);
        MIST_get_dz = (mm->z2 - mm->z1) / (mm->detail - 1);
        MIST_get_du = 0.5F / float(mm->detail - 1);
        MIST_get_dv = 0.5F / float(mm->detail - 1);

        MIST_get_base_u = float(mm->type & 1) * 0.5f;
        MIST_get_base_v = float(mm->type >> 1) * 0.5f;

        // Rotate wibble phase per layer and per turn for animated variation
        yaw_odd = float(MIST_get_turn) / (float(mm->type + 1) * 15.0F);
        yaw_odd += MIST_get_upto;
        yaw_even = yaw_odd + (PI / 1.75F);

        radius = 0.1F / float(mm->detail - 1);

        MIST_off_u_odd = sin(yaw_odd) * radius;
        MIST_off_v_odd = cos(yaw_odd) * radius;

        MIST_off_u_even = sin(yaw_even) * radius;
        MIST_off_v_even = cos(yaw_even) * radius;

        return mm->detail;
    }
}

// uc_orig: MIST_get_point (fallen/Source/mist.cpp)
// Returns the world-space position of the mist grid point at (px, pz).
// Height is a fixed constant per layer (the commented-out PAP call was never implemented).
void MIST_get_point(SLONG px, SLONG pz,
    SLONG* x,
    SLONG* y,
    SLONG* z)
{
    MIST_Mist* mm = MIST_get_mist;

    ASSERT(WITHIN(mm, &MIST_mist[0], &MIST_mist[MIST_mist_upto - 1]));
    ASSERT(WITHIN(px, 0, mm->detail - 1));
    ASSERT(WITHIN(pz, 0, mm->detail - 1));

    *x = mm->x1 + px * MIST_get_dx;
    *z = mm->z1 + pz * MIST_get_dz;
    *y = mm->height;
}

// uc_orig: MIST_get_texture (fallen/Source/mist.cpp)
// Returns the UV texture coordinates for the mist grid point at (px, pz).
// Adds per-point dynamic displacement from MIST_point[] plus layer-level wibble.
// Border points (edges) are not wibbled to avoid seam artifacts.
void MIST_get_texture(SLONG px, SLONG pz,
    float* u,
    float* v)
{
    SLONG p_index;

    MIST_Point* mp;
    MIST_Mist* mm = MIST_get_mist;

    ASSERT(WITHIN(mm, &MIST_mist[0], &MIST_mist[MIST_mist_upto - 1]));
    ASSERT(WITHIN(px, 0, mm->detail - 1));
    ASSERT(WITHIN(pz, 0, mm->detail - 1));

    *u = MIST_get_base_u;
    *v = MIST_get_base_v;

    *u += float(px) * MIST_get_du;
    *v += float(pz) * MIST_get_dv;

    if (px == 0 || px == mm->detail - 1 || pz == 0 || pz == mm->detail - 1) {
        // Don't wibble the border points (avoids visible seams)
        return;
    }

    if ((px + pz) & 0x1) {
        *u += MIST_off_u_odd;
        *v += MIST_off_v_odd;
    } else {
        *u += MIST_off_u_even;
        *v += MIST_off_v_even;
    }

    p_index = mm->p_index;
    p_index += px + pz * mm->detail;

    ASSERT(WITHIN(p_index, 0, MIST_point_upto - 1));

    mp = &MIST_point[p_index];

    *u += mp->u;
    *v += mp->v;
}
