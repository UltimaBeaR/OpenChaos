#include "engine/compression/image_compression.h"
#include "engine/platform/uc_common.h"

// uc_orig: IC_convert (fallen/DDEngine/Source/ic.cpp)
static ULONG IC_convert(UBYTE r, UBYTE g, UBYTE b)
{
    SLONG nr = r;
    SLONG ng = g;
    SLONG nb = b;

    nr = (nr + 4) >> 3;
    ng = (ng + 2) >> 2;
    nb = (nb + 4) >> 3;

    if (nr > 31) { nr = 31; }
    if (ng > 63) { ng = 63; }
    if (nb > 31) { nb = 31; }

    return (ULONG)((nr << 11) | (ng << 5) | (nb));
}

// uc_orig: IC_pack (fallen/DDEngine/Source/ic.cpp)
IC_Packet IC_pack(TGA_Pixel* tga, SLONG tga_width, SLONG tga_height, SLONG px, SLONG py)
{
    SLONG i, j, k, l;
    SLONG x, y, x1, y1, x2, y2;
    SLONG r1, g1, b1, r2, g2, b2;
    SLONG dr, dg, db;
    SLONG r[4], g[4], b[4];
    TGA_Pixel *tp, *tp1, *tp2;
    SLONG error;
    ULONG bit;
    SLONG dist, best_dist, best_bit;
    SLONG best_error = INFINITY;
    IC_Packet best_ans;

    for (i = 1; i < 16; i++)
    for (j = 0; j <  i; j++) {
        if (i == j) { continue; }

        x1 = px + (i  & 0x3);
        y1 = py + (i >> 0x2);
        x2 = px + (j  & 0x3);
        y2 = py + (j >> 0x2);

        tp1 = &tga[x1 + y1 * tga_width];
        tp2 = &tga[x2 + y2 * tga_width];

        r1 = tp1->red;   g1 = tp1->green; b1 = tp1->blue;
        r2 = tp2->red;   g2 = tp2->green; b2 = tp2->blue;

        if (r1 == r2 && g1 == g2 && b1 == b2) { continue; }

        r[0] = r1; g[0] = g1; b[0] = b1;
        dr = r2 - r1; dg = g2 - g1; db = b2 - b1;
        r[1] = r1 + (dr *  85 >> 8); g[1] = g1 + (dg *  85 >> 8); b[1] = b1 + (db *  85 >> 8);
        r[2] = r1 + (dr * 170 >> 8); g[2] = g1 + (dg * 170 >> 8); b[2] = b1 + (db * 170 >> 8);
        r[3] = r2; g[3] = g2; b[3] = b2;

        bit = 0; error = 0;
        for (k = 0; k < 16; k++) {
            x = px + (k  & 0x3);
            y = py + (k >> 0x2);
            tp = &tga[x + y * tga_width];
            best_dist = INFINITY; best_bit = 0;
            for (l = 0; l < 4; l++) {
                dr = r[l] - tp->red; dg = g[l] - tp->green; db = b[l] - tp->blue;
                dist = dr * dr * 3 + dg * dg * 2 + db * db;
                if (dist < best_dist) { best_dist = dist; best_bit = l; }
            }
            error += best_dist;
            bit <<= 2; bit |= (ULONG)best_bit;
        }

        if (error < best_error) {
            best_error       = error;
            best_ans.colour1 = (UWORD)IC_convert((UBYTE)r1, (UBYTE)g1, (UBYTE)b1);
            best_ans.colour2 = (UWORD)IC_convert((UBYTE)r2, (UBYTE)g2, (UBYTE)b2);
            best_ans.bit     = bit;
        }
    }

    if (best_error == INFINITY) {
        tp = &tga[px + py * tga_width];
        best_ans.colour1 = (UWORD)IC_convert(tp->red, tp->green, tp->blue);
        best_ans.colour2 = (UWORD)IC_convert(tp->red, tp->green, tp->blue);
        best_ans.bit     = 0;
    }

    return best_ans;
}

// uc_orig: IC_unpack (fallen/DDEngine/Source/ic.cpp)
void IC_unpack(IC_Packet ip, TGA_Pixel* tga, SLONG tga_width, SLONG tga_height, SLONG px, SLONG py)
{
    SLONG i, bit;
    SLONG r[4], g[4], b[4];
    TGA_Pixel* tp;

    r[0] = (((ip.colour1 >> 11)       ) << 3) + 4;
    g[0] = (((ip.colour1 >>  5) & 0x3f) << 2) + 2;
    b[0] = (((ip.colour1      ) & 0x1f) << 3) + 4;
    r[3] = (((ip.colour2 >> 11)       ) << 3) + 4;
    g[3] = (((ip.colour2 >>  5) & 0x3f) << 2) + 2;
    b[3] = (((ip.colour2      ) & 0x1f) << 3) + 4;

    SLONG dr = (r[3] - r[0]) * 85 >> 8;
    SLONG dg = (g[3] - g[0]) * 85 >> 8;
    SLONG db = (b[3] - b[0]) * 85 >> 8;
    r[1] = r[0] + dr; g[1] = g[0] + dg; b[1] = b[0] + db;
    r[2] = r[1] + dr; g[2] = g[1] + dg; b[2] = b[1] + db;

    for (i = 0; i < 4; i++) {
        tp = &tga[px + (py + i) * tga_width];

        #define IC_DECOMPRESS_A_PIXEL() \
        { bit = ip.bit >> 30; ip.bit <<= 2; tp->red = (UBYTE)r[bit]; tp->green = (UBYTE)g[bit]; tp->blue = (UBYTE)b[bit]; tp += 1; }

        IC_DECOMPRESS_A_PIXEL();
        IC_DECOMPRESS_A_PIXEL();
        IC_DECOMPRESS_A_PIXEL();
        IC_DECOMPRESS_A_PIXEL();

        #undef IC_DECOMPRESS_A_PIXEL
    }
}
