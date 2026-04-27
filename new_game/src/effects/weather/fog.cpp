#include "effects/weather/fog.h"
#include "effects/weather/fog_globals.h"
#include "game/game_types.h"
#include "engine/platform/uc_common.h"
#include "map/pap_globals.h"

// uc_orig: FOG_get_dyaw (fallen/Source/fog.cpp)
// Return the natural (equilibrium) angular velocity for fog patch f_index.
// Based on the lower 4 bits of the index, giving a range of -7..+8 dyaw values.
static SLONG FOG_get_dyaw(SLONG f_index) { return (f_index & 15) - 7; }

// uc_orig: FOG_create (fallen/Source/fog.cpp)
// Place a fog patch at a random position on the edge of the current focus circle.
// Height is computed from the terrain at the chosen x,z position.
static void FOG_create(SLONG f_index)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG yaw;
    SLONG radius;

    ASSERT(WITHIN(f_index, 0, FOG_MAX_FOG - 1));

    FOG_Fog* ff = &FOG_fog[f_index];

    yaw = rand() & 2047;
    radius = 192;
    radius += rand() & 63;

    dx = SIN(yaw) * radius >> 16;
    dz = COS(yaw) * radius >> 16;
    dy = 16;
    dy += rand() & 127;

    dx = dx * FOG_focus_radius >> 8;
    dz = dz * FOG_focus_radius >> 8;

    ff->type = rand() & 3;
    ff->yaw = rand() & 2047;
    ff->dyaw = FOG_get_dyaw(f_index);
    ff->size = rand() & 0x63;
    ff->size += 64;
    ff->x = FOG_focus_x + dx;
    ff->z = FOG_focus_z + dz;
    ff->y = PAP_calc_height_at(ff->x, ff->z) + dy;
}

// uc_orig: FOG_init (fallen/Source/fog.cpp)
void FOG_init()
{
    SLONG i;

    for (i = 0; i < FOG_MAX_FOG; i++) {
        FOG_fog[i].type = FOG_TYPE_UNUSED;
    }
}

// uc_orig: FOG_set_focus (fallen/Source/fog.cpp)
// Update the focus center and radius. Fog patches outside the new radius are
// recycled to the edge. Unused slots are filled immediately.
void FOG_set_focus(
    SLONG x,
    SLONG z,
    SLONG radius)
{
    SLONG i;
    SLONG dx;
    SLONG dz;
    SLONG dist;

    FOG_Fog* ff;

    FOG_focus_x = x;
    FOG_focus_z = z;
    FOG_focus_radius = radius;

    for (i = 0; i < FOG_MAX_FOG; i++) {
        ff = &FOG_fog[i];

        if (ff->type == FOG_TYPE_UNUSED) {
            FOG_create(i);
        } else {
            dx = abs(ff->x - FOG_focus_x);
            dz = abs(ff->z - FOG_focus_z);

            dist = QDIST2(dx, dz);

            if (dist > FOG_focus_radius) {
                FOG_create(i);
            }
        }
    }
}

// uc_orig: FOG_gust (fallen/Source/fog.cpp)
// Apply a wind gust that spans from (gx1,gz1) to (gx2,gz2).
// Fog patches within range receive an angular velocity kick proportional to
// the cross product of the gust vector and the fog-to-gust-origin vector.
void FOG_gust(
    SLONG gx1, SLONG gz1,
    SLONG gx2, SLONG gz2)
{
    SLONG i;
    SLONG dx;
    SLONG dz;
    SLONG dgx;
    SLONG dgz;
    SLONG dist;
    SLONG push;
    SLONG cross;
    SLONG strength;
    SLONG dyaw;
    SLONG ddyaw;

    FOG_Fog* ff;

    dgx = gx2 - gx1;
    dgz = gz2 - gz1;

    strength = QDIST2(abs(dgx), abs(dgz));

    if (strength == 0) {
        return;
    }

    for (i = 0; i < FOG_MAX_FOG; i++) {
        ff = &FOG_fog[i];

        if (ff->type == FOG_TYPE_UNUSED) {
            continue;
        }

        dx = gx1 - ff->x;
        dz = gz1 - ff->z;

        dist = QDIST2(abs(dx), abs(dz));

        if (dist == 0) {
            continue;
        }

        if (dist > (ff->size * 300 >> 8)) {
            continue;
        }

        push = strength - (abs(dist - ff->size) >> 5);

        if (push > 0) {
            cross = dgx * dz - dgz * dx;

            ddyaw = cross;
            ddyaw /= dist;
            ddyaw *= push;
            ddyaw /= strength;
            ddyaw <<= 1;

            SATURATE(ddyaw, -40, +40);

            dyaw = ff->dyaw;
            dyaw += ddyaw;

            SATURATE(dyaw, -127, +127);

            ff->dyaw = dyaw;
        }
    }
}

// uc_orig: FOG_process (fallen/Source/fog.cpp)
// Advance the fog simulation: rotate each patch and damp its angular velocity
// toward the natural spin rate for its index slot.
void FOG_process()
{
    SLONG i;
    SLONG dyaw;
    SLONG wantdyaw;
    SLONG ddyaw;

    FOG_Fog* ff;

    for (i = 0; i < FOG_MAX_FOG; i++) {
        ff = &FOG_fog[i];

        ff->yaw += ff->dyaw;

        wantdyaw = FOG_get_dyaw(i);
        dyaw = ff->dyaw;

        ddyaw = dyaw - wantdyaw;
        dyaw -= SIGN(ddyaw);
        dyaw -= dyaw / 16;

        ff->dyaw = dyaw;
    }
}

// uc_orig: FOG_get_start (fallen/Source/fog.cpp)
void FOG_get_start() { FOG_get_upto = 0; }

// uc_orig: FOG_get_info (fallen/Source/fog.cpp)
// Return draw descriptor for the next fog patch in the pool.
// Transparency is modulated by the patch's current angular velocity: faster spin → less transparent.
FOG_Info FOG_get_info()
{
    SLONG trans;

    FOG_Fog* ff;
    FOG_Info ans;

    if (FOG_get_upto >= FOG_MAX_FOG) {
        ans.type = FOG_TYPE_NO_MORE;
    } else {
        ASSERT(WITHIN(FOG_get_upto, 0, FOG_MAX_FOG - 1));

        ff = &FOG_fog[FOG_get_upto];

        trans = 32 - (abs(ff->dyaw) >> 2);

        if (trans < 8) {
            trans = 8;
        }

        ans.type = ff->type;
        ans.size = ff->size;
        ans.yaw = ff->yaw & 2047;
        ans.trans = trans;
        ans.x = ff->x;
        ans.y = ff->y;
        ans.z = ff->z;

        FOG_get_upto += 1;
    }

    return ans;
}
