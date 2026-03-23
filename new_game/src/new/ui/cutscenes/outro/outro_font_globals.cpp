#include "ui/cutscenes/outro/outro_font_globals.h"

// uc_orig: FONT_letter (fallen/outro/outroFont.cpp)
FONT_Letter FONT_letter[FONT_NUM_LETTERS];

// uc_orig: FONT_punct (fallen/outro/outroFont.cpp)
CBYTE FONT_punct[] = {
    "!\""
    "\xa3"
    "$%^&*(){}[]<>\\/:;'@#~?-=+.,"

    "\xa9\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfc\xfd\xfe\xff"
};

// uc_orig: FONT_ot (fallen/outro/outroFont.cpp)
OS_Texture* FONT_ot = nullptr;

// uc_orig: FONT_data (fallen/outro/outroFont.cpp)
OUTRO_TGA_Pixel FONT_data[256][256];

// uc_orig: FONT_end_x (fallen/outro/outroFont.cpp)
float FONT_end_x = 0.0F;

// uc_orig: FONT_end_y (fallen/outro/outroFont.cpp)
float FONT_end_y = 0.0F;
