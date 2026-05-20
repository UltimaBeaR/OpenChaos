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
#include <vector>  // figure_build_consolidated_skin scratch buffers (P2-A)
#include <cstring> // memcpy
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
// Pre-computes a 128-entry ULONG lookup table used by the D3D MultiMatrix extension.
// Index 0..63: ramp from ambient to full directional lighting.
// Index 64..127: flat ambient colour (back-face / shadow side).
// p: if non-null, the character is on fire (darkens with soot).
// colour_and: applied to MM_pcFadeTableTint for tinted texture materials.
// uc_orig: AMBIENT_FACTOR (fallen/DDEngine/Source/figure.cpp)
#define AMBIENT_FACTOR (0.5f * 256.0f)
void BuildMMLightingTable(Pyro* p, DWORD colour_and)
{
    ASSERT(MM_pcFadeTable != NULL);
    ASSERT(MM_pcFadeTableTint != NULL);
    ASSERT(MM_pMatrix != NULL);
    ASSERT(MM_Vertex != NULL);
    ASSERT(MM_pNormal != NULL);

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

    // Darken on-fire characters with soot.
    if (p != NULL) {
        cvLight.r = (cvLight.r > p->counter) ? (cvLight.r - p->counter) : 10;
        cvLight.g = (cvLight.g > p->counter) ? (cvLight.g - p->counter) : 4;
        cvLight.b = (cvLight.b > p->counter) ? (cvLight.b - p->counter) : 3;
        cvAmb.r = (cvAmb.r > p->counter) ? (cvAmb.r - p->counter) : 10;
        cvAmb.g = (cvAmb.g > p->counter) ? (cvAmb.g - p->counter) : 4;
        cvAmb.b = (cvAmb.b > p->counter) ? (cvAmb.b - p->counter) : 3;
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

    // Fill slots 64..127 with flat ambient colour.
    unsigned char cR, cG, cB;
    if (cvCur.r > 255.0f) {
        cR = 255;
    } else if (cvCur.r < 0.0f) {
        cR = 0;
    } else {
        cR = (unsigned char)(cvCur.r);
    }
    if (cvCur.g > 255.0f) {
        cG = 255;
    } else if (cvCur.g < 0.0f) {
        cG = 0;
    } else {
        cG = (unsigned char)(cvCur.g);
    }
    if (cvCur.b > 255.0f) {
        cB = 255;
    } else if (cvCur.b < 0.0f) {
        cB = 0;
    } else {
        cB = (unsigned char)(cvCur.b);
    }

    DWORD dwColour2 = ((DWORD)cR << 16) | ((DWORD)cG << 8) | ((DWORD)cB);
    for (int i = 64; i < 128; i++) {
        MM_pcFadeTable[i] = dwColour2;
        MM_pcFadeTableTint[i] = dwColour2 & colour_and;
    }

    // Fill slots 0..63 with a ramp from ambient to fully lit.
    for (int i = 0; i < 64; i++) {
        if (cvCur.r > 255.0f) {
            cR = 255;
        } else if (cvCur.r < 0.0f) {
            cR = 0;
        } else {
            cR = (unsigned char)(cvCur.r);
        }
        if (cvCur.g > 255.0f) {
            cG = 255;
        } else if (cvCur.g < 0.0f) {
            cG = 0;
        } else {
            cG = (unsigned char)(cvCur.g);
        }
        if (cvCur.b > 255.0f) {
            cB = 255;
        } else if (cvCur.b < 0.0f) {
            cB = 0;
        } else {
            cB = (unsigned char)(cvCur.b);
        }
        DWORD dwColour3 = ((DWORD)cR << 16) | ((DWORD)cG << 8) | ((DWORD)cB);
        MM_pcFadeTable[i] = dwColour3;
        MM_pcFadeTableTint[i] = dwColour3 & colour_and;
        cvCur.r += cvLight.r;
        cvCur.g += cvLight.g;
        cvCur.b += cvLight.b;
    }
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

// Milestone 1D (skeletal_skinning_plan.md): return the persistent GPU-mesh
// cache slot for one model material, lazily allocating the per-material
// array on the model. Returns NULL only on allocation failure (→ caller
// streams). The array is freed in FIGURE_clean_LRU_slot.
static void** figure_skin_cache_slot(TomsPrimObject* po, PrimObjectMaterial* pMat)
{
    if (po->skin_gpu == NULL) {
        po->skin_gpu = (void**)MemAlloc(po->wNumMaterials * sizeof(void*));
        if (po->skin_gpu == NULL)
            return NULL;
        for (int i = 0; i < (int)po->wNumMaterials; i++)
            po->skin_gpu[i] = NULL;
    }
    return &po->skin_gpu[pMat - po->pMaterials];
}

// Phase 2 P2-A (skeletal_skinning_phase2_plan.md): build one consolidated
// GESkinMesh holding the geometry of ALL materials of a person's
// TomsPrimObject. Replaces per-material GPU caching with one VBO + N
// index slices (one per material) drawn via ge_skin_mesh_draw_range.
//
// Mirrors the ge_draw_multi_matrix accumulation in polypage.cpp:
//  - GEVertex stream per material → GESkinVertex with model-space pos,
//    raw normal, bMatIndex bone, color/specular zeroed (unlit path
//    re-derives them in-shader).
//  - Strip indices (UC_TRUE-init even/odd winding flip, 0xFFFF terminator)
//    → triangle list, with per-material base-vertex offset so all
//    materials share one VBO.
//
// On success stores the mesh and a 2 × wNumMaterials uint32 ranges array
// (index_start, index_count pairs) on pPrimObj. Failure (alloc, > 65535
// verts) leaves consolidated = NULL; caller falls back to per-material
// path.
static bool figure_build_consolidated_skin(TomsPrimObject* pPrimObj)
{
    if (pPrimObj->skin_consolidated != NULL)
        return true;
    if (pPrimObj->wNumMaterials == 0 || pPrimObj->pMaterials == NULL
        || pPrimObj->pD3DVertices == NULL || pPrimObj->pwStripIndices == NULL)
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

        // Copy this material's vertices to the shared VBO.
        // GEVertex view gives normals; the GEVertexLit view (same memory)
        // is used only to read the MM index byte (offset 12).
        GEVertex* pMatVerts = pVertex;
        GEVertexLit* pMatVertsLit = (GEVertexLit*)pMatVerts;
        for (uint32_t v = 0; v < pMat->wNumVertices; v++) {
            GESkinVertex sv;
            sv.x  = pMatVerts[v].x;
            sv.y  = pMatVerts[v].y;
            sv.z  = pMatVerts[v].z;
            sv.nx = pMatVerts[v].nx;
            sv.ny = pMatVerts[v].ny;
            sv.nz = pMatVerts[v].nz;
            BYTE bMatIndex = ((unsigned char*)(pMatVertsLit + v))[12];
            sv.bone = (uint32_t)bMatIndex;
            sv.color    = 0u; // unlit: derived in-shader
            sv.specular = 0u; // fog computed in-shader (1C)
            sv.u = pMatVertsLit[v].u;
            sv.v = pMatVertsLit[v].v;
            verts.push_back(sv);

            if ((uint32_t)bMatIndex + 1 > palette_n)
                palette_n = (uint32_t)bMatIndex + 1;
        }

        // Decode strip indices to triangle list — same walk as
        // ge_draw_multi_matrix (polypage.cpp): read 2, then loop reading
        // 1 at a time, emit triangle with even/odd winding, 0xFFFF
        // terminates the strip. Multiple strips per material are
        // separated by 0xFFFF and walked until indices run out.
        UWORD* p = pwStripIndices;
        uint32_t remaining = pMat->wNumStripIndices;
        while (remaining >= 3) {
            uint16_t wi[3];
            wi[1] = *p++;
            wi[2] = *p++;
            remaining -= 2;
            bool bEven = true; // matches UC_TRUE init in polypage.cpp
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
                    a = wi[0]; b = wi[2]; c = wi[1]; // {0,2,1}
                } else {
                    a = wi[0]; b = wi[1]; c = wi[2]; // {0,1,2}
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
    if (verts.size() > 0xFFFFu) {
        // Index range exceeds uint16. Caller falls back to per-material
        // path. Unlikely for a person (~hundreds of verts/material).
        return false;
    }

    GESkinMesh* mesh = ge_skin_mesh_create(
        verts.data(), (uint32_t)verts.size(),
        inds.data(), (uint32_t)inds.size(),
        palette_n);
    if (!mesh)
        return false;

    uint32_t* ranges_storage = (uint32_t*)MemAlloc(ranges.size() * sizeof(uint32_t));
    if (!ranges_storage) {
        ge_skin_mesh_destroy(mesh);
        return false;
    }
    std::memcpy(ranges_storage, ranges.data(), ranges.size() * sizeof(uint32_t));

    pPrimObj->skin_consolidated = mesh;
    pPrimObj->skin_consolidated_ranges = ranges_storage;
    return true;
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
    float                      out_palette[POSE_PERSON_BONE_COUNT * 12])
{
    constexpr float S = 1.0f / 32768.0f;
    for (int i = 0; i < POSE_PERSON_BONE_COUNT; ++i) {
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
}

// Phase 2 P2-C helper: build the per-character screen-xform bake — the
// camera*projection*viewport matrix shared by every bone (no per-bone
// transform baked in; skin handles that). Same column layout as
// MMBodyParts_pMatrix per FIGURE_draw_prim_tween_person_only_just_set_matrix
// (figure.cpp:3470-3516), but the input matTemp uses CAMERA-ONLY g_matWorld.
//
// Caller responsibility: g_matWorld must hold the camera-only transform
// when this is called — POLY_set_local_rotation_none() resets it to that
// state. After this returns, g_matWorld retains the camera-only state
// (the caller should not depend on a particular value past this point).
static void figure_build_screen_xform_bake(GEMatrix* out)
{
    extern GEMatrix g_matProjection;
    extern GEMatrix g_matWorld;
    extern GEViewport g_viewData;
    extern DWORD g_dw3DStuffHeight;
    extern DWORD g_dw3DStuffY;

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

    // Viewport bake — identical column layout to MMBodyParts_pMatrix in
    // figure.cpp:3499-3516. Column 1 is unused (._{1,1}=._{2,1}=._{3,1}=0,
    // ._{4,1} is the EVAL validation pattern in the baked path; the new
    // shader doesn't validate, so 0 is fine here).
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
}

// Phase 2 P2-C (skeletal_skinning_phase2_plan.md): build the bind-space
// counterpart to skin_consolidated. Every vertex (and normal) is
// pre-multiplied by bind_palette[bMatIndex] of the matching anim_type, so
// the resulting mesh lives in a single shared bind-space and is ready for
// the world-space skin path (skin_world_vert.glsl).
//
// Same topology / per-material ranges as figure_build_consolidated_skin —
// reuses the existing skin_consolidated_ranges (so this must be called
// AFTER the bone-local mesh build). On success stores the GPU mesh on
// pPrimObj->skin_consolidated_world; failure (alloc, > 65535 verts,
// missing bind palette) leaves it NULL — caller falls back to the
// baked-palette path.
static bool figure_build_consolidated_skin_world(TomsPrimObject* pPrimObj, int anim_type)
{
    if (pPrimObj->skin_consolidated_world != NULL)
        return true;
    if (pPrimObj->wNumMaterials == 0 || pPrimObj->pMaterials == NULL
        || pPrimObj->pD3DVertices == NULL || pPrimObj->pwStripIndices == NULL)
        return false;

    // Bind palette gates the build: only 15-bone person rigs participate
    // in the world path. Other chunks (animals, bats, Bane, Balrog) fall
    // through to the baked path here.
    const GEMatrix* bind_world = NULL;
    if (!bind_palette_get(anim_type, &bind_world, NULL) || !bind_world)
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
            // Skin world path is only valid for 15-bone person rigs.
            // Out-of-range bone index here means the loader produced
            // something this rig can't handle; bail rather than indexing
            // beyond bind_world[15]. ASSERT in debug, fail cleanly in release.
            ASSERT(bMatIndex < 15);
            if (bMatIndex >= 15)
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

    GESkinMesh* mesh = ge_skin_mesh_create(
        verts.data(), (uint32_t)verts.size(),
        inds.data(), (uint32_t)inds.size(),
        palette_n);
    if (!mesh)
        return false;

    pPrimObj->skin_consolidated_world = mesh;
    // Ranges array is identical to the bone-local mesh — make sure it
    // exists. The caller is expected to drive both builds, so this is
    // a defensive check; if missing we still keep the mesh and the draw
    // path can produce ranges on demand from the bone-local build.
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
    // Milestone 1D: free the persistent GPU skin meshes alongside the CPU
    // mesh — buffer lifetime is tied to the model's lifetime. Done before
    // wNumMaterials is zeroed (it sizes the array).
    if (ptpo->skin_gpu != NULL) {
        for (int iMat = 0; iMat < (int)ptpo->wNumMaterials; iMat++) {
            if (ptpo->skin_gpu[iMat] != NULL)
                ge_skin_mesh_destroy((GESkinMesh*)ptpo->skin_gpu[iMat]);
        }
        MemFree(ptpo->skin_gpu);
        ptpo->skin_gpu = NULL;
    }
    // Phase 2 P2-A: consolidated single-VBO mesh + per-material ranges.
    if (ptpo->skin_consolidated != NULL) {
        ge_skin_mesh_destroy((GESkinMesh*)ptpo->skin_consolidated);
        ptpo->skin_consolidated = NULL;
    }
    if (ptpo->skin_consolidated_ranges != NULL) {
        MemFree(ptpo->skin_consolidated_ranges);
        ptpo->skin_consolidated_ranges = NULL;
    }
    // Phase 2 P2-C: parallel bind-space mesh shares the ranges array with
    // the bone-local mesh above (freed there), only the GPU buffer is
    // separate.
    if (ptpo->skin_consolidated_world != NULL) {
        ge_skin_mesh_destroy((GESkinMesh*)ptpo->skin_consolidated_world);
        ptpo->skin_consolidated_world = NULL;
    }

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

    // Milestone 1D: every TPO that can reach the LRU/clean path is init'd
    // here first, so NULL the skin-mesh cache here — removes any reliance
    // on the struct's memory being zero-initialised (the asserts above
    // guarantee a fresh object, so this never leaks an existing array).
    pPrimObj->skin_gpu = NULL;
    // Phase 2 P2-A: consolidated single-VBO + per-material ranges.
    pPrimObj->skin_consolidated = NULL;
    pPrimObj->skin_consolidated_ranges = NULL;
    // Phase 2 P2-C: parallel bind-space mesh (built on demand when the
    // world-skin path is enabled).
    pPrimObj->skin_consolidated_world = NULL;

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

// uc_orig: FIGURE_draw_prim_tween (fallen/DDEngine/Source/figure.cpp)
// Flat-skeleton body-part renderer (DT_ANIM_PRIM: bats / Bane / Balrog /
// Gargoyle via ANIM_obj_draw, plus the non-15 ElementCount person fallback).
// Per-part WORLD transform comes from the interpolation pose snapshot (the
// single source of pose). Submits faces via POLY_add_quad/POLY_add_triangle
// and the GPU MultiMatrix path on opaque non-near-clipped materials.
// part_number: which skeleton slot this prim represents (0xffffffff = unknown).
// colour_and: multiplied into the tint fade table.
void FIGURE_draw_prim_tween(
    SLONG prim,
    ULONG colour,
    ULONG specular,
    Thing* p_thing,
    SLONG part_number,
    ULONG colour_and)
{

    SLONG i;

    SLONG sp;
    SLONG ep;

    SLONG p0;
    SLONG p1;
    SLONG p2;
    SLONG p3;

    SLONG page;

    Matrix33 mat_final;

    PrimFace4* p_f4;
    PrimFace3* p_f3;
    PrimObject* p_obj;

    POLY_Point* pp;

    POLY_Point* tri[3];
    POLY_Point* quad[4];
    SLONG tex_page_offset;

    tex_page_offset = p_thing->Genus.Person->pcom_colour & 0x3;

    // Per-part WORLD transform read straight from the interpolation pose
    // snapshot — the SINGLE always-available source of pose. No legacy
    // keyframe recompute. Flat, non-recursive path; part_number is the
    // element index.
    const BoneInterpTransform* pose = render_interp_get_cached_pose(p_thing);
    if (!pose || part_number < 0 || part_number >= POSE_MAX_BONES)
        return;

    const BoneInterpTransform& xf = pose[part_number];
    float off_x = xf.pos_x;
    float off_y = xf.pos_y;
    float off_z = xf.pos_z;
    mat_final = xf.rot;

    float fmatrix[9];
    fmatrix[0] = float(mat_final.M[0][0]) * (1.0F / 32768.0F);
    fmatrix[1] = float(mat_final.M[0][1]) * (1.0F / 32768.0F);
    fmatrix[2] = float(mat_final.M[0][2]) * (1.0F / 32768.0F);
    fmatrix[3] = float(mat_final.M[1][0]) * (1.0F / 32768.0F);
    fmatrix[4] = float(mat_final.M[1][1]) * (1.0F / 32768.0F);
    fmatrix[5] = float(mat_final.M[1][2]) * (1.0F / 32768.0F);
    fmatrix[6] = float(mat_final.M[2][0]) * (1.0F / 32768.0F);
    fmatrix[7] = float(mat_final.M[2][1]) * (1.0F / 32768.0F);
    fmatrix[8] = float(mat_final.M[2][2]) * (1.0F / 32768.0F);

    POLY_set_local_rotation(
        off_x,
        off_y,
        off_z,
        fmatrix);

    p_obj = &prim_objects[prim];

    sp = p_obj->StartPoint;
    ep = p_obj->EndPoint;

    POLY_buffer_upto = 0;

    // Gun muzzle position extraction: vertex 0 of each gun prim is the muzzle point.
    if (prim == 256) {
        i = sp;
    } else if (prim == 258) {
        i = sp + 15;
    } else if (prim == 260) {
        i = sp + 32;
    } else
        goto no_muzzle_calcs;

    pp = &POLY_buffer[POLY_buffer_upto]; // reused (no ++)
    pp->x = AENG_dx_prim_points[i].X;
    pp->y = AENG_dx_prim_points[i].Y;
    pp->z = AENG_dx_prim_points[i].Z;
    MATRIX_MUL(
        fmatrix,
        pp->x,
        pp->y,
        pp->z);

    pp->x += off_x;
    pp->y += off_y;
    pp->z += off_z;
    p_thing->Genus.Person->GunMuzzle.X = pp->x * 256;
    p_thing->Genus.Person->GunMuzzle.Y = pp->y * 256;
    p_thing->Genus.Person->GunMuzzle.Z = pp->z * 256;

no_muzzle_calcs:

    if (!MM_bLightTableAlreadySetUp) {
    }

    if (WITHIN(prim, 261, 263)) {
        // Muzzle flash prims: no lighting, flat grey colour.

        for (i = sp; i < ep; i++) {
            ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

            pp = &POLY_buffer[POLY_buffer_upto++];

            POLY_transform_using_local_rotation(
                AENG_dx_prim_points[i].X,
                AENG_dx_prim_points[i].Y,
                AENG_dx_prim_points[i].Z,
                pp);

            pp->colour = 0xff808080;
            pp->specular = 0xff000000;
        }

        for (i = p_obj->StartFace4; i < p_obj->EndFace4; i++) {
            p_f4 = &prim_faces4[i];

            p0 = p_f4->Points[0] - sp;
            p1 = p_f4->Points[1] - sp;
            p2 = p_f4->Points[2] - sp;
            p3 = p_f4->Points[3] - sp;

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
                quad[0]->v = float(p_f4->UV[0][1]) * (1.0F / 32.0F);

                quad[1]->u = float(p_f4->UV[1][0]) * (1.0F / 32.0F);
                quad[1]->v = float(p_f4->UV[1][1]) * (1.0F / 32.0F);

                quad[2]->u = float(p_f4->UV[2][0]) * (1.0F / 32.0F);
                quad[2]->v = float(p_f4->UV[2][1]) * (1.0F / 32.0F);

                quad[3]->u = float(p_f4->UV[3][0]) * (1.0F / 32.0F);
                quad[3]->v = float(p_f4->UV[3][1]) * (1.0F / 32.0F);

                page = p_f4->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f4->TexturePage;

                if (tex_page_offset && page > 10 * 64 && alt_texture[page - 10 * 64]) {
                    page = alt_texture[page - 10 * 64] + tex_page_offset - 1;
                } else
                    page += FACE_PAGE_OFFSET;

                POLY_add_quad(quad, page, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
            }
        }

        for (i = p_obj->StartFace3; i < p_obj->EndFace3; i++) {
            p_f3 = &prim_faces3[i];

            p0 = p_f3->Points[0] - sp;
            p1 = p_f3->Points[1] - sp;
            p2 = p_f3->Points[2] - sp;

            ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
            ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
            ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));

            tri[0] = &POLY_buffer[p0];
            tri[1] = &POLY_buffer[p1];
            tri[2] = &POLY_buffer[p2];

            if (POLY_valid_triangle(tri)) {
                tri[0]->u = float(p_f3->UV[0][0] & 0x3f) * (1.0F / 32.0F);
                tri[0]->v = float(p_f3->UV[0][1]) * (1.0F / 32.0F);

                tri[1]->u = float(p_f3->UV[1][0]) * (1.0F / 32.0F);
                tri[1]->v = float(p_f3->UV[1][1]) * (1.0F / 32.0F);

                tri[2]->u = float(p_f3->UV[2][0]) * (1.0F / 32.0F);
                tri[2]->v = float(p_f3->UV[2][1]) * (1.0F / 32.0F);

                page = p_f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f3->TexturePage;

                if (tex_page_offset && page > 10 * 64 && alt_texture[page - 10 * 64]) {
                    page = alt_texture[page - 10 * 64] + tex_page_offset - 1;
                } else
                    page += FACE_PAGE_OFFSET;

                POLY_add_triangle(tri, page, !(p_f3->DrawFlags & POLY_FLAG_DOUBLESIDED));
            }
        }

        return;
    } else {

        if (!MM_bLightTableAlreadySetUp) {
            // Build lighting table if not already done (lazy for this character).
            Pyro* p = NULL;
            if (p_thing->Class == CLASS_PERSON && p_thing->Genus.Person->BurnIndex) {
                p = TO_PYRO(p_thing->Genus.Person->BurnIndex - 1);
                if (p->PyroType != PYRO_IMMOLATE) {
                    p = NULL;
                }
            }
            BuildMMLightingTable(p, colour_and);
        }

        extern float POLY_cam_matrix_comb[9];
        extern float POLY_cam_off_x;
        extern float POLY_cam_off_y;
        extern float POLY_cam_off_z;

        extern GEMatrix g_matProjection;
        extern GEMatrix g_matWorld;
        extern GEViewport g_viewData;

        GEMatrix matTemp;

        {
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
        }

        // Build the MM matrix from the projection*world combined matrix and viewport.
        // Uses letterbox-mode height/Y override (g_dw3DStuffHeight / g_dw3DStuffY).
        extern DWORD g_dw3DStuffHeight;
        extern DWORD g_dw3DStuffY;
        DWORD dwWidth = g_viewData.dwWidth >> 1;
        DWORD dwHeight = g_dw3DStuffHeight >> 1;
        DWORD dwX = g_viewData.dwX;
        DWORD dwY = g_dw3DStuffY;
        MM_pMatrix[0]._11 = 0.0f;
        MM_pMatrix[0]._12 = matTemp._11 * (float)dwWidth + matTemp._14 * (float)(dwX + dwWidth);
        MM_pMatrix[0]._13 = matTemp._12 * -(float)dwHeight + matTemp._14 * (float)(dwY + dwHeight);
        MM_pMatrix[0]._14 = matTemp._14;
        MM_pMatrix[0]._21 = 0.0f;
        MM_pMatrix[0]._22 = matTemp._21 * (float)dwWidth + matTemp._24 * (float)(dwX + dwWidth);
        MM_pMatrix[0]._23 = matTemp._22 * -(float)dwHeight + matTemp._24 * (float)(dwY + dwHeight);
        MM_pMatrix[0]._24 = matTemp._24;
        MM_pMatrix[0]._31 = 0.0f;
        MM_pMatrix[0]._32 = matTemp._31 * (float)dwWidth + matTemp._34 * (float)(dwX + dwWidth);
        MM_pMatrix[0]._33 = matTemp._32 * -(float)dwHeight + matTemp._34 * (float)(dwY + dwHeight);
        MM_pMatrix[0]._34 = matTemp._34;
        // Validation magic number required by the MM driver.
        ULONG EVal = 0xe0001000;
        MM_pMatrix[0]._41 = *(float*)&EVal;
        MM_pMatrix[0]._42 = matTemp._41 * (float)dwWidth + matTemp._44 * (float)(dwX + dwWidth);
        MM_pMatrix[0]._43 = matTemp._42 * -(float)dwHeight + matTemp._44 * (float)(dwY + dwHeight);
        MM_pMatrix[0]._44 = matTemp._44;

        // 251 is a magic number for the DIP call.
        const float fNormScale = 251.0f;

        // Transform light direction into object space (inverse = transpose for orthonormal matrices).
        GEVector vTemp;
        vTemp.x = MM_vLightDir.x * fmatrix[0] + MM_vLightDir.y * fmatrix[3] + MM_vLightDir.z * fmatrix[6];
        vTemp.y = MM_vLightDir.x * fmatrix[1] + MM_vLightDir.y * fmatrix[4] + MM_vLightDir.z * fmatrix[7];
        vTemp.z = MM_vLightDir.x * fmatrix[2] + MM_vLightDir.y * fmatrix[5] + MM_vLightDir.z * fmatrix[8];

        MM_pNormal[0] = 0.0f;
        MM_pNormal[1] = vTemp.x * fNormScale;
        MM_pNormal[2] = vTemp.y * fNormScale;
        MM_pNormal[3] = vTemp.z * fNormScale;
    }

    // Disable specular — MM extension requires it off.
    ge_set_specular_enabled(false);

    // Lazy-compile the D3D representation of this prim if it hasn't been done yet.
    TomsPrimObject* pPrimObj = &(D3DObj[prim]);
    if (pPrimObj->wNumMaterials == 0) {
        FIGURE_generate_D3D_object(prim);
    }

    FIGURE_touch_LRU_of_object(pPrimObj);

    ASSERT(pPrimObj->pD3DVertices != NULL);
    ASSERT(pPrimObj->pMaterials != NULL);
    ASSERT(pPrimObj->pwListIndices != NULL);
    ASSERT(pPrimObj->pwStripIndices != NULL);
    ASSERT(pPrimObj->wNumMaterials != 0);

    PrimObjectMaterial* pMat = pPrimObj->pMaterials;

    GEMultiMatrix mm;
    mm.matrices = MM_pMatrix;
    mm.lpvLightDirs = MM_pNormal;
    mm.skin_cache = nullptr; // 1D: set per-material before each draw

    GEVertex* pVertex = (GEVertex*)pPrimObj->pD3DVertices;
    UWORD* pwStripIndices = pPrimObj->pwStripIndices;
    for (int iMatNum = pPrimObj->wNumMaterials; iMatNum > 0; iMatNum--) {
        UWORD wPage = pMat->wTexturePage;
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

        extern GEMatrix g_matWorld;

        PolyPage* pa = &(POLY_Page[wRealPage]);
        if (!pa->RS.NeedsSorting() && (FIGURE_alpha == 255)) {
            // Opaque: use fast MultiMatrix path. z guard in ge_draw_multi_matrix handles near vertices.
            if (wPage & TEXTURE_PAGE_FLAG_TINT) {
                mm.lpLightTable = MM_pcFadeTableTint;
            } else {
                mm.lpLightTable = MM_pcFadeTable;
            }
            mm.lpvVertices = pVertex;
            mm.skin_cache = figure_skin_cache_slot(pPrimObj, pMat);

            pa->RS.SetCullMode(GECullMode::CCW);
            pa->RS.SetAlphaBlendEnabled(false);
            pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
            pa->RS.SetChanged();

            {
                // Set view-space Z for CPU fog in ge_draw_multi_matrix.
                g_mm_fog_view_z = g_matWorld._43;

                ge_draw_multi_matrix(
                    GEMMVertexType::Unlit,
                    &mm,
                    pMat->wNumVertices,
                    pwStripIndices,
                    pMat->wNumStripIndices);
            }

        } else {
            // Alpha or near-clipped path: currently unimplemented (fast-reject instead).
        }

        pVertex += pMat->wNumVertices;
        pwStripIndices += pMat->wNumStripIndices;

        pMat++;
    }

    ge_set_specular_enabled(true);

    // uc_orig: Environment mapping for CLASS_VEHICLE exists in the original (figure.cpp:5130-5260)
    // but is dead code: CLASS_VEHICLE things use DT_VEHICLE → draw_car() → MESH_draw_poly(),
    // which has its own envmap handling in mesh.cpp. They never reach FIGURE_draw_prim_tween.
    // CLASS_BIKE also exists in the codebase but bikes are not present in any shipping level.
    // Envmap for vehicles via mesh.cpp already works (visible on windshields and chrome bumpers).

    if (!MM_bLightTableAlreadySetUp) {
    }
}

#include "things/items/special.h"
#include "things/core/interact.h"
#include "engine/input/keyboard_globals.h"
#include "things/core/hierarchy.h"

// uc_orig: FIGURE_draw_hierarchical_prim_recurse (fallen/DDEngine/Source/figure.cpp)
// Optimised 15-body-part character renderer using D3D MultiMatrix batching.
// First pass: builds D3D mesh (if not cached) via FIGURE_TPO_*.
// Second pass: calls FIGURE_draw_prim_tween_person_only_just_set_matrix per part,
// then issues one DrawIndPrimMM call. Falls back to individual-cull variant if
// partial visibility is detected.
void FIGURE_draw_hierarchical_prim_recurse(Thing* p_person)
{
    SLONG recurse_level = 0;
    SLONG dx, dy, dz;
    UWORD f1, f2;
    struct Matrix33* rot_mat;

    f1 = p_person->Draw.Tweened->CurrentFrame->Flags;
    f2 = p_person->Draw.Tweened->NextFrame->Flags;

    dx = ((f1 & 0xe) << (28)) >> 21;
    dx -= ((f2 & 0xe) << (28)) >> 21;

    dy = ((f1 & 0xff00) << (16)) >> 16;
    dy -= ((f2 & 0xff00) << (16)) >> 16;

    dz = ((f1 & 0x70) << (25)) >> 21;
    dz -= ((f1 & 0x70) << (25)) >> 21;

    ASSERT(p_person->Class == CLASS_PERSON);

    ASSERT(recurse_level == 0);
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

    structFIGURE_dhpr_rdata1 FIGURE_dhpr_rdata1_0_copy = FIGURE_dhpr_rdata1[0];
    structFIGURE_dhpr_data FIGURE_dhpr_data_copy = FIGURE_dhpr_data_copy;

    ASSERT(iIndex < MAX_NUMBER_D3D_PEOPLE);
    ASSERT((PERSON_NUM_TYPES + 32 + NUM_ROPERS_THINGIES) <= MAX_NUMBER_D3D_PEOPLE);
    TomsPrimObject* pPrimObj = &(D3DPeopleObj[iIndex]);
    if (pPrimObj->wNumMaterials == 0) {
        FIGURE_TPO_init_3d_object(pPrimObj);
        int iTPOPartNumber = 0;

        recurse_level = 0;
        while (recurse_level >= 0) {
            structFIGURE_dhpr_rdata1* pDHPR1 = FIGURE_dhpr_rdata1 + recurse_level;
            int iPartNumber = pDHPR1->part_number;
            if (pDHPR1->current_child_number == 0) {
                SLONG body_part;

                ASSERT(iPartNumber >= 0);
                ASSERT(iPartNumber <= 14);

                body_part = FIGURE_dhpr_data.body_def->BodyPart[iPartNumber];

                SLONG prim = FIGURE_dhpr_data.start_object + body_part;

                FIGURE_TPO_add_prim_to_current_object(prim, iTPOPartNumber);
                iTPOPartNumber++;
            }

            if (body_part_children[iPartNumber][pDHPR1->current_child_number] != -1) {
                structFIGURE_dhpr_rdata1* pDHPR1Inc = FIGURE_dhpr_rdata1 + recurse_level + 1;

                pDHPR1Inc->current_child_number = 0;

                ASSERT(iPartNumber >= 0);
                ASSERT(iPartNumber <= 14);
                pDHPR1Inc->part_number = body_part_children[iPartNumber][pDHPR1->current_child_number];

                pDHPR1Inc->current_child_number = 0;

                pDHPR1->current_child_number++;

                recurse_level++;
            } else {
                recurse_level--;
            }
        }

        FIGURE_TPO_finish_3d_object(pPrimObj, 1);

        FIGURE_dhpr_rdata1[0] = FIGURE_dhpr_rdata1_0_copy;
        FIGURE_dhpr_data_copy = FIGURE_dhpr_data_copy;
    }

    int iTPOPartNumber = -1;
    bool bWholePersonVisible = UC_TRUE;
    bool bBitsOfPersonVisible = UC_FALSE;

    recurse_level = 0;
    while (recurse_level >= 0) {

        structFIGURE_dhpr_rdata1* pDHPR1 = FIGURE_dhpr_rdata1 + recurse_level;
        int iPartNumber = pDHPR1->part_number;

        if (pDHPR1->current_child_number == 0) {
            {
                SLONG body_part;
                SLONG id;

                ASSERT(iPartNumber >= 0);
                ASSERT(iPartNumber <= 14);

                body_part = FIGURE_dhpr_data.body_def->BodyPart[iPartNumber];
                rot_mat = FIGURE_dhpr_data.world_mat;

                iTPOPartNumber++;
                ASSERT(iTPOPartNumber < MAX_NUM_BODY_PARTS_AT_ONCE);
                bool bVisible = FIGURE_draw_prim_tween_person_only_just_set_matrix(
                    iTPOPartNumber,
                    FIGURE_dhpr_data.start_object + body_part,
                    rot_mat,
                    FIGURE_dhpr_data.dx + dx,
                    FIGURE_dhpr_data.dy + dy,
                    FIGURE_dhpr_data.dz + dz,
                    recurse_level,
                    p_person);

                bWholePersonVisible &= bVisible;
                bBitsOfPersonVisible |= bVisible;

                if (p_person->Genus.Person->PersonType != PERSON_ROPER)
                    if ((id = (p_person->Draw.Tweened->PersonID >> 5))) {

                        SLONG hand;
                        if (id == 2)
                            hand = SUB_OBJECT_RIGHT_HAND;
                        else
                            hand = SUB_OBJECT_LEFT_HAND;

                        if (iPartNumber == hand) {
                            if (p_person->Draw.Tweened->Flags & DT_FLAG_GUNFLASH) {
                                SLONG prim;
                                bool bDrawMuzzleFlash = UC_FALSE;
                                p_person->Draw.Tweened->Flags &= ~DT_FLAG_GUNFLASH;
                                switch (p_person->Draw.Tweened->PersonID >> 5) {
                                case 1:
                                    prim = 261;
                                    bDrawMuzzleFlash = UC_TRUE;
                                    break;

                                case 3:
                                    prim = 262;
                                    bDrawMuzzleFlash = UC_TRUE;
                                    break;

                                case 5:
                                    prim = 263;
                                    bDrawMuzzleFlash = UC_TRUE;
                                    break;

                                default:
                                    break;
                                }

                                if (bDrawMuzzleFlash) {
                                    FIGURE_draw_prim_tween_person_only(
                                        prim,
                                        FIGURE_dhpr_data.world_mat,
                                        FIGURE_dhpr_data.dx + dx,
                                        FIGURE_dhpr_data.dy + dy,
                                        FIGURE_dhpr_data.dz + dz,
                                        recurse_level,
                                        p_person);
                                }
                            }

                            FIGURE_draw_prim_tween_person_only(
                                255 + (p_person->Draw.Tweened->PersonID >> 5),
                                FIGURE_dhpr_data.world_mat,
                                FIGURE_dhpr_data.dx + dx,
                                FIGURE_dhpr_data.dy + dy,
                                FIGURE_dhpr_data.dz + dz,
                                recurse_level,
                                p_person);
                        }
                    }
            }
        }

        if (body_part_children[iPartNumber][pDHPR1->current_child_number] != -1) {

            structFIGURE_dhpr_rdata1* pDHPR1Inc = FIGURE_dhpr_rdata1 + recurse_level + 1;

            pDHPR1Inc->current_child_number = 0;

            pDHPR1->pos.M[0] = FIGURE_dhpr_data.ae1[iPartNumber].OffsetX;
            pDHPR1->pos.M[1] = FIGURE_dhpr_data.ae1[iPartNumber].OffsetY;
            pDHPR1->pos.M[2] = FIGURE_dhpr_data.ae1[iPartNumber].OffsetZ;

            ASSERT(iPartNumber >= 0);
            ASSERT(iPartNumber <= 14);
            pDHPR1Inc->part_number = body_part_children[iPartNumber][pDHPR1->current_child_number];
            // Hierarchy parent-matrix propagation removed: the pose snapshot
            // is the single source of each bone's world transform, so the
            // leaf draw functions no longer read parent_*/end_* — see
            // skeletal_skinning_plan.md.
            pDHPR1Inc->current_child_number = 0;

            pDHPR1->current_child_number++;
            recurse_level++;
        } else {
            recurse_level--;
        }
    };

    if (!bWholePersonVisible) {
        if (bBitsOfPersonVisible) {
            FIGURE_dhpr_rdata1[0] = FIGURE_dhpr_rdata1_0_copy;
            FIGURE_dhpr_data_copy = FIGURE_dhpr_data_copy;
            FIGURE_draw_hierarchical_prim_recurse_individual_cull(p_person);
        }
        return;
    }

    iTPOPartNumber++;
    ASSERT(iTPOPartNumber <= MAX_NUM_BODY_PARTS_AT_ONCE);

    SLONG tex_page_offset = p_person->Genus.Person->pcom_colour & 0x3;

    ASSERT(MM_bLightTableAlreadySetUp);

    ge_set_specular_enabled(false);

    FIGURE_touch_LRU_of_object(pPrimObj);

    ASSERT(pPrimObj->pD3DVertices != NULL);
    ASSERT(pPrimObj->pMaterials != NULL);
    ASSERT(pPrimObj->pwListIndices != NULL);
    ASSERT(pPrimObj->pwStripIndices != NULL);
    ASSERT(pPrimObj->wNumMaterials != 0);

    PrimObjectMaterial* pMat = pPrimObj->pMaterials;

    // Phase 2 P2-A (skeletal_skinning_phase2_plan.md): build one
    // consolidated skin mesh for the whole person on first use, then
    // each material is one range-slice of the shared VBO. Falls back
    // to per-material ge_draw_multi_matrix path if the build fails
    // (alloc, > 65535 verts; very unlikely for a person).
    bool consolidated = (pPrimObj->skin_consolidated != NULL)
        || figure_build_consolidated_skin(pPrimObj);
    GESkinMesh* skinMesh = (GESkinMesh*)pPrimObj->skin_consolidated;
    uint32_t* skinRanges = pPrimObj->skin_consolidated_ranges;
    if (!skinMesh || !skinRanges) consolidated = false;

    // Phase 2 P2-C (skeletal_skinning_phase2_plan.md): when the world-skin
    // toggle is on and we have a valid bind palette + a built bind-space
    // mesh, draw via the world-skin shader instead. Per-frame skin
    // palette (= current * inv_bind) + per-character screen bake are
    // computed once below and reused by every material's draw range.
    bool use_world_path = false;
    GESkinMesh* worldMesh = NULL;
    // 15 bones × 3 vec4 each = 180 floats. Bumping to GE_SKIN_MAX_BONES
    // would be wasteful (we only use the first 15) but harmless if needed
    // for non-person rigs later — for now stay tight.
    float skin_palette_world[POSE_PERSON_BONE_COUNT * 12];
    GEMatrix screen_xform_world = {};
    float lightdir_world[3] = {0.0f, 0.0f, 0.0f};
    if (consolidated && g_skin_world_path_enabled
        && p_person->Genus.Person
        && p_person->Draw.Tweened
        && p_person->Draw.Tweened->TheChunk
        && p_person->Draw.Tweened->TheChunk->ElementCount == POSE_PERSON_BONE_COUNT) {

        const int anim_type = p_person->Genus.Person->AnimType;
        const GEMatrix* bind_inv = NULL;
        if (bind_palette_get(anim_type, NULL, &bind_inv) && bind_inv) {
            // Lazy build of the bind-space mesh on first use of this rig.
            if (pPrimObj->skin_consolidated_world == NULL) {
                figure_build_consolidated_skin_world(pPrimObj, anim_type);
            }
            worldMesh = (GESkinMesh*)pPrimObj->skin_consolidated_world;
            const BoneInterpTransform* current =
                worldMesh ? render_interp_get_cached_pose(p_person) : NULL;
            if (current) {
                figure_build_skin_world_palette(current, bind_inv, skin_palette_world);
                figure_build_screen_xform_bake(&screen_xform_world);
                lightdir_world[0] = MM_vLightDir.x;
                lightdir_world[1] = MM_vLightDir.y;
                lightdir_world[2] = MM_vLightDir.z;
                use_world_path = true;
            }
        }
    }

    GEMultiMatrix mm;
    mm.matrices = MMBodyParts_pMatrix;
    mm.lpvLightDirs = MMBodyParts_pNormal;
    mm.skin_cache = nullptr; // 1D: set per-material before each draw (fallback only)

    GEVertex* pVertex = (GEVertex*)pPrimObj->pD3DVertices;
    UWORD* pwStripIndices = pPrimObj->pwStripIndices;
    int iMatIndex = 0;
    for (int iMatNum = pPrimObj->wNumMaterials; iMatNum > 0; iMatNum--, iMatIndex++) {

        UWORD wPage = pMat->wTexturePage;
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

        extern GEMatrix g_matWorld;

        PolyPage* pa = &(POLY_Page[wRealPage]);
        {
            // Explicit cast: MM_pcFadeTable is ULONG* (= unsigned long*).
            // ULONG is 32-bit on Win but 64-bit on Linux/Mac — same bit
            // pattern as uint32_t on our targets but the pointer types
            // differ. Cast keeps things portable.
            const uint32_t* fadeTable = (const uint32_t*)((wPage & TEXTURE_PAGE_FLAG_TINT)
                ? MM_pcFadeTableTint : MM_pcFadeTable);

            pa->RS.SetCullMode(GECullMode::CCW);
            pa->RS.SetAlphaBlendEnabled(false);
            pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
            pa->RS.SetChanged();

            // Set view-space Z for CPU fog (same scalar both paths).
            g_mm_fog_view_z = g_matWorld._43;

            if (consolidated) {
                // P2-A consolidated path: one shared VBO, this material
                // is one slice [start, count) of the index buffer.
                const uint32_t index_start = skinRanges[iMatIndex * 2 + 0];
                const uint32_t index_count = skinRanges[iMatIndex * 2 + 1];
                if (index_count > 0) {
                    if (use_world_path) {
                        // P2-C world path: bind-space verts + per-frame
                        // skin palette + per-character screen bake.
                        ge_skin_world_draw_range(
                            worldMesh, index_start, index_count,
                            skin_palette_world,
                            POSE_PERSON_BONE_COUNT,
                            &screen_xform_world,
                            lightdir_world,
                            g_mm_fog_view_z,
                            /*unlit=*/ true,
                            fadeTable);
                    } else {
                        ge_skin_mesh_draw_range(
                            skinMesh, index_start, index_count,
                            MMBodyParts_pMatrix,
                            /*unlit=*/ true,
                            (const float*)MMBodyParts_pNormal,
                            fadeTable);
                    }
                }
            } else {
                // Fallback: original per-material accumulation path.
                mm.lpLightTable = (ULONG*)fadeTable;
                mm.lpvVertices = pVertex;
                mm.skin_cache = figure_skin_cache_slot(pPrimObj, pMat);

                ge_draw_multi_matrix(
                    GEMMVertexType::Unlit,
                    &mm,
                    pMat->wNumVertices,
                    pwStripIndices,
                    pMat->wNumStripIndices);
            }
        }

        pVertex += pMat->wNumVertices;
        pwStripIndices += pMat->wNumStripIndices;

        pMat++;
    }

    ge_set_specular_enabled(true);

    ASSERT(p_person && (p_person->Class != CLASS_VEHICLE));

    ASSERT(MM_bLightTableAlreadySetUp);
}

// uc_orig: FIGURE_draw_hierarchical_prim_recurse_individual_cull (fallen/DDEngine/Source/figure.cpp)
// Slower fallback that culls individual body parts separately (when the fast-path full-person
// visibility test fails). Calls FIGURE_draw_prim_tween_person_only per body part.
void FIGURE_draw_hierarchical_prim_recurse_individual_cull(Thing* p_person)
{
    SLONG recurse_level = 0;
    SLONG dx, dy, dz;
    UWORD f1, f2;
    struct Matrix33* rot_mat;

    f1 = p_person->Draw.Tweened->CurrentFrame->Flags;
    f2 = p_person->Draw.Tweened->NextFrame->Flags;

    dx = ((f1 & 0xe) << (28)) >> 21;
    dx -= ((f2 & 0xe) << (28)) >> 21;

    dy = ((f1 & 0xff00) << (16)) >> 16;
    dy -= ((f2 & 0xff00) << (16)) >> 16;

    dz = ((f1 & 0x70) << (25)) >> 21;
    dz -= ((f1 & 0x70) << (25)) >> 21;

    ASSERT(p_person->Class == CLASS_PERSON);

    recurse_level = 0;
    while (recurse_level >= 0) {

        structFIGURE_dhpr_rdata1* pDHPR1 = FIGURE_dhpr_rdata1 + recurse_level;
        int iPartNumber = pDHPR1->part_number;

        if (pDHPR1->current_child_number == 0) {
            {
                SLONG body_part;
                SLONG id;

                ASSERT(iPartNumber >= 0);
                ASSERT(iPartNumber <= 14);

                body_part = FIGURE_dhpr_data.body_def->BodyPart[iPartNumber];
                rot_mat = FIGURE_dhpr_data.world_mat;

                FIGURE_draw_prim_tween_person_only(
                    FIGURE_dhpr_data.start_object + body_part,
                    rot_mat,
                    FIGURE_dhpr_data.dx + dx,
                    FIGURE_dhpr_data.dy + dy,
                    FIGURE_dhpr_data.dz + dz,
                    recurse_level,
                    p_person);

                if (p_person->Genus.Person->PersonType != PERSON_ROPER)
                    if ((id = (p_person->Draw.Tweened->PersonID >> 5))) {

                        SLONG hand;
                        if (id == 2)
                            hand = SUB_OBJECT_RIGHT_HAND;
                        else
                            hand = SUB_OBJECT_LEFT_HAND;

                        if (iPartNumber == hand) {
                            if (p_person->Draw.Tweened->Flags & DT_FLAG_GUNFLASH) {
                                SLONG prim;
                                bool bDrawMuzzleFlash = UC_FALSE;
                                p_person->Draw.Tweened->Flags &= ~DT_FLAG_GUNFLASH;
                                switch (p_person->Draw.Tweened->PersonID >> 5) {
                                case 1:
                                    prim = 261;
                                    bDrawMuzzleFlash = UC_TRUE;
                                    break;

                                case 3:
                                    prim = 262;
                                    bDrawMuzzleFlash = UC_TRUE;
                                    break;

                                case 5:
                                    prim = 263;
                                    bDrawMuzzleFlash = UC_TRUE;
                                    break;

                                default:
                                    break;
                                }

                                if (bDrawMuzzleFlash) {
                                    FIGURE_draw_prim_tween_person_only(
                                        prim,
                                        FIGURE_dhpr_data.world_mat,
                                        FIGURE_dhpr_data.dx + dx,
                                        FIGURE_dhpr_data.dy + dy,
                                        FIGURE_dhpr_data.dz + dz,
                                        recurse_level,
                                        p_person);
                                }
                            }

                            FIGURE_draw_prim_tween_person_only(
                                255 + (p_person->Draw.Tweened->PersonID >> 5),
                                FIGURE_dhpr_data.world_mat,
                                FIGURE_dhpr_data.dx + dx,
                                FIGURE_dhpr_data.dy + dy,
                                FIGURE_dhpr_data.dz + dz,
                                recurse_level,
                                p_person);
                        }
                    }
            }
        }

        if (body_part_children[iPartNumber][pDHPR1->current_child_number] != -1) {

            structFIGURE_dhpr_rdata1* pDHPR1Inc = FIGURE_dhpr_rdata1 + recurse_level + 1;

            pDHPR1Inc->current_child_number = 0;

            pDHPR1->pos.M[0] = FIGURE_dhpr_data.ae1[iPartNumber].OffsetX;
            pDHPR1->pos.M[1] = FIGURE_dhpr_data.ae1[iPartNumber].OffsetY;
            pDHPR1->pos.M[2] = FIGURE_dhpr_data.ae1[iPartNumber].OffsetZ;

            ASSERT(iPartNumber >= 0);
            ASSERT(iPartNumber <= 14);
            pDHPR1Inc->part_number = body_part_children[iPartNumber][pDHPR1->current_child_number];
            // Hierarchy parent-matrix propagation removed: the pose snapshot
            // is the single source of each bone's world transform, so the
            // leaf draw functions no longer read parent_*/end_* — see
            // skeletal_skinning_plan.md.
            pDHPR1Inc->current_child_number = 0;

            pDHPR1->current_child_number++;
            recurse_level++;
        } else {
            recurse_level--;
        }
    };
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

    if (ele_count == 15) {
        Matrix31 pos;
        pos.M[0] = (p_thing->WorldPos.X >> 8) + wx;
        pos.M[1] = (p_thing->WorldPos.Y >> 8) + wy;
        pos.M[2] = (p_thing->WorldPos.Z >> 8) + wz;

        FIGURE_dhpr_rdata1[0].part_number = 0;
        FIGURE_dhpr_rdata1[0].current_child_number = 0;
        // parent_*/end_* fields no longer used — pose snapshot is the single
        // source of each bone's world transform (skeletal_skinning_plan.md).

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
    } else {
        for (int i = 0; i < ele_count; i++) {
            object_offset = dt->TheChunk->PeopleTypes[(dt->PersonID & 0x1f)].BodyPart[i];

            FIGURE_draw_prim_tween(
                start_object + object_offset,
                colour,
                specular,
                p_thing,
                i);
        }
    }

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

// uc_orig: ANIM_obj_draw (fallen/DDEngine/Source/figure.cpp)
// Draws an animated non-character object using the DrawTween keyframe system.
// Uses flat body-part loop (no hierarchical dispatch). Calls NIGHT_find() for lighting.
void ANIM_obj_draw(Thing* p_thing, DrawTween* dt)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    ULONG colour;
    ULONG specular;

    Matrix33 r_matrix;

    GameKeyFrameElement* ae1;
    GameKeyFrameElement* ae2;

    if (dt->CurrentFrame == 0 || dt->NextFrame == 0) {
        MSG_add("!!!!!!!!!!!!!!!!!!!!!!!!ERROR AENG_draw_figure");
        return;
    }

    dx = 0;
    dy = 0;
    dz = 0;

    ae1 = dt->CurrentFrame->FirstElement;
    ae2 = dt->NextFrame->FirstElement;

    if (!ae1 || !ae2) {
        MSG_add("!!!!!!!!!!!!!!!!!!!ERROR AENG_draw_figure has no animation elements");

        return;
    }

    FIGURE_rotate_obj(
        dt->Tilt,
        dt->Angle,
        (dt->Roll + 2048) & 2047,
        &r_matrix);

    colour = 0;
    specular = 0;

    SLONG lx;
    SLONG ly;
    SLONG lz;

    lx = (p_thing->WorldPos.X >> 8);
    ly = (p_thing->WorldPos.Y >> 8) + 0x60;
    lz = (p_thing->WorldPos.Z >> 8);

    NIGHT_find(lx, ly, lz);

    SLONG i;
    SLONG ele_count;
    SLONG start_object;
    SLONG object_offset;

    ele_count = dt->TheChunk->ElementCount;
    start_object = prim_multi_objects[dt->TheChunk->MultiObject[0]].StartObject;

    for (i = 0; i < ele_count; i++) {
        object_offset = i;
        FIGURE_draw_prim_tween(
            start_object + object_offset,
            colour,
            specular,
            p_thing,
            i,
            0xffff00ff);
    }
}

// uc_orig: FIGURE_MAX_DY (fallen/DDEngine/Source/figure.cpp)
#define FIGURE_MAX_DY (128.0F)
// uc_orig: FIGURE_255_DIVIDED_BY_MAX_DY (fallen/DDEngine/Source/figure.cpp)
#define FIGURE_255_DIVIDED_BY_MAX_DY (255.0F / FIGURE_MAX_DY)

// uc_orig: FIGURE_draw_prim_tween_reflection (fallen/DDEngine/Source/figure.cpp)
// Renders one body-part mesh into the FIGURE_rpoint[] buffer for the water reflection effect.
// Identical tween/slerp pipeline to FIGURE_draw_prim_tween, but projects vertices to screen space,
// mirrors them about FIGURE_reflect_height, fades by distance, and stores in FIGURE_rpoint[].
// Updates the FIGURE_reflect_x1/y1/x2/y2 bounding box for each valid reflected point.
void FIGURE_draw_prim_tween_reflection(
    SLONG prim,
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG tween,
    struct GameKeyFrameElement* anim_info,
    struct GameKeyFrameElement* anim_info_next,
    struct Matrix33* rot_mat,
    SLONG off_dx,
    SLONG off_dy,
    SLONG off_dz,
    ULONG colour,
    ULONG specular,
    Thing* p_thing)
{
    SLONG i;

    SLONG sp;
    SLONG ep;

    SLONG p0;
    SLONG p1;
    SLONG p2;
    SLONG p3;

    SLONG px;
    SLONG py;

    SLONG fog;

    float world_x;
    float world_y;
    float world_z;

    SLONG page;

    Matrix33 mat_final;

    PrimFace4* p_f4;
    PrimFace3* p_f3;
    PrimObject* p_obj;

    POLY_Point* tri[3];
    POLY_Point* quad[4];

    FIGURE_Rpoint* frp;

    colour >>= 1;
    specular >>= 1;

    colour &= 0x7f7f7f7f;
    specular &= 0x7f7f7f7f;

    colour |= 0xff000000;
    specular |= 0xff000000;

    // === Reflection: pose-snapshot, mirrored about the water plane ===
    // The pose snapshot is the SINGLE always-available source of pose (no
    // legacy keyframe recompute). Snapshot is in normal (non-mirrored)
    // world space; apply geometric reflection through the water plane y = H:
    //   - position: y_reflected = 2 * FIGURE_reflect_height - y_world
    //   - rotation: left-multiply by diag(1,-1,1) → negate the Y ROW
    //     (M[1][*]). (Algebraically equivalent to the legacy "negate Y
    //     column of R_body before composing mat_final = R_body × end_mat" —
    //     verified numerically. Negating the Y column of the per-bone WORLD
    //     mat_final instead does NOT give geometric reflection — see
    //     skeletal_skinning_plan.md.)
    float off_x;
    float off_y;
    float off_z;
    float fmatrix[9];

    const BoneInterpTransform* pose = render_interp_get_cached_pose(p_thing);
    DrawTween* dt = p_thing->Draw.Tweened;
    if (!pose || !dt || !dt->CurrentFrame || !dt->CurrentFrame->FirstElement)
        return;
    SLONG part = SLONG(anim_info - dt->CurrentFrame->FirstElement);
    if (part < 0 || part >= POSE_MAX_BONES)
        return;

    const BoneInterpTransform& xf = pose[part];
    off_x = xf.pos_x;
    off_y = 2.0f * FIGURE_reflect_height - xf.pos_y;
    off_z = xf.pos_z;
    mat_final = xf.rot;
    mat_final.M[1][0] = -mat_final.M[1][0];
    mat_final.M[1][1] = -mat_final.M[1][1];
    mat_final.M[1][2] = -mat_final.M[1][2];
    fmatrix[0] = float(mat_final.M[0][0]) * (1.0F / 32768.0F);
    fmatrix[1] = float(mat_final.M[0][1]) * (1.0F / 32768.0F);
    fmatrix[2] = float(mat_final.M[0][2]) * (1.0F / 32768.0F);
    fmatrix[3] = float(mat_final.M[1][0]) * (1.0F / 32768.0F);
    fmatrix[4] = float(mat_final.M[1][1]) * (1.0F / 32768.0F);
    fmatrix[5] = float(mat_final.M[1][2]) * (1.0F / 32768.0F);
    fmatrix[6] = float(mat_final.M[2][0]) * (1.0F / 32768.0F);
    fmatrix[7] = float(mat_final.M[2][1]) * (1.0F / 32768.0F);
    fmatrix[8] = float(mat_final.M[2][2]) * (1.0F / 32768.0F);

    POLY_set_local_rotation(
        off_x,
        off_y,
        off_z,
        fmatrix);

    p_obj = &prim_objects[prim];

    sp = p_obj->StartPoint;
    ep = p_obj->EndPoint;

    FIGURE_rpoint_upto = 0;

    for (i = sp; i < ep; i++) {
        ASSERT(WITHIN(FIGURE_rpoint_upto, 0, FIGURE_MAX_RPOINTS - 1));

        frp = &FIGURE_rpoint[FIGURE_rpoint_upto++];

        POLY_transform_using_local_rotation(
            AENG_dx_prim_points[i].X,
            AENG_dx_prim_points[i].Y,
            AENG_dx_prim_points[i].Z,
            &frp->pp);

        if (frp->pp.MaybeValid()) {
            frp->pp.colour = colour;
            frp->pp.specular = specular;

            world_x = AENG_dx_prim_points[i].X;
            world_y = AENG_dx_prim_points[i].Y;
            world_z = AENG_dx_prim_points[i].Z;

            MATRIX_MUL(
                fmatrix,
                world_x,
                world_y,
                world_z);

            world_x += off_x;
            world_y += off_y;
            world_z += off_z;

            frp->distance = FIGURE_reflect_height - world_y;

            if (frp->distance >= 0.0F) {
                px = SLONG(frp->pp.X);
                py = SLONG(frp->pp.Y);

                if (px < FIGURE_reflect_x1) {
                    FIGURE_reflect_x1 = px;
                }
                if (px > FIGURE_reflect_x2) {
                    FIGURE_reflect_x2 = px;
                }
                if (py < FIGURE_reflect_y1) {
                    FIGURE_reflect_y1 = py;
                }
                if (py > FIGURE_reflect_y2) {
                    FIGURE_reflect_y2 = py;
                }

                fog = frp->pp.specular >> 24;
                fog += SLONG(frp->distance * FIGURE_255_DIVIDED_BY_MAX_DY);

                if (fog > 255) {
                    fog = 255;
                }

                frp->pp.specular &= 0x00ffffff;
                frp->pp.specular |= fog << 24;
            } else {
                frp->pp.clip = 0;
            }
        }
    }

    for (i = p_obj->StartFace4; i < p_obj->EndFace4; i++) {
        p_f4 = &prim_faces4[i];

        p0 = p_f4->Points[0] - sp;
        p1 = p_f4->Points[1] - sp;
        p2 = p_f4->Points[2] - sp;
        p3 = p_f4->Points[3] - sp;

        ASSERT(WITHIN(p0, 0, FIGURE_rpoint_upto - 1));
        ASSERT(WITHIN(p1, 0, FIGURE_rpoint_upto - 1));
        ASSERT(WITHIN(p2, 0, FIGURE_rpoint_upto - 1));
        ASSERT(WITHIN(p3, 0, FIGURE_rpoint_upto - 1));

        // Add the quads in reverse winding for the mirror effect.
        quad[0] = &FIGURE_rpoint[p0].pp;
        quad[2] = &FIGURE_rpoint[p1].pp;
        quad[1] = &FIGURE_rpoint[p2].pp;
        quad[3] = &FIGURE_rpoint[p3].pp;

        if (POLY_valid_quad(quad)) {
            if (p_f4->DrawFlags & POLY_FLAG_TEXTURED) {
                quad[0]->u = float(p_f4->UV[0][0] & 0x3f) * (1.0F / 32.0F);
                quad[0]->v = float(p_f4->UV[0][1]) * (1.0F / 32.0F);

                quad[1]->u = float(p_f4->UV[1][0]) * (1.0F / 32.0F);
                quad[1]->v = float(p_f4->UV[1][1]) * (1.0F / 32.0F);

                quad[2]->u = float(p_f4->UV[2][0]) * (1.0F / 32.0F);
                quad[2]->v = float(p_f4->UV[2][1]) * (1.0F / 32.0F);

                quad[3]->u = float(p_f4->UV[3][0]) * (1.0F / 32.0F);
                quad[3]->v = float(p_f4->UV[3][1]) * (1.0F / 32.0F);

                page = p_f4->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f4->TexturePage;
                page += FACE_PAGE_OFFSET;

                if (ge_supports_adami_lighting()) {
                    POLY_add_quad(quad, POLY_PAGE_COLOUR, UC_TRUE);
                }
                POLY_add_quad(quad, page, UC_TRUE);
            } else {
            }
        }
    }

    for (i = p_obj->StartFace3; i < p_obj->EndFace3; i++) {
        p_f3 = &prim_faces3[i];

        p0 = p_f3->Points[0] - sp;
        p1 = p_f3->Points[1] - sp;
        p2 = p_f3->Points[2] - sp;

        ASSERT(WITHIN(p0, 0, FIGURE_rpoint_upto - 1));
        ASSERT(WITHIN(p1, 0, FIGURE_rpoint_upto - 1));
        ASSERT(WITHIN(p2, 0, FIGURE_rpoint_upto - 1));

        // Add the triangles in reverse winding for the mirror effect.
        tri[0] = &FIGURE_rpoint[p0].pp;
        tri[2] = &FIGURE_rpoint[p1].pp;
        tri[1] = &FIGURE_rpoint[p2].pp;

        if (POLY_valid_triangle(tri)) {
            if (p_f3->DrawFlags & POLY_FLAG_TEXTURED) {
                tri[0]->u = float(p_f3->UV[0][0] & 0x3f) * (1.0F / 32.0F);
                tri[0]->v = float(p_f3->UV[0][1]) * (1.0F / 32.0F);

                tri[1]->u = float(p_f3->UV[1][0]) * (1.0F / 32.0F);
                tri[1]->v = float(p_f3->UV[1][1]) * (1.0F / 32.0F);

                tri[2]->u = float(p_f3->UV[2][0]) * (1.0F / 32.0F);
                tri[2]->v = float(p_f3->UV[2][1]) * (1.0F / 32.0F);

                page = p_f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f3->TexturePage;
                page += FACE_PAGE_OFFSET;

                if (ge_supports_adami_lighting()) {
                    POLY_add_triangle(tri, POLY_PAGE_COLOUR, UC_TRUE);
                }
                POLY_add_triangle(tri, page, UC_TRUE);
            } else {
            }
        }
    }
}

// uc_orig: FIGURE_draw_reflection (fallen/DDEngine/Source/figure.cpp)
// Top-level water reflection renderer for one character.
// Iterates all body parts (same loop as FIGURE_draw) but calls FIGURE_draw_prim_tween_reflection
// instead of the normal draw function to accumulate reflected screen-space points.
// height: world-space Y coordinate of the water surface to reflect about.
void FIGURE_draw_reflection(Thing* p_thing, SLONG height)
{
    SLONG dx;
    SLONG dy;
    SLONG dz;

    ULONG colour;
    ULONG specular;

    Matrix33 r_matrix;

    GameKeyFrameElement* ae1;
    GameKeyFrameElement* ae2;

    DrawTween* dt = p_thing->Draw.Tweened;

    if (dt->CurrentFrame == 0 || dt->NextFrame == 0) {
        MSG_add("!!!!!!!!!!!!!!!!!!!!!!!!ERROR FIGURE_draw_reflection");
        return;
    }

    dx = 0;
    dy = 0;
    dz = 0;

    ae1 = dt->CurrentFrame->FirstElement;
    ae2 = dt->NextFrame->FirstElement;

    if (!ae1 || !ae2) {
        MSG_add("!!!!!!!!!!!!!!!!!!!ERROR AENG_draw_figure has no animation elements");
        return;
    }

    FIGURE_rotate_obj(
        dt->Tilt,
        dt->Angle,
        -dt->Roll, // - = JCL
        &r_matrix);

    SLONG posx = p_thing->WorldPos.X >> 8;
    SLONG posy = p_thing->WorldPos.Y >> 8;
    SLONG posz = p_thing->WorldPos.Z >> 8;

    // Reflect about y = height.
    posy = height - (posy - height);
    dy = -dy;

    r_matrix.M[0][1] = -r_matrix.M[0][1];
    r_matrix.M[1][1] = -r_matrix.M[1][1];
    r_matrix.M[2][1] = -r_matrix.M[2][1];

    FIGURE_reflect_x1 = +UC_INFINITY;
    FIGURE_reflect_y1 = +UC_INFINITY;
    FIGURE_reflect_x2 = -UC_INFINITY;
    FIGURE_reflect_y2 = -UC_INFINITY;

    FIGURE_reflect_height = float(height);

    SLONG lx;
    SLONG ly;
    SLONG lz;

    NIGHT_Colour col;

    {
        // Pelvis world position for reflection lighting lookup, from the
        // interpolated pose snapshot — the single always-available source.
        // Sampling the light at the same lerped pelvis the reflection
        // geometry uses keeps the colour matched (no legacy recompute path).
        const BoneInterpTransform* pose = render_interp_get_cached_pose(p_thing);
        lx = SLONG(pose[0].pos_x);
        ly = SLONG(pose[0].pos_y);
        lz = SLONG(pose[0].pos_z);
    }

    col = NIGHT_get_light_at(lx, ly, lz);

    if (!ControlFlag) {
        // Brighten up the person if he is going to be drawn too dark.
        if (col.red < 32) {
            col.red += 32 - col.red >> 1;
        }
        if (col.green < 32) {
            col.green += 32 - col.green >> 1;
        }
        if (col.blue < 32) {
            col.blue += 32 - col.blue >> 1;
        }
    }

    NIGHT_get_colour(
        col,
        &colour,
        &specular);

    colour &= ~POLY_colour_restrict;
    specular &= ~POLY_colour_restrict;

    SLONG i;
    SLONG ele_count;
    SLONG start_object;
    SLONG object_offset;

    ele_count = dt->TheChunk->ElementCount;
    start_object = prim_multi_objects[dt->TheChunk->MultiObject[dt->MeshID]].StartObject;

    for (i = 0; i < ele_count; i++) {
        object_offset = dt->TheChunk->PeopleTypes[dt->PersonID & 0x1f].BodyPart[i];

        FIGURE_draw_prim_tween_reflection(
            start_object + object_offset,
            posx,
            posy,
            posz,
            dt->AnimTween,
            &ae1[i],
            &ae2[i],
            &r_matrix,
            dx, dy, dz,
            colour,
            specular,
            p_thing);
    }
}

// uc_orig: FIGURE_draw_prim_tween_person_only_just_set_matrix (fallen/DDEngine/Source/figure.cpp)
// "Matrix-only" body-part step for the D3D MultiMatrix fast path.
// Computes the interpolated bone transform and stores it in MMBodyParts_pMatrix[iMatrixNum].
// Also computes the per-bone light direction vector for MMBodyParts_pNormal.
// Does NOT emit geometry — that comes in a single DrawIndPrimMM call from the caller.
// Returns UC_FALSE if this body part is entirely behind the near-Z clip plane.
bool FIGURE_draw_prim_tween_person_only_just_set_matrix(
    int iMatrixNum,
    SLONG prim,
    struct Matrix33* rot_mat,
    SLONG off_dx,
    SLONG off_dy,
    SLONG off_dz,
    SLONG recurse_level,
    Thing* p_thing)
{
    SLONG i;

    SLONG sp;

    Matrix33 mat_final;

    PrimObject* p_obj;

    POLY_Point* pp;

    // Per-body-part WORLD transform read straight from the interpolation
    // pose snapshot — the SINGLE always-available source of pose. No legacy
    // keyframe / hierarchy recompute: the snapshot already bakes body
    // rotation, character scale and the inter-bone hierarchy, so each bone
    // is read directly by its part index. (The old end_mat/end_pos hierarchy
    // propagation only ever fed HIERARCHY_Get_Body_Part_Offset, whose result
    // was always overwritten by this snapshot — see skeletal_skinning_plan.md.)
    const BoneInterpTransform* pose = render_interp_get_cached_pose(p_thing);
    SLONG part = FIGURE_dhpr_rdata1[recurse_level].part_number;
    if (!pose || part < 0 || part >= POSE_MAX_BONES)
        return UC_FALSE;

    const BoneInterpTransform& xf = pose[part];
    float off_x = xf.pos_x;
    float off_y = xf.pos_y;
    float off_z = xf.pos_z;
    mat_final = xf.rot;

    float fmatrix[9];
    fmatrix[0] = float(mat_final.M[0][0]) * (1.0F / 32768.0F);
    fmatrix[1] = float(mat_final.M[0][1]) * (1.0F / 32768.0F);
    fmatrix[2] = float(mat_final.M[0][2]) * (1.0F / 32768.0F);
    fmatrix[3] = float(mat_final.M[1][0]) * (1.0F / 32768.0F);
    fmatrix[4] = float(mat_final.M[1][1]) * (1.0F / 32768.0F);
    fmatrix[5] = float(mat_final.M[1][2]) * (1.0F / 32768.0F);
    fmatrix[6] = float(mat_final.M[2][0]) * (1.0F / 32768.0F);
    fmatrix[7] = float(mat_final.M[2][1]) * (1.0F / 32768.0F);
    fmatrix[8] = float(mat_final.M[2][2]) * (1.0F / 32768.0F);

    // NOT portable: stores off/fmatrix into global transform state used by D3D MultiMatrix upload.
    POLY_set_local_rotation(
        off_x,
        off_y,
        off_z,
        fmatrix);

    // Near-Z bounding sphere cull per body part removed — frustum cull in
    // POLY_sphere_visible handles visibility, z guard in ge_draw_multi_matrix
    // prevents artifacts for near vertices.

    p_obj = &prim_objects[prim];

    sp = p_obj->StartPoint;

    POLY_buffer_upto = 0;

    // Check for being a gun
    if (prim == 256) {
        i = sp;
    } else if (prim == 258) {
        // Or a shotgun
        i = sp + 15;
    } else if (prim == 260) {
        // or an AK
        i = sp + 32;
    } else {
        goto no_muzzle_calcs; // which skips...
    }

    // this bit, which only executes if one of the above tests is true.
    pp = &POLY_buffer[POLY_buffer_upto]; // no ++, so reused
    pp->x = AENG_dx_prim_points[i].X;
    pp->y = AENG_dx_prim_points[i].Y;
    pp->z = AENG_dx_prim_points[i].Z;
    MATRIX_MUL(
        fmatrix,
        pp->x,
        pp->y,
        pp->z);

    pp->x += off_x;
    pp->y += off_y;
    pp->z += off_z;
    p_thing->Genus.Person->GunMuzzle.X = pp->x * 256;
    p_thing->Genus.Person->GunMuzzle.Y = pp->y * 256;
    p_thing->Genus.Person->GunMuzzle.Z = pp->z * 256;

no_muzzle_calcs:

    ASSERT(MM_bLightTableAlreadySetUp);

    ASSERT(!WITHIN(prim, 261, 263));

    {
        ASSERT(MM_bLightTableAlreadySetUp);

        extern float POLY_cam_matrix_comb[9];
        extern float POLY_cam_off_x;
        extern float POLY_cam_off_y;
        extern float POLY_cam_off_z;

        extern GEMatrix g_matProjection;
        extern GEMatrix g_matWorld;
        extern GEViewport g_viewData;

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

        extern DWORD g_dw3DStuffHeight;
        extern DWORD g_dw3DStuffY;
        DWORD dwWidth = g_viewData.dwWidth >> 1;
        DWORD dwHeight = g_dw3DStuffHeight >> 1;
        DWORD dwX = g_viewData.dwX;
        DWORD dwY = g_dw3DStuffY;

        // Set up the bone transform matrix in screen-space projection form.
        GEMatrix* pmat = &(MMBodyParts_pMatrix[iMatrixNum]);
        pmat->_11 = 0.0f;
        pmat->_12 = matTemp._11 * (float)dwWidth + matTemp._14 * (float)(dwX + dwWidth);
        pmat->_13 = matTemp._12 * -(float)dwHeight + matTemp._14 * (float)(dwY + dwHeight);
        pmat->_14 = matTemp._14;
        pmat->_21 = 0.0f;
        pmat->_22 = matTemp._21 * (float)dwWidth + matTemp._24 * (float)(dwX + dwWidth);
        pmat->_23 = matTemp._22 * -(float)dwHeight + matTemp._24 * (float)(dwY + dwHeight);
        pmat->_24 = matTemp._24;
        pmat->_31 = 0.0f;
        pmat->_32 = matTemp._31 * (float)dwWidth + matTemp._34 * (float)(dwX + dwWidth);
        pmat->_33 = matTemp._32 * -(float)dwHeight + matTemp._34 * (float)(dwY + dwHeight);
        pmat->_34 = matTemp._34;
        // Validation magic number.
        ULONG EVal = 0xe0001000;
        pmat->_41 = *(float*)&EVal;
        pmat->_42 = matTemp._41 * (float)dwWidth + matTemp._44 * (float)(dwX + dwWidth);
        pmat->_43 = matTemp._42 * -(float)dwHeight + matTemp._44 * (float)(dwY + dwHeight);
        pmat->_44 = matTemp._44;

        // 251 is a magic number for the DIP call.
        const float fNormScale = 251.0f;

        // Transform light direction by inverse (=transpose) object matrix to get object-space light.
        GEVector vTemp;
        vTemp.x = MM_vLightDir.x * fmatrix[0] + MM_vLightDir.y * fmatrix[3] + MM_vLightDir.z * fmatrix[6];
        vTemp.y = MM_vLightDir.x * fmatrix[1] + MM_vLightDir.y * fmatrix[4] + MM_vLightDir.z * fmatrix[7];
        vTemp.z = MM_vLightDir.x * fmatrix[2] + MM_vLightDir.y * fmatrix[5] + MM_vLightDir.z * fmatrix[8];

        float* pnorm = &(MMBodyParts_pNormal[iMatrixNum << 2]);
        pnorm[0] = 0.0f;
        pnorm[1] = vTemp.x * fNormScale;
        pnorm[2] = vTemp.y * fNormScale;
        pnorm[3] = vTemp.z * fNormScale;
    }

    // No environment mapping.
    ASSERT(p_thing && (p_thing->Class != CLASS_VEHICLE));

    ASSERT(MM_bLightTableAlreadySetUp);

    return UC_TRUE;
}

// uc_orig: FIGURE_draw_prim_tween_person_only (fallen/DDEngine/Source/figure.cpp)
// Full software-renderer fallback for one body part in person-only mode (D3D MultiMatrix not available).
// Unlike _just_set_matrix, this also iterates all faces, transforms vertices in software,
// computes per-face lighting via dot product, and submits to the software polygon rasteriser.
void FIGURE_draw_prim_tween_person_only(
    SLONG prim,
    struct Matrix33* rot_mat,
    SLONG off_dx,
    SLONG off_dy,
    SLONG off_dz,
    SLONG recurse_level,
    Thing* p_thing)
{
    SLONG i;

    SLONG sp;
    SLONG ep;

    SLONG p0;
    SLONG p1;
    SLONG p2;
    SLONG p3;

    Matrix33 mat_final;

    SLONG page;

    PrimFace4* p_f4;
    PrimFace3* p_f3;
    PrimObject* p_obj;

    POLY_Point* pp;

    POLY_Point* tri[3];
    POLY_Point* quad[4];
    SLONG tex_page_offset;

    tex_page_offset = p_thing->Genus.Person->pcom_colour & 0x3;

    // Per-body-part WORLD transform read straight from the interpolation
    // pose snapshot — the SINGLE always-available source of pose. No legacy
    // keyframe / hierarchy recompute: the snapshot already bakes body
    // rotation, character scale and the inter-bone hierarchy. (The old
    // end_mat/end_pos propagation only fed HIERARCHY_Get_Body_Part_Offset,
    // whose result was always overwritten by this snapshot — see
    // skeletal_skinning_plan.md.)
    const BoneInterpTransform* pose = render_interp_get_cached_pose(p_thing);
    SLONG part = FIGURE_dhpr_rdata1[recurse_level].part_number;
    if (!pose || part < 0 || part >= POSE_MAX_BONES)
        return;

    const BoneInterpTransform& xf = pose[part];
    float off_x = xf.pos_x;
    float off_y = xf.pos_y;
    float off_z = xf.pos_z;
    mat_final = xf.rot;

    float fmatrix[9];
    fmatrix[0] = float(mat_final.M[0][0]) * (1.0F / 32768.0F);
    fmatrix[1] = float(mat_final.M[0][1]) * (1.0F / 32768.0F);
    fmatrix[2] = float(mat_final.M[0][2]) * (1.0F / 32768.0F);
    fmatrix[3] = float(mat_final.M[1][0]) * (1.0F / 32768.0F);
    fmatrix[4] = float(mat_final.M[1][1]) * (1.0F / 32768.0F);
    fmatrix[5] = float(mat_final.M[1][2]) * (1.0F / 32768.0F);
    fmatrix[6] = float(mat_final.M[2][0]) * (1.0F / 32768.0F);
    fmatrix[7] = float(mat_final.M[2][1]) * (1.0F / 32768.0F);
    fmatrix[8] = float(mat_final.M[2][2]) * (1.0F / 32768.0F);

    POLY_set_local_rotation(
        off_x,
        off_y,
        off_z,
        fmatrix);

    p_obj = &prim_objects[prim];

    sp = p_obj->StartPoint;
    ep = p_obj->EndPoint;

    POLY_buffer_upto = 0;

    // Check for being a gun
    if (prim == 256) {
        i = sp;
    } else if (prim == 258) {
        // Or a shotgun
        i = sp + 15;
    } else if (prim == 260) {
        // or an AK
        i = sp + 32;
    } else {
        goto no_muzzle_calcs; // which skips...
    }

    // this bit, which only executes if one of the above tests is true.
    pp = &POLY_buffer[POLY_buffer_upto]; // no ++, so reused
    pp->x = AENG_dx_prim_points[i].X;
    pp->y = AENG_dx_prim_points[i].Y;
    pp->z = AENG_dx_prim_points[i].Z;
    MATRIX_MUL(
        fmatrix,
        pp->x,
        pp->y,
        pp->z);

    pp->x += off_x;
    pp->y += off_y;
    pp->z += off_z;
    p_thing->Genus.Person->GunMuzzle.X = pp->x * 256;
    p_thing->Genus.Person->GunMuzzle.Y = pp->y * 256;
    p_thing->Genus.Person->GunMuzzle.Z = pp->z * 256;

no_muzzle_calcs:

    ASSERT(MM_bLightTableAlreadySetUp);

    if (WITHIN(prim, 261, 263)) {
        // This is a muzzle flash — no lighting.

        for (i = sp; i < ep; i++) {
            ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

            pp = &POLY_buffer[POLY_buffer_upto++];

            POLY_transform_using_local_rotation(
                AENG_dx_prim_points[i].X,
                AENG_dx_prim_points[i].Y,
                AENG_dx_prim_points[i].Z,
                pp);

            pp->colour = 0xff808080;
            pp->specular = 0xff000000;
        }

        for (i = p_obj->StartFace4; i < p_obj->EndFace4; i++) {
            p_f4 = &prim_faces4[i];

            p0 = p_f4->Points[0] - sp;
            p1 = p_f4->Points[1] - sp;
            p2 = p_f4->Points[2] - sp;
            p3 = p_f4->Points[3] - sp;

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
                quad[0]->v = float(p_f4->UV[0][1]) * (1.0F / 32.0F);

                quad[1]->u = float(p_f4->UV[1][0]) * (1.0F / 32.0F);
                quad[1]->v = float(p_f4->UV[1][1]) * (1.0F / 32.0F);

                quad[2]->u = float(p_f4->UV[2][0]) * (1.0F / 32.0F);
                quad[2]->v = float(p_f4->UV[2][1]) * (1.0F / 32.0F);

                quad[3]->u = float(p_f4->UV[3][0]) * (1.0F / 32.0F);
                quad[3]->v = float(p_f4->UV[3][1]) * (1.0F / 32.0F);

                page = p_f4->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f4->TexturePage;

                if (tex_page_offset && page > 10 * 64 && alt_texture[page - 10 * 64]) {
                    page = alt_texture[page - 10 * 64] + tex_page_offset - 1;
                } else
                    page += FACE_PAGE_OFFSET;

                POLY_add_quad(quad, page, !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED));
            }
        }

        for (i = p_obj->StartFace3; i < p_obj->EndFace3; i++) {
            p_f3 = &prim_faces3[i];

            p0 = p_f3->Points[0] - sp;
            p1 = p_f3->Points[1] - sp;
            p2 = p_f3->Points[2] - sp;

            ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
            ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
            ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));

            tri[0] = &POLY_buffer[p0];
            tri[1] = &POLY_buffer[p1];
            tri[2] = &POLY_buffer[p2];

            if (POLY_valid_triangle(tri)) {
                tri[0]->u = float(p_f3->UV[0][0] & 0x3f) * (1.0F / 32.0F);
                tri[0]->v = float(p_f3->UV[0][1]) * (1.0F / 32.0F);

                tri[1]->u = float(p_f3->UV[1][0]) * (1.0F / 32.0F);
                tri[1]->v = float(p_f3->UV[1][1]) * (1.0F / 32.0F);

                tri[2]->u = float(p_f3->UV[2][0]) * (1.0F / 32.0F);
                tri[2]->v = float(p_f3->UV[2][1]) * (1.0F / 32.0F);

                page = p_f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f3->TexturePage;

                if (tex_page_offset && page > 10 * 64 && alt_texture[page - 10 * 64]) {
                    page = alt_texture[page - 10 * 64] + tex_page_offset - 1;
                } else
                    page += FACE_PAGE_OFFSET;

                POLY_add_triangle(tri, page, !(p_f3->DrawFlags & POLY_FLAG_DOUBLESIDED));
            }
        }

        return;
    } else {

        ASSERT(MM_bLightTableAlreadySetUp);

        extern float POLY_cam_matrix_comb[9];
        extern float POLY_cam_off_x;
        extern float POLY_cam_off_y;
        extern float POLY_cam_off_z;

        extern GEMatrix g_matProjection;
        extern GEMatrix g_matWorld;
        extern GEViewport g_viewData;

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

        extern DWORD g_dw3DStuffHeight;
        extern DWORD g_dw3DStuffY;
        DWORD dwWidth = g_viewData.dwWidth >> 1;
        DWORD dwHeight = g_dw3DStuffHeight >> 1;
        DWORD dwX = g_viewData.dwX;
        DWORD dwY = g_dw3DStuffY;
        MM_pMatrix[0]._11 = 0.0f;
        MM_pMatrix[0]._12 = matTemp._11 * (float)dwWidth + matTemp._14 * (float)(dwX + dwWidth);
        MM_pMatrix[0]._13 = matTemp._12 * -(float)dwHeight + matTemp._14 * (float)(dwY + dwHeight);
        MM_pMatrix[0]._14 = matTemp._14;
        MM_pMatrix[0]._21 = 0.0f;
        MM_pMatrix[0]._22 = matTemp._21 * (float)dwWidth + matTemp._24 * (float)(dwX + dwWidth);
        MM_pMatrix[0]._23 = matTemp._22 * -(float)dwHeight + matTemp._24 * (float)(dwY + dwHeight);
        MM_pMatrix[0]._24 = matTemp._24;
        MM_pMatrix[0]._31 = 0.0f;
        MM_pMatrix[0]._32 = matTemp._31 * (float)dwWidth + matTemp._34 * (float)(dwX + dwWidth);
        MM_pMatrix[0]._33 = matTemp._32 * -(float)dwHeight + matTemp._34 * (float)(dwY + dwHeight);
        MM_pMatrix[0]._34 = matTemp._34;
        // Validation magic number.
        ULONG EVal = 0xe0001000;
        MM_pMatrix[0]._41 = *(float*)&EVal;
        MM_pMatrix[0]._42 = matTemp._41 * (float)dwWidth + matTemp._44 * (float)(dwX + dwWidth);
        MM_pMatrix[0]._43 = matTemp._42 * -(float)dwHeight + matTemp._44 * (float)(dwY + dwHeight);
        MM_pMatrix[0]._44 = matTemp._44;

        // 251 is a magic number for the DIP call.
        const float fNormScale = 251.0f;

        // Transform light direction by inverse (=transpose) object matrix.
        GEVector vTemp;
        vTemp.x = MM_vLightDir.x * fmatrix[0] + MM_vLightDir.y * fmatrix[3] + MM_vLightDir.z * fmatrix[6];
        vTemp.y = MM_vLightDir.x * fmatrix[1] + MM_vLightDir.y * fmatrix[4] + MM_vLightDir.z * fmatrix[7];
        vTemp.z = MM_vLightDir.x * fmatrix[2] + MM_vLightDir.y * fmatrix[5] + MM_vLightDir.z * fmatrix[8];

        MM_pNormal[0] = 0.0f;
        MM_pNormal[1] = vTemp.x * fNormScale;
        MM_pNormal[2] = vTemp.y * fNormScale;
        MM_pNormal[3] = vTemp.z * fNormScale;
    }

    // The MM stuff doesn't like specular to be enabled.
    ge_set_specular_enabled(false);

    // For now, just calculate as-and-when.
    TomsPrimObject* pPrimObj = &(D3DObj[prim]);
    if (pPrimObj->wNumMaterials == 0) {
        // Not initialised. Do so.
        FIGURE_generate_D3D_object(prim);
    }

    // Tell the LRU cache we used this one.
    FIGURE_touch_LRU_of_object(pPrimObj);

    ASSERT(pPrimObj->pD3DVertices != NULL);
    ASSERT(pPrimObj->pMaterials != NULL);
    ASSERT(pPrimObj->pwListIndices != NULL);
    ASSERT(pPrimObj->pwStripIndices != NULL);
    ASSERT(pPrimObj->wNumMaterials != 0);

    PrimObjectMaterial* pMat = pPrimObj->pMaterials;

    GEMultiMatrix mm;
    mm.matrices = MM_pMatrix;
    mm.lpvLightDirs = MM_pNormal;
    mm.skin_cache = nullptr; // 1D: set per-material before each draw

    GEVertex* pVertex = (GEVertex*)pPrimObj->pD3DVertices;
    UWORD* pwStripIndices = pPrimObj->pwStripIndices;
    for (int iMatNum = pPrimObj->wNumMaterials; iMatNum > 0; iMatNum--) {
        UWORD wPage = pMat->wTexturePage;
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

        extern GEMatrix g_matWorld;

        PolyPage* pa = &(POLY_Page[wRealPage]);
        ASSERT(!pa->RS.NeedsSorting() && (FIGURE_alpha == 255));

        // Originally had a distance check here that skipped near-plane geometry (with an empty
        // else branch — the fallback was never implemented). Now always renders via MultiMatrix;
        // z guard in ge_draw_multi_matrix prevents div/0 for near vertices, GPU clips the rest.

        if (wPage & TEXTURE_PAGE_FLAG_TINT) {
            mm.lpLightTable = MM_pcFadeTableTint;
        } else {
            mm.lpLightTable = MM_pcFadeTable;
        }
        mm.lpvVertices = pVertex;
        mm.skin_cache = figure_skin_cache_slot(pPrimObj, pMat);

        pa->RS.SetCullMode(GECullMode::CCW);
        pa->RS.SetAlphaBlendEnabled(false);
        pa->RS.SetTextureBlend(GETextureBlend::ModulateAlpha);
        pa->RS.SetChanged();

        {
            // Set view-space Z for CPU fog in ge_draw_multi_matrix.
            g_mm_fog_view_z = g_matWorld._43;

            ge_draw_multi_matrix(
                GEMMVertexType::Unlit,
                &mm,
                pMat->wNumVertices,
                pwStripIndices,
                pMat->wNumStripIndices);
        }

        // Next material
        pVertex += pMat->wNumVertices;
        pwStripIndices += pMat->wNumStripIndices;

        pMat++;
    }

    // The MM stuff doesn't like specular to be enabled.
    ge_set_specular_enabled(true);

    // No environment mapping.
    ASSERT(p_thing && (p_thing->Class != CLASS_VEHICLE));

    ASSERT(MM_bLightTableAlreadySetUp);
}
