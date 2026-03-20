#ifndef CORE_MATH_H
#define CORE_MATH_H

#include "core/types.h"
#include "core/math_globals.h"
#include <cmath>
#include <cstdlib>

// PSX-era angle system: 2048 steps = 360 degrees.
// 512 = 90 deg, 1024 = 180 deg, 1536 = 270 deg.
// All trig lookup tables use this convention.

// Integer sine/cosine via PSX angle (0-2047).
// Values are 16-bit fixed-point: 65536 = 1.0.
// Usage: result = (SIN(angle) * magnitude) >> 16

// uc_orig: SIN (MFStdLib/Headers/StdMaths.h)
#define SIN(a) SinTable[a]

// uc_orig: COS (MFStdLib/Headers/StdMaths.h)
#define COS(a) CosTable[a]

// Float sine/cosine via PSX angle.
// Only covers 0-511 (0-90 degrees). Out-of-range is undefined.

// uc_orig: SIN_F (MFStdLib/Headers/StdMaths.h)
#define SIN_F(a) SinTableF[a]

// uc_orig: COS_F (MFStdLib/Headers/StdMaths.h)
#define COS_F(a) CosTableF[a]

// Perspective correction table size and accessor.

// uc_orig: PROPTABLE_SIZE (MFStdLib/Headers/StdMaths.h)
#define PROPTABLE_SIZE 256

// uc_orig: PROP (MFStdLib/Headers/StdMaths.h)
#define PROP(x) Proportions[(x) + PROPTABLE_SIZE]

// Arctangent in PSX angle units (0-2047). 8-quadrant decomposition
// using AtanTable for the 0-90 degree range.

// uc_orig: Arctan (MFStdLib/Headers/StdMaths.h)
SLONG Arctan(SLONG X, SLONG Y);

// Integer square root wrapper around std::sqrt.

// uc_orig: Root (MFStdLib/Headers/StdMaths.h)
SLONG Root(SLONG square);

// Fast approximate hypotenuse using perspective correction table.
// Avoids sqrt by using the PROP lookup for angle-based scaling.

// uc_orig: Hypotenuse (MFStdLib/Headers/StdMaths.h)
static inline SLONG Hypotenuse(SLONG x, SLONG y)
{
    x = abs(x);
    y = abs(y);
    if (x > y)
        return ((PROP((y << 8) / x) * x) >> 13);
    else if (y)
        return ((PROP((x << 8) / y) * y) >> 13);
    else
        return (0);
}

// 2D segment intersection test in XZ plane.
// Returns TRUE if segment V (vx1,vz1)→(vx2,vz2) intersects segment W (wx1,wz1)→(wx2,wz2).
// Touching endpoints count as an intersection.
// Uses bounding box pre-check then parametric cross-product test (no division).
// uc_orig: MATHS_seg_intersect (fallen/Headers/maths.h)
SLONG MATHS_seg_intersect(
    SLONG vx1, SLONG vz1, SLONG vx2, SLONG vz2,
    SLONG wx1, SLONG wz1, SLONG wx2, SLONG wz2);

#endif // CORE_MATH_H
