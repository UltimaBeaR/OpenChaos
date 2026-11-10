#include "effects/environment/snowflake.h"

#include <math.h>

#include "engine/core/macros.h"
#include "engine/graphics/pipeline/aeng_globals.h"
#include "game/game_types.h"
#include "map/pap.h"

// Fall speed of a snowflake once it reaches terminal velocity, in world units
// per physics tick. DIRT_process adds -(4 << TICK_SHIFT) to dy every tick and
// halves dy as drag, so dy settles where dy = dy / 2 - (4 << TICK_SHIFT), i.e.
// dy = -(8 << TICK_SHIFT): 8 world units per tick.
static const float SNOW_FALL_UNITS_PER_TICK = 2.0F * 4.0F;

// How long before touching the ground a falling flake starts to tip over from
// facing the camera to lying flat. Short enough to read as "lands flat", long
// enough not to look like a snap.
static const float SNOW_TIP_OVER_SECONDS = 0.25F;

// Height band above the ground in which the tip-over happens: the distance a
// flake falls in SNOW_TIP_OVER_SECONDS at terminal velocity.
static const float SNOW_TIP_OVER_HEIGHT = SNOW_FALL_UNITS_PER_TICK * float(UC_PHYSICS_DESIGN_HZ) * SNOW_TIP_OVER_SECONDS;

// A flake kicked up by a character carries pitch/roll spin from the leaf
// physics. A camera-facing flake cannot tumble, so the spin becomes a rotation
// in its own plane, damped so it reads as a sway rather than a spin.
static const float SNOW_SWAY_SCALE = 0.5F;

// Leaf/snow pitch and roll units: 2048 per full turn (see AENG_draw_dirt).
static const int SNOW_ANGLE_FULL_TURN = 2048;
static const int SNOW_ANGLE_HALF_TURN = SNOW_ANGLE_FULL_TURN / 2;
static const float SNOW_ANGLE_TO_RADIANS = PI / float(SNOW_ANGLE_HALF_TURN);

// Below this length a horizontal direction is treated as degenerate.
static const float SNOW_MIN_AXIS_LENGTH = 1e-4F;

static void snow_normalise(float v[3])
{
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > SNOW_MIN_AXIS_LENGTH) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

// Pitch/roll swing around zero, but render interpolation hands them over
// wrapped to [0, 2048), so a small negative tilt arrives as nearly a full
// turn. The leaf code only takes sin/cos and does not care; the sway below
// scales the angle, so bring it back to [-1024, 1024) first.
static int snow_signed_angle(int angle)
{
    angle &= SNOW_ANGLE_FULL_TURN - 1;
    if (angle >= SNOW_ANGLE_HALF_TURN) {
        angle -= SNOW_ANGLE_FULL_TURN;
    }
    return angle;
}

// Blend factor between facing the camera (0) and lying flat (1), eased so the
// tip-over starts and ends gently.
static float snow_flatness(const DIRT_Dirt* dd)
{
    if (dd->flag & DIRT_FLAG_STILL) {
        return 1.0F;
    }

    float height = float(dd->y - PAP_calc_height_at(dd->x, dd->z));
    float t = 1.0F - height / SNOW_TIP_OVER_HEIGHT;
    if (t <= 0.0F) {
        return 0.0F;
    }
    if (t >= 1.0F) {
        return 1.0F;
    }
    return t * t * (3.0F - 2.0F * t);
}

void SNOWFLAKE_calc_corners(const DIRT_Dirt* dd, float size, float up, float wx[3], float wy[3], float wz[3])
{
    // Camera axes in world space: rows of the view matrix.
    const float* cam_right = &AENG_cam_matrix[0];
    const float* cam_up = &AENG_cam_matrix[3];

    // Flat basis: the camera's right and forward directions laid on the
    // ground. Following the camera's heading (rather than fixed world axes)
    // keeps the tip-over a pure rotation about the right axis, with no twist,
    // and resting flakes keep the same look as the camera turns.
    // The right axis is horizontal unless the camera rolls; if a roll ever
    // stood it on end, fall back to world X.
    float flat_a[3] = { cam_right[0], 0.0F, cam_right[2] };
    if (fabsf(flat_a[0]) + fabsf(flat_a[2]) < SNOW_MIN_AXIS_LENGTH) {
        flat_a[0] = 1.0F;
        flat_a[2] = 0.0F;
    }
    snow_normalise(flat_a);
    // Forward on the ground = right x world up. The camera normally looks
    // down, so its up axis leans forward and tipping it onto this vector is
    // a turn of less than 90 degrees.
    float flat_b[3] = { -flat_a[2], 0.0F, flat_a[0] };

    float t = snow_flatness(dd);

    float a[3], b[3];
    for (int k = 0; k < 3; k++) {
        a[k] = cam_right[k] + (flat_a[k] - cam_right[k]) * t;
        b[k] = cam_up[k] + (flat_b[k] - cam_up[k]) * t;
    }
    snow_normalise(a);
    snow_normalise(b);

    if ((dd->pitch | dd->roll) != 0) {
        int tilt = snow_signed_angle(dd->pitch) + snow_signed_angle(dd->roll);
        float sway = float(tilt) * SNOW_ANGLE_TO_RADIANS * SNOW_SWAY_SCALE;
        float c = cosf(sway);
        float s = sinf(sway);
        for (int k = 0; k < 3; k++) {
            float ak = a[k];
            a[k] = c * ak + s * b[k];
            b[k] = c * b[k] - s * ak;
        }
    }

    // Same triangle as the leaf code, in the (a, b) plane:
    // (0, +size), (+size, -size), (-size, -size).
    float cx = float(dd->x);
    float cy = float(dd->y) + up;
    float cz = float(dd->z);

    wx[0] = cx + b[0] * size;
    wy[0] = cy + b[1] * size;
    wz[0] = cz + b[2] * size;

    wx[1] = cx + (a[0] - b[0]) * size;
    wy[1] = cy + (a[1] - b[1]) * size;
    wz[1] = cz + (a[2] - b[2]) * size;

    wx[2] = cx + (-a[0] - b[0]) * size;
    wy[2] = cy + (-a[1] - b[1]) * size;
    wz[2] = cz + (-a[2] - b[2]) * size;
}
