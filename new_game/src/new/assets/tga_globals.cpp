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
