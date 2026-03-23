#include "engine/effects/flamengine.h"
#include "engine/graphics/pipeline/poly.h"
#include <DDLib.h>

// Temporary: assets/texture.h is in a higher DAG layer (assets/ depends on engine/).
// These externs provide the texture globals flamengine needs without pulling in all of assets/texture.h.
// When texture globals are split into a separate engine-layer header this can be replaced.

// uc_orig: TEXTURE_texture (fallen/DDEngine/Source/flamengine.cpp)
// Direct3D texture array; referenced for locking/unlocking the flame texture.
extern D3DTexture TEXTURE_texture[];

// uc_orig: TEXTURE_page_menuflame (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_page_menuflame;

// uc_orig: TEXTURE_shadow_bitmap (fallen/DDEngine/Headers/texture.h)
extern UWORD* TEXTURE_shadow_bitmap;
// uc_orig: TEXTURE_shadow_pitch (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_pitch;
// uc_orig: TEXTURE_shadow_mask_red (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_mask_red;
// uc_orig: TEXTURE_shadow_mask_green (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_mask_green;
// uc_orig: TEXTURE_shadow_mask_blue (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_mask_blue;
// uc_orig: TEXTURE_shadow_mask_alpha (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_mask_alpha;
// uc_orig: TEXTURE_shadow_shift_red (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_shift_red;
// uc_orig: TEXTURE_shadow_shift_green (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_shift_green;
// uc_orig: TEXTURE_shadow_shift_blue (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_shift_blue;
// uc_orig: TEXTURE_shadow_shift_alpha (fallen/DDEngine/Headers/texture.h)
extern SLONG TEXTURE_shadow_shift_alpha;

// Texture lock/unlock helpers ------------------------------------------
// These mirror the TEXTURE_86_lock/TEXTURE_shadow_lock pattern from texture.cpp,
// but operate on the TEXTURE_page_menuflame page.

// uc_orig: TEXTURE_flame_lock (fallen/DDEngine/Source/flamengine.cpp)
static SLONG TEXTURE_flame_lock()
{
    HRESULT res;

    res = TEXTURE_texture[TEXTURE_page_menuflame].LockUser(
        &TEXTURE_shadow_bitmap,
        &TEXTURE_shadow_pitch);

    if (FAILED(res)) {
        TEXTURE_shadow_bitmap = NULL;
        TEXTURE_shadow_pitch = 0;
        return UC_FALSE;
    } else {
        TEXTURE_shadow_mask_red   = TEXTURE_texture[TEXTURE_page_menuflame].mask_red;
        TEXTURE_shadow_mask_green = TEXTURE_texture[TEXTURE_page_menuflame].mask_green;
        TEXTURE_shadow_mask_blue  = TEXTURE_texture[TEXTURE_page_menuflame].mask_blue;
        TEXTURE_shadow_mask_alpha = TEXTURE_texture[TEXTURE_page_menuflame].mask_alpha;
        TEXTURE_shadow_shift_red   = TEXTURE_texture[TEXTURE_page_menuflame].shift_red;
        TEXTURE_shadow_shift_green = TEXTURE_texture[TEXTURE_page_menuflame].shift_green;
        TEXTURE_shadow_shift_blue  = TEXTURE_texture[TEXTURE_page_menuflame].shift_blue;
        TEXTURE_shadow_shift_alpha = TEXTURE_texture[TEXTURE_page_menuflame].shift_alpha;
        return UC_TRUE;
    }
}

// uc_orig: TEXTURE_flame_unlock (fallen/DDEngine/Source/flamengine.cpp)
static void TEXTURE_flame_unlock()
{
    TEXTURE_texture[TEXTURE_page_menuflame].UnlockUser();
}

// uc_orig: TEXTURE_flame_update (fallen/DDEngine/Source/flamengine.cpp)
static void TEXTURE_flame_update()
{
    // Intentionally empty — Direct3D handles the upload.
}

// uc_orig: FlameRand (fallen/DDEngine/Source/flamengine.cpp)
// Returns a random integer in [0, max).
static int FlameRand(int max)
{
    return rand() % max;
}

// Flamengine implementation -------------------------------------------

// uc_orig: Flamengine::Flamengine (fallen/DDEngine/Source/flamengine.cpp)
Flamengine::Flamengine(char* fname)
{
    MFFileHandle handle = FILE_OPEN_ERROR;
    UWORD ver;

    handle = FileOpen(fname);

    if (handle != FILE_OPEN_ERROR) {
        FileRead(handle, (UBYTE*)&ver, sizeof(ver));
        ReadHeader(handle);
        ReadParts(handle);
        FileClose(handle);
    }

    memset(data, 0, 256 * 256);
    memset(work, 0, 256 * 256);
}

// uc_orig: Flamengine::~Flamengine (fallen/DDEngine/Source/flamengine.cpp)
Flamengine::~Flamengine()
{
}

// uc_orig: Flamengine::ReadHeader (fallen/DDEngine/Source/flamengine.cpp)
void Flamengine::ReadHeader(MFFileHandle handle)
{
    UBYTE skip;

    FileRead(handle, (UBYTE*)&params.blur,    1);
    FileRead(handle, (UBYTE*)&params.dark,    1);
    FileRead(handle, (UBYTE*)&params.convec,  1);
    FileRead(handle, (UBYTE*)&params.palette, 768);
    FileRead(handle, (UBYTE*)&skip,           1);
    FileRead(handle, (UBYTE*)&params.free,    2);
    FileRead(handle, (UBYTE*)&params.posn,    2);
}

// uc_orig: Flamengine::ReadParts (fallen/DDEngine/Source/flamengine.cpp)
void Flamengine::ReadParts(MFFileHandle handle)
{
    int i;
    FlameParticle* pp;
    SLONG skip;

    for (i = 2000, pp = params.particles; i; i--, pp++) {
        FileRead(handle, (UBYTE*)&pp->pos.loc.x, 1);
        FileRead(handle, (UBYTE*)&pp->pos.loc.y, 1);
        FileRead(handle, (UBYTE*)&pp->jx,        1);
        FileRead(handle, (UBYTE*)&pp->jy,        1);
        FileRead(handle, (UBYTE*)&pp->ex,        1);
        FileRead(handle, (UBYTE*)&pp->ey,        1);
        FileRead(handle, (UBYTE*)&pp->life,      1);
        FileRead(handle, (UBYTE*)&skip,          1);
        FileRead(handle, (UBYTE*)&pp->pulse,     2);
        FileRead(handle, (UBYTE*)&pp->prate,     1);
        FileRead(handle, (UBYTE*)&pp->pmode,     1);
        FileRead(handle, (UBYTE*)&pp->wmode,     1);
        FileRead(handle, (UBYTE*)&skip,          3);
        FileRead(handle, (UBYTE*)&pp->pstart,    4);
        FileRead(handle, (UBYTE*)&pp->pend,      4);
    }
}

// uc_orig: Flamengine::Run (fallen/DDEngine/Source/flamengine.cpp)
void Flamengine::Run()
{
    AddParticles();
    if (params.blur) {
        if (!params.randomize)
            ConvectionBlur();
        else
            ConvectionBlur2();
    } else {
        Darkening();
    }
    UpdateTexture();
}

// uc_orig: Flamengine::AddParticles (fallen/DDEngine/Source/flamengine.cpp)
// Writes each active particle's current brightness into the fire field,
// then advances the particle (pulse, walk mode).
void Flamengine::AddParticles()
{
    int i;
    SWORD si;
    UBYTE* pt;
    FlameXY pos;
    FlameParticle* pp;

    for (i = 2000, pp = params.particles; i; i--, pp++) {
        if (!pp->life) continue;

        pos.ofs = pp->pos.ofs;
        if (pp->jx) pos.loc.x += FlameRand(pp->jx) - (pp->jx >> 1);
        if (pp->jy) pos.loc.y += FlameRand(pp->jy) - (pp->jy >> 1);
        pt  = data;
        pt += pos.ofs;
        *pt = (UBYTE)pp->pulse;

        switch (pp->pmode) {
        case 0: // ramp up
            pp->pulse += pp->prate;
            if ((ULONG)pp->pulse >= pp->pend) pp->pulse = (SWORD)pp->pstart;
            break;
        case 1: // ramp down
            pp->pulse -= pp->prate;
            if ((ULONG)pp->pulse <= pp->pstart) pp->pulse = (SWORD)pp->pend;
            break;
        case 2: // cycle up phase
            si = pp->pulse + pp->prate;
            if (si > (SWORD)pp->pend) { si = (SWORD)pp->pend; pp->pmode = 3; }
            pp->pulse = si;
            break;
        case 3: // cycle down phase
            si = pp->pulse - pp->prate;
            if (si < (SWORD)pp->pstart) { si = (SWORD)pp->pstart; pp->pmode = 2; }
            pp->pulse = si;
            break;
        }

        switch (pp->wmode) {
        case 1: // fountain
            switch (i & 3) {
            case 0: if (pp->pos.loc.x > 1)   pp->pos.loc.x -= 2; break;
            case 1: if (pp->pos.loc.x > 0)   pp->pos.loc.x--;    break;
            case 2: if (pp->pos.loc.x < 254) pp->pos.loc.x += 2; break;
            case 3: if (pp->pos.loc.x < 255) pp->pos.loc.x++;    break;
            }
            pp->pos.loc.y += pp->life - 10;
            pp->life++;
            if (pp->life > 50) { pp->life = 1; pp->pos.loc.x = pp->ex; pp->pos.loc.y = pp->ey; }
            break;
        case 2: // cascade
            pp->pos.loc.y += 1 + (i & 3);
            if ((pp->pos.loc.y == 255) || (pp->pos.loc.y < pp->ey)) pp->pos.loc.y = pp->ey;
            break;
        case 3: // gravity
            pp->pos.loc.y++;
            if ((pp->pos.loc.y == 255) || (pp->pos.loc.y < pp->ey)) pp->pos.loc.y = pp->ey;
            break;
        case 4: // sparks
            switch (i & 3) {
            case 0: if (pp->pos.loc.x > 1)   pp->pos.loc.x -= 2; break;
            case 1: if (pp->pos.loc.x > 0)   pp->pos.loc.x--;    break;
            case 2: if (pp->pos.loc.x < 254) pp->pos.loc.x += 2; break;
            case 3: if (pp->pos.loc.x < 255) pp->pos.loc.x++;    break;
            }
            switch ((i >> 1) & 3) {
            case 0: if (pp->pos.loc.y > 1)   pp->pos.loc.y -= 2; break;
            case 1: if (pp->pos.loc.y > 0)   pp->pos.loc.y--;    break;
            case 2: if (pp->pos.loc.y < 254) pp->pos.loc.y += 2; break;
            case 3: if (pp->pos.loc.y < 255) pp->pos.loc.y++;    break;
            }
            pp->life++;
            if (pp->life > 50) { pp->life = 1; pp->pos.loc.x = pp->ex; pp->pos.loc.y = pp->ey; }
            break;
        case 5: // wander
            pp->pos.loc.x += FlameRand(11) - 5;
            pp->pos.loc.y += FlameRand(11) - 5;
            break;
        case 6: // jump to random position
            pp->pos.loc.x = FlameRand(256);
            pp->pos.loc.y = FlameRand(256);
            break;
        }
    }
}

// uc_orig: Flamengine::Darkening (fallen/DDEngine/Source/flamengine.cpp)
// Simple scroll + darken pass (no blur). Each pixel is decremented by 1 if dark is set.
void Flamengine::Darkening()
{
    int x, y, i = 0;
    UBYTE *pt, *dpt;

    dpt = pt = data;
    pt += 256;
    if (!params.convec) dpt += 256;
    if (params.dark) i = 1;
    for (y = 255; y; y--) {
        for (x = 256; x; x--) {
            *dpt++ = (*pt++) - i;
        }
    }
}

// uc_orig: Flamengine::ConvectionBlur (fallen/DDEngine/Source/flamengine.cpp)
// Box blur with optional scroll: averages each pixel with its 4 neighbours,
// then blends with original, with optional darkening.
void Flamengine::ConvectionBlur()
{
    UBYTE *pt1, *pt2, *pt3, *pt4, *pt, *wpt;
    int x, y, i;

    wpt = work;
    pt  = data;
    pt  += 257;
    wpt += 1;
    if (!params.convec) wpt += 256;

    pt1 = pt2 = pt3 = pt4 = pt;
    pt1 -= 1; pt2 += 1; pt3 -= 256; pt4 += 256;

    for (y = 1; y < 254; y++) {
        for (x = 0; x < 256; x++) {
            i  = *pt1;
            i += (*pt2) + (*pt3) + (*pt4);
            i >>= 2;
            i += *pt;
            i >>= 1;
            if (params.dark && i) i--;
            *wpt = i;

            pt++; pt1++; pt2++; pt3++; pt4++; wpt++;
        }
    }
    memcpy(data, work, 65536);
}

// uc_orig: DarkZones (fallen/DDEngine/Source/flamengine.cpp)
// State for ConvectionBlur2's dark band effect.
struct DarkZones {
    SLONG offset, offtime, midpnt, width;
};

// uc_orig: ZONES (fallen/DDEngine/Source/flamengine.cpp)
#define ZONES 3

// uc_orig: Flamengine::ConvectionBlur2 (fallen/DDEngine/Source/flamengine.cpp)
// Enhanced blur with animated dark bands (randomise=1 mode).
void Flamengine::ConvectionBlur2()
{
    UBYTE *pt1, *pt2, *pt3, *pt4, *pt, *wpt;
    SLONG x, y, i, j, dif;
    static DarkZones zones[ZONES];
    SLONG difs[256];

    wpt = work;
    pt  = data;
    pt  += 257;
    wpt += 1;
    if (!params.convec) wpt += 256;

    pt1 = pt2 = pt3 = pt4 = pt;
    pt1 -= 1; pt2 += 1; pt3 -= 256; pt4 += 256;

    for (j = 0; j < ZONES; j++) {
        if (!zones[j].offtime) {
            zones[j].offset = FlameRand(256);
            zones[j].midpnt = 50 + FlameRand(150);
            zones[j].offtime = zones[j].midpnt * 2;
        }
        zones[j].offtime--;
    }
    for (x = 0; x < 256; x++) {
        difs[x] = 0;
        for (j = 0; j < 3; j++) {
            dif = abs(x - zones[j].offset) - abs(zones[j].midpnt - zones[j].offtime);
            if (dif < 0) dif = 0;
            difs[x] += (SLONG)(dif * 0.2f);
        }
    }
    for (y = 1; y < 254; y++) {
        for (x = 0; x < 256; x++) {
            i  = *pt1;
            i += (*pt2) + (*pt3) + (*pt4);
            i >>= 2;
            i += *pt;
            i >>= 1;
            i -= difs[x];
            if (i < 0) i = 0;
            *wpt = (UBYTE)i;

            pt++; pt1++; pt2++; pt3++; pt4++; wpt++;
        }
    }
    memcpy(data, work, 65536);
}

// uc_orig: Flamengine::UpdateTexture (fallen/DDEngine/Source/flamengine.cpp)
// Locks the flame texture, applies the palette to convert fire bytes to pixels,
// then unlocks.
void Flamengine::UpdateTexture()
{
    if (TEXTURE_flame_lock()) {
        SLONG x;
        SLONG y;
        UWORD* image;
        UWORD pixel;
        UBYTE *pt, *pt2;
        UBYTE red, green, blue;

        for (y = 0; y < 256; y++) {
            image  = TEXTURE_shadow_bitmap;
            image += y * (TEXTURE_shadow_pitch >> 1);

            pt  = data;
            pt += 256 * y;

            for (x = 0; x < 256; x++, image++, pt++) {
                pt2 = params.palette + ((*pt) * 3);
                red   = *pt2++;
                green = *pt2++;
                blue  = *pt2++;

                pixel  = (red   >> TEXTURE_shadow_mask_red)   << TEXTURE_shadow_shift_red;
                pixel |= (green >> TEXTURE_shadow_mask_green) << TEXTURE_shadow_shift_green;
                pixel |= (blue  >> TEXTURE_shadow_mask_blue)  << TEXTURE_shadow_shift_blue;
                *image = pixel;
            }
        }

        TEXTURE_flame_unlock();
        TEXTURE_flame_update();
    }
}

// uc_orig: Flamengine::Blit (fallen/DDEngine/Source/flamengine.cpp)
// Renders the flame texture as a full-screen quad to the back buffer.
void Flamengine::Blit()
{
    POLY_Point pp[4], *quad[4];

    quad[0] = &pp[0]; quad[1] = &pp[1]; quad[2] = &pp[2]; quad[3] = &pp[3];

    pp[0].X = 0;   pp[0].Y = 200; pp[0].Z = 0.5f; pp[0].u = 0.0; pp[0].v = 0.0;
    pp[1].X = 640; pp[1].Y = 200; pp[1].Z = 0.5f; pp[1].u = 2.0; pp[1].v = 0.0;
    pp[2].X = 0;   pp[2].Y = 480; pp[2].Z = 0.5f; pp[2].u = 0.0; pp[2].v = 1.0;
    pp[3].X = 640; pp[3].Y = 480; pp[3].Z = 0.5f; pp[3].u = 2.0; pp[3].v = 1.0;

    for (int i = 0; i < 4; i++) { pp[i].colour = 0xffffff; pp[i].specular = 0xff000000; }

    POLY_add_quad(quad, POLY_PAGE_MENUFLAME, UC_FALSE, UC_TRUE);
}

// uc_orig: Flamengine::BlitHalf (fallen/DDEngine/Source/flamengine.cpp)
// Renders half of the flame texture (left or right side, side=0 or 1).
void Flamengine::BlitHalf(CBYTE side)
{
    POLY_Point pp[4], *quad[4];

    quad[0] = &pp[0]; quad[1] = &pp[1]; quad[2] = &pp[2]; quad[3] = &pp[3];

    if (side) {
        pp[0].X = 320; pp[1].X = 640; pp[2].X = 320; pp[3].X = 640;
    } else {
        pp[0].X = 0;   pp[1].X = 320; pp[2].X = 0;   pp[3].X = 320;
    }

    pp[0].Y = 200; pp[0].Z = 0.5f; pp[0].u = 0.0; pp[0].v = 0.0;
    pp[1].Y = 200; pp[1].Z = 0.5f; pp[1].u = 1.0; pp[1].v = 0.0;
    pp[2].Y = 480; pp[2].Z = 0.5f; pp[2].u = 0.0; pp[2].v = 1.0;
    pp[3].Y = 480; pp[3].Z = 0.5f; pp[3].u = 1.0; pp[3].v = 1.0;

    for (int i = 0; i < 4; i++) { pp[i].colour = 0xffffff; pp[i].specular = 0xff000000; }

    POLY_add_quad(quad, POLY_PAGE_MENUFLAME, UC_FALSE, UC_TRUE);
}

// uc_orig: Flamengine::BlitOffset (fallen/DDEngine/Source/flamengine.cpp)
// Renders the flame texture skewed for the feedback effect (two overlapping quads).
void Flamengine::BlitOffset()
{
    POLY_Point pp[4], *quad[4];

    quad[0] = &pp[0]; quad[1] = &pp[1]; quad[2] = &pp[2]; quad[3] = &pp[3];

    pp[0].X = 0;   pp[0].Y = 0;   pp[0].Z = 0.5f; pp[0].u = 0.0; pp[0].v = 0.0;
    pp[1].X = 256; pp[1].Y = 0;   pp[1].Z = 0.5f; pp[1].u = 1.0; pp[1].v = 0.0;
    pp[2].X = 0;   pp[2].Y = 256; pp[2].Z = 0.5f; pp[2].u = 0.0; pp[2].v = 1.0;
    pp[3].X = 256; pp[3].Y = 256; pp[3].Z = 0.5f; pp[3].u = 1.0; pp[3].v = 1.0;

    for (int i = 0; i < 4; i++) { pp[i].colour = 0xffffff; pp[i].specular = 0xff000000; }

    POLY_add_quad(quad, POLY_PAGE_MENUFLAME, UC_FALSE, UC_TRUE);

    pp[0].u = 0.1f; pp[0].v = 0.2f;
    pp[1].u = 0.9f; pp[1].v = 0.2f;
    pp[2].u = 0.1f; pp[2].v = 1.0f;
    pp[3].u = 0.9f; pp[3].v = 1.0f;
    for (int i = 0; i < 4; i++) { pp[i].colour = 0xafafaf; }

    POLY_add_quad(quad, POLY_PAGE_MENUFLAME, UC_FALSE, UC_TRUE);
}

// uc_orig: Flamengine::Feedback (fallen/DDEngine/Source/flamengine.cpp)
// Full feedback cycle: clear viewport, render flame quad skewed, blit back to texture.
// Must be called BEFORE any other screen draw each frame.
void Flamengine::Feedback()
{
    RECT rcSource;

    the_display.lp_D3D_Viewport->Clear(1, &the_display.ViewportRect, D3DCLEAR_TARGET);

    POLY_frame_init(UC_FALSE, UC_FALSE);
    BlitOffset();
    POLY_frame_draw(UC_FALSE, UC_TRUE);

    rcSource.top    = 0;
    rcSource.left   = 0;
    rcSource.bottom = 256;
    rcSource.right  = 256;

    TEXTURE_texture[TEXTURE_page_menuflame].GetSurface()->Blt(NULL, the_display.lp_DD_BackSurface, &rcSource, DDBLT_WAIT, NULL);
}
