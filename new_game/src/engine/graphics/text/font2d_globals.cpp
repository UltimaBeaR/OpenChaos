#include "engine/graphics/text/font2d_globals.h"

// Total character count: 26 lower + 26 upper + 10 digits + 30 punctuation
// + 10 German + 14 French + 11 Spanish + 12 Italian = 139 entries.
// uc_orig: FONT2D_letter (fallen/DDEngine/Source/font2d.cpp)
FONT2D_Letter FONT2D_letter[139];

// Characters in the order they appear in the font atlas after the alphanumeric block.
// Multi-byte escape sequences are intentional — they match the TGA layout exactly.
// uc_orig: FONT2D_punct (fallen/DDEngine/Source/font2d.cpp)
CBYTE FONT2D_punct[] = {
    "!\""
    "\xa3"
    "$%^&*(){}[]<>\\/:;'@#_?-=+.,"

    // German
    "\xc4\xcb\xcf\xd6\xd8\xdc\xdf\xef\xf6\xf8"

    // French
    "\xc6\xc7\xd4\xe0\xe2\xe7\xe8\xe9\xea\xee\xef\xf4\xf8\xfb"

    // Spanish
    "\xa1\xbf\xd8\xe1\xe4\xe9\xed\xf1\xf3\xf8\xfa"

    // Italian
    "\xc0\xc8\xcc\xd2\xd9\xe0\xec\xf2\xf9\xfc\xa9\xae"
};

// Strikethrough offset state — reused across calls when bUseLastOffset is true.
// uc_orig: DST_offset1 (fallen/DDEngine/Source/font2d.cpp)
SLONG DST_offset1;
// uc_orig: DST_offset2 (fallen/DDEngine/Source/font2d.cpp)
SLONG DST_offset2;

// x-coordinate of the rightmost character drawn by the last DrawString call.
// uc_orig: FONT2D_rightmost_x (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_rightmost_x;
// x-coordinate of the leftmost character drawn during right-justify layout.
// uc_orig: FONT2D_leftmost_x (fallen/DDEngine/Headers/font2d.h)
SLONG FONT2D_leftmost_x;

// Temporary TGA pixel buffer used during FONT2D_init for atlas scanning (freed after init).
// uc_orig: FONT2D_data (fallen/DDEngine/Source/font2d.cpp)
void* font2d_data;
