// Integer 3x3 matrix operations using PSX angle system and 16.16 fixed-point.

#include "engine/core/fmatrix.h"
#include "engine/core/math.h"

// uc_orig: FMATRIX_calc (fallen/Source/FMatrix.cpp)
void FMATRIX_calc(SLONG matrix[9], SLONG yaw, SLONG pitch, SLONG roll)
{
    SLONG cy, cp, cr;
    SLONG sy, sp, sr;

    if (pitch && roll) {
        cy = COS(yaw & 2047);
        cp = COS(pitch & 2047);
        cr = COS(roll & 2047);

        sy = SIN(yaw & 2047);
        sp = SIN(pitch & 2047);
        sr = SIN(roll & 2047);

        matrix[0] = MUL64(cy, cr) + MUL64(MUL64(sy, sp), sr);
        matrix[3] = MUL64(cy, sr) - MUL64(MUL64(sy, sp), cr);
        matrix[6] = MUL64(sy, cp);
        matrix[1] = MUL64(-cp, sr);
        matrix[4] = MUL64(cp, cr);
        matrix[7] = sp;
        matrix[2] = MUL64(-sy, cr) + MUL64(MUL64(cy, sp), sr);
        matrix[5] = MUL64(-sy, sr) - MUL64(MUL64(cy, sp), cr);
        matrix[8] = MUL64(cy, cp);
    } else if (pitch) {
        cy = COS(yaw & 2047);
        cp = COS(pitch & 2047);

        sy = SIN(yaw & 2047);
        sp = SIN(pitch & 2047);

        matrix[0] = cy;
        matrix[3] = -MUL64(sy, sp);
        matrix[6] = MUL64(sy, cp);
        matrix[1] = 0;
        matrix[4] = cp;
        matrix[7] = sp;
        matrix[2] = -sy;
        matrix[5] = -MUL64(cy, sp);
        matrix[8] = MUL64(cy, cp);

    } else if (roll) {
        cy = COS(yaw & 2047);
        cr = COS(roll & 2047);

        sy = SIN(yaw & 2047);
        sr = SIN(roll & 2047);

        matrix[0] = MUL64(cy, cr);
        matrix[3] = MUL64(cy, sr);
        matrix[6] = sy;
        matrix[1] = -sr;
        matrix[4] = cr;
        matrix[7] = 0;
        matrix[2] = MUL64(-sy, cr);
        matrix[5] = MUL64(-sy, sr);
        matrix[8] = cy;

    } else {
        cy = COS(yaw & 2047);

        sy = SIN(yaw & 2047);

        matrix[0] = cy;
        matrix[3] = 0;
        matrix[6] = sy;
        matrix[1] = 0;
        matrix[4] = 65536;
        matrix[7] = 0;
        matrix[2] = -sy;
        matrix[5] = 0;
        matrix[8] = cy;
    }
}

// uc_orig: FMATRIX_vector (fallen/Source/FMatrix.cpp)
void FMATRIX_vector(SLONG vector[3], SLONG yaw, SLONG pitch)
{
    SLONG cy, cp;
    SLONG sy, sp;

    sy = SIN(yaw & 2047);
    sp = SIN(pitch & 2047);

    cy = COS(yaw & 2047);
    cp = COS(pitch & 2047);

    vector[0] = MUL64(sy, cp);
    vector[1] = sp;
    vector[2] = MUL64(cy, cp);
}

// uc_orig: init_matrix33 (fallen/Source/FMatrix.cpp)
void init_matrix33(struct Matrix33* mat)
{
    mat->M[0][0] = (1 << 15);
    mat->M[0][1] = 0;
    mat->M[0][2] = 0;
    mat->M[1][0] = 0;
    mat->M[1][1] = (1 << 15);
    mat->M[1][2] = 0;
    mat->M[2][0] = 0;
    mat->M[2][1] = 0;
    mat->M[2][2] = (1 << 15);
}

// uc_orig: matrix_transformZMY (fallen/Source/FMatrix.cpp)
void matrix_transformZMY(Matrix31* result, Matrix33* trans, Matrix31* mat2)
{
    result->M[0] = (mat2->M[0] * trans->M[0][0]) + (mat2->M[1] * trans->M[0][1]) + (mat2->M[2] * trans->M[0][2]) >> 15;
    result->M[1] = (mat2->M[0] * trans->M[1][0]) + (mat2->M[1] * trans->M[1][1]) + (mat2->M[2] * trans->M[1][2]) >> 15;
    result->M[2] = (mat2->M[0] * trans->M[2][0]) + (mat2->M[1] * trans->M[2][1]) + (mat2->M[2] * trans->M[2][2]) >> 15;
}

// uc_orig: matrix_transform (fallen/Source/FMatrix.cpp)
void matrix_transform(struct Matrix31* result, struct Matrix33* trans, struct Matrix31* mat2)
{
    result->M[0] = (mat2->M[0] * trans->M[0][0]) + (mat2->M[1] * trans->M[0][1]) + (mat2->M[2] * trans->M[0][2]) >> 15;
    result->M[1] = (mat2->M[0] * trans->M[1][0]) + (mat2->M[1] * trans->M[1][1]) + (mat2->M[2] * trans->M[1][2]) >> 15;
    result->M[2] = (mat2->M[0] * trans->M[2][0]) + (mat2->M[1] * trans->M[2][1]) + (mat2->M[2] * trans->M[2][2]) >> 15;
}

// uc_orig: matrix_transform_small (fallen/Source/FMatrix.cpp)
void matrix_transform_small(struct Matrix31* result, struct Matrix33* trans, struct SMatrix31* mat2)
{
    result->M[0] = (mat2->M[0] * trans->M[0][0]) + (mat2->M[1] * trans->M[0][1]) + (mat2->M[2] * trans->M[0][2]) >> 15;
    result->M[1] = (mat2->M[0] * trans->M[1][0]) + (mat2->M[1] * trans->M[1][1]) + (mat2->M[2] * trans->M[1][2]) >> 15;
    result->M[2] = (mat2->M[0] * trans->M[2][0]) + (mat2->M[1] * trans->M[2][1]) + (mat2->M[2] * trans->M[2][2]) >> 15;
}

// uc_orig: normalise_matrix (fallen/Source/FMatrix.cpp)
void normalise_matrix(struct Matrix33* mat)
{
    SLONG c0;

    for (c0 = 0; c0 < 3; c0++) {
        SLONG size;
        size = (mat->M[0][c0] * mat->M[0][c0]);
        size += (mat->M[1][c0] * mat->M[1][c0]);
        size += (mat->M[2][c0] * mat->M[2][c0]);
        size = Root(size);
        if (size == 0)
            size = 1;
        mat->M[0][c0] = (mat->M[0][c0] << 15) / size;
        mat->M[1][c0] = (mat->M[1][c0] << 15) / size;
        mat->M[2][c0] = (mat->M[2][c0] << 15) / size;
    }
}

// uc_orig: normalise_matrix_rows (fallen/Source/FMatrix.cpp)
void normalise_matrix_rows(struct Matrix33* mat)
{
    SLONG c0;

    for (c0 = 0; c0 < 3; c0++) {
        SLONG size;
        size = (mat->M[c0][0] * mat->M[c0][0]);
        size += (mat->M[c0][1] * mat->M[c0][1]);
        size += (mat->M[c0][2] * mat->M[c0][2]);
        size = Root(size);
        if (size == 0)
            size = 1;
        mat->M[c0][0] = (mat->M[c0][0] << 15) / size;
        mat->M[c0][1] = (mat->M[c0][1] << 15) / size;
        mat->M[c0][2] = (mat->M[c0][2] << 15) / size;
    }
}

// uc_orig: MAT_SHIFT (fallen/Source/FMatrix.cpp)
#define MAT_SHIFT (6)
// uc_orig: MAT_SHIFTD (fallen/Source/FMatrix.cpp)
#define MAT_SHIFTD (8 - MAT_SHIFT)

// uc_orig: build_tween_matrix (fallen/Source/FMatrix.cpp)
void build_tween_matrix(struct Matrix33* mat, struct CMatrix33* cmat1, struct CMatrix33* cmat2, SLONG tween)
{
    SLONG v, w;

    v = ((cmat1->M[0] & CMAT0_MASK) << 2) >> 22;
    w = ((cmat2->M[0] & CMAT0_MASK) << 2) >> 22;

    mat->M[0][0] = (v << MAT_SHIFT) + (((w - v) * tween) >> MAT_SHIFTD);

    v = ((cmat1->M[0] & CMAT1_MASK) << 12) >> 22;
    w = ((cmat2->M[0] & CMAT1_MASK) << 12) >> 22;

    mat->M[0][1] = (v << MAT_SHIFT) + (((w - v) * tween) >> MAT_SHIFTD);

    v = ((cmat1->M[0] & CMAT2_MASK) << 22) >> 22;
    w = ((cmat2->M[0] & CMAT2_MASK) << 22) >> 22;

    mat->M[0][2] = (v << MAT_SHIFT) + (((w - v) * tween) >> MAT_SHIFTD);

    v = ((cmat1->M[1] & CMAT0_MASK) << 2) >> 22;
    w = ((cmat2->M[1] & CMAT0_MASK) << 2) >> 22;

    mat->M[1][0] = (v << MAT_SHIFT) + (((w - v) * tween) >> MAT_SHIFTD);

    v = ((cmat1->M[1] & CMAT1_MASK) << 12) >> 22;
    w = ((cmat2->M[1] & CMAT1_MASK) << 12) >> 22;

    mat->M[1][1] = (v << MAT_SHIFT) + (((w - v) * tween) >> MAT_SHIFTD);

    v = ((cmat1->M[1] & CMAT2_MASK) << 22) >> 22;
    w = ((cmat2->M[1] & CMAT2_MASK) << 22) >> 22;

    mat->M[1][2] = (v << MAT_SHIFT) + (((w - v) * tween) >> MAT_SHIFTD);

    v = ((cmat1->M[2] & CMAT0_MASK) << 2) >> 22;
    w = ((cmat2->M[2] & CMAT0_MASK) << 2) >> 22;

    mat->M[2][0] = (v << MAT_SHIFT) + (((w - v) * tween) >> MAT_SHIFTD);

    v = ((cmat1->M[2] & CMAT1_MASK) << 12) >> 22;
    w = ((cmat2->M[2] & CMAT1_MASK) << 12) >> 22;

    mat->M[2][1] = (v << MAT_SHIFT) + (((w - v) * tween) >> MAT_SHIFTD);

    v = ((cmat1->M[2] & CMAT2_MASK) << 22) >> 22;
    w = ((cmat2->M[2] & CMAT2_MASK) << 22) >> 22;

    mat->M[2][2] = (v << MAT_SHIFT) + (((w - v) * tween) >> MAT_SHIFTD);
}

// uc_orig: FMATRIX_find_angles (fallen/Source/FMatrix.cpp)
void FMATRIX_find_angles(SLONG* matrix, SLONG* yaw, SLONG* pitch, SLONG* roll)
{
    SLONG x;
    SLONG y;
    SLONG z;
    SLONG xz;

    x = matrix[6];
    y = matrix[7];
    z = matrix[8];

    if (abs(x) + abs(z) == 0) {
        *yaw = 0;
    } else {
        *yaw = 1024 - Arctan(x, z);
    }

    xz = Root((((x >> 1) * (x >> 1)) >> 14) + (((z >> 1) * (z >> 1)) >> 14)) << 8;

    if (abs(xz) + abs(y) == 0) {
        if (y < 0) {
            *pitch = -1024;
        } else {
            *pitch = +1024;
        }
    } else {
        *pitch = 1024 - Arctan(y, xz);
    }

    SLONG cos_roll;
    SLONG sin_roll;
    SLONG cos_pitch;

    *pitch = (*pitch + 2048) & 2047;
    cos_pitch = COS(*pitch);

    if (abs(cos_pitch) == 0) {
        *roll = 0;
    } else {
        cos_roll = (matrix[4] << 14) / cos_pitch;
        sin_roll = (matrix[1] << 14) / -cos_pitch;

        cos_roll <<= 2;
        sin_roll <<= 2;

        *roll = 1024 - Arctan(sin_roll, cos_roll);
    }

    *roll = (*roll + 2048) & 2047;
    *pitch = (*pitch + 2048) & 2047;
}

// uc_orig: matrix_mult33 (fallen/Source/maths.cpp)
void matrix_mult33(Matrix33* result, Matrix33* mat1, Matrix33* mat2)
{
    result->M[0][0] = ((mat1->M[0][0] * mat2->M[0][0]) + (mat1->M[0][1] * mat2->M[1][0]) + (mat1->M[0][2] * mat2->M[2][0])) >> 15;
    result->M[0][1] = ((mat1->M[0][0] * mat2->M[0][1]) + (mat1->M[0][1] * mat2->M[1][1]) + (mat1->M[0][2] * mat2->M[2][1])) >> 15;
    result->M[0][2] = ((mat1->M[0][0] * mat2->M[0][2]) + (mat1->M[0][1] * mat2->M[1][2]) + (mat1->M[0][2] * mat2->M[2][2])) >> 15;
    result->M[1][0] = ((mat1->M[1][0] * mat2->M[0][0]) + (mat1->M[1][1] * mat2->M[1][0]) + (mat1->M[1][2] * mat2->M[2][0])) >> 15;
    result->M[1][1] = ((mat1->M[1][0] * mat2->M[0][1]) + (mat1->M[1][1] * mat2->M[1][1]) + (mat1->M[1][2] * mat2->M[2][1])) >> 15;
    result->M[1][2] = ((mat1->M[1][0] * mat2->M[0][2]) + (mat1->M[1][1] * mat2->M[1][2]) + (mat1->M[1][2] * mat2->M[2][2])) >> 15;
    result->M[2][0] = ((mat1->M[2][0] * mat2->M[0][0]) + (mat1->M[2][1] * mat2->M[1][0]) + (mat1->M[2][2] * mat2->M[2][0])) >> 15;
    result->M[2][1] = ((mat1->M[2][0] * mat2->M[0][1]) + (mat1->M[2][1] * mat2->M[1][1]) + (mat1->M[2][2] * mat2->M[2][1])) >> 15;
    result->M[2][2] = ((mat1->M[2][0] * mat2->M[0][2]) + (mat1->M[2][1] * mat2->M[1][2]) + (mat1->M[2][2] * mat2->M[2][2])) >> 15;
}

// uc_orig: rotate_obj (fallen/Source/maths.cpp)
void rotate_obj(SWORD xangle, SWORD yangle, SWORD zangle, Matrix33* r3)
{
    SLONG sinx, cosx, siny, cosy, sinz, cosz;
    SLONG cxcz, sysz, sxsycz, sxsysz, sysx, cxczsy, sxsz, cxsysz, czsx, cxsy, sycz, cxsz;

    sinx = SIN(xangle & (2048 - 1)) >> 1;
    cosx = COS(xangle & (2048 - 1)) >> 1;
    siny = SIN(yangle & (2048 - 1)) >> 1;
    cosy = COS(yangle & (2048 - 1)) >> 1;
    sinz = SIN(zangle & (2048 - 1)) >> 1;
    cosz = COS(zangle & (2048 - 1)) >> 1;

    cxsy = (cosx * cosy);
    sycz = (cosy * cosz);
    cxcz = (cosx * cosz);
    cxsz = (cosx * sinz);
    czsx = (cosz * sinx);
    sysx = (cosy * sinx);
    sysz = (cosy * sinz);
    sxsz = (sinx * sinz);
    sxsysz = (sxsz >> 15) * siny;
    cxczsy = (cxcz >> 15) * siny;
    cxsysz = ((cosx * siny) >> 15) * sinz;
    sxsycz = (czsx >> 15) * siny;

    r3->M[0][0] = (sycz) >> 15;
    r3->M[0][1] = (-sysz) >> 15;
    r3->M[0][2] = siny;
    r3->M[1][0] = (sxsycz + cxsz) >> 15;
    r3->M[1][1] = (cxcz - sxsysz) >> 15;
    r3->M[1][2] = (-sysx) >> 15;
    r3->M[2][0] = (sxsz - cxczsy) >> 15;
    r3->M[2][1] = (cxsysz + czsx) >> 15;
    r3->M[2][2] = (cxsy) >> 15;
}
