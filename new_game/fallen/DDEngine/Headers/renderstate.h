// renderstate.h
//
// render state class; encapsulates a state of the renderer

#ifndef FALLEN_DDENGINE_HEADERS_RENDERSTATE_H
#define FALLEN_DDENGINE_HEADERS_RENDERSTATE_H

enum SpecialEffect {
    RS_None, // no special effect
    RS_AlphaPremult, // premultiply vertex colours by alpha and set alpha to 0
    RS_BlackWithAlpha, // set vertex R,G,B to (0,0,0)
    RS_InvAlphaPremult, // premultiply vertex colours by 1-alpha and set alpha to 0
    RS_DecalMode // set vertex A,R,G,B to (255,255,255,255)
};

#pragma pack(push, 4)
struct RenderState {
    RenderState(DWORD mag_filter = D3DFILTER_LINEAR, DWORD min_filter = D3DFILTER_NEAREST);

    void SetTexture(LPDIRECT3DTEXTURE2 texture);
    void SetRenderState(DWORD index, DWORD value);
    void SetEffect(DWORD effect);

    void SetTempTransparent();
    void ResetTempTransparent();

    LPDIRECT3DTEXTURE2 GetTexture() { return TextureMap; }
    DWORD GetEffect() { return Effect; }

    void InitScene(DWORD fog_colour);

    void SetChanged(); // set changed members

    bool NeedsSorting() { return !ZWriteEnable; }
    DWORD ZLift() { return ZBias; }

    void WrapJustOnce() { WrapOnce = true; } // Temporarily sets the state to wrapping just for one call to SetChanged()


    bool IsSameRenderState(RenderState* pRS);

    static RenderState s_State;


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

#endif // FALLEN_DDENGINE_HEADERS_RENDERSTATE_H
