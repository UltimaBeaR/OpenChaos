// Character rendering system — multi-matrix body part drawing.
// Ported from original DDEngine, now uses ge_* abstraction (OpenGL backend).

#include "engine/graphics/geometry/figure.h"
#include "engine/graphics/geometry/figure_globals.h"

// D3D, game types, and all subsystems needed throughout the original file.
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/geometry/sprite.h"
#include "engine/core/fmatrix.h"
#include "things/core/interact.h"
#include "engine/core/matrix.h"
#include "things/characters/anim_ids.h"
#include "engine/graphics/geometry/mesh.h"
#include "map/level_pools.h"
#include "assets/texture.h"
#include "assets/texture_globals.h" // alt_texture
#include <math.h>
#include "things/core/hierarchy.h"
#include "engine/core/quaternion.h"
#include "things/characters/person.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include <vector>  // figure_build_consolidated_skin_world scratch buffers
#include <cstring> // memcpy
#include <cfloat>  // FLT_MAX (reflection screen-bbox accumulator)
#include "engine/graphics/render_interp.h" // BoneInterpTransform, render_interp_get_cached_pose
#include "engine/graphics/geometry/pose_composer.h" // POSE_MAX_BONES (pose snapshot)
#include "engine/graphics/geometry/bind_palette.h" // bind_palette_get (P2-C world-skin build)
#include "engine/animation/anim_types.h" // GameKeyFrame layout (FirstElement)
#include "things/core/drawtype.h"
#include "game/game_types.h" // NET_PERSON (player Thing*) for figure-morph debug log
#include "game/game_globals.h" // g_physics_hz for log
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks for log timestamp
#include <stdio.h>
#include <stdint.h>

// Enable per-render-frame CSV log of player root body part morph state.
// Lines go to stderr (and stderr.log via Makefile redirect). Use to compare
// blend behaviour at different physics rates.
#define FIGURE_MORPH_LOG 0

// Note: animation depends on lighting (BuildMMLightingTable reads NIGHT_* globals).
// This cross-engine coupling exists in the original; will be resolved in Stage 7.
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"

// uc_orig: DeadAndBuried (fallen/DDEngine/Source/figure.cpp)
// Paints a solid 50x50 colour block on the front surface for crash diagnostics.
void DeadAndBuried(DWORD dwColour)
{
    ge_debug_paint_block(dwColour);
}

// MM_* pointer initialisation is done in figure_globals.cpp via a static constructor.
// (The backing storage arrays and the initialiser struct live there to keep all global
//  state in _globals files per project rules.)

// uc_orig: BuildMMLightingTable (fallen/DDEngine/Source/figure.cpp)
// Computes the per-character half-Lambert lighting endpoints used by the
// world-skin shader. The original wrote a 128-entry ULONG ramp; that ramp
// was itself built as a linear float interpolation between ambient (idx 0)
// and fully-lit (idx 63), truncated to bytes per step, so two RGB
// endpoints fully describe it. We store them in MM_FadeStart / MM_FadeStep
// (and the masked tint variant in MM_FadeStart/StepTint) — the shader
// reconstructs ramp[i] = floor(clamp(start + i*step, 0, 255)).
// p: if non-null, the character is on fire (darkens with soot).
// colour_and: tint mask — each byte must be 0x00 or 0xFF (the masks the
// game actually passes, 0 and 0xffff00ff, satisfy this); a 0x00 byte
// zeroes the corresponding RGB channel in the tint endpoints.
// uc_orig: AMBIENT_FACTOR (fallen/DDEngine/Source/figure.cpp)
#define AMBIENT_FACTOR (0.5f * 256.0f)
void BuildMMLightingTable(Pyro* p, DWORD colour_and)
{
    // Find the dominant light direction by summing all weighted light vectors.
    float fBright = NIGHT_amb_red * 0.35f + NIGHT_amb_green * 0.45f + NIGHT_amb_blue * 0.2f;
    GEVector vTotal;
    vTotal.x = fBright * NIGHT_amb_norm_x;
    vTotal.y = fBright * NIGHT_amb_norm_y;
    vTotal.z = fBright * NIGHT_amb_norm_z;
    NIGHT_Found* nf;
    for (int j = 0; j < NIGHT_found_upto; j++) {
        nf = &NIGHT_found[j];
        float fBright2 = nf->r * 0.35f + nf->g * 0.45f + nf->b * 0.2f;
        {
            vTotal.x -= fBright2 * nf->dx;
            vTotal.y -= fBright2 * nf->dy;
            vTotal.z -= fBright2 * nf->dz;
        }
    }

    // Normalise the dominant direction.
    float fLength = 1.0f / sqrtf(vTotal.x * vTotal.x + vTotal.y * vTotal.y + vTotal.z * vTotal.z);
    vTotal.x *= fLength;
    vTotal.y *= fLength;
    vTotal.z *= fLength;

    // Compute ambient and fully-lit colours using the normalised direction.
    GEColorValue cvAmb, cvLight;
    cvAmb.r = NIGHT_amb_red * AMBIENT_FACTOR;
    cvAmb.g = NIGHT_amb_green * AMBIENT_FACTOR;
    cvAmb.b = NIGHT_amb_blue * AMBIENT_FACTOR;

    // Fudge: brighten ambient on night missions to compensate for dim ambient.
    if ((NIGHT_amb_red + NIGHT_amb_green + NIGHT_amb_blue) < 100) {
        cvAmb.r *= 2.0f;
        cvAmb.g *= 2.0f;
        cvAmb.b *= 2.0f;
    }

    cvLight = cvAmb;

    float fDotProd = vTotal.x * NIGHT_amb_norm_x + vTotal.y * NIGHT_amb_norm_y + vTotal.z * NIGHT_amb_norm_z;
    if (fDotProd < 0.0f) {
        cvAmb.r -= fDotProd * NIGHT_amb_red;
        cvAmb.g -= fDotProd * NIGHT_amb_green;
        cvAmb.b -= fDotProd * NIGHT_amb_blue;
    } else {
        cvLight.r += fDotProd * NIGHT_amb_red;
        cvLight.g += fDotProd * NIGHT_amb_green;
        cvLight.b += fDotProd * NIGHT_amb_blue;
    }
    for (int j = 0; j < NIGHT_found_upto; j++) {
        nf = &NIGHT_found[j];
        fDotProd = -vTotal.x * nf->dx - vTotal.y * nf->dy - vTotal.z * nf->dz;
        if (fDotProd < 0.0f) {
            cvAmb.r -= fDotProd * nf->r;
            cvAmb.g -= fDotProd * nf->g;
            cvAmb.b -= fDotProd * nf->b;
        } else {
            cvLight.r += fDotProd * nf->r;
            cvLight.g += fDotProd * nf->g;
            cvLight.b += fDotProd * nf->b;
        }
    }

    MM_vLightDir = vTotal;

    // Darken on-fire characters with soot. The immolate pyro's `counter`
    // climbs 0..255 over the burn; lerp the character's lighting from full
    // toward a dark soot colour as it climbs, so a burning character chars
    // to near-black. The legacy `cvX - counter` form is a no-op here:
    // cvLight/cvAmb are in the thousands while counter caps at 255, so it
    // darkened by under 3% — invisible. The dark target keeps the legacy
    // 10/4/3 floor (a faintly warm ember-dark, not pure black).
    if (p != NULL) {
        // counter value at which the character is fully charred. Lower =
        // faster blackening; the immolate counter climbs ~1/tick and caps
        // at 255, so 60 ≈ a 2 s burn to full char.
        constexpr float BURN_CHAR_FULL_COUNTER = 60.0f;
        constexpr float SOOT_R = 10.0f, SOOT_G = 4.0f, SOOT_B = 3.0f;
        float t = (float)p->counter / BURN_CHAR_FULL_COUNTER;
        if (t > 1.0f)
            t = 1.0f;
        cvLight.r += (SOOT_R - cvLight.r) * t;
        cvLight.g += (SOOT_G - cvLight.g) * t;
        cvLight.b += (SOOT_B - cvLight.b) * t;
        cvAmb.r += (SOOT_R - cvAmb.r) * t;
        cvAmb.g += (SOOT_G - cvAmb.g) * t;
        cvAmb.b += (SOOT_B - cvAmb.b) * t;
    }

    // PC people are lit x2.
    cvLight.r *= 2.0f;
    cvLight.g *= 2.0f;
    cvLight.b *= 2.0f;
    cvAmb.r *= 2.0f;
    cvAmb.g *= 2.0f;
    cvAmb.b *= 2.0f;

    const float fColourScale = (1.0f / (256.0f * 128.0f)) * 255.0f * NIGHT_LIGHT_MULTIPLIER;
    GEColorValue cvCur = cvAmb;
    cvLight.r -= cvAmb.r;
    cvLight.g -= cvAmb.g;
    cvLight.b -= cvAmb.b;
    cvCur.r *= fColourScale;
    cvCur.g *= fColourScale;
    cvCur.b *= fColourScale;
    cvLight.r *= (fColourScale / 64.0f);
    cvLight.g *= (fColourScale / 64.0f);
    cvLight.b *= (fColourScale / 64.0f);

    // Store the two endpoints — cvCur is the ambient end (ramp[0]),
    // cvLight is the per-step delta. The shader applies the same
    // clamp+floor as the legacy CPU loop did per-step, so values match
    // 1:1 modulo single-LSB float precision drift.
    MM_FadeStart[0] = cvCur.r;
    MM_FadeStart[1] = cvCur.g;
    MM_FadeStart[2] = cvCur.b;
    MM_FadeStep[0]  = cvLight.r;
    MM_FadeStep[1]  = cvLight.g;
    MM_FadeStep[2]  = cvLight.b;

    // Tint variant: legacy code applied `colour_and` as a per-byte mask
    // to each table entry. For the masks the game actually passes (0,
    // 0xffff00ff) each byte is 0x00 or 0xFF, so masking commutes with
    // the linear-then-floor pipeline — zeroing a channel here gives the
    // same shader output as ANDing every byte the CPU loop produced.
    // Bit order matches the original packing: R in bits 16..23, G in
    // 8..15, B in 0..7.
    const float mR = ((colour_and >> 16) & 0xFFu) ? 1.0f : 0.0f;
    const float mG = ((colour_and >>  8) & 0xFFu) ? 1.0f : 0.0f;
    const float mB = ( colour_and        & 0xFFu) ? 1.0f : 0.0f;
    MM_FadeStartTint[0] = cvCur.r   * mR;
    MM_FadeStartTint[1] = cvCur.g   * mG;
    MM_FadeStartTint[2] = cvCur.b   * mB;
    MM_FadeStepTint[0]  = cvLight.r * mR;
    MM_FadeStepTint[1]  = cvLight.g * mG;
    MM_FadeStepTint[2]  = cvLight.b * mB;
}

// --- Microsoft Index List Optimizer (optlist.cpp, 1998-1999) ---
// Reorders an unordered triangle index list into strips for better GPU vertex-cache hit rate.
// Used by FIGURE_TPO_finish_3d_object.
// Types (EdgeList, INDEXTRISTRUCT, INDEXVERTSTRUCT, MSMesh class) are declared in figure.h.

// uc_orig: MSMesh::MSMesh (fallen/DDEngine/Source/figure.cpp)
MSMesh::MSMesh()
{
    memset(this, 0, sizeof(*this));
}

// uc_orig: MSMesh::SetSize (fallen/DDEngine/Source/figure.cpp)
int MSMesh::SetSize(int nTriangles)
{
    if (nTriangles <= 0) {
        if (m_aTri) {
            MemFree(m_aTri);
            m_aTri = (INDEXTRISTRUCT*)NULL;
        }
        if (m_aVert) {
            MemFree(m_aVert);
            m_aVert = (INDEXVERTSTRUCT*)NULL;
        }
        m_nMaxTri = 0;
        m_nMaxVert = 0;
    } else {
        if (nTriangles > m_nMaxTri) {
            SetSize(0);
            m_nMaxTri = nTriangles + 10;
            m_nMaxVert = m_nMaxTri * 3;
            if (m_nMaxVert > 65536)
                m_nMaxVert = 65536;
            m_aTri = (INDEXTRISTRUCT*)MemAlloc(m_nMaxTri * sizeof(INDEXTRISTRUCT));
            if (m_aTri == NULL) {
                DeadAndBuried(0x07e007e0);
            }
            m_aVert = (INDEXVERTSTRUCT*)MemAlloc(m_nMaxVert * sizeof(INDEXVERTSTRUCT));
            if (m_aVert == NULL) {
                DeadAndBuried(0x07e007e0);
            }
            if (!m_aTri || !m_aVert)
                SetSize(0);
        }
    }
    return m_nMaxTri;
}

// uc_orig: MSMesh::~MSMesh (fallen/DDEngine/Source/figure.cpp)
MSMesh::~MSMesh()
{
    SetSize(0);
}

// uc_orig: MSMesh::FindOrAddVertex (fallen/DDEngine/Source/figure.cpp)
int MSMesh::FindOrAddVertex(WORD wVert)
{
    int vertno;
    for (vertno = 0; vertno < m_nNumVert; vertno++)
        if (m_aVert[vertno].wIndex == wVert)
            return vertno;
    vertno = m_nNumVert++;
    m_aVert[vertno].wIndex = wVert;
    m_aVert[vertno].nShareCount = 0;
    return vertno;
}

// uc_orig: MSMesh::AddTri (fallen/DDEngine/Source/figure.cpp)
int MSMesh::AddTri(WORD wV1, WORD wV2, WORD wV3)
{
    int trino = m_nNumTri++;
    int v1 = FindOrAddVertex(wV1);
    int v2 = FindOrAddVertex(wV2);
    int v3 = FindOrAddVertex(wV3);

    m_aVert[v1].rgSharedTris[m_aVert[v1].nShareCount++] = trino;
    m_aVert[v2].rgSharedTris[m_aVert[v2].nShareCount++] = trino;
    m_aVert[v3].rgSharedTris[m_aVert[v3].nShareCount++] = trino;

    m_aTri[trino].v1 = v1;
    m_aTri[trino].v2 = v2;
    m_aTri[trino].v3 = v3;
    m_aTri[trino].done = 0;

    return trino;
}

// uc_orig: MSMesh::ClearTempDone (fallen/DDEngine/Source/figure.cpp)
void MSMesh::ClearTempDone(int nValue)
{
    int trino;
    for (trino = 0; trino < m_nNumTri; trino++)
        if (m_aTri[trino].done == 2) {
            m_aTri[trino].done = nValue;
        }
}

// uc_orig: MSMesh::StripLen (fallen/DDEngine/Source/figure.cpp)
int MSMesh::StripLen(int startTri, int dir, int setit, WORD* pwOut, int nMaxLen)
{
    int v1, v2, v3;
    int trino = startTri;
    int len = 0;

    if ((dir >> 1) == 0) {
        v1 = m_aTri[startTri].v1;
        v2 = m_aTri[startTri].v2;
        v3 = m_aTri[startTri].v3;
    } else if ((dir >> 1) == 1) {
        v1 = m_aTri[startTri].v2;
        v2 = m_aTri[startTri].v3;
        v3 = m_aTri[startTri].v1;
    } else {
        v1 = m_aTri[startTri].v3;
        v2 = m_aTri[startTri].v1;
        v3 = m_aTri[startTri].v2;
    }

    while (trino != -1) {
        if (pwOut) {
            *pwOut++ = m_aVert[v1].wIndex;
            *pwOut++ = m_aVert[v2].wIndex;
            *pwOut++ = m_aVert[v3].wIndex;
        }
        len++;
        m_aTri[trino].done = 2;
        if (len >= nMaxLen)
            break;

        if (len & 1) {
            v1 = v2;
            trino = NextTri(v3, v1, &v2);
        } else {
            v1 = v3;
            trino = NextTri(v1, v2, &v3);
        }
    }

    ClearTempDone(setit);

    return len;
}

// uc_orig: MSMesh::GetStrip (fallen/DDEngine/Source/figure.cpp)
int MSMesh::GetStrip(WORD* pwVT, int maxLen)
{
    int trino;
    int len;
    int dir;

    int bestStart = 0;
    int bestDir = 0;
    int bestLen = 0;

    for (trino = 0; trino < m_nNumTri; trino++) {
        if (m_aTri[trino].done)
            continue;
        for (dir = 0; dir < 3; dir++) {
            if ((len = StripLen(trino, dir, 0, NULL, maxLen)) > bestLen) {
                bestLen = len;
                bestStart = trino;
                bestDir = dir;
                if (bestLen >= maxLen) {
                    trino = m_nNumTri;
                    break;
                }
            }
        }
    }
    if (bestLen > 0)
        StripLen(bestStart, bestDir, 1, pwVT, maxLen);

    return bestLen;
}

// uc_orig: MSMesh::NextTri (fallen/DDEngine/Source/figure.cpp)
int MSMesh::NextTri(int v1, int v2, int* v3)
{
    int subtri;
    int trino;

    for (subtri = 0; subtri < m_aVert[v1].nShareCount; subtri++) {
        trino = m_aVert[v1].rgSharedTris[subtri];
        if (m_aTri[trino].done)
            continue;
        if (m_aTri[trino].v1 == v1) {
            if (m_aTri[trino].v2 == v2) {
                *v3 = m_aTri[trino].v3;
                return trino;
            }
        } else if (m_aTri[trino].v2 == v1) {
            if (m_aTri[trino].v3 == v2) {
                *v3 = m_aTri[trino].v1;
                return trino;
            }
        } else if (m_aTri[trino].v3 == v1) {
            if (m_aTri[trino].v1 == v2) {
                *v3 = m_aTri[trino].v2;
                return trino;
            }
        }
    }
    return -1;
}

// uc_orig: MSOptimizeIndexedList (fallen/DDEngine/Source/figure.cpp)
// Reorders pwIndices (nTriangles*3 indices) into vertex-cache-friendly strips in-place.
BOOL MSOptimizeIndexedList(WORD* pwIndices, int nTriangles)
{
    if (!g_mesh.SetSize(nTriangles))
        return UC_FALSE;

    g_mesh.Clear();

    WORD* pwInd = pwIndices;

    for (int i = 0; i < nTriangles; i++) {
        g_mesh.AddTri(pwInd[0], pwInd[1], pwInd[2]);
        pwInd += 3;
    }

    if (g_mesh.NumVertices() == 0)
        return UC_FALSE;

    int nLength = nTriangles;
    int nTotal = 0;

    while (nTotal < nTriangles) {
        nLength = g_mesh.GetStrip(pwIndices, nLength);
        pwIndices += (nLength * 3);
        nTotal += nLength;
    }

    return UC_TRUE;
}

// uc_orig: get_steam_rand (fallen/DDEngine/Source/figure.cpp)
// Cheap LCG pseudo-random number; steam_seed is reset per draw call for reproducible patterns.
SLONG get_steam_rand(void)
{
    steam_seed *= 31415965;
    steam_seed += 123456789;
    return (steam_seed >> 8);
}

extern int AENG_detail_crinkles;

// uc_orig: MAX_STEAM (fallen/DDEngine/Source/figure.cpp)
#define MAX_STEAM 100

// uc_orig: draw_steam (fallen/DDEngine/Source/figure.cpp)
// Draws a column of animated steam/vapour sprites above world position (x,y,z).
// lod: number of sprites to draw, capped at 40.
void draw_steam(SLONG x, SLONG y, SLONG z, SLONG lod)
{
    SLONG c0;
    float u, v;
    SLONG trans;

    POLY_set_local_rotation_none();

    steam_seed = 54321678;
    if (lod > 40)
        lod = 40;

    for (c0 = 0; c0 < lod; c0++) {
        SLONG dx, dy, dz;

        if (c0 & 1)
            u = 0.0;
        else
            u = 0.5;
        if (c0 & 2)
            v = 0.0;
        else
            v = 0.5;

        dy = get_steam_rand() & 0x1ff;
        dy += (VISUAL_TURN * ((c0 & 3) + 2));
        dy %= 500;
        dx = (((get_steam_rand() & 0xff) - 128) * ((dy >> 2) + 80)) >> 9;
        dz = (((get_steam_rand() & 0xff) - 128) * ((dy >> 2) + 80)) >> 9;

        if (dy > 16) {
        }

        if (c0 & 4) {
            dx += COS(dy * 4);
            dz += SIN(dy * 4);
        }

        if (dy > 500 - (13 * 16)) {
            trans = (500 - dy) >> 4;
        }
        if (dy < 13 * 4) {
            trans = dy >> 2;
        } else {
            trans = 13;
        }

        dx += x;
        dy += y;
        dz += z;

        if (trans >= 1) {
            trans |= trans << 8;
            trans |= trans << 8;

            SPRITE_draw_tex(
                float(dx),
                float(dy),
                float(dz),
                float(((dy - y) >> 2) + 150),
                trans,
                0xff000000, // specular.a = 1.0: no CPU fog applied, use table fog
                POLY_PAGE_STEAM,
                u, v, 0.5, 0.5,
                SPRITE_SORT_NORMAL);
        }
    }
}

// uc_orig: init_flames (fallen/DDEngine/Source/figure.cpp)
// Loads the 256-entry RGB flame palette from data\flames1.pal into fire_pal[].
void init_flames()
{
    MFFileHandle handle = FILE_OPEN_ERROR;

    handle = FileOpen("data/flames1.pal");

    if (handle != FILE_OPEN_ERROR) {
        FileRead(handle, (UBYTE*)&fire_pal, 768);

        FileClose(handle);
    }
}

extern int AENG_detail_skyline;

// uc_orig: draw_flames (fallen/DDEngine/Source/figure.cpp)
// Draws layered fire sprites at world position (x,y,z).
// lod: sprite count. offset: seeds per-fire variation.
// Types 0,3 = large animated sprite; type 1 = outer; type 2 = inner.
void draw_flames(SLONG x, SLONG y, SLONG z, SLONG lod, SLONG offset)
{
    SLONG c0;
    SLONG trans;
    SLONG page;
    float scale;
    float u, v, w, h;
    SLONG palndx;
    float wx1, wy1, wx2, wy2, wx3, wy3, wx4, wy4;
    UBYTE type;

    steam_seed = 54321678 + offset;

    for (c0 = 0; c0 < lod; c0++) {
        SLONG dx, dy, dz;

        w = h = 1.0;
        u = v = 0.0;

        type = c0 & 3;

        {
            if ((type == 1) || (type == 2))
                page = POLY_PAGE_FLAMES2;
            else
                page = POLY_PAGE_PCFLAMER;
            dy = (VISUAL_TURN + c0) / 2;
            u = 0.25f * (dy & 3);
            v = 0.25f * ((dy & !3) / 3);
            v += 0.002f;
            w = h = 0.25f;
            dy = 0;
        }

        dy = get_steam_rand() & 0x1ff;
        dy += (VISUAL_TURN * 5);
        dy %= 500;
        dx = (((get_steam_rand() & 0xff) - 128) * ((dy >> 2) + 150)) >> 9;
        dz = (((get_steam_rand() & 0xff) - 128) * ((dy >> 2) + 150)) >> 9;
        if (type == 2) {
            dx >>= 2;
            dz >>= 2;
        }
        if (type == 1) {
            dx >>= 1;
            dz >>= 1;
        }

        wy1 = wy2 = wy3 = wy4 = 0;
        wx1 = (float)COS(((dy + c0) * 11) & 1023) * 0.0001f;
        wx2 = (float)COS(((dy + c0) * 10) & 1023) * 0.0001f;
        wx3 = (float)COS(((dy + c0) * 8) & 1023) * 0.0001f;
        wx4 = (float)COS(((dy + c0) * 9) & 1023) * 0.0001f;

        trans = 255 - dy;

        dx += x;
        dy += y;
        dz += z;

        if (trans >= 1) {
            switch (type) {
            case 1:
                dy >>= 1;
            case 2:
                palndx = (256 - trans) * 3;
                trans <<= 24;
                trans += (fire_pal[palndx] << 16) + (fire_pal[palndx + 1] << 8) + fire_pal[palndx + 2];
                scale = 50;
                break;
            case 0:
            case 3:
                trans = abs(dy - y) * 20;
                trans = SIN(trans & 2047);
                trans |= 0x00FFFFFF;
                scale = 100;
                dy = y;
                break;
            }

            SPRITE_draw_tex_distorted(
                float(dx),
                float(dy),
                float(dz),
                float(scale),
                trans,
                0xff000000,
                page,
                u, v, w, h,
                wx1, wy1, wx2, wy2, wx3, wy3, wx4, wy4,
                SPRITE_SORT_NORMAL);
        }
    }
}

// uc_orig: draw_flame_element (fallen/DDEngine/Source/figure.cpp)
// Draws a single animated fire/smoke sprite (used by fire.cpp/pyro.cpp per frame).
// c0: frame index. base: 0=FLAMES, 1=FLAMES2. rand: 1=random seed variation.
void draw_flame_element(SLONG x, SLONG y, SLONG z, SLONG c0, UBYTE base, UBYTE rand)
{
    SLONG trans;
    SLONG page;
    float scale;
    float u, v, w, h;
    SLONG palndx;
    float wx1, wy1, wx2, wy2, wx3, wy3, wx4, wy4;
    SLONG dx, dy, dz;

    if (rand)
        steam_seed = 54321678 + (c0 * get_steam_rand());
    else
        steam_seed = 54321678 + c0;

    w = h = 1.0;
    u = v = 0.0;

    if (!(c0 & 3))
        page = POLY_PAGE_FLAMES;
    else if (c0 & 4)
        page = POLY_PAGE_SMOKE;
    else {
        if (!base) {
            page = POLY_PAGE_FLAMES;
        } else {
            page = POLY_PAGE_FLAMES2;
            dy = (VISUAL_TURN + c0) / 2;
            u = 0.25f * (dy & 3);
            v = 0.25f * ((dy >> 2) & 3);
            w = h = 0.25f;
        }
    }

    dy = get_steam_rand() & 0x1ff;
    dy += (VISUAL_TURN * 5);
    dy %= 500;
    dx = (((get_steam_rand() & 0xff) - 128) * ((dy >> 2) + 150)) >> 9;
    dz = (((get_steam_rand() & 0xff) - 128) * ((dy >> 2) + 150)) >> 9;
    if (page == POLY_PAGE_FLAMES) {
        dx >>= 2;
        dz >>= 2;
    }

    wx1 = wx2 = wx3 = wx4 = 0;
    wy1 = wy2 = wy3 = wy4 = 0;
    wx1 = (float)COS(((dy + c0) * 11) & 1023) * 0.0001f;
    wx2 = (float)COS(((dy + c0) * 10) & 1023) * 0.0001f;
    wx3 = (float)COS(((dy + c0) * 8) & 1023) * 0.0001f;
    wx4 = (float)COS(((dy + c0) * 9) & 1023) * 0.0001f;

    trans = 255 - dy;

    dx += x;
    dy += y;
    dz += z;

    if (trans >= 1) {
        switch (page) {
        case POLY_PAGE_FLAMES:
            palndx = (256 - trans) * 3;
            trans <<= 24;
            trans += (fire_pal[palndx] << 16) + (fire_pal[palndx + 1] << 8) + fire_pal[palndx + 2];
            scale = 50;
            break;
        case POLY_PAGE_FLAMES2:
            palndx = (256 - trans) * 3;
            trans <<= 24;
            trans += (fire_pal[palndx] << 16) + (fire_pal[palndx + 1] << 8) + fire_pal[palndx + 2];
            scale = 100;
            dy = 50;
            break;
        case POLY_PAGE_SMOKE:
            trans += (trans << 8) | (trans << 16) | (trans << 24);
            scale = float(((dy - y) >> 1) + 50);
            dy += 50;
            break;
        }

        SPRITE_draw_tex_distorted(
            float(dx),
            float(dy),
            float(dz),
            float(scale),
            trans,
            0xff000000,
            page,
            u, v, w, h,
            wx1, wy1, wx2, wy2, wx3, wy3, wx4, wy4,
            SPRITE_SORT_NORMAL);
    }
}

// uc_orig: FIGURE_rotate_obj2 (fallen/DDEngine/Source/figure.cpp)
// Builds a 3x3 rotation matrix from pitch/yaw/roll (game fixed-point 0..2047 = 0..360 degrees).
// Matrix elements are scaled by 32768 (fixed-point 1.0). Jan Svarovsky's rotation order.
void FIGURE_rotate_obj2(SLONG pitch, SLONG yaw, SLONG roll, Matrix33* r3)
{
    SLONG cy, cp, cr;
    SLONG sy, sp, sr;

    cy = COS(yaw & 2047);
    cp = COS(pitch & 2047);
    cr = COS(roll & 2047);

    sy = SIN(yaw & 2047);
    sp = SIN(pitch & 2047);
    sr = SIN(roll & 2047);

    r3->M[0][0] = (MUL64(cy, cr) + MUL64(MUL64(sy, sp), sr)) >> 1;
    r3->M[0][1] = (MUL64(cy, sr) - MUL64(MUL64(sy, sp), cr)) >> 1;
    r3->M[0][2] = (MUL64(sy, cp)) >> 1;
    r3->M[1][0] = (MUL64(-cp, sr)) >> 1;
    r3->M[1][1] = (MUL64(cp, cr)) >> 1;
    r3->M[1][2] = (sp) >> 1;
    r3->M[2][0] = (MUL64(-sy, cr) + MUL64(MUL64(cy, sp), sr)) >> 1;
    r3->M[2][1] = (MUL64(-sy, sr) - MUL64(MUL64(cy, sp), cr)) >> 1;
    r3->M[2][2] = (MUL64(cy, cp)) >> 1;
}

// uc_orig: FIGURE_rotate_obj (fallen/DDEngine/Source/figure.cpp)
// Legacy wrapper: immediately calls FIGURE_rotate_obj2 (old integer-only path below is dead code).
void FIGURE_rotate_obj(SLONG xangle, SLONG yangle, SLONG zangle, Matrix33* r3)
{
    SLONG sinx, cosx, siny, cosy, sinz, cosz;
    SLONG cxcz, sysz, sxsycz, sxsysz, sysx, cxczsy, sxsz, cxsysz, czsx, cxsy, sycz, cxsz;

    FIGURE_rotate_obj2(xangle, yangle, zangle, r3);
    return;

    sinx = SIN(xangle & (2048 - 1)) >> 1;
    cosx = COS(xangle & (2048 - 1)) >> 1;
    siny = SIN(yangle & (2048 - 1)) >> 1;
    cosy = COS(yangle & (2048 - 1)) >> 1;
    sinz = SIN(zangle & (2048 - 1)) >> 1;
    cosz = COS(zangle & (2048 - 1)) >> 1;

    cxsy = (cosx * cosy);
    sycz = (cosy * cosz);
    cxcz = (cosx * cosz);
    cxsz = (cosx * sinz);
    czsx = (cosz * sinx);
    sysx = (cosy * sinx);
    sysz = (cosy * sinz);
    sxsz = (sinx * sinz);
    sxsysz = (sxsz >> 15) * siny;
    cxczsy = (cxcz >> 15) * siny;
    cxsysz = ((cosx * siny) >> 15) * sinz;
    sxsycz = (czsx >> 15) * siny;

    r3->M[0][0] = (sycz) >> 15;
    r3->M[0][1] = (-sysz) >> 15;
    r3->M[0][2] = siny;
    r3->M[1][0] = (sxsycz + cxsz) >> 15;
    r3->M[1][1] = (cxcz - sxsysz) >> 15;
    r3->M[1][2] = (-sysx) >> 15;
    r3->M[2][0] = (sxsz - cxczsy) >> 15;
    r3->M[2][1] = (cxsysz + czsx) >> 15;
    r3->M[2][2] = (cxsy) >> 15;
}

extern UBYTE TEXTURE_dontexist[];

// uc_orig: FIGURE_find_face_D3D_texture_page (fallen/DDEngine/Source/figure.cpp)
// Resolves the D3D texture page for a prim face (tri or quad).
// Returns a UWORD combining page index and flag bits:
//   TEXTURE_PAGE_FLAG_JACKET  — gang jacket (page depends on NPC skill)
//   TEXTURE_PAGE_FLAG_OFFSET  — alternate clothes colour (pcom_colour-based)
//   TEXTURE_PAGE_FLAG_TINT    — uses tinted lighting table
//   TEXTURE_PAGE_FLAG_NOT_TEXTURED — flat colour face
UWORD FIGURE_find_face_D3D_texture_page(int iFaceNum, bool bTri)
{
    UWORD wTexturePage;
    DWORD dwDrawFlags, dwFaceFlags;
    if (bTri) {
        PrimFace3* p_f3 = &prim_faces3[iFaceNum];
        dwDrawFlags = p_f3->DrawFlags;
        dwFaceFlags = p_f3->FaceFlags;

        wTexturePage = p_f3->UV[0][0] & 0xc0;
        wTexturePage <<= 2;
        wTexturePage |= p_f3->TexturePage;
    } else {
        PrimFace4* p_f4 = &prim_faces4[iFaceNum];
        dwDrawFlags = p_f4->DrawFlags;
        dwFaceFlags = p_f4->FaceFlags;

        wTexturePage = p_f4->UV[0][0] & 0xc0;
        wTexturePage <<= 2;
        wTexturePage |= p_f4->TexturePage;
    }

    if ((dwDrawFlags & POLY_FLAG_TEXTURED) == 0) {
        wTexturePage = TEXTURE_PAGE_FLAG_NOT_TEXTURED | POLY_PAGE_COLOUR;
    } else {
        if (dwFaceFlags & FACE_FLAG_THUG_JACKET) {
            switch (wTexturePage) {
            case 64 + 21:
            case 10 * 64 + 2:
            case 10 * 64 + 32:
                wTexturePage = 0 | TEXTURE_PAGE_FLAG_JACKET;
                break;
            case 64 + 22:
            case 10 * 64 + 3:
            case 10 * 64 + 33:
                wTexturePage = 1 | TEXTURE_PAGE_FLAG_JACKET;
                break;
            case 64 + 24:
            case 10 * 64 + 4:
            case 10 * 64 + 36:
                wTexturePage = 2 | TEXTURE_PAGE_FLAG_JACKET;
                break;
            case 64 + 25:
            case 10 * 64 + 5:
            case 10 * 64 + 37:
                wTexturePage = 3 | TEXTURE_PAGE_FLAG_JACKET;
                break;
            default:
                ASSERT(UC_FALSE);
                break;
            }
        } else if ((wTexturePage > 10 * 64) && (alt_texture[wTexturePage - 10 * 64] != 0)) {
            ASSERT((wTexturePage & ~TEXTURE_PAGE_MASK) == 0);
            wTexturePage |= TEXTURE_PAGE_FLAG_OFFSET;
        } else {
            wTexturePage += FACE_PAGE_OFFSET;
            ASSERT((wTexturePage & ~TEXTURE_PAGE_MASK) == 0);
        }
    }

    if (dwFaceFlags & FACE_FLAG_TINT) {
        wTexturePage |= TEXTURE_PAGE_FLAG_TINT;
    }

    return (wTexturePage);
}

// Phase 2 P2-C helper: build the per-frame world-skin palette.
//   skin[i] = current[i] * inv_bind[i]
// in "M * v" convention, output as 3 vec4 per bone (rotation rows with
// translation packed in .w) — same layout the shadow-silhouette path
// uploads. current[i] is the interpolated world transform from the pose
// snapshot (Matrix33 ×32768, float pos); inv_bind[i] is a GEMatrix in
// "M * v" form (R top-left at scale 1.0, t in column 4 — bind_palette.cpp).
// Output rotation lands at scale 1.0 (we divide current's ×32768 here).
static void figure_build_skin_world_palette(
    const BoneInterpTransform* current,
    const GEMatrix*            inv_bind,
    int                        bone_count,
    float*                     out_palette)
{
    constexpr float S = 1.0f / 32768.0f;
    for (int i = 0; i < bone_count; ++i) {
        const Matrix33& cR  = current[i].rot;
        const GEMatrix& bi  = inv_bind[i];
        const float pos[3]  = { current[i].pos_x, current[i].pos_y, current[i].pos_z };

        for (int r = 0; r < 3; ++r) {
            float* o = &out_palette[(i * 3 + r) * 4];
            // R_skin[r][c] = (1/32768) * sum_k cR.M[r][k] * bi.m[k][c]
            const float cR0 = float(cR.M[r][0]);
            const float cR1 = float(cR.M[r][1]);
            const float cR2 = float(cR.M[r][2]);
            o[0] = S * (cR0 * bi.m[0][0] + cR1 * bi.m[1][0] + cR2 * bi.m[2][0]);
            o[1] = S * (cR0 * bi.m[0][1] + cR1 * bi.m[1][1] + cR2 * bi.m[2][1]);
            o[2] = S * (cR0 * bi.m[0][2] + cR1 * bi.m[1][2] + cR2 * bi.m[2][2]);
            // t_skin[r] = (1/32768) * sum_k cR.M[r][k] * bi.m[k][3] + current.pos[r]
            o[3] = S * (cR0 * bi.m[0][3] + cR1 * bi.m[1][3] + cR2 * bi.m[2][3]) + pos[r];
        }
    }

    // Skeleton debug overlay — see g_skin_debug_draw_skeleton declaration
    // in bind_palette.h. Single early gate: when the toggle is off the
    // entire block is skipped, costing exactly one branch per call (zero
    // POLY_transform, zero AENG_world_*, zero matrix save/restore).
    //
    // Draws the animated skeleton on top of the model: bone lines
    // parent → child plus per-bone wireframe spheres at each joint.
    // `current[i].pos_*` is the world-space joint position in the current
    // pose, so connecting it to `current[body_part_parent[i]]` gives the
    // bone segment directly.
    //
    // g_matWorld save/restore: at this point in the FK recurse, g_matWorld
    // holds the last-bone local transform. POLY_transform inside
    // AENG_world_line multiplies its inputs by g_matWorld, so absolute
    // world coords would be double-transformed. POLY_set_local_rotation_none
    // writes the camera-only matrix (object at world identity).
    // Restoring afterwards is mandatory — the per-material fog calc in
    // the caller reads g_matWorld._43 (same gotcha figure_build_screen_xform_bake
    // handles for fog).
    // Skeleton overlay is hard-coded to the 15-bone person rig
    // (body_part_parent[], BONE_COLOURS[]); skip for flat-skeleton
    // creatures (Bane / Balrog / bats / Gargoyle) where the parent
    // table is meaningless.
    if (g_skin_debug_draw_skeleton && bone_count == POSE_PERSON_BONE_COUNT) {
        extern GEMatrix g_matWorld;
        const GEMatrix saved_world = g_matWorld;
        POLY_set_local_rotation_none();

        // Per-bone colour palette. Symmetric bones share a hue with
        // brightness paired LEFT = bright, RIGHT = dark, so it's obvious
        // at a glance which side you're looking at. Center-line bones
        // (pelvis, torso, head) get distinct neutral / high-contrast
        // colours so they don't get confused with the pairs. Indices
        // match body_part_parent[] in pose_composer.cpp.
        //
        // Pairing key:
        //   femur  (1/12) red       tibia (2/13) green   foot  (3/14) orange
        //   humer  (5/8)  blue      radius(6/9)  cyan    hand  (7/10) magenta
        //   pelvis(0)  white         torso(4)  light gray         head(11) yellow
        static const ULONG BONE_COLOURS[POSE_PERSON_BONE_COUNT] = {
            0xffffffff, //  0 PELVIS         — white (root)
            0xffff5050, //  1 LEFT_FEMUR     — bright red
            0xff50ff50, //  2 LEFT_TIBIA     — bright green
            0xffffaa00, //  3 LEFT_FOOT      — bright orange
            0xffaaaaaa, //  4 TORSO          — light gray
            0xff5080ff, //  5 LEFT_HUMORUS   — bright blue
            0xff50ffff, //  6 LEFT_RADIUS    — bright cyan
            0xffff50ff, //  7 LEFT_HAND      — bright magenta
            0xff002080, //  8 RIGHT_HUMORUS  — dark blue
            0xff008080, //  9 RIGHT_RADIUS   — dark cyan
            0xff800080, // 10 RIGHT_HAND     — dark magenta
            0xffffff00, // 11 HEAD           — yellow
            0xff800000, // 12 RIGHT_FEMUR    — dark red
            0xff008000, // 13 RIGHT_TIBIA    — dark green
            0xff804400, // 14 RIGHT_FOOT     — dark orange / brown
        };

        // Bones — thin lines parent → child, coloured by the CHILD bone
        // (so each line carries its own bone's identity, not the parent's).
        // 1 px width — minimal, doesn't clutter the model silhouette.
        constexpr SLONG BONE_LINE_PX = 1;
        for (int i = 0; i < POSE_PERSON_BONE_COUNT; ++i) {
            const int p = body_part_parent[i];
            if (p < 0) continue; // root (PELVIS) — no parent line to draw
            const ULONG colour = BONE_COLOURS[i];
            AENG_world_line(
                (SLONG)current[p].pos_x, (SLONG)current[p].pos_y, (SLONG)current[p].pos_z,
                BONE_LINE_PX, colour,
                (SLONG)current[i].pos_x, (SLONG)current[i].pos_y, (SLONG)current[i].pos_z,
                BONE_LINE_PX, colour,
                UC_TRUE);
        }

        // Joints — wireframe spheres at each bone's pivot via the pure-
        // debug AENG_world_sphere primitive (3 perpendicular great
        // circles). Radius 7 in MS units — small bead per joint, doesn't
        // fuse into a blob when limbs are bent close together.
        constexpr SLONG JOINT_BALL_RADIUS  = 4;
        constexpr SLONG JOINT_BALL_LINE_PX = 1;
        for (int i = 0; i < POSE_PERSON_BONE_COUNT; ++i) {
            AENG_world_sphere(
                (SLONG)current[i].pos_x,
                (SLONG)current[i].pos_y,
                (SLONG)current[i].pos_z,
                JOINT_BALL_RADIUS,
                JOINT_BALL_LINE_PX,
                BONE_COLOURS[i]);
        }

        // Per-bone orientation gizmo — three short axis lines at each
        // joint show the bone's full local frame, so twist (rotation
        // around the bone direction) is visible, not just the bone
        // direction. Standard convention: local X = red, Y = green,
        // Z = blue. Basis vectors are the columns of current[i].rot
        // (M·v form, ×32768 scaled — divide before use). Length 30 in
        // MS units, a bit longer than the joint ball radius (7) so the
        // axes stick out clearly. 1 px width.
        constexpr SLONG AXIS_LENGTH    = 10;
        constexpr SLONG AXIS_LINE_PX   = 1;
        constexpr float AXIS_INV_SCALE = 1.0f / 32768.0f;
        for (int i = 0; i < POSE_PERSON_BONE_COUNT; ++i) {
            const Matrix33& R = current[i].rot;
            const float cx = current[i].pos_x;
            const float cy = current[i].pos_y;
            const float cz = current[i].pos_z;

            // Columns of R = world directions of local X / Y / Z axes.
            const float ax = float(R.M[0][0]) * AXIS_INV_SCALE;
            const float ay = float(R.M[1][0]) * AXIS_INV_SCALE;
            const float az = float(R.M[2][0]) * AXIS_INV_SCALE;
            const float bx = float(R.M[0][1]) * AXIS_INV_SCALE;
            const float by = float(R.M[1][1]) * AXIS_INV_SCALE;
            const float bz = float(R.M[2][1]) * AXIS_INV_SCALE;
            const float gx = float(R.M[0][2]) * AXIS_INV_SCALE;
            const float gy = float(R.M[1][2]) * AXIS_INV_SCALE;
            const float gz = float(R.M[2][2]) * AXIS_INV_SCALE;

            const float L = float(AXIS_LENGTH);

            // Local X — red
            AENG_world_line(
                (SLONG)cx, (SLONG)cy, (SLONG)cz, AXIS_LINE_PX, 0xffff0000,
                (SLONG)(cx + ax * L), (SLONG)(cy + ay * L), (SLONG)(cz + az * L),
                AXIS_LINE_PX, 0xffff0000,
                UC_TRUE);
            // Local Y — green
            AENG_world_line(
                (SLONG)cx, (SLONG)cy, (SLONG)cz, AXIS_LINE_PX, 0xff00ff00,
                (SLONG)(cx + bx * L), (SLONG)(cy + by * L), (SLONG)(cz + bz * L),
                AXIS_LINE_PX, 0xff00ff00,
                UC_TRUE);
            // Local Z — blue
            AENG_world_line(
                (SLONG)cx, (SLONG)cy, (SLONG)cz, AXIS_LINE_PX, 0xff0000ff,
                (SLONG)(cx + gx * L), (SLONG)(cy + gy * L), (SLONG)(cz + gz * L),
                AXIS_LINE_PX, 0xff0000ff,
                UC_TRUE);
        }

        g_matWorld = saved_world;
    }
}

// Phase 2 P2-C helper: build the per-character screen-xform bake — the
// camera*projection*viewport matrix shared by every bone (no per-bone
// transform baked in; the skin palette handles per-bone). Uses
// CAMERA-ONLY g_matWorld (via POLY_set_local_rotation_none).
//
// Saves & restores g_matWorld around the camera-only reset: the
// per-material loop in the caller reads g_matWorld._43 to set
// g_mm_fog_view_z (per-character fog Z), and that needs to keep the
// LAST BONE's transform that the legacy recurse left behind, not the
// camera-only state we'd otherwise leak out of this helper. Without the
// restore, fog factor would track the camera's world-Z offset instead
// of the character's view-Z — visible as the model fading to fog color
// (often black) as the camera orbits around them.
static void figure_build_screen_xform_bake(GEMatrix* out)
{
    extern GEMatrix g_matProjection;
    extern GEMatrix g_matWorld;
    extern GEViewport g_viewData;
    extern DWORD g_dw3DStuffHeight;
    extern DWORD g_dw3DStuffY;

    // Save the caller's g_matWorld so the side-effect of
    // POLY_set_local_rotation_none() doesn't reach the per-material fog
    // calculation that runs after we return.
    const GEMatrix saved_world = g_matWorld;

    // Camera-only world matrix — wipes any per-bone offset/rotation left in
    // g_matWorld from a previous _just_set_matrix call.
    POLY_set_local_rotation_none();

    // matTemp = g_matWorld * g_matProjection. Same byte-for-byte expression
    // as figure.cpp:3470-3488.
    GEMatrix matTemp;
    matTemp._11 = g_matWorld._11 * g_matProjection._11 + g_matWorld._12 * g_matProjection._21 + g_matWorld._13 * g_matProjection._31 + g_matWorld._14 * g_matProjection._41;
    matTemp._12 = g_matWorld._11 * g_matProjection._12 + g_matWorld._12 * g_matProjection._22 + g_matWorld._13 * g_matProjection._32 + g_matWorld._14 * g_matProjection._42;
    matTemp._13 = g_matWorld._11 * g_matProjection._13 + g_matWorld._12 * g_matProjection._23 + g_matWorld._13 * g_matProjection._33 + g_matWorld._14 * g_matProjection._43;
    matTemp._14 = g_matWorld._11 * g_matProjection._14 + g_matWorld._12 * g_matProjection._24 + g_matWorld._13 * g_matProjection._34 + g_matWorld._14 * g_matProjection._44;
    matTemp._21 = g_matWorld._21 * g_matProjection._11 + g_matWorld._22 * g_matProjection._21 + g_matWorld._23 * g_matProjection._31 + g_matWorld._24 * g_matProjection._41;
    matTemp._22 = g_matWorld._21 * g_matProjection._12 + g_matWorld._22 * g_matProjection._22 + g_matWorld._23 * g_matProjection._32 + g_matWorld._24 * g_matProjection._42;
    matTemp._23 = g_matWorld._21 * g_matProjection._13 + g_matWorld._22 * g_matProjection._23 + g_matWorld._23 * g_matProjection._33 + g_matWorld._24 * g_matProjection._43;
    matTemp._24 = g_matWorld._21 * g_matProjection._14 + g_matWorld._22 * g_matProjection._24 + g_matWorld._23 * g_matProjection._34 + g_matWorld._24 * g_matProjection._44;
    matTemp._31 = g_matWorld._31 * g_matProjection._11 + g_matWorld._32 * g_matProjection._21 + g_matWorld._33 * g_matProjection._31 + g_matWorld._34 * g_matProjection._41;
    matTemp._32 = g_matWorld._31 * g_matProjection._12 + g_matWorld._32 * g_matProjection._22 + g_matWorld._33 * g_matProjection._32 + g_matWorld._34 * g_matProjection._42;
    matTemp._33 = g_matWorld._31 * g_matProjection._13 + g_matWorld._32 * g_matProjection._23 + g_matWorld._33 * g_matProjection._33 + g_matWorld._34 * g_matProjection._43;
    matTemp._34 = g_matWorld._31 * g_matProjection._14 + g_matWorld._32 * g_matProjection._24 + g_matWorld._33 * g_matProjection._34 + g_matWorld._34 * g_matProjection._44;
    matTemp._41 = g_matWorld._41 * g_matProjection._11 + g_matWorld._42 * g_matProjection._21 + g_matWorld._43 * g_matProjection._31 + g_matWorld._44 * g_matProjection._41;
    matTemp._42 = g_matWorld._41 * g_matProjection._12 + g_matWorld._42 * g_matProjection._22 + g_matWorld._43 * g_matProjection._32 + g_matWorld._44 * g_matProjection._42;
    matTemp._43 = g_matWorld._41 * g_matProjection._13 + g_matWorld._42 * g_matProjection._23 + g_matWorld._43 * g_matProjection._33 + g_matWorld._44 * g_matProjection._43;
    matTemp._44 = g_matWorld._41 * g_matProjection._14 + g_matWorld._42 * g_matProjection._24 + g_matWorld._43 * g_matProjection._34 + g_matWorld._44 * g_matProjection._44;

    const DWORD dwWidth  = g_viewData.dwWidth  >> 1;
    const DWORD dwHeight = g_dw3DStuffHeight   >> 1;
    const DWORD dwX      = g_viewData.dwX;
    const DWORD dwY      = g_dw3DStuffY;

    // Viewport bake — column 1 (._{1,1}/._{2,1}/._{3,1}/._{4,1}) is unused
    // by the shader (column-pick reads columns 2/3/4), keep at 0.
    out->_11 = 0.0f;
    out->_12 = matTemp._11 * (float)dwWidth  + matTemp._14 * (float)(dwX + dwWidth);
    out->_13 = matTemp._12 * -(float)dwHeight + matTemp._14 * (float)(dwY + dwHeight);
    out->_14 = matTemp._14;
    out->_21 = 0.0f;
    out->_22 = matTemp._21 * (float)dwWidth  + matTemp._24 * (float)(dwX + dwWidth);
    out->_23 = matTemp._22 * -(float)dwHeight + matTemp._24 * (float)(dwY + dwHeight);
    out->_24 = matTemp._24;
    out->_31 = 0.0f;
    out->_32 = matTemp._31 * (float)dwWidth  + matTemp._34 * (float)(dwX + dwWidth);
    out->_33 = matTemp._32 * -(float)dwHeight + matTemp._34 * (float)(dwY + dwHeight);
    out->_34 = matTemp._34;
    out->_41 = 0.0f;
    out->_42 = matTemp._41 * (float)dwWidth  + matTemp._44 * (float)(dwX + dwWidth);
    out->_43 = matTemp._42 * -(float)dwHeight + matTemp._44 * (float)(dwY + dwHeight);
    out->_44 = matTemp._44;

    // Restore caller's g_matWorld so downstream fog setup
    // (g_mm_fog_view_z = g_matWorld._43) sees the last-bone view-Z left
    // by the legacy per-bone recurse, not our camera-only override.
    g_matWorld = saved_world;
    ge_set_transform(GETransform::World, &g_matWorld);
}

// Build the world-skin consolidated GPU mesh for one TomsPrimObject.
// Every vertex (and normal) is pre-multiplied by bind_world[bMatIndex]
// of the rig's chunk, so the resulting mesh lives in a single shared
// bind-space and is ready for the world-skin shader
// (skin_world_vert.glsl).
//
// Works for both 15-bone person rigs (DARCI/ROPER/ROPER2/CIV) and flat
// skeletons (Bane, Balrog, bats, Gargoyle, ...). The chunk pointer
// identifies which rig — the bind palette infers the right derivation
// internally (A-pose FK for 15-bone, identity-rot + raw offsets for
// flat). On flat rigs the per-leaf-joint auto-rig further down is a
// no-op because none of the parent_part / leaf_part indices appear in
// the mesh's bone references.
//
// One VBO + per-material index slices stored as 2 × wNumMaterials
// uint32 (index_start, index_count) pairs in skin_consolidated_ranges.
// On success stores the GPU mesh on pPrimObj->skin_consolidated_world;
// failure (alloc, > 65535 verts, missing bind palette) leaves it NULL —
// caller falls back to the per-material multi-matrix path.
static bool figure_build_consolidated_skin_world(TomsPrimObject* pPrimObj,
                                                 const GameKeyFrameChunk* chunk)
{
    if (pPrimObj->skin_consolidated_world != NULL)
        return true;
    if (pPrimObj->wNumMaterials == 0 || pPrimObj->pMaterials == NULL
        || pPrimObj->pD3DVertices == NULL || pPrimObj->pwStripIndices == NULL)
        return false;

    const GEMatrix* bind_world = NULL;
    int             bone_count = 0;
    if (!bind_palette_get(chunk, &bind_world, NULL, &bone_count) || !bind_world)
        return false;

    GEVertex* pVertex = (GEVertex*)pPrimObj->pD3DVertices;
    UWORD* pwStripIndices = pPrimObj->pwStripIndices;
    PrimObjectMaterial* pMat = pPrimObj->pMaterials;

    std::vector<GESkinVertex> verts;
    std::vector<uint16_t>     inds;
    const int n_mats = (int)pPrimObj->wNumMaterials;
    std::vector<uint32_t>     ranges(n_mats * 2, 0u);

    uint32_t palette_n = 0; // max bone index referenced + 1

    for (int iMat = 0; iMat < n_mats; iMat++) {
        const uint16_t base_vertex = (uint16_t)verts.size();
        const uint32_t range_start = (uint32_t)inds.size();

        GEVertex*    pMatVerts    = pVertex;
        GEVertexLit* pMatVertsLit = (GEVertexLit*)pMatVerts;
        for (uint32_t v = 0; v < pMat->wNumVertices; v++) {
            BYTE bMatIndex = ((unsigned char*)(pMatVertsLit + v))[12];
            // Out-of-range bone index means the loader produced something
            // this rig can't handle; bail rather than indexing beyond
            // bind_world[bone_count]. ASSERT in debug, fail cleanly in release.
            ASSERT(bMatIndex < bone_count);
            if (bMatIndex >= bone_count)
                return false;

            // Bind transform: v_bind = R * v_local + t.
            // bind_world[bone] uses the "M * v" convention with R in the
            // top-left 3x3 and t in column 4 (see bind_palette.cpp comment
            // on rigid_to_ge). Position uses the full affine, normal uses
            // the rotation-only part.
            const GEMatrix& M = bind_world[bMatIndex];
            const float lx = pMatVerts[v].x;
            const float ly = pMatVerts[v].y;
            const float lz = pMatVerts[v].z;
            const float lnx = pMatVerts[v].nx;
            const float lny = pMatVerts[v].ny;
            const float lnz = pMatVerts[v].nz;

            GESkinVertex sv;
            sv.x  = M._11 * lx + M._12 * ly + M._13 * lz + M._14;
            sv.y  = M._21 * lx + M._22 * ly + M._23 * lz + M._24;
            sv.z  = M._31 * lx + M._32 * ly + M._33 * lz + M._34;
            sv.nx = M._11 * lnx + M._12 * lny + M._13 * lnz;
            sv.ny = M._21 * lnx + M._22 * lny + M._23 * lnz;
            sv.nz = M._31 * lnx + M._32 * lny + M._33 * lnz;
            sv.bone     = (uint32_t)bMatIndex;
            sv.color    = 0u;
            sv.specular = 0u;
            sv.u = pMatVertsLit[v].u;
            sv.v = pMatVertsLit[v].v;
            // Initial trivial weights (single bone, full weight). The soft
            // auto-rig pass below overwrites these for verts within a leaf
            // joint's blend zone.
            sv.bones[0]   = bMatIndex; sv.bones[1]   = 0; sv.bones[2]   = 0; sv.bones[3]   = 0;
            sv.weights[0] = 255;       sv.weights[1] = 0; sv.weights[2] = 0; sv.weights[3] = 0;
            verts.push_back(sv);

            if ((uint32_t)bMatIndex + 1 > palette_n)
                palette_n = (uint32_t)bMatIndex + 1;
        }

        // Strip → triangle list decode is identical to the bone-local
        // build; same data source, same winding rules.
        UWORD* p = pwStripIndices;
        uint32_t remaining = pMat->wNumStripIndices;
        while (remaining >= 3) {
            uint16_t wi[3];
            wi[1] = *p++;
            wi[2] = *p++;
            remaining -= 2;
            bool bEven = true;
            while (remaining > 0) {
                bEven = !bEven;
                wi[0] = wi[1];
                wi[1] = wi[2];
                wi[2] = *p++;
                remaining--;
                if (wi[2] == 0xffff)
                    break;
                uint16_t a, b, c;
                if (bEven) {
                    a = wi[0]; b = wi[2]; c = wi[1];
                } else {
                    a = wi[0]; b = wi[1]; c = wi[2];
                }
                inds.push_back((uint16_t)(base_vertex + a));
                inds.push_back((uint16_t)(base_vertex + b));
                inds.push_back((uint16_t)(base_vertex + c));
            }
        }

        ranges[iMat * 2 + 0] = range_start;
        ranges[iMat * 2 + 1] = (uint32_t)inds.size() - range_start;

        pVertex += pMat->wNumVertices;
        pwStripIndices += pMat->wNumStripIndices;
        pMat++;
    }

    if (verts.empty() || inds.empty())
        return false;
    if (verts.size() > 0xFFFFu)
        return false;

    // ----- P2-E auto-rig: soft weights at leaf joints --------------------
    //
    // 15-bone-person-only. Non-people (Bane, Balrog, bats, ...) keep the
    // trivial single-bone weights written above — same shader path, no
    // anatomy assumptions. The pair indices below (HEAD=11, HAND=7/10,
    // FOOT=3/14) are pose_composer.cpp body_part_parent[] indices that
    // exist only on the person rig.
    //
    // Bidirectional targeted blend over 5 (parent_part, leaf_part) pairs.
    // Each pair belongs to a tuning GROUP (mirror pairs share params):
    //   group 0 HEAD:   TORSO       ↔ HEAD
    //   group 1 HANDS:  L/R RADIUS  ↔ L/R HAND
    //   group 2 FEET:   L/R TIBIA   ↔ L/R FOOT
    //
    // For each pair, the leaf JOINT POSITION (in bind space) is the
    // single blend centre. Two independent linear-falloff blends use it:
    //
    //   PARENT-side blend: parent_part verts within parent_band of the
    //     joint pick up share parent_wmax×(1 − d/parent_band) of the
    //     leaf bone's transform. Closes the wrist/ankle/neck "stub
    //     sticking out of body" gap when the leaf moves.
    //
    //   CHILD-side blend: leaf_part verts within child_band of the
    //     joint pick up share child_wmax×(1 − d/child_band) of the
    //     parent bone's transform. Pulls the head/hand/foot mesh
    //     toward the parent so e.g. the bottom of the head model
    //     follows the torso a little when the neck twists.
    //
    // All other joints (shoulders, elbows, hips, knees, pelvis ↔ torso)
    // stay HARD-bound. Keeps the change set small and focused on the
    // seams that actually crease on screen.
    //
    // Defaults are zero → no-op (full hard binding). Per-character mesh
    // calibration happens live via debug keys, then good values land
    // back in bind_palette.cpp as the new defaults.
    //
    // Cost: per pair we scan `verts` up to twice (one side per direction).
    // Build-time only — once per mesh; result baked into the VBO weights.
    if (bone_count == POSE_PERSON_BONE_COUNT) {
        struct LeafPair { int8_t parent_part; int8_t leaf_part; int8_t group; };
        constexpr int LEAF_PAIRS_N = 5;
        // Indices match pose_composer.cpp body_part_parent[] table.
        // PELVIS=0, LEFT_FEMUR=1, LEFT_TIBIA=2, LEFT_FOOT=3,
        // TORSO=4, LEFT_HUMORUS=5, LEFT_RADIUS=6, LEFT_HAND=7,
        // RIGHT_HUMORUS=8, RIGHT_RADIUS=9, RIGHT_HAND=10, HEAD=11,
        // RIGHT_FEMUR=12, RIGHT_TIBIA=13, RIGHT_FOOT=14.
        constexpr LeafPair PAIRS[LEAF_PAIRS_N] = {
            {  4, 11, SKIN_TUNE_GROUP_HEAD  }, // TORSO        ↔ HEAD
            {  6,  7, SKIN_TUNE_GROUP_HANDS }, // LEFT_RADIUS  ↔ LEFT_HAND
            {  9, 10, SKIN_TUNE_GROUP_HANDS }, // RIGHT_RADIUS ↔ RIGHT_HAND
            {  2,  3, SKIN_TUNE_GROUP_FEET  }, // LEFT_TIBIA   ↔ LEFT_FOOT
            { 13, 14, SKIN_TUNE_GROUP_FEET  }, // RIGHT_TIBIA  ↔ RIGHT_FOOT
        };

        for (int k = 0; k < LEAF_PAIRS_N; ++k) {
            const LeafPair& pair = PAIRS[k];
            const SkinTuneGroup& gp = g_skin_tune_groups[pair.group];
            const int parent_part = pair.parent_part;
            const int leaf_part   = pair.leaf_part;

            // Both sides centre on the leaf joint position.
            const float jx = bind_world[leaf_part]._14;
            const float jy = bind_world[leaf_part]._24;
            const float jz = bind_world[leaf_part]._34;

            // PARENT side: parent_part verts blend toward leaf.
            if (gp.parent_band > 0.0f && gp.parent_wmax > 0.0f) {
                for (GESkinVertex& sv : verts) {
                    if ((int)sv.bones[0] != parent_part) continue;
                    const float dx = sv.x - jx;
                    const float dy = sv.y - jy;
                    const float dz = sv.z - jz;
                    const float d  = sqrtf(dx * dx + dy * dy + dz * dz);
                    if (d >= gp.parent_band) continue;
                    const float t      = d / gp.parent_band;
                    const float w_leaf = (1.0f - t) * gp.parent_wmax;
                    const float w_own  = 1.0f - w_leaf;
                    sv.bones[1]   = (uint8_t)leaf_part;
                    sv.weights[0] = (uint8_t)(w_own  * 255.0f + 0.5f);
                    sv.weights[1] = (uint8_t)(w_leaf * 255.0f + 0.5f);
                }
            }

            // CHILD side: leaf_part verts blend toward parent.
            if (gp.child_band > 0.0f && gp.child_wmax > 0.0f) {
                for (GESkinVertex& sv : verts) {
                    if ((int)sv.bones[0] != leaf_part) continue;
                    const float dx = sv.x - jx;
                    const float dy = sv.y - jy;
                    const float dz = sv.z - jz;
                    const float d  = sqrtf(dx * dx + dy * dy + dz * dz);
                    if (d >= gp.child_band) continue;
                    const float t        = d / gp.child_band;
                    const float w_parent = (1.0f - t) * gp.child_wmax;
                    const float w_own    = 1.0f - w_parent;
                    sv.bones[1]   = (uint8_t)parent_part;
                    sv.weights[0] = (uint8_t)(w_own    * 255.0f + 0.5f);
                    sv.weights[1] = (uint8_t)(w_parent * 255.0f + 0.5f);
                }
            }
        }
    }

    GESkinMesh* mesh = ge_skin_mesh_create(
        verts.data(), (uint32_t)verts.size(),
        inds.data(), (uint32_t)inds.size(),
        palette_n);
    if (!mesh)
        return false;

    // Per-material index ranges (2 × n_mats uint32 pairs) — needed by the
    // draw loop to slice the shared VBO. Previously this was populated by
    // the OLD bone-local consolidated build (deleted at P2-J cleanup);
    // now this is the only place that fills it for the world-skin path.
    uint32_t* ranges_storage = (uint32_t*)MemAlloc(ranges.size() * sizeof(uint32_t));
    if (!ranges_storage) {
        ge_skin_mesh_destroy(mesh);
        return false;
    }
    std::memcpy(ranges_storage, ranges.data(), ranges.size() * sizeof(uint32_t));

    // Per-bone bind-space AABB — used by the shadow-silhouette path to
    // build a tight world-space shadow box (transform each bone's 8
    // AABB corners by skin[bone] = current × inv_bind at frame time;
    // see SMAP_person_gpu for the projection code).
    //
    // Vertices grouped by their PRIMARY bone (bones[0]) — for trivial
    // weights this is exact; for soft auto-rigged seam verts the
    // primary bone is still the body part the vertex authored on, so
    // it stays in the right bucket. Soft-blended secondary contributions
    // remain inside the convex hull of the influencing bones' boxes,
    // so the union of per-bone AABBs transformed by skin[*] still
    // bounds the world geometry.
    const size_t aabb_bytes = (size_t)bone_count * 6 * sizeof(float);
    float* bone_aabb = (float*)MemAlloc(aabb_bytes);
    if (!bone_aabb) {
        MemFree(ranges_storage);
        ge_skin_mesh_destroy(mesh);
        return false;
    }
    for (int b = 0; b < bone_count; ++b) {
        bone_aabb[b * 6 + 0] = +1e30f; // min.x
        bone_aabb[b * 6 + 1] = +1e30f; // min.y
        bone_aabb[b * 6 + 2] = +1e30f; // min.z
        bone_aabb[b * 6 + 3] = -1e30f; // max.x
        bone_aabb[b * 6 + 4] = -1e30f; // max.y
        bone_aabb[b * 6 + 5] = -1e30f; // max.z
    }
    for (const GESkinVertex& sv : verts) {
        const int b = (int)sv.bones[0];
        if (b < 0 || b >= bone_count) continue;
        float* mn = &bone_aabb[b * 6 + 0];
        float* mx = &bone_aabb[b * 6 + 3];
        if (sv.x < mn[0]) mn[0] = sv.x;
        if (sv.y < mn[1]) mn[1] = sv.y;
        if (sv.z < mn[2]) mn[2] = sv.z;
        if (sv.x > mx[0]) mx[0] = sv.x;
        if (sv.y > mx[1]) mx[1] = sv.y;
        if (sv.z > mx[2]) mx[2] = sv.z;
    }
    // Bones with no vertices stay at min=+inf / max=-inf; the shadow box
    // code must skip those (min > max → unreferenced bone). For
    // bind-space inv_bind to skin = current × inv_bind to be valid we
    // need bind_palette[bone] anyway, which exists for every bone in
    // the rig — but a body variant might not use every body part, e.g.
    // a head-only model. Skipping unreferenced bones is correct.

    pPrimObj->skin_consolidated_world      = mesh;
    pPrimObj->skin_consolidated_ranges     = ranges_storage;
    pPrimObj->skin_consolidated_bone_aabb  = bone_aabb;
    pPrimObj->skin_consolidated_bone_count = bone_count;
    return true;
}

// uc_orig: FIGURE_clean_LRU_slot (fallen/DDEngine/Source/figure.cpp)
// Frees vertex/index memory for one LRU cache slot and clears its metadata.
void FIGURE_clean_LRU_slot(int iSlot)
{
    TomsPrimObject* ptpo = ptpoLRUQueue[iSlot];

    ASSERT(ptpo != NULL);
    if (ptpo == NULL) {
        return;
    }

    m_dwSizeOfQueue -= (DWORD)ptpo->wTotalSizeOfObj;
    ASSERT((m_dwSizeOfQueue & 0x80000000) == 0);

    ASSERT(ptpo != NULL);
    ASSERT(ptpo->bLRUQueueNumber == iSlot);
    ASSERT(ptpo->wNumMaterials > 0);
    ASSERT(ptpo->pMaterials != NULL);
    ASSERT(ptpo->pwListIndices != NULL);
    // Per-material index ranges array (shared layout used by the world-
    // skin GPU mesh below).
    if (ptpo->skin_consolidated_ranges != NULL) {
        MemFree(ptpo->skin_consolidated_ranges);
        ptpo->skin_consolidated_ranges = NULL;
    }
    // World-skin GPU mesh (bind-space verts).
    if (ptpo->skin_consolidated_world != NULL) {
        ge_skin_mesh_destroy((GESkinMesh*)ptpo->skin_consolidated_world);
        ptpo->skin_consolidated_world = NULL;
    }
    // Per-bone bind-space AABB (for shadow box).
    if (ptpo->skin_consolidated_bone_aabb != NULL) {
        MemFree(ptpo->skin_consolidated_bone_aabb);
        ptpo->skin_consolidated_bone_aabb = NULL;
    }
    ptpo->skin_consolidated_bone_count = 0;

    MemFree(ptpo->pMaterials);
    MemFree(ptpo->pwListIndices);

    ptpo->pMaterials = NULL;
    ptpo->pwListIndices = NULL;
    ptpo->pD3DVertices = NULL;
    ptpo->pwStripIndices = NULL;
    ptpo->bLRUQueueNumber = 0xff;
    ptpo->wNumMaterials = 0;
    ptpo->wTotalSizeOfObj = 0;

    ptpoLRUQueue[iSlot] = NULL;
}

// uc_orig: FIGURE_clean_all_LRU_slots (fallen/DDEngine/Source/figure.cpp)
// Flushes all cached TomsPrimObjects. Call at level end to release GPU/CPU mesh memory.
void FIGURE_clean_all_LRU_slots(void)
{
    for (int i = 0; i < m_iLRUQueueSize; i++) {
        if (ptpoLRUQueue[i] != NULL) {
            FIGURE_clean_LRU_slot(i);
        }
    }
    m_iLRUQueueSize = 0;

    ASSERT(m_dwSizeOfQueue == 0);

    // Clear our consolidated-mesh look-up keys. FIGURE_clean_LRU_slot
    // already zeroed each TomsPrimObject's per-mesh state, but the
    // D3DAnimObjKeys[] entries (a side cache outside the LRU machinery)
    // would otherwise keep pointing at stale chunk pointers — the next
    // mission's reload of game data may put DIFFERENT contents at the
    // SAME chunk address, and figure_anim_obj_get_consolidated would
    // then return a slot whose wNumMaterials is 0 but whose key looks
    // valid. Resetting here avoids that race.
    for (int i = 0; i < MAX_NUMBER_D3D_ANIMALS; ++i) {
        D3DAnimObjKeys[i].chunk        = NULL;
        D3DAnimObjKeys[i].start_object = 0;
    }
    // Bind-palette cache is also chunk-pointer-keyed — same staleness
    // concern after a reload.
    bind_palette_invalidate_all();
}

// uc_orig: FIGURE_find_and_clean_prim_queue_item (fallen/DDEngine/Source/figure.cpp)
// Adds pPrimObj to the LRU cache, evicting the oldest entry if there is no room.
// Limits: PRIM_LRU_QUEUE_LENGTH=250 slots, PRIM_LRU_QUEUE_SIZE=6000 vertices total.
void FIGURE_find_and_clean_prim_queue_item(TomsPrimObject* pPrimObj, int iThrashIndex)
{
    int iQueuePos;

    ASSERT((DWORD)pPrimObj->wTotalSizeOfObj * 5 < PRIM_LRU_QUEUE_SIZE);
    ASSERT((DWORD)pPrimObj->wTotalSizeOfObj > 0);

    if ((m_iLRUQueueSize < PRIM_LRU_QUEUE_LENGTH) && (((DWORD)pPrimObj->wTotalSizeOfObj + m_dwSizeOfQueue) < PRIM_LRU_QUEUE_SIZE)) {
        iQueuePos = m_iLRUQueueSize;
        m_iLRUQueueSize++;
    } else {
        while (((DWORD)pPrimObj->wTotalSizeOfObj + m_dwSizeOfQueue) >= PRIM_LRU_QUEUE_SIZE) {

            DWORD dwMostTurns = 0;
            int iOldestSlot = -1;
            for (int i = 0; i < m_iLRUQueueSize; i++) {
                if (ptpoLRUQueue[i] != NULL) {
                    DWORD dwTurnsAgo = (VISUAL_TURN - dwGameTurnLastUsed[i]);
                    if (dwMostTurns <= dwTurnsAgo) {
                        dwMostTurns = dwTurnsAgo;
                        iOldestSlot = i;
                    }
                }
            }
            ASSERT((iOldestSlot > -1) && (iOldestSlot < m_iLRUQueueSize));
            if (dwMostTurns < 3) {
                // Cache too small for this scene; nothing to prevent thrash here.
            }

            FIGURE_clean_LRU_slot(iOldestSlot);

            iQueuePos = iOldestSlot;
        }

        if (m_iLRUQueueSize >= PRIM_LRU_QUEUE_LENGTH) {
            DWORD dwMostTurns = 0;
            int iOldestSlot = -1;
            for (int i = 0; i < m_iLRUQueueSize; i++) {
                if (ptpoLRUQueue[i] != NULL) {
                    DWORD dwTurnsAgo = (VISUAL_TURN - dwGameTurnLastUsed[i]);
                    if (dwMostTurns <= dwTurnsAgo) {
                        dwMostTurns = dwTurnsAgo;
                        iOldestSlot = i;
                    }
                }
            }
            ASSERT((iOldestSlot > -1) && (iOldestSlot < m_iLRUQueueSize));
            if (dwMostTurns < 3) {
                if (ptpoLRUQueue[iThrashIndex] == NULL) {
                    // Allow the thrash.
                } else {
                    iOldestSlot = iThrashIndex;
                }
            }

            if (ptpoLRUQueue[iOldestSlot] == NULL) {
                ASSERT(UC_FALSE);

                iOldestSlot = -1;
                for (int i = 0; i < m_iLRUQueueSize; i++) {
                    if (ptpoLRUQueue[i] != NULL) {
                        iOldestSlot = i;
                        break;
                    }
                }
            }

            FIGURE_clean_LRU_slot(iOldestSlot);

            iQueuePos = iOldestSlot;
        }
    }

    ptpoLRUQueue[iQueuePos] = pPrimObj;
    pPrimObj->bLRUQueueNumber = iQueuePos;
    dwGameTurnLastUsed[iQueuePos] = VISUAL_TURN;
    m_dwSizeOfQueue += (DWORD)pPrimObj->wTotalSizeOfObj;
    ASSERT(m_dwSizeOfQueue < PRIM_LRU_QUEUE_SIZE);
}

// uc_orig: FIGURE_touch_LRU_of_object (fallen/DDEngine/Source/figure.cpp)
// Updates the last-used game turn for a cached prim mesh to prevent premature eviction.
void FIGURE_touch_LRU_of_object(TomsPrimObject* pPrimObj)
{
    ASSERT((pPrimObj->bLRUQueueNumber >= 0) && (pPrimObj->bLRUQueueNumber < m_iLRUQueueSize));
    ASSERT(ptpoLRUQueue[pPrimObj->bLRUQueueNumber] == pPrimObj);
    dwGameTurnLastUsed[pPrimObj->bLRUQueueNumber] = VISUAL_TURN;
}

// uc_orig: FIGURE_TPO_init_3d_object (fallen/DDEngine/Source/figure.cpp)
// Begins lazy D3D compilation of a TomsPrimObject: allocates staging CPU buffers.
// FIGURE_TPO_add_prim_to_current_object() registers each prim, then
// FIGURE_TPO_finish_3d_object() emits faces and registers with the LRU cache.
void FIGURE_TPO_init_3d_object(TomsPrimObject* pPrimObj)
{
    ASSERT(TPO_pVert == NULL);
    ASSERT(TPO_pStripIndices == NULL);
    ASSERT(TPO_pListIndices == NULL);
    ASSERT(TPO_piVertexRemap == NULL);
    ASSERT(TPO_piVertexLinks == NULL);
    ASSERT(TPO_pCurVertex == NULL);
    ASSERT(TPO_pCurStripIndex == NULL);
    ASSERT(TPO_pCurListIndex == NULL);
    ASSERT(TPO_pPrimObj == NULL);
    ASSERT(TPO_iNumListIndices == 0);
    ASSERT(TPO_iNumStripIndices == 0);
    ASSERT(TPO_iNumVertices == 0);
    ASSERT(TPO_iNumPrims == 0);

    TPO_pVert = (GEVertex*)MemAlloc(MAX_VERTS * sizeof(GEVertex));
    ASSERT(TPO_pVert != NULL);
    if (TPO_pVert == NULL) {
        DeadAndBuried(0xf800f800);
    }
    TPO_pStripIndices = (UWORD*)MemAlloc(MAX_INDICES * sizeof(UWORD));
    ASSERT(TPO_pStripIndices != NULL);
    if (TPO_pStripIndices == NULL) {
        DeadAndBuried(0xf800f800);
    }
    TPO_pListIndices = (UWORD*)MemAlloc(MAX_INDICES * sizeof(UWORD));
    ASSERT(TPO_pListIndices != NULL);
    if (TPO_pListIndices == NULL) {
        DeadAndBuried(0xf800f800);
    }

    TPO_piVertexRemap = (int*)MemAlloc(MAX_VERTS * sizeof(int));
    ASSERT(TPO_piVertexRemap != NULL);
    if (TPO_piVertexRemap == NULL) {
        DeadAndBuried(0xf800f800);
    }
    TPO_piVertexLinks = (int*)MemAlloc(MAX_VERTS * sizeof(int));
    ASSERT(TPO_piVertexLinks != NULL);
    if (TPO_piVertexLinks == NULL) {
        DeadAndBuried(0xf800f800);
    }

    TPO_pCurVertex = TPO_pVert;
    TPO_pCurStripIndex = TPO_pStripIndices;
    TPO_pCurListIndex = TPO_pListIndices;

    ASSERT(pPrimObj->pD3DVertices == NULL);
    ASSERT(pPrimObj->pMaterials == NULL);
    ASSERT(pPrimObj->pwListIndices == NULL);
    ASSERT(pPrimObj->pwStripIndices == NULL);
    ASSERT(pPrimObj->wNumMaterials == 0);

    // World-skin consolidated VBO + per-material index ranges (built on
    // demand once per rig).
    pPrimObj->skin_consolidated_ranges     = NULL;
    pPrimObj->skin_consolidated_world      = NULL;
    pPrimObj->skin_consolidated_bone_aabb  = NULL;
    pPrimObj->skin_consolidated_bone_count = 0;

    TPO_pPrimObj = pPrimObj;

    TPO_iPrimObjIndexOffset[0] = 0;

    pPrimObj->wFlags = 0;
    pPrimObj->fBoundingSphereRadius = 0.0f;

    TPO_iNumListIndices = 0;
    TPO_iNumStripIndices = 0;
    TPO_iNumVertices = 0;
    TPO_iNumPrims = 0;
}

// uc_orig: FIGURE_TPO_add_prim_to_current_object (fallen/DDEngine/Source/figure.cpp)
// Registers one prim into the active TomsPrimObject compilation context.
// Must be called after FIGURE_TPO_init_3d_object and before FIGURE_TPO_finish_3d_object.
// ubSubObjectNumber is the MultiMatrix slot index for this body part (0-based).
void FIGURE_TPO_add_prim_to_current_object(SLONG prim, UBYTE ubSubObjectNumber)
{
    ASSERT(TPO_pVert != NULL);
    ASSERT(TPO_pStripIndices != NULL);
    ASSERT(TPO_pListIndices != NULL);
    ASSERT(TPO_piVertexRemap != NULL);
    ASSERT(TPO_piVertexLinks != NULL);
    ASSERT(TPO_pCurVertex != NULL);
    ASSERT(TPO_pCurStripIndex != NULL);
    ASSERT(TPO_pCurListIndex != NULL);
    ASSERT(TPO_pPrimObj != NULL);

    ASSERT(prim < MAX_NUMBER_D3D_PRIMS);
    PrimObject* p_obj = &prim_objects[prim];
    ASSERT(p_obj != NULL);

    TPO_PrimObjects[TPO_iNumPrims] = prim;
    TPO_ubPrimObjMMIndex[TPO_iNumPrims] = ubSubObjectNumber;

    // Record vertex index offset for the next prim: this prim's offset plus its vertex count.
    TPO_iPrimObjIndexOffset[TPO_iNumPrims + 1] = TPO_iPrimObjIndexOffset[TPO_iNumPrims] + (p_obj->EndPoint - p_obj->StartPoint);

    TPO_iNumPrims++;
    ASSERT(TPO_iNumPrims <= TPO_MAX_NUMBER_PRIMS);
}

// uc_orig: FIGURE_TPO_finish_3d_object (fallen/DDEngine/Source/figure.cpp)
// Compiles all registered prims into the TomsPrimObject pPrimObj.
// Groups faces by texture page (material), deduplicates vertices, builds index and strip lists,
// then copies the result into a single heap block and registers it in the LRU cache.
// iThrashIndex: hint for LRU placement, normally 0.
void FIGURE_TPO_finish_3d_object(TomsPrimObject* pPrimObj, int iThrashIndex)
{
    ASSERT(TPO_pVert != NULL);
    ASSERT(TPO_pStripIndices != NULL);
    ASSERT(TPO_pListIndices != NULL);
    ASSERT(TPO_piVertexRemap != NULL);
    ASSERT(TPO_piVertexLinks != NULL);
    ASSERT(TPO_pCurVertex != NULL);
    ASSERT(TPO_pCurStripIndex != NULL);
    ASSERT(TPO_pCurListIndex != NULL);
    ASSERT(TPO_pPrimObj != NULL);

    ASSERT(pPrimObj == TPO_pPrimObj);

    // Scan all registered prims' faces. For each face, find or create a material slot
    // matching its texture page. Then emit vertices (deduplicating by position+normal+UV)
    // and indices. Quads and tris are processed in alternating do-while passes.

    for (int iOuterPrimNumber = 0; iOuterPrimNumber < TPO_iNumPrims; iOuterPrimNumber++) {
        PrimObject* pOuterObj = &prim_objects[TPO_PrimObjects[iOuterPrimNumber]];

        ASSERT(pOuterObj != NULL);

        bool bOuterTris = UC_FALSE;
        do {
            int iOuterFaceNum;
            int iOuterFaceEnd;
            if (bOuterTris) {
                iOuterFaceNum = pOuterObj->StartFace3;
                iOuterFaceEnd = pOuterObj->EndFace3;
            } else {
                iOuterFaceNum = pOuterObj->StartFace4;
                iOuterFaceEnd = pOuterObj->EndFace4;
            }

            for (; iOuterFaceNum < iOuterFaceEnd; iOuterFaceNum++) {
                UWORD wTexturePage = FIGURE_find_face_D3D_texture_page(iOuterFaceNum, bOuterTris);

                PolyPage* pRenderedPage = NULL;

                if ((wTexturePage & ~TEXTURE_PAGE_MASK) != 0) {
                    // Special page flags (jacket, offset, tint, etc.) — don't combine with others.
                    pRenderedPage = NULL;
                } else {
                    pRenderedPage = POLY_Page[wTexturePage & TEXTURE_PAGE_MASK].pTheRealPolyPage;
                }

                // Find an existing material with this texture page, or create one.
                PrimObjectMaterial* pMaterial = pPrimObj->pMaterials;
                int iMatNum;
                for (iMatNum = pPrimObj->wNumMaterials; iMatNum > 0; iMatNum--) {
                    if (pRenderedPage != NULL) {
                        if ((pMaterial->wTexturePage & ~TEXTURE_PAGE_MASK) == 0) {
                            if (pRenderedPage == (POLY_Page[pMaterial->wTexturePage & TEXTURE_PAGE_MASK].pTheRealPolyPage)) {
                                break;
                            }
                        }
                    }

                    if (pMaterial->wTexturePage == wTexturePage) {
                        break;
                    }
                    pMaterial++;
                }
                if (iMatNum != 0) {
                    // Face already added to an existing material — skip.
                } else {
                    // New material: allocate and initialise a slot for it.

                    pPrimObj->wNumMaterials++;
                    void* pOldMats = (void*)pPrimObj->pMaterials;
                    pPrimObj->pMaterials = (PrimObjectMaterial*)MemAlloc(pPrimObj->wNumMaterials * sizeof(*pMaterial));
                    ASSERT(pPrimObj->pMaterials != NULL);
                    if (pPrimObj->pMaterials == NULL) {
                        DeadAndBuried(0x001f001f);
                    }

                    if (pOldMats != NULL) {
                        memcpy(pPrimObj->pMaterials, pOldMats, (pPrimObj->wNumMaterials - 1) * sizeof(*pMaterial));
                        MemFree(pOldMats);
                    }

                    pMaterial = pPrimObj->pMaterials + pPrimObj->wNumMaterials - 1;
                    pMaterial->wNumListIndices = 0;
                    pMaterial->wNumStripIndices = 0;
                    pMaterial->wNumVertices = 0;
                    pMaterial->wTexturePage = wTexturePage;

                    GEVertex* pFirstVertex = TPO_pCurVertex;

                    WORD* pFirstListIndex = TPO_pCurListIndex;
                    WORD* pFirstStripIndex = TPO_pCurStripIndex;

                    // Reset the vertex deduplication tables for this new material.
                    for (int i = 0; i < MAX_VERTS; i++) {
                        TPO_piVertexRemap[i] = -1;
                        TPO_piVertexLinks[i] = -1;
                    }

                    // Inner loop: scan all remaining prims and add faces that share this material.
                    for (int iInnerPrimNumber = iOuterPrimNumber; iInnerPrimNumber < TPO_iNumPrims; iInnerPrimNumber++) {
                        PrimObject* pInnerObj = &prim_objects[TPO_PrimObjects[iInnerPrimNumber]];

                        float* pfBoundingSphereRadius = &(m_fObjectBoundingSphereRadius[TPO_PrimObjects[iInnerPrimNumber]]);
                        *pfBoundingSphereRadius = 0.0f;

                        ASSERT(pInnerObj != NULL);

                        bool bInnerTris;
                        if (iInnerPrimNumber == iOuterPrimNumber) {
                            bInnerTris = bOuterTris;
                        } else {
                            bInnerTris = UC_FALSE;
                        }

                        do {
                            int iInnerFaceEnd;
                            int iInnerFaceNum;
                            if (iInnerPrimNumber == iOuterPrimNumber) {
                                if (bInnerTris) {
                                    if (bOuterTris) {
                                        iInnerFaceNum = iOuterFaceNum;
                                    } else {
                                        iInnerFaceNum = pInnerObj->StartFace3;
                                    }
                                    iInnerFaceEnd = pInnerObj->EndFace3;
                                } else {
                                    if (bOuterTris) {
                                        ASSERT(UC_FALSE);
                                        iInnerFaceNum = pInnerObj->StartFace4;
                                    } else {
                                        iInnerFaceNum = iOuterFaceNum;
                                    }
                                    iInnerFaceEnd = pInnerObj->EndFace4;
                                }
                            } else {
                                if (bInnerTris) {
                                    iInnerFaceNum = pInnerObj->StartFace3;
                                    iInnerFaceEnd = pInnerObj->EndFace3;
                                } else {
                                    iInnerFaceNum = pInnerObj->StartFace4;
                                    iInnerFaceEnd = pInnerObj->EndFace4;
                                }
                            }

                            ASSERT(iInnerFaceNum <= iInnerFaceEnd);
                            for (; iInnerFaceNum < iInnerFaceEnd; iInnerFaceNum++) {
                                UWORD wTexturePage = FIGURE_find_face_D3D_texture_page(iInnerFaceNum, bInnerTris);

                                bool bSamePage = UC_FALSE;

                                if (pMaterial->wTexturePage == wTexturePage) {
                                    bSamePage = UC_TRUE;
                                } else if (pRenderedPage != NULL) {
                                    if (((pMaterial->wTexturePage | wTexturePage) & ~TEXTURE_PAGE_MASK) == 0) {
                                        if (pRenderedPage == (POLY_Page[wTexturePage & TEXTURE_PAGE_MASK].pTheRealPolyPage)) {
                                            bSamePage = UC_TRUE;
                                        }
                                    }
                                }

                                if (bSamePage) {
                                    // Determine actual page for UV offset/scale.
                                    UWORD wRealPage = wTexturePage & TEXTURE_PAGE_MASK;
                                    if (wTexturePage & TEXTURE_PAGE_FLAG_JACKET) {
                                        wRealPage = jacket_lookup[wRealPage][0];
                                        wRealPage += FACE_PAGE_OFFSET;
                                    } else if (wTexturePage & TEXTURE_PAGE_FLAG_OFFSET) {
                                        wRealPage += FACE_PAGE_OFFSET;
                                    }

                                    PolyPage* pa = &(POLY_Page[wRealPage]);

                                    // Emit vertices for this face, deduplicating by (pos,normal,UV).
                                    int iIndices[4];
                                    PrimFace3* p_f3;
                                    PrimFace4* p_f4;
                                    int iVerts;
                                    if (bInnerTris) {
                                        p_f3 = &prim_faces3[iInnerFaceNum];
                                        iVerts = 3;
                                    } else {
                                        p_f4 = &prim_faces4[iInnerFaceNum];
                                        iVerts = 4;
                                    }
                                    for (int i = 0; i < iVerts; i++) {
                                        const float fNormScale = 1.0f / 256.0f;

                                        GEVertex vert;

                                        int pt;
                                        if (bInnerTris) {
                                            pt = p_f3->Points[i];
                                            vert.u = float(p_f3->UV[i][0] & 0x3f) * (1.0F / 32.0F);
                                            vert.v = float(p_f3->UV[i][1]) * (1.0F / 32.0F);
                                        } else {
                                            pt = p_f4->Points[i];
                                            vert.u = float(p_f4->UV[i][0] & 0x3f) * (1.0F / 32.0F);
                                            vert.v = float(p_f4->UV[i][1]) * (1.0F / 32.0F);
                                        }
                                        // Clamp UVs to [0,1] to avoid texture border bleeding.
                                        if (vert.u < 0.0f) {
                                            vert.u = 0.0f;
                                        } else if (vert.u > 1.0f) {
                                            vert.u = 1.0f;
                                        }
                                        if (vert.v < 0.0f) {
                                            vert.v = 0.0f;
                                        } else if (vert.v > 1.0f) {
                                            vert.v = 1.0f;
                                        }

                                        vert.u = vert.u * pa->m_UScale + pa->m_UOffset;
                                        vert.v = vert.v * pa->m_VScale + pa->m_VOffset;

                                        vert.x = AENG_dx_prim_points[pt].X;
                                        vert.y = AENG_dx_prim_points[pt].Y;
                                        vert.z = AENG_dx_prim_points[pt].Z;
                                        vert.nx = prim_normal[pt].X * fNormScale;
                                        vert.ny = prim_normal[pt].Y * fNormScale;
                                        vert.nz = prim_normal[pt].Z * fNormScale;
                                        // MM index must be set last — it writes into byte 12 of the vertex.
                                        SET_MM_INDEX(vert, TPO_ubPrimObjMMIndex[iInnerPrimNumber]);

                                        ASSERT(pt >= pInnerObj->StartPoint);
                                        ASSERT(pt < pInnerObj->EndPoint);
                                        ASSERT((pt - pInnerObj->StartPoint) < MAX_VERTS);

                                        if ((pt - pInnerObj->StartPoint) >= MAX_VERTS) {
                                            DeadAndBuried(0xffffffff);
                                        }

                                        int iPtIndex = TPO_iPrimObjIndexOffset[iInnerPrimNumber] + (pt - pInnerObj->StartPoint);
                                        int iVertIndex = TPO_piVertexRemap[iPtIndex];
                                        if (iVertIndex == -1) {
                                            // First reference to this point: add as new vertex.
                                            TPO_piVertexRemap[iPtIndex] = pMaterial->wNumVertices;
                                            TPO_piVertexLinks[pMaterial->wNumVertices] = -1;

                                            iIndices[i] = pMaterial->wNumVertices;

                                            *TPO_pCurVertex = vert;
                                            TPO_pCurVertex++;
                                            pMaterial->wNumVertices++;
                                            TPO_iNumVertices++;
                                            ASSERT(TPO_iNumVertices < MAX_VERTS);

                                            if (TPO_iNumVertices >= MAX_VERTS) {
                                                DeadAndBuried(0xffffffff);
                                            }

                                            // Grow bounding sphere if this vertex is farther out.
                                            float fDistSqu = (vert.x * vert.x) + (vert.y * vert.y) + (vert.z * vert.z);
                                            if ((*pfBoundingSphereRadius * *pfBoundingSphereRadius) < fDistSqu) {
                                                *pfBoundingSphereRadius = sqrtf(fDistSqu);
                                            }
                                        } else {
                                            // Walk the UV-variant chain for this position+normal.
                                            int iLastIndex = iVertIndex;
                                            while (iVertIndex != -1) {
                                                ASSERT(pFirstVertex[iVertIndex].x == vert.x);
                                                ASSERT(pFirstVertex[iVertIndex].y == vert.y);
                                                ASSERT(pFirstVertex[iVertIndex].z == vert.z);
                                                ASSERT(pFirstVertex[iVertIndex].nx == vert.nx);
                                                ASSERT(pFirstVertex[iVertIndex].ny == vert.ny);
                                                ASSERT(pFirstVertex[iVertIndex].nz == vert.nz);
// uc_orig: CLOSE_ENOUGH (fallen/DDEngine/Source/figure.cpp)
#define CLOSE_ENOUGH(a, b) (fabsf((a) - (b)) < 0.00001f)
                                                if (CLOSE_ENOUGH(pFirstVertex[iVertIndex].u, vert.u) && CLOSE_ENOUGH(pFirstVertex[iVertIndex].v, vert.v)) {
                                                    iIndices[i] = iVertIndex;
                                                    break;
                                                } else {
                                                    iLastIndex = iVertIndex;
                                                    iVertIndex = TPO_piVertexLinks[iVertIndex];
                                                }
                                            }
                                            if (iVertIndex == -1) {
                                                // No matching UV variant — add a new one.
                                                TPO_piVertexLinks[iLastIndex] = pMaterial->wNumVertices;
                                                TPO_piVertexLinks[pMaterial->wNumVertices] = -1;

                                                iIndices[i] = pMaterial->wNumVertices;

                                                *TPO_pCurVertex = vert;
                                                TPO_pCurVertex++;
                                                pMaterial->wNumVertices++;
                                                TPO_iNumVertices++;
                                                ASSERT(TPO_iNumVertices < MAX_VERTS);

                                                if (TPO_iNumVertices >= MAX_VERTS) {
                                                    DeadAndBuried(0xffffffff);
                                                }
                                            }
                                        }
                                    }

                                    // Emit triangle indices (quads split into two tris).
                                    if (bInnerTris) {
                                        ASSERT((iIndices[0] >= 0) && (iIndices[0] < pMaterial->wNumVertices));
                                        ASSERT((iIndices[1] >= 0) && (iIndices[1] < pMaterial->wNumVertices));
                                        ASSERT((iIndices[2] >= 0) && (iIndices[2] < pMaterial->wNumVertices));

                                        *TPO_pCurListIndex++ = iIndices[0];
                                        *TPO_pCurListIndex++ = iIndices[1];
                                        *TPO_pCurListIndex++ = iIndices[2];
                                        TPO_iNumListIndices += 3;
                                        pMaterial->wNumListIndices += 3;
                                        ASSERT(TPO_iNumListIndices < MAX_INDICES);

                                        if (TPO_iNumListIndices >= MAX_INDICES) {
                                            DeadAndBuried(0x07ff07ff);
                                        }
                                    } else {
                                        ASSERT((iIndices[0] >= 0) && (iIndices[0] < pMaterial->wNumVertices));
                                        ASSERT((iIndices[1] >= 0) && (iIndices[1] < pMaterial->wNumVertices));
                                        ASSERT((iIndices[2] >= 0) && (iIndices[2] < pMaterial->wNumVertices));
                                        ASSERT((iIndices[3] >= 0) && (iIndices[3] < pMaterial->wNumVertices));

                                        // Quad split: two CCW triangles (0,1,2) and (2,1,3).
                                        *TPO_pCurListIndex++ = iIndices[0];
                                        *TPO_pCurListIndex++ = iIndices[1];
                                        *TPO_pCurListIndex++ = iIndices[2];
                                        *TPO_pCurListIndex++ = iIndices[2];
                                        *TPO_pCurListIndex++ = iIndices[1];
                                        *TPO_pCurListIndex++ = iIndices[3];
                                        TPO_iNumListIndices += 6;
                                        pMaterial->wNumListIndices += 6;
                                        ASSERT(TPO_iNumListIndices < MAX_INDICES);

                                        if (TPO_iNumStripIndices >= MAX_INDICES) {
                                            DeadAndBuried(0x07ff07ff);
                                        }
                                    }
                                }
                            }

                            bInnerTris = !bInnerTris;
                        } while (bInnerTris);
                    }

                    // All faces for this material collected. Run MS strip optimiser, then rebuild strip.
                    WORD* pSrcIndex;

                    int iRes = MSOptimizeIndexedList(pFirstListIndex, pMaterial->wNumListIndices / 3);
                    ASSERT(iRes != 0);

                    ASSERT(TPO_pCurStripIndex == pFirstStripIndex);
                    pSrcIndex = pFirstListIndex;
                    WORD wIndex0 = -1;
                    WORD wIndex1 = -1;
                    bool bOdd = UC_FALSE;
                    bool bFirst = UC_TRUE;
                    for (int i = pMaterial->wNumListIndices / 3; i > 0; i--) {
                        WORD wNextIndex = -1;
                        if ((wIndex0 == pSrcIndex[2]) && (wIndex1 == pSrcIndex[0])) {
                            wNextIndex = pSrcIndex[1];
                        } else if ((wIndex0 == pSrcIndex[0]) && (wIndex1 == pSrcIndex[1])) {
                            wNextIndex = pSrcIndex[2];
                        } else if ((wIndex0 == pSrcIndex[1]) && (wIndex1 == pSrcIndex[2])) {
                            wNextIndex = pSrcIndex[0];
                        }
                        if (wNextIndex != (WORD)-1) {
                            // Continue the current strip.
                            *TPO_pCurStripIndex++ = wNextIndex;
                            TPO_iNumStripIndices += 1;
                            pMaterial->wNumStripIndices += 1;
                            ASSERT(TPO_iNumStripIndices < MAX_INDICES);

                            if (TPO_iNumStripIndices >= MAX_INDICES) {
                                DeadAndBuried(0x07ff07ff);
                            }

                            if (bOdd) {
                                wIndex0 = wNextIndex;
                            } else {
                                wIndex1 = wNextIndex;
                            }
                            bOdd = !bOdd;
                        } else {
                            // Start a new strip.
                            if (!bFirst) {
                                *TPO_pCurStripIndex++ = -1;
                                TPO_iNumStripIndices += 1;
                                pMaterial->wNumStripIndices += 1;
                                ASSERT(TPO_iNumStripIndices < MAX_INDICES);

                                if (TPO_iNumStripIndices >= MAX_INDICES) {
                                    DeadAndBuried(0x07ff07ff);
                                }

                            } else {
                                bFirst = UC_FALSE;
                            }
                            *TPO_pCurStripIndex++ = pSrcIndex[0];
                            *TPO_pCurStripIndex++ = pSrcIndex[1];
                            *TPO_pCurStripIndex++ = pSrcIndex[2];
                            TPO_iNumStripIndices += 3;
                            pMaterial->wNumStripIndices += 3;
                            ASSERT(TPO_iNumStripIndices < MAX_INDICES);

                            if (TPO_iNumStripIndices >= MAX_INDICES) {
                                DeadAndBuried(0x07ff07ff);
                            }

                            wIndex0 = pSrcIndex[2];
                            wIndex1 = pSrcIndex[1];
                            bOdd = UC_FALSE;
                        }
                        pSrcIndex += 3;
                    }
                    // Terminate the strip sequence with -1.
                    *TPO_pCurStripIndex++ = -1;
                    TPO_iNumStripIndices += 1;
                    pMaterial->wNumStripIndices += 1;
                    ASSERT(TPO_iNumStripIndices < MAX_INDICES);

                    if (TPO_iNumStripIndices >= MAX_INDICES) {
                        DeadAndBuried(0x07ff07ff);
                    }

                    ASSERT(pMaterial->wNumStripIndices == (TPO_pCurStripIndex - pFirstStripIndex));
                }
            }

            bOuterTris = !bOuterTris;
        } while (bOuterTris);
    }

    pPrimObj->wTotalSizeOfObj = TPO_iNumVertices;

    // Allocate one unified block for list indices, vertices (32-byte aligned), and strip indices.
    // The extra 4 WORDs at the end prevent a page fault from the MM driver reading past the end.
    DWORD dwTotalSize = 0;
    dwTotalSize += TPO_iNumListIndices * sizeof(UWORD);
    dwTotalSize += 32 + TPO_iNumVertices * sizeof(GEVertex);
    dwTotalSize += TPO_iNumStripIndices * sizeof(UWORD);
    dwTotalSize += 4 * sizeof(WORD);

    char* pcBlock = (char*)MemAlloc(dwTotalSize);
    ASSERT(pcBlock != NULL);
    if (pcBlock == NULL) {
        DeadAndBuried(0xffe0ffe0);
    }

    pPrimObj->pwListIndices = (UWORD*)pcBlock;
    memcpy(pPrimObj->pwListIndices, TPO_pListIndices, TPO_iNumListIndices * sizeof(UWORD));
    pcBlock += TPO_iNumListIndices * sizeof(UWORD);

    // Align vertices to 32-byte cache lines for the MM extension.
    pPrimObj->pD3DVertices = (void*)(((uintptr_t)pcBlock + 31) & ~(uintptr_t)31);
    memcpy(pPrimObj->pD3DVertices, TPO_pVert, TPO_iNumVertices * sizeof(GEVertex));
    pcBlock = (char*)pPrimObj->pD3DVertices + TPO_iNumVertices * sizeof(GEVertex);

    pPrimObj->pwStripIndices = (UWORD*)pcBlock;
    memcpy(pPrimObj->pwStripIndices, TPO_pStripIndices, TPO_iNumStripIndices * sizeof(UWORD));
    pcBlock += TPO_iNumStripIndices * sizeof(UWORD);

    ASSERT((uintptr_t)pcBlock < (uintptr_t)pPrimObj->pwListIndices + dwTotalSize);

    MemFree(TPO_piVertexLinks);
    MemFree(TPO_piVertexRemap);
    MemFree(TPO_pListIndices);
    MemFree(TPO_pStripIndices);
    MemFree(TPO_pVert);

    pPrimObj->fBoundingSphereRadius = sqrtf(pPrimObj->fBoundingSphereRadius);

    FIGURE_find_and_clean_prim_queue_item(pPrimObj, iThrashIndex);

    // Clear all working state so the next object can start cleanly.
    TPO_pVert = NULL;
    TPO_pStripIndices = NULL;
    TPO_pListIndices = NULL;
    TPO_piVertexRemap = NULL;
    TPO_piVertexLinks = NULL;
    TPO_pCurVertex = NULL;
    TPO_pCurStripIndex = NULL;
    TPO_pCurListIndex = NULL;
    TPO_pPrimObj = NULL;
    TPO_iNumListIndices = 0;
    TPO_iNumStripIndices = 0;
    TPO_iNumVertices = 0;
    TPO_iNumPrims = 0;
}

// uc_orig: FIGURE_generate_D3D_object (fallen/DDEngine/Source/figure.cpp)
// Convenience wrapper: compiles a single-prim TomsPrimObject in D3DObj[prim].
// Called lazily on first draw of each body-part prim.
void FIGURE_generate_D3D_object(SLONG prim)
{
    TomsPrimObject* pPrimObj = &(D3DObj[prim]);

    FIGURE_TPO_init_3d_object(pPrimObj);
    FIGURE_TPO_add_prim_to_current_object(prim, 0);
    FIGURE_TPO_finish_3d_object(pPrimObj);
}


#include "things/items/special.h"
#include "things/core/interact.h"
#include "engine/input/keyboard_globals.h"
#include "things/core/hierarchy.h"

// Draw the muzzle-flash prim (261/262/263) as flat-grey unlit polys via
// the POLY_buffer software pipeline — matches the legacy
// FIGURE_draw_prim_tween_person_only branch for muzzle prims. Lit
// rendering through MESH_draw_guts is wrong for muzzle flash: the flame
// texture relies on a fixed 0xff808080 vertex colour modulated against
// an additive texture page, so per-vertex NIGHT lighting darkens the
// flame to invisibility in night scenes. (Shotgun flash happens to use
// a brighter texture so MESH path looked OK, but pistol/AK went dark.)
//
// Assumes POLY_set_local_rotation has already been called with the
// hand-bone transform — figure_draw_held_item does that before calling
// us via MESH_draw_poly_at_matrix when the prim is non-muzzle, or
// directly via the inline POLY_set_local_rotation here when prim is a
// muzzle flash.
static void figure_draw_muzzle_flash(SLONG prim)
{
    PrimObject* p_obj = &prim_objects[prim];
    const SLONG sp    = p_obj->StartPoint;
    const SLONG ep    = p_obj->EndPoint;

    POLY_buffer_upto = 0;

    // All muzzle verts get the same flat grey colour — flame brightness
    // lives in the texture, which is added on top via the texture page's
    // additive blend mode.
    for (SLONG i = sp; i < ep; i++) {
        ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));
        POLY_Point* pp = &POLY_buffer[POLY_buffer_upto++];
        POLY_transform_using_local_rotation(
            AENG_dx_prim_points[i].X,
            AENG_dx_prim_points[i].Y,
            AENG_dx_prim_points[i].Z,
            pp);
        pp->colour   = 0xff808080;
        pp->specular = 0xff000000;
    }

    POLY_Point* quad[4];
    for (SLONG i = p_obj->StartFace4; i < p_obj->EndFace4; i++) {
        PrimFace4* p_f4 = &prim_faces4[i];
        const SLONG p0 = p_f4->Points[0] - sp;
        const SLONG p1 = p_f4->Points[1] - sp;
        const SLONG p2 = p_f4->Points[2] - sp;
        const SLONG p3 = p_f4->Points[3] - sp;
        ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p3, 0, POLY_buffer_upto - 1));
        quad[0] = &POLY_buffer[p0];
        quad[1] = &POLY_buffer[p1];
        quad[2] = &POLY_buffer[p2];
        quad[3] = &POLY_buffer[p3];
        if (POLY_valid_quad(quad)) {
            quad[0]->u = float(p_f4->UV[0][0] & 0x3f) * (1.0F / 32.0F);
            quad[0]->v = float(p_f4->UV[0][1])        * (1.0F / 32.0F);
            quad[1]->u = float(p_f4->UV[1][0])        * (1.0F / 32.0F);
            quad[1]->v = float(p_f4->UV[1][1])        * (1.0F / 32.0F);
            quad[2]->u = float(p_f4->UV[2][0])        * (1.0F / 32.0F);
            quad[2]->v = float(p_f4->UV[2][1])        * (1.0F / 32.0F);
            quad[3]->u = float(p_f4->UV[3][0])        * (1.0F / 32.0F);
            quad[3]->v = float(p_f4->UV[3][1])        * (1.0F / 32.0F);
            SLONG page = p_f4->UV[0][0] & 0xc0;
            page <<= 2;
            page |= p_f4->TexturePage;
            page += FACE_PAGE_OFFSET;
            POLY_add_quad(quad, page, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
        }
    }

    POLY_Point* tri[3];
    for (SLONG i = p_obj->StartFace3; i < p_obj->EndFace3; i++) {
        PrimFace3* p_f3 = &prim_faces3[i];
        const SLONG p0 = p_f3->Points[0] - sp;
        const SLONG p1 = p_f3->Points[1] - sp;
        const SLONG p2 = p_f3->Points[2] - sp;
        ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
        ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));
        tri[0] = &POLY_buffer[p0];
        tri[1] = &POLY_buffer[p1];
        tri[2] = &POLY_buffer[p2];
        if (POLY_valid_triangle(tri)) {
            tri[0]->u = float(p_f3->UV[0][0] & 0x3f) * (1.0F / 32.0F);
            tri[0]->v = float(p_f3->UV[0][1])        * (1.0F / 32.0F);
            tri[1]->u = float(p_f3->UV[1][0])        * (1.0F / 32.0F);
            tri[1]->v = float(p_f3->UV[1][1])        * (1.0F / 32.0F);
            tri[2]->u = float(p_f3->UV[2][0])        * (1.0F / 32.0F);
            tri[2]->v = float(p_f3->UV[2][1])        * (1.0F / 32.0F);
            SLONG page = p_f3->UV[0][0] & 0xc0;
            page <<= 2;
            page |= p_f3->TexturePage;
            page += FACE_PAGE_OFFSET;
            POLY_add_triangle(tri, page, !(p_f3->DrawFlags & POLY_FLAG_DOUBLESIDED));
        }
    }
}

// Draw a held item (gun or muzzle flash) attached to a character's hand
// bone. Renders the static prim at the bone's world transform from the
// pose snapshot.
//
// Two flavours of static prim live on the hand:
//
//   - Gun mesh (prim 256/258/260): drawn lit through MESH_draw_poly_at_matrix
//     (same path as held grenades, vehicles etc — proper NIGHT lighting).
//     Side effect: writes the muzzle vertex's world position into
//     p_person->Genus.Person->GunMuzzle, consumed by gunfire FX (bullet
//     spawn / particle emit point).
//
//   - Muzzle flash (prim 261/262/263): drawn UNLIT via figure_draw_muzzle_flash
//     (flat 0xff808080 colour + additive texture page) — see that
//     function's comment for why MESH_draw_guts can't render these
//     correctly.
static void figure_draw_held_item(Thing* p_person, SLONG prim, int hand_part)
{
    const BoneInterpTransform* pose = render_interp_get_cached_pose(p_person);
    if (!pose || hand_part < 0 || hand_part >= POSE_MAX_BONES) return;

    const BoneInterpTransform& xf = pose[hand_part];

    // Bone rotation matrix in row-major M*v form, scale 1.0
    // (POLY_set_local_rotation expects exactly this layout).
    constexpr float S = 1.0f / 32768.0f;
    float fmatrix[9];
    fmatrix[0] = float(xf.rot.M[0][0]) * S;
    fmatrix[1] = float(xf.rot.M[0][1]) * S;
    fmatrix[2] = float(xf.rot.M[0][2]) * S;
    fmatrix[3] = float(xf.rot.M[1][0]) * S;
    fmatrix[4] = float(xf.rot.M[1][1]) * S;
    fmatrix[5] = float(xf.rot.M[1][2]) * S;
    fmatrix[6] = float(xf.rot.M[2][0]) * S;
    fmatrix[7] = float(xf.rot.M[2][1]) * S;
    fmatrix[8] = float(xf.rot.M[2][2]) * S;

    // Muzzle-point side effect for the three known gun prims. The muzzle
    // vertex index inside each gun's prim_objects[] data is hard-coded:
    // prim 256 = pistol (muzzle = vertex 0), prim 258 = shotgun (vertex 15),
    // prim 260 = AK (vertex 32). Carried over from the legacy per-bone path.
    if (prim == 256 || prim == 258 || prim == 260) {
        const SLONG sp  = prim_objects[prim].StartPoint;
        const SLONG idx = sp + ((prim == 256) ? 0
                              : (prim == 258) ? 15
                              :                 32);
        float vx = AENG_dx_prim_points[idx].X;
        float vy = AENG_dx_prim_points[idx].Y;
        float vz = AENG_dx_prim_points[idx].Z;
        MATRIX_MUL(fmatrix, vx, vy, vz);
        p_person->Genus.Person->GunMuzzle.X = SLONG((vx + xf.pos_x) * 256.0f);
        p_person->Genus.Person->GunMuzzle.Y = SLONG((vy + xf.pos_y) * 256.0f);
        p_person->Genus.Person->GunMuzzle.Z = SLONG((vz + xf.pos_z) * 256.0f);
    }

    if (prim == 261 || prim == 262 || prim == 263) {
        // Muzzle flash: unlit, flat-grey path. Need POLY_set_local_rotation
        // ourselves (MESH_draw_poly_at_matrix would do it, but we go
        // through the manual POLY_buffer pipeline below).
        POLY_set_local_rotation(xf.pos_x, xf.pos_y, xf.pos_z, fmatrix);
        figure_draw_muzzle_flash(prim);
    } else {
        MESH_draw_poly_at_matrix(prim,
                                 SLONG(xf.pos_x), SLONG(xf.pos_y), SLONG(xf.pos_z),
                                 fmatrix, NULL, 0xff);
    }
}

// Forward declaration — defined later in this file (used by
// FIGURE_get_skin_mesh_for_thing's non-person branch).
static TomsPrimObject* figure_anim_obj_get_consolidated(const GameKeyFrameChunk* chunk,
                                                       SLONG start_object,
                                                       SLONG ele_count);

// Compute the D3DPeopleObj[] index used to cache the consolidated mesh
// for one person Thing. Same indexing as FIGURE_draw_hierarchical_prim_recurse:
//   - PERSON_ROPER  : (PersonID >> 5) + PERSON_NUM_TYPES + 32
//   - PERSON_CIV    : (PersonID & 0x1f) + PERSON_NUM_TYPES
//   - everything else: PersonType
static int figure_person_iIndex(Thing* p_person)
{
    DrawTween* dt = p_person->Draw.Tweened;
    const BYTE person_type = (BYTE)p_person->Genus.Person->PersonType;
    const BYTE person_id   = (BYTE)dt->PersonID;
    ASSERT(person_type < PERSON_NUM_TYPES);
    if (person_type == PERSON_CIV)
        return (int)(person_id & 0x1f) + PERSON_NUM_TYPES;
    if (person_type == PERSON_ROPER) {
        ASSERT((person_id >> 5) < NUM_ROPERS_THINGIES);
        return (int)(person_id >> 5) + PERSON_NUM_TYPES + 32;
    }
    return (int)person_type;
}

// Lazy TPO build for one person rig. Walks the 15-bone body-part hierarchy
// (pre-order DFS via body_part_children[]) and adds each part's prim to
// the consolidated TomsPrimObject. Same shape as the inline walk that
// used to live in FIGURE_draw_hierarchical_prim_recurse — factored out so
// the shadow path (SMAP_person_gpu) can trigger it too when it runs
// BEFORE the body draw in a frame.
static void figure_tpo_build_person(TomsPrimObject* pPrimObj, BodyDef* body_def, SLONG start_object)
{
    structFIGURE_dhpr_rdata1 saved_root = FIGURE_dhpr_rdata1[0];
    FIGURE_dhpr_rdata1[0].part_number          = 0;
    FIGURE_dhpr_rdata1[0].current_child_number = 0;

    FIGURE_TPO_init_3d_object(pPrimObj);
    int iTPOPartNumber = 0;
    SLONG recurse_level = 0;
    while (recurse_level >= 0) {
        structFIGURE_dhpr_rdata1* pDHPR1 = FIGURE_dhpr_rdata1 + recurse_level;
        int iPartNumber = pDHPR1->part_number;
        if (pDHPR1->current_child_number == 0) {
            ASSERT(iPartNumber >= 0);
            ASSERT(iPartNumber <= 14);
            SLONG body_part = body_def->BodyPart[iPartNumber];
            SLONG prim      = start_object + body_part;
            FIGURE_TPO_add_prim_to_current_object(prim, iTPOPartNumber);
            iTPOPartNumber++;
        }
        if (body_part_children[iPartNumber][pDHPR1->current_child_number] != -1) {
            structFIGURE_dhpr_rdata1* pDHPR1Inc = FIGURE_dhpr_rdata1 + recurse_level + 1;
            pDHPR1Inc->current_child_number = 0;
            pDHPR1Inc->part_number = body_part_children[iPartNumber][pDHPR1->current_child_number];
            pDHPR1->current_child_number++;
            recurse_level++;
        } else {
            recurse_level--;
        }
    }
    FIGURE_TPO_finish_3d_object(pPrimObj, 1);
    FIGURE_dhpr_rdata1[0] = saved_root;
}

// Public helper: get the consolidated skin mesh for any skinned Thing
// (CLASS_PERSON 15-bone rig OR DT_ANIM_PRIM flat skeleton). Lazily builds
// the TomsPrimObject (TPO walk) AND the bind-space VBO if neither is
// built yet. Returns false (leaving out_* untouched) if the Thing isn't
// skinned, has no valid pose data, or bind palette is unavailable.
//
// out_bind_inv is the inverse-bind matrix array (bone_count entries) the
// caller uses to build the per-frame skin palette. out_bone_aabb points
// at the per-bone bind-space AABB array (bone_count × 6 floats) stored
// on the consolidated TomsPrimObject.
bool FIGURE_get_skin_mesh_for_thing(Thing* p_thing,
                                    GESkinMesh**             out_mesh,
                                    const float**            out_bone_aabb,
                                    int*                     out_bone_count,
                                    const GameKeyFrameChunk** out_chunk,
                                    const GEMatrix**         out_bind_inv,
                                    TomsPrimObject**         out_prim_obj)
{
    if (!p_thing) return false;
    DrawTween* dt = p_thing->Draw.Tweened;
    if (!dt || !dt->CurrentFrame || !dt->NextFrame) return false;
    if (!dt->CurrentFrame->FirstElement || !dt->NextFrame->FirstElement) return false;
    const GameKeyFrameChunk* chunk = dt->TheChunk;
    if (!chunk) return false;

    // Resolve the per-Thing consolidated TomsPrimObject (built lazily).
    TomsPrimObject* pPrimObj = NULL;
    const bool is_person = (p_thing->Class == CLASS_PERSON
                            && chunk->ElementCount == POSE_PERSON_BONE_COUNT);
    if (is_person) {
        const int iIndex = figure_person_iIndex(p_thing);
        ASSERT(iIndex < MAX_NUMBER_D3D_PEOPLE);
        pPrimObj = &(D3DPeopleObj[iIndex]);
        if (pPrimObj->wNumMaterials == 0) {
            const BYTE person_type = (BYTE)p_thing->Genus.Person->PersonType;
            const BYTE person_id   = (BYTE)dt->PersonID;
            BodyDef* body_def = (person_type == PERSON_ROPER)
                ? &chunk->PeopleTypes[person_id >> 5]
                : &chunk->PeopleTypes[person_id & 0x1f];
            const SLONG start_object = prim_multi_objects[chunk->MultiObject[dt->MeshID]].StartObject;
            figure_tpo_build_person(pPrimObj, body_def, start_object);
        }
    } else {
        // Flat-skeleton creature (DT_ANIM_PRIM: bats / Bane / Balrog /
        // Gargoyle). Same consolidation strategy as ANIM_obj_draw uses.
        const SLONG start_object = prim_multi_objects[chunk->MultiObject[0]].StartObject;
        pPrimObj = figure_anim_obj_get_consolidated(chunk, start_object, chunk->ElementCount);
    }
    if (!pPrimObj) return false;

    // Resolve bind palette.
    const GEMatrix* bind_inv   = NULL;
    int             bone_count = 0;
    if (!bind_palette_get(chunk, NULL, &bind_inv, &bone_count) || !bind_inv) return false;

    // Lazy-build the bind-space VBO + per-bone AABB if absent.
    if (pPrimObj->skin_consolidated_world == NULL) {
        figure_build_consolidated_skin_world(pPrimObj, chunk);
    }
    GESkinMesh* mesh = (GESkinMesh*)pPrimObj->skin_consolidated_world;
    if (!mesh || !pPrimObj->skin_consolidated_bone_aabb) return false;

    if (out_mesh)       *out_mesh       = mesh;
    if (out_bone_aabb)  *out_bone_aabb  = pPrimObj->skin_consolidated_bone_aabb;
    if (out_bone_count) *out_bone_count = bone_count;
    if (out_chunk)      *out_chunk      = chunk;
    if (out_bind_inv)   *out_bind_inv   = bind_inv;
    if (out_prim_obj)   *out_prim_obj   = pPrimObj;
    return true;
}

// uc_orig: FIGURE_draw_hierarchical_prim_recurse (fallen/DDEngine/Source/figure.cpp)
// 15-body-part character renderer. Builds a consolidated TomsPrimObject
// on first use (one walk of the body-part hierarchy via FIGURE_TPO_*),
// then draws the whole rig in one pass through the world-skin shader
// (skin_world_vert.glsl) — one ge_skin_world_draw_range call per
// material. Held items (gun / muzzle flash) for armed characters are
// drawn separately as rigid meshes at the hand bone's world transform
// (figure_draw_held_item).
void FIGURE_draw_hierarchical_prim_recurse(Thing* p_person)
{
    ASSERT(p_person->Class == CLASS_PERSON);
    ASSERT(FIGURE_dhpr_rdata1[0].current_child_number == 0);

    int iIndex = (int)(FIGURE_dhpr_data.bPersonType);
    ASSERT(FIGURE_dhpr_data.bPersonType >= 0);
    ASSERT(FIGURE_dhpr_data.bPersonType < PERSON_NUM_TYPES);
    if (FIGURE_dhpr_data.bPersonType == PERSON_CIV) {
        iIndex = (int)(FIGURE_dhpr_data.bPersonID & 0x1f) + PERSON_NUM_TYPES;
    } else if (FIGURE_dhpr_data.bPersonType == PERSON_ROPER) {
        ASSERT((FIGURE_dhpr_data.bPersonID >> 5) < NUM_ROPERS_THINGIES);
        iIndex = (int)(FIGURE_dhpr_data.bPersonID >> 5) + PERSON_NUM_TYPES + 32;
    }

    ASSERT(iIndex < MAX_NUMBER_D3D_PEOPLE);
    ASSERT((PERSON_NUM_TYPES + 32 + NUM_ROPERS_THINGIES) <= MAX_NUMBER_D3D_PEOPLE);
    TomsPrimObject* pPrimObj = &(D3DPeopleObj[iIndex]);
    if (pPrimObj->wNumMaterials == 0) {
        figure_tpo_build_person(pPrimObj,
                                FIGURE_dhpr_data.body_def,
                                FIGURE_dhpr_data.start_object);
    }

    SLONG tex_page_offset = p_person->Genus.Person->pcom_colour & 0x3;
    ASSERT(MM_bLightTableAlreadySetUp);

    ge_set_specular_enabled(false);

    FIGURE_touch_LRU_of_object(pPrimObj);

    ASSERT(pPrimObj->pD3DVertices != NULL);
    ASSERT(pPrimObj->pMaterials != NULL);
    ASSERT(pPrimObj->pwListIndices != NULL);
    ASSERT(pPrimObj->pwStripIndices != NULL);
    ASSERT(pPrimObj->wNumMaterials != 0);

    // ---- Held items (gun + optional muzzle flash) ------------------------
    //
    // Armed non-ROPER characters carry one of three guns in either hand.
    // Drawn as rigid prims at the hand bone's world transform. The gun
    // draw also writes back the muzzle world position for gunfire FX.
    // Muzzle flash is fired by DT_FLAG_GUNFLASH and consumes the flag.
    //
    // The hand is determined by (PersonID >> 5) — id==2 = right hand,
    // anything else (1/3/5) = left hand. id==0 = unarmed → no held items.
    if (p_person->Genus.Person->PersonType != PERSON_ROPER) {
        const SLONG id = p_person->Draw.Tweened->PersonID >> 5;
        if (id) {
            const int hand_part = (id == 2) ? SUB_OBJECT_RIGHT_HAND
                                            : SUB_OBJECT_LEFT_HAND;

            // Muzzle flash — one of three prims depending on which gun
            // ID is in use. Fired by DT_FLAG_GUNFLASH (consumed here).
            if (p_person->Draw.Tweened->Flags & DT_FLAG_GUNFLASH) {
                p_person->Draw.Tweened->Flags &= ~DT_FLAG_GUNFLASH;
                SLONG mf_prim = -1;
                if      (id == 1) mf_prim = 261;
                else if (id == 3) mf_prim = 262;
                else if (id == 5) mf_prim = 263;
                if (mf_prim > 0) {
                    figure_draw_held_item(p_person, mf_prim, hand_part);
                }
            }

            // Gun mesh (prim 256/258/260 for id 1/3/5). figure_draw_held_item
            // also writes p_person->Genus.Person->GunMuzzle for these prims.
            figure_draw_held_item(p_person, 255 + id, hand_part);
        }
    }

    // ---- Body draw: one VBO, world-skin shader, palette = current × inv_bind ----
    //
    // 15-bone person bind palette is built lazily per chunk; once that
    // exists the consolidated bind-space VBO is built lazily per
    // TomsPrimObject. Per-frame: build the skin palette (current × inv_bind)
    // and the camera-only screen bake, then draw each material as a slice
    // of the shared index buffer.
    PrimObjectMaterial* pMat = pPrimObj->pMaterials;
    // 20 bones × 3 vec4 each = 240 floats. Persons use 15; sized to
    // POSE_MAX_BONES so the same buffer works for any rig (matches the
    // ANIM_obj_draw flat-rig variant for code-shape consistency).
    float       skin_palette_world[POSE_MAX_BONES * 12];
    GEMatrix    screen_xform_world = {};
    float       lightdir_world[3]  = { 0.0f, 0.0f, 0.0f };

    ASSERT(p_person->Genus.Person);
    ASSERT(p_person->Draw.Tweened);
    ASSERT(p_person->Draw.Tweened->TheChunk);
    ASSERT(p_person->Draw.Tweened->TheChunk->ElementCount == POSE_PERSON_BONE_COUNT);
    const GameKeyFrameChunk* chunk    = p_person->Draw.Tweened->TheChunk;
    const GEMatrix*          bind_inv = NULL;
    if (!bind_palette_get(chunk, NULL, &bind_inv, NULL) || !bind_inv) {
        ge_set_specular_enabled(true);
        return;
    }
    if (pPrimObj->skin_consolidated_world == NULL) {
        figure_build_consolidated_skin_world(pPrimObj, chunk);
    }
    GESkinMesh* worldMesh  = (GESkinMesh*)pPrimObj->skin_consolidated_world;
    uint32_t*   skinRanges = pPrimObj->skin_consolidated_ranges;
    if (!worldMesh || !skinRanges) {
        ge_set_specular_enabled(true);
        return;
    }
    const BoneInterpTransform* current = render_interp_get_cached_pose(p_person);
    if (!current) {
        ge_set_specular_enabled(true);
        return;
    }

    // Anchor g_matWorld at the root bone (pelvis) so the per-material
    // fog read further down (g_mm_fog_view_z = g_matWorld._43) sees this
    // character's view-Z and not stale leftover from a previous draw.
    // figure_build_screen_xform_bake save/restores g_matWorld around
    // its camera-only override, so the value we set here survives that
    // call. Held-item draws above (gun / muzzle flash) also touched
    // g_matWorld, leaving the hand bone transform there for ARMED
    // characters — unarmed characters would otherwise inherit fog Z
    // from whatever the last frame's last draw left in g_matWorld, and
    // come out black at random camera angles (regression introduced when
    // P2-G.7 removed the per-bone _just_set_matrix walk).
    {
        float ident_mat[9] = { 1, 0, 0,  0, 1, 0,  0, 0, 1 };
        POLY_set_local_rotation(current[0].pos_x, current[0].pos_y, current[0].pos_z, ident_mat);
    }

    figure_build_skin_world_palette(current, bind_inv,
                                    POSE_PERSON_BONE_COUNT,
                                    skin_palette_world);
    figure_build_screen_xform_bake(&screen_xform_world);
    // Half-Lambert ramp divides dot(normal, L) by fNormScale = 251 to
    // land cos in [-1,1]. Pre-scale L by 251 here so the ramp index
    // matches the same scaling the original CPU lit path used.
    {
        constexpr float MM_LIGHT_SCALE = 251.0f;
        lightdir_world[0] = MM_vLightDir.x * MM_LIGHT_SCALE;
        lightdir_world[1] = MM_vLightDir.y * MM_LIGHT_SCALE;
        lightdir_world[2] = MM_vLightDir.z * MM_LIGHT_SCALE;
    }

    extern GEMatrix g_matWorld;
    g_mm_fog_view_z = g_matWorld._43;

    int iMatIndex = 0;
    for (int iMatNum = pPrimObj->wNumMaterials; iMatNum > 0; iMatNum--, iMatIndex++) {
        UWORD wPage     = pMat->wTexturePage;
        UWORD wRealPage = wPage & TEXTURE_PAGE_MASK;

        if (wPage & TEXTURE_PAGE_FLAG_JACKET) {
            wRealPage = jacket_lookup[wRealPage][GET_SKILL(p_person) >> 2];
            wRealPage += FACE_PAGE_OFFSET;
        } else if (wPage & TEXTURE_PAGE_FLAG_OFFSET) {
            if (tex_page_offset == 0) {
                wRealPage += FACE_PAGE_OFFSET;
            } else {
                wRealPage = alt_texture[wRealPage - (10 * 64)] + tex_page_offset - 1;
            }
        }

        const bool tinted = (wPage & TEXTURE_PAGE_FLAG_TINT) != 0;
        const float* fadeStart = tinted ? MM_FadeStartTint : MM_FadeStart;
        const float* fadeStep  = tinted ? MM_FadeStepTint  : MM_FadeStep;

        PolyPage* pa = &(POLY_Page[wRealPage]);
        pa->RS.SetCullMode(GECullMode::CCW);
        pa->RS.SetAlphaBlendEnabled(false);
        pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
        pa->RS.SetChanged();

        const uint32_t index_start = skinRanges[iMatIndex * 2 + 0];
        const uint32_t index_count = skinRanges[iMatIndex * 2 + 1];
        if (index_count > 0) {
            ge_skin_world_draw_range(
                worldMesh, index_start, index_count,
                skin_palette_world,
                POSE_PERSON_BONE_COUNT,
                &screen_xform_world,
                lightdir_world,
                g_mm_fog_view_z,
                /*unlit=*/ true,
                fadeStart, fadeStep);
        }

        pMat++;
    }

    ge_set_specular_enabled(true);

    ASSERT(p_person && (p_person->Class != CLASS_VEHICLE));
    ASSERT(MM_bLightTableAlreadySetUp);
}

// uc_orig: FIGURE_draw (fallen/DDEngine/Source/figure.cpp)
// Top-level entry point: renders one character Thing* for the current frame.
// Dispatches to hierarchical (15-part D3D) or flat loop depending on ElementCount.
// Also renders held grenade and handles immolation (Pyro fire overlay).
void FIGURE_draw(Thing* p_thing)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;
    SLONG wx, wy, wz;

    ULONG colour;
    ULONG specular;

    Matrix33 r_matrix;

    GameKeyFrameElement* ae1;
    GameKeyFrameElement* ae2;

    DrawTween* dt = p_thing->Draw.Tweened;

    if (dt->CurrentFrame == 0 || dt->NextFrame == 0) {
        MSG_add("!!!!!!!!!!!!!!!!!!!!!!!!ERROR AENG_draw_figure");
        return;
    }

    dx = 0;
    dy = 0;
    dz = 0;

    FIGURE_rotate_obj(
        dt->Tilt,
        dt->Angle,
        (dt->Roll + 2048) & 2047,
        &r_matrix);

    wx = 0;
    wy = 0;
    wz = 0;

    ae1 = dt->CurrentFrame->FirstElement;
    ae2 = dt->NextFrame->FirstElement;

    if (!ae1 || !ae2) {
        MSG_add("!!!!!!!!!!!!!!!!!!!ERROR FIGURE_draw has no animation elements");
        return;
    }

    colour = 0;
    specular = 0;

    SLONG lx;
    SLONG ly;
    SLONG lz;

    {
        // Pelvis world position for NIGHT_find lighting lookup, from the
        // interpolated pose snapshot — the single always-available source
        // (lazily captured on first use; no legacy recompute path).
        const BoneInterpTransform* pose = render_interp_get_cached_pose(p_thing);
        lx = SLONG(pose[0].pos_x);
        ly = SLONG(pose[0].pos_y);
        lz = SLONG(pose[0].pos_z);
    }

    NIGHT_find(lx, ly, lz);

    ASSERT(!MM_bLightTableAlreadySetUp);

    Pyro* p = NULL;
    if (p_thing->Class == CLASS_PERSON && p_thing->Genus.Person->BurnIndex) {
        p = TO_PYRO(p_thing->Genus.Person->BurnIndex - 1);
        if (p->PyroType != PYRO_IMMOLATE) {
            p = NULL;
        }
    }

    BuildMMLightingTable(p, 0);

    MM_bLightTableAlreadySetUp = UC_TRUE;

    SLONG ele_count;
    SLONG start_object;
    SLONG object_offset;

    ele_count = dt->TheChunk->ElementCount;
    start_object = prim_multi_objects[dt->TheChunk->MultiObject[dt->MeshID]].StartObject;

    // Persons always have a 15-bone rig (the world-skin path is built on
    // that assumption). A non-15 ElementCount on a CLASS_PERSON Thing
    // would mean corrupted/unexpected data — fail loud in debug, skip
    // the body draw cleanly in release.
    ASSERT(ele_count == POSE_PERSON_BONE_COUNT);
    if (ele_count == POSE_PERSON_BONE_COUNT) {
        Matrix31 pos;
        pos.M[0] = (p_thing->WorldPos.X >> 8) + wx;
        pos.M[1] = (p_thing->WorldPos.Y >> 8) + wy;
        pos.M[2] = (p_thing->WorldPos.Z >> 8) + wz;

        FIGURE_dhpr_rdata1[0].part_number = 0;
        FIGURE_dhpr_rdata1[0].current_child_number = 0;

        FIGURE_dhpr_data.start_object = start_object;
        if (p_thing->Genus.Person->PersonType == PERSON_ROPER)
            FIGURE_dhpr_data.body_def = &dt->TheChunk->PeopleTypes[(dt->PersonID >> 5)];
        else
            FIGURE_dhpr_data.body_def = &dt->TheChunk->PeopleTypes[(dt->PersonID & 0x1f)];
        FIGURE_dhpr_data.world_pos = &pos;
        FIGURE_dhpr_data.tween = dt->AnimTween;
        FIGURE_dhpr_data.ae1 = ae1;
        FIGURE_dhpr_data.ae2 = ae2;
        FIGURE_dhpr_data.world_mat = &r_matrix;
        FIGURE_dhpr_data.dx = dx;
        FIGURE_dhpr_data.dy = dy;
        FIGURE_dhpr_data.dz = dz;
        FIGURE_dhpr_data.colour = colour;
        FIGURE_dhpr_data.specular = specular;

        FIGURE_dhpr_data.bPersonType = p_thing->Genus.Person->PersonType;
        FIGURE_dhpr_data.bPersonID = dt->PersonID;

        FIGURE_draw_hierarchical_prim_recurse(p_thing);
    }
    // object_offset declared above is now unused with the legacy
    // non-15-bone person fallback removed; left in place rather than
    // churning the local-variable list.
    (void)object_offset;

    ASSERT(MM_bLightTableAlreadySetUp);
    MM_bLightTableAlreadySetUp = UC_FALSE;

    if (p_thing->Class == CLASS_PERSON) {

        Thing* p_person = p_thing;

        if (p_person->Genus.Person->SpecialUse && TO_THING(p_person->Genus.Person->SpecialUse)->Genus.Special->SpecialType == SPECIAL_GRENADE) {
            SLONG px;
            SLONG py;
            SLONG pz;

            // Left hand world position for grenade-in-hand draw, from the
            // interpolated pose snapshot — the single always-available source
            // (keeps the grenade tracking the hand smoothly; no legacy path).
            const BoneInterpTransform* pose = render_interp_get_cached_pose(p_person);
            px = SLONG(pose[SUB_OBJECT_LEFT_HAND].pos_x);
            py = SLONG(pose[SUB_OBJECT_LEFT_HAND].pos_y);
            pz = SLONG(pose[SUB_OBJECT_LEFT_HAND].pos_z);

            kludge_shrink = UC_TRUE;

            MESH_draw_poly(
                PRIM_OBJ_ITEM_GRENADE,
                px,
                py,
                pz,
                0, 0, 0,
                NULL, 0xff);

            kludge_shrink = UC_FALSE;
        }
    }
    p_thing->Flags &= ~FLAGS_PERSON_AIM_AND_RUN;
}

// Resolve or build the consolidated TomsPrimObject for one
// (chunk, start_object) rig. Returns NULL if the cache is full or any of
// the constituent prim_objects[] can't be compiled. Slot reuse is keyed
// by chunk+start_object so different rigs never share a slot.
//
// Walks parts 0..ele_count-1, calling FIGURE_TPO_add_prim_to_current_object
// with ubSubObjectNumber = i so each merged vertex carries its origin
// part as its bone index — same shape as D3DPeopleObj for persons.
static TomsPrimObject* figure_anim_obj_get_consolidated(const GameKeyFrameChunk* chunk,
                                                       SLONG start_object,
                                                       SLONG ele_count)
{
    // Scan: prefer an exact-key match (chunk + start_object). Cached
    // slot may be in EVICTED state — `wNumMaterials == 0` after
    // FIGURE_clean_LRU_slot cleared the TPO data but our key here stayed
    // pointing at the chunk. Re-build into that same slot below.
    int slot = -1;
    int free_slot = -1;
    for (int i = 0; i < MAX_NUMBER_D3D_ANIMALS; ++i) {
        if (D3DAnimObjKeys[i].chunk == chunk && D3DAnimObjKeys[i].start_object == start_object) {
            slot = i;
            break;
        }
        if (free_slot < 0 && D3DAnimObjKeys[i].chunk == NULL)
            free_slot = i;
    }
    if (slot < 0) slot = free_slot;
    if (slot < 0) return NULL;

    TomsPrimObject* pPrimObj = &D3DAnimObj[slot];
    if (pPrimObj->wNumMaterials == 0) {
        // First use of this slot OR the slot was LRU-evicted (TPO data
        // freed). Either way, (re)build the consolidated TPO. After
        // FIGURE_TPO_finish_3d_object the slot is LRU-registered again
        // with a valid bLRUQueueNumber.
        FIGURE_TPO_init_3d_object(pPrimObj);
        for (SLONG i = 0; i < ele_count; ++i) {
            FIGURE_TPO_add_prim_to_current_object(start_object + i, (UBYTE)i);
        }
        FIGURE_TPO_finish_3d_object(pPrimObj, 1);
    }
    D3DAnimObjKeys[slot].chunk        = chunk;
    D3DAnimObjKeys[slot].start_object = start_object;
    return pPrimObj;
}

// uc_orig: ANIM_obj_draw (fallen/DDEngine/Source/figure.cpp)
// Draws a flat-skeleton creature (DT_ANIM_PRIM: bats, Bane, Balrog,
// Gargoyle) through the same world-skin path persons use — a single
// consolidated VBO per (chunk, start_object), one ge_skin_world_draw_range
// call per material, per-frame palette = current world transforms from
// the interpolated pose snapshot. The bind palette is identity-rotation
// + raw keyframe-0 offsets (built by bind_palette_get's flat branch);
// vertex weights are the trivial single-bone w0=255 the consolidated
// build writes by default — auto-rig is people-anatomy-specific and not
// applied here. Falls through silently with no draw if the rig can't be
// resolved (missing chunk data, etc.).
void ANIM_obj_draw(Thing* p_thing, DrawTween* dt)
{
    if (!dt || dt->CurrentFrame == 0 || dt->NextFrame == 0) {
        MSG_add("!!!!!!!!!!!!!!!!!!!!!!!!ERROR AENG_draw_figure");
        return;
    }
    if (!dt->CurrentFrame->FirstElement || !dt->NextFrame->FirstElement) {
        MSG_add("!!!!!!!!!!!!!!!!!!!ERROR AENG_draw_figure has no animation elements");
        return;
    }
    const GameKeyFrameChunk* chunk = dt->TheChunk;
    if (!chunk) return;

    // Lighting — same as the legacy per-part path: night-tinted fade
    // table sampled at a point slightly above the thing's foot world
    // pos so foot-level shadow doesn't dominate.
    {
        SLONG lx = (p_thing->WorldPos.X >> 8);
        SLONG ly = (p_thing->WorldPos.Y >> 8) + 0x60;
        SLONG lz = (p_thing->WorldPos.Z >> 8);
        NIGHT_find(lx, ly, lz);
    }
    BuildMMLightingTable(NULL, 0xffff00ff);
    MM_bLightTableAlreadySetUp = UC_TRUE;

    // Per-frame world bone transforms (with body angles + character_scale
    // baked in — see compose_flat_skeletal_pose). Skipping the draw is
    // safe: the pose snapshot is lazily captured on first use, so this
    // is only NULL for a Thing that genuinely can't produce a pose.
    const BoneInterpTransform* current = render_interp_get_cached_pose(p_thing);
    if (!current) {
        MM_bLightTableAlreadySetUp = UC_FALSE;
        return;
    }

    // Bind palette (identity rotation + raw keyframe-0 offsets for flat
    // rigs — see bind_palette.cpp build_palette_flat). Cached per chunk.
    const GEMatrix* bind_inv   = NULL;
    int             bone_count = 0;
    if (!bind_palette_get(chunk, NULL, &bind_inv, &bone_count) || !bind_inv) {
        MM_bLightTableAlreadySetUp = UC_FALSE;
        return;
    }

    // Mesh: lazy consolidated build per (chunk, start_object) slot.
    // ANIM_obj_draw historically hard-codes MultiObject[0] (animals don't
    // use the MeshID variant indexing that persons do), so the slot key
    // is (chunk, MultiObject[0].StartObject).
    const SLONG start_object = prim_multi_objects[chunk->MultiObject[0]].StartObject;
    TomsPrimObject* pPrimObj = figure_anim_obj_get_consolidated(chunk, start_object,
                                                                chunk->ElementCount);
    if (!pPrimObj) {
        MM_bLightTableAlreadySetUp = UC_FALSE;
        return;
    }
    FIGURE_touch_LRU_of_object(pPrimObj);

    // Bind-space VBO build (one-shot per slot — figure_build_consolidated_skin_world
    // is a no-op if already built).
    if (pPrimObj->skin_consolidated_world == NULL) {
        figure_build_consolidated_skin_world(pPrimObj, chunk);
    }
    GESkinMesh* worldMesh  = (GESkinMesh*)pPrimObj->skin_consolidated_world;
    uint32_t*   skinRanges = pPrimObj->skin_consolidated_ranges;
    if (!worldMesh || !skinRanges) {
        MM_bLightTableAlreadySetUp = UC_FALSE;
        return;
    }

    // Anchor g_matWorld at the root bone in world space so fog view-Z
    // is meaningful (the per-material loop below reads g_matWorld._43
    // as the per-character fog distance). Identity local rotation: only
    // the translation matters for fog. figure_build_screen_xform_bake
    // save/restores around its camera-only override, so the value we
    // set here survives that call and the loop sees it.
    {
        float ident_mat[9] = { 1, 0, 0,  0, 1, 0,  0, 0, 1 };
        POLY_set_local_rotation(current[0].pos_x, current[0].pos_y, current[0].pos_z, ident_mat);
    }

    // Pre-character setup for the shared world-skin shader:
    //   skin_palette[i] = current[i] × inv_bind[i]    (one row per bone)
    //   screen_xform    = camera × projection × viewport (no model)
    //   lightdir        = MM_vLightDir × 251 (matches legacy fNormScale)
    //
    // Buffer is sized to POSE_MAX_BONES so the same per-frame allocation
    // fits every rig we might see.
    float    skin_palette[POSE_MAX_BONES * 12];
    GEMatrix screen_xform = {};
    float    lightdir[3]  = { 0.0f, 0.0f, 0.0f };
    figure_build_skin_world_palette(current, bind_inv, bone_count, skin_palette);
    figure_build_screen_xform_bake(&screen_xform);
    {
        constexpr float MM_LIGHT_SCALE = 251.0f;
        lightdir[0] = MM_vLightDir.x * MM_LIGHT_SCALE;
        lightdir[1] = MM_vLightDir.y * MM_LIGHT_SCALE;
        lightdir[2] = MM_vLightDir.z * MM_LIGHT_SCALE;
    }

    extern GEMatrix g_matWorld;
    g_mm_fog_view_z = g_matWorld._43;

    ge_set_specular_enabled(false);

    // Per-material draw — slice the shared VBO by skinRanges[mat*2 + {0,1}].
    PrimObjectMaterial* pMat = pPrimObj->pMaterials;
    const int n_mats = (int)pPrimObj->wNumMaterials;
    for (int iMat = 0; iMat < n_mats; iMat++, pMat++) {
        UWORD wPage     = pMat->wTexturePage;
        UWORD wRealPage = wPage & TEXTURE_PAGE_MASK;
        // Skin/jacket flags are person-only; non-person rigs don't carry
        // them. Drop the FACE_PAGE_OFFSET that the legacy per-part path
        // applied by default — same effective texture page as the OLD
        // pipeline through pa = &POLY_Page[wRealPage] below.
        const bool tinted = (wPage & TEXTURE_PAGE_FLAG_TINT) != 0;
        const float* fadeStart = tinted ? MM_FadeStartTint : MM_FadeStart;
        const float* fadeStep  = tinted ? MM_FadeStepTint  : MM_FadeStep;

        PolyPage* pa = &(POLY_Page[wRealPage]);
        pa->RS.SetCullMode(GECullMode::CCW);
        pa->RS.SetAlphaBlendEnabled(false);
        pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
        pa->RS.SetChanged();

        const uint32_t index_start = skinRanges[iMat * 2 + 0];
        const uint32_t index_count = skinRanges[iMat * 2 + 1];
        if (index_count > 0) {
            ge_skin_world_draw_range(
                worldMesh, index_start, index_count,
                skin_palette,
                (uint32_t)bone_count,
                &screen_xform,
                lightdir,
                g_mm_fog_view_z,
                /*unlit=*/ true,
                fadeStart, fadeStep);
        }
    }

    ge_set_specular_enabled(true);
    MM_bLightTableAlreadySetUp = UC_FALSE;
}

// uc_orig: FIGURE_MAX_DY (fallen/DDEngine/Source/figure.cpp)
#define FIGURE_MAX_DY (128.0F)
// uc_orig: FIGURE_255_DIVIDED_BY_MAX_DY (fallen/DDEngine/Source/figure.cpp)
#define FIGURE_255_DIVIDED_BY_MAX_DY (255.0F / FIGURE_MAX_DY)

// Water-reflection renderer (P2-I). One draw call per material through
// the shared bind-space skinning path (skin_reflect_vert.glsl) — same
// per-frame skin palette the body and shadow use, so any soft weights
// from P2-E deform the reflection in lock-step with the body. The shader
// mirrors Y about the water plane and adds a height-above-water fade to
// the fog alpha. Replaces the legacy CPU rasteriser that walked every
// vertex through Matrix33 + POLY_transform on the host.
//
// Limits (matches the legacy filter): 15-bone people only. Non-people
// reflections (Balrog etc.) are extremely rare in puddle-bearing levels
// and never reflected in the original code either; FIGURE_get_skin_mesh_for_thing
// returns a flat skeleton mesh for those that we explicitly skip below.
//
// height — world-space Y of the water surface to reflect about.

void FIGURE_draw_reflection(Thing* p_thing, SLONG height)
{
    DrawTween* dt = p_thing->Draw.Tweened;
    if (!dt || dt->CurrentFrame == 0 || dt->NextFrame == 0) {
        MSG_add("!!!!!!!!!!!!!!!!!!!!!!!!ERROR FIGURE_draw_reflection");
        return;
    }

    // Resolve the consolidated bind-space VBO + bone AABB + chunk + bind
    // inverse + the underlying TomsPrimObject (for materials / ranges).
    // Lazy-built on first use, shared with the body and shadow draws.
    GESkinMesh*              worldMesh  = NULL;
    const float*             bone_aabb  = NULL;
    int                      bone_count = 0;
    const GameKeyFrameChunk* chunk      = NULL;
    const GEMatrix*          bind_inv   = NULL;
    TomsPrimObject*          pPrimObj   = NULL;
    if (!FIGURE_get_skin_mesh_for_thing(p_thing, &worldMesh, &bone_aabb,
            &bone_count, &chunk, &bind_inv, &pPrimObj)) {
        return;
    }
    // Reflection is only meaningful for 15-bone people (matches the legacy
    // PeopleTypes / PersonID-driven filter). Flat-skeleton creatures never
    // reflected in the original game either.
    if (bone_count != POSE_PERSON_BONE_COUNT) return;
    if (!worldMesh || !bone_aabb || !bind_inv || !pPrimObj) return;
    if (!pPrimObj->skin_consolidated_ranges) return;

    const BoneInterpTransform* current = render_interp_get_cached_pose(p_thing);
    if (!current) return;

    // --- NIGHT lighting at pelvis (flat per-character colour) -----------
    // Identical to the legacy reflection path: sample the NIGHT light at
    // the pelvis position, optionally brighten if too dark (unless we're
    // in cutscene `ControlFlag` mode), pack via NIGHT_get_colour. The
    // halve / force-alpha-FF that the legacy per-bone path did is folded
    // in here so the shader receives the final per-character colour.
    NIGHT_Colour col = NIGHT_get_light_at(SLONG(current[0].pos_x),
                                          SLONG(current[0].pos_y),
                                          SLONG(current[0].pos_z));
    if (!ControlFlag) {
        if (col.red   < 32) col.red   += (32 - col.red)   >> 1;
        if (col.green < 32) col.green += (32 - col.green) >> 1;
        if (col.blue  < 32) col.blue  += (32 - col.blue)  >> 1;
    }
    ULONG colour   = 0;
    ULONG specular = 0;
    NIGHT_get_colour(col, &colour, &specular);
    colour   &= ~POLY_colour_restrict;
    specular &= ~POLY_colour_restrict;
    // Halve + force opaque alpha (legacy `>>= 1; |= 0xff000000`).
    colour   = ((colour   >> 1) & 0x7f7f7f7f) | 0xff000000;
    specular = ((specular >> 1) & 0x7f7f7f7f) | 0xff000000;

    // --- Per-frame skin palette + screen-xform bake (same as body) ------
    // Anchor g_matWorld at the pelvis so figure_build_screen_xform_bake's
    // save/restore preserves a valid per-character view-Z for fog (same
    // gotcha the body draw deals with).
    {
        float ident_mat[9] = { 1, 0, 0,  0, 1, 0,  0, 0, 1 };
        POLY_set_local_rotation(current[0].pos_x, current[0].pos_y,
                                current[0].pos_z, ident_mat);
    }

    float skin_palette[POSE_MAX_BONES * 3 * 4];
    figure_build_skin_world_palette(current, bind_inv, bone_count, skin_palette);

    GEMatrix screen_xform;
    figure_build_screen_xform_bake(&screen_xform);

    extern GEMatrix g_matWorld;
    const float fog_view_z = g_matWorld._43;

    // --- Screen-space bbox for wibble (replaces FIGURE_rpoint walk) -----
    // The wibble post-process in aeng.cpp intersects each puddle's screen
    // bbox (POLY_Point.X/Y from POLY_transform) with the reflection's
    // bbox (FIGURE_reflect_x1/x2/y1/y2). Both must live in the SAME pixel
    // scale — POLY_screen_width / _height (display coords), NOT the FBO
    // viewport scale that figure_build_screen_xform_bake outputs. Hence
    // POLY_transform per corner here, not the bake-matrix math we use for
    // the GPU draw itself (the shader's NDC step normalises the scale
    // difference, but POLY_Point.X for puddles already lives in display
    // coords). 8 corners × 15 bones = 120 calls per character — cheap.
    {
        SLONG x1 = +UC_INFINITY;
        SLONG y1 = +UC_INFINITY;
        SLONG x2 = -UC_INFINITY;
        SLONG y2 = -UC_INFINITY;
        const float H2 = 2.0f * float(height);
        for (int b = 0; b < bone_count; ++b) {
            const float* aabb = &bone_aabb[b * 6]; // min.xyz, max.xyz
            // Bones with no vertices stay at min=+1e30 / max=-1e30 from
            // figure_build_consolidated_skin_world. Transforming those
            // corners through skin[b] yields infinities, blowing up the
            // bbox. Skip same way SMAP_person_gpu does.
            if (aabb[0] > aabb[3]) continue;
            const float* sk = &skin_palette[b * 3 * 4];
            for (int c = 0; c < 8; ++c) {
                const float lx = (c & 1) ? aabb[3] : aabb[0];
                const float ly = (c & 2) ? aabb[4] : aabb[1];
                const float lz = (c & 4) ? aabb[5] : aabb[2];
                // skin: bind-space corner → world.
                const float wx = sk[0]*lx + sk[1]*ly + sk[2]*lz  + sk[3];
                float       wy = sk[4]*lx + sk[5]*ly + sk[6]*lz  + sk[7];
                const float wz = sk[8]*lx + sk[9]*ly + sk[10]*lz + sk[11];
                wy = H2 - wy; // mirror Y about water plane (matches shader)
                POLY_Point pt;
                POLY_transform(wx, wy, wz, &pt);
                if (!pt.MaybeValid()) continue;
                const SLONG px = SLONG(pt.X);
                const SLONG py = SLONG(pt.Y);
                if (px < x1) x1 = px;
                if (px > x2) x2 = px;
                if (py < y1) y1 = py;
                if (py > y2) y2 = py;
            }
        }
        FIGURE_reflect_x1 = x1;
        FIGURE_reflect_y1 = y1;
        FIGURE_reflect_x2 = x2;
        FIGURE_reflect_y2 = y2;
    }

    // --- Per-material draw ---------------------------------------------
    // Same material loop as the body. ranges[] are shared with the body's
    // consolidated mesh (one bind-space VBO per (chunk, MeshID)). Texture
    // page resolve (jacket / offset / face) is identical to body draw.
    constexpr float REFLECT_DY_SCALE = 255.0f / FIGURE_MAX_DY;
    const uint32_t* skinRanges = pPrimObj->skin_consolidated_ranges;
    const SLONG tex_page_offset =
        p_thing->Genus.Person->pcom_colour & 0x3;
    PrimObjectMaterial* pMat = pPrimObj->pMaterials;
    int iMatIndex = 0;
    for (int iMatNum = pPrimObj->wNumMaterials; iMatNum > 0;
         iMatNum--, iMatIndex++, pMat++)
    {
        UWORD wPage     = pMat->wTexturePage;
        UWORD wRealPage = wPage & TEXTURE_PAGE_MASK;
        if (wPage & TEXTURE_PAGE_FLAG_JACKET) {
            wRealPage = jacket_lookup[wRealPage][GET_SKILL(p_thing) >> 2];
            wRealPage += FACE_PAGE_OFFSET;
        } else if (wPage & TEXTURE_PAGE_FLAG_OFFSET) {
            if (tex_page_offset == 0) {
                wRealPage += FACE_PAGE_OFFSET;
            } else {
                wRealPage = alt_texture[wRealPage - (10 * 64)] + tex_page_offset - 1;
            }
        }

        PolyPage* pa = &(POLY_Page[wRealPage]);
        pa->RS.SetCullMode(GECullMode::CCW);
        pa->RS.SetAlphaBlendEnabled(false);
        pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
        pa->RS.SetChanged();

        const uint32_t index_start = skinRanges[iMatIndex * 2 + 0];
        const uint32_t index_count = skinRanges[iMatIndex * 2 + 1];
        if (index_count > 0) {
            ge_skin_reflect_draw_range(
                worldMesh, index_start, index_count,
                skin_palette, bone_count,
                &screen_xform,
                float(height), REFLECT_DY_SCALE,
                colour, specular, fog_view_z);
        }
    }
}

