#ifndef UI_CUTSCENES_OUTRO_OUTRO_TGA_H
#define UI_CUTSCENES_OUTRO_OUTRO_TGA_H

// Use the original outro Tga.h for type definitions to avoid ODR violations
// with outroFont.cpp which is still compiled from old/ and uses the same types.
#include "fallen/outro/always.h" // Temporary: provides UBYTE, SLONG, ULONG etc. for Tga.h
#include "fallen/outro/Tga.h"   // Temporary: OUTRO_TGA_Pixel, OUTRO_TGA_Info, TGA_FLAG_*

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

#endif // UI_CUTSCENES_OUTRO_OUTRO_TGA_H
