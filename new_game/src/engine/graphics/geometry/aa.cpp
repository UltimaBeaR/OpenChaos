#include "engine/graphics/geometry/aa.h"
#include "engine/graphics/geometry/aa_globals.h"
#include "engine/core/macros.h"

// Fixed-point precision: 10 bits sub-pixel, 1024 units per pixel.
// uc_orig: AA_FIX (fallen/DDEngine/Source/aa.cpp)
#define AA_FIX 10
// uc_orig: AA_PIX (fallen/DDEngine/Source/aa.cpp)
#define AA_PIX (1 << AA_FIX)
// uc_orig: AA_PMN (fallen/DDEngine/Source/aa.cpp)
#define AA_PMN (AA_PIX - 1)

// Accumulates right-hand-side (entering) coverage into a single pixel on scanline y.
// frac is the sub-pixel coverage area in fixed-point; shifted to 0-255 range before storing.
// uc_orig: AA_pixel_rhs (fallen/DDEngine/Source/aa.cpp)
static void AA_pixel_rhs(SLONG x, SLONG y, SLONG frac)
{
    frac >>= AA_FIX - 8;
    AA_Span* as = &AA_span[y];
    if (WITHIN(x, as->rhs_min, as->rhs_max)) {
        as->pixel[x] += frac;
    } else {
        as->pixel[x] = frac;
        if (x < as->rhs_min)
            as->rhs_min = x;
        if (x > as->rhs_max)
            as->rhs_max = x;
    }
}

// Accumulates left-hand-side (exiting) coverage into a single pixel on scanline y.
// Subtracts frac_left from the rhs pixel (overlap correction) and adds frac_right to the lhs extent.
// uc_orig: AA_pixel_lhs (fallen/DDEngine/Source/aa.cpp)
static void AA_pixel_lhs(SLONG x, SLONG y, SLONG frac_left, SLONG frac_right)
{
    frac_left >>= AA_FIX - 8;
    frac_right >>= AA_FIX - 8;
    AA_Span* as = &AA_span[y];
    if (WITHIN(x, as->rhs_min, as->rhs_max)) {
        as->pixel[x] -= frac_left;
        if (x < as->lhs_min)
            as->lhs_min = x;
        if (x > as->lhs_max)
            as->lhs_max = x;
    } else {
        if (WITHIN(x, as->lhs_min, as->lhs_max)) {
            as->pixel[x] += frac_right;
        } else {
            as->pixel[x] = frac_right;
            if (x < as->lhs_min)
                as->lhs_min = x;
            if (x > as->lhs_max)
                as->lhs_max = x;
        }
    }
}

// Rasterizes one horizontal pixel-row segment of a right-hand edge.
// Walks sub-pixel columns between x1 and x2 on the row starting at y1,
// computing trapezoid coverage area for each pixel column crossed.
// uc_orig: AA_span_rhs (fallen/DDEngine/Source/aa.cpp)
static void AA_span_rhs(SLONG dydx, SLONG x1, SLONG y1, SLONG x2, SLONG y2)
{
    SLONG dx, dy, xleft, xright, ytop, ybot, frac;
    if (x2 == x1) {
        frac = (x1 & AA_PMN) * (y2 - y1) >> AA_FIX;
        AA_pixel_rhs(x1 >> AA_FIX, y1 >> AA_FIX, frac);
        return;
    }
    if (x2 > x1) {
        xleft = x1;
        ytop = y1;
        while (1) {
            xright = xleft + AA_PIX;
            xright &= ~AA_PMN;
            if (xright > x2)
                xright = x2;
            dx = xright - xleft;
            dy = dx * dydx >> AA_FIX;
            ybot = ytop + dy;
            frac = dx * dy >> (AA_FIX + 1);
            frac += dx * (y2 - ybot) >> AA_FIX;
            if (xleft & AA_PMN) {
                frac += dy * (xleft & AA_PMN) >> AA_FIX;
                frac += (y2 - ybot) * (xleft & AA_PMN) >> AA_FIX;
            }
            AA_pixel_rhs(xleft >> AA_FIX, y1 >> AA_FIX, frac);
            if (xright >= x2)
                return;
            xleft = xright;
            ytop = ybot;
        }
    } else {
        xright = x1;
        ytop = y1;
        while (1) {
            xleft = xright - 1;
            xleft &= ~AA_PMN;
            if (xleft < x2)
                xleft = x2;
            dx = xright - xleft;
            dy = -dx * dydx >> AA_FIX;
            ybot = ytop + dy;
            frac = dx * dy >> (AA_FIX + 1);
            frac += dx * (ytop - y1) >> AA_FIX;
            if (xleft & AA_PMN) {
                frac += dy * (xleft & AA_PMN) >> AA_FIX;
                frac += (ytop - y1) * (xleft & AA_PMN) >> AA_FIX;
            }
            AA_pixel_rhs(xleft >> AA_FIX, y1 >> AA_FIX, frac);
            if (xleft <= x2)
                return;
            xright = xleft;
            ytop = ybot;
        }
    }
}

// Rasterizes one horizontal pixel-row segment of a left-hand edge.
// Mirrors AA_span_rhs but computes both the subtracted left and added right coverage fractions.
// uc_orig: AA_span_lhs (fallen/DDEngine/Source/aa.cpp)
static void AA_span_lhs(SLONG dydx, SLONG x1, SLONG y1, SLONG x2, SLONG y2)
{
    SLONG dx, dy, xleft, xright, xpixel, ytop, ybot, frac_left, frac_right;
    if (x2 == x1) {
        frac_left = (x1 & AA_PMN) * (y2 - y1) >> AA_FIX;
        frac_right = (AA_PMN - (x1 & AA_PMN)) * (y2 - y1) >> AA_FIX;
        AA_pixel_lhs(x1 >> AA_FIX, y1 >> AA_FIX, frac_left, frac_right);
        return;
    }
    if (x2 > x1) {
        xleft = x1;
        ytop = y1;
        while (1) {
            xright = xleft + AA_PIX;
            xright &= ~AA_PMN;
            xpixel = xright;
            if (xright > x2)
                xright = x2;
            dx = xright - xleft;
            dy = dx * dydx >> AA_FIX;
            ybot = ytop + dy;
            frac_right = dx * dy >> (AA_FIX + 1);
            frac_right += dx * (ytop - y1) >> AA_FIX;
            if (xpixel - xright) {
                frac_right += dy * (xpixel - xright) >> AA_FIX;
                frac_right += (ytop - y1) * (xpixel - xright) >> AA_FIX;
            }
            frac_left = dx * dy >> (AA_FIX + 1);
            frac_left += dx * (y2 - ybot) >> AA_FIX;
            if (xleft & AA_PMN) {
                frac_left += dy * (xleft & AA_PMN) >> AA_FIX;
                frac_left += (y2 - ybot) * (xleft & AA_PMN) >> AA_FIX;
            }
            AA_pixel_lhs(xleft >> AA_FIX, y1 >> AA_FIX, frac_left, frac_right);
            if (xright >= x2)
                return;
            xleft = xright;
            ytop = ybot;
        }
    } else {
        xright = x1;
        ytop = y1;
        while (1) {
            xleft = xright - 1;
            xleft &= ~AA_PMN;
            xpixel = xleft + AA_PIX;
            if (xleft < x2)
                xleft = x2;
            dx = xright - xleft;
            dy = dx * -dydx >> AA_FIX;
            ybot = ytop + dy;
            frac_right = dx * dy >> (AA_FIX + 1);
            frac_right += dx * (y2 - ybot) >> AA_FIX;
            if (xpixel - xright) {
                frac_right += dy * (xpixel - xright) >> AA_FIX;
                frac_right += (y2 - ybot) * (xpixel - xright) >> AA_FIX;
            }
            frac_left = dx * dy >> (AA_FIX + 1);
            frac_left += dx * (ytop - y1) >> AA_FIX;
            frac_left += dy * (xleft & AA_PMN) >> AA_FIX;
            frac_left += (ytop - y1) * (xleft & AA_PMN) >> AA_FIX;
            AA_pixel_lhs(xleft >> AA_FIX, y1 >> AA_FIX, frac_left, frac_right);
            if (xleft <= x2)
                return;
            xright = xleft;
            ytop = ybot;
        }
    }
}

// Walks a right-hand edge from (px1,py1) to (px2,py2) in sub-pixel space,
// slicing it into scanline-height segments and calling AA_span_rhs for each.
// uc_orig: AA_line_rhs (fallen/DDEngine/Source/aa.cpp)
static void AA_line_rhs(SLONG px1, SLONG py1, SLONG px2, SLONG py2)
{
    if (py1 == py2)
        return;
    SLONG dy, dx, dydx, dxdy, x1, x2, y1, y2;
    dx = px2 - px1;
    dy = py2 - py1;
    dxdy = (dx << AA_FIX) / dy;
    if (dx) {
        dydx = dy << AA_FIX;
        dydx /= dx;
    } else {
        dydx = 0;
    }
    x1 = px1;
    y1 = py1;
    while (1) {
        y2 = y1 + AA_PIX;
        y2 &= ~AA_PMN;
        if (y2 > py2)
            y2 = py2;
        dy = y2 - y1;
        if (dy != 0) {
            x2 = x1 + (dy * dxdy >> AA_FIX);
            AA_span_rhs(dydx, x1, y1, x2, y2);
        }
        if (y2 >= py2)
            return;
        x1 = x2;
        y1 = y2;
    }
}

// Walks a left-hand edge from (px1,py1) to (px2,py2) in sub-pixel space,
// slicing it into scanline-height segments and calling AA_span_lhs for each.
// uc_orig: AA_line_lhs (fallen/DDEngine/Source/aa.cpp)
static void AA_line_lhs(SLONG px1, SLONG py1, SLONG px2, SLONG py2)
{
    if (py1 == py2)
        return;
    SLONG dy, dx, dydx, dxdy, x1, x2, y1, y2;
    dx = px2 - px1;
    dy = py2 - py1;
    dxdy = (dx << AA_FIX) / dy;
    if (dx) {
        dydx = dy << AA_FIX;
        dydx /= dx;
    } else {
        dydx = 0;
    }
    x1 = px1;
    y1 = py1;
    while (1) {
        y2 = y1 + AA_PIX;
        y2 &= ~AA_PMN;
        if (y2 > py2)
            y2 = py2;
        dy = y2 - y1;
        if (dy != 0) {
            x2 = x1 + (dy * dxdy >> AA_FIX);
            AA_span_lhs(dydx, x1, y1, x2, y2);
        }
        if (y2 >= py2)
            return;
        y1 = y2;
        x1 = x2;
    }
}

// Renders a single monotone (already sorted top-to-bottom) triangle into the bitmap.
// Classifies each edge as rhs or lhs based on winding, then composites the coverage buffer
// scanline by scanline: lhs pixels, full-fill interior pixels, rhs pixels.
// uc_orig: AA_draw_do (fallen/DDEngine/Source/aa.cpp)
static void AA_draw_do(UBYTE* bitmap, UBYTE x_res, UBYTE y_res, SLONG pitch, SLONG px[3], SLONG py[3])
{
    SLONG i, miny, maxy, p1, p2;
    SLONG x, y, x1, y1, x2, y2;
    SLONG nextp[3] = { 1, 2, 0 };
    SLONG val, fill_top, fill_bot, fill;
    UBYTE* line;

    miny = py[0];
    maxy = py[0];
    if (py[1] < miny)
        miny = py[1];
    if (py[1] > maxy)
        maxy = py[1];
    if (py[2] < miny)
        miny = py[2];
    if (py[2] > maxy)
        maxy = py[2];

    fill_top = AA_PIX - (miny & AA_PMN);
    fill_bot = ((maxy - 1) & AA_PMN);
    miny >>= AA_FIX;
    maxy -= 1;
    maxy >>= AA_FIX;

    for (i = miny; i <= maxy; i++) {
        AA_span[i].lhs_min = +UC_INFINITY;
        AA_span[i].lhs_max = -UC_INFINITY;
        AA_span[i].rhs_min = +UC_INFINITY;
        AA_span[i].rhs_max = -UC_INFINITY;
    }

    p1 = 0;
    for (i = 0; i < 3; i++) {
        p2 = nextp[p1];
        x1 = px[p1];
        y1 = py[p1];
        x2 = px[p2];
        y2 = py[p2];
        if (y1 < y2)
            AA_line_rhs(x1, y1, x2, y2);
        else
            AA_line_lhs(x2, y2, x1, y1);
        p1 = p2;
    }

    fill = fill_top;
    for (y = miny; y <= maxy; y++) {
        if (y == maxy)
            fill = fill_bot;
        AA_Span* as = &AA_span[y];
        line = &bitmap[y * pitch];
        x = as->lhs_min;
        while (x <= as->lhs_max) {
            val = line[x];
            val += as->pixel[x];
            SATURATE(val, 0, 255);
            line[x] = (UBYTE)val;
            x += 1;
        }
        while (x < as->rhs_min) {
            val = line[x];
            val += fill;
            SATURATE(val, 0, 255);
            line[x] = (UBYTE)val;
            x += 1;
        }
        while (x <= as->rhs_max) {
            val = line[x];
            val += as->pixel[x];
            SATURATE(val, 0, 255);
            line[x] = (UBYTE)val;
            x += 1;
        }
        fill = 255;
    }
}

// Converts 16-bit fixed-point input coordinates to internal AA fixed-point,
// sorts vertices by Y, splits non-monotone triangles at the middle vertex,
// and dispatches to AA_draw_do for each monotone sub-triangle.
// uc_orig: AA_draw (fallen/DDEngine/Source/aa.cpp)
void AA_draw(UBYTE* bitmap, UBYTE x_res, UBYTE y_res, SLONG pitch,
    SLONG p1x, SLONG p1y, SLONG p2x, SLONG p2y, SLONG p3x, SLONG p3y)
{
    SLONG minp, p1, p2, p3;
    SLONG px[3], py[3];
    SLONG nextp[3] = { 1, 2, 0 };

    px[0] = p1x >> (16 - AA_FIX);
    py[0] = p1y >> (16 - AA_FIX);
    px[1] = p2x >> (16 - AA_FIX);
    py[1] = p2y >> (16 - AA_FIX);
    px[2] = p3x >> (16 - AA_FIX);
    py[2] = p3y >> (16 - AA_FIX);

    px[0] &= ~0xf;
    py[0] &= ~0xf;
    px[1] &= ~0xf;
    py[1] &= ~0xf;
    px[2] &= ~0xf;
    py[2] &= ~0xf;

    if (px[0] == AA_MAX_SPAN_X << AA_FIX)
        px[0] -= 1;
    if (px[1] == AA_MAX_SPAN_X << AA_FIX)
        px[1] -= 1;
    if (px[2] == AA_MAX_SPAN_X << AA_FIX)
        px[2] -= 1;
    if (py[0] == AA_MAX_SPAN_Y << AA_FIX)
        py[0] -= 1;
    if (py[1] == AA_MAX_SPAN_Y << AA_FIX)
        py[1] -= 1;
    if (py[2] == AA_MAX_SPAN_Y << AA_FIX)
        py[2] -= 1;

    minp = 0;
    if (py[1] < py[0])
        minp = 1;
    if (py[2] < py[minp])
        minp = 2;

    p1 = minp;
    p2 = nextp[p1];
    p3 = nextp[p2];

    if (py[0] == py[1] || py[1] == py[2] || py[2] == py[0]) {
        SLONG tx[3], ty[3];
        tx[0] = px[p1];
        ty[0] = py[p1];
        tx[1] = px[p2];
        ty[1] = py[p2];
        tx[2] = px[p3];
        ty[2] = py[p3];
        AA_draw_do(bitmap, x_res, y_res, pitch, tx, ty);
    } else {
        if (py[p2] > py[p3]) {
            SLONG dx = px[p2] - px[p1], dy = py[p2] - py[p1];
            SLONG nx = px[p1] + (dx * (py[p3] - py[p1])) / dy, ny = py[p3];
            SLONG tx[3], ty[3];
            tx[0] = px[p1];
            ty[0] = py[p1];
            tx[1] = nx;
            ty[1] = ny;
            tx[2] = px[p3];
            ty[2] = py[p3];
            AA_draw_do(bitmap, x_res, y_res, pitch, tx, ty);
            tx[0] = px[p3];
            ty[0] = py[p3];
            tx[1] = nx;
            ty[1] = ny;
            tx[2] = px[p2];
            ty[2] = py[p2];
            AA_draw_do(bitmap, x_res, y_res, pitch, tx, ty);
        } else {
            SLONG dx = px[p3] - px[p1], dy = py[p3] - py[p1];
            SLONG nx = px[p1] + (dx * (py[p2] - py[p1])) / dy, ny = py[p2];
            SLONG tx[3], ty[3];
            tx[0] = px[p1];
            ty[0] = py[p1];
            tx[1] = px[p2];
            ty[1] = py[p2];
            tx[2] = nx;
            ty[2] = ny;
            AA_draw_do(bitmap, x_res, y_res, pitch, tx, ty);
            tx[0] = nx;
            ty[0] = ny;
            tx[1] = px[p2];
            ty[1] = py[p2];
            tx[2] = px[p3];
            ty[2] = py[p3];
            AA_draw_do(bitmap, x_res, y_res, pitch, tx, ty);
        }
    }
}
