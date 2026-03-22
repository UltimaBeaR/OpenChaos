#ifndef ASSETS_TGA_H
#define ASSETS_TGA_H

#include "assets/file_clump.h"
#include "core/types.h"
#include <windows.h>

// RGBA pixel layout matching the TGA file format (stored as BGRA on disk).
// uc_orig: TGA_Pixel (fallen/DDLibrary/Headers/Tga.h)
typedef struct
{
    UBYTE blue;
    UBYTE green;
    UBYTE red;
    UBYTE alpha;

} TGA_Pixel;

// Metadata returned after loading a TGA: dimensions, validity flag, alpha presence.
// uc_orig: TGA_Info (fallen/DDLibrary/Headers/Tga.h)
typedef struct
{
    SLONG valid;
    SLONG width;
    SLONG height;
    SLONG contains_alpha;

} TGA_Info;

// Load a TGA from file or clump. If the image exceeds max_width/max_height it is rejected.
// When bCanShrink is UC_TRUE the image may be downscaled to fit. Returns TGA_Info with valid=0 on failure.
// uc_orig: TGA_load (fallen/DDLibrary/Headers/Tga.h)
TGA_Info TGA_load(
    const CBYTE* file,
    SLONG max_width,
    SLONG max_height,
    TGA_Pixel* data,
    ULONG id,
    BOOL bCanShrink = UC_TRUE);

// Load a paletted TGA and remap its colours using a remap palette file.
// uc_orig: TGA_load_remap (fallen/DDLibrary/Source/Tga.cpp)
TGA_Info TGA_load_remap(const CBYTE* file, const CBYTE* pname, SLONG max_width, SLONG max_height, TGA_Pixel* data);

// Save pixel data to a TGA file. Pass contains_alpha=UC_FALSE to write 24-bit without alpha.
// uc_orig: TGA_save (fallen/DDLibrary/Headers/Tga.h)
void TGA_save(
    const CBYTE* file,
    SLONG width,
    SLONG height,
    TGA_Pixel* data,
    SLONG contains_alpha);

// Open a texture clump archive for reading or writing.
// uc_orig: OpenTGAClump (fallen/DDLibrary/Headers/Tga.h)
void OpenTGAClump(const char* clumpfn, size_t maxid, bool readonly);

// Check whether a TGA entry exists (by filename when writing, by id when reading).
// uc_orig: DoesTGAExist (fallen/DDLibrary/Headers/Tga.h)
bool DoesTGAExist(const char* filename, ULONG id);

// Close and free the active texture clump.
// uc_orig: CloseTGAClump (fallen/DDLibrary/Headers/Tga.h)
void CloseTGAClump();

// Return the active texture clump pointer.
// uc_orig: GetTGAClump (fallen/DDLibrary/Headers/Tga.h)
FileClump* GetTGAClump();

#endif // ASSETS_TGA_H
