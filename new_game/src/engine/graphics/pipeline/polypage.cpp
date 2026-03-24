#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/pipeline/polypage_globals.h"
#include "engine/graphics/pipeline/vertex_buffer_globals.h"
#include "engine/core/matrix.h"
#include "engine/platform/uc_common.h"
#include "engine/graphics/resources/d3d_texture.h"   // D3DTexture full definition for SetTexOffset

#include <math.h>

// AENG_total_polys_drawn is defined in aeng.cpp (not yet migrated).
extern int AENG_total_polys_drawn;

// g_dw3DStuffHeight and g_dw3DStuffY are defined in poly.cpp (not yet migrated).
// Used by GenerateMMMatrixFromStandardD3DOnes to support letterbox mode.
extern DWORD g_dw3DStuffHeight;
extern DWORD g_dw3DStuffY;

// PolyPage static member definitions.
// uc_orig: s_AlphaSort (fallen/DDEngine/Source/polypage.cpp)
bool PolyPage::s_AlphaSort = true;
// uc_orig: s_ColourMask (fallen/DDEngine/Source/polypage.cpp)
ULONG PolyPage::s_ColourMask = 0xFFFFFFFF;
// uc_orig: s_XScale (fallen/DDEngine/Source/polypage.cpp)
float PolyPage::s_XScale = 1.0;
// uc_orig: s_YScale (fallen/DDEngine/Source/polypage.cpp)
float PolyPage::s_YScale = 1.0;

// uc_orig: AlphaPremult (fallen/DDEngine/Source/polypage.cpp)
// Premultiply colour channels by alpha channel.
static inline void AlphaPremult(UBYTE* color)
{
    color[0] = UBYTE((ULONG(color[0]) * ULONG(color[3])) >> 8);
    color[1] = UBYTE((ULONG(color[1]) * ULONG(color[3])) >> 8);
    color[2] = UBYTE((ULONG(color[2]) * ULONG(color[3])) >> 8);
}

// uc_orig: InvAlphaPremult (fallen/DDEngine/Source/polypage.cpp)
// Premultiply colour channels by inverse alpha channel.
static inline void InvAlphaPremult(UBYTE* color)
{
    color[0] = UBYTE((ULONG(color[0]) * ULONG(255 - color[3])) >> 8);
    color[1] = UBYTE((ULONG(color[1]) * ULONG(255 - color[3])) >> 8);
    color[2] = UBYTE((ULONG(color[2]) * ULONG(255 - color[3])) >> 8);
}

// uc_orig: PolyPage (fallen/DDEngine/Source/polypage.cpp)
PolyPage::PolyPage(ULONG logsize)
{
    m_VertexBuffer = NULL;
    m_VertexPtr = NULL;
    m_VBLogSize = logsize;
    m_VBUsed = 0;

    m_PolyBuffer = NULL;
    m_PolyBufSize = 1 << logsize;
    m_PolyBufUsed = 0;

    m_PolySortBuffer = NULL;
    m_PolySortBufSize = 0;

    m_UScale = 1;
    m_UOffset = 0;
    m_VScale = 1;
    m_VOffset = 0;
    pTheRealPolyPage = this;

    ASSERT(sizeof(PolyPoint2D) == sizeof(D3DTLVERTEX));
}

// uc_orig: ~PolyPage (fallen/DDEngine/Source/polypage.cpp)
PolyPage::~PolyPage()
{
    delete[] m_PolyBuffer;
    delete[] m_PolySortBuffer;

    if (m_VertexBuffer != NULL) {
        if (TheVPool != NULL) {
            TheVPool->ReleaseBuffer(m_VertexBuffer);
        }
    }
}

// uc_orig: SetTexOffset (fallen/DDEngine/Source/polypage.cpp)
void PolyPage::SetTexOffset(D3DTexture* src)
{
    src->GetTexOffsetAndScale(&m_UScale, &m_UOffset, &m_VScale, &m_VOffset);
}

// uc_orig: SetGreenScreen (fallen/DDEngine/Source/polypage.cpp)
void PolyPage::SetGreenScreen(bool enabled)
{
    s_ColourMask = enabled ? 0xFF00FF00 : 0xFFFFFFFF;
}

// uc_orig: SetScaling (fallen/DDEngine/Source/polypage.cpp)
void PolyPage::SetScaling(float xmul, float ymul)
{
    not_private_smiley_xscale = s_XScale = xmul;
    not_private_smiley_yscale = s_YScale = ymul;
}

// uc_orig: PointAlloc (fallen/DDEngine/Source/polypage.cpp)
PolyPoint2D* PolyPage::PointAlloc(ULONG num_points)
{
    if (!m_VertexBuffer) {
        ASSERT(!m_VBUsed);

        m_VertexBuffer = TheVPool->GetBuffer(m_VBLogSize);

        if (!m_VertexBuffer)
            return NULL;

        m_VertexPtr = (PolyPoint2D*)m_VertexBuffer->GetPtr();
        m_VBLogSize = m_VertexBuffer->GetLogSize();
    }

    if (m_VBUsed + num_points > GetVBSize()) {
        m_VertexBuffer = TheVPool->ExpandBuffer(m_VertexBuffer);
        m_VertexPtr = (PolyPoint2D*)m_VertexBuffer->GetPtr();
        m_VBLogSize = m_VertexBuffer->GetLogSize();

        if (m_VBUsed + num_points > GetVBSize()) {
            ASSERT(UC_FALSE);
            return NULL;
        }
    }

    PolyPoint2D* ptr = m_VertexPtr + m_VBUsed;
    m_VBUsed += num_points;

    return ptr;
}

// uc_orig: PolyBufAlloc (fallen/DDEngine/Source/polypage.cpp)
PolyPoly* PolyPage::PolyBufAlloc()
{
    if (!m_PolyBuffer) {
        ASSERT(!m_PolyBufUsed);

        m_PolyBuffer = new PolyPoly[m_PolyBufSize];
        if (!m_PolyBuffer)
            return NULL;
    }

    if (m_PolyBufUsed == m_PolyBufSize) {
        PolyPoly* newbuf = new PolyPoly[m_PolyBufSize * 2];
        if (!newbuf)
            return NULL;

        memcpy(newbuf, m_PolyBuffer, m_PolyBufSize * sizeof(PolyPoly));

        delete[] m_PolyBuffer;
        m_PolyBuffer = newbuf;
        m_PolyBufSize *= 2;
    }

    return m_PolyBuffer + (m_PolyBufUsed++);
}

// uc_orig: AddFan (fallen/DDEngine/Source/polypage.cpp)
void PolyPage::AddFan(POLY_Point** pts, ULONG num_vertices)
{
    ULONG ii;

    PolyPage* ppDrawn = pTheRealPolyPage;

    PolyPoly* pp = ppDrawn->PolyBufAlloc();
    ASSERT(pp != NULL);

    PolyPoint2D* pv = ppDrawn->PointAlloc(num_vertices);
    ASSERT(pv != NULL);

    pp->first_vertex = pv - ppDrawn->m_VertexPtr;
    pp->num_vertices = num_vertices;

    float zmax = pts[0]->Z;

    if (RS.ZLift()) {
        float zbias = float(RS.ZLift()) / 65536.0F;
        for (ii = 0; ii < num_vertices; ii++) {
            pv[ii].SetSC(pts[ii]->X * s_XScale, pts[ii]->Y * s_YScale, 1.0F - pts[ii]->Z - zbias);
            pv[ii].SetUV2(pts[ii]->u * m_UScale + m_UOffset,
                pts[ii]->v * m_VScale + m_VOffset);
            pv[ii].SetColour(pts[ii]->colour & s_ColourMask);
            pv[ii].SetSpecular(pts[ii]->specular);

            if (pts[ii]->Z > zmax)
                zmax = pts[ii]->Z;
        }

        pp->sort_z = zmax + zbias;
    } else {
        float zmax = pts[0]->Z;

        for (ii = 0; ii < num_vertices; ii++) {
            pv[ii].SetSC(pts[ii]->X * s_XScale, pts[ii]->Y * s_YScale, 1.0F - pts[ii]->Z);
            pv[ii].SetUV2(pts[ii]->u * m_UScale + m_UOffset,
                pts[ii]->v * m_VScale + m_VOffset);
            pv[ii].SetColour(pts[ii]->colour & s_ColourMask);
            pv[ii].SetSpecular(pts[ii]->specular);

            if (pts[ii]->Z > zmax)
                zmax = pts[ii]->Z;
        }

        pp->sort_z = zmax;
    }
}

// uc_orig: AddWirePoly (fallen/DDEngine/Source/polypage.cpp)
void PolyPage::AddWirePoly(POLY_Point** pts, ULONG num_vertices)
{
    // Wireframe submission not implemented on PC.
    (void)pts;
    (void)num_vertices;
}

// uc_orig: MassageVertices (fallen/DDEngine/Source/polypage.cpp)
// Apply per-vertex colour transformations controlled by the render state effect.
void PolyPage::MassageVertices()
{
    if (pTheRealPolyPage != this) {
        return;
    }

    if (RS.GetEffect()) {
        ULONG ii;
        D3DTLVERTEX* vptr = m_VertexBuffer->GetPtr();

        switch (RS.GetEffect()) {
        case RS_AlphaPremult:
            for (ii = 0; ii < m_VBUsed; ii++) {
                AlphaPremult((UBYTE*)&vptr[ii].color);
            }
            break;

        case RS_BlackWithAlpha:
            for (ii = 0; ii < m_VBUsed; ii++) {
                vptr[ii].color &= 0xFF000000;
            }
            break;

        case RS_InvAlphaPremult:
            for (ii = 0; ii < m_VBUsed; ii++) {
                InvAlphaPremult((UBYTE*)&vptr[ii].color);
            }
            break;

        case RS_DecalMode:
            for (ii = 0; ii < m_VBUsed; ii++) {
                vptr[ii].color = 0xFFFFFFFF;
            }
            break;
        }
    }
}

// uc_orig: Render (fallen/DDEngine/Source/polypage.cpp)
// Flush all buffered polygons to D3D using indexed primitive drawing.
void PolyPage::Render(IDirect3DDevice3* dev)
{
    ULONG ii;

    if (!m_VertexBuffer)
        return;

    MassageVertices();

    IDirect3DVertexBuffer* vb = TheVPool->PrepareBuffer(m_VertexBuffer);
    m_VertexBuffer = NULL;
    m_VertexPtr = NULL;

    PolyPoly* src = m_PolyBuffer;
    UWORD* dst = IxBuffer;

    for (ii = 0; ii < m_PolyBufUsed; ii++) {
        UWORD v1 = src->first_vertex;

        ASSERT(dst - IxBuffer + 3 < 65536);

        {
            for (ULONG jj = 2; jj < src->num_vertices; jj++) {
                *dst++ = v1;
                *dst++ = v1 + jj - 1;
                *dst++ = v1 + jj;
            }
        }
        src++;
    }

    dev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, vb, IxBuffer, dst - IxBuffer, D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT);

    AENG_total_polys_drawn += m_PolyBufUsed;
    m_PolyBufUsed = 0;

    m_VBUsed = 0;
}

// uc_orig: DrawSinglePoly (fallen/DDEngine/Source/polypage.cpp)
// Render a single polygon from a bucket sort pass.
void PolyPage::DrawSinglePoly(PolyPoly* poly, IDirect3DDevice3* dev)
{
    UWORD* dst = IxBuffer;

    UWORD v1 = poly->first_vertex;

    for (ULONG jj = 2; jj < poly->num_vertices; jj++) {
        *dst++ = v1;
        *dst++ = v1 + jj - 1;
        *dst++ = v1 + jj;
    }

    dev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, m_VB, IxBuffer, dst - IxBuffer, D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTCLIP | D3DDP_DONOTLIGHT);
}

// uc_orig: AddToBuckets (fallen/DDEngine/Source/polypage.cpp)
// Distribute this page's polygons into Z-depth buckets for later bucket-sorted rendering.
void PolyPage::AddToBuckets(PolyPoly* buckets[], int count)
{
    if (!m_VertexBuffer || !m_PolyBufUsed)
        return;

    MassageVertices();

    m_VB = TheVPool->PrepareBuffer(m_VertexBuffer);

    for (DWORD ii = 0; ii < m_PolyBufUsed; ii++) {
        PolyPoly* poly = &m_PolyBuffer[ii];

        int bucket = int(poly->sort_z * count);
        if (bucket < 0)
            bucket = 0;
        if (bucket >= count)
            bucket = count - 1;

        poly->page = this;
        poly->next = buckets[bucket];
        buckets[bucket] = poly;
    }

    m_VertexBuffer = NULL;
    m_VertexPtr = NULL;
    m_VBUsed = 0;
    m_PolyBufUsed = 0;
}

// uc_orig: Clear (fallen/DDEngine/Source/polypage.cpp)
void PolyPage::Clear()
{
    if (!m_VertexBuffer)
        return;

    TheVPool->ReleaseBuffer(m_VertexBuffer);
    m_VertexBuffer = NULL;
    m_VertexPtr = NULL;

    m_VBUsed = 0;
    m_PolyBufUsed = 0;
}

// uc_orig: SortBackFirst (fallen/DDEngine/Source/polypage.cpp)
// Approximate back-to-front merge sort of polygons by Z depth.
void PolyPage::SortBackFirst()
{
    if (!m_PolyBufUsed || !PolyPage::AlphaSortEnabled())
        return;

    if (m_PolySortBuffer && (m_PolySortBufSize != m_PolyBufSize)) {
        delete[] m_PolySortBuffer;
        m_PolySortBuffer = NULL;
    }

    if (!m_PolySortBuffer) {
        m_PolySortBufSize = m_PolyBufSize;
        m_PolySortBuffer = new PolyPoly[m_PolySortBufSize];

        if (!m_PolySortBuffer)
            return;
    }

    ULONG sort_len = 1;

    while (sort_len < m_PolyBufUsed) {
        MergeSortIteration(sort_len);

        PolyPoly* tmp = m_PolyBuffer;
        m_PolyBuffer = m_PolySortBuffer;
        m_PolySortBuffer = tmp;

        sort_len *= 2;
    }
}

// uc_orig: DoMerge (fallen/DDEngine/Source/polypage.cpp)
// Merge two sorted subarrays of src into dst.
template <class T>
inline static void DoMerge(const T* src, T* dst, ULONG len1, ULONG len2)
{
    ULONG pos1 = 0;
    ULONG pos2 = len1;
    ULONG wpos = 0;
    ULONG end = len1 + len2;

    for (;;) {
        if (src[pos2] < src[pos1]) {
            dst[wpos++] = src[pos2++];
            if (pos2 == end)
                break;
        } else {
            dst[wpos++] = src[pos1++];
            if (pos1 == len1)
                break;
        }
    }

    if (pos1 == len1) {
        while (pos2 < end) {
            dst[wpos++] = src[pos2++];
        }
    } else {
        while (pos1 < len1) {
            dst[wpos++] = src[pos1++];
        }
    }
}

// uc_orig: MergeSortIteration (fallen/DDEngine/Source/polypage.cpp)
void PolyPage::MergeSortIteration(ULONG sort_len)
{
    ULONG ii;
    ULONG set_len = sort_len * 2;
    ULONG limit = set_len <= m_PolyBufUsed ? m_PolyBufUsed - set_len : 0;
    PolyPoly* src = m_PolyBuffer;
    PolyPoly* dst = m_PolySortBuffer;

    if (sort_len == 1) {
        for (ii = 0; ii <= limit; ii += set_len) {
            if (src[0].sort_z > src[1].sort_z) {
                dst[0] = src[1];
                dst[1] = src[0];
            } else {
                dst[0] = src[0];
                dst[1] = src[1];
            }
            src += set_len;
            dst += set_len;
        }
        if (ii != m_PolyBufUsed) {
            dst[0] = src[0];
        }
    } else {
        for (ii = 0; ii <= limit; ii += set_len) {
            DoMerge<PolyPoly>(src, dst, sort_len, sort_len);
            src += set_len;
            dst += set_len;
        }
        if (ii != m_PolyBufUsed) {
            if (ii + sort_len >= m_PolyBufUsed) {
                while (ii < m_PolyBufUsed) {
                    *dst++ = *src++;
                    ii++;
                }
            } else {
                DoMerge<PolyPoly>(src, dst, sort_len, m_PolyBufUsed - (ii + sort_len));
            }
        }
    }
}

// uc_orig: GenerateMMMatrixFromStandardD3DOnes (fallen/DDEngine/Source/polypage.cpp)
// Builds a combined world-view-projection matrix usable by DrawIndPrimMM.
// Accounts for the letterbox rendering mode via g_dw3DStuffHeight/g_dw3DStuffY.
void GenerateMMMatrixFromStandardD3DOnes(D3DMATRIX* pmOutput,
    const D3DMATRIX* mProjectionMatrix,
    const D3DMATRIX* mWorldMatrix,
    const D3DVIEWPORT2* d3dvpt)
{
    ASSERT(((DWORD)(pmOutput) & 31) == 0);

    D3DMATRIX mMyPrivateWorld;
    if (mWorldMatrix == NULL) {
        float POLY_cam_off_x = -POLY_cam_x;
        float POLY_cam_off_y = -POLY_cam_y;
        float POLY_cam_off_z = -POLY_cam_z;

        MATRIX_MUL(
            POLY_cam_matrix,
            POLY_cam_off_x,
            POLY_cam_off_y,
            POLY_cam_off_z);

        mMyPrivateWorld._11 = POLY_cam_matrix[0];
        mMyPrivateWorld._21 = POLY_cam_matrix[1];
        mMyPrivateWorld._31 = POLY_cam_matrix[2];
        mMyPrivateWorld._41 = POLY_cam_off_x;
        mMyPrivateWorld._12 = POLY_cam_matrix[3];
        mMyPrivateWorld._22 = POLY_cam_matrix[4];
        mMyPrivateWorld._32 = POLY_cam_matrix[5];
        mMyPrivateWorld._42 = POLY_cam_off_y;
        mMyPrivateWorld._13 = POLY_cam_matrix[6];
        mMyPrivateWorld._23 = POLY_cam_matrix[7];
        mMyPrivateWorld._33 = POLY_cam_matrix[8];
        mMyPrivateWorld._43 = POLY_cam_off_z;
        mMyPrivateWorld._14 = 0.0f;
        mMyPrivateWorld._24 = 0.0f;
        mMyPrivateWorld._34 = 0.0f;
        mMyPrivateWorld._44 = 1.0f;
        mWorldMatrix = &mMyPrivateWorld;
    }

    D3DMATRIX matTemp;
    {
        matTemp._11 = mWorldMatrix->_11 * mProjectionMatrix->_11 + mWorldMatrix->_12 * mProjectionMatrix->_21 + mWorldMatrix->_13 * mProjectionMatrix->_31 + mWorldMatrix->_14 * mProjectionMatrix->_41;
        matTemp._12 = mWorldMatrix->_11 * mProjectionMatrix->_12 + mWorldMatrix->_12 * mProjectionMatrix->_22 + mWorldMatrix->_13 * mProjectionMatrix->_32 + mWorldMatrix->_14 * mProjectionMatrix->_42;
        matTemp._13 = mWorldMatrix->_11 * mProjectionMatrix->_13 + mWorldMatrix->_12 * mProjectionMatrix->_23 + mWorldMatrix->_13 * mProjectionMatrix->_33 + mWorldMatrix->_14 * mProjectionMatrix->_43;
        matTemp._14 = mWorldMatrix->_11 * mProjectionMatrix->_14 + mWorldMatrix->_12 * mProjectionMatrix->_24 + mWorldMatrix->_13 * mProjectionMatrix->_34 + mWorldMatrix->_14 * mProjectionMatrix->_44;

        matTemp._21 = mWorldMatrix->_21 * mProjectionMatrix->_11 + mWorldMatrix->_22 * mProjectionMatrix->_21 + mWorldMatrix->_23 * mProjectionMatrix->_31 + mWorldMatrix->_24 * mProjectionMatrix->_41;
        matTemp._22 = mWorldMatrix->_21 * mProjectionMatrix->_12 + mWorldMatrix->_22 * mProjectionMatrix->_22 + mWorldMatrix->_23 * mProjectionMatrix->_32 + mWorldMatrix->_24 * mProjectionMatrix->_42;
        matTemp._23 = mWorldMatrix->_21 * mProjectionMatrix->_13 + mWorldMatrix->_22 * mProjectionMatrix->_23 + mWorldMatrix->_23 * mProjectionMatrix->_33 + mWorldMatrix->_24 * mProjectionMatrix->_43;
        matTemp._24 = mWorldMatrix->_21 * mProjectionMatrix->_14 + mWorldMatrix->_22 * mProjectionMatrix->_24 + mWorldMatrix->_23 * mProjectionMatrix->_34 + mWorldMatrix->_24 * mProjectionMatrix->_44;

        matTemp._31 = mWorldMatrix->_31 * mProjectionMatrix->_11 + mWorldMatrix->_32 * mProjectionMatrix->_21 + mWorldMatrix->_33 * mProjectionMatrix->_31 + mWorldMatrix->_34 * mProjectionMatrix->_41;
        matTemp._32 = mWorldMatrix->_31 * mProjectionMatrix->_12 + mWorldMatrix->_32 * mProjectionMatrix->_22 + mWorldMatrix->_33 * mProjectionMatrix->_32 + mWorldMatrix->_34 * mProjectionMatrix->_42;
        matTemp._33 = mWorldMatrix->_31 * mProjectionMatrix->_13 + mWorldMatrix->_32 * mProjectionMatrix->_23 + mWorldMatrix->_33 * mProjectionMatrix->_33 + mWorldMatrix->_34 * mProjectionMatrix->_43;
        matTemp._34 = mWorldMatrix->_31 * mProjectionMatrix->_14 + mWorldMatrix->_32 * mProjectionMatrix->_24 + mWorldMatrix->_33 * mProjectionMatrix->_34 + mWorldMatrix->_34 * mProjectionMatrix->_44;

        matTemp._41 = mWorldMatrix->_41 * mProjectionMatrix->_11 + mWorldMatrix->_42 * mProjectionMatrix->_21 + mWorldMatrix->_43 * mProjectionMatrix->_31 + mWorldMatrix->_44 * mProjectionMatrix->_41;
        matTemp._42 = mWorldMatrix->_41 * mProjectionMatrix->_12 + mWorldMatrix->_42 * mProjectionMatrix->_22 + mWorldMatrix->_43 * mProjectionMatrix->_32 + mWorldMatrix->_44 * mProjectionMatrix->_42;
        matTemp._43 = mWorldMatrix->_41 * mProjectionMatrix->_13 + mWorldMatrix->_42 * mProjectionMatrix->_23 + mWorldMatrix->_43 * mProjectionMatrix->_33 + mWorldMatrix->_44 * mProjectionMatrix->_43;
        matTemp._44 = mWorldMatrix->_41 * mProjectionMatrix->_14 + mWorldMatrix->_42 * mProjectionMatrix->_24 + mWorldMatrix->_43 * mProjectionMatrix->_34 + mWorldMatrix->_44 * mProjectionMatrix->_44;
    }

    // Version that handles the letterbox mode height/Y offset hack.
    DWORD dwWidth = d3dvpt->dwWidth >> 1;
    DWORD dwHeight = g_dw3DStuffHeight >> 1;
    DWORD dwX = d3dvpt->dwX;
    DWORD dwY = g_dw3DStuffY;
    pmOutput->_11 = 0.0f;
    pmOutput->_12 = matTemp._11 * (float)dwWidth + matTemp._14 * (float)(dwX + dwWidth);
    pmOutput->_13 = matTemp._12 * -(float)dwHeight + matTemp._14 * (float)(dwY + dwHeight);
    pmOutput->_14 = matTemp._14;
    pmOutput->_21 = 0.0f;
    pmOutput->_22 = matTemp._21 * (float)dwWidth + matTemp._24 * (float)(dwX + dwWidth);
    pmOutput->_23 = matTemp._22 * -(float)dwHeight + matTemp._24 * (float)(dwY + dwHeight);
    pmOutput->_24 = matTemp._24;
    pmOutput->_31 = 0.0f;
    pmOutput->_32 = matTemp._31 * (float)dwWidth + matTemp._34 * (float)(dwX + dwWidth);
    pmOutput->_33 = matTemp._32 * -(float)dwHeight + matTemp._34 * (float)(dwY + dwHeight);
    pmOutput->_34 = matTemp._34;
    // Validation magic number used by DrawIndPrimMM to check matrix alignment.
    unsigned long EVal = 0xe0001000;
    pmOutput->_41 = *(float*)&EVal;
    pmOutput->_42 = matTemp._41 * (float)dwWidth + matTemp._44 * (float)(dwX + dwWidth);
    pmOutput->_43 = matTemp._42 * -(float)dwHeight + matTemp._44 * (float)(dwY + dwHeight);
    pmOutput->_44 = matTemp._44;
}

// uc_orig: DrawIndPrimMM (fallen/DDEngine/Source/polypage.cpp)
// Software emulation of the Dreamcast's DrawPrimitiveMM.
// Transforms vertices using per-vertex matrix indices, then submits as indexed triangles.
HRESULT DrawIndPrimMM(LPDIRECT3DDEVICE3 lpDevice,
    DWORD dwFVFType,
    D3DMULTIMATRIX* d3dmm,
    WORD wNumVertices,
    WORD* pwIndices,
    DWORD dwNumIndices)
{
    ASSERT(((DWORD)(d3dmm->lpd3dMatrices) & 31) == 0);
    ASSERT(((DWORD)(d3dmm->lpvVertices) & 31) == 0);
    ASSERT(((DWORD)(d3dmm->lpLightTable) & 3) == 0);
    ASSERT(((DWORD)(d3dmm->lpvLightDirs) & 7) == 0);

    ASSERT((dwFVFType == D3DFVF_LVERTEX) || (dwFVFType == D3DFVF_VERTEX));

    D3DTLVERTEX pTLVert[3];
    D3DLVERTEX* pLVert = (D3DLVERTEX*)d3dmm->lpvVertices;

    WORD* pwCurIndex = pwIndices;
    while (UC_TRUE) {
        WORD wIndex[3];
        wIndex[1] = *pwCurIndex++;
        wIndex[2] = *pwCurIndex++;
        ASSERT(dwNumIndices > 1);
        dwNumIndices -= 2;
        bool bEven = UC_TRUE;
        while (UC_TRUE) {
            bEven = !bEven;
            wIndex[0] = wIndex[1];
            wIndex[1] = wIndex[2];
            wIndex[2] = *pwCurIndex++;
            ASSERT(dwNumIndices > 0);
            dwNumIndices--;

            if (wIndex[2] == 0xffff) {
                break;
            }

            for (int i = 0; i < 3; i++) {
                WORD wVertIndex = wIndex[i];
                ASSERT(wVertIndex < wNumVertices);
                D3DLVERTEX* pLVertCur = pLVert + wVertIndex;

                BYTE bMatIndex = ((unsigned char*)(pLVertCur))[12];

                D3DMATRIX* pmCur = &(d3dmm->lpd3dMatrices[bMatIndex]);
                ASSERT(*((DWORD*)(&(pmCur->_41))) == 0xe0001000);

                pTLVert[i].dvSX = pLVertCur->dvX * pmCur->_12 + pLVertCur->dvY * pmCur->_22 + pLVertCur->dvZ * pmCur->_32 + pmCur->_42;
                pTLVert[i].dvSY = pLVertCur->dvX * pmCur->_13 + pLVertCur->dvY * pmCur->_23 + pLVertCur->dvZ * pmCur->_33 + pmCur->_43;
                pTLVert[i].dvSZ = pLVertCur->dvX * pmCur->_14 + pLVertCur->dvY * pmCur->_24 + pLVertCur->dvZ * pmCur->_34 + pmCur->_44;
                pTLVert[i].dvRHW = 1.0f / pTLVert[i].dvSZ;
                pTLVert[i].dvSX *= pTLVert[i].dvRHW;
                pTLVert[i].dvSY *= pTLVert[i].dvRHW;
                pTLVert[i].dvSZ *= (POLY_ZCLIP_PLANE);

                pTLVert[i].dvTU = pLVert[wIndex[i]].dvTU;
                pTLVert[i].dvTV = pLVert[wIndex[i]].dvTV;

                if (dwFVFType == D3DFVF_VERTEX) {
                    pTLVert[i].dcColor = 0xffffffff;
                    pTLVert[i].dcSpecular = 0xffffffff;
                } else {
                    pTLVert[i].dcColor = pLVert[wIndex[i]].dcColor;
                    pTLVert[i].dcSpecular = pLVert[wIndex[i]].dcSpecular;
                }
            }

            WORD wMyIndices[3];
            if (bEven) {
                wMyIndices[0] = 0;
                wMyIndices[1] = 2;
                wMyIndices[2] = 1;
            } else {
                wMyIndices[0] = 0;
                wMyIndices[1] = 1;
                wMyIndices[2] = 2;
            }
            HRESULT hres = lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, pTLVert, 3, wMyIndices, 3, 0);
            if (FAILED(hres)) {
                return (hres);
            }
        }
        if (dwNumIndices == 0) {
            break;
        }
    }

    return (DD_OK);
}
