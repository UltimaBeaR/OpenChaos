#include "assets/tga_globals.h"

// uc_orig: tga_width (fallen/DDLibrary/Source/Tga.cpp)
SLONG tga_width;
// uc_orig: tga_height (fallen/DDLibrary/Source/Tga.cpp)
SLONG tga_height;

// uc_orig: TGA_header (fallen/DDLibrary/Source/Tga.cpp)
UBYTE TGA_header[18] = {
    0, 0, 2, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 1,
    0, 1,
    24,
    0
};

// uc_orig: tclump (fallen/DDLibrary/Source/Tga.cpp)
FileClump* tclump = NULL;
// uc_orig: writing (fallen/DDLibrary/Source/Tga.cpp)
bool writing;
// uc_orig: init_convert (fallen/DDLibrary/Source/Tga.cpp)
bool init_convert = false;

// uc_orig: C8to4 (fallen/DDLibrary/Source/Tga.cpp)
UBYTE C8to4[256];
// uc_orig: C8to5 (fallen/DDLibrary/Source/Tga.cpp)
UBYTE C8to5[256];
// uc_orig: C8to6 (fallen/DDLibrary/Source/Tga.cpp)
UBYTE C8to6[256];
// uc_orig: C4to8 (fallen/DDLibrary/Source/Tga.cpp)
UBYTE C4to8[16];
// uc_orig: C5to8 (fallen/DDLibrary/Source/Tga.cpp)
UBYTE C5to8[32];
// uc_orig: C6to8 (fallen/DDLibrary/Source/Tga.cpp)
UBYTE C6to8[64];
