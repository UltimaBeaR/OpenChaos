#include "fallen/Headers/Game.h"    // Temporary: TICK_RATIO, TICK_SHIFT, MUL64, DIV64, Root, calc_angle, WITHIN, QDIST2, SATURATE
#include "world/map/pap_globals.h"
#include "world/map/road.h"
#include "core/fmatrix.h"
#include "actors/items/dike.h"
#include "actors/items/dike_globals.h"

// Physical constants for the bike simulation.
// uc_orig: DIKE_WHEEL_APART (fallen/Source/dike.cpp)
#define DIKE_WHEEL_APART (0xc0)
// uc_orig: DIKE_WHEEL_RADIUS (fallen/Source/dike.cpp)
#define DIKE_WHEEL_RADIUS (0x50)
// uc_orig: DIKE_GRAVITY (fallen/Source/dike.cpp)
#define DIKE_GRAVITY (-0x400)

// Returns grip coefficient in 8.8 fixed point for position (x,z).
// Roads give full grip (256); off-road gives half (128).
// uc_orig: DIKE_get_grip (fallen/Source/dike.cpp)
static SLONG DIKE_get_grip(SLONG x, SLONG z)
{
    if (ROAD_is_road(x >> 8, z >> 8)) {
        return 256;
    } else {
        return 128;
    }
}

// uc_orig: DIKE_init (fallen/Source/dike.cpp)
void DIKE_init()
{
    memset(DIKE_dike, 0, sizeof(DIKE_Dike) * DIKE_MAX_DIKES);
    DIKE_dike_upto = 0;
}

// uc_orig: DIKE_create (fallen/Source/dike.cpp)
DIKE_Dike* DIKE_create(SLONG x, SLONG z, SLONG yaw)
{
    DIKE_Dike* dd;

    if (!WITHIN(DIKE_dike_upto, 0, DIKE_MAX_DIKES - 1)) {
        return NULL;
    }

    dd = &DIKE_dike[DIKE_dike_upto++];

    SLONG vector[3];

    FMATRIX_vector(vector, 0, 0);

    vector[0] = vector[0] * DIKE_WHEEL_APART >> 9;
    vector[2] = vector[2] * DIKE_WHEEL_APART >> 9;

    dd->fx = (x << 8) + vector[0];
    dd->fz = (z << 8) + vector[2];

    dd->bx = (x << 8) - vector[0];
    dd->bz = (z << 8) - vector[2];

    dd->fy = PAP_calc_map_height_at(dd->fx >> 8, dd->fz >> 8) + DIKE_WHEEL_RADIUS << 8;
    dd->by = PAP_calc_map_height_at(dd->bx >> 8, dd->bz >> 8) + DIKE_WHEEL_RADIUS << 8;

    return dd;
}

// uc_orig: DIKE_process (fallen/Source/dike.cpp)
void DIKE_process(DIKE_Dike* dd)
{
    SLONG fx;
    SLONG fy;
    SLONG fz;

    SLONG grip;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG dprod;

    SLONG dist;
    SLONG ddist;

    SLONG radx;
    SLONG radz;

    SLONG steer[3];
    SLONG vector[3];

    SLONG sx;
    SLONG sy;
    SLONG sz;

    SLONG dsx;
    SLONG dsy;
    SLONG dsz;

    SLONG sdist;
    SLONG oversdist;

    SLONG ground;

    // Apply user input to power and steering.
    if (dd->control & DIKE_CONTROL_ACCEL) {
        dd->power += 8;
    } else {
        if (dd->power > 0) {
            dd->power -= 1;
        }
    }

    if (dd->control & DIKE_CONTROL_LEFT) {
        dd->steer -= 7;
    }

    if (dd->control & DIKE_CONTROL_RIGHT) {
        dd->steer += 7;
    }

    SATURATE(dd->steer, -80, +80);
    SATURATE(dd->power, -80, +80);

    // Apply drive force to back wheel.
    if (dd->flag & DIKE_FLAG_ON_GROUND_BACK) {
        grip = DIKE_get_grip(dd->bx >> 8, dd->bz >> 8);

        FMATRIX_vector(vector, dd->yaw, 0);

        fx = vector[0] * (dd->power * grip >> 8) >> 13;
        fy = vector[1] * (dd->power * grip >> 8) >> 13;
        fz = vector[2] * (dd->power * grip >> 8) >> 13;

        dd->bdx += fx;
        dd->bdy += fy;
        dd->bdz += fz;

        // Cancel lateral sliding based on grip level.
        radx = -vector[2];
        radz = vector[0];

        dprod = MUL64(radx, dd->bdx) + MUL64(radz, dd->bdz);
        dprod *= grip;
        dprod >>= 8;

        dd->bdx -= radx * dprod >> 16;
        dd->bdz -= radz * dprod >> 16;
    }

    // Gravity on each wheel.
    dd->bdy += DIKE_GRAVITY;
    dd->fdy += DIKE_GRAVITY;

    // Compute frame spring force between wheels.
// uc_orig: DIKE_DIST_SHIFT (fallen/Source/dike.cpp)
#define DIKE_DIST_SHIFT 2

    dx = dd->fx - dd->bx >> (DIKE_DIST_SHIFT);
    dy = dd->fy - dd->by >> (DIKE_DIST_SHIFT);
    dz = dd->fz - dd->bz >> (DIKE_DIST_SHIFT);

    dist = Root(dx * dx + dy * dy + dz * dz) << (DIKE_DIST_SHIFT);

    dx = dd->fx - dd->bx;
    dy = dd->fy - dd->by;
    dz = dd->fz - dd->bz;

    ddist = dist - (DIKE_WHEEL_APART << 8);

    fx = dx * ddist >> 19;
    fy = dy * ddist >> 19;
    fz = dz * ddist >> 19;

    dd->fdx -= fx;
    dd->fdy -= fy;
    dd->fdz -= fz;

    dd->bdx += fx;
    dd->bdy += fy;
    dd->bdz += fz;

    // Move back wheel.
    dd->bx += dd->bdx;
    dd->by += dd->bdy;
    dd->bz += dd->bdz;

    // Move front wheel along steering direction.
    if (dd->flag & DIKE_FLAG_ON_GROUND_FRONT) {
        // Blend steering vector with bike direction proportional to grip.
        FMATRIX_vector(steer, (dd->yaw + (dd->steer << 2)) & 2047, dd->pitch);
        FMATRIX_vector(vector, dd->yaw, 0);

        grip = DIKE_get_grip(dd->fx >> 8, dd->fz >> 8);

        sx = (steer[0] * grip >> 8) + (vector[0] * (256 - grip) >> 8);
        sy = (steer[1] * grip >> 8) + (vector[1] * (256 - grip) >> 8);
        sz = (steer[2] * grip >> 8) + (vector[2] * (256 - grip) >> 8);

        // Normalise the blended direction vector.
        dsx = sx >> 2;
        dsy = sy >> 2;
        dsz = sz >> 2;

        sdist = Root(dsx * dsx + dsy * dsy + dsz * dsz) << 2;
        oversdist = DIV64(0x10000, sdist);

        sx = MUL64(sx, oversdist);
        sy = MUL64(sy, oversdist);
        sz = MUL64(sz, oversdist);

        // Project velocity onto rolling direction.
        dprod = MUL64(dd->fdx, sx) + MUL64(dd->fdy, sy) + MUL64(dd->fdz, sz);

        dd->fdx = MUL64(dprod, sx);
        dd->fdy = MUL64(dprod, sy);
        dd->fdz = MUL64(dprod, sz);
    }

    dd->fx += dd->fdx;
    dd->fy += dd->fdy;
    dd->fz += dd->fdz;

    // Ground collision for back wheel.
    dd->flag &= ~DIKE_FLAG_ON_GROUND_BACK;
    ground = PAP_calc_map_height_at(dd->bx >> 8, dd->bz >> 8) << 8;

    if (dd->by < ground + (DIKE_WHEEL_RADIUS << 8)) {
        dd->by = ground + (DIKE_WHEEL_RADIUS << 8);
        dd->bdy = 0;
        dd->flag |= DIKE_FLAG_ON_GROUND_BACK;
    }

    // Ground collision for front wheel.
    dd->flag &= ~DIKE_FLAG_ON_GROUND_FRONT;
    ground = PAP_calc_map_height_at(dd->fx >> 8, dd->fz >> 8) << 8;

    if (dd->fy < ground + (DIKE_WHEEL_RADIUS << 8)) {
        dd->fy = ground + (DIKE_WHEEL_RADIUS << 8);
        dd->fdy = 0;
        dd->flag |= DIKE_FLAG_ON_GROUND_FRONT;
    }

    // Damping.
    dd->fdx -= dd->fdx / 16;
    dd->fdy -= dd->fdy / 16;
    dd->fdz -= dd->fdz / 16;

    dd->bdx -= dd->bdx / 16;
    dd->bdy -= dd->bdy / 16;
    dd->bdz -= dd->bdz / 16;

    // Compute bike yaw and pitch from wheel positions.
    SLONG dxdz = QDIST2(abs(dx), abs(dz));

    if (dxdz >= 0x10) {
        dd->yaw = calc_angle(dx, dz);
        dd->yaw &= 2047;
    }

    dd->pitch = calc_angle(dy, dxdz);
}

// uc_orig: DIKE_draw (fallen/Source/dike.cpp)
void DIKE_draw(DIKE_Dike* dd)
{
    SLONG steer[3];

    AENG_world_line(
        dd->fx >> 8,
        dd->fy >> 8,
        dd->fz >> 8,
        0,
        0xffeeee,
        dd->bx >> 8,
        dd->by >> 8,
        dd->bz >> 8,
        32,
        0xffffff,
        UC_FALSE);

    FMATRIX_vector(steer, (dd->yaw + (dd->steer << 2)) & 2047, dd->pitch);

    steer[0] >>= 3;
    steer[1] >>= 3;
    steer[2] >>= 3;

    AENG_world_line(
        dd->fx + steer[0] >> 8,
        dd->fy + steer[1] >> 8,
        dd->fz + steer[2] >> 8,
        0,
        0x00ffee,
        dd->fx - steer[0] >> 8,
        dd->fy - steer[1] >> 8,
        dd->fz - steer[2] >> 8,
        16,
        0xffffff,
        UC_FALSE);
}
