#include <MFStdLib.h>
#include "assets/compression.h"
#include "assets/image_compression.h"
#include "assets/tga.h"

// Internal delta data layout: zero or more COMP_Pan RLE entries followed by
// one or more COMP_Update entries. The last COMP_Update has last == TRUE.

// uc_orig: COMP_Pan (fallen/DDEngine/Source/comp.cpp)
typedef struct
{
    UBYTE num;
    UBYTE pan;

} COMP_Pan;

// uc_orig: COMP_Update (fallen/DDEngine/Source/comp.cpp)
typedef struct
{
    UBYTE x;
    UBYTE y;
    UBYTE num;
    UBYTE last; // UC_TRUE => there are no more update structures.
    IC_Packet ip[];

} COMP_Update;


// Samples the loaded TGA at normalised coordinates [0,1) x [0,1).
// uc_orig: COMP_tga_colour (fallen/DDEngine/Source/comp.cpp)
static TGA_Pixel COMP_tga_colour(float x, float y)
{
    TGA_Pixel ans;
    SLONG px;
    SLONG py;

    px = SLONG(float(COMP_tga_info.width) * x);
    py = SLONG(float(COMP_tga_info.height) * y);

    SATURATE(px, 0, COMP_tga_info.width - 1);
    SATURATE(py, 0, COMP_tga_info.height - 1);

    ans = COMP_tga_data[px + py * COMP_tga_info.width];

    return ans;
}

// uc_orig: COMP_load (fallen/DDEngine/Source/comp.cpp)
SLONG COMP_load(CBYTE* filename, COMP_Frame* cf)
{

    SLONG px;
    SLONG py;

    SLONG dx;
    SLONG dy;

    SLONG r;
    SLONG g;
    SLONG b;

    TGA_Pixel tp;

    float x;
    float y;

    COMP_tga_info = TGA_load(
        filename,
        COMP_TGA_MAX_WIDTH,
        COMP_TGA_MAX_HEIGHT,
        COMP_tga_data, -1);

    if (!COMP_tga_info.valid) {
        return UC_FALSE;
    }

    // Downsample the TGA to fit into a COMP_SIZE x COMP_SIZE frame
    // uc_orig: COMP_SAMPLES_PER_PIXEL (fallen/DDEngine/Source/comp.cpp)
#define COMP_SAMPLES_PER_PIXEL 4

    for (px = 0; px < COMP_SIZE; px++)
        for (py = 0; py < COMP_SIZE; py++) {
            r = 0;
            g = 0;
            b = 0;

            for (dx = 0; dx < COMP_SAMPLES_PER_PIXEL; dx++)
                for (dy = 0; dy < COMP_SAMPLES_PER_PIXEL; dy++) {
                    x = float((px * COMP_SAMPLES_PER_PIXEL) + dx) * (1.0F / float(COMP_SIZE * COMP_SAMPLES_PER_PIXEL));
                    y = float((py * COMP_SAMPLES_PER_PIXEL) + dy) * (1.0F / float(COMP_SIZE * COMP_SAMPLES_PER_PIXEL));

                    tp = COMP_tga_colour(x, y);

                    r += tp.red;
                    g += tp.green;
                    b += tp.blue;
                }

            r /= COMP_SAMPLES_PER_PIXEL * COMP_SAMPLES_PER_PIXEL;
            g /= COMP_SAMPLES_PER_PIXEL * COMP_SAMPLES_PER_PIXEL;
            b /= COMP_SAMPLES_PER_PIXEL * COMP_SAMPLES_PER_PIXEL;

            cf->p[py][px].red = r;
            cf->p[py][px].green = g;
            cf->p[py][px].blue = b;
        }

    return UC_TRUE;
}

// Returns the sum of absolute per-channel differences between two same-sized
// squares, each clamped to remain inside the frame bounds.
// uc_orig: COMP_square_error (fallen/DDEngine/Source/comp.cpp)
static SLONG COMP_square_error(
    COMP_Frame* f1,
    SLONG sx1,
    SLONG sy1,
    COMP_Frame* f2,
    SLONG sx2,
    SLONG sy2,
    SLONG size)
{
    SLONG dx;
    SLONG dy;

    SLONG x1;
    SLONG y1;

    SLONG x2;
    SLONG y2;

    TGA_Pixel* tp1;
    TGA_Pixel* tp2;

    SLONG error = 0;

    for (dx = 0; dx < size; dx++)
        for (dy = 0; dy < size; dy++) {
            x1 = sx1 + dx;
            y1 = sy1 + dy;

            x2 = sx2 + dx;
            y2 = sy2 + dy;

            SATURATE(x1, 0, COMP_SIZE - 1);
            SATURATE(y1, 0, COMP_SIZE - 1);

            SATURATE(x2, 0, COMP_SIZE - 1);
            SATURATE(y2, 0, COMP_SIZE - 1);

            tp1 = &f1->p[y1][x1];
            tp2 = &f2->p[y2][x2];

            error += abs(tp1->red - tp2->red);
            error += abs(tp1->green - tp2->green);
            error += abs(tp1->blue - tp2->blue);
        }

    return error;
}

// Copies a square region from f1 into f2; coordinates are clamped to frame bounds.
// uc_orig: COMP_square_copy (fallen/DDEngine/Source/comp.cpp)
static void COMP_square_copy(
    COMP_Frame* f1,
    SLONG sx1,
    SLONG sy1,
    COMP_Frame* f2,
    SLONG sx2,
    SLONG sy2,
    SLONG size)
{
    SLONG dx;
    SLONG dy;

    SLONG x1;
    SLONG y1;

    SLONG x2;
    SLONG y2;

    TGA_Pixel* tp1;
    TGA_Pixel* tp2;

    for (dx = 0; dx < size; dx++)
        for (dy = 0; dy < size; dy++) {
            x1 = sx1 + dx;
            y1 = sy1 + dy;

            x2 = sx2 + dx;
            y2 = sy2 + dy;

            SATURATE(x1, 0, COMP_SIZE - 1);
            SATURATE(y1, 0, COMP_SIZE - 1);

            SATURATE(x2, 0, COMP_SIZE - 1);
            SATURATE(y2, 0, COMP_SIZE - 1);

            tp1 = &f1->p[y1][x1];
            tp2 = &f2->p[y2][x2];

            *tp2 = *tp1;
        }
}


// uc_orig: COMP_calc (fallen/DDEngine/Source/comp.cpp)
COMP_Delta* COMP_calc(COMP_Frame* f1, COMP_Frame* f2, COMP_Frame* ans)
{
    SLONG i;

    SLONG sx;
    SLONG sy;

    SLONG dx;
    SLONG dy;

    SLONG sx1;
    SLONG sy1;

    SLONG sx2;
    SLONG sy2;

    SLONG error;

    SLONG best_error;
    SLONG best_dx;
    SLONG best_dy;

    UBYTE pan[COMP_SNUM * COMP_SNUM];
    SLONG pan_upto = 0;
    SLONG pan_index;

    COMP_Pan* cp;
    COMP_Update* cu;
    IC_Packet* ip;

    SLONG cu_valid;
    SLONG cu_num;

    // uc_orig: COMP_MAX_PAN (fallen/DDEngine/Source/comp.cpp)
#define COMP_MAX_PAN 5

    for (sx = 0; sx < COMP_SNUM; sx++)
        for (sy = 0; sy < COMP_SNUM; sy++) {
            sx2 = sx * COMP_SSIZE;
            sy2 = sy * COMP_SSIZE;

            best_error = UC_INFINITY;
            best_dx = 0;
            best_dy = 0;

            for (dx = -COMP_MAX_PAN; dx <= +COMP_MAX_PAN; dx++)
                for (dy = -COMP_MAX_PAN; dy <= +COMP_MAX_PAN; dy++) {
                    sx1 = sx2 + dx;
                    sy1 = sy2 + dy;

                    error = COMP_square_error(
                        f1, sx1, sy1,
                        f2, sx2, sy2,
                        COMP_SSIZE);

                    if (error < best_error) {
                        best_error = error;
                        best_dx = dx;
                        best_dy = dy;

                        if (best_error == 0) {
                            goto found_best_pan;
                        }
                    }
                }

        found_best_pan:;

    // uc_orig: COMP_MAX_ERROR_PER_PIXEL (fallen/DDEngine/Source/comp.cpp)
#define COMP_MAX_ERROR_PER_PIXEL 32

            if (best_error < (COMP_SSIZE * COMP_SSIZE * COMP_MAX_ERROR_PER_PIXEL)) {
                pan[pan_upto++] = (best_dx + COMP_MAX_PAN) | ((best_dy + COMP_MAX_PAN) << 4);
            } else {
                pan[pan_upto++] = 255;
            }
        }

    // RLE-encode the pan values into the output buffer.

    cp = (COMP_Pan*)COMP_data.data;

    cp->num = 1;
    cp->pan = pan[0];

    for (i = 1; i < COMP_SNUM * COMP_SNUM; i++) {
        if (pan[i] == cp->pan && cp->num != 255) {
            cp->num += 1;
        } else {
            cp += 1;

            cp->num = 1;
            cp->pan = pan[i];
        }
    }

    // Apply pan offsets to build the preliminary reconstructed frame.

    pan_upto = 0;

    for (sx = 0; sx < COMP_SNUM; sx++)
        for (sy = 0; sy < COMP_SNUM; sy++) {
            sx2 = sx * COMP_SSIZE;
            sy2 = sy * COMP_SSIZE;

            if (pan[pan_upto] != 255) {
                dx = pan[pan_upto] & 0xf;
                dy = pan[pan_upto] >> 4;

                dx -= COMP_MAX_PAN;
                dy -= COMP_MAX_PAN;

                sx1 = sx2 + dx;
                sy1 = sy2 + dy;

                COMP_square_copy(
                    f1, sx1, sy1,
                    ans, sx2, sy2,
                    COMP_SSIZE);
            }

            pan_upto += 1;
        }

    // Emit IC packets for 4x4 blocks that differ too much from f2.

    cu = (COMP_Update*)(cp + 1);
    ip = (IC_Packet*)(cp + 1);
    cu_valid = UC_FALSE;
    cu_num = 0;

    for (sx = 0; sx < COMP_SIZE; sx += 4)
        for (sy = 0; sy < COMP_SIZE; sy += 4) {
            pan_index = (sx / COMP_SSIZE) * COMP_SNUM + (sy / COMP_SSIZE);

            if (pan[pan_index] == 255) {
                error = UC_INFINITY;
            } else {
                error = COMP_square_error(
                    f1, sx, sy,
                    f2, sx, sy,
                    4);
            }

            if (error >= 16 * COMP_MAX_ERROR_PER_PIXEL) {
                cu_num += 1;

                if (cu_valid) {
                    *ip = IC_pack(
                        (TGA_Pixel*)f2->p,
                        COMP_SIZE,
                        COMP_SIZE,
                        sx,
                        sy);

                    cu->num += 1;

                    if (cu->num == 255) {
                        cu_valid = UC_FALSE;
                    }
                } else {
                    cu = (COMP_Update*)ip;

                    cu->x = sx;
                    cu->y = sy;
                    cu->num = 1;
                    cu->last = UC_FALSE;
                    ip = cu->ip;

                    *ip = IC_pack(
                        (TGA_Pixel*)f2->p,
                        COMP_SIZE,
                        COMP_SIZE,
                        sx,
                        sy);

                    cu_valid = UC_TRUE;
                }

                IC_unpack(
                    *ip,
                    (TGA_Pixel*)ans->p,
                    COMP_SIZE,
                    COMP_SIZE,
                    sx,
                    sy);

                ip += 1;
            } else {
                cu_valid = UC_FALSE;
            }
        }

    if (cu_num == 0) {
        // No updates: write a sentinel last packet with zero blocks.
        cu->x = 0;
        cu->y = 0;
        cu->num = 0;
        cu->last = 1;
        ip = cu->ip;
    } else {
        cu->last = UC_TRUE;
    }

    UBYTE* data_start = COMP_data.data;
    UBYTE* data_end = (UBYTE*)ip;

    COMP_data.size = data_end - data_start;

    return (COMP_Delta*)&COMP_data;
}

// uc_orig: COMP_decomp (fallen/DDEngine/Source/comp.cpp)
void COMP_decomp(
    COMP_Frame* base,
    COMP_Delta* delta,
    COMP_Frame* result)
{
    SLONG i;
    SLONG sx;
    SLONG sy;
    SLONG dx;
    SLONG dy;

    COMP_Pan* cp;
    COMP_Update* cu;
    IC_Packet* ip;

    // Apply RLE pan entries to copy sub-blocks from base into result.

    cp = (COMP_Pan*)delta->data;

    sx = 0;
    sy = 0;

    while (1) {
        dx = cp->pan & 0xf;
        dy = cp->pan >> 4;

        dx -= COMP_MAX_PAN;
        dy -= COMP_MAX_PAN;

        for (i = 0; i < cp->num; i++) {
            if (cp->pan != 255) {
                COMP_square_copy(
                    base,
                    sx + dx,
                    sy + dy,
                    result,
                    sx,
                    sy,
                    COMP_SSIZE);
            }

            sy += COMP_SSIZE;

            if (sy >= COMP_SIZE) {
                sy = 0;

                sx += COMP_SSIZE;

                if (sx >= COMP_SIZE) {
                    goto finished_panning;
                }
            }
        }

        cp += 1;
    }

finished_panning:;

    // Apply IC packets to overwrite blocks that needed full updates.

    cu = (COMP_Update*)(cp + 1);

    while (1) {
        sx = cu->x;
        sy = cu->y;
        ip = cu->ip;

        for (i = 0; i < cu->num; i++) {
            IC_unpack(
                *ip,
                (TGA_Pixel*)result->p,
                COMP_SIZE,
                COMP_SIZE,
                sx,
                sy);

            sy += 4;

            if (sy >= COMP_SIZE) {
                sy = 0;

                sx += 4;
            }

            ip += 1;
        }

        if (cu->last) {
            break;
        }

        cu = (COMP_Update*)ip;
    }
}
