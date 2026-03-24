// 3x3 float matrix operations for 3D transforms.
// All angles in radians. Row-major float[9] layout.

#include "engine/core/matrix.h"

#include <math.h>

// uc_orig: MATRIX_calc (fallen/DDEngine/Source/Matrix.cpp)
void MATRIX_calc(float matrix[9], float yaw, float pitch, float roll)
{

    float cy, cp, cr;
    float sy, sp, sr;

    sy = sin(yaw);
    sp = sin(pitch);
    sr = sin(roll);

    cy = cos(yaw);
    cp = cos(pitch);
    cr = cos(roll);

    matrix[0] = cy * cr + sy * sp * sr;
    matrix[3] = cy * sr - sy * sp * cr;
    matrix[6] = sy * cp;
    matrix[1] = -cp * sr;
    matrix[4] = cp * cr;
    matrix[7] = sp;
    matrix[2] = -sy * cr + cy * sp * sr;
    matrix[5] = -sy * sr - cy * sp * cr;
    matrix[8] = cy * cp;
}

// uc_orig: MATRIX_calc_int (fallen/DDEngine/Source/Matrix.cpp)
void MATRIX_calc_int(SLONG matrix[9], SLONG yaw, SLONG pitch, SLONG roll)
{
    float cy, cp, cr;
    float sy, sp, sr;

    sy = sin(yaw);
    sp = sin(pitch);
    sr = sin(roll);

    cy = cos(yaw);
    cp = cos(pitch);
    cr = cos(roll);

    matrix[0] = cy * cr + sy * sp * sr;
    matrix[3] = cy * sr - sy * sp * cr;
    matrix[6] = sy * cp;
    matrix[1] = -cp * sr;
    matrix[4] = cp * cr;
    matrix[7] = sp;
    matrix[2] = -sy * cr + cy * sp * sr;
    matrix[5] = -sy * sr - cy * sp * cr;
    matrix[8] = cy * cp;
}

// uc_orig: MATRIX_vector (fallen/DDEngine/Source/Matrix.cpp)
void MATRIX_vector(float vector[3], float yaw, float pitch)
{
    float cy, cp;
    float sy, sp;

    sy = sin(yaw);
    sp = sin(pitch);

    cy = cos(yaw);
    cp = cos(pitch);

    vector[0] = sy * cp;
    vector[1] = sp;
    vector[2] = cy * cp;
}

// uc_orig: MATRIX_skew (fallen/DDEngine/Source/Matrix.cpp)
void MATRIX_skew(float matrix[9], float skew, float zoom, float scale)
{
    matrix[0] = matrix[0] * skew;
    matrix[1] = matrix[1] * skew;
    matrix[2] = matrix[2] * skew;

    matrix[0] = zoom * matrix[0];
    matrix[1] = zoom * matrix[1];
    matrix[2] = zoom * matrix[2];

    matrix[3] = zoom * matrix[3];
    matrix[4] = zoom * matrix[4];
    matrix[5] = zoom * matrix[5];

    matrix[0] *= scale;
    matrix[1] *= scale;
    matrix[2] *= scale;
    matrix[3] *= scale;
    matrix[4] *= scale;
    matrix[5] *= scale;
    matrix[6] *= scale;
    matrix[7] *= scale;
    matrix[8] *= scale;
}

// uc_orig: MATRIX_3x3mul (fallen/DDEngine/Source/Matrix.cpp)
void MATRIX_3x3mul(float a[9], float m[9], float n[9])
{
    a[0] = m[0] * n[0] + m[1] * n[3] + m[2] * n[6];
    a[1] = m[0] * n[1] + m[1] * n[4] + m[2] * n[7];
    a[2] = m[0] * n[2] + m[1] * n[5] + m[2] * n[8];

    a[3] = m[3] * n[0] + m[4] * n[3] + m[5] * n[6];
    a[4] = m[3] * n[1] + m[4] * n[4] + m[5] * n[7];
    a[5] = m[3] * n[2] + m[4] * n[5] + m[5] * n[8];

    a[6] = m[6] * n[0] + m[7] * n[3] + m[8] * n[6];
    a[7] = m[6] * n[1] + m[7] * n[4] + m[8] * n[7];
    a[8] = m[6] * n[2] + m[7] * n[5] + m[8] * n[8];
}

// uc_orig: MATRIX_ROTATE_ABOUT_Z (fallen/DDEngine/Source/Matrix.cpp)
#define MATRIX_ROTATE_ABOUT_Z(ax, ay, az, bx, by, bz, cx, cy, cz, angle) \
    {                                                                    \
        float c = cos(angle);                                            \
        float s = sin(angle);                                            \
                                                                         \
        float px;                                                        \
        float py;                                                        \
        float pz;                                                        \
                                                                         \
        float qx;                                                        \
        float qy;                                                        \
        float qz;                                                        \
                                                                         \
        if (angle) {                                                     \
            px = c * (ax) + s * (bx);                                    \
            py = c * (ay) + s * (by);                                    \
            pz = c * (az) + s * (bz);                                    \
                                                                         \
            qx = -s * (ax) + c * (bx);                                   \
            qy = -s * (ay) + c * (by);                                   \
            qz = -s * (az) + c * (bz);                                   \
                                                                         \
            (ax) = px;                                                   \
            (ay) = py;                                                   \
            (az) = pz;                                                   \
            (bx) = qx;                                                   \
            (by) = qy;                                                   \
            (bz) = qz;                                                   \
        }                                                                \
    }

// uc_orig: MATRIX_rotate_about_its_x (fallen/DDEngine/Source/Matrix.cpp)
void MATRIX_rotate_about_its_x(float* matrix, float angle)
{
    MATRIX_ROTATE_ABOUT_Z(
        matrix[1], matrix[4], matrix[7],
        matrix[2], matrix[5], matrix[8],
        matrix[0], matrix[3], matrix[6],
        -angle);
}

// uc_orig: MATRIX_rotate_about_its_y (fallen/DDEngine/Source/Matrix.cpp)
void MATRIX_rotate_about_its_y(float* matrix, float angle)
{
    MATRIX_ROTATE_ABOUT_Z(
        matrix[2], matrix[5], matrix[8],
        matrix[0], matrix[3], matrix[6],
        matrix[1], matrix[4], matrix[7],
        -angle);
}

// uc_orig: MATRIX_rotate_about_its_z (fallen/DDEngine/Source/Matrix.cpp)
void MATRIX_rotate_about_its_z(float* matrix, float angle)
{
    MATRIX_ROTATE_ABOUT_Z(
        matrix[0], matrix[3], matrix[6],
        matrix[1], matrix[4], matrix[7],
        matrix[2], matrix[5], matrix[8],
        -angle);
}

// uc_orig: MATRIX_FA_VECTOR_TOO_SMALL (fallen/DDEngine/Source/Matrix.cpp)
#define MATRIX_FA_VECTOR_TOO_SMALL (0.001F)
// uc_orig: MATRIX_FA_ANGLE_TOO_SMALL (fallen/DDEngine/Source/Matrix.cpp)
#define MATRIX_FA_ANGLE_TOO_SMALL (PI * 2.0F / 180.0F)

// uc_orig: MATRIX_find_angles_old (fallen/DDEngine/Source/Matrix.cpp)
Direction MATRIX_find_angles_old(float matrix[9])
{
    float x;
    float y;
    float z;
    float xz;

    Direction ans;

    x = matrix[6];
    y = matrix[7];
    z = matrix[8];

    if (fabsf(x) + fabsf(z) < MATRIX_FA_VECTOR_TOO_SMALL) {
        float x1 = matrix[0];
        float z1 = matrix[2];

        ans.yaw = atan2(-z1, x1);
    } else {
        ans.yaw = atan2(x, z);
    }

    xz = sqrt(x * x + z * z);

    if (fabsf(xz) + fabsf(y) < MATRIX_FA_VECTOR_TOO_SMALL) {
        if (y < 0) {
            ans.pitch = -PI;
        } else {
            ans.pitch = +PI;
        }
    } else {
        ans.pitch = atan2(y, xz);
    }

    float cos_roll;
    float sin_roll;
    float cos_pitch;

    cos_pitch = cos(ans.pitch);

    if (fabsf(cos_pitch) < MATRIX_FA_ANGLE_TOO_SMALL) {
        ans.roll = 0.0F;
    } else {
        cos_roll = matrix[4] / cos_pitch;
        sin_roll = matrix[1] / -cos_pitch;

        ans.roll = atan2(sin_roll, cos_roll);
    }

    return ans;
}

// uc_orig: MATRIX_find_angles (fallen/DDEngine/Source/Matrix.cpp)
Direction MATRIX_find_angles(float matrix[9])
{
    Direction ans;

    ans.pitch = (float)asin(matrix[7]);

    if (fabsf(ans.pitch) > (PI / 4.0F)) {
        if (fabsf(matrix[0]) + fabsf(matrix[2]) < 0.1F) {
        }

        ans.yaw = (float)atan2(matrix[0], matrix[2]) - (PI / 2.0F);
    } else {
        ans.yaw = (float)atan2(matrix[6], matrix[8]);
    }

    float cos_roll;
    float sin_roll;
    float cos_pitch;

    cos_pitch = (float)cos(ans.pitch);

    if (fabsf(cos_pitch) < 0.0001F) {
        ans.roll = 0.0F;
    } else {
        cos_roll = matrix[4] / cos_pitch;
        sin_roll = matrix[1] / -cos_pitch;

        ans.roll = (float)atan2(sin_roll, cos_roll);
    }

    return ans;
}
