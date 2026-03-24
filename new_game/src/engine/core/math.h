#ifndef ENGINE_CORE_MATH_H
#define ENGINE_CORE_MATH_H

#include "engine/core/types.h"
#include "engine/core/math_globals.h"
#include <cmath>

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

// Arctangent in PSX angle units (0-2047). 8-quadrant decomposition
// using AtanTable for the 0-90 degree range.

// uc_orig: Arctan (MFStdLib/Headers/StdMaths.h)
SLONG Arctan(SLONG X, SLONG Y);

// Integer square root wrapper around std::sqrt.

// uc_orig: Root (MFStdLib/Headers/StdMaths.h)
SLONG Root(SLONG square);

// 2D segment intersection test in XZ plane.
// Returns TRUE if segment V (vx1,vz1)→(vx2,vz2) intersects segment W (wx1,wz1)→(wx2,wz2).
// Touching endpoints count as an intersection.
// Uses bounding box pre-check then parametric cross-product test (no division).
// uc_orig: MATHS_seg_intersect (fallen/Headers/maths.h)
SLONG MATHS_seg_intersect(
    SLONG vx1, SLONG vz1, SLONG vx2, SLONG vz2,
    SLONG wx1, SLONG wz1, SLONG wx2, SLONG wz2);

#endif // ENGINE_CORE_MATH_H
