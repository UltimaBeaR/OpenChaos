// Temporary: D3D-specific character rendering system from DDEngine.
// This entire file will be replaced in Stage 7 (new renderer).
// Preserved 1:1 here for Stage 4 completeness.

#include "engine/animation/figure.h"
#include "engine/animation/figure_globals.h"

// Temporary: D3D, game types, and all subsystems needed throughout the original file.
#include "Game.h"
#include "aeng.h"
#include "poly.h"
#include "sprite.h"
#include "..\headers\fmatrix.h"
#include "..\headers\mav.h"
#include "..\headers\interact.h"
#include "night.h"
#include "shadow.h"
#include "matrix.h"
#include "animate.h"
#include "mesh.h"
#include "dirt.h"
#include "texture.h"
#include "math.h"
#include "interfac.h"
#include "Hierarchy.h"
#include "Quaternion.h"
#include "memory.h"
#include "..\headers\person.h"
#include "..\headers\pcom.h"
#include "..\headers\eway.h"
#include "..\headers\dirt.h"
#include "ddlib.h"
#include "panel.h"
#include "renderstate.h"
#include "polypage.h"
#include "psystem.h"

// Note: animation depends on lighting (BuildMMLightingTable reads NIGHT_* globals).
// This cross-engine coupling exists in the original; will be resolved in Stage 7.
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"

// uc_orig: DeadAndBuried (fallen/DDEngine/Source/figure.cpp)
// Paints a solid 50x50 colour block on the front DirectDraw surface for crash diagnostics.
void DeadAndBuried(DWORD dwColour)
{
    DDSURFACEDESC2 mine;
    HRESULT res;

    InitStruct(mine);
    mine.dwFlags = DDSD_PITCH;
    res = the_display.lp_DD_FrontSurface->Lock(NULL, &mine, DDLOCK_WAIT, NULL);
    if (FAILED(res))
        return;

    char* pdwDest = (char*)mine.lpSurface;
    for (int i = 0; i < 50; i++) {
        DWORD* pdwDest1 = (DWORD*)pdwDest;
        pdwDest += mine.lPitch;
        for (int j = 0; j < 50; j++) {
            *pdwDest1++ = dwColour;
        }
    }

    res = the_display.lp_DD_FrontSurface->Unlock(NULL);
}

// MM_* pointer initialisation is done in figure_globals.cpp via a static constructor.
// (The backing storage arrays and the initialiser struct live there to keep all global
//  state in _globals files per project rules.)

// uc_orig: BuildMMLightingTable (fallen/DDEngine/Source/figure.cpp)
// Pre-computes a 128-entry D3DCOLOR lookup table used by the D3D MultiMatrix extension.
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
    D3DVECTOR vTotal;
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
    D3DCOLORVALUE cvAmb, cvLight;
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
    D3DCOLORVALUE cvCur = cvAmb;
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
        dy += (GAME_TURN * ((c0 & 3) + 2));
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
                0,
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

    handle = FileOpen("data\\flames1.pal");

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
    UBYTE* palptr;
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
            dy = (GAME_TURN + c0) / 2;
            u = 0.25f * (dy & 3);
            v = 0.25f * ((dy & !3) / 3);
            v += 0.002f;
            w = h = 0.25f;
            dy = 0;
        }

        dy = get_steam_rand() & 0x1ff;
        dy += (GAME_TURN * 5);
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
                palptr = (trans * 3) + fire_pal;
                palndx = (256 - trans) * 3;
                trans <<= 24;
                trans += (fire_pal[palndx] << 16) + (fire_pal[palndx + 1] << 8) + fire_pal[palndx + 2];
                scale = 50;
                break;
            case 0:
            case 3:
                trans = abs(dy - y) * 20;
                trans = SIN(trans);
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
    UBYTE* palptr;
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
            dy = (GAME_TURN + c0) / 2;
            u = 0.25f * (dy & 3);
            v = 0.25f * ((dy >> 2) & 3);
            w = h = 0.25f;
        }
    }

    dy = get_steam_rand() & 0x1ff;
    dy += (GAME_TURN * 5);
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
            palptr = (trans * 3) + fire_pal;
            palndx = (256 - trans) * 3;
            trans <<= 24;
            trans += (fire_pal[palndx] << 16) + (fire_pal[palndx + 1] << 8) + fire_pal[palndx + 2];
            scale = 50;
            break;
        case POLY_PAGE_FLAMES2:
            palptr = (trans * 3) + fire_pal;
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
                    DWORD dwTurnsAgo = (GAME_TURN - dwGameTurnLastUsed[i]);
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
                    DWORD dwTurnsAgo = (GAME_TURN - dwGameTurnLastUsed[i]);
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
    dwGameTurnLastUsed[iQueuePos] = GAME_TURN;
    m_dwSizeOfQueue += (DWORD)pPrimObj->wTotalSizeOfObj;
    ASSERT(m_dwSizeOfQueue < PRIM_LRU_QUEUE_SIZE);
}

// uc_orig: FIGURE_touch_LRU_of_object (fallen/DDEngine/Source/figure.cpp)
// Updates the last-used game turn for a cached prim mesh to prevent premature eviction.
void FIGURE_touch_LRU_of_object(TomsPrimObject* pPrimObj)
{
    ASSERT((pPrimObj->bLRUQueueNumber >= 0) && (pPrimObj->bLRUQueueNumber < m_iLRUQueueSize));
    ASSERT(ptpoLRUQueue[pPrimObj->bLRUQueueNumber] == pPrimObj);
    dwGameTurnLastUsed[pPrimObj->bLRUQueueNumber] = GAME_TURN;
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

    TPO_pVert = (D3DVERTEX*)MemAlloc(MAX_VERTS * sizeof(D3DVERTEX));
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

                // Find the rendered page (allows texture paging/aliasing).
                UWORD wRealPage = wTexturePage & TEXTURE_PAGE_MASK;

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

                    D3DVERTEX* pFirstVertex = TPO_pCurVertex;

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

                                        D3DVERTEX d3dvert;

                                        int pt;
                                        if (bInnerTris) {
                                            pt = p_f3->Points[i];
                                            d3dvert.dvTU = float(p_f3->UV[i][0] & 0x3f) * (1.0F / 32.0F);
                                            d3dvert.dvTV = float(p_f3->UV[i][1]) * (1.0F / 32.0F);
                                        } else {
                                            pt = p_f4->Points[i];
                                            d3dvert.dvTU = float(p_f4->UV[i][0] & 0x3f) * (1.0F / 32.0F);
                                            d3dvert.dvTV = float(p_f4->UV[i][1]) * (1.0F / 32.0F);
                                        }
                                        // Clamp UVs to [0,1] to avoid texture border bleeding.
                                        if (d3dvert.dvTU < 0.0f) {
                                            d3dvert.dvTU = 0.0f;
                                        } else if (d3dvert.dvTU > 1.0f) {
                                            d3dvert.dvTU = 1.0f;
                                        }
                                        if (d3dvert.dvTV < 0.0f) {
                                            d3dvert.dvTV = 0.0f;
                                        } else if (d3dvert.dvTV > 1.0f) {
                                            d3dvert.dvTV = 1.0f;
                                        }

                                        d3dvert.dvTU = d3dvert.dvTU * pa->m_UScale + pa->m_UOffset;
                                        d3dvert.dvTV = d3dvert.dvTV * pa->m_VScale + pa->m_VOffset;

                                        d3dvert.dvX = AENG_dx_prim_points[pt].X;
                                        d3dvert.dvY = AENG_dx_prim_points[pt].Y;
                                        d3dvert.dvZ = AENG_dx_prim_points[pt].Z;
                                        d3dvert.dvNX = prim_normal[pt].X * fNormScale;
                                        d3dvert.dvNY = prim_normal[pt].Y * fNormScale;
                                        d3dvert.dvNZ = prim_normal[pt].Z * fNormScale;
                                        // MM index must be set last — it writes into byte 12 of the vertex.
                                        SET_MM_INDEX(d3dvert, TPO_ubPrimObjMMIndex[iInnerPrimNumber]);

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

                                            *TPO_pCurVertex = d3dvert;
                                            TPO_pCurVertex++;
                                            pMaterial->wNumVertices++;
                                            TPO_iNumVertices++;
                                            ASSERT(TPO_iNumVertices < MAX_VERTS);

                                            if (TPO_iNumVertices >= MAX_VERTS) {
                                                DeadAndBuried(0xffffffff);
                                            }

                                            // Grow bounding sphere if this vertex is farther out.
                                            float fDistSqu = (d3dvert.dvX * d3dvert.dvX) + (d3dvert.dvY * d3dvert.dvY) + (d3dvert.dvZ * d3dvert.dvZ);
                                            if ((*pfBoundingSphereRadius * *pfBoundingSphereRadius) < fDistSqu) {
                                                *pfBoundingSphereRadius = sqrtf(fDistSqu);
                                            }
                                        } else {
                                            // Walk the UV-variant chain for this position+normal.
                                            int iLastIndex = iVertIndex;
                                            while (iVertIndex != -1) {
                                                ASSERT(pFirstVertex[iVertIndex].dvX == d3dvert.dvX);
                                                ASSERT(pFirstVertex[iVertIndex].dvY == d3dvert.dvY);
                                                ASSERT(pFirstVertex[iVertIndex].dvZ == d3dvert.dvZ);
                                                ASSERT(pFirstVertex[iVertIndex].dvNX == d3dvert.dvNX);
                                                ASSERT(pFirstVertex[iVertIndex].dvNY == d3dvert.dvNY);
                                                ASSERT(pFirstVertex[iVertIndex].dvNZ == d3dvert.dvNZ);
// uc_orig: CLOSE_ENOUGH (fallen/DDEngine/Source/figure.cpp)
#define CLOSE_ENOUGH(a, b) (fabsf((a) - (b)) < 0.00001f)
                                                if (CLOSE_ENOUGH(pFirstVertex[iVertIndex].dvTU, d3dvert.dvTU) && CLOSE_ENOUGH(pFirstVertex[iVertIndex].dvTV, d3dvert.dvTV)) {
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

                                                *TPO_pCurVertex = d3dvert;
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
    dwTotalSize += 32 + TPO_iNumVertices * sizeof(D3DVERTEX);
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
    pPrimObj->pD3DVertices = (void*)(((DWORD)pcBlock + 31) & ~31);
    memcpy(pPrimObj->pD3DVertices, TPO_pVert, TPO_iNumVertices * sizeof(D3DVERTEX));
    pcBlock = (char*)pPrimObj->pD3DVertices + TPO_iNumVertices * sizeof(D3DVERTEX);

    pPrimObj->pwStripIndices = (UWORD*)pcBlock;
    memcpy(pPrimObj->pwStripIndices, TPO_pStripIndices, TPO_iNumStripIndices * sizeof(UWORD));
    pcBlock += TPO_iNumStripIndices * sizeof(UWORD);

    ASSERT((DWORD)pcBlock < (DWORD)pPrimObj->pwListIndices + dwTotalSize);

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
    PrimObject* p_obj = &prim_objects[prim];
    TomsPrimObject* pPrimObj = &(D3DObj[prim]);

    FIGURE_TPO_init_3d_object(pPrimObj);
    FIGURE_TPO_add_prim_to_current_object(prim, 0);
    FIGURE_TPO_finish_3d_object(pPrimObj);
}

// uc_orig: FIGURE_draw_prim_tween (fallen/DDEngine/Source/figure.cpp)
// Software-path body-part renderer (used when D3D MultiMatrix is active for matrix setup
// but the per-face submission still goes through here for the non-person-only path).
// Interpolates keyframe A→B via lerp (offsets) and slerp (rotations), transforms all vertices
// via the combined world matrix, then draws each face via POLY_add_quad / POLY_add_triangle
// for the software rasteriser.  The GPU MultiMatrix path (DrawIndPrimMM) is also triggered here
// for opaque, non-near-clipped materials.
// part_number: which skeleton slot this prim represents (0xffffffff = unknown).
// colour_and: multiplied into the tint fade table.
void FIGURE_draw_prim_tween(
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
    CMatrix33* parent_base_mat,
    Matrix31* parent_base_pos,
    Matrix33* parent_curr_mat,
    Matrix31* parent_curr_pos,
    Matrix33* end_mat,
    Matrix31* end_pos,
    Thing* p_thing,
    SLONG part_number,
    ULONG colour_and)
{

    SLONG i;
    SLONG j;

    SLONG sp;
    SLONG ep;

    SLONG p0;
    SLONG p1;
    SLONG p2;
    SLONG p3;

    SLONG nx;
    SLONG ny;
    SLONG nz;

    SLONG red;
    SLONG green;
    SLONG blue;
    SLONG dprod;
    SLONG r;
    SLONG g;
    SLONG b;

    SLONG dr;
    SLONG dg;
    SLONG db;

    SLONG face_colour;

    SLONG page;

    Matrix31 offset;
    Matrix33 mat2;
    Matrix33 mat_final;

    ULONG qc0;
    ULONG qc1;
    ULONG qc2;
    ULONG qc3;

    SVector temp;

    PrimFace4* p_f4;
    PrimFace3* p_f3;
    PrimObject* p_obj;
    NIGHT_Found* nf;

    POLY_Point* pp;
    POLY_Point* ps;

    POLY_Point* tri[3];
    POLY_Point* quad[4];
    SLONG tex_page_offset;

    tex_page_offset = p_thing->Genus.Person->pcom_colour & 0x3;

    // Forward declarations of matrix helpers used below.
    void matrix_transform(Matrix31 * result, Matrix33 * trans, Matrix31 * mat2);
    void matrix_transformZMY(Matrix31 * result, Matrix33 * trans, Matrix31 * mat2);
    void matrix_mult33(Matrix33 * result, Matrix33 * mat1, Matrix33 * mat2);

    if (parent_base_mat) {
        // Hierarchical body part: compute offset in parent space.
        Matrix31 p;
        p.M[0] = anim_info->OffsetX;
        p.M[1] = anim_info->OffsetY;
        p.M[2] = anim_info->OffsetZ;

        HIERARCHY_Get_Body_Part_Offset(&offset, &p,
            parent_base_mat, parent_base_pos,
            parent_curr_mat, parent_curr_pos);

        if (end_pos)
            *end_pos = offset;
    } else {
        // Lerp offset between keyframe A and B.
        offset.M[0] = (anim_info->OffsetX << 8) + ((anim_info_next->OffsetX + off_dx - anim_info->OffsetX) * tween);
        offset.M[1] = (anim_info->OffsetY << 8) + ((anim_info_next->OffsetY + off_dy - anim_info->OffsetY) * tween);
        offset.M[2] = (anim_info->OffsetZ << 8) + ((anim_info_next->OffsetZ + off_dz - anim_info->OffsetZ) * tween);

        /* We don't have bikes.
                        if (p_thing->Class == CLASS_BIKE && part_number == 3)
                        {
                                //offset.M[0] = 0x0;
                                //offset.M[1] = 0x900;
                                offset.M[2] = 0x3500;
                        }
        */
        if (end_pos) {
            *end_pos = offset;
        }
    }

    // Convert offset to float world-space position via rot_mat (fixed-point ×32768).
    float off_x = (float(offset.M[0]) / 256.f) * (float(rot_mat->M[0][0]) / 32768.f) + (float(offset.M[1]) / 256.f) * (float(rot_mat->M[0][1]) / 32768.f) + (float(offset.M[2]) / 256.f) * (float(rot_mat->M[0][2]) / 32768.f);
    float off_y = (float(offset.M[0]) / 256.f) * (float(rot_mat->M[1][0]) / 32768.f) + (float(offset.M[1]) / 256.f) * (float(rot_mat->M[1][1]) / 32768.f) + (float(offset.M[2]) / 256.f) * (float(rot_mat->M[1][2]) / 32768.f);
    float off_z = (float(offset.M[0]) / 256.f) * (float(rot_mat->M[2][0]) / 32768.f) + (float(offset.M[1]) / 256.f) * (float(rot_mat->M[2][1]) / 32768.f) + (float(offset.M[2]) / 256.f) * (float(rot_mat->M[2][2]) / 32768.f);

    SLONG character_scale = person_get_scale(p_thing);
    float character_scalef = float(character_scale) / 256.f;

    off_x *= character_scalef;
    off_y *= character_scalef;
    off_z *= character_scalef;

    /*

    if (p_thing->Class == CLASS_BIKE)
    {
            off_x = 0;
            off_y = 0;
            off_z = 0;
    }

    */

    off_x += float(x);
    off_y += float(y);
    off_z += float(z);

    float fmatrix[9];
    SLONG imatrix[9];

    {
        // Slerp rotation between keyframe A and B, then combine with parent/world matrix.
        CMatrix33 m1, m2;
        GetCMatrix(anim_info, &m1);
        GetCMatrix(anim_info_next, &m2);

        CQuaternion::BuildTween(&mat2, &m1, &m2, tween);

        if (end_mat)
            *end_mat = mat2;

        matrix_mult33(&mat_final, rot_mat, &mat2);

        // Scale the combined matrix by character_scale (256-based fixed point).
        mat_final.M[0][0] = (mat_final.M[0][0] * character_scale) / 256;
        mat_final.M[0][1] = (mat_final.M[0][1] * character_scale) / 256;
        mat_final.M[0][2] = (mat_final.M[0][2] * character_scale) / 256;
        mat_final.M[1][0] = (mat_final.M[1][0] * character_scale) / 256;
        mat_final.M[1][1] = (mat_final.M[1][1] * character_scale) / 256;
        mat_final.M[1][2] = (mat_final.M[1][2] * character_scale) / 256;
        mat_final.M[2][0] = (mat_final.M[2][0] * character_scale) / 256;
        mat_final.M[2][1] = (mat_final.M[2][1] * character_scale) / 256;
        mat_final.M[2][2] = (mat_final.M[2][2] * character_scale) / 256;

        fmatrix[0] = float(mat_final.M[0][0]) * (1.0F / 32768.0F);
        fmatrix[1] = float(mat_final.M[0][1]) * (1.0F / 32768.0F);
        fmatrix[2] = float(mat_final.M[0][2]) * (1.0F / 32768.0F);
        fmatrix[3] = float(mat_final.M[1][0]) * (1.0F / 32768.0F);
        fmatrix[4] = float(mat_final.M[1][1]) * (1.0F / 32768.0F);
        fmatrix[5] = float(mat_final.M[1][2]) * (1.0F / 32768.0F);
        fmatrix[6] = float(mat_final.M[2][0]) * (1.0F / 32768.0F);
        fmatrix[7] = float(mat_final.M[2][1]) * (1.0F / 32768.0F);
        fmatrix[8] = float(mat_final.M[2][2]) * (1.0F / 32768.0F);

        imatrix[0] = mat_final.M[0][0] * 2;
        imatrix[1] = mat_final.M[0][1] * 2;
        imatrix[2] = mat_final.M[0][2] * 2;
        imatrix[3] = mat_final.M[1][0] * 2;
        imatrix[4] = mat_final.M[1][1] * 2;
        imatrix[5] = mat_final.M[1][2] * 2;
        imatrix[6] = mat_final.M[2][0] * 2;
        imatrix[7] = mat_final.M[2][1] * 2;
        imatrix[8] = mat_final.M[2][2] * 2;
    }

    /*

    if (part_number == SUB_OBJECT_HEAD)
    {
            fmatrix[0]=	+0.131534;
            fmatrix[1]=	-0.000000;
            fmatrix[2]=	+0.991312;
            fmatrix[3]=	+0.00133604;
            fmatrix[4]=	+0.999999;
            fmatrix[5]=	-0.000177275;
            fmatrix[6]=	-0.991311;
            fmatrix[7]=	0.00134775;
            fmatrix[8]=	0.131534;;
    }

    */

    if (prim == 267) {
        static int count = 0;

        count += 1;
    }

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
    } else
        if (prim == 258) {
            i = sp + 15;
        }
        else if (prim == 260) {
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

        extern D3DMATRIX g_matProjection;
        extern D3DMATRIX g_matWorld;
        extern D3DVIEWPORT2 g_viewData;

        D3DMATRIX matTemp;

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
        unsigned long EVal = 0xe0001000;
        MM_pMatrix[0]._41 = *(float*)&EVal;
        MM_pMatrix[0]._42 = matTemp._41 * (float)dwWidth + matTemp._44 * (float)(dwX + dwWidth);
        MM_pMatrix[0]._43 = matTemp._42 * -(float)dwHeight + matTemp._44 * (float)(dwY + dwHeight);
        MM_pMatrix[0]._44 = matTemp._44;

        // 251 is a magic number for the DIP call.
        const float fNormScale = 251.0f;

        // Transform light direction into object space (inverse = transpose for orthonormal matrices).
        D3DVECTOR vTemp;
        vTemp.x = MM_vLightDir.x * fmatrix[0] + MM_vLightDir.y * fmatrix[3] + MM_vLightDir.z * fmatrix[6];
        vTemp.y = MM_vLightDir.x * fmatrix[1] + MM_vLightDir.y * fmatrix[4] + MM_vLightDir.z * fmatrix[7];
        vTemp.z = MM_vLightDir.x * fmatrix[2] + MM_vLightDir.y * fmatrix[5] + MM_vLightDir.z * fmatrix[8];

        MM_pNormal[0] = 0.0f;
        MM_pNormal[1] = vTemp.x * fNormScale;
        MM_pNormal[2] = vTemp.y * fNormScale;
        MM_pNormal[3] = vTemp.z * fNormScale;
    }

    // Disable specular — MM extension requires it off.
    (the_display.lp_D3D_Device)->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, UC_FALSE);

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

    D3DMULTIMATRIX d3dmm;
    d3dmm.lpd3dMatrices = MM_pMatrix;
    d3dmm.lpvLightDirs = MM_pNormal;

    D3DVERTEX* pVertex = (D3DVERTEX*)pPrimObj->pD3DVertices;
    UWORD* pwListIndices = pPrimObj->pwListIndices;
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

        extern D3DMATRIX g_matWorld;

        PolyPage* pa = &(POLY_Page[wRealPage]);
        ASSERT((character_scalef < 1.2f) && (character_scalef > 0.8f));
        if (!pa->RS.NeedsSorting() && (FIGURE_alpha == 255) && (((g_matWorld._43 * 32768.0f) - (pPrimObj->fBoundingSphereRadius * character_scalef)) > (POLY_ZCLIP_PLANE * 32768.0f))) {
            // Opaque, not near-clipped: use fast MultiMatrix path.
            if (wPage & TEXTURE_PAGE_FLAG_TINT) {
                d3dmm.lpLightTable = MM_pcFadeTableTint;
            } else {
                d3dmm.lpLightTable = MM_pcFadeTable;
            }
            d3dmm.lpvVertices = pVertex;

            pa->RS.SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
            pa->RS.SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_FALSE);
            pa->RS.SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATEALPHA);
            pa->RS.SetChanged();

            HRESULT hres;

            {
                hres = DrawIndPrimMM(
                    (the_display.lp_D3D_Device),
                    D3DFVF_VERTEX,
                    &d3dmm,
                    pMat->wNumVertices,
                    pwStripIndices,
                    pMat->wNumStripIndices);
            }

        } else {
            // Alpha or near-clipped path: currently unimplemented (fast-reject instead).
        }

        pVertex += pMat->wNumVertices;
        pwListIndices += pMat->wNumListIndices;
        pwStripIndices += pMat->wNumStripIndices;

        pMat++;
    }

    (the_display.lp_D3D_Device)->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, UC_TRUE);

    if (!MM_bLightTableAlreadySetUp) {
    }
}

// Chunk 3+ (FIGURE_draw_prim_tween_warped, FIGURE_draw_hierarchical_prim_recurse,
// part_type, mandom, local_set_seed, FIGURE_draw, ANIM_obj_draw, ANIM_obj_draw_warped,
// FIGURE_draw_prim_tween_reflection, FIGURE_draw_reflection,
// FIGURE_draw_prim_tween_person_only_just_set_matrix, FIGURE_draw_prim_tween_person_only)
// — deferred to future iterations.
