#include "engine/graphics/text/font_atlas.h"
#include "engine/graphics/text/font.h"
#include "engine/graphics/text/font_globals.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/platform/uc_common.h"

#include <stdlib.h>

// -- Atlas layout ------------------------------------------------------------
//
// Uppercase A-Z (0..25), lowercase a-z (26..51), digits 0-9 (52..61),
// punct[0..FONT_PUNCT_NUMBER-1] (62..86). 87 glyphs total.
//
// Each glyph occupies a CELL_W × CELL_H cell (pow-of-two sizing keeps UV math
// trivial and leaves 3 texels horizontal + CELL_H-FONT_HEIGHT texels vertical
// slack as zero-padding — nearest-filtered sampling never bleeds across cells
// because the quad's UVs span only the FONT_WIDTH × FONT_HEIGHT populated
// region. ATLAS_H is not strictly PoT but GL 4.1 has no NPOT restrictions.

#define CELL_W            8
#define CELL_H            16
#define ATLAS_COLS        16
#define ATLAS_ROWS        6     // ceil((26+26+10+FONT_PUNCT_NUMBER) / ATLAS_COLS) = ceil(87/16)
#define ATLAS_W           (CELL_W  * ATLAS_COLS)   // 128
#define ATLAS_H           (CELL_H  * ATLAS_ROWS)   // 96

// Index into a flat glyph array where [0..25] = upper, [26..51] = lower,
// [52..61] = digit, [62..86] = punct.
static int32_t s_ch_to_index(CBYTE ch)
{
    if (ch >= 'A' && ch <= 'Z') return      0 + (ch - 'A');
    if (ch >= 'a' && ch <= 'z') return     26 + (ch - 'a');
    if (ch >= '0' && ch <= '9') return     52 + (ch - '0');

    // Punctuation dispatch mirrors FONT_draw_coloured_char exactly. A mismatch
    // here would render a different bitmap than the pre-atlas path.
    int32_t p;
    switch (ch) {
    case '.':   p = FONT_PUNCT_DOT;    break;
    case ',':   p = FONT_PUNCT_COMMA;  break;
    case '?':   p = FONT_PUNCT_QMARK;  break;
    case '!':   p = FONT_PUNCT_PLING;  break;
    case '"':   p = FONT_PUNCT_QUOTES; break;
    case '(':   p = FONT_PUNCT_OPEN;   break;
    case ')':   p = FONT_PUNCT_CLOSE;  break;
    case '+':   p = FONT_PUNCT_PLUS;   break;
    case '-':   p = FONT_PUNCT_MINUS;  break;
    case '=':   p = FONT_PUNCT_EQUAL;  break;
    case '#':   p = FONT_PUNCT_HASH;   break;
    case '%':   p = FONT_PUNCT_PCENT;  break;
    case '*':   p = FONT_PUNCT_STAR;   break;
    case '\\':  p = FONT_PUNCT_BSLASH; break;
    case '/':   p = FONT_PUNCT_FSLASH; break;
    case ':':   p = FONT_PUNCT_COLON;  break;
    case ';':   p = FONT_PUNCT_SCOLON; break;
    case '\'':  p = FONT_PUNCT_APOST;  break;
    case '&':   p = FONT_PUNCT_AMPER;  break;
    case '\xa3': p = FONT_PUNCT_POUND; break;
    case '$':   p = FONT_PUNCT_DOLLAR; break;
    case '<':   p = FONT_PUNCT_LT;     break;
    case '>':   p = FONT_PUNCT_GT;     break;
    case '@':   p = FONT_PUNCT_AT;     break;
    case '_':   p = FONT_PUNCT_UNDER;  break;
    default:    p = FONT_PUNCT_QMARK;  break;
    }
    return 62 + p;
}

static GETextureHandle s_atlas = GE_TEXTURE_NONE;

// Bake FONT_upper/lower/number/punct bitmaps into ATLAS_W × ATLAS_H R8 buffer.
// Each row of a FONT_Char bitmap is a 5-bit mask with MSB (0x10) leftmost.
// We write 0xFF for set bits, 0x00 for clear — the shader samples as RRRR and
// the existing color-key path discards zeros.
static void s_paste_glyph(uint8_t* atlas, int32_t atlas_index, const FONT_Char* fc)
{
    int32_t col = atlas_index % ATLAS_COLS;
    int32_t row = atlas_index / ATLAS_COLS;
    int32_t x0  = col * CELL_W;
    int32_t y0  = row * CELL_H;

    for (int32_t y = 0; y < FONT_HEIGHT; ++y) {
        uint8_t row_bits = fc->bit[y];
        uint8_t* dst = atlas + (y0 + y) * ATLAS_W + x0;
        // Bits run MSB (0x10) → LSB (0x01), 5 columns total.
        dst[0] = (row_bits & 0x10) ? 0xFF : 0x00;
        dst[1] = (row_bits & 0x08) ? 0xFF : 0x00;
        dst[2] = (row_bits & 0x04) ? 0xFF : 0x00;
        dst[3] = (row_bits & 0x02) ? 0xFF : 0x00;
        dst[4] = (row_bits & 0x01) ? 0xFF : 0x00;
    }
}

void FONT_atlas_ensure()
{
    if (s_atlas != GE_TEXTURE_NONE) return;

    uint8_t* atlas = (uint8_t*)calloc(1, ATLAS_W * ATLAS_H);
    if (!atlas) return;

    for (int32_t i = 0; i < 26; ++i) s_paste_glyph(atlas,       i, &FONT_upper[i]);
    for (int32_t i = 0; i < 26; ++i) s_paste_glyph(atlas,  26 + i, &FONT_lower[i]);
    for (int32_t i = 0; i < 10; ++i) s_paste_glyph(atlas,  52 + i, &FONT_number[i]);
    for (int32_t i = 0; i < FONT_PUNCT_NUMBER; ++i)
        s_paste_glyph(atlas, 62 + i, &FONT_punct[i]);

    s_atlas = ge_create_user_texture_r8_rrrr(ATLAS_W, ATLAS_H, atlas);
    free(atlas);
}

void FONT_atlas_draw_glyph(SLONG sx, SLONG sy, UBYTE r, UBYTE g, UBYTE b, CBYTE ch)
{
    if (ch == ' ') return;

    FONT_atlas_ensure();
    if (s_atlas == GE_TEXTURE_NONE) return;

    int32_t idx = s_ch_to_index(ch);
    int32_t col = idx % ATLAS_COLS;
    int32_t row = idx / ATLAS_COLS;

    // Glyphs are 5 columns wide (the legacy plot loop writes bits 0x10..0x01).
    // Screen quad and UV span only the populated 5 × FONT_HEIGHT region; the
    // rest of the cell is zero padding that stays untouched.
    const int32_t GLYPH_COLS = 5;
    const float inv_w = 1.0f / (float)ATLAS_W;
    const float inv_h = 1.0f / (float)ATLAS_H;
    float u0 = (col * CELL_W)               * inv_w;
    float v0 = (row * CELL_H)               * inv_h;
    float u1 = (col * CELL_W + GLYPH_COLS)  * inv_w;
    float v1 = (row * CELL_H + FONT_HEIGHT) * inv_h;

    // D3D6 color format expected by tl_vert.glsl: bytes in memory are B, G, R, A.
    uint32_t color = ((uint32_t)0xFF << 24)
                   | ((uint32_t)r    << 16)
                   | ((uint32_t)g    <<  8)
                   | ((uint32_t)b    <<  0);

    // Quad spans 5 × FONT_HEIGHT screen pixels starting at (sx, sy) — same
    // footprint as the legacy per-pixel plot loop.
    //
    // tl_vert.glsl subtracts 0.5 from every vertex (D3D6 integer-coord →
    // OpenGL half-integer pixel-center mapping). Vertices passed as plain
    // integers land on *pixel centers* in GL space, which puts the quad
    // edges through the centers of the boundary pixels — rasterizer partial
    // coverage + UV interpolation at half-texels produces the blurry look.
    // Shift by +0.5 so the quad edges land on pixel boundaries instead; each
    // screen pixel then maps to exactly one atlas texel. Same trick as the
    // fullscreen blit in ge_unlock_screen.
    const float ox = 0.5f, oy = 0.5f;
    const float x0f = (float)sx                + ox;
    const float y0f = (float)sy                + oy;
    const float x1f = (float)(sx + GLYPH_COLS) + ox;
    const float y1f = (float)(sy + FONT_HEIGHT) + oy;
    const float z   = 0.0f;
    const float rhw = 1.0f;

    GEVertexTL verts[4];
    verts[0].x = x0f; verts[0].y = y0f; verts[0].z = z; verts[0].rhw = rhw;
    verts[0].color = color; verts[0].specular = 0xFF000000;
    verts[0].u = u0; verts[0].v = v0;

    verts[1].x = x1f; verts[1].y = y0f; verts[1].z = z; verts[1].rhw = rhw;
    verts[1].color = color; verts[1].specular = 0xFF000000;
    verts[1].u = u1; verts[1].v = v0;

    verts[2].x = x0f; verts[2].y = y1f; verts[2].z = z; verts[2].rhw = rhw;
    verts[2].color = color; verts[2].specular = 0xFF000000;
    verts[2].u = u0; verts[2].v = v1;

    verts[3].x = x1f; verts[3].y = y1f; verts[3].z = z; verts[3].rhw = rhw;
    verts[3].color = color; verts[3].specular = 0xFF000000;
    verts[3].u = u1; verts[3].v = v1;

    uint16_t indices[6] = { 0, 1, 2, 2, 1, 3 };

    // 2D overlay config: no depth, no blend, color-key discards the zero
    // texels between glyph bits. Modulate so vertex color tints the mask.
    // No push/pop here — state save/restore is handled once per batch by
    // FONT_atlas_begin_batch / FONT_atlas_end_batch, so drawing N glyphs in a
    // row only pays the GL state-query cost once.
    ge_set_depth_mode(GEDepthMode::Off);
    ge_set_blend_enabled(false);
    ge_set_color_key_enabled(true);
    ge_set_alpha_test_enabled(false);
    ge_set_specular_enabled(false);
    ge_set_fog_enabled(false);
    ge_set_texture_blend(GETextureBlend::Modulate);
    ge_bind_texture(s_atlas);
    ge_set_bound_texture_has_alpha(false);

    ge_draw_indexed_primitive(GEPrimitiveType::TriangleList, verts, 4, indices, 6);
}

void FONT_atlas_begin_batch()
{
    ge_push_render_state();
}

void FONT_atlas_end_batch()
{
    ge_pop_render_state();
}
