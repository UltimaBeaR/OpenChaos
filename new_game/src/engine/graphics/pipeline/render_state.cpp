#include "engine/graphics/pipeline/render_state.h"
#include "engine/platform/uc_common.h"
#include "engine/graphics/graphics_api/display_macros.h" // REALLY_SET_*, the_display

// uc_orig: RenderState::s_State (fallen/DDEngine/Source/renderstate.cpp)
RenderState RenderState::s_State;

// Cast float bits to DWORD without conversion — used for D3D fog table states.
// uc_orig: FloatAsDword (fallen/DDEngine/Source/renderstate.cpp)
static inline DWORD FloatAsDword(float fArg)
{
    return (*((DWORD*)(&fArg)));
}

// uc_orig: RenderState::RenderState (fallen/DDEngine/Source/renderstate.cpp)
RenderState::RenderState(DWORD mag_filter, DWORD min_filter)
{
    TextureMap = NULL;
    TextureAddress = D3DTADDRESS_CLAMP;
    TextureMag = mag_filter;
    TextureMin = min_filter;
    TextureMapBlend = D3DTBLEND_MODULATE;

    ZEnable = D3DZB_TRUE;
    ZWriteEnable = UC_TRUE;
    ZFunc = D3DCMP_LESSEQUAL;
    ColorKeyEnable = UC_FALSE;

    FogEnable = UC_TRUE;
    AlphaTestEnable = UC_FALSE;
    SrcBlend = D3DBLEND_ONE;
    DestBlend = D3DBLEND_ZERO;
    AlphaBlendEnable = UC_FALSE;
    CullMode = D3DCULL_NONE;

    ZBias = 0;
    WrapOnce = false;

    TempTransparent = false;
    TempSrcBlend = 0;
    TempDestBlend = 0;
    TempAlphaBlendEnable = 0;
    TempZWriteEnable = 0;
    TempTextureMapBlend = 0;

    Effect = RS_None;
}

// Saves current state and switches to alpha-blended transparent rendering.
// Has no effect if already in temp-transparent mode.
// uc_orig: RenderState::SetTempTransparent (fallen/DDEngine/Source/renderstate.cpp)
void RenderState::SetTempTransparent()
{
    if (!TempTransparent) {
        TempTransparent = true;
        TempSrcBlend = SrcBlend;
        TempDestBlend = DestBlend;
        TempAlphaBlendEnable = AlphaBlendEnable;
        TempZWriteEnable = ZWriteEnable;
        TempTextureMapBlend = TextureMapBlend;
        TempEffect = Effect;

        SrcBlend = D3DBLEND_SRCALPHA;
        DestBlend = D3DBLEND_INVSRCALPHA;
        AlphaBlendEnable = UC_TRUE;
        ZWriteEnable = UC_FALSE;
        TextureMapBlend = D3DTBLEND_MODULATEALPHA;
        Effect = RS_None;
    }
}

// Restores the state saved by SetTempTransparent.
// uc_orig: RenderState::ResetTempTransparent (fallen/DDEngine/Source/renderstate.cpp)
void RenderState::ResetTempTransparent()
{
    if (TempTransparent) {
        TempTransparent = false;
        SrcBlend = TempSrcBlend;
        DestBlend = TempDestBlend;
        AlphaBlendEnable = TempAlphaBlendEnable;
        ZWriteEnable = TempZWriteEnable;
        TextureMapBlend = TempTextureMapBlend;
        Effect = TempEffect;
    }
}

// uc_orig: RenderState::SetTexture (fallen/DDEngine/Source/renderstate.cpp)
void RenderState::SetTexture(LPDIRECT3DTEXTURE2 texture)
{
    TextureMap = texture;
}

// Mirrors D3D SetRenderState() semantics but stores values in the struct.
// Some hardware workarounds are applied (e.g. Rage Pro MODULATEALPHA issue).
// uc_orig: RenderState::SetRenderState (fallen/DDEngine/Source/renderstate.cpp)
void RenderState::SetRenderState(DWORD ix, DWORD value)
{
    if ((ix == D3DRENDERSTATE_TEXTUREMAPBLEND) && (value == D3DTBLEND_MODULATEALPHA) && !the_display.GetDeviceInfo()->ModulateAlphaSupported()) {
        value = D3DTBLEND_MODULATE;
    }

    if ((ix == D3DRENDERSTATE_TEXTUREMAPBLEND) && (value == D3DTBLEND_DECAL)) {
        if (the_display.GetDeviceInfo()->DestInvSourceColourSupported() && !the_display.GetDeviceInfo()->ModulateAlphaSupported()) {
            // Rage Pro - don't do this
        } else {
            value = D3DTBLEND_MODULATE;
            Effect = RS_DecalMode;
        }
    }

    switch (ix) {
    case D3DRENDERSTATE_TEXTUREADDRESS:
        TextureAddress = value;
        break;
    case D3DRENDERSTATE_TEXTUREMAG:
        TextureMag = value;
        break;
    case D3DRENDERSTATE_TEXTUREMIN:
        TextureMin = value;
        break;
    case D3DRENDERSTATE_TEXTUREMAPBLEND:
        TextureMapBlend = value;
        break;
    case D3DRENDERSTATE_ZENABLE:
        ZEnable = value;
        break;
    case D3DRENDERSTATE_ZWRITEENABLE:
        ZWriteEnable = value;
        break;
    case D3DRENDERSTATE_ZFUNC:
        ZFunc = value;
        break;
    case D3DRENDERSTATE_COLORKEYENABLE:
        ColorKeyEnable = value;
        break;
    case D3DRENDERSTATE_FOGENABLE:
        FogEnable = value;
        break;
    case D3DRENDERSTATE_ALPHATESTENABLE:
        AlphaTestEnable = value;
        break;
    case D3DRENDERSTATE_SRCBLEND:
        SrcBlend = value;
        break;
    case D3DRENDERSTATE_DESTBLEND:
        DestBlend = value;
        break;
    case D3DRENDERSTATE_ALPHABLENDENABLE:
        AlphaBlendEnable = value;
        break;
    case D3DRENDERSTATE_ZBIAS:
        ZBias = value;
        break;
    case D3DRENDERSTATE_CULLMODE:
        CullMode = value;
        break;
    default:
        ASSERT(0);
    }
}

// Applies a special vertex effect — adjusts blend/texture modes accordingly.
// uc_orig: RenderState::SetEffect (fallen/DDEngine/Source/renderstate.cpp)
void RenderState::SetEffect(DWORD effect)
{
    Effect = effect;

    switch (effect) {
    case RS_AlphaPremult:
    case RS_InvAlphaPremult:
        SrcBlend = D3DBLEND_ONE;
        DestBlend = D3DBLEND_ONE;
        TextureMapBlend = D3DTBLEND_MODULATE;
        break;

    case RS_BlackWithAlpha:
        SrcBlend = D3DBLEND_SRCALPHA;
        DestBlend = D3DBLEND_INVSRCALPHA;
        TextureMapBlend = D3DTBLEND_MODULATEALPHA;
        break;

    case RS_DecalMode:
        TextureMapBlend = D3DTBLEND_MODULATE;
        break;
    }
}

// Sends all render states to the card and saves them in s_State.
// Call once per frame before drawing; SetChanged() is used for subsequent updates.
// uc_orig: RenderState::InitScene (fallen/DDEngine/Source/renderstate.cpp)
void RenderState::InitScene(DWORD fog_colour)
{
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_CULLMODE, CullMode);

    // Use > 0x07 threshold: more robust than != 0, works on Permedia 2, looks better with interpolated alpha.
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHAREF, 0x07);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER);

    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SPECULARENABLE, UC_TRUE);

    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGCOLOR, fog_colour);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);

    extern SLONG CurDrawDistance;
    float fFogDist = CurDrawDistance * (60.0f / (22.f * 256.0f));
    float fFogDistNear = fFogDist * 0.7f;
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLESTART, FloatAsDword(fFogDistNear));
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLEEND, FloatAsDword(fFogDist));
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLEDENSITY, FloatAsDword(0.5f));

    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREMAPBLEND, TextureMapBlend);
    REALLY_SET_TEXTURE(TextureMap);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREADDRESS, TextureAddress);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREMAG, TextureMag);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_TEXTUREMIN, TextureMin);

    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZENABLE, ZEnable);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZWRITEENABLE, ZWriteEnable);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZFUNC, ZFunc);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_COLORKEYENABLE, ColorKeyEnable);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGENABLE, FogEnable);

    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHATESTENABLE, AlphaTestEnable);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SRCBLEND, SrcBlend);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_DESTBLEND, DestBlend);
    REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, AlphaBlendEnable);

    // ZBias via SetRenderState doesn't work — done in PolyPage::AddFan instead.

    s_State = *this;
}

#define MAYBE_SET(CAPS, SMALL)                                 \
    if (s_State.SMALL != SMALL) {                              \
        REALLY_SET_RENDER_STATE(D3DRENDERSTATE_##CAPS, SMALL); \
        s_State.SMALL = SMALL;                                 \
    }

// Sends only the render states that differ from the last committed state.
// uc_orig: RenderState::SetChanged (fallen/DDEngine/Source/renderstate.cpp)
void RenderState::SetChanged()
{
    DWORD old_texture_address = TextureAddress;

    if (WrapOnce) {
        TextureAddress = D3DTADDRESS_WRAP;
    }

    if (s_State.TextureMap != TextureMap) {
        REALLY_SET_TEXTURE(TextureMap);
        s_State.TextureMap = TextureMap;
    }

    MAYBE_SET(TEXTUREADDRESS, TextureAddress);
    MAYBE_SET(TEXTUREMAG, TextureMag);
    MAYBE_SET(TEXTUREMIN, TextureMin);
    MAYBE_SET(TEXTUREMAPBLEND, TextureMapBlend);

    MAYBE_SET(ZENABLE, ZEnable);
    MAYBE_SET(ZWRITEENABLE, ZWriteEnable);
    MAYBE_SET(ALPHATESTENABLE, AlphaTestEnable);
    MAYBE_SET(SRCBLEND, SrcBlend);
    MAYBE_SET(DESTBLEND, DestBlend);
    MAYBE_SET(ALPHABLENDENABLE, AlphaBlendEnable);
    MAYBE_SET(ZFUNC, ZFunc);
    MAYBE_SET(CULLMODE, CullMode);
    MAYBE_SET(FOGENABLE, FogEnable);
    MAYBE_SET(COLORKEYENABLE, ColorKeyEnable);

    if (WrapOnce) {
        TextureAddress = old_texture_address;
        WrapOnce = false;
    }
}

#undef MAYBE_SET

// Returns true if all render state fields are identical (texture + all D3D states).
// uc_orig: RenderState::IsSameRenderState (fallen/DDEngine/Source/renderstate.cpp)
bool RenderState::IsSameRenderState(RenderState* pRS)
{
    bool bRes = UC_TRUE;

#define CHECK_RS(name)       \
    if (name != pRS->name) { \
        bRes = UC_FALSE;        \
    }

    CHECK_RS(TextureMap);
    CHECK_RS(TextureAddress);
    CHECK_RS(TextureMag);
    CHECK_RS(TextureMin);
    CHECK_RS(TextureMapBlend);

    CHECK_RS(ZEnable);
    CHECK_RS(ZWriteEnable);
    CHECK_RS(AlphaTestEnable);
    CHECK_RS(SrcBlend);
    CHECK_RS(DestBlend);
    CHECK_RS(ZFunc);
    CHECK_RS(AlphaBlendEnable);
    CHECK_RS(FogEnable);
    CHECK_RS(ColorKeyEnable);

    CHECK_RS(CullMode);
    CHECK_RS(ZBias);
    CHECK_RS(Effect);

#undef CHECK_RS

    return (bRes);
}
