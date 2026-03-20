#include "core/math.h"

// uc_orig: LookUp (MFStdLib/Source/StdLib/StdMaths.cpp)
// Arctangent index: computes (minor/major)*256 as lookup into AtanTable.
#define LookUp ax = AtanTable[(ax << 8) / bx];

// uc_orig: xchg (MFStdLib/Source/StdLib/StdMaths.cpp)
// XOR swap without temporary.
#define xchg(a, b) \
    {              \
        a ^= b;    \
        b ^= a;    \
        a ^= b;    \
    }

// uc_orig: Arctan (MFStdLib/Source/StdLib/StdMaths.cpp)
SLONG Arctan(SLONG X, SLONG Y)
{
    register SLONG ax, bx;

    ax = X;
    if (ax)
        goto just_do_it;
    bx = Y;
    if (bx)
        goto done_it;
    return 0;
just_do_it:
    bx = Y;
done_it:
    if (ax < 0)
        goto xneg;

    // x positive
    if (bx < 0)
        goto xposyneg;

    // x positive, y positive
    if (ax < bx)
        goto ppyprimary;

    // ppxprimary
    xchg(ax, bx)
            LookUp return ax
        + 512;
ppyprimary:
    LookUp return (-ax) + 1024;
xposyneg: //*******************************************************************
    bx = -bx;
    if (ax < bx)
        goto pnyprimary;

    // pnxprimary
    xchg(ax, bx)
            LookUp return (-ax)
        + 512;
pnyprimary:
    LookUp return ax;
xneg:
    ax = -ax;
    if (bx < 0)
        goto xnegyneg;
    // x negative, y positive
    if (ax < bx)
        goto npyprimary;

    // npxprimary
    xchg(ax, bx)
            LookUp return (-ax)
        + 1536;
npyprimary:
    LookUp return ax + 1024;

// x negative, y negative
xnegyneg:
    bx = -bx;
    if (ax < bx)
        goto nnyprimary;

    // nnxprimary
    xchg(ax, bx)
            LookUp return ax
        + 1536;
nnyprimary:
    LookUp return (-ax) + 2048;
}

// uc_orig: Root (MFStdLib/Source/StdLib/StdMaths.cpp)
SLONG Root(SLONG square)
{
    return (int)sqrt(square);
}

// uc_orig: MATHS_seg_intersect (fallen/Source/maths.cpp)
SLONG MATHS_seg_intersect(
    SLONG vx1, SLONG vz1, SLONG vx2, SLONG vz2,
    SLONG wx1, SLONG wz1, SLONG wx2, SLONG wz2)
{
    SLONG ax, az;
    SLONG bx, bz;
    SLONG dx, dz;

    SLONG acrossb;
    SLONG dcrossa;
    SLONG dcrossb;

#define MATH_DIFFERENT_SIGN(c1, c2) (((c1) & 0x80000000) ^ ((c2) & 0x80000000))

    // Bounding box pre-check in X.
    if (vx1 < vx2) {
        if (wx1 < vx1 && wx2 < vx1)
            return FALSE;
        if (wx1 > vx2 && wx2 > vx2)
            return FALSE;
    } else {
        if (wx1 > vx1 && wx2 > vx1)
            return FALSE;
        if (wx1 < vx2 && wx2 < vx2)
            return FALSE;
    }

    // Bounding box pre-check in Z.
    if (vz1 < vz2) {
        if (wz1 < vz1 && wz2 < vz1)
            return FALSE;
        if (wz1 > vz2 && wz2 > vz2)
            return FALSE;
    } else {
        if (wz1 > vz1 && wz2 > vz1)
            return FALSE;
        if (wz1 < vz2 && wz2 < vz2)
            return FALSE;
    }

    // Vectors: a = V, b = v1→w1, d = W.
    ax = vx2 - vx1;
    az = vz2 - vz1;
    bx = wx1 - vx1;
    bz = wz1 - vz1;
    dx = wx2 - wx1;
    dz = wz2 - wz1;

    // t = dcrossb / dcrossa must be in [0,1].
    dcrossa = dx * az - dz * ax;
    dcrossb = dx * bz - dz * bx;

    if (MATH_DIFFERENT_SIGN(dcrossb, dcrossa))
        return FALSE;
    if (abs(dcrossb) > abs(dcrossa))
        return FALSE;

    // q = acrossb / dcrossa must be in [0,1].
    acrossb = ax * bz - az * bx;

    if (MATH_DIFFERENT_SIGN(acrossb, dcrossa))
        return FALSE;
    if (abs(acrossb) > abs(dcrossa))
        return FALSE;

    return TRUE;

#undef MATH_DIFFERENT_SIGN
}
