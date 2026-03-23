#ifndef ENGINE_GRAPHICS_PIPELINE_RENDER_STATE_H
#define ENGINE_GRAPHICS_PIPELINE_RENDER_STATE_H

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>

// Special per-polygon rendering effects applied during vertex massaging.
// uc_orig: SpecialEffect (fallen/DDEngine/Headers/renderstate.h)
enum SpecialEffect {
    RS_None,           // no special effect
    RS_AlphaPremult,   // premultiply vertex colours by alpha, set alpha to 0
    RS_BlackWithAlpha, // set vertex R,G,B to (0,0,0)
    RS_InvAlphaPremult, // premultiply vertex colours by 1-alpha, set alpha to 0
    RS_DecalMode       // set vertex A,R,G,B to (255,255,255,255)
};

// Encapsulates the full D3D render state: texture, blending, Z, fog, etc.
// Tracks the "current card state" in a static member to avoid redundant SetRenderState calls.
// uc_orig: RenderState (fallen/DDEngine/Headers/renderstate.h)
#pragma pack(push, 4)
struct RenderState {
    // uc_orig: RenderState (fallen/DDEngine/Headers/renderstate.h)
    RenderState(DWORD mag_filter = D3DFILTER_LINEAR, DWORD min_filter = D3DFILTER_NEAREST);

    // uc_orig: RenderState::SetTexture (fallen/DDEngine/Headers/renderstate.h)
    void SetTexture(LPDIRECT3DTEXTURE2 texture);

    // uc_orig: RenderState::SetRenderState (fallen/DDEngine/Headers/renderstate.h)
    void SetRenderState(DWORD index, DWORD value);

    // uc_orig: RenderState::SetEffect (fallen/DDEngine/Headers/renderstate.h)
    void SetEffect(DWORD effect);

    // uc_orig: RenderState::SetTempTransparent (fallen/DDEngine/Headers/renderstate.h)
    void SetTempTransparent();

    // uc_orig: RenderState::ResetTempTransparent (fallen/DDEngine/Headers/renderstate.h)
    void ResetTempTransparent();

    // uc_orig: RenderState::GetTexture (fallen/DDEngine/Headers/renderstate.h)
    LPDIRECT3DTEXTURE2 GetTexture() { return TextureMap; }

    // uc_orig: RenderState::GetEffect (fallen/DDEngine/Headers/renderstate.h)
    DWORD GetEffect() { return Effect; }

    // uc_orig: RenderState::InitScene (fallen/DDEngine/Headers/renderstate.h)
    void InitScene(DWORD fog_colour);

    // uc_orig: RenderState::SetChanged (fallen/DDEngine/Headers/renderstate.h)
    void SetChanged();

    // uc_orig: RenderState::NeedsSorting (fallen/DDEngine/Headers/renderstate.h)
    bool NeedsSorting() { return !ZWriteEnable; }

    // uc_orig: RenderState::ZLift (fallen/DDEngine/Headers/renderstate.h)
    DWORD ZLift() { return ZBias; }

    // uc_orig: RenderState::WrapJustOnce (fallen/DDEngine/Headers/renderstate.h)
    void WrapJustOnce() { WrapOnce = true; }

    // uc_orig: RenderState::IsSameRenderState (fallen/DDEngine/Headers/renderstate.h)
    bool IsSameRenderState(RenderState* pRS);

    // uc_orig: RenderState::s_State (fallen/DDEngine/Source/renderstate.cpp)
    static RenderState s_State;

    // uc_orig: RenderState::IsAlphaBlendEnabled (fallen/DDEngine/Headers/renderstate.h)
    bool IsAlphaBlendEnabled() { return (AlphaBlendEnable != FALSE); }

private:
    LPDIRECT3DTEXTURE2 TextureMap;
    DWORD TextureAddress;
    DWORD TextureMag;
    DWORD TextureMin;
    DWORD TextureMapBlend;

    DWORD ZEnable;
    DWORD ZWriteEnable;
    DWORD AlphaTestEnable;
    DWORD SrcBlend;
    DWORD DestBlend;
    DWORD ZFunc;
    DWORD AlphaBlendEnable;
    DWORD FogEnable;
    DWORD ColorKeyEnable;
    DWORD CullMode;
    DWORD ZBias;
    DWORD Effect;
    bool WrapOnce;

    bool TempTransparent;
    DWORD TempSrcBlend;
    DWORD TempDestBlend;
    DWORD TempAlphaBlendEnable;
    DWORD TempZWriteEnable;
    DWORD TempTextureMapBlend;
    DWORD TempEffect;
};
#pragma pack(pop)

#endif // ENGINE_GRAPHICS_PIPELINE_RENDER_STATE_H
