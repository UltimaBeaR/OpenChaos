#ifndef ENGINE_GRAPHICS_PIPELINE_POLYPAGE_H
#define ENGINE_GRAPHICS_PIPELINE_POLYPAGE_H

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/pipeline/polypoint.h"
#include "engine/graphics/pipeline/poly.h"


class PolyPage;

// uc_orig: PolyPoly (fallen/DDEngine/Headers/polypage.h)
// A single polygon stored inside a PolyPage. Holds its sort depth, index into
// the vertex buffer, and a linked-list pointer used during bucket sorting.
struct PolyPoly {
    float sort_z;        // Z value used for back-to-front sorting
    UWORD first_vertex;  // Index of first vertex in the PolyPage vertex buffer
    UWORD num_vertices;  // Number of vertices; if high bit set, draw as wireframe
    PolyPage* page;      // Page owning this polygon (set during bucket sort)
    PolyPoly* next;      // Next polygon in bucket linked list
};

// uc_orig: operator< (fallen/DDEngine/Headers/polypage.h)
inline bool operator<(const PolyPoly& arg1, const PolyPoly& arg2) { return arg1.sort_z < arg2.sort_z; }
// uc_orig: operator<= (fallen/DDEngine/Headers/polypage.h)
inline bool operator<=(const PolyPoly& arg1, const PolyPoly& arg2) { return arg1.sort_z <= arg2.sort_z; }
// uc_orig: operator> (fallen/DDEngine/Headers/polypage.h)
inline bool operator>(const PolyPoly& arg1, const PolyPoly& arg2) { return !(arg1 <= arg2); }
// uc_orig: operator>= (fallen/DDEngine/Headers/polypage.h)
inline bool operator>=(const PolyPoly& arg1, const PolyPoly& arg2) { return !(arg1 < arg2); }

// uc_orig: PolyPage (fallen/DDEngine/Headers/polypage.h)
// One rendering bucket per texture page. Accumulates polygons during a frame
// (AddFan / AddWirePoly), then flushes them all to D3D in a single Render() call.
// POLY_NUM_PAGES of these exist as a global array (POLY_Page[]).
#pragma pack(push, 4)
class PolyPage {
public:
    // uc_orig: PolyPage (fallen/DDEngine/Headers/polypage.h)
    PolyPage(ULONG logsize = 6);
    // uc_orig: ~PolyPage (fallen/DDEngine/Headers/polypage.h)
    ~PolyPage();

    // Pointer to the page that actually receives submitted triangles.
    // Always non-null; may point back to this page.
    // uc_orig: pTheRealPolyPage (fallen/DDEngine/Headers/polypage.h)
    PolyPage* pTheRealPolyPage;

    // uc_orig: SetTexOffset (fallen/DDEngine/Headers/polypage.h)
    void SetTexOffset(SLONG page);

    // uc_orig: AddFan (fallen/DDEngine/Headers/polypage.h)
    void AddFan(POLY_Point** pts, ULONG num_vertices);
    // uc_orig: AddWirePoly (fallen/DDEngine/Headers/polypage.h)
    void AddWirePoly(POLY_Point** pts, ULONG num_vertices);

    // uc_orig: SetGreenScreen (fallen/DDEngine/Headers/polypage.h)
    static void SetGreenScreen(bool enabled = true);
    // uc_orig: SetScaling (fallen/DDEngine/Headers/polypage.h)
    static void SetScaling(float xmul, float ymul);

    // uc_orig: SortBackFirst (fallen/DDEngine/Headers/polypage.h)
    void SortBackFirst();

    // uc_orig: EnableAlphaSort (fallen/DDEngine/Headers/polypage.h)
    static void EnableAlphaSort() { s_AlphaSort = true; }
    // uc_orig: DisableAlphaSort (fallen/DDEngine/Headers/polypage.h)
    static void DisableAlphaSort() { s_AlphaSort = false; }
    // uc_orig: AlphaSortEnabled (fallen/DDEngine/Headers/polypage.h)
    static bool AlphaSortEnabled() { return s_AlphaSort; }

    // uc_orig: NeedsRendering (fallen/DDEngine/Headers/polypage.h)
    bool NeedsRendering() { return m_PolyBufUsed > 0; }
    // uc_orig: Render (fallen/DDEngine/Headers/polypage.h)
    void Render();

    // uc_orig: AddToBuckets (fallen/DDEngine/Headers/polypage.h)
    // Original 2048-bucket sort replaced by CollectForSort + std::stable_sort
    // for precise per-polygon depth ordering.
    void CollectForSort(PolyPoly** sort_array, int& count, int max_count);
    // uc_orig: DrawSinglePoly (fallen/DDEngine/Headers/polypage.h)
    void DrawSinglePoly(PolyPoly* poly);

    // Draw multiple pre-sorted polygons in one batched draw call.
    // Used by batched alpha sort to avoid per-polygon GL overhead.
    void DrawBatchedPolys(const UWORD* indices, uint32_t index_count);

    // uc_orig: Clear (fallen/DDEngine/Headers/polypage.h)
    void Clear();

    // uc_orig: RS (fallen/DDEngine/Headers/polypage.h)
    RenderState RS;

    // uc_orig: s_AlphaSort (fallen/DDEngine/Headers/polypage.h)
    static bool s_AlphaSort;
    // uc_orig: s_ColourMask (fallen/DDEngine/Headers/polypage.h)
    static ULONG s_ColourMask;
    // uc_orig: s_XScale (fallen/DDEngine/Headers/polypage.h)
    static float s_XScale;
    // uc_orig: s_YScale (fallen/DDEngine/Headers/polypage.h)
    static float s_YScale;

    // uc_orig: PointAlloc (fallen/DDEngine/Headers/polypage.h)
    PolyPoint2D* PointAlloc(ULONG num_points);
    // FanAlloc had no implementation anywhere — dead declaration, not migrated.

    // uc_orig: m_UScale (fallen/DDEngine/Headers/polypage.h)
    float m_UScale;
    // uc_orig: m_UOffset (fallen/DDEngine/Headers/polypage.h)
    float m_UOffset;
    // uc_orig: m_VScale (fallen/DDEngine/Headers/polypage.h)
    float m_VScale;
    // uc_orig: m_VOffset (fallen/DDEngine/Headers/polypage.h)
    float m_VOffset;

    // Opaque backend vertex buffer. D3D: VertexBuffer*, OpenGL: TBD.
    // uc_orig: m_VertexBuffer (fallen/DDEngine/Headers/polypage.h)
    void* m_VertexBuffer;
    // Pointer into the locked vertex data (API-agnostic).
    // uc_orig: m_VertexPtr (fallen/DDEngine/Headers/polypage.h)
    PolyPoint2D* m_VertexPtr;
    // uc_orig: m_VBLogSize (fallen/DDEngine/Headers/polypage.h)
    ULONG m_VBLogSize;
    // uc_orig: m_VBUsed (fallen/DDEngine/Headers/polypage.h)
    ULONG m_VBUsed;
    // uc_orig: GetVBSize (fallen/DDEngine/Headers/polypage.h)
    ULONG GetVBSize() { return 1 << m_VBLogSize; }

    // uc_orig: m_PolyBuffer (fallen/DDEngine/Headers/polypage.h)
    PolyPoly* m_PolyBuffer;
    // uc_orig: m_PolyBufSize (fallen/DDEngine/Headers/polypage.h)
    ULONG m_PolyBufSize;
    // uc_orig: m_PolyBufUsed (fallen/DDEngine/Headers/polypage.h)
    ULONG m_PolyBufUsed;

    // uc_orig: m_PolySortBuffer (fallen/DDEngine/Headers/polypage.h)
    PolyPoly* m_PolySortBuffer;
    // uc_orig: m_PolySortBufSize (fallen/DDEngine/Headers/polypage.h)
    ULONG m_PolySortBufSize;

    // Opaque prepared VB handle for bucket-sort rendering.
    // D3D: IDirect3DVertexBuffer*, OpenGL: TBD.
    // uc_orig: m_VB (fallen/DDEngine/Headers/polypage.h)
    void* m_VB;

    // uc_orig: MergeSortIteration (fallen/DDEngine/Headers/polypage.h)
    void MergeSortIteration(ULONG sort_len);

    // uc_orig: PolyBufAlloc (fallen/DDEngine/Headers/polypage.h)
    PolyPoly* PolyBufAlloc();

    // uc_orig: MassageVertices (fallen/DDEngine/Headers/polypage.h)
    void MassageVertices();
};
#pragma pack(pop)

// uc_orig: POLY_Page (fallen/DDEngine/Headers/polypage.h)
// Defined in poly.cpp (not yet migrated), declared here for external use.
extern PolyPage POLY_Page[];

// uc_orig: GenerateMMMatrix (fallen/DDEngine/Headers/polypage.h)
// Builds a multi-matrix transform from standard D3D camera matrices.
// If mWorldMatrix is NULL, uses the current POLY_cam_* globals.
extern void GenerateMMMatrix(GEMatrix* mOutput,
    const GEMatrix* mProjectionMatrix,
    const GEMatrix* mWorldMatrix,
    const GEViewport* viewport);

// Camera matrix globals maintained by the poly system (defined in poly.cpp).
// uc_orig: g_matProjection (fallen/DDEngine/Headers/polypage.h)
extern GEMatrix g_matProjection;
// uc_orig: g_matWorld (fallen/DDEngine/Headers/polypage.h)
extern GEMatrix g_matWorld;
// uc_orig: g_viewData (fallen/DDEngine/Headers/polypage.h)
extern GEViewport g_viewData;

// uc_orig: GEMultiMatrix (fallen/DDEngine/Headers/polypage.h)
// Multi-matrix vertex draw block for batched character/object rendering.
struct GEMultiMatrix {
    void* lpvVertices;        // Pointer to vertex data, must be 32-byte aligned
    GEMatrix* matrices;  // Pointer to matrix array, must be 32-byte aligned
    void* lpvLightDirs;       // Pointer to light direction array (NULL if unlit), 8-byte aligned
    ULONG* lpLightTable;      // Pointer to fade table (NULL if unlit), 4-byte aligned
};

// uc_orig: SET_MM_INDEX (fallen/DDEngine/Headers/polypage.h)
// Writes the matrix index byte into byte 12 of a vertex. Must be called after
// all other vertex data is set, as byte 12 is the LSB of the N.X mantissa.
#define SET_MM_INDEX(v, i) (((unsigned char*)&v)[12] = (unsigned char)i)

// Vertex type for DrawMultiMatrix: determines whether vertex colors are used or forced to white.
enum class GEMMVertexType {
    Lit,    // GEVertexLit — use vertex dcColor/dcSpecular (D3DFVF_LVERTEX equivalent)
    Unlit,  // GEVertex — force white color (D3DFVF_VERTEX equivalent)
};

// uc_orig: DrawIndPrimMM (fallen/DDEngine/Headers/polypage.h)
// Software emulation of the DC's DrawPrimitiveMM on PC.
// CPU-side multi-matrix transform: reads per-vertex matrix index from byte 12,
// transforms to screen space, then submits via ge_draw_indexed_primitive.
void ge_draw_multi_matrix(GEMMVertexType vertex_type,
    GEMultiMatrix* mm,
    uint16_t num_vertices,
    uint16_t* indices,
    uint32_t num_indices);

// uc_orig: GET_MM_INDEX (fallen/DDEngine/Headers/polypage.h)
#define GET_MM_INDEX(v) (((unsigned char*)&v)[12])

// View-space Z of the current character, set by figure.cpp before each
// ge_draw_multi_matrix call. Used for CPU fog (0=near, 1=far), same scale
// as POLY_Point::z in POLY_fadeout_point.
extern float g_mm_fog_view_z;

#endif // ENGINE_GRAPHICS_PIPELINE_POLYPAGE_H
