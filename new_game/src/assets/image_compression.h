#ifndef ASSETS_IMAGE_COMPRESSION_H
#define ASSETS_IMAGE_COMPRESSION_H

#include "assets/tga.h"
#include "engine/platform/platform.h"

// S3-style compressed block: two 5:6:5 endpoint colours and a 2-bit-per-pixel index bitfield.
// uc_orig: IC_Packet (fallen/DDEngine/Headers/ic.h)
typedef struct
{
    UWORD colour1;
    UWORD colour2;
    ULONG bit;

} IC_Packet;

// Pack a 4x4 pixel block from a TGA into an IC_Packet using S3-style compression.
// Ignores the alpha channel.
// uc_orig: IC_pack (fallen/DDEngine/Headers/ic.h)
IC_Packet IC_pack(
    TGA_Pixel* tga,
    SLONG tga_width,
    SLONG tga_height,
    SLONG px,
    SLONG py);

// Decompress an IC_Packet back into a 4x4 pixel block in a TGA.
// uc_orig: IC_unpack (fallen/DDEngine/Headers/ic.h)
void IC_unpack(
    IC_Packet ip,
    TGA_Pixel* tga,
    SLONG tga_width,
    SLONG tga_height,
    SLONG px,
    SLONG py);

#endif // ASSETS_IMAGE_COMPRESSION_H
