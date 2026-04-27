#ifndef OUTRO_CORE_OUTRO_ALWAYS_H
#define OUTRO_CORE_OUTRO_ALWAYS_H

// Base type definitions and utility macros for the outro/credits sequence.
// The outro is a self-contained mini-engine that ships its own type system
// rather than sharing the main game's MFStdLib types.

#pragma warning(disable : 4200) // no warning for flexible array members

#include <cstdint>

// uc_orig: SLONG (fallen/outro/always.h)
typedef int32_t SLONG;
// uc_orig: ULONG (fallen/outro/always.h)
typedef uint32_t ULONG;
// uc_orig: SWORD (fallen/outro/always.h)
typedef signed short int SWORD;
// uc_orig: UWORD (fallen/outro/always.h)
typedef unsigned short int UWORD;
// uc_orig: SBYTE (fallen/outro/always.h)
typedef signed char SBYTE;
// uc_orig: UBYTE (fallen/outro/always.h)
typedef unsigned char UBYTE;
// uc_orig: CBYTE (fallen/outro/always.h)
typedef char CBYTE;

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// uc_orig: PI (fallen/outro/always.h)
#define PI (3.14159265F)
// uc_orig: UC_INFINITY (fallen/outro/always.h)
#define UC_INFINITY (0x7fffffff)

// uc_orig: MIN (fallen/outro/always.h)
#define MIN(mnx, mny) (((mnx) < (mny)) ? (mnx) : (mny))
// uc_orig: MAX (fallen/outro/always.h)
#define MAX(mnx, mny) (((mnx) > (mny)) ? (mnx) : (mny))
// uc_orig: MAX3 (fallen/outro/always.h)
#define MAX3(mnx, mny, mnz) (((mnx) > (mny)) ? MAX(mnx, mnz) : MAX(mny, mnz))
// uc_orig: MIN3 (fallen/outro/always.h)
#define MIN3(mnx, mny, mnz) (((mnx) < (mny)) ? MIN(mnx, mnz) : MIN(mny, mnz))
// uc_orig: MIN4 (fallen/outro/always.h)
#define MIN4(a, b, c, d) (MIN(MIN(a, b), MIN(c, d)))
// uc_orig: MAX4 (fallen/outro/always.h)
#define MAX4(a, b, c, d) (MAX(MAX(a, b), MAX(c, d)))

// uc_orig: SWAP (fallen/outro/always.h)
#define SWAP(x, y)        \
    {                     \
        SLONG SWAP_spare; \
        SWAP_spare = x;   \
        x = y;            \
        y = SWAP_spare;   \
    }
// uc_orig: SWAP_UB (fallen/outro/always.h)
#define SWAP_UB(x, y)     \
    {                     \
        UBYTE SWAP_spare; \
        SWAP_spare = x;   \
        x = y;            \
        y = SWAP_spare;   \
    }
// uc_orig: SWAP_UW (fallen/outro/always.h)
#define SWAP_UW(x, y)     \
    {                     \
        UWORD SWAP_spare; \
        SWAP_spare = x;   \
        x = y;            \
        y = SWAP_spare;   \
    }
// uc_orig: SWAP_FL (fallen/outro/always.h)
#define SWAP_FL(x, y)     \
    {                     \
        float SWAP_spare; \
        SWAP_spare = x;   \
        x = y;            \
        y = SWAP_spare;   \
    }
// uc_orig: SWAP_DB (fallen/outro/always.h)
#define SWAP_DB(x, y)      \
    {                      \
        double SWAP_spare; \
        SWAP_spare = x;    \
        x = y;             \
        y = SWAP_spare;    \
    }
// uc_orig: SWAP_HF (fallen/outro/always.h)
#define SWAP_HF(x, y)       \
    {                       \
        Pointhf SWAP_spare; \
        SWAP_spare = x;     \
        x = y;              \
        y = SWAP_spare;     \
    }
// uc_orig: SWAP_P3 (fallen/outro/always.h)
#define SWAP_P3(x, y)       \
    {                       \
        Point3d SWAP_spare; \
        SWAP_spare = x;     \
        x = y;              \
        y = SWAP_spare;     \
    }
// uc_orig: SWAP_VD (fallen/outro/always.h)
#define SWAP_VD(x, y)     \
    {                     \
        void* SWAP_spare; \
        SWAP_spare = x;   \
        x = y;            \
        y = SWAP_spare;   \
    }
// uc_orig: SWAP_UWP (fallen/outro/always.h)
#define SWAP_UWP(x, y)     \
    {                      \
        UWORD* SWAP_spare; \
        SWAP_spare = x;    \
        x = y;             \
        y = SWAP_spare;    \
    }
// uc_orig: SWAP_UBP (fallen/outro/always.h)
#define SWAP_UBP(x, y)     \
    {                      \
        UBYTE* SWAP_spare; \
        SWAP_spare = x;    \
        x = y;             \
        y = SWAP_spare;    \
    }

// uc_orig: WITHIN (fallen/outro/always.h)
#define WITHIN(x, a, b) ((x) >= (a) && (x) <= (b))
// uc_orig: SATURATE (fallen/outro/always.h)
#define SATURATE(x, a, b) \
    {                     \
        if ((x) < (a))    \
            (x) = (a);    \
        if ((x) > (b))    \
            (x) = (b);    \
    }
// uc_orig: SIGN (fallen/outro/always.h)
#define SIGN(x) (((x)) ? (((x) > 0) ? 1 : -1) : 0)
// uc_orig: QLEN (fallen/outro/always.h)
#define QLEN(x, y) (((x) > (y)) ? (x) + ((y) >> 1) : (y) + ((x) >> 1))
// uc_orig: XOR (fallen/outro/always.h)
#define XOR(a, b) ((!(a) && (b)) || ((a) && !(b)))
// uc_orig: SQRT16 (fallen/outro/always.h)
#define SQRT16(x) ((sqrt((double)(x)) * 256))

#ifndef UC_FALSE
// uc_orig: UC_FALSE (fallen/outro/always.h)
#define UC_FALSE 0
#endif
#ifndef UC_TRUE
// uc_orig: UC_TRUE (fallen/outro/always.h)
#define UC_TRUE 1
#endif

// Outro uses the same project-wide ASSERT (logs to crash_log.txt and aborts).
// uc_orig: ASSERT (fallen/outro/always.h) — was __assume(x), now a real check.
void uc_assert_fail(const char* expr, const char* file, int line);
#define ASSERT(x)                                   \
    do {                                            \
        if (!(x))                                   \
            uc_assert_fail(#x, __FILE__, __LINE__); \
    } while (0)
// uc_orig: VERIFY (fallen/outro/always.h)
#define VERIFY(x) x

#pragma warning(disable : 4553) // no warning for x == 1; from VERIFY

// uc_orig: ftol (fallen/outro/always.h)
static inline int ftol(float f)
{
    return (int)f;
}

// uc_orig: Point3d (fallen/outro/always.h)
typedef struct {
    float x, y, z;
} Point3d;

// uc_orig: Point2d (fallen/outro/always.h)
typedef struct {
    float x, y;
} Point2d;

// uc_orig: Pointhf (fallen/outro/always.h)
typedef struct {
    float x, z;
} Pointhf;

// uc_orig: Pointuv (fallen/outro/always.h)
typedef struct {
    float u, v;
} Pointuv;

// uc_orig: Direction (fallen/outro/always.h)
typedef struct {
    float yaw;
    float pitch;
    float roll;
} Direction;

// uc_orig: Colour (fallen/outro/always.h)
typedef struct {
    UBYTE r;
    UBYTE g;
    UBYTE b;
} Colour;

// Fast approximation of sqrt(x*x + y*y); x and y must be non-negative.
// uc_orig: qdist2 (fallen/outro/always.h)
static inline float qdist2(float x, float y)
{
    float ans;
    ASSERT(x >= 0.0F);
    ASSERT(y >= 0.0F);
    if (x > y) {
        ans = x + y * 0.5F;
    } else {
        ans = y + x * 0.5F;
    }
    return ans;
}

// Fast approximation of sqrt(x*x + y*y + z*z); all args must be non-negative.
// uc_orig: qdist3 (fallen/outro/always.h)
static inline float qdist3(float x, float y, float z)
{
    float ans;
    ASSERT(x >= 0.0F);
    ASSERT(y >= 0.0F);
    ASSERT(z >= 0.0F);
    if (x > y) {
        if (x > z) {
            ans = x + (y + z) * 0.2941F;
            return ans;
        }
    } else {
        if (y > z) {
            ans = y + (x + z) * 0.2941F;
            return ans;
        }
    }
    ans = z + (x + y) * 0.2941F;
    return ans;
}

// Returns a random float in [0, 1).
// uc_orig: frand (fallen/outro/always.h)
static inline float frand(void)
{
    SLONG irand = rand();
    float ans = float(irand) * (1.0F / float(RAND_MAX));
    return ans;
}

#endif // OUTRO_CORE_OUTRO_ALWAYS_H
