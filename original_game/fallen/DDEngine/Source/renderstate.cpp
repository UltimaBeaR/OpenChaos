// renderstate.cpp
//
// RenderState class

#include <MFStdLib.h>
#include <DDLib.h>
#include <math.h>
#include "poly.h"
#include "env.h"

#include "renderstate.h"

// s_State
//
// current state of the 3D card

RenderState	RenderState::s_State;

_inline DWORD FloatAsDword ( float fArg )
{
	return ( *((DWORD *)(&fArg)) );
}

// RenderState
//
// construct a default render state

RenderState::RenderState(DWORD mag_filter, DWORD min_filter)
{
	TextureMap = NULL;
	TextureAddress = D3DTADDRESS_CLAMP;
	TextureMag = mag_filter;
	TextureMin = min_filter;
	TextureMapBlend = D3DTBLEND_MODULATE;

	ZEnable = D3DZB_TRUE;
	ZWriteEnable = TRUE;
	ZFunc = D3DCMP_LESSEQUAL;
	ColorKeyEnable = FALSE;

	FogEnable = TRUE;
	AlphaTestEnable = FALSE;
	SrcBlend = D3DBLEND_ONE;
	DestBlend = D3DBLEND_ZERO;
	AlphaBlendEnable = FALSE;
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

// SetTempTransparent
//
// set to be transparent for this cycle only

#ifndef TARGET_DC
void RenderState::SetTempTransparent()
{
	if (!TempTransparent)
	{
		TempTransparent = true;
		TempSrcBlend = SrcBlend;
		TempDestBlend = DestBlend;
		TempAlphaBlendEnable = AlphaBlendEnable;
		TempZWriteEnable = ZWriteEnable;
		TempTextureMapBlend = TextureMapBlend;
		TempEffect = Effect;

		SrcBlend = D3DBLEND_SRCALPHA;
		DestBlend = D3DBLEND_INVSRCALPHA;
		AlphaBlendEnable = TRUE;
		ZWriteEnable = FALSE;
		TextureMapBlend = D3DTBLEND_MODULATEALPHA;
		Effect = RS_None;
	}
}
#endif

// ResetTempTransparent
//
// reset transparency

#ifndef TARGET_DC
void RenderState::ResetTempTransparent()
{
	if (TempTransparent)
	{
		TempTransparent = false;
		SrcBlend = TempSrcBlend;
		DestBlend = TempDestBlend;
		AlphaBlendEnable = TempAlphaBlendEnable;
		ZWriteEnable = TempZWriteEnable;
		TextureMapBlend = TempTextureMapBlend;
		Effect = TempEffect;
	}
}
#endif

// SetTexture
//
// set texture

void RenderState::SetTexture(LPDIRECT3DTEXTURE2 texture)
{
	TextureMap = texture;
}

// SetRenderState
//
// emulate SetRenderState() call

void RenderState::SetRenderState(DWORD ix, DWORD value)
{
	if ((ix == D3DRENDERSTATE_TEXTUREMAPBLEND) && (value == D3DTBLEND_MODULATEALPHA) && !the_display.GetDeviceInfo()->ModulateAlphaSupported())
	{
		value = D3DTBLEND_MODULATE;	// why doesn't DX do this?
	}

	if ((ix == D3DRENDERSTATE_TEXTUREMAPBLEND) && (value == D3DTBLEND_DECAL))
	{
		if (the_display.GetDeviceInfo()->DestInvSourceColourSupported() &&
			!the_display.GetDeviceInfo()->ModulateAlphaSupported())
		{
			// Rage Pro - don't do this
		}
		else
		{
			value = D3DTBLEND_MODULATE;
			Effect = RS_DecalMode;
		}
	}

	switch (ix)
	{
	// things we can set
	case D3DRENDERSTATE_TEXTUREADDRESS:		TextureAddress = value;		break;
	case D3DRENDERSTATE_TEXTUREMAG:			TextureMag = value;			break;
	case D3DRENDERSTATE_TEXTUREMIN:			TextureMin = value;			break;
	case D3DRENDERSTATE_TEXTUREMAPBLEND:	TextureMapBlend = value;	break;
	case D3DRENDERSTATE_ZENABLE:			ZEnable = value;			break;
	case D3DRENDERSTATE_ZWRITEENABLE:		ZWriteEnable = value;		break;
	case D3DRENDERSTATE_ZFUNC:				ZFunc = value;				break;
	case D3DRENDERSTATE_COLORKEYENABLE:		ColorKeyEnable = value;		break;

	case D3DRENDERSTATE_FOGENABLE:			FogEnable = value;			break;
	case D3DRENDERSTATE_ALPHATESTENABLE:	AlphaTestEnable = value;	break;
	case D3DRENDERSTATE_SRCBLEND:			SrcBlend = value;			break;
	case D3DRENDERSTATE_DESTBLEND:			DestBlend = value;			break;
	case D3DRENDERSTATE_ALPHABLENDENABLE:	AlphaBlendEnable = value;	break;
	case D3DRENDERSTATE_ZBIAS:				ZBias = value;				break;
	case D3DRENDERSTATE_CULLMODE:			CullMode = value;			break;

	default:								ASSERT(0);
	}
}

// SetEffect
//
// set a special vertex effect

void RenderState::SetEffect(DWORD effect)
{
	Effect = effect;

	switch (effect)
	{
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

#ifdef _DEBUG
// Validate
//
// validate the render state for the PerMedia 2 ;)

char* RenderState::Validate()
{
	// check which alpha modes are used
	if (AlphaBlendEnable)
	{
		if ((SrcBlend == D3DBLEND_ONE) && (DestBlend == D3DBLEND_ZERO))	return NULL;
		if ((SrcBlend == D3DBLEND_ZERO) && (DestBlend == D3DBLEND_ONE))	return NULL;
		if ((SrcBlend == D3DBLEND_ONE) && (DestBlend == D3DBLEND_ONE))	return NULL;
		if ((SrcBlend == D3DBLEND_SRCALPHA) && (DestBlend == D3DBLEND_INVSRCALPHA))	return NULL;
		if ((SrcBlend == D3DBLEND_ONE) && (DestBlend == D3DBLEND_SRCALPHA))	return NULL;

		//TRACE("SrcBlend = %d DestBlend = %d\n", SrcBlend, DestBlend);
		//return "Renderstate has a bad alpha blend mode";
		return "";
	}

	return NULL;
}
#endif

// InitScene
//
// set all state variables and remember

void RenderState::InitScene(DWORD fog_colour)
{
	// set constant state variables
	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_CULLMODE, CullMode);

	// we use > 0x07 instead of != 0x00 because it's
	// (a) more robust
	// (b) works on the Permedia 2 drivers
	// (c) looks better when alpha is interpolated
	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHAREF, 0x07);
	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER);

	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_SPECULARENABLE, TRUE); 

	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGCOLOR, fog_colour);
	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);

	extern SLONG	CurDrawDistance;
	float fFogDist = CurDrawDistance * (60.0f / (22.f * 256.0f));
	float fFogDistNear = fFogDist * 0.7f;
	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLESTART, FloatAsDword(fFogDistNear));
	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLEEND, FloatAsDword(fFogDist));
	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_FOGTABLEDENSITY, FloatAsDword(0.5f));
	// set variable state variables

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

//	REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ZBIAS, ZBias);	// doesn't work - done in PolyPage::AddFan instead

	// remember the state

	s_State = *this;
}

#define	MAYBE_SET(CAPS, SMALL)		if (s_State.SMALL != SMALL)	{ REALLY_SET_RENDER_STATE(D3DRENDERSTATE_ ## CAPS, SMALL); s_State.SMALL = SMALL; }

// SetChanged
//
// set just the different render states

void RenderState::SetChanged()
{
	DWORD old_texture_address = TextureAddress;

	if (WrapOnce)
	{
		TextureAddress = D3DTADDRESS_WRAP;
	}

	if (s_State.TextureMap != TextureMap)
	{
		REALLY_SET_TEXTURE(TextureMap);
		s_State.TextureMap = TextureMap;
	}

	MAYBE_SET(TEXTUREADDRESS,	TextureAddress);
	MAYBE_SET(TEXTUREMAG,		TextureMag);
	MAYBE_SET(TEXTUREMIN,		TextureMin);
	MAYBE_SET(TEXTUREMAPBLEND, TextureMapBlend);

	MAYBE_SET(ZENABLE,			ZEnable);
	MAYBE_SET(ZWRITEENABLE,		ZWriteEnable);
	MAYBE_SET(ALPHATESTENABLE,	AlphaTestEnable);
	MAYBE_SET(SRCBLEND,			SrcBlend);
	MAYBE_SET(DESTBLEND,		DestBlend);
	MAYBE_SET(ALPHABLENDENABLE,	AlphaBlendEnable);
	MAYBE_SET(ZFUNC,			ZFunc);
	MAYBE_SET(CULLMODE,			CullMode);


	// Hack the fog off for the DC - it doesn't seem to work ATM.
	// Does now.
	MAYBE_SET(FOGENABLE,		FogEnable);

	MAYBE_SET(COLORKEYENABLE,	ColorKeyEnable);
//	MAYBE_SET(ZBIAS,			ZBias);	// doesn't work - done in PolyPage::AddFan instead

	if (WrapOnce)
	{
		TextureAddress = old_texture_address;
		WrapOnce       = false;
	}
}

// Returns TRUE if the renderstates are equivalent,
// ie you don't need any renderstate changes to go from one to the other.
bool RenderState::IsSameRenderState ( RenderState *pRS )
{
	bool bRes = TRUE; 

#define CHECK_RS(name) if ( name != pRS->name ) { bRes = FALSE; }

	CHECK_RS ( TextureMap );
	CHECK_RS ( TextureAddress );
	CHECK_RS ( TextureMag );
	CHECK_RS ( TextureMin );
	CHECK_RS ( TextureMapBlend );

	CHECK_RS ( ZEnable );
	CHECK_RS ( ZWriteEnable );
	CHECK_RS ( AlphaTestEnable );
	CHECK_RS ( SrcBlend );
	CHECK_RS ( DestBlend );
	CHECK_RS ( ZFunc );
	CHECK_RS ( AlphaBlendEnable );
	CHECK_RS ( FogEnable );
	CHECK_RS ( ColorKeyEnable );

	CHECK_RS ( CullMode );
	CHECK_RS ( ZBias );
	CHECK_RS ( Effect );

#undef CHECK_RS

	return ( bRes );
}
