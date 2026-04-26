#ifndef ENGINE_COMPRESSION_IMAGE_COMPRESSION_H
#define ENGINE_COMPRESSION_IMAGE_COMPRESSION_H

// S3 texture compression (4x4 block, 5:6:5 colour pairs + 2-bit per-pixel index).
// uc_orig: ic.h (fallen/DDEngine/Headers/ic.h)

#include "engine/core/types.h"
#include "assets/formats/tga.h"

// uc_orig: IC_Packet (fallen/DDEngine/Headers/ic.h)
typedef struct {
    UWORD colour1; // 5:6:5
    UWORD colour2; // 5:6:5
    ULONG bit;
} IC_Packet;

// uc_orig: IC_pack (fallen/DDEngine/Headers/ic.h)
IC_Packet IC_pack(TGA_Pixel* tga, SLONG tga_width, SLONG tga_height, SLONG px, SLONG py);

// uc_orig: IC_unpack (fallen/DDEngine/Headers/ic.h)
void IC_unpack(IC_Packet ip, TGA_Pixel* tga, SLONG tga_width, SLONG tga_height, SLONG px, SLONG py);

#endif // ENGINE_COMPRESSION_IMAGE_COMPRESSION_H
