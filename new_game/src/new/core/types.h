#ifndef CORE_TYPES_H
#define CORE_TYPES_H

// Fundamental integer types used throughout the codebase.

// uc_orig: UBYTE (MFStdLib/Headers/MFStdLib.h)
typedef unsigned char UBYTE;

// uc_orig: SBYTE (MFStdLib/Headers/MFStdLib.h)
typedef signed char SBYTE;

// uc_orig: CBYTE (MFStdLib/Headers/MFStdLib.h)
typedef char CBYTE;

// uc_orig: UWORD (MFStdLib/Headers/MFStdLib.h)
typedef unsigned short UWORD;

// uc_orig: SWORD (MFStdLib/Headers/MFStdLib.h)
typedef signed short SWORD;

// uc_orig: ULONG (MFStdLib/Headers/MFStdLib.h)
typedef unsigned long ULONG;

// uc_orig: SLONG (MFStdLib/Headers/MFStdLib.h)
typedef signed long SLONG;

// uc_orig: TRUE (MFStdLib/Headers/MFStdLib.h)
#ifndef TRUE
#define TRUE 1
#endif

// uc_orig: FALSE (MFStdLib/Headers/MFStdLib.h)
#ifndef FALSE
#define FALSE 0
#endif

// 2D point with integer coordinates.
// uc_orig: MFPoint (MFStdLib/Headers/MFStdLib.h)
typedef struct
{
    SLONG X,
        Y;
} MFPoint;

// Axis-aligned rectangle with cached width/height.
// uc_orig: MFRect (MFStdLib/Headers/MFStdLib.h)
typedef struct
{
    SLONG Left,
        Top,
        Right,
        Bottom,
        Width,
        Height;
} MFRect;

#endif // CORE_TYPES_H
