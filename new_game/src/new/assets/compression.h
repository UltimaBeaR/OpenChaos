#ifndef ASSETS_COMPRESSION_H
#define ASSETS_COMPRESSION_H

#include "assets/tga.h"

// uc_orig: COMP_SIZE (fallen/DDEngine/Headers/comp.h)
#define COMP_SIZE 64
// uc_orig: COMP_SNUM (fallen/DDEngine/Headers/comp.h)
#define COMP_SNUM 8
// uc_orig: COMP_SSIZE (fallen/DDEngine/Headers/comp.h)
#define COMP_SSIZE (COMP_SIZE / COMP_SNUM)

// uc_orig: COMP_Frame (fallen/DDEngine/Headers/comp.h)
typedef struct
{
    TGA_Pixel p[COMP_SIZE][COMP_SIZE]; // Access (y,x)!

} COMP_Frame;

// uc_orig: COMP_Delta (fallen/DDEngine/Headers/comp.h)
typedef struct
{
    SLONG size;
    UBYTE data[];

} COMP_Delta;

// uc_orig: COMP_TGA_MAX_WIDTH (fallen/DDEngine/Headers/comp.h)
#define COMP_TGA_MAX_WIDTH 640
// uc_orig: COMP_TGA_MAX_HEIGHT (fallen/DDEngine/Headers/comp.h)
#define COMP_TGA_MAX_HEIGHT 480

// uc_orig: COMP_MAX_DATA (fallen/DDEngine/Source/comp.cpp)
#define COMP_MAX_DATA (1024 * 16)

// Named wrapper for the anonymous struct used by COMP_data.
typedef struct
{
    SLONG size;
    UBYTE data[COMP_MAX_DATA];

} COMP_DataBuffer;

// Loads a frame from a .TGA file. Returns TRUE on success.
// uc_orig: COMP_load (fallen/DDEngine/Headers/comp.h)
SLONG COMP_load(CBYTE* filename, COMP_Frame* cf);

// Calculates a delta that maps f1 onto f2, storing the resulting frame in ans.
// Returns a pointer to a static COMP_Delta — subsequent calls overwrite previous results.
// uc_orig: COMP_calc (fallen/DDEngine/Headers/comp.h)
COMP_Delta* COMP_calc(COMP_Frame* f1, COMP_Frame* f2, COMP_Frame* ans);

// Reconstructs a frame by applying delta to base, writing the result into result.
// uc_orig: COMP_decomp (fallen/DDEngine/Headers/comp.h)
void COMP_decomp(
    COMP_Frame* base,
    COMP_Delta* delta,
    COMP_Frame* result);

#include "compression_globals.h"

#endif // ASSETS_COMPRESSION_H
