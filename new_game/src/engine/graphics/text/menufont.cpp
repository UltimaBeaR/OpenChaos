#include "engine/graphics/text/menufont.h"
#include "engine/graphics/text/menufont_globals.h"
#include "engine/graphics/text/font8x8.h" // public-domain glyph source for the alt atlas
#include "engine/graphics/graphics_engine/game_graphics_engine.h" // ge_texture_load_rgba
#include "assets/formats/tga.h"
#include "engine/graphics/pipeline/poly.h"

#include <cstring> // memcpy / memset
#include <cmath> // floorf (alt-atlas grunge noise)
#include <cstdint> // uint32_t (alt-atlas hash)

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

// Atlas is 512x512 RGBA (1 MB, generated once at startup, then freed from RAM —
// only the GPU texture stays). 512 (vs the FONT2D alt's 256) buys room for the
// per-glyph halo margin the weathered look needs. UVs are fractional so the size
// is otherwise free.
#define MENUFONT_ALT_ATLAS_SIZE 512
// Glyph box size in atlas px (= on-screen px, drawn 1:1). WIDTH is an integer scale
// of the 8px source (crisp). HEIGHT is an explicit pixel target (NOT an integer
// multiple of 8), so the letter height can be fine-tuned in small steps — the
// source is resampled to it — without touching the width or the streak/grunge size
// (those are in atlas px, so they stay put). The game menu letters are taller than
// wide, so GLYPH_H > GLYPH_W.
#define MENUFONT_ALT_SCALE_X 3
#define MENUFONT_ALT_GLYPH_W (FONT8X8_GLYPH_PX * MENUFONT_ALT_SCALE_X) // 24
#define MENUFONT_ALT_GLYPH_H 29 // shorter than the original 32 (was 8*4)
// Bleed-guard gap between cells (the blur must not reach a neighbour's ink).
#define MENUFONT_ALT_GAP 2
// Blank margin around each glyph (atlas px) the halo/blur bleeds into. The UV box
// is extended by this on every side so the glow shows around the whole letter. Must
// be >= MENUFONT_ALT_HALO_BLUR_R so the halo stays inside the cell.
#define MENUFONT_ALT_HALO_MARGIN 4
// Inter-letter spacing, in SOURCE font pixels scaled with the glyph (the game menu
// font has wide letter spacing). Baked into the atlas as a blank strip to the RIGHT
// of the ink (see the UV note in the builder): the fade-in draw stretches each
// glyph's UV across its full advance, so a clean gap needs blank atlas pixels, not
// just a bigger advance number.
#define MENUFONT_ALT_SPACING_SRC_PX 2
#define MENUFONT_ALT_SPACING_PX (MENUFONT_ALT_SPACING_SRC_PX * MENUFONT_ALT_SCALE_X)
// Atlas cell footprint: glyph box + halo margin on each side + right spacing strip,
// plus the bleed-guard gap. COLS sized from the cell width.
#define MENUFONT_ALT_CELL_W (MENUFONT_ALT_GLYPH_W + 2 * MENUFONT_ALT_HALO_MARGIN + MENUFONT_ALT_SPACING_PX + MENUFONT_ALT_GAP)
#define MENUFONT_ALT_CELL_H (MENUFONT_ALT_GLYPH_H + 2 * MENUFONT_ALT_HALO_MARGIN + MENUFONT_ALT_GAP)
#define MENUFONT_ALT_COLS (MENUFONT_ALT_ATLAS_SIZE / MENUFONT_ALT_CELL_W)
// Advance width of the space character (source px, scaled). No glyph drawn.
#define MENUFONT_ALT_SPACE_SRC_PX 4
#define MENUFONT_ALT_SPACE_PX (MENUFONT_ALT_SPACE_SRC_PX * MENUFONT_ALT_SCALE_X)

// --- Baked weathered-look effect tuning (all applied once, into the atlas) ---
// The game menu font is soft, slightly glowing and grungy (irregular bright/dark
// horizontal streaks). We approximate that on the hard font8x8 mask so our text
// sits next to it. All of this is TUNABLE — these are starting values.
//
// Edge softening (anti-alias): small blur of the mask, then a gain so stroke cores
// stay solid while edges feather.
#define MENUFONT_ALT_CORE_BLUR_R 1
#define MENUFONT_ALT_CORE_GAIN 2.9f // higher = sharper edges (less feather)
// Halo / glow: a wider, fainter blur added around the letter (the "ареол").
#define MENUFONT_ALT_HALO_BLUR_R 3
#define MENUFONT_ALT_HALO_STRENGTH 0.75f
// Grunge streaks: value-noise stretched wide-in-X / thin-in-Y makes broad horizontal
// bands across the letters. Two octaves (broad + fine). Positive noise BRIGHTENS
// (toward white); negative noise CARVES the letter away to full transparency (holes),
// like the weathered original — see the carve ramp in the post-process loop.
#define MENUFONT_ALT_GRUNGE_BRIGHT 0.9f // bright-streak strength
// Dark streaks punch holes: negative-noise magnitude below CARVE_START does nothing,
// at/above CARVE_FULL it cuts a FULL hole (v -> 0), ramping between. The window is
// kept high so only strong bands carve, giving occasional holes (not a dim wash).
#define MENUFONT_ALT_CARVE_START 0.28f
#define MENUFONT_ALT_CARVE_FULL 0.60f
#define MENUFONT_ALT_STREAK_LEN1 5.8f   // horizontal band length (px) — short & frequent
#define MENUFONT_ALT_STREAK_THICK1 1.15f // vertical band thickness (px) — thin & frequent
#define MENUFONT_ALT_STREAK_LEN2 2.3f   // finer octave
#define MENUFONT_ALT_STREAK_THICK2 0.6f
// Blur applied ONLY to the grunge (streak/hole) modulation — NOT the letter. The
// streaks and carved holes get soft edges while the LETTER shape stays as crisp as
// the core blur makes it. (Letter-edge softness is CORE_BLUR_R; this is independent.)
// MIX blends the blurred grunge back toward the sharp grunge (0 = no streak blur,
// 1 = full box-blur) — lets the streak softening go below one whole pixel.
#define MENUFONT_ALT_STREAK_BLUR_R 1
#define MENUFONT_ALT_STREAK_BLUR_MIX 0.5f
#define MENUFONT_ALT_GRUNGE_W1 0.65f // broad-octave weight
#define MENUFONT_ALT_GRUNGE_W2 0.35f // fine-octave weight
// Hardcoded seed for the grunge pattern. The streaks come from a pure positional
// hash (NOT rand()/any global RNG), so the atlas is already identical every launch
// and fully independent of any randomness the game uses elsewhere. This seed is
// mixed into that hash purely to PIN the pattern explicitly and to let us pick a
// different fixed pattern by changing one number — never tie it to a runtime/random
// value or the fonts would change between runs.
#define MENUFONT_ALT_GRUNGE_SEED 0x9E3779B9u

// Deterministic 32-bit integer hash -> [0,1]. Pure function of (xi, yi) + the fixed
// seed, so the atlas is byte-identical every run (the streaks never re-randomise).
static float menufont_alt_hash01(SLONG xi, SLONG yi)
{
    uint32_t n = (uint32_t)xi * 374761393u + (uint32_t)yi * 668265263u + MENUFONT_ALT_GRUNGE_SEED;
    n = (n ^ (n >> 13)) * 1274126177u;
    n = n ^ (n >> 16);
    return (float)(n & 0x7fffffffu) / (float)0x7fffffffu;
}

// Smooth (smoothstep-interpolated) 2D value noise in [0,1] over the hash lattice.
static float menufont_alt_vnoise(float x, float y)
{
    const SLONG xi = (SLONG)floorf(x);
    const SLONG yi = (SLONG)floorf(y);
    float fx = x - (float)xi;
    float fy = y - (float)yi;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    const float a = menufont_alt_hash01(xi, yi);
    const float b = menufont_alt_hash01(xi + 1, yi);
    const float c = menufont_alt_hash01(xi, yi + 1);
    const float d = menufont_alt_hash01(xi + 1, yi + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

// Separable box blur of a float buffer (src -> dst), using tmp as scratch. Edges
// clamp. radius<=0 copies through.
static void menufont_alt_box_blur(const float* src, float* dst, float* tmp, SLONG sz, SLONG radius)
{
    if (radius <= 0) {
        memcpy(dst, src, (size_t)sz * sz * sizeof(float));
        return;
    }
    const float inv = 1.0f / (float)(2 * radius + 1);
    for (SLONG y = 0; y < sz; y++) {
        const float* srow = src + (size_t)y * sz;
        float* trow = tmp + (size_t)y * sz;
        for (SLONG x = 0; x < sz; x++) {
            float acc = 0.0f;
            for (SLONG k = -radius; k <= radius; k++) {
                SLONG xx = x + k;
                if (xx < 0)
                    xx = 0;
                else if (xx >= sz)
                    xx = sz - 1;
                acc += srow[xx];
            }
            trow[x] = acc * inv;
        }
    }
    for (SLONG y = 0; y < sz; y++) {
        for (SLONG x = 0; x < sz; x++) {
            float acc = 0.0f;
            for (SLONG k = -radius; k <= radius; k++) {
                SLONG yy = y + k;
                if (yy < 0)
                    yy = 0;
                else if (yy >= sz)
                    yy = sz - 1;
                acc += tmp[(size_t)yy * sz + x];
            }
            dst[(size_t)y * sz + x] = acc * inv;
        }
    }
}

void MENUFONT_build_alt_atlas(SLONG alt_texture_slot, SLONG alt_page)
{
    MENUFONT_alt_page = alt_page;

    const SLONG sz = MENUFONT_ALT_ATLAS_SIZE;
    const size_t npix = (size_t)sz * sz;

    // Work in a float intensity buffer (the hard glyph mask), then post-process it
    // (blur -> halo -> grunge) into the final RGBA. The page is additive with a
    // Modulate texture blend, so intensity lives in RGB (tinted by the vertex
    // colour at draw time); alpha mirrors it for completeness.
    float* mask = (float*)MemAlloc(npix * sizeof(float));
    float* core = (float*)MemAlloc(npix * sizeof(float));
    float* halo = (float*)MemAlloc(npix * sizeof(float));
    float* tmp = (float*)MemAlloc(npix * sizeof(float));
    UBYTE* rgba = (UBYTE*)MemAlloc(npix * 4);
    ASSERT(mask && core && halo && tmp && rgba);
    memset(mask, 0, npix * sizeof(float));
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

        // Glyph box origin inside the cell — offset by the halo margin so there is
        // blank room on every side for the blur/halo to bleed into.
        const SLONG gx0 = cell_x + MENUFONT_ALT_HALO_MARGIN;
        const SLONG gy0 = cell_y + MENUFONT_ALT_HALO_MARGIN;

        // Rasterise the hard mask. Width: source col -> SCALE_X px (crisp). Height:
        // resample the 8 source rows into GLYPH_H px (nearest; the weathering blur
        // hides the slightly uneven row heights from the non-integer ratio).
        for (SLONG py = 0; py < MENUFONT_ALT_GLYPH_H; py++) {
            const SLONG sy = py * FONT8X8_GLYPH_PX / MENUFONT_ALT_GLYPH_H;
            const SLONG ty = gy0 + py;
            if (!WITHIN(ty, 0, sz - 1))
                continue;
            for (SLONG px = 0; px < MENUFONT_ALT_GLYPH_W; px++) {
                const SLONG sx = px / MENUFONT_ALT_SCALE_X;
                if (!(glyph[sy] & (1u << sx)))
                    continue;
                const SLONG tx = gx0 + px;
                if (!WITHIN(tx, 0, sz - 1))
                    continue;
                mask[(size_t)ty * sz + tx] = 1.0f;
            }
        }

        // UV box = ink + halo margin on every side + the right-hand spacing strip.
        // Horizontally it starts one halo margin LEFT of the ink (so the left glow
        // shows) and ends one halo margin + the spacing PAST the ink; the drawn quad
        // (cd.width/height) matches this box 1:1, so nothing stretches. ink_x+ink_w
        // <= GLYPH_W keeps the box inside the cell (GAP guards the next glyph).
        const SLONG ink_x = lo * MENUFONT_ALT_SCALE_X;
        const SLONG ink_w = (hi - lo + 1) * MENUFONT_ALT_SCALE_X;

        CharData& cd = FontInfo_alt[ch];
        cd.x = (float)(gx0 + ink_x - MENUFONT_ALT_HALO_MARGIN) / (float)sz;
        cd.y = (float)(gy0 - MENUFONT_ALT_HALO_MARGIN) / (float)sz;
        cd.ox = (float)(gx0 + ink_x + ink_w + MENUFONT_ALT_HALO_MARGIN + MENUFONT_ALT_SPACING_PX) / (float)sz;
        cd.oy = (float)(gy0 + MENUFONT_ALT_GLYPH_H + MENUFONT_ALT_HALO_MARGIN) / (float)sz;
        cd.width = (UBYTE)(ink_w + 2 * MENUFONT_ALT_HALO_MARGIN + MENUFONT_ALT_SPACING_PX);
        cd.height = (UBYTE)(MENUFONT_ALT_GLYPH_H + 2 * MENUFONT_ALT_HALO_MARGIN);
        cd.xofs = 0;
        cd.yofs = 0;
    }

    // Post-process into the final intensity. The LETTER stays crisp; only the grunge
    // (streaks/holes) is blurred:
    //  - core: small blur + gain   -> soft anti-aliased edges, solid stroke cores
    //  - halo: wider faint blur     -> the glow/areola around letters
    //  -> these two make the SHARP letter intensity (mask).
    //  - grunge: short value-noise bands -> a per-pixel multiplier (core).
    //  - blur ONLY that multiplier (halo) -> soft streak/hole edges.
    //  - final = sharp letter * blurred grunge.
    menufont_alt_box_blur(mask, core, tmp, sz, MENUFONT_ALT_CORE_BLUR_R);
    menufont_alt_box_blur(mask, halo, tmp, sz, MENUFONT_ALT_HALO_BLUR_R);

    // Sharp letter intensity (core over halo) -> mask (hard mask no longer needed).
    for (size_t i = 0; i < npix; i++) {
        float c = core[i] * MENUFONT_ALT_CORE_GAIN;
        if (c > 1.0f)
            c = 1.0f;
        const float h = halo[i] * MENUFONT_ALT_HALO_STRENGTH;
        mask[i] = (c > h) ? c : h;
    }

    // Grunge multiplier per pixel -> core (reused). >1 brightens, ->0 carves a hole.
    for (SLONG y = 0; y < sz; y++) {
        for (SLONG x = 0; x < sz; x++) {
            const float g1 = menufont_alt_vnoise(
                                 (float)x / MENUFONT_ALT_STREAK_LEN1,
                                 (float)y / MENUFONT_ALT_STREAK_THICK1)
                - 0.5f;
            const float g2 = menufont_alt_vnoise(
                                 (float)x / MENUFONT_ALT_STREAK_LEN2 + 100.0f,
                                 (float)y / MENUFONT_ALT_STREAK_THICK2 + 50.0f)
                - 0.5f;
            const float g = 2.0f * (g1 * MENUFONT_ALT_GRUNGE_W1 + g2 * MENUFONT_ALT_GRUNGE_W2);
            float m;
            if (g > 0.0f) {
                // Bright band: push toward white.
                m = 1.0f + g * MENUFONT_ALT_GRUNGE_BRIGHT;
            } else {
                // Dark band: carve toward a full hole once strong enough.
                float t = (-g - MENUFONT_ALT_CARVE_START)
                    / (MENUFONT_ALT_CARVE_FULL - MENUFONT_ALT_CARVE_START);
                if (t < 0.0f)
                    t = 0.0f;
                else if (t > 1.0f)
                    t = 1.0f;
                m = 1.0f - t;
            }
            core[(size_t)y * sz + x] = m;
        }
    }

    // Blur ONLY the grunge multiplier (core -> halo), then blend back toward the
    // sharp grunge by MIX (so the streak softening can be less than one pixel).
    menufont_alt_box_blur(core, halo, tmp, sz, MENUFONT_ALT_STREAK_BLUR_R);
    for (size_t i = 0; i < npix; i++) {
        halo[i] = core[i] + (halo[i] - core[i]) * MENUFONT_ALT_STREAK_BLUR_MIX;
    }

    // Combine: crisp letter * soft grunge.
    for (size_t i = 0; i < npix; i++) {
        float v = mask[i] * halo[i];
        if (v > 1.0f)
            v = 1.0f;
        const UBYTE b = (UBYTE)(v * 255.0f + 0.5f);
        UBYTE* p = &rgba[i * 4];
        p[0] = p[1] = p[2] = p[3] = b; // intensity; tinted by the vertex colour
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
    MemFree(tmp);
    MemFree(halo);
    MemFree(core);
    MemFree(mask);
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
