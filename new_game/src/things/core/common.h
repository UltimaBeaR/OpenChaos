#ifndef THINGS_CORE_COMMON_H
#define THINGS_CORE_COMMON_H

#include "engine/core/types.h"

// COMMON(TYPE) — header macro for all secondary-Thing genus structs.
// Declares the required first fields: a type byte, a padding byte,
// the owning Thing index, and a flags word.
// Must be the first field in any struct that is stored in Thing.Genus.
// uc_orig: COMMON (fallen/Headers/Structs.h)
#define COMMON(TYPE)   \
    UBYTE TYPE;        \
    UBYTE padding;     \
    THING_INDEX Thing; \
    ULONG Flags;

#endif // THINGS_CORE_COMMON_H
