#ifndef ENGINE_CORE_TYPES_H
#define ENGINE_CORE_TYPES_H

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
#define UC_TRUE 1

// uc_orig: FALSE (MFStdLib/Headers/MFStdLib.h)
#define UC_FALSE 0

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

// Windows DWORD type used in platform-specific code (GetTickCount, etc.).
// Equivalent to unsigned long on 32-bit Windows.
#ifndef _WINDEF_
typedef unsigned long DWORD;
#endif

// Index type for Things and other pool objects.
// 16-bit unsigned on PC; defined here so headers can use it without including Game.h.
// uc_orig: THING_INDEX (fallen/Headers/Game.h)
#ifndef THING_INDEX
#define THING_INDEX UWORD
#endif
// uc_orig: COMMON_INDEX (fallen/Headers/Game.h)
#ifndef COMMON_INDEX
#define COMMON_INDEX UWORD
#endif

// World-space coordinate alias at 16-bit fixed-point precision.
// uc_orig: MAPCO16 (fallen/Headers/Game.h)
typedef SLONG MAPCO16;

#endif // ENGINE_CORE_TYPES_H
