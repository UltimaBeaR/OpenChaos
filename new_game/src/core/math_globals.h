#ifndef CORE_MATH_GLOBALS_H
#define CORE_MATH_GLOBALS_H

#include "core/types.h"

// Integer sine table (2560 entries) and cosine pointer (offset +512 into SinTable).

// uc_orig: SinTable (MFStdLib/Headers/StdMaths.h)
extern SLONG SinTable[];

// uc_orig: CosTable (MFStdLib/Headers/StdMaths.h)
extern SLONG* CosTable;

// Float sine table (512 entries, 0-90 deg only) and cosine pointer.

// uc_orig: SinTableF (MFStdLib/Headers/StdMaths.h)
extern float SinTableF[];

// uc_orig: CosTableF (MFStdLib/Headers/StdMaths.h)
extern float* CosTableF;

// Arctangent lookup for Arctan() function (256 entries, 0-90 deg).

// uc_orig: AtanTable (MFStdLib/Headers/StdMaths.h)
extern SWORD AtanTable[];

// Perspective correction lookup (512 entries, indexed via PROP macro).

// uc_orig: Proportions (MFStdLib/Headers/StdMaths.h)
extern SLONG Proportions[];

#endif // CORE_MATH_GLOBALS_H
