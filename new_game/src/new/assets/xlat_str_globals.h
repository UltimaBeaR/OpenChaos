#ifndef ASSETS_XLAT_STR_GLOBALS_H
#define ASSETS_XLAT_STR_GLOBALS_H

#include "assets/xlat_str.h"

// Buffer for translated text output (used by XLAT_str when no dest is given).
// uc_orig: xlat_buffer (fallen/Source/xlat_str.cpp)
extern CBYTE xlat_buffer[_MAX_PATH + 100];

// Raw storage pool for all loaded translation strings.
// uc_orig: xlat_set (fallen/Source/xlat_str.cpp)
extern CBYTE xlat_set[MAX_STRING_SPACE];

// Pointer table: maps string IDs to positions inside xlat_set.
// uc_orig: xlat_ptr (fallen/Source/xlat_str.cpp)
extern CBYTE* xlat_ptr[MAX_STRINGS];

// Points to the next free position in xlat_set.
// uc_orig: xlat_upto (fallen/Source/xlat_str.cpp)
extern CBYTE* xlat_upto;

// Tracks the previous byte for MBCS (multi-byte character set) state.
// On PC this is always zero (MBCS disabled), kept for structure parity.
// uc_orig: previous_byte (fallen/Source/xlat_str.cpp)
extern UBYTE previous_byte;

#endif // ASSETS_XLAT_STR_GLOBALS_H
