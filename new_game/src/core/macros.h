#ifndef CORE_MACROS_H
#define CORE_MACROS_H

#include "core/types.h"

// General-purpose utility macros.

// uc_orig: sgn (MFStdLib/Headers/MFStdLib.h)
#define sgn(a) (((a) < 0) ? -1 : 1)

// uc_orig: swap (MFStdLib/Headers/MFStdLib.h)
#define swap(a, b) \
    {              \
        a ^= b;    \
        b ^= a;    \
        a ^= b;    \
    }

// Clamp 'a' to [min, max] range.
// uc_orig: in_range (MFStdLib/Headers/MFStdLib.h)
#define in_range(a, min, max) \
    {                         \
        if (a > (max))        \
            a = (max);        \
        else if (a < (min))   \
            a = (min);        \
    }

// uc_orig: min (MFStdLib/Headers/MFStdLib.h)
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// uc_orig: max (MFStdLib/Headers/MFStdLib.h)
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

// uc_orig: INFINITY (MFStdLib/Headers/MFStdLib.h)
#define UC_INFINITY 0x7fffffff

// uc_orig: PI (MFStdLib/Headers/MFStdLib.h)
#define PI (3.14159265F)

// True if x is within [a, b] inclusive.
// uc_orig: WITHIN (MFStdLib/Headers/MFStdLib.h)
#define WITHIN(x, a, b) ((x) >= (a) && (x) <= (b))

// Clamp x to [a, b] range.
// uc_orig: SATURATE (MFStdLib/Headers/MFStdLib.h)
#define SATURATE(x, a, b)       \
    {                           \
        if ((x) < (a)) {        \
            (x) = (a);          \
        } else if ((x) > (b)) { \
            (x) = (b);          \
        }                       \
    }

// uc_orig: SWAP (MFStdLib/Headers/MFStdLib.h)
#define SWAP(a, b)  \
    {               \
        SLONG temp; \
        temp = (a); \
        (a) = (b);  \
        (b) = temp; \
    }

// uc_orig: SWAP_FL (MFStdLib/Headers/MFStdLib.h)
#define SWAP_FL(a, b) \
    {                 \
        float temp;   \
        temp = (a);   \
        (a) = (b);    \
        (b) = temp;   \
    }

// uc_orig: MIN (MFStdLib/Headers/MFStdLib.h)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

// uc_orig: MAX (MFStdLib/Headers/MFStdLib.h)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Returns -1, 0, or +1.
// uc_orig: SIGN (MFStdLib/Headers/MFStdLib.h)
#define SIGN(x) (((x)) ? (((x) > 0) ? +1 : -1) : 0)

// Quick approximate 3D distance (no sqrt, uses max + quarter of others).
// uc_orig: QDIST3 (MFStdLib/Headers/MFStdLib.h)
#define QDIST3(x, y, z) ((x) > (y) ? ((x) > (z) ? (x) + ((y) >> 2) + ((z) >> 2) : (z) + ((x) >> 2) + ((y) >> 2)) : ((y) > (z) ? ((y) + ((x) >> 2) + ((z) >> 2)) : (z) + ((x) >> 2) + ((y) >> 2)))

// Quick approximate 2D distance (no sqrt).
// uc_orig: QDIST2 (MFStdLib/Headers/MFStdLib.h)
#define QDIST2(x, y) ((x) > (y) ? ((x) + ((y) >> 1)) : ((y) + ((x) >> 1)))

// Squared 3D distance (for comparisons without sqrt).
// uc_orig: SDIST3 (MFStdLib/Headers/MFStdLib.h)
#define SDIST3(x, y, z) (((x) * (x)) + ((y) * (y)) + ((z) * (z)))

// Squared 2D distance.
// uc_orig: SDIST2 (MFStdLib/Headers/MFStdLib.h)
#define SDIST2(x, y) (((x) * (x)) + ((y) * (y)))

#endif // CORE_MACROS_H
