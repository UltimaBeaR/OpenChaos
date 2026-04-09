// uc_orig: drawxtra.cpp (fallen/DDEngine/Source/drawxtra.cpp)
// 2D screen-space drawing utilities used by the UI widget system.


#include "engine/graphics/pipeline/poly.h"

#include "engine/graphics/pipeline/draw2d.h"

// uc_orig: DRAW2D_Box (fallen/DDEngine/Source/drawxtra.cpp)
void DRAW2D_Box(SLONG x, SLONG y, SLONG ox, SLONG oy, SLONG rgb, UBYTE flag, UBYTE depth)
{
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
    SLONG page = flag ? POLY_PAGE_SUBTRACTIVEALPHA : POLY_PAGE_ADDITIVEALPHA;
    float fdepth = (float)depth;

    pp[0].colour = rgb;
    pp[0].specular = 0xff000000; // alpha=1.0: no CPU fog, use table fog
    pp[0].Z = fdepth / 256.0f;

    pp[1] = pp[2] = pp[3] = pp[0];

    pp[0].X = x;
    pp[0].Y = y;
    pp[1].X = ox;
    pp[1].Y = y;
    pp[2].X = x;
    pp[2].Y = oy;
    pp[3].X = ox;
    pp[3].Y = oy;

    POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
}

// uc_orig: DRAW2D_Box_Page (fallen/DDEngine/Source/drawxtra.cpp)
void DRAW2D_Box_Page(SLONG x, SLONG y, SLONG ox, SLONG oy, SLONG rgb, SLONG page, UBYTE depth)
{
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
    float fdepth = (float)depth;

    pp[0].colour = rgb;
    pp[0].specular = 0xff000000; // alpha=1.0: no CPU fog, use table fog
    pp[0].Z = fdepth / 256.0f;

    pp[1] = pp[2] = pp[3] = pp[0];

    pp[0].X = x;
    pp[0].Y = y;
    pp[1].X = ox;
    pp[1].Y = y;
    pp[2].X = x;
    pp[2].Y = oy;
    pp[3].X = ox;
    pp[3].Y = oy;

    POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
}

// uc_orig: DRAW2D_Tri (fallen/DDEngine/Source/drawxtra.cpp)
void DRAW2D_Tri(SLONG x, SLONG y, SLONG ox, SLONG oy, SLONG tx, SLONG ty, SLONG rgb, UBYTE flag)
{
    POLY_Point pp[3];
    POLY_Point* tri[3] = { &pp[0], &pp[1], &pp[2] };
    SLONG page = flag ? POLY_PAGE_SUBTRACTIVEALPHA : POLY_PAGE_ADDITIVEALPHA;

    pp[0].colour = rgb;
    pp[0].specular = 0xff000000; // alpha=1.0: no CPU fog, use table fog
    pp[0].Z = 0.5f;

    pp[1] = pp[2] = pp[0];

    pp[0].X = x;
    pp[0].Y = y;
    pp[1].X = ox;
    pp[1].Y = oy;
    pp[2].X = tx;
    pp[2].Y = ty;

    POLY_add_triangle(tri, page, UC_FALSE, UC_TRUE);
}

// uc_orig: DRAW2D_Sprite (fallen/DDEngine/Source/drawxtra.cpp)
void DRAW2D_Sprite(SLONG x, SLONG y, SLONG ox, SLONG oy, float u, float v, float ou, float ov, SLONG page, SLONG rgb)
{
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };

    pp[0].colour = rgb;
    pp[0].specular = 0xff000000; // alpha=1.0: no CPU fog, use table fog
    pp[0].Z = 0.5f;

    pp[1] = pp[2] = pp[3] = pp[0];

    pp[0].X = x;
    pp[0].Y = y;
    pp[0].u = u;
    pp[0].v = v;
    pp[1].X = ox;
    pp[1].Y = y;
    pp[1].u = ou;
    pp[1].v = v;
    pp[2].X = x;
    pp[2].Y = oy;
    pp[2].u = u;
    pp[2].v = ov;
    pp[3].X = ox;
    pp[3].Y = oy;
    pp[3].u = ou;
    pp[3].v = ov;

    POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
}
