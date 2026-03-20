#ifndef CORE_VECTOR_H
#define CORE_VECTOR_H

#include "core/types.h"

// 3D integer vector with 32-bit components.
// uc_orig: SVector (fallen/Headers/Structs.h)
struct SVector {
    SLONG X, Y, Z;
};

// 3D integer vector with 16-bit components.
// uc_orig: SmallSVector (fallen/Headers/Structs.h)
struct SmallSVector {
    SWORD X, Y, Z;
};

// Alias for SVector, matches PSX naming convention.
// uc_orig: SVECTOR (fallen/Headers/Structs.h)
struct SVECTOR {
    SLONG X, Y, Z;
};

// 3D world coordinate, same layout as SVector.
// uc_orig: GameCoord (fallen/Headers/Structs.h)
typedef struct
{
    SLONG X, Y, Z;
} GameCoord;

// Compact 2D displacement with facing angle (used for small movements).
// uc_orig: TinyXZ (fallen/Headers/Structs.h)
struct TinyXZ {
    SBYTE Dx, Dz;
    SWORD Angle;
};

#endif // CORE_VECTOR_H
