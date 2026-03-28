// Graphics engine — API-agnostic parts (render state cache, etc.).
// Backend-specific code lives in d3d/ (or future opengl/).

#include "engine/graphics/graphics_engine/game_graphics_engine.h"

GERenderState GERenderState::s_State;

// Capability queries — for now, hardcoded. TODO: expose through ge_* if needed.
static bool s_modulateAlphaSupported = true;
static bool s_destInvSourceColourSupported = false;

GERenderState::GERenderState(GETextureFilter mag, GETextureFilter min)
{
    Texture = GE_TEXTURE_NONE;
    TexAddress = GETextureAddress::Clamp;
    TexMag = mag;
    TexMin = min;
    TexBlend = GETextureBlend::Modulate;

    DepthEnabled = true;
    DepthWrite = true;
    DepthFunc = GECompareFunc::LessEqual;
    ColorKeyEnabled = false;

    FogEnabled = true;
    AlphaTestEnabled = false;
    SrcBlend = GEBlendFactor::One;
    DstBlend = GEBlendFactor::Zero;
    AlphaBlendEnabled = false;
    Cull = GECullMode::None;

    DepthBias = 0;
    WrapOnce = false;
    TempTransparent = false;
    TempSrcBlend = GEBlendFactor::One;
    TempDstBlend = GEBlendFactor::Zero;
    TempAlphaBlendEnabled = false;
    TempDepthWrite = true;
    TempTexBlend = GETextureBlend::Modulate;
    TempEffect = GE_EFFECT_NONE;

    Effect = GE_EFFECT_NONE;
}

void GERenderState::SetTexture(GETextureHandle texture) { Texture = texture; }
void GERenderState::SetTextureAddress(GETextureAddress addr) { TexAddress = addr; }
void GERenderState::SetTextureFilter(GETextureFilter mag, GETextureFilter min) { TexMag = mag; TexMin = min; }
void GERenderState::SetTextureMag(GETextureFilter mag) { TexMag = mag; }
void GERenderState::SetTextureMin(GETextureFilter min) { TexMin = min; }

void GERenderState::SetTextureBlend(GETextureBlend blend)
{
    if (blend == GETextureBlend::ModulateAlpha && !s_modulateAlphaSupported) {
        blend = GETextureBlend::Modulate;
    }
    if (blend == GETextureBlend::Decal) {
        if (!s_destInvSourceColourSupported || s_modulateAlphaSupported) {
            blend = GETextureBlend::Modulate;
            Effect = GE_EFFECT_DECAL_MODE;
        }
    }
    TexBlend = blend;
}

void GERenderState::SetBlendMode(GEBlendMode mode)
{
    switch (mode) {
    case GEBlendMode::Opaque:
        SrcBlend = GEBlendFactor::One;
        DstBlend = GEBlendFactor::Zero;
        AlphaBlendEnabled = false;
        break;
    case GEBlendMode::Alpha:
        SrcBlend = GEBlendFactor::SrcAlpha;
        DstBlend = GEBlendFactor::InvSrcAlpha;
        AlphaBlendEnabled = true;
        break;
    case GEBlendMode::Additive:
        SrcBlend = GEBlendFactor::One;
        DstBlend = GEBlendFactor::One;
        AlphaBlendEnabled = true;
        break;
    case GEBlendMode::Modulate:
        SrcBlend = GEBlendFactor::Zero;
        DstBlend = GEBlendFactor::SrcColor;
        AlphaBlendEnabled = true;
        break;
    case GEBlendMode::InvModulate:
        SrcBlend = GEBlendFactor::Zero;
        DstBlend = GEBlendFactor::InvSrcColor;
        AlphaBlendEnabled = true;
        break;
    }
}

void GERenderState::SetSrcBlend(GEBlendFactor src) { SrcBlend = src; }
void GERenderState::SetDstBlend(GEBlendFactor dst) { DstBlend = dst; }
void GERenderState::SetAlphaBlendEnabled(bool enabled) { AlphaBlendEnabled = enabled; }

void GERenderState::SetDepthEnabled(bool enabled) { DepthEnabled = enabled; }
void GERenderState::SetDepthWrite(bool enabled) { DepthWrite = enabled; }
void GERenderState::SetDepthFunc(GECompareFunc func) { DepthFunc = func; }
void GERenderState::SetDepthBias(int32_t bias) { DepthBias = bias; }
void GERenderState::SetFogEnabled(bool enabled) { FogEnabled = enabled; }
void GERenderState::SetAlphaTestEnabled(bool enabled) { AlphaTestEnabled = enabled; }
void GERenderState::SetColorKeyEnabled(bool enabled) { ColorKeyEnabled = enabled; }
void GERenderState::SetCullMode(GECullMode mode) { Cull = mode; }

void GERenderState::SetEffect(GERenderEffect effect)
{
    Effect = effect;
    switch (effect) {
    case GE_EFFECT_ALPHA_PREMULT:
    case GE_EFFECT_INV_ALPHA_PREMULT:
        SrcBlend = GEBlendFactor::One;
        DstBlend = GEBlendFactor::One;
        AlphaBlendEnabled = true;
        TexBlend = GETextureBlend::Modulate;
        break;
    case GE_EFFECT_BLACK_WITH_ALPHA:
        SrcBlend = GEBlendFactor::SrcAlpha;
        DstBlend = GEBlendFactor::InvSrcAlpha;
        AlphaBlendEnabled = true;
        TexBlend = GETextureBlend::ModulateAlpha;
        break;
    case GE_EFFECT_DECAL_MODE:
        TexBlend = GETextureBlend::Modulate;
        break;
    default:
        break;
    }
}

void GERenderState::SetTempTransparent()
{
    if (!TempTransparent) {
        TempTransparent = true;
        TempSrcBlend = SrcBlend;
        TempDstBlend = DstBlend;
        TempAlphaBlendEnabled = AlphaBlendEnabled;
        TempDepthWrite = DepthWrite;
        TempTexBlend = TexBlend;
        TempEffect = Effect;

        SrcBlend = GEBlendFactor::SrcAlpha;
        DstBlend = GEBlendFactor::InvSrcAlpha;
        AlphaBlendEnabled = true;
        DepthWrite = false;
        TexBlend = GETextureBlend::ModulateAlpha;
        Effect = GE_EFFECT_NONE;
    }
}

void GERenderState::ResetTempTransparent()
{
    if (TempTransparent) {
        TempTransparent = false;
        SrcBlend = TempSrcBlend;
        DstBlend = TempDstBlend;
        AlphaBlendEnabled = TempAlphaBlendEnabled;
        DepthWrite = TempDepthWrite;
        TexBlend = TempTexBlend;
        Effect = TempEffect;
    }
}

void GERenderState::InitScene(uint32_t fog_colour, float fog_near, float fog_far)
{
    ge_set_cull_mode(Cull);
    ge_set_specular_enabled(true);

    ge_set_fog_params(fog_colour, fog_near, fog_far);

    ge_set_texture_blend(TexBlend);
    ge_bind_texture(Texture);
    ge_set_texture_address(TexAddress);
    ge_set_texture_filter(TexMag, TexMin);

    ge_set_depth_mode(DepthEnabled
        ? (DepthWrite ? GEDepthMode::ReadWrite : GEDepthMode::ReadOnly)
        : (DepthWrite ? GEDepthMode::WriteOnly : GEDepthMode::Off));
    ge_set_depth_func(DepthFunc);

    ge_set_fog_enabled(FogEnabled);
    ge_set_color_key_enabled(ColorKeyEnabled);
    ge_set_alpha_test_enabled(AlphaTestEnabled);
    ge_set_alpha_ref(0x07);
    ge_set_alpha_func(GECompareFunc::Greater);
    if (AlphaBlendEnabled) {
        ge_set_blend_factors(SrcBlend, DstBlend);
    } else {
        ge_set_blend_enabled(false);
    }

    s_State = *this;
}

#define MAYBE_FLUSH(field, setter) \
    if (s_State.field != field) { setter; s_State.field = field; }

void GERenderState::SetChanged()
{
    GETextureAddress old_addr = TexAddress;
    if (WrapOnce) {
        TexAddress = GETextureAddress::Wrap;
    }

    if (s_State.Texture != Texture) {
        ge_bind_texture(Texture);
        s_State.Texture = Texture;
    }

    MAYBE_FLUSH(TexAddress, ge_set_texture_address(TexAddress));
    MAYBE_FLUSH(TexMag, ge_set_texture_filter(TexMag, TexMin));
    s_State.TexMin = TexMin;
    MAYBE_FLUSH(TexBlend, ge_set_texture_blend(TexBlend));

    if (s_State.DepthEnabled != DepthEnabled || s_State.DepthWrite != DepthWrite) {
        ge_set_depth_mode(DepthEnabled
            ? (DepthWrite ? GEDepthMode::ReadWrite : GEDepthMode::ReadOnly)
            : (DepthWrite ? GEDepthMode::WriteOnly : GEDepthMode::Off));
        s_State.DepthEnabled = DepthEnabled;
        s_State.DepthWrite = DepthWrite;
    }

    MAYBE_FLUSH(AlphaTestEnabled, ge_set_alpha_test_enabled(AlphaTestEnabled));
    if (s_State.SrcBlend != SrcBlend || s_State.DstBlend != DstBlend || s_State.AlphaBlendEnabled != AlphaBlendEnabled) {
        if (AlphaBlendEnabled) {
            ge_set_blend_factors(SrcBlend, DstBlend);
        } else {
            ge_set_blend_enabled(false);
        }
        s_State.SrcBlend = SrcBlend;
        s_State.DstBlend = DstBlend;
        s_State.AlphaBlendEnabled = AlphaBlendEnabled;
    }
    MAYBE_FLUSH(DepthFunc, ge_set_depth_func(DepthFunc));
    MAYBE_FLUSH(Cull, ge_set_cull_mode(Cull));
    MAYBE_FLUSH(FogEnabled, ge_set_fog_enabled(FogEnabled));
    MAYBE_FLUSH(ColorKeyEnabled, ge_set_color_key_enabled(ColorKeyEnabled));

    if (WrapOnce) {
        TexAddress = old_addr;
        WrapOnce = false;
    }
}

#undef MAYBE_FLUSH

bool GERenderState::IsSameRenderState(GERenderState* pRS)
{
    return Texture == pRS->Texture &&
        TexAddress == pRS->TexAddress &&
        TexMag == pRS->TexMag &&
        TexMin == pRS->TexMin &&
        TexBlend == pRS->TexBlend &&
        DepthEnabled == pRS->DepthEnabled &&
        DepthWrite == pRS->DepthWrite &&
        AlphaTestEnabled == pRS->AlphaTestEnabled &&
        SrcBlend == pRS->SrcBlend &&
        DstBlend == pRS->DstBlend &&
        AlphaBlendEnabled == pRS->AlphaBlendEnabled &&
        DepthFunc == pRS->DepthFunc &&
        FogEnabled == pRS->FogEnabled &&
        ColorKeyEnabled == pRS->ColorKeyEnabled &&
        Cull == pRS->Cull &&
        DepthBias == pRS->DepthBias &&
        Effect == pRS->Effect;
}
