#ifndef ENGINE_GRAPHICS_PIPELINE_POLYPOINT_H
#define ENGINE_GRAPHICS_PIPELINE_POLYPOINT_H

#include "engine/core/types.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"

// Copy U/V fields as raw int bits, because U/V may be uninitialized in many
// callers and reading them into the FPU (for a float copy) could trap.
// uc_orig: INT_COPY_FLOAT (fallen/DDEngine/Headers/polypoint.h)
#define INT_COPY_FLOAT(DST, SRC) *((int*)&(DST)) = *((int*)&(SRC))

// uc_orig: PolyPoint2D (fallen/DDEngine/Headers/polypoint.h)
// Wraps GEVertexTL with convenient helpers for setting screen coords,
// UV coordinates, and colour values. Private base prevents accidental
// GEVertexTL* casts from outside this class.
class PolyPoint2D : private GEVertexTL {
public:
    // uc_orig: SetSC (fallen/DDEngine/Headers/polypoint.h)
    void SetSC(float sx, float sy, float sz = 0);
    // uc_orig: SetUV2 (fallen/DDEngine/Headers/polypoint.h)
    void SetUV2(float u, float v);
    // uc_orig: MakeColour (fallen/DDEngine/Headers/polypoint.h)
    static ULONG MakeColour(UBYTE r, UBYTE g, UBYTE b, UBYTE a);
    // uc_orig: ModulateColours (fallen/DDEngine/Headers/polypoint.h)
    static ULONG ModulateColours(ULONG c1, ULONG c2);
    // uc_orig: SetColour (fallen/DDEngine/Headers/polypoint.h)
    void SetColour(ULONG col);
    // uc_orig: SetColour (fallen/DDEngine/Headers/polypoint.h)
    void SetColour(UBYTE r, UBYTE g, UBYTE b, UBYTE a = 0xFF);
    // uc_orig: SetSpecular (fallen/DDEngine/Headers/polypoint.h)
    void SetSpecular(ULONG col);
    // uc_orig: SetSpecular (fallen/DDEngine/Headers/polypoint.h)
    void SetSpecular(UBYTE r, UBYTE g, UBYTE b, UBYTE a = 0xFF);
};

// uc_orig: SetSC (fallen/DDEngine/Headers/polypoint.h)
//
// PIXEL_HALF_OFFSET cancels the -0.5 that tl_vert.glsl applies to every
// incoming TL vertex (D3D6 pixel-center compatibility). Adding it here
// at the single chokepoint that every POLY-path vertex passes through
// (AddFan, POLY_add_triangle_fast, POLY_add_quad_fast, the nearclipped
// fan paths in poly.cpp — all call SetSC) gives every 2D / 3D draw a
// net-zero shift in framebuffer pixel space. Closes the universal
// 1-px gap on the right and bottom edges of fullscreen rects (pause
// menu darken, debug panel backdrop, any AENG_draw_rect that fills the
// screen) and lines pixel centres on texel centres for textured quads.
//
// Direct-TL paths that bypass this class (font_atlas, sky stars,
// gl_blit_textured_quad, ge_blit_to_framed_area) still apply their own
// +0.5 manually because they don't go through SetSC.
inline void PolyPoint2D::SetSC(float _sx, float _sy, float _sz)
{
    constexpr float PIXEL_HALF_OFFSET = 0.5f;
    sx = _sx + PIXEL_HALF_OFFSET;
    sy = _sy + PIXEL_HALF_OFFSET;
    sz = _sz;
    rhw = 1.0F - _sz;
}

// uc_orig: SetUV2 (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetUV2(float u, float v)
{
    INT_COPY_FLOAT(tu, u);
    INT_COPY_FLOAT(tv, v);
}

// uc_orig: MakeColour (fallen/DDEngine/Headers/polypoint.h)
inline ULONG PolyPoint2D::MakeColour(UBYTE r, UBYTE g, UBYTE b, UBYTE a)
{
    return (ULONG(a) << 24) | (ULONG(r) << 16) | (ULONG(g) << 8) | ULONG(b);
}

// uc_orig: SetColour (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetColour(ULONG col)
{
    color = col;
}

// uc_orig: SetColour (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetColour(UBYTE r, UBYTE g, UBYTE b, UBYTE a)
{
    SetColour(MakeColour(r, g, b, a));
}

// uc_orig: SetSpecular (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetSpecular(ULONG col)
{
    specular = col;
}

// uc_orig: SetSpecular (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetSpecular(UBYTE r, UBYTE g, UBYTE b, UBYTE a)
{
    SetSpecular(MakeColour(r, g, b, a));
}

// Modulate two D3D colours. Does not round correctly, does not modulate alpha.
// uc_orig: ModulateColours (fallen/DDEngine/Headers/polypoint.h)
inline ULONG PolyPoint2D::ModulateColours(ULONG c1, ULONG c2)
{
    ULONG res;

    res = ((c1 & 0xFF) * (c2 & 0xFF)) >> 8;
    c1 >>= 8;
    c2 >>= 8;
    res |= ((c1 & 0xFF) * (c2 & 0xFF)) & 0xFF00;
    c1 >>= 8;
    c2 >>= 8;
    res |= (((c1 & 0xFF) * (c2 & 0xFF)) & 0xFF00) << 8;
    res |= (c1 & 0xFF00) << 16;

    return res;
}

#endif // ENGINE_GRAPHICS_PIPELINE_POLYPOINT_H
