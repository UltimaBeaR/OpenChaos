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

void PolyPage::push_uniform_fullscreen_ui_mode()
{
    // Uniform scale = g_frame_scale (matches the framed menus, so text/glyphs
    // are the same physical size and stay square — no distortion), offset 0
    // (top-left). The visible virtual extent is larger than 640x480 in whichever
    // axis the window is "longer": [0, g_screen_w_px/scale] x
    // [0, g_screen_h_px/scale]. Callers lay out across that range (see the
    // scrollable help screen). The +0.5 pixel-half offset is applied uniformly in
    // AddFan (PIXEL_HALF_OFFSET), as for the other fullscreen mode.
    //
    // Scissor is pinned EXPLICITLY to the whole FBO (0,0,w,h) rather than
    // "disabled": ge_disable_scissor() leaves the scissor equal to the current
    // viewport (the 3D scene viewport in the menu pass), so a full-window draw
    // should pin its own clip region instead of inheriting whatever was active.
    // Mirrors how push_ui_mode pins the scissor for its framed region.
    const float scale = ui_coords::g_frame_scale;
    const int32_t fbo_w = (int32_t)(ui_coords::g_screen_w_px + 0.5f);
    const int32_t fbo_h = (int32_t)(ui_coords::g_screen_h_px + 0.5f);
    apply_ui_state(scale, scale, 0.0f, 0.0f,
        /*scissor=*/true, 0, 0, fbo_w, fbo_h);
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

    // Multi-pass overlay (Phase C). Always set the state — including
    // mode=0 — so the next draw's overlay doesn't leak from whatever
    // page rendered last. ge_set_overlay_state is a couple of MOVs +
    // a snapshot compare in the upload path, so the cost when off
    // (the vast majority of pages) is trivial.
    ge_set_overlay_state(m_OverlayPage, m_OverlayMode);

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
    // Phase C — see PolyPage::Render() for rationale.
    ge_set_overlay_state(m_OverlayPage, m_OverlayMode);
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

