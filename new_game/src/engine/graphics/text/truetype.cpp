#include "engine/platform/uc_common.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include <mbctype.h>
#include <mbstring.h>
#include "engine/graphics/text/truetype.h"
#include "engine/graphics/text/truetype_globals.h"
#include "engine/graphics/pipeline/polypoint.h"
#include "engine/io/env.h"

// Convenience aliases matching the original names.
// These map to the renamed globals in truetype_globals.h.
#define FontHeight   tt_FontHeight
#define pShadowSurface tt_pShadowSurface
#define hFont        tt_hFont
#define hMidFont     tt_hMidFont
#define hSmallFont   tt_hSmallFont
#define hOldFont     tt_hOldFont
#define Cache        tt_Cache
#define NumCacheLines tt_NumCacheLines
#define PixMapping   tt_PixMapping
#define Commands     tt_Commands
#define AA_SIZE      TT_AA_SIZE
#define NUM_TT_PAGES TT_NUM_PAGES
#define MIN_TT_HEIGHT TT_MIN_HEIGHT
#define MAX_TT_HEIGHT TT_MAX_HEIGHT
#define MAX_LINES_PER_PAGE TT_MAX_LINES_PER_PAGE
#define MAX_CACHELINES TT_MAX_CACHELINES
#define MAX_TEXTCOMMANDS TT_MAX_TEXTCOMMANDS
#define TYPEFACE NULL

// Forward declarations for internal helpers.
// uc_orig: MeasureTextCommand (fallen/DDEngine/Source/truetype.cpp)
static void MeasureTextCommand(TextCommand* tcmd);
// uc_orig: DoTextCommand (fallen/DDEngine/Source/truetype.cpp)
static void DoTextCommand(TextCommand* tcmd);
// uc_orig: CreateTextLine (fallen/DDEngine/Source/truetype.cpp)
static void CreateTextLine(char* string, int nchars, int width, int x, int y, TextCommand* owner);
// uc_orig: NewCacheLine (fallen/DDEngine/Source/truetype.cpp)
static int NewCacheLine();
// uc_orig: CopyToCache (fallen/DDEngine/Source/truetype.cpp)
static void CopyToCache(CacheLine* cptr, UBYTE* sptr, int spitch, int width);
// uc_orig: BlitText (fallen/DDEngine/Source/truetype.cpp)
static void BlitText();
// uc_orig: TexBlit (fallen/DDEngine/Source/truetype.cpp)
static void TexBlit(int x1, int y1, int x2, int y2, int dx, int dy, ULONG rgb, ULONG scale);

// uc_orig: TT_Init (fallen/DDEngine/Source/truetype.cpp)
void TT_Init()
{
    int ii;

    if (pShadowSurface)
        return;

    FontHeight = ENV_get_value_number("FontHeight", 32, "TrueType");
    if (FontHeight < MIN_TT_HEIGHT)
        FontHeight = MIN_TT_HEIGHT;
    if (FontHeight > MAX_TT_HEIGHT)
        FontHeight = MAX_TT_HEIGHT;

    // set MBCS support
    _setmbcp(_MB_CP_LOCALE); // set locale-specific codepage

    // create the memory surface for drawing
    pShadowSurface = ge_text_surface_create(640 * AA_SIZE, FontHeight * AA_SIZE);
    ASSERT(pShadowSurface);

    //  create the font - hey, a function with fourteen parameters!
    hFont = CreateFont(FontHeight * AA_SIZE,
        0, 0, 0,
        FW_BOLD, // weight
        UC_FALSE, UC_FALSE, UC_FALSE,
        SHIFTJIS_CHARSET, // charset
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, // quality
        FF_DONTCARE | DEFAULT_PITCH,
        TYPEFACE); // typeface name

    ASSERT(hFont);

    // create the texture cache
    NumCacheLines = 0;

    for (ii = 0; ii < NUM_TT_PAGES; ii++) {
        int32_t page = TT_TEXTURE_PAGE_BASE + ii;

        // create a page
        ge_texture_create_user_page(page, 256, true);

        // create cacheline mappings
        for (int jj = 0; jj + FontHeight <= 256; jj += FontHeight) {
            Cache[NumCacheLines].owner = NULL;
            Cache[NumCacheLines].texture_page = page;
            Cache[NumCacheLines].y = jj;

            NumCacheLines++;
        }
    }

    // set up pixmap
    int32_t rr, lr, rg, lg, rb, lb, ra, la;
    ge_get_texture_pixel_format(TT_TEXTURE_PAGE_BASE, &rr, &rg, &rb, &ra, &lr, &lg, &lb, &la);

    for (ii = 0; ii < 256; ii++) {
        PixMapping[ii] = 0;
        PixMapping[ii] |= (255 >> rr) << lr;
        PixMapping[ii] |= (255 >> rg) << lg;
        PixMapping[ii] |= (255 >> rb) << lb;
        PixMapping[ii] |= (ii >> ra) << la;
    }

    // initialize the textcommands
    for (ii = 0; ii < MAX_TEXTCOMMANDS; ii++) {
        Commands[ii].validity = Free;
    }

    // initialize the DC (since it's owned, this is remembered)
    void* hDC;

    bool ok = ge_text_surface_get_dc(pShadowSurface, &hDC);
    ASSERT(ok);

    // prepare the DC
    SetBkColor((HDC)hDC, RGB(0, 0, 0));
    SetBkMode((HDC)hDC, OPAQUE);
    SetTextAlign((HDC)hDC, TA_LEFT | TA_TOP | TA_NOUPDATECP);
    SetTextCharacterExtra((HDC)hDC, ENV_get_value_number("ExtraSpaceX", 0, "TrueType"));
    SetTextColor((HDC)hDC, RGB(255, 255, 255));

    HFONT hOldFont = (HFONT)SelectObject((HDC)hDC, hFont);
    ASSERT(hOldFont);

    ge_text_surface_release_dc(pShadowSurface, hDC);
}

// uc_orig: TT_Term (fallen/DDEngine/Source/truetype.cpp)
void TT_Term()
{
    if (!pShadowSurface)
        return;

    // unprepare the DC
    void* hDC;

    bool ok = ge_text_surface_get_dc(pShadowSurface, &hDC);
    ASSERT(ok);

    SelectObject((HDC)hDC, hOldFont);

    ge_text_surface_release_dc(pShadowSurface, hDC);

    ge_text_surface_destroy(pShadowSurface);
    pShadowSurface = GE_TEXT_SURFACE_NONE;

    DeleteObject(hFont);
    hFont = NULL;

    for (int ii = 0; ii < NUM_TT_PAGES; ii++) {
        ge_texture_destroy(TT_TEXTURE_PAGE_BASE + ii);
    }
}

// uc_orig: DrawTextTT (fallen/DDEngine/Source/truetype.cpp)
int DrawTextTT(char* string, int x, int y, int rx, int scale, ULONG rgb, int command, long* width)
{
    int nbytes;

    ASSERT(rx > x);

    if (scale > 256)
        scale = 256; // no zooming up, it looks poo

    // check string length
    nbytes = strlen(string);
    ASSERT(nbytes < MAX_TT_TEXT - 1);

    // look for command
    TextCommand* tcmd = Commands;
    TextCommand* best = NULL;
    bool exact = false;

    for (int ii = 0; ii < MAX_TEXTCOMMANDS; ii++) {
        if (tcmd->validity == Pending) {
            if ((tcmd->x == x) && (tcmd->y == y) && (tcmd->rx == rx) && (tcmd->command == command) && (tcmd->nbytes == nbytes)) {
                if (!strcmp(string, tcmd->data)) {
                    // exact match
                    best = tcmd;
                    exact = true;
                    break;
                }
            }
        } else if (tcmd->validity == Free) {
            if (!best) {
                best = tcmd;
            }
        }

        tcmd++;
    }

    if (!best)
        return y; // no space

    if (!exact) {
        best->x = x;
        best->y = y;
        best->rx = rx;
        best->scale = scale;
        best->command = command;
        best->nbytes = nbytes;
        strcpy(best->data, string);
        best->nchars = _mbslen((unsigned char*)string);
        best->in_cache = false;
        MeasureTextCommand(best);
    }

    best->rgb = rgb;
    best->validity = Current;

    if (width)
        *width = best->x + best->fwidth;

    return y + best->lines * FontHeight * scale / 256;
}

// uc_orig: GetTextHeightTT (fallen/DDEngine/Source/truetype.cpp)
int GetTextHeightTT()
{
    return FontHeight;
}

// uc_orig: PreFlipTT (fallen/DDEngine/Source/truetype.cpp)
void PreFlipTT()
{
    if (!pShadowSurface)
        return; // not initialized

    int ii;
    bool work = false;

    // release all unused text commands
    for (ii = 0; ii < MAX_TEXTCOMMANDS; ii++) {
        if (Commands[ii].validity == Pending)
            Commands[ii].validity = Free;
        else if ((Commands[ii].validity == Current) && !Commands[ii].in_cache)
            work = true;
    }

    // check cachelines and release if owned by a deleted text command
    for (ii = 0; ii < NumCacheLines; ii++) {
        if (Cache[ii].owner && (Cache[ii].owner->validity == Free))
            Cache[ii].owner = NULL;
    }

    // draw text if there is any to do
    if (work) {
        TextCommand* tcmd = Commands;

        for (ii = 0; ii < MAX_TEXTCOMMANDS; ii++) {
            if ((tcmd->validity == Current) && !tcmd->in_cache) {
                DoTextCommand(tcmd);
                tcmd->in_cache = true;
            }
            tcmd++;
        }
    }

    // set all Current commands to Pending
    for (ii = 0; ii < MAX_TEXTCOMMANDS; ii++) {
        if (Commands[ii].validity == Current)
            Commands[ii].validity = Pending;
    }

    // copy to the screen
    BlitText();
}

// uc_orig: MeasureTextCommand (fallen/DDEngine/Source/truetype.cpp)
static void MeasureTextCommand(TextCommand* tcmd)
{
    char* string = tcmd->data;
    int clen = tcmd->nchars;

    void* hDC;

    bool ok = ge_text_surface_get_dc(pShadowSurface, &hDC);
    ASSERT(ok);

    tcmd->lines = 0;
    tcmd->fwidth = tcmd->rx - tcmd->x;

    while (clen) {
        SIZE size;
        int chars;

        GetTextExtentExPoint((HDC)hDC, string, clen, tcmd->fwidth * AA_SIZE, &chars, NULL, &size);

        if ((chars < clen) && (clen - chars < 3))
            chars = clen - 3;
        ASSERT(chars);
        if (!chars)
            break;

        tcmd->lines++;
        clen -= chars;
        string = (char*)_mbsninc((unsigned char*)string, chars);
    }

    ge_text_surface_release_dc(pShadowSurface, hDC);
}

// uc_orig: DoTextCommand (fallen/DDEngine/Source/truetype.cpp)
static void DoTextCommand(TextCommand* tcmd)
{
    char* string = tcmd->data;
    int clen = tcmd->nchars;
    int y = tcmd->y;

    while (clen) {
        // set up the DC so we can measure strings etc.
        void* hDC;

        bool ok = ge_text_surface_get_dc(pShadowSurface, &hDC);
        ASSERT(ok);

        // measure string
        SIZE size;
        int chars;

        GetTextExtentExPoint((HDC)hDC, string, clen, tcmd->fwidth * AA_SIZE, &chars, NULL, &size);

        // fix up for 1 or 2 characters over
        if ((chars < clen) && (clen - chars < 3))
            chars = clen - 3;

        ASSERT(chars);
        if (!chars)
            return;

        // get width
        GetTextExtentExPoint((HDC)hDC, string, chars, 0, NULL, NULL, &size);
        int width = size.cx / AA_SIZE;

        // release the DC
        ge_text_surface_release_dc(pShadowSurface, hDC);

        // draw this line
        switch (tcmd->command) {
        case LeftJustify:
            CreateTextLine(string, chars, width, tcmd->x, y, tcmd);
            break;

        case RightJustify:
            CreateTextLine(string, chars, width, tcmd->rx - width, y, tcmd);
            break;

        case Centred:
            CreateTextLine(string, chars, width, (tcmd->x + tcmd->rx - width * tcmd->scale / 256) / 2, y, tcmd);
            break;

        default:
            ASSERT(0);
            break;
        }

        y += FontHeight * tcmd->scale >> 8;
        clen -= chars;
        string = (char*)_mbsninc((unsigned char*)string, chars);
    }
}

// uc_orig: CreateTextLine (fallen/DDEngine/Source/truetype.cpp)
static void CreateTextLine(char* string, int nchars, int width, int x, int y, TextCommand* owner)
{
    ASSERT(width <= 640);

    // set up the DC
    void* hDC;

    bool ok = ge_text_surface_get_dc(pShadowSurface, &hDC);
    ASSERT(ok);

    // set up drawing rectangle
    RECT rect;

    rect.left = 0;
    rect.right = width * AA_SIZE;
    rect.top = 0;
    rect.bottom = FontHeight * AA_SIZE;

    // draw text
    BOOL res = ExtTextOut((HDC)hDC, 0, 0, ETO_OPAQUE, &rect, string, nchars, NULL);
    ASSERT(res != 0);

    ge_text_surface_release_dc(pShadowSurface, hDC);

    //
    // copy to cache
    //

    // lock the surface
    uint8_t* sptr;
    int32_t spitch;

    ok = ge_text_surface_lock(pShadowSurface, &sptr, &spitch);
    ASSERT(ok);

    // copy to cache
    int line;
    int seg = 0;

    while (width >= 0) {
        ASSERT(seg < 3);

        line = NewCacheLine();
        if (line != -1) {
            CopyToCache(&Cache[line], sptr, spitch, width);
            Cache[line].owner = owner;
            Cache[line].sx = x;
            Cache[line].sy = y;
        }

        width -= 256;
        x += owner->scale;
        seg++;
        sptr += 256 * AA_SIZE;
    }

    // unlock the surface
    ge_text_surface_unlock(pShadowSurface);
}

// uc_orig: NewCacheLine (fallen/DDEngine/Source/truetype.cpp)
static int NewCacheLine()
{
    for (int ii = 0; ii < NumCacheLines; ii++) {
        if (!Cache[ii].owner)
            return ii;
    }
    return -1;
}

// uc_orig: CopyToCache (fallen/DDEngine/Source/truetype.cpp)
static void CopyToCache(CacheLine* cptr, UBYTE* sptr, int spitch, int width)
{
    if (width > 256)
        width = 256;

    uint16_t* dptr;
    int32_t dpitch;

    bool ok = ge_lock_texture_pixels(cptr->texture_page, &dptr, &dpitch);
    if (!ok)
        return;

    dpitch /= 2;
    dptr += cptr->y * dpitch;

    for (int y = 0; y < FontHeight; y++) {
        for (int x = 0; x < width; x++) {
            int acc = sptr[0] + sptr[1] + sptr[spitch + 0] + sptr[spitch + 1];
            dptr[x] = PixMapping[acc / 4];
            sptr += 2;
        }
        sptr += (spitch - width) * 2;
        dptr += dpitch;
    }

    ge_unlock_texture_pixels(cptr->texture_page);
    cptr->width = width;
}

// uc_orig: BlitText (fallen/DDEngine/Source/truetype.cpp)
static void BlitText()
{
    int ii;
    int32_t current_page = -1;
    CacheLine* cptr = Cache;

    // go through in texture order
    for (ii = 0; ii < NumCacheLines; ii++) {
        if (cptr->owner) {
            // set render states
            if (current_page == -1) {
                ge_begin_scene();

                // Fixme! I need to be updated if this ever gets called - TomF.
                ASSERT(UC_FALSE);

                ge_set_texture_filter(GETextureFilter::Nearest, GETextureFilter::Nearest);
                ge_set_texture_blend(GETextureBlend::ModulateAlpha);
                ge_set_depth_mode(GEDepthMode::Off);
                ge_set_blend_mode(GEBlendMode::Alpha);
            }
            if (current_page != cptr->texture_page) {
                current_page = cptr->texture_page;
                ge_bind_texture(ge_get_texture_handle(current_page));
            }

            // blit area
            TexBlit(0, cptr->y, cptr->width, cptr->y + FontHeight, cptr->sx, cptr->sy, cptr->owner->rgb, cptr->owner->scale);
        }
        cptr++;
    }

    if (current_page != -1) {
        ge_end_scene();
    }
}

// uc_orig: TexBlit (fallen/DDEngine/Source/truetype.cpp)
static void TexBlit(int x1, int y1, int x2, int y2, int dx, int dy, ULONG rgb, ULONG scale)
{
    PolyPoint2D vert[4], *vp = vert;

    float u1 = float(x1) / 256;
    float u2 = float(x2) / 256;
    float v1 = float(y1) / 256;
    float v2 = float(y2) / 256;

    int width = (x2 - x1) * scale / 256;
    int height = (y2 - y1) * scale / 256;

    vp->SetSC(dx, dy);
    vp->SetColour(rgb);
    vp->SetSpecular(0xFF000000);
    vp->SetUV(u1, v1);
    vp++;

    vp->SetSC(dx + width, dy);
    vp->SetColour(rgb);
    vp->SetSpecular(0xFF000000);
    vp->SetUV(u2, v1);
    vp++;

    vp->SetSC(dx + width, dy + height);
    vp->SetColour(rgb);
    vp->SetSpecular(0xFF000000);
    vp->SetUV(u2, v2);
    vp++;

    vp->SetSC(dx, dy + height);
    vp->SetColour(rgb);
    vp->SetSpecular(0xFF000000);
    vp->SetUV(u1, v2);
    vp++;

    static WORD indices[6] = { 0, 3, 1, 1, 2, 3 };

    ge_draw_indexed_primitive(GEPrimitiveType::TriangleList,
        reinterpret_cast<const GEVertexTL*>(vert), 4, indices, 6);
}
