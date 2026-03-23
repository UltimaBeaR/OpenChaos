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

// Chunk 2+ (FIGURE_TPO_add_prim_to_current_object, FIGURE_TPO_finish_3d_object,
// FIGURE_generate_D3D_object, FIGURE_draw_prim_tween, FIGURE_draw_prim_tween_warped,
// FIGURE_draw_hierarchical_prim_recurse, FIGURE_draw, ANIM_obj_draw,
// FIGURE_draw_prim_tween_reflection, FIGURE_draw_reflection,
// FIGURE_draw_prim_tween_person_only_just_set_matrix, FIGURE_draw_prim_tween_person_only)
// — deferred to future iterations.
