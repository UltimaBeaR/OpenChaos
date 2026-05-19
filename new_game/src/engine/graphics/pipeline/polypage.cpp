#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/pipeline/polypage_globals.h"
#include "engine/platform/uc_common.h"
#include "assets/texture.h"

#include <math.h>
#include <vector>

// Callback for reporting drawn poly count to game code.
static GEPolysDrawnCallback s_polys_drawn_callback = nullptr;

void ge_set_polys_drawn_callback(GEPolysDrawnCallback callback)
{
    s_polys_drawn_callback = callback;
}

// g_dw3DStuffHeight and g_dw3DStuffY are defined in poly.cpp (not yet migrated).
// Used by GenerateMMMatrix to support letterbox mode.
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
// Affine offsets — see polypage.h. Default 0; only nonzero inside push_ui_mode.
float PolyPage::s_XOffset = 0.0f;
float PolyPage::s_YOffset = 0.0f;

// uc_orig: AlphaPremult (fallen/DDEngine/Source/polypage.cpp)
// Premultiply colour channels by alpha channel.
static void AlphaPremult(UBYTE* color)
{
    color[0] = UBYTE((ULONG(color[0]) * ULONG(color[3])) >> 8);
    color[1] = UBYTE((ULONG(color[1]) * ULONG(color[3])) >> 8);
    color[2] = UBYTE((ULONG(color[2]) * ULONG(color[3])) >> 8);
}

// uc_orig: InvAlphaPremult (fallen/DDEngine/Source/polypage.cpp)
// Premultiply colour channels by inverse alpha channel.
static void InvAlphaPremult(UBYTE* color)
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

    ASSERT(sizeof(PolyPoint2D) == sizeof(GEVertexTL));
}

// uc_orig: ~PolyPage (fallen/DDEngine/Source/polypage.cpp)
PolyPage::~PolyPage()
{
    delete[] m_PolyBuffer;
    delete[] m_PolySortBuffer;

    if (m_VertexBuffer != NULL) {
        ge_vb_release(m_VertexBuffer);
    }
}

// uc_orig: SetTexOffset (fallen/DDEngine/Source/polypage.cpp)
void PolyPage::SetTexOffset(SLONG page)
{
    TEXTURE_get_tex_offset(page, &m_UScale, &m_UOffset, &m_VScale, &m_VOffset);
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
    s_XOffset = 0.0f;
    s_YOffset = 0.0f;
}

// UI mode stack — every push snapshots the current affine + scissor,
// every pop restores it. Supports nesting different modes (framed inside
// framed, fullscreen inside framed for one-off effects, etc.). 8 levels
// is more than enough; if exhausted the assert fires loudly during dev.
namespace {
struct UIState {
    float xs, ys, xo, yo;
    bool scissor_active;
    int32_t sx, sy, sw, sh;
};
constexpr int UI_STACK_MAX = 8;
UIState s_ui_stack[UI_STACK_MAX];
int s_ui_stack_top = 0;

// Mirrors what's currently programmed on the backend so push/pop can
// snapshot it. Initial state matches "no UI mode active": scissor wide
// open (the viewport scissor that ge_set_viewport already programs).
bool s_current_scissor_active = false;
int32_t s_cur_sx = 0, s_cur_sy = 0, s_cur_sw = 0, s_cur_sh = 0;

void apply_scissor(bool active, int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (active) {
        ge_set_scissor(x, y, w, h);
    } else {
        ge_disable_scissor();
    }
    s_current_scissor_active = active;
    s_cur_sx = x;
    s_cur_sy = y;
    s_cur_sw = w;
    s_cur_sh = h;
}

void apply_ui_state(float xs, float ys, float xo, float yo,
    bool scissor_active, int32_t sx, int32_t sy, int32_t sw, int32_t sh)
{
    ASSERT(s_ui_stack_top < UI_STACK_MAX);
    s_ui_stack[s_ui_stack_top++] = {
        PolyPage::s_XScale,
        PolyPage::s_YScale,
        PolyPage::s_XOffset,
        PolyPage::s_YOffset,
        s_current_scissor_active,
        s_cur_sx,
        s_cur_sy,
        s_cur_sw,
        s_cur_sh,
    };
    not_private_smiley_xscale = PolyPage::s_XScale = xs;
    not_private_smiley_yscale = PolyPage::s_YScale = ys;
    PolyPage::s_XOffset = xo;
    PolyPage::s_YOffset = yo;
    apply_scissor(scissor_active, sx, sy, sw, sh);
}
} // namespace

void PolyPage::push_ui_mode(ui_coords::UIAnchor anchor)
{
    // Affine mapping virtual 640x480 -> framed 4:3 region anchored in
    // the real framebuffer. Scissor clips draws to that region so
    // particles / overdraw don't spill onto the black bars.
    const float scale = ui_coords::g_frame_scale;
    const ui_coords::Vec2f origin01 = ui_coords::frame_origin_screen(anchor);
    const float xo = origin01.x * ui_coords::g_screen_w_px;
    const float yo = origin01.y * ui_coords::g_screen_h_px;
    const int32_t fw = int32_t(float(DisplayWidth) * scale + 0.5f);
    const int32_t fh = int32_t(float(DisplayHeight) * scale + 0.5f);
    apply_ui_state(scale, scale, xo, yo,
        /*scissor=*/true, int32_t(xo + 0.5f), int32_t(yo + 0.5f), fw, fh);
}

void PolyPage::push_fullscreen_ui_mode()
{
    // Stretch virtual 640×480 non-uniformly across the entire scene FBO.
    // Used by fullscreen effects that must cover every FBO pixel (menu
    // darken, fade-out transitions, kibble particles) — invariant I5 of
    // the FBO-as-virtual-screen refactor. The default affine from
    // POLY_camera_set is uniform (preserves character proportions) and
    // only fills the 4:3-shaped centre of the FBO; this mode is the
    // opt-in escape hatch for draws that explicitly want the whole FBO.
    // No scissor — draws are free to fill the FBO, overriding any parent
    // framed clip.
    //
    // The +0.5 pixel-half offset that used to live in xo/yo here is now
    // applied uniformly inside PolyPage::AddFan (see PIXEL_HALF_OFFSET
    // there) — adding it again would double-shift fullscreen overlays.
    const float xs = ui_coords::g_screen_w_px / float(DisplayWidth);
    const float ys = ui_coords::g_screen_h_px / float(DisplayHeight);
    apply_ui_state(xs, ys, 0.0f, 0.0f,
        /*scissor=*/false, 0, 0, 0, 0);
}

void PolyPage::pop_ui_mode()
{
    ASSERT(s_ui_stack_top > 0);
    --s_ui_stack_top;
    const UIState& s = s_ui_stack[s_ui_stack_top];
    not_private_smiley_xscale = s_XScale = s.xs;
    not_private_smiley_yscale = s_YScale = s.ys;
    s_XOffset = s.xo;
    s_YOffset = s.yo;
    apply_scissor(s.scissor_active, s.sx, s.sy, s.sw, s.sh);
}

bool PolyPage::in_ui_mode()
{
    return s_ui_stack_top > 0;
}

// uc_orig: PointAlloc (fallen/DDEngine/Source/polypage.cpp)
PolyPoint2D* PolyPage::PointAlloc(ULONG num_points)
{
    if (!m_VertexBuffer) {
        ASSERT(!m_VBUsed);

        void* ptr = NULL;
        uint32_t logsize = 0;
        m_VertexBuffer = ge_vb_alloc(m_VBLogSize, &ptr, &logsize);

        if (!m_VertexBuffer)
            return NULL;

        m_VertexPtr = (PolyPoint2D*)ptr;
        m_VBLogSize = logsize;
    }

    if (m_VBUsed + num_points > GetVBSize()) {
        void* ptr = NULL;
        uint32_t logsize = 0;
        m_VertexBuffer = ge_vb_expand(m_VertexBuffer, &ptr, &logsize);
        m_VertexPtr = (PolyPoint2D*)ptr;
        m_VBLogSize = logsize;

        if (m_VBUsed + num_points > GetVBSize()) {
            ASSERT(UC_FALSE);
            return NULL;
        }
    }

    ASSERT(m_VBUsed + num_points <= GetVBSize());

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

// uc_orig: MassageVertices (fallen/DDEngine/Source/polypage.cpp)
// Apply per-vertex colour transformations controlled by the render state effect.
void PolyPage::MassageVertices()
{
    if (pTheRealPolyPage != this) {
        return;
    }

    if (RS.GetEffect()) {
        ULONG ii;
        GEVertexTL* vptr = reinterpret_cast<GEVertexTL*>(ge_vb_get_ptr(m_VertexBuffer));

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
void PolyPage::Render()
{
    ULONG ii;

    if (!m_VertexBuffer)
        return;

    MassageVertices();

    void* prepared = ge_vb_prepare(m_VertexBuffer);
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

    ge_draw_indexed_primitive_vb(prepared, IxBuffer, dst - IxBuffer);

    if (s_polys_drawn_callback) {
        s_polys_drawn_callback(m_PolyBufUsed);
    }
    m_PolyBufUsed = 0;

    m_VBUsed = 0;
}

// Draw pre-sorted polygons in one batched call using this page's prepared VB.
void PolyPage::DrawBatchedPolys(const UWORD* indices, uint32_t index_count)
{
    ge_draw_indexed_primitive_vb(m_VB, indices, index_count);
}

// uc_orig: AddToBuckets (fallen/DDEngine/Source/polypage.cpp)
// Original 2048-bucket sort replaced by CollectForSort + std::stable_sort.
void PolyPage::CollectForSort(PolyPoly** sort_array, int& count, int max_count)
{
    if (!m_VertexBuffer || !m_PolyBufUsed)
        return;

    MassageVertices();

    m_VB = ge_vb_prepare(m_VertexBuffer);

    for (DWORD ii = 0; ii < m_PolyBufUsed; ii++) {
        if (count >= max_count)
            break;

        PolyPoly* poly = &m_PolyBuffer[ii];

        poly->page = this;
        sort_array[count++] = poly;
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

    ge_vb_release(m_VertexBuffer);
    m_VertexBuffer = NULL;
    m_VertexPtr = NULL;

    m_VBUsed = 0;
    m_PolyBufUsed = 0;
}

namespace {
// Stage-1 draw-call batching for the character/figure path.
//
// ge_draw_multi_matrix used to submit ONE draw call per triangle, so 130 MM
// calls exploded into ~4187 GL draws (×32). We now accumulate the whole MM
// call's CPU-transformed triangles and submit them in a single
// ge_draw_indexed_primitive. Vertex data and winding are byte-identical to
// the old per-triangle path — pure draw-call reduction, no visual change.
// State (texture/blend/fog) is constant across one MM call (set by the
// caller before the call), so collapsing it into one draw is
// state-equivalent. Cross-call merging (by texture) is a later stage.
//
// Render-thread singletons; std::vector capacity is retained across calls so
// steady state has no per-frame malloc/free (same proven pattern as the
// sky.cpp star batching). This accumulator is intentionally local for now;
// the later effects/UI work (Stage 3) will factor a shared batcher out.
// One triangle = 3 unique verts (transform is per-vertex via the bone
// matrix, so verts cannot be deduplicated). Flush before a triangle that
// would push the vertex count past the uint16_t index limit. 65532/3 = 21844
// triangles — far above any single body part, so in the streaming fallback
// there is in practice exactly one flush per MM call.
constexpr uint32_t MM_FLUSH_VERT_THRESHOLD = 65535 - 3;

// Skeletal-skinning accumulator (skeletal_skinning_plan.md). Stores
// MODEL-space position + normal + per-vertex bone index + uv (and, for the
// lit path, colour/specular); the GPU (skin_vert.glsl) does the transform,
// lighting and fog. One persistent mesh is built per model material (1D),
// or streamed when no cache slot is provided.
std::vector<GESkinVertex> s_skin_verts;
std::vector<uint16_t>     s_skin_inds;
const GEMatrix*           s_skin_palette = nullptr;   // = mm->matrices
uint32_t                  s_skin_palette_n = 0;       // max bone index + 1
const float*              s_skin_lightdirs = nullptr; // = mm->lpvLightDirs
const uint32_t*           s_skin_fadetable = nullptr; // = mm->lpLightTable
bool                      s_skin_unlit = false;       // character path
void**                    s_skin_cache_slot = nullptr; // = mm->skin_cache

inline void mm_flush_skin()
{
    if (s_skin_inds.empty() || !s_skin_palette || !s_skin_palette_n)
        return;
    if (s_skin_cache_slot) {
        // Milestone 1D: first time this model-material is drawn — build a
        // persistent GPU mesh from the (verified-static) geometry, store
        // it on the model, draw it. Subsequent frames hit the early-out
        // in ge_draw_multi_matrix and skip the whole accumulation loop.
        GESkinMesh* mesh = ge_skin_mesh_create(
            s_skin_verts.data(), uint32_t(s_skin_verts.size()),
            s_skin_inds.data(), uint32_t(s_skin_inds.size()),
            s_skin_palette_n);
        if (mesh) {
            *s_skin_cache_slot = mesh;
            ge_skin_mesh_draw(mesh, s_skin_palette,
                s_skin_unlit, s_skin_lightdirs, s_skin_fadetable);
        } else {
            ge_draw_skinned(s_skin_palette, s_skin_palette_n,
                s_skin_verts.data(), uint32_t(s_skin_verts.size()),
                s_skin_inds.data(), uint32_t(s_skin_inds.size()),
                s_skin_unlit, s_skin_lightdirs, s_skin_fadetable);
        }
    } else {
        ge_draw_skinned(s_skin_palette, s_skin_palette_n,
            s_skin_verts.data(), uint32_t(s_skin_verts.size()),
            s_skin_inds.data(), uint32_t(s_skin_inds.size()),
            s_skin_unlit, s_skin_lightdirs, s_skin_fadetable);
    }
    s_skin_verts.clear();
    s_skin_inds.clear();
    s_skin_palette = nullptr;
    s_skin_palette_n = 0;
    s_skin_lightdirs = nullptr;
    s_skin_fadetable = nullptr;
    s_skin_unlit = false;
    s_skin_cache_slot = nullptr;
}
} // namespace

// uc_orig: DrawIndPrimMM (fallen/DDEngine/Source/polypage.cpp)
// Software emulation of the Dreamcast's DrawPrimitiveMM. Captures the
// model-space verts + per-vertex bone index and submits them for the GPU
// skinning path (skin_vert.glsl): the GPU does transform, lighting and
// fog. One persistent mesh per model material (1D), reused every frame.
void ge_draw_multi_matrix(GEMMVertexType vertex_type,
    GEMultiMatrix* mm,
    uint16_t num_vertices,
    uint16_t* indices,
    uint32_t num_indices)
{
    ASSERT(((uintptr_t)(mm->matrices) & 31) == 0);
    ASSERT(((uintptr_t)(mm->lpvVertices) & 31) == 0);
    ASSERT(((uintptr_t)(mm->lpLightTable) & 3) == 0);
    ASSERT(((uintptr_t)(mm->lpvLightDirs) & 7) == 0);

    bool unlit = (vertex_type == GEMMVertexType::Unlit);

    // Milestone 1D fast path: if this model-material already has a
    // persistent GPU mesh, skip the entire per-vertex accumulation loop —
    // geometry is static (verified), only the bone palette / lighting
    // changes. This is where the per-frame CPU cost of characters drops.
    if (mm->skin_cache && *mm->skin_cache) {
        ge_skin_mesh_draw((GESkinMesh*)*mm->skin_cache, mm->matrices,
            unlit, (const float*)mm->lpvLightDirs,
            (const uint32_t*)mm->lpLightTable);
        return;
    }

    GESkinVertex skinTmp[3]; // model-space verts captured for the GPU path
    GEVertexLit* pLVert = (GEVertexLit*)mm->lpvVertices;
    // For unlit (character) path the buffer actually contains GEVertex with packed
    // normals; x/y/z/u/v offsets match GEVertexLit so position/UV reads are shared.
    GEVertex* pVert = (GEVertex*)mm->lpvVertices;

    uint16_t* pwCurIndex = indices;
    while (UC_TRUE) {
        uint16_t wIndex[3];
        wIndex[1] = *pwCurIndex++;
        wIndex[2] = *pwCurIndex++;
        ASSERT(num_indices > 1);
        num_indices -= 2;
        bool bEven = UC_TRUE;
        while (UC_TRUE) {
            bEven = !bEven;
            wIndex[0] = wIndex[1];
            wIndex[1] = wIndex[2];
            wIndex[2] = *pwCurIndex++;
            ASSERT(num_indices > 0);
            num_indices--;

            if (wIndex[2] == 0xffff) {
                break;
            }

            for (int i = 0; i < 3; i++) {
                uint16_t wVertIndex = wIndex[i];
                ASSERT(wVertIndex < num_vertices);
                GEVertexLit* pLVertCur = pLVert + wVertIndex;

                BYTE bMatIndex = ((unsigned char*)(pLVertCur))[12];
                // Sanity: mm->matrices[bMatIndex] must be a valid MM matrix
                // (the bone slot's _41 carries the DC MultiMatrix sentinel).
                ASSERT(*((uint32_t*)(&(mm->matrices[bMatIndex]._41))) == 0xe0001000);

                // Capture MODEL-space pos + normal + bone index; the GPU
                // (skin_vert.glsl) does transform, lighting and fog.
                skinTmp[i].x = pLVertCur->x;
                skinTmp[i].y = pLVertCur->y;
                skinTmp[i].z = pLVertCur->z;
                if (unlit) {
                    // Raw model normal — shader does the half-Lambert ramp.
                    GEVertex* pVertCur = pVert + wVertIndex;
                    skinTmp[i].nx = pVertCur->nx;
                    skinTmp[i].ny = pVertCur->ny;
                    skinTmp[i].nz = pVertCur->nz;
                } else {
                    skinTmp[i].nx = skinTmp[i].ny = skinTmp[i].nz = 0.0f;
                }
                skinTmp[i].bone = (uint32_t)bMatIndex;
                // Unlit (character) colour is derived in-shader from the
                // ramp, so the stored colour is unused there. Lit path
                // keeps the per-vertex colour. Fog → specular.a in-shader
                // (1C); a_specular carries only the original RGB (lit).
                skinTmp[i].color = unlit ? 0u : pLVert[wIndex[i]].color;
                skinTmp[i].specular = unlit
                    ? 0u
                    : (pLVert[wIndex[i]].specular & 0x00FFFFFFu);
                skinTmp[i].u = pLVert[wIndex[i]].u;
                skinTmp[i].v = pLVert[wIndex[i]].v;
                // Palette + lighting inputs = the call's arrays (constant
                // per MM call). Track the max bone index referenced.
                s_skin_palette = mm->matrices;
                s_skin_lightdirs = (const float*)mm->lpvLightDirs;
                s_skin_fadetable = (const uint32_t*)mm->lpLightTable;
                s_skin_unlit = unlit;
                s_skin_cache_slot = mm->skin_cache; // 1D: build target
                if ((uint32_t)bMatIndex + 1 > s_skin_palette_n)
                    s_skin_palette_n = (uint32_t)bMatIndex + 1;
            }

            uint16_t wMyIndices[3];
            if (bEven) {
                wMyIndices[0] = 0;
                wMyIndices[1] = 2;
                wMyIndices[2] = 1;
            } else {
                wMyIndices[0] = 0;
                wMyIndices[1] = 1;
                wMyIndices[2] = 2;
            }
            // Accumulate this triangle (same winding as the per-triangle
            // path); flushed once per MM call after the loop. Milestone 1D:
            // while building a persistent cache the WHOLE material must end
            // up in ONE mesh — the threshold flush would split it across
            // several mm_flush_skin() calls, each overwriting the cache
            // slot (missing-polygons bug). So suppress the mid-call flush
            // while building a cache; a character material is far below the
            // 16-bit index limit. Streaming fallback (no cache slot) keeps
            // the threshold.
            if (!mm->skin_cache && s_skin_verts.size() > MM_FLUSH_VERT_THRESHOLD)
                mm_flush_skin();
            ASSERT(s_skin_verts.size() < 0xFFFF); // 16-bit index base
            uint16_t base = (uint16_t)s_skin_verts.size();
            s_skin_verts.push_back(skinTmp[0]);
            s_skin_verts.push_back(skinTmp[1]);
            s_skin_verts.push_back(skinTmp[2]);
            s_skin_inds.push_back((uint16_t)(base + wMyIndices[0]));
            s_skin_inds.push_back((uint16_t)(base + wMyIndices[1]));
            s_skin_inds.push_back((uint16_t)(base + wMyIndices[2]));
        }
        if (num_indices == 0) {
            break;
        }
    }
    mm_flush_skin(); // one GPU draw per MM call (cached mesh or streamed)
}
