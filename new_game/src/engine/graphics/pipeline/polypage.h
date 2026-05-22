#ifndef ENGINE_GRAPHICS_PIPELINE_POLYPAGE_H
#define ENGINE_GRAPHICS_PIPELINE_POLYPAGE_H

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/pipeline/polypoint.h"
#include "engine/graphics/ui_coords.h"

class PolyPage;

// uc_orig: PolyPoly (fallen/DDEngine/Headers/polypage.h)
// A single polygon stored inside a PolyPage. Holds its sort depth, index into
// the vertex buffer, and a linked-list pointer used during bucket sorting.
struct PolyPoly {
    float sort_z; // Z value used for back-to-front sorting
    UWORD first_vertex; // Index of first vertex in the PolyPage vertex buffer
    UWORD num_vertices; // Number of vertices; if high bit set, draw as wireframe
    PolyPage* page; // Page owning this polygon (set during bucket sort)
    PolyPoly* next; // Next polygon in bucket linked list
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

    // uc_orig: SetGreenScreen (fallen/DDEngine/Headers/polypage.h)
    static void SetGreenScreen(bool enabled = true);
    // uc_orig: SetScaling (fallen/DDEngine/Headers/polypage.h)
    static void SetScaling(float xmul, float ymul);

    // UI draw mode: configures AddFan to draw existing virtual-640x480
    // call sites into a 4:3 framed region pinned at the given anchor in
    // the real screen. Snapshots s_XScale/s_YScale/s_XOffset/s_YOffset
    // and replaces them with affine values derived from ui_coords. The
    // depth counter makes push/pop pairs reentrant; only the outermost
    // pop restores the snapshot. See engine/graphics/ui_coords.h.
    static void push_ui_mode(ui_coords::UIAnchor anchor);
    // Stretch virtual 640x480 across the whole real framebuffer (non-uniform
    // scaling). Use for fullscreen UI effects (kibble particles, fade
    // transitions) that should fill the screen rather than be confined
    // to the framed UI region. Pops via the same pop_ui_mode().
    static void push_fullscreen_ui_mode();
    static void pop_ui_mode();
    // True while at least one UI scope is active. Lets backend paths that
    // bypass AddFan (e.g. fullscreen background blit) decide whether to
    // honour the framed region.
    static bool in_ui_mode();

    // RAII wrappers around the push/pop pair. Use at the top of any UI
    // render entry-point so every return path pops correctly.
    struct UIModeScope {
        UIModeScope(ui_coords::UIAnchor anchor) { PolyPage::push_ui_mode(anchor); }
        ~UIModeScope() { PolyPage::pop_ui_mode(); }
        UIModeScope(const UIModeScope&) = delete;
        UIModeScope& operator=(const UIModeScope&) = delete;
    };
    struct FullscreenUIModeScope {
        FullscreenUIModeScope() { PolyPage::push_fullscreen_ui_mode(); }
        ~FullscreenUIModeScope() { PolyPage::pop_ui_mode(); }
        FullscreenUIModeScope(const FullscreenUIModeScope&) = delete;
        FullscreenUIModeScope& operator=(const FullscreenUIModeScope&) = delete;
    };

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

    // Affine offset added after scaling in AddFan. Default 0 — used by
    // push_ui_mode to pin a virtual 640x480 draw region at a screen
    // anchor without changing per-call sites. SetScaling resets to 0.
    static float s_XOffset;
    static float s_YOffset;

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

    // Multi-pass overlay composition state for this page
    // (render_batching_plan.md Phase C — Шаг 3.2). When m_OverlayMode is
    // non-zero, every Render() / DrawBatchedPolys() call binds the
    // overlay texture (texture unit 1) and the fragment shader composes
    // it on top of the diffuse texture in a single pass — replacing the
    // legacy "goto second_page" mechanism that submitted the same polys
    // twice (once on this page, once on page+1) when POLY_PAGE_FLAG_2PASS
    // was set. Default 0 (no overlay) means the page renders identically
    // to before.
    //
    // Set up by POLY_init_render_states in poly_render.cpp from the
    // texture-metadata 'I' (SELF_ILLUM) / 'D' (WINDOW) flags.
    //
    //   m_OverlayMode = 0   no overlay (default)
    //   m_OverlayMode = 1   SELF_ILLUM (additive emissive overlay)
    //   m_OverlayMode = 2   WINDOW (alpha-blended overlay)
    //
    // m_OverlayPage stores the POLY_Page INDEX (not a GL texture id) of
    // the overlay's source texture page. The backend resolves it through
    // its own texture table at draw time — this lets the resolution
    // happen lazily (the page+1 texture may not be loaded yet when
    // POLY_init_render_states runs).
    int32_t m_OverlayPage = -1;
    uint8_t m_OverlayMode = 0;

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

    // uc_orig: PolyBufAlloc (fallen/DDEngine/Headers/polypage.h)
    PolyPoly* PolyBufAlloc();

    // uc_orig: MassageVertices (fallen/DDEngine/Headers/polypage.h)
    void MassageVertices();
};
#pragma pack(pop)

// uc_orig: POLY_Page (fallen/DDEngine/Headers/polypage.h)
// Defined in poly.cpp (not yet migrated), declared here for external use.
extern PolyPage POLY_Page[];

// Camera matrix globals maintained by the poly system (defined in poly.cpp).
// uc_orig: g_matProjection (fallen/DDEngine/Headers/polypage.h)
extern GEMatrix g_matProjection;
// uc_orig: g_matWorld (fallen/DDEngine/Headers/polypage.h)
extern GEMatrix g_matWorld;
// uc_orig: g_viewData (fallen/DDEngine/Headers/polypage.h)
extern GEViewport g_viewData;

// uc_orig: SET_MM_INDEX (fallen/DDEngine/Headers/polypage.h)
// Writes the matrix index byte into byte 12 of a vertex. Must be called after
// all other vertex data is set, as byte 12 is the LSB of the N.X mantissa.
// (Originally for the D3D MultiMatrix extension byte-12 slot; today the
// world-skin path reads the same byte for its bone index, so the
// authoring layout is unchanged.)
#define SET_MM_INDEX(v, i) (((unsigned char*)&v)[12] = (unsigned char)i)

// uc_orig: GET_MM_INDEX (fallen/DDEngine/Headers/polypage.h)
#define GET_MM_INDEX(v) (((unsigned char*)&v)[12])

// View-space Z of the current character. Set by figure.cpp before each
// ge_skin_world_draw_range call so the world-skin shader can match the
// legacy CPU fog formula (0=near, 1=far, same scale as POLY_Point::z in
// POLY_fadeout_point).
extern float g_mm_fog_view_z;

#endif // ENGINE_GRAPHICS_PIPELINE_POLYPAGE_H
