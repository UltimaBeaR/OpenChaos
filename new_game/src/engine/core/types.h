#ifndef ENGINE_CORE_TYPES_H
#define ENGINE_CORE_TYPES_H

#include <cstdint>

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
// Was `unsigned long` — changed to uint32_t for 64-bit portability
// (long is 4 bytes on Windows LLP64 but 8 bytes on Linux/macOS LP64).
typedef uint32_t ULONG;

// uc_orig: SLONG (MFStdLib/Headers/MFStdLib.h)
typedef int32_t SLONG;

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

// Platform-compatible integer types used throughout the codebase.
// Guarded by _WINDEF_ to avoid redefinition when windows.h is included (DX6 backend).
#ifndef _WINDEF_
typedef uint32_t DWORD;
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
#endif

#ifndef _WINNT_
typedef char CHAR;
typedef wchar_t WCHAR;
typedef CHAR* LPSTR;
typedef const CHAR* LPCSTR;
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif

#ifndef TEXT
#define TEXT(x) x
#endif

#ifndef LOWORD
#define LOWORD(l) ((WORD)((DWORD)(l) & 0xffff))
#endif
#ifndef HIWORD
#define HIWORD(l) ((WORD)(((DWORD)(l) >> 16) & 0xffff))
#endif

typedef CHAR TCHAR;

// Cross-platform compat for POSIX functions that MSVC renames with underscores.
#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#define oc_getcwd    _getcwd
#define oc_stricmp   _stricmp
#define oc_strnicmp  _strnicmp
#define oc_mkdir(p)  _mkdir(p)
#else
#include <unistd.h>
#include <strings.h>
#define oc_getcwd    getcwd
#define oc_stricmp   strcasecmp
#define oc_strnicmp  strncasecmp
#define oc_mkdir(p)  mkdir(p, 0755)
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
