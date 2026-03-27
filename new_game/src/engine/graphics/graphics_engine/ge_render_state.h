#ifndef ENGINE_GRAPHICS_GRAPHICS_ENGINE_GE_RENDER_STATE_H
#define ENGINE_GRAPHICS_GRAPHICS_ENGINE_GE_RENDER_STATE_H

// API-agnostic render state cache.
// Batches state changes and flushes only deltas to the graphics engine.
// Replaces the D3D-specific RenderState class from the original code.

#include "engine/graphics/graphics_engine/graphics_engine.h"

// Special per-polygon rendering effects applied during vertex massaging.
// uc_orig: SpecialEffect (fallen/DDEngine/Headers/renderstate.h)
enum GERenderEffect {
    GE_EFFECT_NONE,            // no special effect
    GE_EFFECT_ALPHA_PREMULT,   // premultiply vertex colours by alpha, set alpha to 0
    GE_EFFECT_BLACK_WITH_ALPHA,// set vertex R,G,B to (0,0,0)
    GE_EFFECT_INV_ALPHA_PREMULT, // premultiply vertex colours by 1-alpha, set alpha to 0
    GE_EFFECT_DECAL_MODE       // set vertex A,R,G,B to (255,255,255,255)
};

// Legacy aliases for SpecialEffect names used throughout the codebase.
#define RS_None           GE_EFFECT_NONE
#define RS_AlphaPremult   GE_EFFECT_ALPHA_PREMULT
#define RS_BlackWithAlpha GE_EFFECT_BLACK_WITH_ALPHA
#define RS_InvAlphaPremult GE_EFFECT_INV_ALPHA_PREMULT
#define RS_DecalMode      GE_EFFECT_DECAL_MODE

#pragma pack(push, 4)
struct GERenderState {
    GERenderState(GETextureFilter mag = GETextureFilter::Linear,
                  GETextureFilter min = GETextureFilter::Nearest);

    // Typed setters (replace generic SetRenderState(DWORD, DWORD))
    void SetTexture(GETextureHandle texture);
    void SetTextureAddress(GETextureAddress addr);
    void SetTextureFilter(GETextureFilter mag, GETextureFilter min);
    void SetTextureMag(GETextureFilter mag);
    void SetTextureMin(GETextureFilter min);
    void SetTextureBlend(GETextureBlend blend);
    void SetBlendMode(GEBlendMode mode);
    void SetSrcBlend(GEBlendFactor src);
    void SetDstBlend(GEBlendFactor dst);
    void SetAlphaBlendEnabled(bool enabled);
    void SetDepthEnabled(bool enabled);
    void SetDepthWrite(bool enabled);
    void SetDepthFunc(GECompareFunc func);
    void SetDepthBias(int32_t bias);
    void SetFogEnabled(bool enabled);
    void SetAlphaTestEnabled(bool enabled);
    void SetColorKeyEnabled(bool enabled);
    void SetCullMode(GECullMode mode);
    void SetEffect(GERenderEffect effect);

    void SetTempTransparent();
    void ResetTempTransparent();

    GETextureHandle GetTexture() { return Texture; }
    GERenderEffect GetEffect() { return Effect; }
    bool NeedsSorting() { return !DepthWrite; }
    int32_t ZLift() { return DepthBias; }
    void WrapJustOnce() { WrapOnce = true; }
    bool IsAlphaBlendEnabled() { return AlphaBlendEnabled; }

    bool IsSameRenderState(GERenderState* pRS);

    // Flush all states to ge_* (full).
    void InitScene(uint32_t fog_colour);
    // Flush only changed states to ge_* (delta).
    void SetChanged();

    static GERenderState s_State;

private:
    GETextureHandle Texture;
    GETextureAddress TexAddress;
    GETextureFilter TexMag;
    GETextureFilter TexMin;
    GETextureBlend TexBlend;

    bool DepthEnabled;
    bool DepthWrite;
    GECompareFunc DepthFunc;
    bool AlphaTestEnabled;
    GEBlendFactor SrcBlend;
    GEBlendFactor DstBlend;
    bool AlphaBlendEnabled;
    bool FogEnabled;
    bool ColorKeyEnabled;
    GECullMode Cull;
    int32_t DepthBias;
    GERenderEffect Effect;
    bool WrapOnce;

    bool TempTransparent;
    GEBlendFactor TempSrcBlend;
    GEBlendFactor TempDstBlend;
    bool TempAlphaBlendEnabled;
    bool TempDepthWrite;
    GETextureBlend TempTexBlend;
    GERenderEffect TempEffect;
};
#pragma pack(pop)

// Legacy alias — old code uses "RenderState", new code should prefer "GERenderState".
using RenderState = GERenderState;
// Legacy alias — old code uses "SpecialEffect".
using SpecialEffect = GERenderEffect;

#endif // ENGINE_GRAPHICS_GRAPHICS_ENGINE_GE_RENDER_STATE_H
