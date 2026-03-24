#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "things/items/hook.h"
#include "things/items/hook_globals.h"
#include "world/map/pap.h"
#include "engine/core/fmatrix.h"

// Physics constants for the rope simulation.
// uc_orig: HOOK_POINT_DIST (fallen/Source/hook.cpp)
#define HOOK_POINT_DIST (0x800)
// uc_orig: HOOK_POINT_GRAVITY (fallen/Source/hook.cpp)
#define HOOK_POINT_GRAVITY (-0x80)
// uc_orig: HOOK_POINT_FRICTION (fallen/Source/hook.cpp)
#define HOOK_POINT_FRICTION (2)
// uc_orig: HOOK_POINT_ATTRACT (fallen/Source/hook.cpp)
#define HOOK_POINT_ATTRACT (2)
// uc_orig: HOOK_POINT_JUMP (fallen/Source/hook.cpp)
#define HOOK_POINT_JUMP (0x1800)
// uc_orig: HOOK_GRAPPLE_GRAVITY (fallen/Source/hook.cpp)
#define HOOK_GRAPPLE_GRAVITY (-0x80)

// Layout constants for the coiled loop.
// uc_orig: HOOK_LOOP_RADIUS (fallen/Source/hook.cpp)
#define HOOK_LOOP_RADIUS (0x2000)
// uc_orig: HOOK_LOOP_ANGLE (fallen/Source/hook.cpp)
#define HOOK_LOOP_ANGLE ((1436 * 2048) / 36000) // 14.36 degrees
// uc_orig: HOOK_LOOP_RAISE (fallen/Source/hook.cpp)
#define HOOK_LOOP_RAISE (0x0)

// Number of fixed string points connecting Darci's hand to the grapple head.
// uc_orig: HOOK_NUM_HOLD (fallen/Source/hook.cpp)
#define HOOK_NUM_HOLD 5

// Arranges all string points in a flat coil at the given map position.
// Used when initialising or resetting the hook.
// uc_orig: HOOK_make_loop (fallen/Source/hook.cpp)
static void HOOK_make_loop(SLONG x, SLONG z)
{
    SLONG i;
    SLONG dy;
    SLONG angle;
    SLONG dx;
    SLONG dz;
    SLONG mx;
    SLONG mz;
    SLONG ground;

    HOOK_Point* hp;

    dy = 0;
    angle = 0;
    ground = PAP_calc_height_at(x, z) << 8;

    mx = x << 8;
    mz = z << 8;

    for (i = HOOK_NUM_POINTS - 1; i >= 0; i--) {
        hp = &HOOK_point[i];

        dx = SIN(angle) * HOOK_LOOP_RADIUS >> 16;
        dz = COS(angle) * HOOK_LOOP_RADIUS >> 16;

        hp->x = mx + dx;
        hp->z = mz + dz;
        hp->y = ground + dy;
        hp->dx = 0;
        hp->dy = 0;
        hp->dz = 0;
        hp->alive = UC_FALSE;

        mx += SIN(i << 2) >> 11;
        mz += COS(i << 2) >> 11;

        dy += HOOK_LOOP_RAISE;
        angle += HOOK_LOOP_ANGLE;

        angle &= 2047;
    }
}

// uc_orig: HOOK_init (fallen/Source/hook.cpp)
void HOOK_init(SLONG x, SLONG z)
{
    HOOK_make_loop(x, z);
    HOOK_grapple_pitch = 512;
}

// Advances physics simulation for all active string points from start_point to the last reeled point.
// Applies link forces, gravity, friction, and ground collision.
// Automatically unreels additional points if tension demands it.
// uc_orig: HOOK_process_points (fallen/Source/hook.cpp)
static void HOOK_process_points(SLONG start_point)
{
    SLONG i;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG ddx;
    SLONG ddy;
    SLONG ddz;

    SLONG dist;
    SLONG ddist;
    SLONG ground;

    HOOK_Point* hp;
    HOOK_Point* hp_near;

    i = start_point;

    while (1) {
        ASSERT(WITHIN(i, 1, HOOK_NUM_POINTS - 1));

        hp = &HOOK_point[i];
        hp_near = &HOOK_point[i - 1];

        dx = hp_near->x - hp->x;
        dy = hp_near->y - hp->y;
        dz = hp_near->z - hp->z;

        dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;

        if (dist > HOOK_POINT_JUMP) {
            // Too far from the previous point — snap closer to avoid overflow.
            hp->x += (dx * (dist - HOOK_POINT_JUMP >> 8)) / (dist >> 8);
            hp->y += (dy * (dist - HOOK_POINT_JUMP >> 8)) / (dist >> 8);
            hp->z += (dz * (dist - HOOK_POINT_JUMP >> 8)) / (dist >> 8);

            dx = hp_near->x - hp->x;
            dy = hp_near->y - hp->y;
            dz = hp_near->z - hp->z;

            dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;
        }

        ddist = dist;
        ddist -= HOOK_POINT_DIST;
        ddist >>= HOOK_POINT_ATTRACT;

        ddx = (dx * ddist) / dist;
        ddy = (dy * ddist) / dist;
        ddz = (dz * ddist) / dist;

        hp->dx += ddx;
        hp->dy += ddy;
        hp->dz += ddz;

        if (i < HOOK_NUM_POINTS - 1) {
            hp_near = &HOOK_point[i + 1];

            dx = hp_near->x - hp->x;
            dy = hp_near->y - hp->y;
            dz = hp_near->z - hp->z;

            dist = QDIST3(abs(dx), abs(dy), abs(dz)) + 1;
            ddist = dist;
            ddist -= HOOK_POINT_DIST;
            ddist >>= HOOK_POINT_ATTRACT;

            if (dist < HOOK_POINT_JUMP) {
                ddx = dx * ddist / dist;
                ddy = dy * ddist / dist;
                ddz = dz * ddist / dist;

                hp->dx += ddx;
                hp->dy += ddy;
                hp->dz += ddz;
            }
        }

        hp->dy += HOOK_POINT_GRAVITY;

        hp->dx -= hp->dx >> HOOK_POINT_FRICTION;
        hp->dy -= hp->dy >> HOOK_POINT_FRICTION;
        hp->dz -= hp->dz >> HOOK_POINT_FRICTION;

        hp->x += hp->dx;
        hp->y += hp->dy;
        hp->z += hp->dz;

        // Keep point above ground.
        ground = PAP_calc_map_height_at(hp->x >> 8, hp->z >> 8) << 8;

        if (hp->y < ground) {
            dy = ground - hp->y;

            if (dy < 0x2000) {
                hp->y = ground;
                hp->dy = 0;

                if (abs(hp->dx) + abs(hp->dz) < 256) {
                    hp->dx >>= 2;
                    hp->dz >>= 2;
                }
            } else {
                // Sliding along a wall — try backing out the offending axis.
                ground = PAP_calc_map_height_at((hp->x - hp->dx) >> 8, hp->z >> 8) << 8;

                if (hp->y > ground) {
                    hp->x -= hp->dx;
                    hp->dx = 0;
                } else {
                    hp->z -= hp->dz;
                    hp->dz = 0;
                }
            }
        }

        if (i == HOOK_reeled) {
            if (HOOK_reeled == HOOK_NUM_POINTS - 1) {
                break;
            } else {
                // Unreel the next point if the string is under tension.
                if (ddist > 0) {
                    HOOK_reeled += 1;
                } else {
                    break;
                }
            }
        }

        i += 1;
    }
}

// Advances the grapple head and all string points during FLYING state.
// Also detects when the hook has stopped moving and transitions to STILL.
// uc_orig: HOOK_process_flying (fallen/Source/hook.cpp)
static void HOOK_process_flying()
{
    SLONG speed;
    SLONG ground;
    SLONG odd;
    SLONG dy;

    HOOK_point[0].dy += HOOK_GRAPPLE_GRAVITY;

    HOOK_point[0].x += HOOK_point[0].dx * TICK_RATIO >> TICK_SHIFT;
    HOOK_point[0].y += HOOK_point[0].dy * TICK_RATIO >> TICK_SHIFT;
    HOOK_point[0].z += HOOK_point[0].dz * TICK_RATIO >> TICK_SHIFT;

    ground = PAP_calc_map_height_at(
                 HOOK_point[0].x >> 8,
                 HOOK_point[0].z >> 8)
        << 8;

    if (HOOK_point[0].y < ground) {
        dy = ground - HOOK_point[0].y;

        if (dy < 0x4000) {
            // Vertical bounce off flat ground.
            HOOK_point[0].y = ground;
            HOOK_point[0].dy = abs(HOOK_point[0].dy);

            HOOK_point[0].dx /= 2;
            HOOK_point[0].dy /= 2;
            HOOK_point[0].dz /= 2;

            speed = abs(HOOK_point[0].dx);
            speed += abs(HOOK_point[0].dy);
            speed += abs(HOOK_point[0].dz);
            speed >>= 4;
            speed += 1;

            odd = rand() % speed;
            odd -= speed >> 1;

            HOOK_point[0].dx += odd << 3;

            odd = rand() % speed;
            odd -= speed >> 1;

            HOOK_point[0].dz += odd << 3;

            HOOK_spin_speed = rand() & 0x1f;
        } else {
            // Bouncing off a wall — determine which axis to reflect.
            SLONG check_x;
            SLONG check_z;
            SLONG check_height;
            SLONG bounced_x = UC_FALSE;
            SLONG bounced_z = UC_FALSE;

            check_x = HOOK_point[0].x - HOOK_point[0].dx >> 8;
            check_z = HOOK_point[0].z >> 8;

            check_height = PAP_calc_map_height_at(check_x, check_z) << 8;
            bounced_x = (check_height < HOOK_point[0].y);

            check_x = HOOK_point[0].x >> 8;
            check_z = HOOK_point[0].z - HOOK_point[0].dz >> 8;

            check_height = PAP_calc_map_height_at(check_x, check_z) << 8;
            bounced_z = (check_height < HOOK_point[0].y);

            if (bounced_x) {
                HOOK_point[0].dx = -HOOK_point[0].dx;
                HOOK_point[0].x += HOOK_point[0].dx;
            }

            if (bounced_z) {
                HOOK_point[0].dz = -HOOK_point[0].dz;
                HOOK_point[0].z += HOOK_point[0].dz;
            }

            HOOK_spin_speed = rand() & 0x1f;
        }
    }

    HOOK_process_points(1);

    speed = abs(HOOK_point[0].dx);
    speed += abs(HOOK_point[0].dy);
    speed += abs(HOOK_point[0].dz);

    if (speed < 0x100) {
        HOOK_state = HOOK_STATE_STILL;
        HOOK_countdown = 512;
    }
}

// Updates the fixed hold-points (index 0..HOOK_NUM_HOLD) to follow the spin pivot,
// then advances the free string points.
// uc_orig: HOOK_process_spinning (fallen/Source/hook.cpp)
static void HOOK_process_spinning()
{
    SLONG i;

    SLONG vector[3];

    SLONG old_x;
    SLONG old_y;
    SLONG old_z;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    SLONG px;
    SLONG py;
    SLONG pz;

    old_x = HOOK_point[0].x;
    old_y = HOOK_point[0].y;
    old_z = HOOK_point[0].z;

    FMATRIX_vector(vector, HOOK_grapple_yaw, HOOK_grapple_pitch);

    vector[0] = vector[0] * HOOK_POINT_DIST >> 16;
    vector[1] = vector[1] * HOOK_POINT_DIST >> 16;
    vector[2] = vector[2] * HOOK_POINT_DIST >> 16;

    px = HOOK_spin_x;
    py = HOOK_spin_y;
    pz = HOOK_spin_z;

    for (i = HOOK_NUM_HOLD; i >= 0; i--) {
        HOOK_point[i].x = px;
        HOOK_point[i].y = py;
        HOOK_point[i].z = pz;

        px += vector[0];
        py += vector[1];
        pz += vector[2];
    }

    dx = ((HOOK_point[0].x - old_x) * (1 << (TICK_SHIFT))) / TICK_RATIO;
    dy = ((HOOK_point[0].y - old_y) * (1 << (TICK_SHIFT))) / TICK_RATIO;
    dz = ((HOOK_point[0].z - old_z) * (1 << (TICK_SHIFT))) / TICK_RATIO;

    HOOK_point[0].dx = dx;
    HOOK_point[0].dy = dy;
    HOOK_point[0].dz = dz;

    HOOK_process_points(HOOK_NUM_HOLD + 1);

    HOOK_grapple_pitch -= HOOK_spin_speed;
    HOOK_grapple_pitch &= 2047;
}

// uc_orig: HOOK_get_state (fallen/Source/hook.cpp)
SLONG HOOK_get_state()
{
    return HOOK_state;
}

// uc_orig: HOOK_spin (fallen/Source/hook.cpp)
void HOOK_spin(SLONG x, SLONG y, SLONG z, SLONG yaw, SLONG speed_or_minus_pitch)
{
    HOOK_spin_x = x << 8;
    HOOK_spin_y = y << 8;
    HOOK_spin_z = z << 8;
    HOOK_grapple_yaw = yaw;
    HOOK_state = HOOK_STATE_SPINNING;
    HOOK_reeled = HOOK_NUM_HOLD + 1;

    if (speed_or_minus_pitch > 0) {
        HOOK_spin_speed = speed_or_minus_pitch;
    } else {
        HOOK_spin_speed = 0;
        HOOK_grapple_pitch = -(speed_or_minus_pitch) & 2047;
    }
}

// uc_orig: HOOK_release (fallen/Source/hook.cpp)
void HOOK_release()
{
    ASSERT(HOOK_state == HOOK_STATE_SPINNING);
    HOOK_state = HOOK_STATE_FLYING;
}

// uc_orig: HOOK_process (fallen/Source/hook.cpp)
void HOOK_process()
{
    switch (HOOK_state) {
    case HOOK_STATE_STILL:
        // Keep simulating for a while after landing to let the string settle.
        if (HOOK_countdown) {
            HOOK_process_flying();
            HOOK_countdown -= 1;
        }
        break;

    case HOOK_STATE_SPINNING:
        HOOK_process_spinning();
        break;

    case HOOK_STATE_FLYING:
        // Run flying physics twice per tick for extra accuracy.
        HOOK_process_flying();
        HOOK_process_flying();
        break;

    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: HOOK_pos_grapple (fallen/Source/hook.cpp)
void HOOK_pos_grapple(SLONG* x, SLONG* y, SLONG* z, SLONG* yaw, SLONG* pitch, SLONG* roll)
{
    *x = HOOK_point[0].x;
    *y = HOOK_point[0].y + 0x1000;
    *z = HOOK_point[0].z;

    *yaw = HOOK_grapple_yaw;
    *pitch = HOOK_grapple_pitch;
    *roll = 0;
}

// uc_orig: HOOK_pos_point (fallen/Source/hook.cpp)
void HOOK_pos_point(SLONG point, SLONG* x, SLONG* y, SLONG* z)
{
    ASSERT(WITHIN(point, 0, HOOK_NUM_POINTS - 1));

    *x = HOOK_point[point].x;
    *y = HOOK_point[point].y;
    *z = HOOK_point[point].z;
}
