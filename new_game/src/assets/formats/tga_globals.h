#ifndef ASSETS_FORMATS_TGA_GLOBALS_H
#define ASSETS_FORMATS_TGA_GLOBALS_H

#include "engine/core/types.h"
#include "assets/formats/file_clump.h"

// uc_orig: tga_width (fallen/DDLibrary/Source/Tga.cpp)
extern SLONG tga_width;
// uc_orig: tga_height (fallen/DDLibrary/Source/Tga.cpp)
extern SLONG tga_height;
// uc_orig: TGA_header (fallen/DDLibrary/Source/Tga.cpp)
extern UBYTE TGA_header[18];

// uc_orig: tclump (fallen/DDLibrary/Source/Tga.cpp)
extern FileClump* tclump;
// uc_orig: writing (fallen/DDLibrary/Source/Tga.cpp)
extern bool writing;
// uc_orig: init_convert (fallen/DDLibrary/Source/Tga.cpp)
extern bool init_convert;

// uc_orig: C8to4 (fallen/DDLibrary/Source/Tga.cpp)
extern UBYTE C8to4[256];
// uc_orig: C8to5 (fallen/DDLibrary/Source/Tga.cpp)
extern UBYTE C8to5[256];
// uc_orig: C8to6 (fallen/DDLibrary/Source/Tga.cpp)
extern UBYTE C8to6[256];
// uc_orig: C4to8 (fallen/DDLibrary/Source/Tga.cpp)
extern UBYTE C4to8[16];
// uc_orig: C5to8 (fallen/DDLibrary/Source/Tga.cpp)
extern UBYTE C5to8[32];
// uc_orig: C6to8 (fallen/DDLibrary/Source/Tga.cpp)
extern UBYTE C6to8[64];

#endif // ASSETS_FORMATS_TGA_GLOBALS_H
