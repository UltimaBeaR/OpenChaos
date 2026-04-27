#ifndef ENGINE_CORE_MATH_GLOBALS_H
#define ENGINE_CORE_MATH_GLOBALS_H

#include "engine/core/types.h"

// Integer sine table (2560 entries) and cosine pointer (offset +512 into SinTable).

// uc_orig: SinTable (MFStdLib/Headers/StdMaths.h)
extern SLONG SinTable[];

// uc_orig: CosTable (MFStdLib/Headers/StdMaths.h)
extern SLONG* CosTable;

// Arctangent lookup for Arctan() function (256 entries, 0-90 deg).

// uc_orig: AtanTable (MFStdLib/Headers/StdMaths.h)
extern SWORD AtanTable[];

#endif // ENGINE_CORE_MATH_GLOBALS_H
