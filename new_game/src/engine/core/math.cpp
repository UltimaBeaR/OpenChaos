#include "engine/core/math.h"

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
    SLONG ax, bx;

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

