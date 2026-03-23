#include "assets/xlat_str_globals.h"

// uc_orig: xlat_buffer (fallen/Source/xlat_str.cpp)
CBYTE xlat_buffer[_MAX_PATH + 100];

// uc_orig: xlat_set (fallen/Source/xlat_str.cpp)
CBYTE xlat_set[MAX_STRING_SPACE];

// uc_orig: xlat_ptr (fallen/Source/xlat_str.cpp)
CBYTE* xlat_ptr[MAX_STRINGS];

// uc_orig: xlat_upto (fallen/Source/xlat_str.cpp)
CBYTE* xlat_upto = 0;

// uc_orig: previous_byte (fallen/Source/xlat_str.cpp)
UBYTE previous_byte = 0;
