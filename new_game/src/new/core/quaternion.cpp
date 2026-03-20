#include "core/quaternion.h"
#include "core/fmatrix.h"
#include "core/math.h"
#include <math.h>
#include <windows.h>

// Temporary: Matrix33, CMatrix33, CMAT masks defined in prim.h (not yet migrated).
#include "fallen/Headers/prim.h"

// Forward declaration of integer SLERP path used by CQuaternion::BuildTween.
// uc_orig: QUATERNION_BuildTweenInteger (fallen/DDEngine/Source/Quaternion.cpp)
void QUATERNION_BuildTweenInteger(struct Matrix33* dest, struct CMatrix33* cm1, struct CMatrix33* cm2, SLONG tween);

// Threshold for switching from SLERP to linear interpolation (quaternions nearly identical).
// uc_orig: DELTA (fallen/DDEngine/Source/Quaternion.cpp)
#define DELTA 0.05

// Spherical linear interpolation between two quaternions using the shortest arc.
// When quaternions are very close (dot product near 1), falls back to lerp.
// uc_orig: QuatSlerp (fallen/DDEngine/Source/Quaternion.cpp)
void QuatSlerp(CQuaternion* from, CQuaternion* to, float t, CQuaternion* res)
{
    float to1[4];
    double omega, cosom, sinom, scale0, scale1;

    // Compute dot product to find cosine of angle between quaternions.
    cosom = from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w;

    // Negate one quaternion if the dot is negative so we take the short arc.
    if (cosom < 0.0) {
        cosom = -cosom;

        to1[0] = -to->x;
        to1[1] = -to->y;
        to1[2] = -to->z;
        to1[3] = -to->w;
    } else {
        to1[0] = to->x;
        to1[1] = to->y;
        to1[2] = to->z;
        to1[3] = to->w;
    }

    if ((1.0 - cosom) > DELTA) {
        // Standard SLERP.
        omega = acos(cosom);
        sinom = sin(omega);
        scale0 = sin((1.0 - t) * omega) / sinom;
        scale1 = sin(t * omega) / sinom;

    } else {
        // Quaternions very close — use linear interpolation.
        scale0 = 1.0 - t;
        scale1 = t;
    }

    res->x = scale0 * from->x + scale1 * to1[0];
    res->y = scale0 * from->y + scale1 * to1[1];
    res->z = scale0 * from->z + scale1 * to1[2];
    res->w = scale0 * from->w + scale1 * to1[3];
}

// Multiply two quaternions: res = q1 * q2.
// uc_orig: QuatMul (fallen/DDEngine/Source/Quaternion.cpp)
void QuatMul(CQuaternion* q1, CQuaternion* q2, CQuaternion* res)
{
    float A, B, C, D, E, F, G, H;

    A = (q1->w + q1->x) * (q2->w + q2->x);
    B = (q1->z - q1->y) * (q2->y - q2->z);
    C = (q1->x - q1->w) * (q2->y - q2->z);
    D = (q1->y + q1->z) * (q2->x - q2->w);
    E = (q1->x + q1->z) * (q2->x + q2->y);
    F = (q1->x - q1->z) * (q2->x - q2->y);
    G = (q1->w + q1->y) * (q2->w - q2->z);
    H = (q1->w - q1->y) * (q2->w + q2->z);

    res->w = B + (-E - F + G + H) / 2;
    res->x = A - (E + F + G + H) / 2;
    res->y = -C + (E - F + G - H) / 2;
    res->z = -D + (E - F - G + H) / 2;
}

// Convert Euler angles (roll, pitch, yaw) in radians to a unit quaternion.
// uc_orig: EulerToQuat (fallen/DDEngine/Source/Quaternion.cpp)
void EulerToQuat(float roll, float pitch, float yaw, CQuaternion* quat)
{
    float cr, cp, cy, sr, sp, sy, cpcy, spsy;

    cr = cos(roll / 2);
    cp = cos(pitch / 2);
    cy = cos(yaw / 2);

    sr = sin(roll / 2);
    sp = sin(pitch / 2);
    sy = sin(yaw / 2);

    cpcy = cp * cy;
    spsy = sp * sy;

    quat->w = cr * cpcy + sr * spsy;
    quat->x = sr * cpcy - cr * spsy;
    quat->y = cr * sp * cy + sr * cp * sy;
    quat->z = cr * cp * sy - sr * sp * cy;
}

// Convert quaternion to floating-point 3x3 rotation matrix.
// uc_orig: QuatToMatrix (fallen/DDEngine/Source/Quaternion.cpp)
void QuatToMatrix(CQuaternion* quat, FloatMatrix* fm)
{
    float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

    x2 = quat->x + quat->x;
    y2 = quat->y + quat->y;
    z2 = quat->z + quat->z;
    xx = quat->x * x2;
    xy = quat->x * y2;
    xz = quat->x * z2;
    yy = quat->y * y2;
    yz = quat->y * z2;
    zz = quat->z * z2;
    wx = quat->w * x2;
    wy = quat->w * y2;
    wz = quat->w * z2;

    fm->M[0][0] = 1.0 - (yy + zz);
    fm->M[1][0] = xy - wz;
    fm->M[2][0] = xz + wy;

    fm->M[0][1] = xy + wz;
    fm->M[1][1] = 1.0 - (xx + zz);
    fm->M[2][1] = yz - wx;

    fm->M[0][2] = xz - wy;
    fm->M[1][2] = yz + wx;
    fm->M[2][2] = 1.0 - (xx + yy);
}

// Convert floating-point 3x3 matrix to quaternion using the trace method.
// Handles the degenerate case (negative diagonal) by finding the largest diagonal element.
// uc_orig: MatrixToQuat (fallen/DDEngine/Source/Quaternion.cpp)
void MatrixToQuat(FloatMatrix* fm, CQuaternion* quat)
{
    float tr, s, q[4];
    int i, j, k;

    int nxt[3] = { 1, 2, 0 };

    tr = fm->M[0][0] + fm->M[1][1] + fm->M[2][2];

    if (tr > 0.0) {
        s = sqrt(tr + 1.0);
        quat->w = s / 2.0;
        s = 0.5 / s;
        quat->x = (fm->M[1][2] - fm->M[2][1]) * s;
        quat->y = (fm->M[2][0] - fm->M[0][2]) * s;
        quat->z = (fm->M[0][1] - fm->M[1][0]) * s;
    } else {
        // Diagonal is negative — find largest diagonal element.
        i = 0;
        if (fm->M[1][1] > fm->M[0][0])
            i = 1;
        if (fm->M[2][2] > fm->M[i][i])
            i = 2;
        j = nxt[i];
        k = nxt[j];

        s = sqrt((fm->M[i][i] - (fm->M[j][j] + fm->M[k][k])) + 1.0);

        q[i] = s * 0.5;

        if (s != 0.0)
            s = 0.5 / s;

        q[3] = (fm->M[j][k] - fm->M[k][j]) * s;
        q[j] = (fm->M[i][j] + fm->M[j][i]) * s;
        q[k] = (fm->M[i][k] + fm->M[k][i]) * s;

        quat->x = q[0];
        quat->y = q[1];
        quat->z = q[2];
        quat->w = q[3];
    }
}

// Convert a compressed integer matrix (CMatrix33) to a floating-point matrix.
// Each CMatrix33 row packs three 10-bit signed values in a single SLONG.
// Values are in range [-1, 1] encoded as 512 = 1.0; values >= 1.0 are sign-extended by subtracting 2.
// uc_orig: cmat_to_fmat (fallen/DDEngine/Source/Quaternion.cpp)
void cmat_to_fmat(CMatrix33* cm, FloatMatrix* fm)
{
    fm->M[0][0] = float(((cm->M[0] & CMAT0_MASK) >> 20)) / 512.f;
    fm->M[0][1] = float(((cm->M[0] & CMAT1_MASK) >> 10)) / 512.f;
    fm->M[0][2] = float(((cm->M[0] & CMAT2_MASK) >> 00)) / 512.f;

    fm->M[1][0] = float(((cm->M[1] & CMAT0_MASK) >> 20)) / 512.f;
    fm->M[1][1] = float(((cm->M[1] & CMAT1_MASK) >> 10)) / 512.f;
    fm->M[1][2] = float(((cm->M[1] & CMAT2_MASK) >> 00)) / 512.f;

    fm->M[2][0] = float(((cm->M[2] & CMAT0_MASK) >> 20)) / 512.f;
    fm->M[2][1] = float(((cm->M[2] & CMAT1_MASK) >> 10)) / 512.f;
    fm->M[2][2] = float(((cm->M[2] & CMAT2_MASK) >> 00)) / 512.f;

    // Values >= 1.0 are actually negative (10-bit unsigned used as signed).
    if (fm->M[0][0] >= 1.f)
        fm->M[0][0] -= 2.f;
    if (fm->M[0][1] >= 1.f)
        fm->M[0][1] -= 2.f;
    if (fm->M[0][2] >= 1.f)
        fm->M[0][2] -= 2.f;
    if (fm->M[1][0] >= 1.f)
        fm->M[1][0] -= 2.f;
    if (fm->M[1][1] >= 1.f)
        fm->M[1][1] -= 2.f;
    if (fm->M[1][2] >= 1.f)
        fm->M[1][2] -= 2.f;
    if (fm->M[2][0] >= 1.f)
        fm->M[2][0] -= 2.f;
    if (fm->M[2][1] >= 1.f)
        fm->M[2][1] -= 2.f;
    if (fm->M[2][2] >= 1.f)
        fm->M[2][2] -= 2.f;
}

// Convert floating-point matrix to integer Matrix33 (fixed-point 1.0 = 32768).
// uc_orig: fmat_to_mat (fallen/DDEngine/Source/Quaternion.cpp)
void fmat_to_mat(FloatMatrix* fm, Matrix33* m)
{
    m->M[0][0] = SLONG(fm->M[0][0] * 32768.f);
    m->M[0][1] = SLONG(fm->M[0][1] * 32768.f);
    m->M[0][2] = SLONG(fm->M[0][2] * 32768.f);

    m->M[1][0] = SLONG(fm->M[1][0] * 32768.f);
    m->M[1][1] = SLONG(fm->M[1][1] * 32768.f);
    m->M[1][2] = SLONG(fm->M[1][2] * 32768.f);

    m->M[2][0] = SLONG(fm->M[2][0] * 32768.f);
    m->M[2][1] = SLONG(fm->M[2][1] * 32768.f);
    m->M[2][2] = SLONG(fm->M[2][2] * 32768.f);
}

// Check whether three components form a unit-length vector (sum of squares near 1).
// uc_orig: is_unit (fallen/DDEngine/Source/Quaternion.cpp)
BOOL is_unit(float a, float b, float c)
{
    return (fabs(1.0 - (a * a + b * b + c * c)) < 0.01);
}

// Check whether the matrix rows form a right-handed orthonormal basis.
// Returns TRUE if the cross product of rows 0 and 1 matches row 2 within tolerance.
// A zero first row is treated as a void (identity) matrix and returns TRUE.
// uc_orig: check_isonormal (fallen/DDEngine/Source/Quaternion.cpp)
BOOL check_isonormal(FloatMatrix& m)
{
    BOOL r = TRUE;

    if ((m.M[0][0] == 0) && (m.M[0][1] == 0) && (m.M[0][2] == 0))
        return TRUE;

    float x = m.M[0][1] * m.M[1][2] - m.M[0][2] * m.M[1][1];
    float y = m.M[0][2] * m.M[1][0] - m.M[0][0] * m.M[1][2];
    float z = m.M[0][0] * m.M[1][1] - m.M[0][1] * m.M[1][0];

    if ((fabs(x - m.M[2][0]) > 0.03) || (fabs(y - m.M[2][1]) > 0.03) || (fabs(z - m.M[2][2]) > 0.03))
        return FALSE;

    return TRUE;
}

// Quaternion SLERP via floating-point path.
// Converts compressed matrices to float, checks orthonormality, performs SLERP,
// then converts back to integer matrix. Falls back to linear tween if handedness differs.
// uc_orig: CQuaternion::BuildTween (fallen/DDEngine/Source/Quaternion.cpp)
void CQuaternion::BuildTween(struct Matrix33* dest, struct CMatrix33* cm1, struct CMatrix33* cm2, SLONG tween)
{
    FloatMatrix f1, f2, f3;

    cmat_to_fmat(cm1, &f1);
    cmat_to_fmat(cm2, &f2);

    BOOL a, b;
    a = check_isonormal(f1);
    b = check_isonormal(f2);

    // If both matrices have the same handedness, use quaternion SLERP.
    if (a == b) {
        float t = float(tween) / 256.f;

        CQuaternion q1, q2, q3;

        MatrixToQuat(&f1, &q1);
        MatrixToQuat(&f2, &q2);

        QuatSlerp(&q1, &q2, t, &q3);
        QuatToMatrix(&q3, &f3);

        fmat_to_mat(&f3, dest);

        return;
    }

    // Fallback for mismatched handedness — use linear matrix tween then renormalize.
    build_tween_matrix(dest, cm1, cm2, tween);
    normalise_matrix_rows(dest);
}

// Integer quaternion (fixed-point, 32768 = 1.0).
// uc_orig: QuatInt (fallen/DDEngine/Source/Quaternion.cpp)
struct QuatInt {
    SLONG x, y, z, w;
};

// Convert integer Matrix33 (32768 = 1.0) to integer quaternion using trace method.
// uc_orig: MatrixToQuatInteger (fallen/DDEngine/Source/Quaternion.cpp)
void MatrixToQuatInteger(Matrix33* m, QuatInt* quat)
{
    SLONG tr, s;
    SLONG q[4];
    SLONG i, j, k;

    SLONG nxt[3] = { 1, 2, 0 };

    tr = m->M[0][0] + m->M[1][1] + m->M[2][2];

    if (tr > 0) {
        s = Root(tr + (1 << 15)) * 181;
        quat->w = s / 2;
        s = ((1 << (14 + 12)) / s) << 3;
        quat->x = ((m->M[1][2] - m->M[2][1]) * s) >> 15;
        quat->y = ((m->M[2][0] - m->M[0][2]) * s) >> 15;
        quat->z = ((m->M[0][1] - m->M[1][0]) * s) >> 15;
    } else {
        i = 0;
        if (m->M[1][1] > m->M[0][0])
            i = 1;
        if (m->M[2][2] > m->M[i][i])
            i = 2;
        j = nxt[i];
        k = nxt[j];

        s = Root((m->M[i][i] - (m->M[j][j] + m->M[k][k])) + (1 << 15)) * 181;

        q[i] = s / 2;

        if (s != 0)
            s = ((1 << (14 + 12)) / s) << 3;

        q[3] = ((m->M[j][k] - m->M[k][j]) * s) >> 15;
        q[j] = ((m->M[i][j] + m->M[j][i]) * s) >> 15;
        q[k] = ((m->M[i][k] + m->M[k][i]) * s) >> 15;

        quat->x = q[0];
        quat->y = q[1];
        quat->z = q[2];
        quat->w = q[3];
    }
}

// Convert integer quaternion to integer Matrix33.
// uc_orig: QuatToMatrixInteger (fallen/DDEngine/Source/Quaternion.cpp)
void QuatToMatrixInteger(QuatInt* quat, Matrix33* m)
{
    SLONG wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

    x2 = quat->x + quat->x;
    y2 = quat->y + quat->y;
    z2 = quat->z + quat->z;
    xx = (quat->x * x2) >> 15;
    xy = (quat->x * y2) >> 15;
    xz = (quat->x * z2) >> 15;
    yy = (quat->y * y2) >> 15;
    yz = (quat->y * z2) >> 15;
    zz = (quat->z * z2) >> 15;
    wx = (quat->w * x2) >> 15;
    wy = (quat->w * y2) >> 15;
    wz = (quat->w * z2) >> 15;

    m->M[0][0] = (1 << 15) - (yy + zz);
    m->M[1][0] = xy - wz;
    m->M[2][0] = xz + wy;

    m->M[0][1] = xy + wz;
    m->M[1][1] = (1 << 15) - (xx + zz);
    m->M[2][1] = yz - wx;

    m->M[0][2] = xz - wy;
    m->M[1][2] = yz + wx;
    m->M[2][2] = (1 << 15) - (xx + yy);
}

// Lookup table for acos in PSX angle units (0-2047), covering [0, 1] in 1025 steps.
// Only half the table needed because input to QuatSlerpInteger is always >= 0.
// uc_orig: acos_table (fallen/DDEngine/Source/Quaternion.cpp)
SWORD acos_table[1025];

// Flag set once BuildACosTable has been called.
// uc_orig: acos_table_init (fallen/DDEngine/Source/Quaternion.cpp)
BOOL acos_table_init = FALSE;

// Populate acos_table with PSX angle values corresponding to acos(i/1025) for i = 0..1024.
// uc_orig: BuildACosTable (fallen/DDEngine/Source/Quaternion.cpp)
void BuildACosTable()
{
    SLONG c0;

    for (c0 = 0; c0 < 1025; c0++) {
        acos_table[c0] = SLONG(acos(float(c0) / 1025.f) / (2 * 3.1415926) * 2047);
    }

    acos_table_init = TRUE;
}

// Threshold for switching from integer SLERP to linear interpolation (32768 = 1.0).
// uc_orig: DELTA_INT (fallen/DDEngine/Source/Quaternion.cpp)
#define DELTA_INT 1638

// Spherical linear interpolation between two integer quaternions.
// tween is 0-256 where 0 = from and 1 = to.
// Uses SIN lookup table and integer acos table for fixed-point trigonometry.
// uc_orig: QuatSlerpInteger (fallen/DDEngine/Source/Quaternion.cpp)
void QuatSlerpInteger(QuatInt* from, QuatInt* to, SLONG tween, QuatInt* res)
{
    SLONG to1[4];
    SLONG omega, cosom, sinom;
    SLONG scale0, scale1;

    if (!acos_table_init)
        BuildACosTable();

    // Dot product in fixed-point (32768 = 1.0).
    cosom = (from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w) >> 15;

    // Negate one quaternion if dot is negative so we take the short arc.
    if (cosom < 0) {
        cosom = -cosom;

        to1[0] = -to->x;
        to1[1] = -to->y;
        to1[2] = -to->z;
        to1[3] = -to->w;
    } else {
        to1[0] = to->x;
        to1[1] = to->y;
        to1[2] = to->z;
        to1[3] = to->w;
    }

    if (((1 << 15) - cosom) > DELTA_INT) {
        // Standard integer SLERP using precomputed acos table and SIN lookup.
        omega = acos_table[cosom / 32];

        sinom = SIN(omega);
        scale0 = (SIN(((256 - tween) * omega) / 256) * 256) / sinom;
        scale1 = (SIN(((tween)*omega) / 256) * 256) / sinom;

    } else {
        // Quaternions very close — use linear interpolation.
        scale0 = 256 - tween;
        scale1 = tween;
    }

    res->x = (scale0 * from->x + scale1 * to1[0]) >> 8;
    res->y = (scale0 * from->y + scale1 * to1[1]) >> 8;
    res->z = (scale0 * from->z + scale1 * to1[2]) >> 8;
    res->w = (scale0 * from->w + scale1 * to1[3]) >> 8;
}

// Convert compressed integer matrix (CMatrix33) to integer Matrix33 (32768 = 1.0).
// Values >= 32768 are sign-extended by subtracting 65536.
// uc_orig: cmat_to_mat (fallen/DDEngine/Source/Quaternion.cpp)
void cmat_to_mat(CMatrix33* cm, Matrix33* m)
{
    m->M[0][0] = ((cm->M[0] & CMAT0_MASK) >> 20) << 6;
    m->M[0][1] = ((cm->M[0] & CMAT1_MASK) >> 10) << 6;
    m->M[0][2] = ((cm->M[0] & CMAT2_MASK) >> 00) << 6;

    m->M[1][0] = ((cm->M[1] & CMAT0_MASK) >> 20) << 6;
    m->M[1][1] = ((cm->M[1] & CMAT1_MASK) >> 10) << 6;
    m->M[1][2] = ((cm->M[1] & CMAT2_MASK) >> 00) << 6;

    m->M[2][0] = ((cm->M[2] & CMAT0_MASK) >> 20) << 6;
    m->M[2][1] = ((cm->M[2] & CMAT1_MASK) >> 10) << 6;
    m->M[2][2] = ((cm->M[2] & CMAT2_MASK) >> 00) << 6;

    // Values >= 32768 are actually negative (sign extension for 10-bit values).
    if (m->M[0][0] >= 32768)
        m->M[0][0] -= 65536;
    if (m->M[0][1] >= 32768)
        m->M[0][1] -= 65536;
    if (m->M[0][2] >= 32768)
        m->M[0][2] -= 65536;
    if (m->M[1][0] >= 32768)
        m->M[1][0] -= 65536;
    if (m->M[1][1] >= 32768)
        m->M[1][1] -= 65536;
    if (m->M[1][2] >= 32768)
        m->M[1][2] -= 65536;
    if (m->M[2][0] >= 32768)
        m->M[2][0] -= 65536;
    if (m->M[2][1] >= 32768)
        m->M[2][1] -= 65536;
    if (m->M[2][2] >= 32768)
        m->M[2][2] -= 65536;
}

// Check whether integer matrix rows form a right-handed orthonormal basis.
// Returns FALSE if cross product of rows 0 and 1 deviates from row 2 by more than 1000.
// uc_orig: check_isonormal_integer (fallen/DDEngine/Source/Quaternion.cpp)
BOOL check_isonormal_integer(Matrix33& m)
{
    SLONG x = (m.M[0][1] * m.M[1][2] - m.M[0][2] * m.M[1][1]) >> 15;
    SLONG y = (m.M[0][2] * m.M[1][0] - m.M[0][0] * m.M[1][2]) >> 15;
    SLONG z = (m.M[0][0] * m.M[1][1] - m.M[0][1] * m.M[1][0]) >> 15;

    if ((abs(x - m.M[2][0]) > 1000) || (abs(y - m.M[2][1]) > 1000) || (abs(z - m.M[2][2]) > 1000))
        return FALSE;

    return TRUE;
}

// Integer SLERP path for quaternion matrix interpolation.
// Converts compressed matrices to integer Matrix33, checks handedness,
// flips the third row if needed, performs integer SLERP, then flips back.
// Falls back to linear tween if handedness differs between the two matrices.
// uc_orig: QUATERNION_BuildTweenInteger (fallen/DDEngine/Source/Quaternion.cpp)
void QUATERNION_BuildTweenInteger(struct Matrix33* dest, struct CMatrix33* cm1, struct CMatrix33* cm2, SLONG tween)
{
    Matrix33 m1, m2;

    cmat_to_mat(cm1, &m1);
    cmat_to_mat(cm2, &m2);

    BOOL a, b;
    a = check_isonormal_integer(m1);
    b = check_isonormal_integer(m2);

    if (a == b) {
        if (!a) {
            // Flip the third row to correct handedness before interpolation.
            m1.M[2][0] = -m1.M[2][0];
            m1.M[2][1] = -m1.M[2][1];
            m1.M[2][2] = -m1.M[2][2];
            m2.M[2][0] = -m2.M[2][0];
            m2.M[2][1] = -m2.M[2][1];
            m2.M[2][2] = -m2.M[2][2];
        }

        QuatInt q1, q2, q3;

        MatrixToQuatInteger(&m1, &q1);
        MatrixToQuatInteger(&m2, &q2);

        QuatSlerpInteger(&q1, &q2, tween, &q3);
        QuatToMatrixInteger(&q3, dest);

        if (!a) {
            // Flip the third row back after interpolation.
            dest->M[2][0] = -dest->M[2][0];
            dest->M[2][1] = -dest->M[2][1];
            dest->M[2][2] = -dest->M[2][2];
        }

        return;
    }

    // Fallback for mismatched handedness.
    build_tween_matrix(dest, cm1, cm2, tween);
    normalise_matrix_rows(dest);
}
