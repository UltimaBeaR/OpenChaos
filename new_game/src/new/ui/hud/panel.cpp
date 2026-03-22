// HUD panel system: health bars, gun sights, timers, inventory,
// text messages, and full-screen effects (fade, widescreen, signs).
// Renders directly into the poly submission buffer.
//
// Chunk 1: core drawing utilities + icon tables + beat/toss/face/text init

#include "ui/hud/panel.h"
#include "ui/hud/panel_globals.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/resources/font2d.h"
#include "engine/graphics/resources/font2d_globals.h"

// Temporary: AENG functions defined in aeng.cpp (not yet migrated)
#include "fallen/DDEngine/Headers/aeng.h"

// Temporary: Thing, Class constants, person types
#include "fallen/Headers/Thing.h"
#include "fallen/Headers/Person.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <windows.h>

// estate: set when the player is in the manor level (affects face display).
extern unsigned char estate;

// uc_orig: HEALTH_BAR_WIDTH (fallen/DDEngine/Source/panel.cpp)
#define HEALTH_BAR_WIDTH 100
// uc_orig: HEALTH_BAR_HEIGHT (fallen/DDEngine/Source/panel.cpp)
#define HEALTH_BAR_HEIGHT 10
// uc_orig: HEALTH_BAR_VERT_GAP (fallen/DDEngine/Source/panel.cpp)
#define HEALTH_BAR_VERT_GAP 2
// uc_orig: HEALTH_BAR_HORIZ_GAP (fallen/DDEngine/Source/panel.cpp)
#define HEALTH_BAR_HORIZ_GAP 2

// uc_orig: PANEL_GetNextDepthBodge (fallen/DDEngine/Source/panel.cpp)
float PANEL_GetNextDepthBodge(void)
{
    return (0.9f);
}

// uc_orig: PANEL_ResetDepthBodge (fallen/DDEngine/Source/panel.cpp)
void PANEL_ResetDepthBodge(void)
{
    // Nothing — static depth value is always 0.9f.
}

// Draws a textured quad at fixed screen coordinates using the depth bodge
// to place it on top of the 3D scene.
// uc_orig: PANEL_draw_quad (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_quad(
    float left,
    float top,
    float right,
    float bottom,
    long page,
    unsigned long colour,
    float u1,
    float v1,
    float u2,
    float v2)
{
    POLY_Point pp[4];
    POLY_Point* quad[4];

    float fWDepthBodge = PANEL_GetNextDepthBodge();
    float fZDepthBodge = 1.0f - fWDepthBodge;

    pp[0].X = left;
    pp[0].Y = top;
    pp[0].z = fZDepthBodge;
    pp[0].Z = fWDepthBodge;
    pp[0].u = u1;
    pp[0].v = v1;
    pp[0].colour = colour;
    pp[0].specular = 0xff000000;

    pp[1].X = right;
    pp[1].Y = top;
    pp[1].z = fZDepthBodge;
    pp[1].Z = fWDepthBodge;
    pp[1].u = u2;
    pp[1].v = v1;
    pp[1].colour = colour;
    pp[1].specular = 0xff000000;

    pp[2].X = left;
    pp[2].Y = bottom;
    pp[2].z = fZDepthBodge;
    pp[2].Z = fWDepthBodge;
    pp[2].u = u1;
    pp[2].v = v2;
    pp[2].colour = colour;
    pp[2].specular = 0xff000000;

    pp[3].X = right;
    pp[3].Y = bottom;
    pp[3].z = fZDepthBodge;
    pp[3].Z = fWDepthBodge;
    pp[3].u = u2;
    pp[3].v = v2;
    pp[3].colour = colour;
    pp[3].specular = 0xff000000;

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
}

// Draws a drop-shadowed debug text string at screen position (x, y).
// uc_orig: PANEL_crap_text (fallen/DDEngine/Source/panel.cpp)
void PANEL_crap_text(int x, int y, char* string)
{
    FONT2D_DrawString(string, x + 1, y + 1, 0x000000, 256, POLY_PAGE_FONT2D, 0);
    FONT2D_DrawString(string, x, y, 0xffffee, 256, POLY_PAGE_FONT2D, 0);
}

// Draws one of the face thumbnails (used by the old-style panel).
// face: 1..6 selecting which face icon.
// uc_orig: PANEL_draw_face (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_face(long x, long y, long face, long size)
{
    float left = (float)x;
    float top = (float)y;
    float right = left + (float)size;
    float bottom = top + (float)size;

    SATURATE(face, 1, 6);

    PANEL_draw_quad(left, top, right, bottom, POLY_PAGE_FACE1 + face - 1);
}

// Draws a two-segment health bar at (x, y): black background + red fill.
// uc_orig: PANEL_draw_health_bar (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_health_bar(long x, long y, long percentage)
{
    AENG_draw_rect(x, y, HEALTH_BAR_WIDTH, HEALTH_BAR_HEIGHT, 0x000000, 2, POLY_PAGE_COLOUR);

    if (percentage < 0) {
        percentage = 0;
    } else if (percentage > 100) {
        percentage = 100;
    }

    AENG_draw_rect(
        x + HEALTH_BAR_HORIZ_GAP,
        y + HEALTH_BAR_VERT_GAP,
        ((HEALTH_BAR_WIDTH - HEALTH_BAR_HORIZ_GAP * 2) * percentage) / 100,
        6,
        0xff0000,
        1,
        POLY_PAGE_COLOUR);
}

// 7-segment digit renderer using rectangles.
// digit must be 0..9.
// uc_orig: PANEL_draw_number (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_number(float x, float y, unsigned char digit)
{
#define B0 (1 << 0)
#define B1 (1 << 1)
#define B2 (1 << 2)
#define B3 (1 << 3)
#define B4 (1 << 4)
#define B5 (1 << 5)
#define B6 (1 << 6)
#define PANEL_SEG_L (16.0F)
#define PANEL_SEG_W (4.0F)

    unsigned char number[10] = {
        B0 | B1 | B2 | B4 | B5 | B6,
        B2 | B5,
        B0 | B2 | B3 | B4 | B6,
        B0 | B2 | B3 | B5 | B6,
        B1 | B2 | B3 | B5,
        B0 | B1 | B3 | B5 | B6,
        B0 | B1 | B3 | B4 | B5 | B6,
        B0 | B2 | B5,
        B0 | B1 | B2 | B3 | B4 | B5 | B6,
        B0 | B1 | B2 | B3 | B5
    };

    struct {
        unsigned char d1;
        unsigned char d2;
        float dx;
        float dy;
    } bit[7] = {
        { 0, 1, 0.0F, PANEL_SEG_W / 2.0F },
        { 0, 2, PANEL_SEG_W / 2.0F, 0.0F },
        { 1, 3, PANEL_SEG_W / 2.0F, 0.0F },
        { 2, 3, 0.0F, PANEL_SEG_W / 2.0F },
        { 2, 4, PANEL_SEG_W / 2.0F, 0.0F },
        { 3, 5, PANEL_SEG_W / 2.0F, 0.0F },
        { 4, 5, 0.0F, PANEL_SEG_W / 2.0F },
    };

    POLY_Point pp[4];
    POLY_Point* quad[4];

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    ASSERT(WITHIN(digit, 0, 9));

    for (int b = 0; b < 7; b++) {
        float x1 = x + ((bit[b].d1 & 1) ? PANEL_SEG_L : 0.0F);
        float y1 = y + (bit[b].d1 >> 1) * PANEL_SEG_L;
        float x2 = x + ((bit[b].d2 & 1) ? PANEL_SEG_L : 0.0F);
        float y2 = y + (bit[b].d2 >> 1) * PANEL_SEG_L;

        unsigned int colour = (number[digit] & (1 << b)) ? 0xeeff0000 : 0x88110000;

        float dx = bit[b].dx;
        float dy = bit[b].dy;

        float fWDepthBodge = PANEL_GetNextDepthBodge();
        float fZDepthBodge = 1.0f - fWDepthBodge;

        pp[0].X = x1 - dx;
        pp[0].Y = y1 - dy;
        pp[0].z = fZDepthBodge;
        pp[0].Z = fWDepthBodge;
        pp[0].u = 0.0F;
        pp[0].v = 0.0F;
        pp[0].colour = colour;
        pp[0].specular = 0xff000000;

        pp[1].X = x2 - dx;
        pp[1].Y = y2 - dy;
        pp[1].z = fZDepthBodge;
        pp[1].Z = fWDepthBodge;
        pp[1].u = 1.0F;
        pp[1].v = 0.0F;
        pp[1].colour = colour;
        pp[1].specular = 0xff000000;

        pp[2].X = x1 + dx;
        pp[2].Y = y1 + dy;
        pp[2].z = fZDepthBodge;
        pp[2].Z = fWDepthBodge;
        pp[2].u = 0.0F;
        pp[2].v = 1.0F;
        pp[2].colour = colour;
        pp[2].specular = 0xff000000;

        pp[3].X = x2 + dx;
        pp[3].Y = y2 + dy;
        pp[3].z = fZDepthBodge;
        pp[3].Z = fWDepthBodge;
        pp[3].u = 1.0F;
        pp[3].v = 1.0F;
        pp[3].colour = colour;
        pp[3].specular = 0xff000000;

        POLY_add_quad(quad, POLY_PAGE_ALPHA, UC_FALSE, UC_TRUE);
    }
#undef B0
#undef B1
#undef B2
#undef B3
#undef B4
#undef B5
#undef B6
#undef PANEL_SEG_L
#undef PANEL_SEG_W
}

// Queues a countdown timer display for PANEL_draw_buffered() to render.
// time is in hundredths of a second.
// uc_orig: PANEL_draw_timer (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_timer(long time, long x, long y)
{
    if (WITHIN(PANEL_store_upto, 0, PANEL_MAX_STORES - 1)) {
        PANEL_store[PANEL_store_upto].time = float(time) * (1.0F / 100.0F);
        PANEL_store[PANEL_store_upto].x = float(x);
        PANEL_store[PANEL_store_upto].y = float(y);
        PANEL_store_upto += 1;
    }
}

// Flushes all queued timers — actually draws the countdown text.
// Pulses red when under 30 seconds.
// uc_orig: PANEL_draw_buffered (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_buffered()
{
    for (int i = 0; i < PANEL_store_upto; i++) {
        float time = PANEL_store[i].time;

        char countdown[32];
        int mins = 0;

        ASSERT(time < 1000.0F);

        while (time >= 60.0F) {
            mins += 1;
            time -= 60.0F;
        }

        sprintf(countdown, "%02d:%02d", mins, (int)time);

        if ((time < 30) && !mins) {
            static unsigned short pulse = 0;
            pulse += (TICK_RATIO * 80) >> TICK_SHIFT;
            int colour = (SIN(pulse & 2047) >> 9) + 128;
            colour = colour | (colour << 8);
            FONT2D_DrawStringCentred(countdown, m_iPanelXPos + 171, m_iPanelYPos - 118, 0xff0000 | colour, 256 + 64);
        } else {
            FONT2D_DrawStringCentred(countdown, m_iPanelXPos + 171, m_iPanelYPos - 118, 0xffffff, 256 + 64);
        }
    }

    PANEL_store_upto = 0;
}

// Draws an in-world health bar floating above position (mx, my, mz).
// The bar moves towards the camera so it sorts in front of the character.
// uc_orig: PANEL_draw_local_health (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_local_health(long mx, long my, long mz, long percentage, long radius)
{
    POLY_Point p1;

    float dx = POLY_cam_x - (float)mx;
    float dy = POLY_cam_y - (float)my;
    float dz = POLY_cam_z - (float)mz;

    float len = sqrt(dx * dx + dy * dy + dz * dz);

    dx = (dx * ((float)radius)) / len;
    dy = (dy * ((float)radius)) / len;
    dz = (dz * ((float)radius)) / len;

    POLY_transform(float(mx + dx), float(my + dy - 10), float(mz + dz), &p1);

    if (percentage > 100) {
        percentage = 100;
    }

    p1.X -= 27.0f;
    p1.colour = 0xc0000000 | 0x0f;
    p1.specular = 0xff000000;

    extern void POLY_add_rect(POLY_Point* p1, long width, long height, long page, unsigned char sort_to_front);

    if (p1.IsValid()) {
        POLY_add_rect(&p1, 54, 4, POLY_PAGE_COLOUR, 0);
    } else {
        return;
    }

    p1.X += 2.0f;
    p1.colour = 0x40000000 | 0xff0000;
    p1.specular = 0xff000000;

    if (p1.IsValid()) {
        POLY_add_rect(&p1, (50 * percentage) / 100, 4, POLY_PAGE_COLOUR, 0);
    }
}

// Draws a gun-sight crosshair in world space at (mx, my, mz).
// accuracy 0=perfect (tight), 255=wildly inaccurate (open).
// scale: 256=normal size.
// uc_orig: PANEL_draw_gun_sight (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_gun_sight(long mx, long my, long mz, long accuracy, long scale)
{
    long angle, cangle;
    long c0;
    long dx1, dy1, dx2, dy2;
    POLY_Point p1, p2, pstart;
    long r_in, r_out;
    unsigned long col;
    long sat_acc;

#define RADIUS_OUT 164
#define RADIUS_IN 84

    angle = accuracy;
    POLY_transform(float(mx), float(my - 36), float(mz), &pstart);
    r_in = (POLY_world_length_to_screen(RADIUS_IN) * pstart.Z * scale) / 256;
    r_out = (POLY_world_length_to_screen(RADIUS_OUT) * pstart.Z * scale) / 256;

    sat_acc = accuracy;
    SATURATE(sat_acc, 0, 255)

    col = sat_acc << 8;
    col |= (255 - sat_acc) << 16;

    // Four outer tick marks spaced ±accuracy around the cardinal angles.
    for (c0 = 0; c0 < 4; c0++) {
        p2 = p1 = pstart;
        switch (c0) {
        case 0: cangle = +angle; break;
        case 1: cangle = -angle; break;
        case 2: cangle = 1024 + angle; break;
        case 3: cangle = 1024 - angle; break;
        }

        dx1 = ((COS(cangle & 2047) * r_out) >> 16);
        dy1 = ((SIN(cangle & 2047) * r_out) >> 16);
        dx2 = ((COS(cangle & 2047) * r_in) >> 16);
        dy2 = ((SIN(cangle & 2047) * r_in) >> 16);

        p1.X += (float)dx1;
        p1.Y += (float)dy1;
        p2.X += (float)dx2;
        p2.Y += (float)dy2;

        p1.colour = 0x40000000 | col;
        p2.colour = 0x40000000 | col;
        p1.specular = 0xff000000;
        p2.specular = 0xff000000;

        if (p1.IsValid() && p2.IsValid()) {
            int width = (30 * scale) >> 8;
            POLY_add_line(&p1, &p2, (float)width, 0.0F, POLY_PAGE_COLOUR_ALPHA, 0);
        }
    }

    // Four inner diagonal tick marks at fixed 45° offsets.
    r_in = 20 + (sat_acc >> 2);
    r_in = (r_in * scale) >> 8;
    r_out = r_in + ((80 * scale) >> 8);

    r_in = (int)(POLY_world_length_to_screen(r_in) * pstart.Z);
    r_out = (int)(POLY_world_length_to_screen(r_out) * pstart.Z);

    for (c0 = 0; c0 < 4; c0++) {
        p2 = p1 = pstart;
        switch (c0) {
        case 0: cangle = 512; break;
        case 1: cangle = -512; break;
        case 2: cangle = 1024; break;
        case 3: cangle = 0; break;
        }

        dx1 = ((COS(cangle & 2047) * (r_out)) >> 16);
        dy1 = ((SIN(cangle & 2047) * (r_out)) >> 16);
        dx2 = ((COS(cangle & 2047) * (r_in)) >> 16);
        dy2 = ((SIN(cangle & 2047) * (r_in)) >> 16);

        p1.X += (float)dx1;
        p1.Y += (float)dy1;
        p2.X += (float)dx2;
        p2.Y += (float)dy2;

        p1.colour = 0x40000000 | col;
        p2.colour = 0x40000000 | col;
        p1.specular = 0xff000000;
        p2.specular = 0xff000000;

        if (p1.IsValid() && p2.IsValid()) {
            int width = (5 * scale) >> 8;
            POLY_add_line(&p1, &p2, (float)width, (float)width, POLY_PAGE_COLOUR_ALPHA, 0);
        }
    }
#undef RADIUS_OUT
#undef RADIUS_IN
}

// Draws a HUD icon from the IC sprite page at screen position (x, y).
// which: PANEL_IC_* constant. panel_page: PANEL_PAGE_* blend mode.
// Pass width/height = -1.0F to use the icon's natural size.
// uc_orig: PANEL_funky_quad (fallen/DDEngine/Source/panel.cpp)
void PANEL_funky_quad(
    long which,
    long x,
    long y,
    long panel_page,
    unsigned long colour,
    float width,
    float height)
{
    float left, top, right, bottom;

    ASSERT(WITHIN(which, 0, PANEL_IC_NUMBER - 1));
    ASSERT(WITHIN(panel_page, 0, PANEL_PAGE_NUMBER - 1));

    PANEL_Ic* pi = &PANEL_ic[which];
    long page = PANEL_page[pi->page][panel_page];

    if (which == PANEL_IC_SHOTGUN) {
        // The shotgun is on its side — rotate 90° to display vertically.
        if (width == -1.0F) {
            width = (pi->v2 - pi->v1) * 256.0F;
        }
        if (height == -1.0F) {
            height = (pi->u2 - pi->u1) * 256.0F;
        }

        left = float(x);
        top = float(y);
        right = left + width;
        bottom = top + height;

        float fWDepthBodge = PANEL_GetNextDepthBodge();
        float fZDepthBodge = 1.0f - fWDepthBodge;

        POLY_Point pp[4];
        POLY_Point* quad[4];
        quad[0] = &pp[0]; quad[1] = &pp[1];
        quad[2] = &pp[2]; quad[3] = &pp[3];

        pp[0].X = left;  pp[0].Y = top;
        pp[0].z = fZDepthBodge; pp[0].Z = fWDepthBodge;
        pp[0].u = pi->u2; pp[0].v = pi->v1;
        pp[0].colour = colour; pp[0].specular = 0xff000000;

        pp[1].X = right; pp[1].Y = top;
        pp[1].z = fZDepthBodge; pp[1].Z = fWDepthBodge;
        pp[1].u = pi->u2; pp[1].v = pi->v2;
        pp[1].colour = colour; pp[1].specular = 0xff000000;

        pp[2].X = left;  pp[2].Y = bottom;
        pp[2].z = fZDepthBodge; pp[2].Z = fWDepthBodge;
        pp[2].u = pi->u1; pp[2].v = pi->v1;
        pp[2].colour = colour; pp[2].specular = 0xff000000;

        pp[3].X = right; pp[3].Y = bottom;
        pp[3].z = fZDepthBodge; pp[3].Z = fWDepthBodge;
        pp[3].u = pi->u1; pp[3].v = pi->v2;
        pp[3].colour = colour; pp[3].specular = 0xff000000;

        POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
    } else {
        left = float(x);
        top = float(y);

        if (width == -1.0F) {
            width = (pi->u2 - pi->u1) * 256.0F;
        }
        if (height == -1.0F) {
            height = (pi->v2 - pi->v1) * 256.0F;
        }

        right = left + width;
        bottom = top + height;

        PANEL_draw_quad(left, top, right, bottom, page, colour,
            pi->u1, pi->v1, pi->u2, pi->v2);
    }
}

// Creates a new ammo-toss particle that flies off-screen.
// uc_orig: PANEL_new_toss (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_toss(long type, float sx, float sy)
{
    ASSERT(WITHIN(type, 0, PANEL_AMMO_NUMBER - 1));

    PANEL_toss_last += 1;
    PANEL_toss_last &= PANEL_MAX_TOSSES - 1;

    PANEL_Toss* pt = &PANEL_toss[PANEL_toss_last];
    PANEL_Ammo* pa = &PANEL_ammo[type];

    pt->used = UC_TRUE;
    pt->type = (unsigned short)type;
    pt->x = sx + pa->width * 0.5F;
    pt->y = sy + pa->height * 0.5F;
    pt->dx = 3.5F + ((float)rand() / (float)RAND_MAX);
    pt->dy = -5.0F - ((float)rand() / (float)RAND_MAX);
    pt->angle = 0.0F;
}

// Advances toss physics and renders all active tossed-ammo particles.
// uc_orig: PANEL_do_tosses (fallen/DDEngine/Source/panel.cpp)
void PANEL_do_tosses(void)
{
    unsigned int now = GetTickCount();

    // Process at 20 Hz, but never more than 4 frames behind.
    if (PANEL_toss_tick < now - (1000 / 20) * 4) {
        PANEL_toss_tick = now - (1000 / 20) * 4;
    }

    while (PANEL_toss_tick < now) {
        PANEL_toss_tick += 1000 / 20;

        for (int i = 0; i < PANEL_MAX_TOSSES; i++) {
            PANEL_Toss* pt = &PANEL_toss[i];

            if (pt->used) {
                pt->x += pt->dx;
                pt->y += pt->dy;
                pt->angle += 0.8F;
                pt->dx *= 0.9F;
                pt->dy += 1.5F;

                if (pt->y > 490.0F) {
                    pt->used = UC_FALSE;
                }
            }
        }
    }

    float fWDepthBodge = PANEL_GetNextDepthBodge();
    float fZDepthBodge = 1.0f - fWDepthBodge;

    POLY_Point pp[4];
    POLY_Point* quad[4];
    quad[0] = &pp[0]; quad[1] = &pp[1];
    quad[2] = &pp[2]; quad[3] = &pp[3];

    for (int k = 0; k < 4; k++) {
        pp[k].z = fZDepthBodge;
        pp[k].Z = fWDepthBodge;
        pp[k].colour = 0xffffffff;
        pp[k].specular = 0xff000000;
    }

    for (int i = 0; i < PANEL_MAX_TOSSES; i++) {
        PANEL_Toss* pt = &PANEL_toss[i];

        if (pt->used) {
            ASSERT(WITHIN(pt->type, 0, PANEL_AMMO_NUMBER - 1));

            PANEL_Ammo* pa = &PANEL_ammo[pt->type];

            float ax = (float)sin(pt->angle) * pa->height * 0.5F;
            float ay = (float)cos(pt->angle) * pa->height * 0.5F;

            float bx = (float)sin(pt->angle + 3.14159265F * 0.5F) * pa->width * 0.5F;
            float by = (float)cos(pt->angle + 3.14159265F * 0.5F) * pa->width * 0.5F;

            pp[0].X = pt->x - bx - ax;
            pp[0].Y = pt->y - by - ay;
            pp[0].u = PANEL_ic[pa->page_one].u1;
            pp[0].v = PANEL_ic[pa->page_one].v1;

            pp[1].X = pt->x + bx - ax;
            pp[1].Y = pt->y + by - ay;
            pp[1].u = PANEL_ic[pa->page_one].u2;
            pp[1].v = PANEL_ic[pa->page_one].v1;

            pp[2].X = pt->x - bx + ax;
            pp[2].Y = pt->y - by + ay;
            pp[2].u = PANEL_ic[pa->page_one].u1;
            pp[2].v = PANEL_ic[pa->page_one].v2;

            pp[3].X = pt->x + bx + ax;
            pp[3].Y = pt->y + by + ay;
            pp[3].u = PANEL_ic[pa->page_one].u2;
            pp[3].v = PANEL_ic[pa->page_one].v2;

            long page = PANEL_page[PANEL_ic[pa->page_one].page][PANEL_PAGE_ALPHA_END];
            POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
        }
    }
}

// Determines the face icon index for a given Thing (person type + mesh ID).
// Returns PANEL_FACE_RADIO for NULL or non-person Things.
// uc_orig: PANEL_new_face (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_face(struct Thing* who, float x, float y, long size)
{
    long face;

#define PANEL_FACE_RADIO 0
#define PANEL_FACE_DARCI 1
#define PANEL_FACE_ROPER 2
#define PANEL_FACE_THUG_RASTA 3
#define PANEL_FACE_THUG_GREY 4
#define PANEL_FACE_THUG_RED 5
#define PANEL_FACE_CIV_1 6
#define PANEL_FACE_CIV_2 7
#define PANEL_FACE_CIV_3 8
#define PANEL_FACE_CIV_4 9
#define PANEL_FACE_SLAG 10
#define PANEL_FACE_FAT_SLAG 11
#define PANEL_FACE_HOSTAGE 12
#define PANEL_FACE_WORKMAN 13
#define PANEL_FACE_BOSS_GOATIE 14
#define PANEL_FACE_BOSS_SHAVEN 15
#define PANEL_FACE_COP 16
#define PANEL_FACE_COMMISSIONER 17
#define PANEL_FACE_MIB 18
#define PANEL_FACE_1920S_MAN 19
#define PANEL_FACE_1920S_GIRL 20
#define PANEL_FACE_TRAMP 21
#define PANEL_FACE_NUMBER 22
    typedef struct {
        unsigned char page;
        unsigned char u;
        unsigned char v;
    } PANEL_Face;

    PANEL_Face face_large[PANEL_FACE_NUMBER] = {
        { 0, 4, 4 }, // Radio
        { 0, 0, 0 }, // Darci
        { 0, 2, 0 }, // Roper
        { 0, 4, 0 }, // Thug rasta
        { 1, 0, 4 }, // Thug grey
        { 0, 4, 2 }, // Thug red
        { 0, 6, 2 }, // Civ1
        { 0, 6, 4 }, // Civ2
        { 0, 4, 6 }, // Civ3
        { 0, 6, 6 }, // Civ4
        { 0, 0, 6 }, // Slag
        { 0, 2, 6 }, // Fat ugly slag
        { 0, 2, 4 }, // Hostage
        { 1, 2, 4 }, // Workman
        { 0, 6, 0 }, // Boss with a goatie
        { 0, 6, 0 }, // Boss with no goatie
        { 0, 0, 4 }, // Cop
        { 0, 6, 0 }, // Commissioner
        { 0, 0, 2 }, // MIB
        { 1, 2, 6 }, // 1920s man
        { 1, 4, 6 }, // 1920s girl
        { 1, 0, 6 }, // Tramp
    };

    PANEL_Face face_small[PANEL_FACE_NUMBER] = {
        { 1, 2, 2 }, // Radio
        { 1, 0, 0 }, // Darci
        { 1, 1, 0 }, // Roper
        { 1, 2, 0 }, // Thug rasta
        { 1, 4, 0 }, // Thug grey
        { 1, 2, 1 }, // Thug red
        { 1, 3, 1 }, // Civ1
        { 1, 3, 2 }, // Civ2
        { 1, 2, 3 }, // Civ3
        { 1, 3, 3 }, // Civ4
        { 1, 0, 3 }, // Slag
        { 1, 1, 3 }, // Fat ugly slag
        { 1, 1, 2 }, // Hostage
        { 1, 5, 0 }, // Workman
        { 1, 3, 0 }, // Boss with goatee
        { 1, 3, 0 }, // Boss without goatee
        { 1, 0, 2 }, // Cop
        { 1, 0, 3 }, // Commissioner
        { 1, 0, 1 }, // MIB
        { 1, 6, 0 }, // 1920s man
        { 1, 7, 0 }, // 1920s girl
        { 1, 4, 1 }, // Tramp
    };

    if (who == NULL || who->Class != CLASS_PERSON) {
        face = PANEL_FACE_RADIO;
    } else {
        switch (who->Genus.Person->PersonType) {
        case PERSON_DARCI:
            face = PANEL_FACE_DARCI;
            break;

        case PERSON_ROPER:
            face = PANEL_FACE_ROPER;
            break;

        case PERSON_COP:
            face = PANEL_FACE_COP;
            break;

        case PERSON_CIV:
            switch (who->Draw.Tweened->MeshID) {
            case 7:
                // Normal (non-wandering) civs — pick face by PersonID offset.
                switch (who->Draw.Tweened->PersonID - 6) {
                case 0:
                    face = PANEL_FACE_CIV_3; // Pinhead
                    break;
                case 2:
                    face = PANEL_FACE_CIV_4; // Oriental
                    break;
                case 1:
                case 3:
                    face = PANEL_FACE_CIV_1; // Black man with moustache
                    break;
                default:
                    ASSERT(0);
                    face = PANEL_FACE_CIV_1;
                    break;
                }
                break;

            case 8:
                face = PANEL_FACE_1920S_MAN; // Male fake wandering civ
                break;

            case 9:
                face = PANEL_FACE_1920S_GIRL; // Female fake wandering civ
                break;

            default:
                ASSERT(0);
                face = PANEL_FACE_CIV_2;
                break;
            }
            break;

        case PERSON_THUG_RASTA:
            face = PANEL_FACE_THUG_RASTA;
            break;

        case PERSON_THUG_GREY:
            face = PANEL_FACE_THUG_GREY;
            break;

        case PERSON_THUG_RED:
            face = PANEL_FACE_THUG_RED;
            break;

        case PERSON_SLAG_TART:
            face = PANEL_FACE_SLAG;
            break;

        case PERSON_SLAG_FATUGLY:
            face = PANEL_FACE_FAT_SLAG;
            break;

        case PERSON_HOSTAGE:
            face = PANEL_FACE_HOSTAGE;
            break;

        case PERSON_MECHANIC:
            face = PANEL_FACE_WORKMAN;
            break;

        case PERSON_TRAMP:
            if (estate) {
                face = PANEL_FACE_BOSS_GOATIE;
            } else {
                face = PANEL_FACE_TRAMP;
            }
            break;

        case PERSON_MIB1:
        case PERSON_MIB2:
        case PERSON_MIB3:
            face = PANEL_FACE_MIB;
            break;

        default:
            ASSERT(0);
            face = PANEL_FACE_RADIO;
            break;
        }
    }

    ASSERT(WITHIN(face, 0, PANEL_FACE_NUMBER - 1));

    PANEL_Face* pf;
    float uvwidth;
    float draw_width;

    switch (size) {
    case PANEL_FACE_LARGE:
        pf = &face_large[face];
        draw_width = 64;
        uvwidth = 64 / 256.0F;
        break;

    case PANEL_FACE_SMALL:
        pf = &face_small[face];
        draw_width = 32;
        uvwidth = 32 / 256.0F;
        break;

    default:
        ASSERT(0);
        return;
    }

    float u = float(pf->u) * (32.0F / 256.0F);
    float v = float(pf->v) * (32.0F / 256.0F);
    int page = (pf->page) ? POLY_PAGE_FACE2 : POLY_PAGE_FACE1;

    PANEL_draw_quad(x, y, x + draw_width, y + draw_width, page, 0xffffffff,
        u, v, u + uvwidth, v + uvwidth);

#undef PANEL_FACE_RADIO
#undef PANEL_FACE_DARCI
#undef PANEL_FACE_ROPER
#undef PANEL_FACE_THUG_RASTA
#undef PANEL_FACE_THUG_GREY
#undef PANEL_FACE_THUG_RED
#undef PANEL_FACE_CIV_1
#undef PANEL_FACE_CIV_2
#undef PANEL_FACE_CIV_3
#undef PANEL_FACE_CIV_4
#undef PANEL_FACE_SLAG
#undef PANEL_FACE_FAT_SLAG
#undef PANEL_FACE_HOSTAGE
#undef PANEL_FACE_WORKMAN
#undef PANEL_FACE_BOSS_GOATIE
#undef PANEL_FACE_BOSS_SHAVEN
#undef PANEL_FACE_COP
#undef PANEL_FACE_COMMISSIONER
#undef PANEL_FACE_MIB
#undef PANEL_FACE_1920S_MAN
#undef PANEL_FACE_1920S_GIRL
#undef PANEL_FACE_TRAMP
#undef PANEL_FACE_NUMBER
}

// Resets the speech text queue.
// uc_orig: PANEL_new_text_init (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_text_init(void)
{
    memset(PANEL_text, 0, sizeof(PANEL_text));
    PANEL_text_head = 0;
    PANEL_text_tail = 0;
    PANEL_text_tick = 0;
}
