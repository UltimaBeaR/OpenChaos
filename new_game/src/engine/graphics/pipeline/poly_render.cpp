// Sets up Direct3D render states for all poly pages. Each page has a specific texture
// and blend mode. Also handles sorting pages by texture for fewer state switches.

#include "engine/platform/uc_common.h"
#include <math.h>
#include "engine/graphics/pipeline/poly_render.h"
#include "engine/graphics/pipeline/poly_render_globals.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/graphics_engine/d3d/render_state.h"
#include "engine/graphics/graphics_engine/graphics_engine.h"
#include "assets/texture.h"

// DefRenderState is defined in poly.cpp (not yet migrated).
// uc_orig: DefRenderState (fallen/DDEngine/Source/poly.cpp)
extern RenderState DefRenderState;

// RenderStates_OK: prevents re-initialising render states every frame.
// Was file-private in original. Kept in .cpp — it's implementation state, not a global.
// uc_orig: RenderStates_OK (fallen/DDEngine/Source/polyrenderstate.cpp)
static bool RenderStates_OK = false;

// Convenience macros to set PolyPage state (locally to this file).
// These were #define-only in the original.
#undef SET_TEXTURE
#undef SET_NO_TEXTURE
#undef SET_RENDER_STATE

#define SET_TEXTURE(PAGE)                                                          \
    {                                                                              \
        pa->RS.SetTexture(TEXTURE_get_handle(PAGE)); \
        pa->SetTexOffset(PAGE);                                                    \
    }
#define SET_NO_TEXTURE pa->RS.SetTexture(GE_TEXTURE_NONE)
// SET_RENDER_STATE removed — GERenderState uses typed setters instead of generic SetRenderState(DWORD, DWORD).
#define SET_EFFECT(FX) pa->RS.SetEffect(FX)

// uc_orig: POLY_reset_render_states (fallen/DDEngine/Source/polyrenderstate.cpp)
void POLY_reset_render_states()
{
    RenderStates_OK = false;
}

// uc_orig: POLY_init_texture_flags (fallen/DDEngine/Source/polyrenderstate.cpp)
void POLY_init_texture_flags(void)
{
    POLY_reset_render_states();
    memset(POLY_page_flag, 0, sizeof(POLY_page_flag));
}

// uc_orig: POLY_load_texture_flags (fallen/DDEngine/Source/polyrenderstate.cpp)
void POLY_load_texture_flags(CBYTE* fname, SLONG offset)
{
    FILE* handle = MF_Fopen(fname, "rb");

    if (handle) {
        CBYTE line[256];
        SLONG match;
        SLONG page = 0;

        while (fgets(line, 256, handle)) {
            match = sscanf(line, "Page %d:", &page);

            page += offset;

            if (match == 1 && WITHIN(page, 0, POLY_NUM_PAGES - 1)) {
                CBYTE* c;

                for (c = line; *c; c++)
                    ; // Zoom to the end of the line.

                while (1) {
                    c--;

                    if (*c == 'T' || *c == 't') {
                        POLY_page_flag[page] |= POLY_PAGE_FLAG_TRANSPARENT;
                    } else if (*c == 'W' || *c == 'w') {
                        POLY_page_flag[page] |= POLY_PAGE_FLAG_WRAP;
                    } else if (*c == 'A' || *c == 'a') {
                        POLY_page_flag[page] |= POLY_PAGE_FLAG_ADD_ALPHA;
                    } else if (*c == 'I' || *c == 'i') {
                        if (WITHIN(page, 0, POLY_NUM_PAGES - 2)) {
                            POLY_page_flag[page + 0] |= POLY_PAGE_FLAG_2PASS;
                            POLY_page_flag[page + 1] |= POLY_PAGE_FLAG_TRANSPARENT;
                            POLY_page_flag[page + 1] |= POLY_PAGE_FLAG_SELF_ILLUM;
                        }
                    } else if (*c == 'S' || *c == 's') {
                        POLY_page_flag[page] |= POLY_PAGE_FLAG_SELF_ILLUM;
                    } else if (*c == 'F' || *c == 'f') {
                        POLY_page_flag[page] |= POLY_PAGE_FLAG_NO_FOG;
                    } else if (*c == 'D' || *c == 'd') {
                        if (WITHIN(page, 0, POLY_NUM_PAGES - 2)) {
                            POLY_page_flag[page + 0] |= POLY_PAGE_FLAG_2PASS | POLY_PAGE_FLAG_WINDOW;
                            POLY_page_flag[page + 1] |= POLY_PAGE_FLAG_WINDOW_2ND;
                        }
                    } else if (*c == 'M' || *c == 'm') {
                        POLY_page_flag[page] |= POLY_PAGE_FLAG_ALPHA;
                    } else if (*c == ':') {
                        break;
                    }
                }
            }
        }

        MF_Fclose(handle);
    }
}

// uc_orig: POLY_init_render_states (fallen/DDEngine/Source/polyrenderstate.cpp)
// Initialises D3D render states for each poly page based on its page ID and flags.
// Also sorts pages by texture handle to reduce state changes during rendering.
void POLY_init_render_states()
{
    if (RenderStates_OK)
        return;

    // AENG_detail_filter is defined in aeng.cpp (not yet migrated).
    // uc_orig: AENG_detail_filter (fallen/DDEngine/Source/aeng.cpp)
    extern int AENG_detail_filter;

    if (!AENG_detail_filter) {
        DefRenderState = RenderState(GETextureFilter::Nearest, GETextureFilter::Nearest);
    } else {
        DefRenderState = RenderState(GETextureFilter::Linear, GETextureFilter::Linear);
    }

    // Set each page to the default render state.
    for (int ii = 0; ii < POLY_NUM_PAGES; ii++) {
        POLY_Page[ii].RS = DefRenderState;
    }

    // Override per-page render states based on page type.
    PolyPage* pa;

    for (int ii = 0; ii < POLY_NUM_PAGES; ii++) {
        pa = &POLY_Page[ii];

        {
            // Default is on!
            pa->RS.SetFogEnabled(true);

            // set using the old interface
            switch (ii) {
            case POLY_PAGE_LADSHAD:
                SET_TEXTURE((TEXTURE_page_ladshad));
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetDepthWrite(false);
                break;

            case POLY_PAGE_SIGN:
                SET_TEXTURE((TEXTURE_page_sign));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                break;

            case POLY_PAGE_LASTPANEL_ALPHA:
                SET_TEXTURE((TEXTURE_page_lastpanel));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);

                break;

            case POLY_PAGE_LASTPANEL_ADDALPHA:
                SET_TEXTURE((TEXTURE_page_lastpanel));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                pa->RS.SetAlphaBlendEnabled(true);
                SET_EFFECT(RS_AlphaPremult);

                break;

            case POLY_PAGE_LASTPANEL_ADD:
                SET_TEXTURE((TEXTURE_page_lastpanel));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);

                break;

            case POLY_PAGE_LASTPANEL_SUB:
                SET_TEXTURE((TEXTURE_page_lastpanel));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::Zero);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_EFFECT(RS_BlackWithAlpha);
                break;

            case POLY_PAGE_LASTPANEL2_ADD:
                SET_TEXTURE((TEXTURE_page_lastpanel2));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);

                break;

            case POLY_PAGE_LASTPANEL2_ADDALPHA:
                SET_TEXTURE((TEXTURE_page_lastpanel2));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_EFFECT(RS_AlphaPremult);

                break;

            case POLY_PAGE_LASTPANEL2_SUB:
                SET_TEXTURE((TEXTURE_page_lastpanel2));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::Zero);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_EFFECT(RS_BlackWithAlpha);
                break;

            case POLY_PAGE_LASTPANEL2_ALPHA:
                SET_TEXTURE((TEXTURE_page_lastpanel2));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);

                break;

            case POLY_PAGE_LITE_BOLT:
                SET_TEXTURE((TEXTURE_page_litebolt));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                break;

            case POLY_PAGE_SHADOW_OVAL:
                SET_TEXTURE((TEXTURE_page_shadowoval));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetFogEnabled(false);

                break;

            case POLY_PAGE_SHADOW_SQUARE:
                SET_TEXTURE((TEXTURE_page_shadowsquare));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_FADECAT:
                SET_TEXTURE((TEXTURE_page_fadecat));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_FADE_MF:
                SET_TEXTURE((TEXTURE_page_fade_MF));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_LADDER:
                SET_TEXTURE((TEXTURE_page_ladder));
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);

                break;

            case POLY_PAGE_FINALGLOW:
                SET_TEXTURE((TEXTURE_page_finalglow));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                pa->RS.SetDepthWrite(false);
                break;

            case POLY_PAGE_IC2_NORMAL:
                SET_TEXTURE((TEXTURE_page_ic2));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                break;

            case POLY_PAGE_IC_NORMAL:
                SET_TEXTURE((TEXTURE_page_ic));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                break;

            case POLY_PAGE_IC2_ALPHA:
            case POLY_PAGE_IC2_ALPHA_END:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                SET_TEXTURE((TEXTURE_page_ic2));
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                break;

            case POLY_PAGE_IC_ALPHA:
            case POLY_PAGE_IC_ALPHA_END:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                SET_TEXTURE((TEXTURE_page_ic));
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                break;

            case POLY_PAGE_IC2_ADDITIVE:
                SET_TEXTURE((TEXTURE_page_ic2));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                break;

            case POLY_PAGE_IC_ADDITIVE:
                SET_TEXTURE((TEXTURE_page_ic));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                break;

            case POLY_PAGE_PRESS1:
                pa->RS.SetFogEnabled(false);
                TEXTURE_set_colour_key(TEXTURE_page_press1);
                pa->RS.SetColorKeyEnabled(true);
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthEnabled(false);
                SET_TEXTURE((TEXTURE_page_press1));
                break;

            case POLY_PAGE_PRESS2:
                pa->RS.SetFogEnabled(false);
                TEXTURE_set_colour_key(TEXTURE_page_press2);
                pa->RS.SetColorKeyEnabled(true);
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthEnabled(false);
                SET_TEXTURE((TEXTURE_page_press2));
                break;

            case POLY_PAGE_TARGET:
                pa->RS.SetFogEnabled(false);
                TEXTURE_set_colour_key(TEXTURE_page_target);
                pa->RS.SetColorKeyEnabled(true);
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthEnabled(false);
                SET_TEXTURE((TEXTURE_page_target));
                break;

            case POLY_PAGE_DEVIL:
                SET_TEXTURE((TEXTURE_page_devil));
                pa->RS.SetDepthWrite(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetTextureBlend(GETextureBlend::Decal);
                break;

            case POLY_PAGE_ANGEL:
                SET_TEXTURE((TEXTURE_page_angel));
                pa->RS.SetDepthWrite(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetTextureBlend(GETextureBlend::Decal);
                break;

            case POLY_PAGE_LEAF:
                SET_TEXTURE((TEXTURE_page_leaf));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaTestEnabled(true);

                break;

            case POLY_PAGE_RUBBISH:
                SET_TEXTURE((TEXTURE_page_rubbish));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaTestEnabled(true);

                break;

            case POLY_PAGE_WINMAP:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_winmap));
                pa->RS.SetFogEnabled(true);
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);

                break;

            case POLY_PAGE_ENVMAP:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_envmap));
                pa->RS.SetFogEnabled(true);
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                break;

            case POLY_PAGE_SEWATER:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                SET_TEXTURE((TEXTURE_page_water));
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                break;

            case POLY_PAGE_SKY:
                SET_TEXTURE((TEXTURE_page_sky));
                // Z-buffered since sky is no longer drawn first.
                pa->RS.SetDepthEnabled(true);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_SHADOW:
                pa->RS.SetAlphaBlendEnabled(true);

                if (ge_supports_dest_inv_src_color()) {
                    // use a density greyscale shadowmap
                    pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                    pa->RS.SetSrcBlend(GEBlendFactor::Zero);
                    pa->RS.SetDstBlend(GEBlendFactor::InvSrcColor);
                } else {
                    // use a density alpha (+black) shadowmap
                    pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                    pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                    pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                }

                SET_TEXTURE((TEXTURE_page_shadow));
                pa->RS.SetDepthWrite(false);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthBias(8);
                break;

            case POLY_PAGE_TEST_SHADOWMAP:
                if (ge_supports_dest_inv_src_color()) {
                    pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                    SET_TEXTURE(TEXTURE_page_shadow);
                    pa->RS.SetFogEnabled(false);
                }
                break;

            case POLY_PAGE_PUDDLE:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_puddle));
                pa->RS.SetAlphaTestEnabled(true);
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_MOON:
                SET_TEXTURE((TEXTURE_page_moon));
                pa->RS.SetFogEnabled(false);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                pa->RS.SetAlphaBlendEnabled(true);
                break;

            case POLY_PAGE_MANONMOON:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                SET_TEXTURE((571));
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                break;

            case POLY_PAGE_CLOUDS:
                pa->RS.SetDepthWrite(false);
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_clouds));
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_ALPHA:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetDepthWrite(false);
                SET_NO_TEXTURE;
                break;

            case POLY_PAGE_ALPHA_OVERLAY:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetDepthWrite(false);
                SET_NO_TEXTURE;
                pa->RS.SetFogEnabled(false);
                pa->RS.SetDepthEnabled(false);
                break;

            case POLY_PAGE_ADDITIVE:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_NO_TEXTURE;
                break;

            case POLY_PAGE_ADDITIVEALPHA:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_NO_TEXTURE;
                SET_EFFECT(RS_AlphaPremult);
                break;

            case POLY_PAGE_MASKED:
                SET_TEXTURE(-1);
                TEXTURE_set_colour_key(0);
                pa->RS.SetColorKeyEnabled(true);
                break;

            case POLY_PAGE_FACE1:
                pa->RS.SetTextureMag(GETextureFilter::Nearest);
                pa->RS.SetTextureMin(GETextureFilter::Nearest);
                SET_TEXTURE((TEXTURE_page_face1));
                pa->RS.SetFogEnabled(false);

                break;

            case POLY_PAGE_FACE2:
                pa->RS.SetTextureMag(GETextureFilter::Nearest);
                pa->RS.SetTextureMin(GETextureFilter::Nearest);
                SET_TEXTURE((TEXTURE_page_face2));
                pa->RS.SetFogEnabled(false);

                break;

            case POLY_PAGE_FACE3:
                SET_TEXTURE((TEXTURE_page_face3));
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_FACE4:
                SET_TEXTURE((TEXTURE_page_face4));
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_FACE5:
                SET_TEXTURE((TEXTURE_page_face5));
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_FACE6:
                SET_TEXTURE((TEXTURE_page_face6));
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_COLOUR_ALPHA:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetFogEnabled(false);
                SET_NO_TEXTURE;

                break;

            case POLY_PAGE_COLOUR:
                pa->RS.SetFogEnabled(false);
                SET_NO_TEXTURE;

                break;

            case POLY_PAGE_COLOUR_WITH_FOG:
                pa->RS.SetFogEnabled(true);
                SET_NO_TEXTURE;
                break;

            case POLY_PAGE_WATER:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_water));
                break;

            case POLY_PAGE_DRIP:
                pa->RS.SetDepthWrite(false);
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_drip));
                break;

            case POLY_PAGE_FOG:
                pa->RS.SetDepthWrite(false);
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_fog));
                break;

            case POLY_PAGE_STEAM:
                pa->RS.SetDepthWrite(false);
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_steam));
                break;

            case POLY_PAGE_BANG:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_bang));
                break;

            case POLY_PAGE_TEXT:
                pa->RS.SetDepthEnabled(false);
                SET_TEXTURE((TEXTURE_page_font));
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_LOGO:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_logo));
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_DROPLET:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_droplet));
                pa->RS.SetDepthWrite(false);
                break;

            case POLY_PAGE_RAINDROP:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                SET_TEXTURE((TEXTURE_page_raindrop));
                pa->RS.SetDepthWrite(false);
                break;

            case POLY_PAGE_SPARKLE:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_sparkle));
                pa->RS.SetDepthWrite(false);
                break;

            case POLY_PAGE_FLAMES:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_flames));
                SET_EFFECT(RS_AlphaPremult);
                break;

            case POLY_PAGE_FLAMES2:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_flame2));
                SET_EFFECT(RS_AlphaPremult);

                break;

            case POLY_PAGE_SMOKE:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::Zero);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcColor);
                SET_TEXTURE((TEXTURE_page_smoke));
                break;

            case POLY_PAGE_MENUFLAME:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_menuflame));
                pa->RS.SetFogEnabled(false);
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                break;

            case POLY_PAGE_MENUTEXT:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(true);
                pa->RS.SetDepthBias(0);
                pa->RS.SetAlphaBlendEnabled(false);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_flames));
                pa->RS.SetFogEnabled(false);
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                break;

            case POLY_PAGE_MENUPASS:
                pa->RS.SetDepthWrite(false);
                SET_TEXTURE((TEXTURE_page_moon));
                pa->RS.SetFogEnabled(false);
                TEXTURE_set_colour_key(TEXTURE_page_moon);
                pa->RS.SetColorKeyEnabled(true);
                break;

            case POLY_PAGE_BARBWIRE:
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_barbwire));
                pa->RS.SetDepthWrite(false);
                break;

            case POLY_PAGE_FOOTPRINT:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_footprint));
                pa->RS.SetFogEnabled(true);
                break;

            case POLY_PAGE_FONT2D:
                pa->RS.SetDepthEnabled(false);
                SET_TEXTURE((TEXTURE_page_font2d));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetTextureMag(GETextureFilter::Linear);
                pa->RS.SetTextureMin(GETextureFilter::Linear);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);

                break;

            case POLY_PAGE_BIGBANG:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_bigbang));
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                SET_EFFECT(RS_InvAlphaPremult);

                break;

            case POLY_PAGE_FLAMES3:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_flames3));
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                SET_EFFECT(RS_InvAlphaPremult);
                break;

            case POLY_PAGE_DUSTWAVE:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_dustwave));
                SET_EFFECT(RS_InvAlphaPremult);

                break;

            case POLY_PAGE_BLOODSPLAT:
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_bloodsplat));
                pa->RS.SetFogEnabled(true);

                break;

            case POLY_PAGE_BLOOM1:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_bloom1));
                SET_EFFECT(RS_AlphaPremult);

                break;

            case POLY_PAGE_BLOOM2:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_bloom2));
                SET_EFFECT(RS_AlphaPremult);
                break;

            case POLY_PAGE_SNOWFLAKE:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_snowflake));

                break;
            case POLY_PAGE_HITSPANG:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_hitspang));
                SET_EFFECT(RS_AlphaPremult);
                break;

            case POLY_PAGE_LENSFLARE:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_lensflare));
                break;

            case POLY_PAGE_TYRETRACK:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_tyretrack_alpha));
                pa->RS.SetFogEnabled(true);
                break;

            case POLY_PAGE_TYRESKID:
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetSrcBlend(GEBlendFactor::Zero);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_tyretrack));
                pa->RS.SetFogEnabled(true);
                pa->RS.SetDepthBias(4);
                SET_EFFECT(RS_BlackWithAlpha);
                break;

            case POLY_PAGE_NEWFONT_INVERSE:
                // All the alternatives in the original were commented out; only the final version
                // that uses TEXTURE_page_lcdfont with additive blending remains.
                SET_TEXTURE((TEXTURE_page_lcdfont));
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetSrcBlend(GEBlendFactor::One);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetFogEnabled(false);

                break;

            case POLY_PAGE_MENULOGO:
                pa->RS.SetDepthEnabled(false);
                SET_TEXTURE((TEXTURE_page_menulogo));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_POLAROID:
                SET_TEXTURE((TEXTURE_page_polaroid));
                pa->RS.SetDepthWrite(false);
                pa->RS.SetDepthEnabled(false);
                pa->RS.SetFogEnabled(false);
                pa->RS.SetTextureBlend(GETextureBlend::Decal);

                break;

            case POLY_PAGE_SMOKER:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_smoker));
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                SET_EFFECT(RS_AlphaPremult);
                break;

            case POLY_PAGE_NEWFONT:
                pa->RS.SetDepthEnabled(false);
                SET_TEXTURE((TEXTURE_page_lcdfont));
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetSrcBlend(GEBlendFactor::Zero);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetFogEnabled(false);
                SET_EFFECT(RS_BlackWithAlpha);
                break;

            case POLY_PAGE_EXPLODE1:
                pa->RS.SetTextureMag(GETextureFilter::Nearest);
                pa->RS.SetTextureMin(GETextureFilter::Nearest);
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_explode1));

                break;

            case POLY_PAGE_EXPLODE2:
                pa->RS.SetTextureMag(GETextureFilter::Nearest);
                pa->RS.SetTextureMin(GETextureFilter::Nearest);
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_explode2));

                break;

            case POLY_PAGE_EXPLODE1_ADDITIVE:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_explode1));
                SET_EFFECT(RS_AlphaPremult);

                break;

            case POLY_PAGE_EXPLODE2_ADDITIVE:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_explode2));
                SET_EFFECT(RS_AlphaPremult);

                break;

            case POLY_PAGE_SPLASH:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_splash));
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                SET_EFFECT(RS_InvAlphaPremult);
                break;

            case POLY_PAGE_METEOR:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_meteor));
                SET_EFFECT(RS_AlphaPremult);
                break;

            case POLY_PAGE_SUBTRACTIVEALPHA:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::Zero);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_NO_TEXTURE;
                pa->RS.SetFogEnabled(false);
                SET_EFFECT(RS_BlackWithAlpha);
                break;

            case POLY_PAGE_SMOKECLOUD:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::Zero);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_smokecloud));
                SET_EFFECT(RS_BlackWithAlpha);
                break;

            case POLY_PAGE_SMOKECLOUD2:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_smokecloud));

                break;

            case POLY_PAGE_BIG_BUTTONS:
                pa->RS.SetTextureBlend(GETextureBlend::Decal);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_bigbutton));
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_TINY_BUTTONS:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_tinybutt));
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_BIG_LEAF:
                pa->RS.SetTextureBlend(GETextureBlend::Modulate);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                SET_TEXTURE((TEXTURE_page_bigleaf));
                pa->RS.SetFogEnabled(false);
                break;

            case POLY_PAGE_BIG_RAIN:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_bigrain));
                pa->RS.SetFogEnabled(false);
                SET_EFFECT(RS_AlphaPremult);
                break;

            case POLY_PAGE_PCFLAMER:
                pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
                pa->RS.SetDepthWrite(false);
                pa->RS.SetAlphaBlendEnabled(true);
                pa->RS.SetSrcBlend(GEBlendFactor::InvSrcAlpha);
                pa->RS.SetDstBlend(GEBlendFactor::One);
                SET_TEXTURE((TEXTURE_page_pcflamer));
                pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                SET_EFFECT(RS_InvAlphaPremult);

                break;

            default:
                // Standard texture pages: apply per-page flags.
                if (ii < TEXTURE_page_num_standard && !(draw_3d && (ii == 510 || ii == 511))) {
                    SET_TEXTURE((ii));

                    if (POLY_page_flag[ii]) {
                        if (POLY_page_flag[ii] & POLY_PAGE_FLAG_TRANSPARENT) {
                            bool use_chroma = false;

                            if (POLY_page_flag[ii] & POLY_PAGE_FLAG_SELF_ILLUM) {
                                if (ge_supports_dest_inv_src_color() && !ge_supports_modulate_alpha()) {
                                    // it's a RagePro, so use chroma keying
                                    use_chroma = true;
                                }
                            }

                            if (use_chroma) {
                                // use colour keying to make the windows work on the Rage Pro
                                TEXTURE_set_colour_key(ii);
                                pa->RS.SetColorKeyEnabled(true);
                            } else {
                                pa->RS.SetAlphaTestEnabled(true);
                            }
                        }

                        if (POLY_page_flag[ii] & POLY_PAGE_FLAG_WRAP) {
                            pa->RS.SetTextureAddress(GETextureAddress::Wrap);
                        }

                        if (POLY_page_flag[ii] & POLY_PAGE_FLAG_ADD_ALPHA) {
                            pa->RS.SetDepthWrite(false);
                            pa->RS.SetAlphaBlendEnabled(true);
                            pa->RS.SetSrcBlend(GEBlendFactor::One);
                            pa->RS.SetDstBlend(GEBlendFactor::One);
                        }

                        if ((POLY_page_flag[ii] & POLY_PAGE_FLAG_SELF_ILLUM) && !draw_3d) {
                            pa->RS.SetTextureBlend(GETextureBlend::Decal);
                        }

                        if (POLY_page_flag[ii] & POLY_PAGE_FLAG_NO_FOG) {
                            pa->RS.SetFogEnabled(false);
                        }

                        if (POLY_page_flag[ii] & POLY_PAGE_FLAG_WINDOW) {
                            pa->RS.SetAlphaTestEnabled(true);

                            pa->RS.SetAlphaBlendEnabled(true);
                            pa->RS.SetSrcBlend(GEBlendFactor::One);
                            pa->RS.SetDstBlend(GEBlendFactor::One);
                        }

                        if (POLY_page_flag[ii] & POLY_PAGE_FLAG_WINDOW_2ND) {
                            pa->RS.SetDepthFunc(GECompareFunc::Less);
                        }

                        if (POLY_page_flag[ii] & POLY_PAGE_FLAG_ALPHA) {
                            pa->RS.SetDepthWrite(false);
                            pa->RS.SetAlphaBlendEnabled(true);
                            pa->RS.SetSrcBlend(GEBlendFactor::SrcAlpha);
                            pa->RS.SetDstBlend(GEBlendFactor::InvSrcAlpha);
                        }
                    }

                    if (ge_supports_adami_lighting()) {
                        switch (ii >> 6) {
                        case 9:
                        case 10:
                        case 18:
                        case 19:
                        case 20:
                        case 21:
                            // People texture page — use dest-colour src-colour blend for Adami lighting.
                            pa->RS.SetSrcBlend(GEBlendFactor::DstColor);
                            pa->RS.SetDstBlend(GEBlendFactor::SrcColor);

                            pa->RS.SetAlphaBlendEnabled(true);
                            pa->RS.SetTextureBlend(GETextureBlend::Decal);

                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    // Flush queued polys before re-sorting pages.
    // uc_orig: POLY_ClearAllPages (fallen/DDEngine/Source/poly.cpp)
    extern void POLY_ClearAllPages(void);
    POLY_ClearAllPages();

    // Sort pages by texture handle to minimise D3D state changes during rendering.
    for (int ii = 0; ii < POLY_NUM_PAGES; ii++) {
        PageOrdered[ii] = false;
    }

    SLONG pos = 0;
    while (pos < POLY_NUM_PAGES) {
        int ii;
        for (ii = 0; ii < POLY_NUM_PAGES; ii++) {
            if (!PageOrdered[ii])
                break;
        }
        if (ii == POLY_NUM_PAGES) {
            // All done.
            break;
        }

        POLY_Page[ii].pTheRealPolyPage = &(POLY_Page[ii]);

        PageOrder[pos++] = ii;
        PageOrdered[ii] = true;
        GETextureHandle tex = POLY_Page[ii].RS.GetTexture();
        PolyPage* pPolyPage = &(POLY_Page[ii]);

        if (tex != GE_TEXTURE_NONE) {
            for (ii++; ii < POLY_NUM_PAGES; ii++) {
                if (POLY_Page[ii].RS.GetTexture() == tex) {
                    // If this page has the same texture AND same render states, make it a
                    // "ghost" page (all polys sent to it go to the real page instead).
                    if (POLY_Page[ii].RS.IsSameRenderState(&(pPolyPage->RS))) {
                        POLY_Page[ii].pTheRealPolyPage = pPolyPage;
                    } else {
                        POLY_Page[ii].pTheRealPolyPage = &(POLY_Page[ii]);
                        PageOrder[pos++] = ii;
                    }
                    PageOrdered[ii] = true;
                }
            }
        }
    }

    // iPolyNumPagesRender is defined in poly.cpp (not yet migrated).
    // uc_orig: iPolyNumPagesRender (fallen/DDEngine/Source/poly.cpp)
    extern int iPolyNumPagesRender;
    iPolyNumPagesRender = pos;

    RenderStates_OK = UC_TRUE;
}
