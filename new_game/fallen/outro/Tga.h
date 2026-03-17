//
// Loads in 32-bit RGBA uncompressed TGAs.
// Standalone TGA loader for the outro module.
//

#ifndef FALLEN_OUTRO_TGA_H
#define FALLEN_OUTRO_TGA_H

//
// The format of a TGA pixel.
// uc_orig: TGA_Pixel - also defined identically in fallen/DDLibrary/Headers/Tga.h;
//   renamed to OUTRO_TGA_Pixel to resolve naming conflict exposed by unique include guards.
//

typedef struct
{
    UBYTE blue;
    UBYTE green;
    UBYTE red;
    UBYTE alpha;

} OUTRO_TGA_Pixel;

//
// Info describing the tga.
// uc_orig: TGA_Info - also defined in fallen/DDLibrary/Headers/Tga.h but with different
//   4th field (SLONG contains_alpha vs ULONG flag bitmask here); renamed to OUTRO_TGA_Info.
//

#define TGA_FLAG_CONTAINS_ALPHA (1 << 0)
#define TGA_FLAG_ONE_BIT_ALPHA (1 << 1) // Alpha is only 255 or 0
#define TGA_FLAG_GRAYSCALE (1 << 2) // For all pixels r == g == b.

typedef struct
{
    SLONG valid;
    SLONG width;
    SLONG height;
    ULONG flag;

} OUTRO_TGA_Info;

//
// If the width and height of the tga exceed the maximums, then the tga is not loaded.
// uc_orig: TGA_load - also declared in fallen/DDLibrary/Headers/Tga.h with different
//   signature (6 params vs 4 here); renamed to OUTRO_TGA_load to resolve conflict.
//

OUTRO_TGA_Info OUTRO_TGA_load(
    const CBYTE* file,
    SLONG max_width,
    SLONG max_height,
    OUTRO_TGA_Pixel* data);


#endif // FALLEN_OUTRO_TGA_H
