#ifndef OUTRO_CORE_OUTRO_TGA_H
#define OUTRO_CORE_OUTRO_TGA_H

#include "outro/core/outro_always.h"

// BGRA pixel from a 32-bit uncompressed TGA.
// Named OUTRO_TGA_Pixel to avoid collision with the main game's TGA_Pixel in DDLibrary.
// uc_orig: TGA_Pixel (fallen/outro/Tga.h)
typedef struct
{
    UBYTE blue;
    UBYTE green;
    UBYTE red;
    UBYTE alpha;
} OUTRO_TGA_Pixel;

// Result descriptor from OUTRO_TGA_load. Uses a flag bitmask rather than the main
// game's TGA_Info (which has a single SLONG contains_alpha field).
// uc_orig: TGA_Info (fallen/outro/Tga.h)
// uc_orig: TGA_FLAG_CONTAINS_ALPHA (fallen/outro/Tga.h)
#define TGA_FLAG_CONTAINS_ALPHA (1 << 0)
// uc_orig: TGA_FLAG_ONE_BIT_ALPHA (fallen/outro/Tga.h)
#define TGA_FLAG_ONE_BIT_ALPHA (1 << 1)
// uc_orig: TGA_FLAG_GRAYSCALE (fallen/outro/Tga.h)
#define TGA_FLAG_GRAYSCALE (1 << 2)

typedef struct
{
    SLONG valid;
    SLONG width;
    SLONG height;
    ULONG flag; // TGA_FLAG_* bitmask
} OUTRO_TGA_Info;

// uc_orig: OUTRO_TGA_FileLoad_Error (fallen/outro/OutroTga.cpp)
// Helper: closes file handle and marks the result as invalid.
OUTRO_TGA_Info OUTRO_TGA_FileLoad_Error(OUTRO_TGA_Info& ans, FILE*& handle, const CBYTE*& file);

// uc_orig: OUTRO_TGA_load (fallen/outro/OutroTga.cpp)
// Loads an uncompressed 32-bit or 24-bit TGA file. Returns metadata; pixel data goes into 'data'.
// If the image exceeds max_width or max_height the file is rejected (valid == UC_FALSE).
OUTRO_TGA_Info OUTRO_TGA_load(
    const CBYTE* file,
    SLONG max_width,
    SLONG max_height,
    OUTRO_TGA_Pixel* data);

#endif // OUTRO_CORE_OUTRO_TGA_H
