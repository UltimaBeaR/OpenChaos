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
