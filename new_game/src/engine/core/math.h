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

// Arctangent in PSX angle units (0-2047). 8-quadrant decomposition
// using AtanTable for the 0-90 degree range.

// uc_orig: Arctan (MFStdLib/Headers/StdMaths.h)
SLONG Arctan(SLONG X, SLONG Y);

// Integer square root wrapper around std::sqrt.

// uc_orig: Root (MFStdLib/Headers/StdMaths.h)
SLONG Root(SLONG square);

// 2D segment intersection test in XZ plane.
#endif // ENGINE_CORE_MATH_H
