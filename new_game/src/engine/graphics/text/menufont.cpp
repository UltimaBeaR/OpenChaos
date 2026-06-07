#include "engine/graphics/text/menufont.h"
#include "engine/graphics/text/menufont_globals.h"
#include "engine/graphics/text/font8x8.h" // public-domain glyph source for the alt atlas
#include "engine/graphics/graphics_engine/game_graphics_engine.h" // ge_texture_load_rgba
#include "assets/formats/tga.h"
#include "engine/graphics/pipeline/poly.h"

#include <cstring> // memcpy / memset

// Forward declaration for PANEL_draw_quad (defined in old/panel.cpp,
// will be migrated later; menufont only calls it from the fadein effect).
void PANEL_draw_quad(
    float left,
    float top,
    float right,
    float bottom,
    SLONG page,
    ULONG colour = 0xffffffff,
    float u1 = 0.0F,
    float v1 = 0.0F,
    float u2 = 1.0F,
    float v2 = 1.0F);

// Returns true if the TGA pixel at ofs is red (marker used in font layout).
// uc_orig: Red (fallen/DDEngine/Source/menufont.cpp)
static BOOL Red(SLONG ofs, TGA_Pixel* data)
{
    return ((data[ofs].red > 200) && (!data[ofs].blue) && (!data[ofs].green));
}

// Returns true if ofs is the top-left corner marker (3 adjacent red pixels).
// uc_orig: Mata (fallen/DDEngine/Source/menufont.cpp)
static BOOL Mata(SLONG ofs, TGA_Pixel* data)
{
    return (Red(ofs, data) && Red(ofs + 1, data) && (Red(ofs + 256, data)));
}

// uc_orig: TEXTURE_EXTRA_DIR (fallen/DDEngine/Source/menufont.cpp)
#define TEXTURE_EXTRA_DIR "server/textures/extras/"

// uc_orig: MENUFONT_Load (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Load(CBYTE* fn, SLONG page, CBYTE* fontlist)
{
    TGA_Pixel* temp;
    UBYTE* pt;
    UWORD x, y, ox, oy, ofs;
    CBYTE tmp[_MAX_PATH];

    if (!oc_stricmp(fn, FontName))
        return;
    strcpy(FontName, fn);
    FontPage = page;

    temp = new TGA_Pixel[256 * 256];
    ASSERT(temp != NULL);
    strcpy(tmp, TEXTURE_EXTRA_DIR);
    strcat(tmp, fn);
    TGA_load(tmp, 256, 256, temp, -1, UC_FALSE);

    pt = (UBYTE*)fontlist;

    memset(FontInfo, 0, sizeof(FontInfo));

    for (y = 0; y < 255; y++)
        for (x = 0; x < 255; x++)
            if (Mata(ofs = x | (y << 8), temp)) {
                FontInfo[*pt].x = ox = x + 1;
                FontInfo[*pt].y = oy = y + 1;
                ofs += 257;
                while (!Red(ofs + 1, temp)) {
                    ofs++;
                    ox++;
                }
                FontInfo[*pt].ox = ox;
                while (!Red(ofs + 256, temp)) {
                    ofs += 256;
                    oy++;
                }
                FontInfo[*pt].oy = oy;

                FontInfo[*pt].width = ox - (x - 1);
                FontInfo[*pt].height = oy - (y);

                FontInfo[*pt].x /= 256.0f;
                FontInfo[*pt].y /= 256.0f;
                FontInfo[*pt].ox /= 256.0f;
                FontInfo[*pt].oy /= 256.0f;
                pt++;
                if (!*pt)
                    break;
            }

    delete[] temp;

    if (!FontInfo[32].width)
        FontInfo[32].width = FontInfo[65].width;
}

// uc_orig: SC (fallen/DDEngine/Source/menufont.cpp)
#define SC(a) (SIN(a & 2047) >> 15)
// uc_orig: CC (fallen/DDEngine/Source/menufont.cpp)
#define CC(a) (COS(a & 2047) >> 15)

// uc_orig: MENUFONT_Draw (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Draw(SWORD x, SWORD y, UWORD scale, CBYTE* msg, SLONG rgb, UWORD flags, SWORD max)
{
    SLONG width = 0, height, c0, len = strlen(msg);
    UBYTE* pt;
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
    UBYTE hchar = (flags & MENUFONT_SUPER_YCTR) ? (UBYTE)*msg : 'M';
    SLONG yofs;

    if (max == -1)
        max = len;

    y -= (FontInfo[hchar].height * scale) >> 9;

    if (flags & (MENUFONT_CENTRED | MENUFONT_RIGHTALIGN)) {
        pt = (UBYTE*)msg;
        for (c0 = 0; c0 < len; c0++) {
            {
                width += ((FontInfo[*(pt++)].width - 1) * scale) >> 8;
            }
        }
        x -= (flags & MENUFONT_CENTRED) ? width >> 1 : width;
    }

    {
        SLONG a = (rgb >> 24) & 0xff;
        SLONG r = (rgb >> 16) & 0xff;
        SLONG g = (rgb >> 8) & 0xff;
        SLONG b = (rgb >> 0) & 0xff;

        r = r * a >> 8;
        g = g * a >> 8;
        b = b * a >> 8;

        rgb = (r << 16) | (g << 8) | (b << 0);
    }

    ASSERT((flags & (MENUFONT_GLIMMER | MENUFONT_SHAKE | MENUFONT_FUTZING | MENUFONT_FLANGED | MENUFONT_SINED | MENUFONT_HALOED)) == 0);

    pp[0].specular = pp[1].specular = pp[2].specular = pp[3].specular = 0xff000000;
    pp[0].colour = pp[1].colour = pp[2].colour = pp[3].colour = rgb;
    pp[0].Z = pp[1].Z = pp[2].Z = pp[3].Z = 0.5f;

    pt = (UBYTE*)msg;
    for (c0 = 0; c0 < max; c0++) {
        {
            width = (FontInfo[*pt].width * scale) >> 8;
            height = (FontInfo[*pt].height * scale) >> 8;

            yofs = (FontInfo[*pt].yofs * scale) >> 8;
            y += yofs;

            if ((width > 0) && (height > 0)) {
                pp[0].u = FontInfo[*pt].x;
                pp[0].v = FontInfo[*pt].y;
                pp[1].u = FontInfo[*pt].ox;
                pp[1].v = FontInfo[*pt].y;
                pp[2].u = FontInfo[*pt].x;
                pp[2].v = FontInfo[*pt].oy;
                pp[3].u = FontInfo[*pt].ox;
                pp[3].v = FontInfo[*pt].oy;

                if (!(flags & MENUFONT_ONLY)) {
                    pp[0].X = x;
                    pp[0].Y = y;
                    pp[1].X = x + width;
                    pp[1].Y = y;
                    pp[2].X = x;
                    pp[2].Y = y + height;
                    pp[3].X = x + width;
                    pp[3].Y = y + height;

                    POLY_add_quad(quad, FontPage, UC_FALSE, UC_TRUE);
                }
            }

            y -= yofs;

            pt++;
            x += width - 1;
        }
    }
}

// uc_orig: MENUFONT_Dimensions (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_Dimensions(CBYTE* fn, SLONG& x, SLONG& y, SWORD max, SWORD scale)
{
    UBYTE* fn2 = (UBYTE*)fn;

    if (!fn2[1]) {
        x = FontInfo[*fn2].width;
        y = FontInfo[*fn2].height;
        x *= scale;
        x >>= 8;
        y *= scale;
        y >>= 8;
        return;
    }
    x = 0;
    y = FontInfo[*fn2].height;
    while (max && *fn2) {
        {
            if (FontInfo[*fn2].height > y)
                y = FontInfo[*fn2].height;
            x += FontInfo[*fn2].width - 1;
            fn2++;
            max--;
        }
    }
    x *= scale;
    x >>= 8;
    y *= scale;
    y >>= 8;
}

// uc_orig: MENUFONT_CharWidth (fallen/DDEngine/Source/menufont.cpp)
SLONG MENUFONT_CharWidth(CBYTE fn, UWORD scale)
{
    return (FontInfo[(UBYTE)fn].width * scale) >> 8;
}

// uc_orig: MENUFONT_CharFit (fallen/DDEngine/Source/menufont.cpp)
SLONG MENUFONT_CharFit(CBYTE* fn, SLONG x, UWORD scale)
{
    SLONG ctr = 0, width = 0;
    UBYTE* fn2 = (UBYTE*)fn;

    if (!*fn)
        return 0;

    while (*fn2 && (width < x)) {
        width += ((FontInfo[*(fn2++)].width * scale) >> 8) - 1;
        ctr++;
    }
    if (width > x)
        ctr--;
    return ctr;
}

// uc_orig: MENUFONT_MergeLower (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_MergeLower()
{
    UBYTE c;

    for (c = 'a'; c <= 'z'; c++) {
        FontInfo[c] = FontInfo[c - 32];
    }
    for (c = 224; c <= 252; c++) {
        FontInfo[c] = FontInfo[c - 32];
    }
}

// =========================================================================
// License-clean ENGLISH replacement menu font (OpenChaos addition).
//
// Mirrors the FONT2D alt atlas: rasterise the embedded public-domain font8x8
// into our own texture so our always-English menu text (the help screens) keeps
// rendering correctly even when an unofficial localisation overwrites the game's
// menu font atlas (olyfont2.tga). The menu font draws through globals (FontInfo +
// FontPage) rather than a page parameter, so instead of branching on a page we
// swap those globals for the duration of the draw (MENUFONT_AltScope).
//
// Rendered ALL-CAPS (lowercase mapped to the uppercase glyph, like the game menu
// font via MENUFONT_MergeLower) so the help screens match the surrounding menus.
// =========================================================================

// The alt glyph table (indexed by char code, like FontInfo) and the POLY page
// that binds the alt atlas. Filled by MENUFONT_build_alt_atlas; consumed by
// MENUFONT_AltScope, which swaps them into the live FontInfo / FontPage.
static CharData FontInfo_alt[256];
static SLONG MENUFONT_alt_page = 0;

// Atlas is 256x256 RGBA (a quarter MB, generated once). The UVs are fractional
// so the atlas size is otherwise free.
#define MENUFONT_ALT_ATLAS_SIZE 256
// Integer scale of the 8x8 source on BOTH axes -> 24px glyphs, drawn 1:1 (crisp,
// no stretch). Rendering only the non-lowercase printable set (lowercase maps to
// uppercase) keeps ~69 glyphs, which fit a 256 atlas at this scale. The menu
// glyph size is tuned later; this is the starting size.
#define MENUFONT_ALT_SCALE 3
// On-screen / atlas glyph box (source 8 * scale).
#define MENUFONT_ALT_GLYPH_PX (FONT8X8_GLYPH_PX * MENUFONT_ALT_SCALE)
// Gap between cells so the draw-time bilinear filter can't bleed a neighbour.
#define MENUFONT_ALT_GAP 2
// Inter-letter spacing, in SOURCE font pixels scaled with the glyph (so the gap
// grows with the font like the game's menu font, which has wide letter spacing).
//
// IMPORTANT: the fade-in draw stretches each glyph's atlas region across its FULL
// advance width AND advances by that same width, so spacing can't just be added to
// the advance (that only stretches the glyph — no visible gap). Instead the spacing
// is baked into the atlas as an EMPTY strip to the RIGHT of the ink, and the glyph's
// UV covers ink + that empty strip. The ink then draws 1:1 (no stretch) followed by
// an empty (additive-black = invisible) gap. This mirrors how the game's menu font
// stores per-glyph padding inside each atlas cell.
#define MENUFONT_ALT_SPACING_SRC_PX 2
#define MENUFONT_ALT_SPACING_PX (MENUFONT_ALT_SPACING_SRC_PX * MENUFONT_ALT_SCALE)
// Atlas cell footprint. Width includes the empty spacing strip; height only needs
// the bleed-guard gap. COLS sized from the cell width.
#define MENUFONT_ALT_CELL_W (MENUFONT_ALT_GLYPH_PX + MENUFONT_ALT_SPACING_PX + MENUFONT_ALT_GAP)
#define MENUFONT_ALT_CELL_H (MENUFONT_ALT_GLYPH_PX + MENUFONT_ALT_GAP)
#define MENUFONT_ALT_COLS (MENUFONT_ALT_ATLAS_SIZE / MENUFONT_ALT_CELL_W)
// Advance width of the space character (source px, scaled). No glyph drawn.
#define MENUFONT_ALT_SPACE_SRC_PX 4
#define MENUFONT_ALT_SPACE_PX (MENUFONT_ALT_SPACE_SRC_PX * MENUFONT_ALT_SCALE)

void MENUFONT_build_alt_atlas(SLONG alt_texture_slot, SLONG alt_page)
{
    MENUFONT_alt_page = alt_page;

    const SLONG sz = MENUFONT_ALT_ATLAS_SIZE;
    // RGBA, cleared to black/transparent. Ink pixels are written opaque white; the
    // additive NEWFONT_INVERSE-style page tints them by the vertex colour/brightness.
    UBYTE* rgba = (UBYTE*)MemAlloc(sz * sz * 4);
    ASSERT(rgba != NULL);
    memset(rgba, 0, sz * sz * 4);
    memset(FontInfo_alt, 0, sizeof(FontInfo_alt));

    SLONG slot = 0;
    for (SLONG ch = FONT8X8_FIRST_CH; ch <= FONT8X8_LAST_CH; ch++) {
        // Space gets an advance but no glyph; lowercase reuses the uppercase glyph
        // (filled by the MergeLower pass below), so don't rasterise either here.
        if (ch == ' ' || (ch >= 'a' && ch <= 'z')) {
            continue;
        }

        const unsigned char* glyph = g_font8x8_basic[ch - FONT8X8_FIRST_CH];

        // Source ink column range, so the advance width stays proportional.
        SLONG lo = FONT8X8_GLYPH_PX, hi = -1;
        for (SLONG c = 0; c < FONT8X8_GLYPH_PX; c++) {
            for (SLONG r = 0; r < FONT8X8_GLYPH_PX; r++) {
                if (glyph[r] & (1u << c)) {
                    if (c < lo)
                        lo = c;
                    if (c > hi)
                        hi = c;
                    break;
                }
            }
        }
        if (hi < lo) {
            lo = 0;
            hi = 0;
        }

        const SLONG cell_x = (slot % MENUFONT_ALT_COLS) * MENUFONT_ALT_CELL_W;
        const SLONG cell_y = (slot / MENUFONT_ALT_COLS) * MENUFONT_ALT_CELL_H;
        slot++;

        // Render at the same integer scale on both axes (each source pixel -> a
        // MENUFONT_ALT_SCALE x MENUFONT_ALT_SCALE block). Every pixel square.
        for (SLONG py = 0; py < MENUFONT_ALT_GLYPH_PX; py++) {
            const SLONG sy = py / MENUFONT_ALT_SCALE;
            const SLONG ty = cell_y + py;
            if (!WITHIN(ty, 0, sz - 1))
                continue;
            for (SLONG px = 0; px < MENUFONT_ALT_GLYPH_PX; px++) {
                const SLONG sx = px / MENUFONT_ALT_SCALE;
                if (!(glyph[sy] & (1u << sx)))
                    continue;
                const SLONG tx = cell_x + px;
                if (!WITHIN(tx, 0, sz - 1))
                    continue;
                UBYTE* p = &rgba[(ty * sz + tx) * 4];
                p[0] = p[1] = p[2] = p[3] = 255; // opaque white; tinted at draw
            }
        }

        // UV spans the ink columns PLUS the empty spacing strip to their right
        // (which lies inside the cell and is never written, so it's blank). The
        // draw stretches this whole span across cd.width = ink_w + spacing, and
        // since the span IS ink_w + spacing wide the ink maps 1:1 (no stretch) and
        // the blank strip becomes the inter-letter gap. ink_x+ink_w <= GLYPH_PX, so
        // the strip stays within the cell and never reaches the next glyph.
        const SLONG ink_x = lo * MENUFONT_ALT_SCALE;
        const SLONG ink_w = (hi - lo + 1) * MENUFONT_ALT_SCALE;

        CharData& cd = FontInfo_alt[ch];
        cd.x = (float)(cell_x + ink_x) / (float)sz;
        cd.y = (float)cell_y / (float)sz;
        cd.ox = (float)(cell_x + ink_x + ink_w + MENUFONT_ALT_SPACING_PX) / (float)sz;
        cd.oy = (float)(cell_y + MENUFONT_ALT_GLYPH_PX) / (float)sz;
        cd.width = (UBYTE)(ink_w + MENUFONT_ALT_SPACING_PX);
        cd.height = (UBYTE)MENUFONT_ALT_GLYPH_PX;
        cd.xofs = 0;
        cd.yofs = 0;
    }

    // All-caps: copy each uppercase glyph entry into its lowercase slot (matches
    // the game menu font's MENUFONT_MergeLower so the look is consistent).
    for (SLONG c = 'a'; c <= 'z'; c++) {
        FontInfo_alt[c] = FontInfo_alt[c - 32];
    }

    // Space: advance only, no visible glyph (height 0 -> the draw quad collapses).
    FontInfo_alt[' '].width = (UBYTE)MENUFONT_ALT_SPACE_PX;
    FontInfo_alt[' '].height = 0;

    ge_texture_load_rgba(alt_texture_slot, sz, sz, rgba, false);
    MemFree(rgba);
}

MENUFONT_AltScope::MENUFONT_AltScope()
{
    // Save the live game menu font, then swap in the alt table + page so the
    // existing draw path renders from the replacement atlas.
    memcpy(saved_info_, FontInfo, sizeof(FontInfo));
    saved_page_ = FontPage;

    memcpy(FontInfo, FontInfo_alt, sizeof(FontInfo));
    FontPage = MENUFONT_alt_page;
}

MENUFONT_AltScope::~MENUFONT_AltScope()
{
    memcpy(FontInfo, saved_info_, sizeof(FontInfo));
    FontPage = saved_page_;
}

// =========================================================================
// Line fade-in text effect.
// =========================================================================

// uc_orig: MENUFONT_fadein_init (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_fadein_init()
{
    MENUFONT_fadein_x = 640.0F;
}

// uc_orig: MENUFONT_fadein_line (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_fadein_line(SLONG x)
{
    MENUFONT_fadein_x = float(x) * (1.0F / 256.0F);
}

// Width of the fading band to the left and right of the reveal line.
// uc_orig: MENUFONT_FADEIN_LEFT (fallen/DDEngine/Source/menufont.cpp)
#define MENUFONT_FADEIN_LEFT 256
// uc_orig: MENUFONT_FADEIN_RIGHT (fallen/DDEngine/Source/menufont.cpp)
#define MENUFONT_FADEIN_RIGHT 16

// Draws a single character relative to the fadein reveal line.
// Characters to the left are fully visible; those at the boundary animate in.
// uc_orig: MENUFONT_fadein_char (fallen/DDEngine/Source/menufont.cpp)
static float MENUFONT_fadein_char(float x, float y, UBYTE ch, UBYTE fade)
{
    CharData* cd = &FontInfo[ch];

    if (x < 0 || x + cd->width >= 640.0F || y < 0 || y + cd->height >= 480.0F) {
        return cd->width;
    }

    if (x + cd->width < MENUFONT_fadein_x - MENUFONT_FADEIN_LEFT) {
        PANEL_draw_quad(
            x, y,
            x + cd->width,
            y + cd->height,
            FontPage,
            (fade << 0) | (fade << 8) | (fade << 16) | (fade << 24),
            cd->x,
            cd->y,
            cd->ox,
            cd->oy);
    } else if (x > MENUFONT_fadein_x + MENUFONT_FADEIN_RIGHT) {
        // Totally to the right of the fadein line — don't draw.
    } else {
        // Crosses the fadein line — draw in segments with distortion.
        // uc_orig: MENUFONT_FADEIN_SEGMENTS (fallen/DDEngine/Source/menufont.cpp)
#define MENUFONT_FADEIN_SEGMENTS 16
        // uc_orig: MENUFONT_FADEIN_MAX_EXPAND (fallen/DDEngine/Source/menufont.cpp)
#define MENUFONT_FADEIN_MAX_EXPAND 16.0F

        SLONG i;

        float x1;
        float y1;
        float x2;
        float y2;

        float u1;
        float v1;
        float u2;
        float v2;

        float mx;
        float my;

        float expand;

        SLONG bright;
        SLONG colour;

        for (i = 0; i < MENUFONT_FADEIN_SEGMENTS; i++) {
            x1 = x + float(i * cd->width) * (1.0F / MENUFONT_FADEIN_SEGMENTS);
            y1 = y;
            x2 = x1 + float(cd->width) * (1.0F / MENUFONT_FADEIN_SEGMENTS);
            y2 = y1 + float(cd->height);

            u1 = cd->x + float(i * (cd->ox - cd->x)) * (1.0F / MENUFONT_FADEIN_SEGMENTS);
            v1 = cd->y;
            u2 = u1 + float(cd->ox - cd->x) * (1.0F / MENUFONT_FADEIN_SEGMENTS);
            v2 = cd->oy;

            if (x1 > MENUFONT_fadein_x) {
                expand = 1.0F - (x1 - MENUFONT_fadein_x) * (1.0F / MENUFONT_FADEIN_RIGHT);
                expand *= expand;
                expand *= expand;
                expand *= expand;
                bright = expand * 16.0F;

                expand *= MENUFONT_FADEIN_MAX_EXPAND;

                SATURATE(bright, 0, 16);
            } else {
                expand = 1.0F - (MENUFONT_fadein_x - x1) * (1.0F / MENUFONT_FADEIN_LEFT);
                bright = 255 - expand * 224;
                expand *= expand;
                expand *= expand;
                expand *= expand;

                expand *= MENUFONT_FADEIN_MAX_EXPAND;
            }

            SATURATE(expand, 0.0F, MENUFONT_FADEIN_MAX_EXPAND);

            mx = (x1 + x2) * 0.5F;
            my = (y1 + y2) * 0.5F;

            x1 = x1 + (x1 - mx) * expand * 0.5F;
            y1 = y1 + (y1 - my) * expand;
            x2 = x2 + (x2 - mx) * expand * 0.25F;
            y2 = y2 + (y2 - my) * expand;

            SATURATE(bright, 0, 255);

            colour = bright * fade >> 8;
            colour |= colour << 8;
            colour |= colour << 8;
            colour |= colour << 8;

            PANEL_draw_quad(
                x1, y1,
                x2, y2,
                FontPage,
                colour,
                u1, v1,
                u2, v2);
        }
    }

    return cd->width;
}

// uc_orig: MENUFONT_fadein_draw (fallen/DDEngine/Source/menufont.cpp)
void MENUFONT_fadein_draw(SLONG x, SLONG y, UBYTE fade, CBYTE* msg)
{
    CBYTE* ch;

    float tx;

    tx = x;

    if (msg == NULL) {
        msg = "<NULL>";
    }

    // Centre the text.
    for (ch = msg; *ch; ch++) {
        tx -= float(FontInfo[*ch].width) * 0.5F;
    }

    for (ch = msg; *ch; ch++) {
        tx += MENUFONT_fadein_char(tx, y, *ch, fade);
    }
}
