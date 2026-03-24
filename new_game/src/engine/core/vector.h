#ifndef ENGINE_CORE_VECTOR_H
#define ENGINE_CORE_VECTOR_H

#include "engine/core/types.h"

// 3D integer vector with 32-bit components.
// uc_orig: SVector (fallen/Headers/Structs.h)
struct SVector {
    SLONG X, Y, Z;
};

// 3D world coordinate, same layout as SVector.
// uc_orig: GameCoord (fallen/Headers/Structs.h)
typedef struct
{
    SLONG X, Y, Z;
} GameCoord;

#endif // ENGINE_CORE_VECTOR_H
