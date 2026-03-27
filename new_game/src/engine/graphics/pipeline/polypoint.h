#ifndef ENGINE_GRAPHICS_PIPELINE_POLYPOINT_H
#define ENGINE_GRAPHICS_PIPELINE_POLYPOINT_H

#include "engine/graphics/graphics_engine/graphics_engine.h"

// Copy U/V fields as raw int bits, because U/V may be uninitialized in many
// callers and reading them into the FPU (for a float copy) could trap.
// uc_orig: INT_COPY_FLOAT (fallen/DDEngine/Headers/polypoint.h)
#define INT_COPY_FLOAT(DST, SRC) *((int*)&(DST)) = *((int*)&(SRC))

// uc_orig: PolyPoint2D (fallen/DDEngine/Headers/polypoint.h)
// Wraps GEVertexTL with convenient helpers for setting screen coords,
// UV coordinates, and colour values. Private base prevents accidental
// GEVertexTL* casts from outside this class.
class PolyPoint2D : private GEVertexTL
{
public:
    // uc_orig: SetSC (fallen/DDEngine/Headers/polypoint.h)
    void SetSC(float sx, float sy, float sz = 0);
    // uc_orig: SetUV (fallen/DDEngine/Headers/polypoint.h)
    void SetUV(float& u, float& v);
    // uc_orig: SetUV2 (fallen/DDEngine/Headers/polypoint.h)
    void SetUV2(float u, float v);
    // uc_orig: MakeD3DColour (fallen/DDEngine/Headers/polypoint.h)
    static ULONG MakeD3DColour(UBYTE r, UBYTE g, UBYTE b, UBYTE a);
    // uc_orig: ModulateD3DColours (fallen/DDEngine/Headers/polypoint.h)
    static ULONG ModulateD3DColours(ULONG c1, ULONG c2);
    // uc_orig: SetColour (fallen/DDEngine/Headers/polypoint.h)
    void SetColour(ULONG d3d_col);
    // uc_orig: SetColour (fallen/DDEngine/Headers/polypoint.h)
    void SetColour(UBYTE r, UBYTE g, UBYTE b, UBYTE a = 0xFF);
    // uc_orig: SetSpecular (fallen/DDEngine/Headers/polypoint.h)
    void SetSpecular(ULONG d3d_col);
    // uc_orig: SetSpecular (fallen/DDEngine/Headers/polypoint.h)
    void SetSpecular(UBYTE r, UBYTE g, UBYTE b, UBYTE a = 0xFF);
};

// uc_orig: SetSC (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetSC(float _sx, float _sy, float _sz)
{
    sx = _sx;
    sy = _sy;
    sz = _sz;
    rhw = 1.0F - _sz;
}

// uc_orig: SetUV (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetUV(float& u, float& v)
{
    INT_COPY_FLOAT(tu, u);
    INT_COPY_FLOAT(tv, v);
}

// uc_orig: SetUV2 (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetUV2(float u, float v)
{
    INT_COPY_FLOAT(tu, u);
    INT_COPY_FLOAT(tv, v);
}

// uc_orig: MakeD3DColour (fallen/DDEngine/Headers/polypoint.h)
inline ULONG PolyPoint2D::MakeD3DColour(UBYTE r, UBYTE g, UBYTE b, UBYTE a)
{
    return (ULONG(a) << 24) | (ULONG(r) << 16) | (ULONG(g) << 8) | ULONG(b);
}

// uc_orig: SetColour (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetColour(ULONG d3d_col)
{
    color = d3d_col;
}

// uc_orig: SetColour (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetColour(UBYTE r, UBYTE g, UBYTE b, UBYTE a)
{
    SetColour(MakeD3DColour(r, g, b, a));
}

// uc_orig: SetSpecular (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetSpecular(ULONG d3d_col)
{
    specular = d3d_col;
}

// uc_orig: SetSpecular (fallen/DDEngine/Headers/polypoint.h)
inline void PolyPoint2D::SetSpecular(UBYTE r, UBYTE g, UBYTE b, UBYTE a)
{
    SetSpecular(MakeD3DColour(r, g, b, a));
}

// Modulate two D3D colours. Does not round correctly, does not modulate alpha.
// uc_orig: ModulateD3DColours (fallen/DDEngine/Headers/polypoint.h)
inline ULONG PolyPoint2D::ModulateD3DColours(ULONG c1, ULONG c2)
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
