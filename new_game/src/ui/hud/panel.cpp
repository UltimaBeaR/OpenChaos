// HUD panel system: health bars, gun sights, timers, inventory,
// text messages, and full-screen effects (fade, widescreen, signs).
// Renders directly into the poly submission buffer.
//
// Chunk 1: core drawing utilities + icon tables + beat/toss/face/text init

#include "version.h"
#include "ui/hud/panel.h"
#include "ui/hud/panel_globals.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/pipeline/aeng.h"

// Scene FBO dimensions (defined in d3d/display_globals.cpp).
extern SLONG ScreenWidth;
extern SLONG ScreenHeight;
#include "engine/graphics/text/font2d.h"
#include "engine/graphics/text/font2d_globals.h"


// Thing, Class constants, person types, special items
#include "things/core/thing.h"
#include "things/core/thing_globals.h"
#include "things/characters/person.h"
#include "things/items/special.h"
#include "things/items/special_globals.h"

// MFX sound playback (MFX_play_ambient)
#include "engine/audio/mfx.h"
// S_RADIO_MESSAGE sound ID
#include "assets/sound_id.h"

#include "missions/eway.h"


// FC_cam[] for compass bearing calculation in PANEL_last
#include "camera/fc.h"
#include "camera/fc_globals.h"

// (DAG violation: ui/ → ai/, pre-existing coupling from original)
#include "ai/pcom.h"

// (DAG violation: ui/ → actors/, pre-existing coupling from original)
#include "things/core/thing.h"
#include "things/core/thing_globals.h"

// (DAG violation: ui/ → missions/, pre-existing coupling from original)
#include "missions/memory_globals.h"

// XLAT_str, X_COMPLETE, X_SEARCHING for search-mode text
#include "assets/xlat_str.h"

// Keys[], KB_V for version number display
#include "engine/input/keyboard.h"
#include "engine/input/keyboard_globals.h"

// ENV_get_value_number for PSX-mode check
#include "engine/io/env.h"

// STATE_DEAD, STATE_SEARCH, SUB_STATE_DEAD_INJURED for beacon/scanner logic
#include "things/core/statedef.h"

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

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
    // Stub — static depth value is always 0.9f.
}

// Draws a textured quad at fixed screen coordinates using the depth bodge
// to place it on top of the 3D scene.
// uc_orig: PANEL_draw_quad (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_quad(
    float left,
    float top,
    float right,
    float bottom,
    SLONG page,
    ULONG colour,
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

// Queues a countdown timer display for PANEL_draw_buffered() to render.
// time is in hundredths of a second.
// uc_orig: PANEL_draw_timer (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_timer(SLONG time, SLONG x, SLONG y)
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
    // Timer text draws at (m_iPanelXPos+171, m_iPanelYPos-118) — inside the
    // radar panel (upper portion). Must share the radar's LEFT_BOTTOM scope
    // so g_frame_scale applies uniformly; default scope uses uniform_scale
    // (ScreenH/480) which oversizes the timer on portrait and lands it in
    // mid-scene instead of inside the radar.
    PolyPage::UIModeScope _timer_scope(ui_coords::UIAnchor::LEFT_BOTTOM);

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
void PANEL_draw_local_health(SLONG mx, SLONG my, SLONG mz, SLONG percentage, SLONG radius)
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

    extern void POLY_add_rect(POLY_Point* p1, SLONG width, SLONG height, SLONG page, unsigned char sort_to_front);

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
void PANEL_draw_gun_sight(SLONG mx, SLONG my, SLONG mz, SLONG accuracy, SLONG scale)
{
    SLONG angle, cangle;
    SLONG c0;
    SLONG dx1, dy1, dx2, dy2;
    POLY_Point p1, p2, pstart;
    SLONG r_in, r_out;
    ULONG col;
    SLONG sat_acc;

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

// uc_orig: PANEL_new_face (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_face(struct Thing* who, float x, float y, SLONG size)
{
    SLONG face;

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

// Queues a floating speech text message above the given thing (NULL = radio message).
// Plays a radio sound for NULL-thing messages. Deduplicates identical messages.
// uc_orig: PANEL_new_text (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_text(Thing* who, SLONG delay, CBYTE* fmt, ...)
{
    CBYTE* ch;
    PANEL_Text* pt;

    if (fmt == NULL) {
        return;
    }

    for (ch = fmt; *ch; ch++) {
        if (!isspace(*ch)) {
            goto found_non_white_space;
        }
    }

    return;

found_non_white_space:;

    CBYTE message[1024];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(message, fmt, ap);
    va_end(ap);

    if (strlen(message) >= PANEL_TEXT_MAX_LENGTH) {
        ASSERT(0);
        return;
    }

    /*
    for (ch = message; *ch; *ch++ = toupper(*ch));
    */

    SLONG i;

    for (i = 0; i < PANEL_MAX_TEXTS; i++) {
        pt = &PANEL_text[i];

        if (pt->delay) {
            if (pt->who == who) {
                if (strcmp(pt->text, message) == 0) {
                    pt->delay = delay;
                    return;
                }
            }
        }
    }

    pt = &PANEL_text[PANEL_text_tail & (PANEL_MAX_TEXTS - 1)];

    pt->who = who;
    pt->delay = delay;
    pt->turns = 0;

    strcpy(pt->text, message);

    PANEL_text_tail += 1;

    if (!who)
        MFX_play_ambient(0, S_RADIO_MESSAGE, 0);
}

// Advances the decay timer for all active text messages and increments turn counters.
// Called once per frame from PANEL_last(). Processes at 10 Hz but caps catch-up at 4 frames.
// uc_orig: PANEL_new_text_process (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_text_process(void)
{
    SLONG i;
    PANEL_Text* pt;
    uint64_t now = sdl3_get_ticks();

    if (PANEL_text_tick < now - (1000 / 10) * 4) {
        PANEL_text_tick = (now - (1000 / 10) * 4);
    }

    while (PANEL_text_tick < now) {
        PANEL_text_tick += 1000 / 10;

        for (i = 0; i < PANEL_MAX_TEXTS; i++) {
            pt = &PANEL_text[i];

            if (pt->delay) {
                pt->delay -= 1000 / 10;

                if (pt->delay < 0) {
                    pt->delay = 0;
                }
            }
        }
    }

    for (i = 0; i < PANEL_MAX_TEXTS; i++) {
        pt = &PANEL_text[i];
        pt->turns += 1;
    }
}

// Sets a tutorial/help text string that displays for 5 seconds in a speech bubble.
// uc_orig: PANEL_new_help_message (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_help_message(CBYTE* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf(PANEL_help_message, fmt, ap);
    va_end(ap);

    PANEL_help_timer = 160 * 20 * 5; // 5 seconds
}

// Draws the widescreen letterbox bars and conversation text during cut-scene dialogue.
// Scans the text queue for new messages and assigns them to top/bottom speaker slots.
// uc_orig: PANEL_new_widescreen (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_widescreen(void)
{
    extern float POLY_screen_clip_top;
    extern float POLY_screen_clip_bottom;

    POLY_screen_clip_top = 0.0F;
    POLY_screen_clip_bottom = float(DisplayHeight);

    // Speaker / text-queue bookkeeping (no rendering).
    if (EWAY_conversation_happening(
            &PANEL_wide_top_person,
            &PANEL_wide_bot_person)) {
        if (PANEL_wide_top_person > PANEL_wide_bot_person) {
            SWAP(PANEL_wide_top_person, PANEL_wide_bot_person);
        }
    }

    SLONG i;
    PANEL_Text* pt;

    for (i = 0; i < PANEL_MAX_TEXTS; i++) {
        pt = &PANEL_text[i];

        if (pt->delay && pt->turns <= 1) {
            if (pt->who == NULL) {
                strcpy(PANEL_wide_text, pt->text);
                PANEL_wide_top_is_talking = UC_FALSE;
                pt->delay = 0;
            } else {
                if (THING_NUMBER(pt->who) == PANEL_wide_top_person) {
                    strcpy(PANEL_wide_text, pt->text);
                    PANEL_wide_top_is_talking = UC_TRUE;
                    pt->delay = 0;
                } else {
                    strcpy(PANEL_wide_text, pt->text);
                    PANEL_wide_top_is_talking = UC_FALSE;
                    PANEL_wide_bot_person = THING_NUMBER(pt->who);
                    pt->delay = 0;
                }
            }
        }
    }

    // Top-speaker content pinned to the FBO top edge (bar backdrop + content
    // both framed so the orange block keeps its original size). 3D viewport
    // letterbox (wideify in POLY_camera_set) is adjusted to match this bar.
    {
        PolyPage::UIModeScope _top_scope(ui_coords::UIAnchor::CENTER_TOP);

        PANEL_draw_quad(
            0.0F,
            0.0F,
            float(DisplayWidth),
            80.0F,
            POLY_PAGE_COLOUR,
            0x00000000);

        if (PANEL_wide_top_person != NULL) {
            PANEL_new_face(
                TO_THING(PANEL_wide_top_person),
                float(DisplayWidth) - 72.0F,
                8.0F,
                PANEL_FACE_LARGE);
        }

        if (PANEL_wide_text[0] && PANEL_wide_top_is_talking) {
            SLONG iYpos = FONT2D_DrawStringRightJustify(
                PANEL_wide_text,
                DisplayWidth - 80,
                0,
                0xff0ffff,
                256,
                POLY_PAGE_FONT2D,
                0,
                UC_TRUE);
            iYpos = 75 - iYpos;
            FONT2D_DrawStringRightJustify(
                PANEL_wide_text,
                DisplayWidth - 80,
                iYpos,
                0xff0ffff,
                256,
                POLY_PAGE_FONT2D,
                0,
                UC_FALSE);
        }
    }

    // Bottom-speaker content pinned to the FBO bottom edge (symmetric).
    {
        PolyPage::UIModeScope _bot_scope(ui_coords::UIAnchor::CENTER_BOTTOM);

        PANEL_draw_quad(
            0.0F,
            float(DisplayHeight) - 80.0F,
            float(DisplayWidth),
            float(DisplayHeight),
            POLY_PAGE_COLOUR,
            0x00000000);

        if (PANEL_wide_text[0]) {
            PANEL_new_face(
                (PANEL_wide_bot_person) ? TO_THING(PANEL_wide_bot_person) : NULL,
                8.0F,
                float(DisplayHeight) - 72.0F,
                PANEL_FACE_LARGE);

            if (!PANEL_wide_top_is_talking) {
                FONT2D_DrawStringWrap(
                    PANEL_wide_text,
                    80,
                    DisplayHeight - 80 + 5,
                    0xffffff);
            }
        }
    }
}

// Initialises the poly frame for a new HUD render pass.
// uc_orig: PANEL_start (fallen/DDEngine/Source/panel.cpp)
void PANEL_start(void)
{
    POLY_frame_init(UC_FALSE, UC_FALSE);
}

// Flushes and draws the HUD poly frame (with depth sorting and clearing).
// uc_orig: PANEL_finish (fallen/DDEngine/Source/panel.cpp)
void PANEL_finish(void)
{
    POLY_frame_draw(UC_TRUE, UC_TRUE);
}

// Resets the fadeout timer. Call once before starting a level.
// uc_orig: PANEL_fadeout_init (fallen/DDEngine/Source/panel.cpp)
void PANEL_fadeout_init(void)
{
    PANEL_fadeout_time = 0;
}

// Arms the fadeout: records the timestamp. No-op if already running.
// uc_orig: PANEL_fadeout_start (fallen/DDEngine/Source/panel.cpp)
void PANEL_fadeout_start(void)
{
    if (!PANEL_fadeout_time) {
        PANEL_fadeout_time = sdl3_get_ticks();
    }
}

// Draws a spinning/zooming full-screen texture fade-out effect.
// After 768ms adds a black vignette in the centre that expands.
// uc_orig: PANEL_fadeout_draw (fallen/DDEngine/Source/panel.cpp)
void PANEL_fadeout_draw(void)
{
    if (PANEL_fadeout_time) {
        // Size the fade-out quad so the cat disc (diameter ≈ quad_width × 0.5
        // at full UV [0,1]) describes the diagonal of the active render
        // target — i.e. covers every pixel the game can draw to. Invariant
        // I5 of the FBO-as-virtual-screen refactor: full-screen effects
        // cover the entire target.
        //
        // Use ui_coords::g_screen_w/h_px instead of ScreenWidth/Height: the
        // latter is the scene FBO size, but PANEL_fadeout_draw runs in the
        // post-composition UI pass on the default FB at dst rect resolution.
        // At OC_RENDER_SCALE < 1.0 the FBO is smaller than the dst rect
        // (e.g. RENDER_SCALE=0.25 → FBO 1/4 of dst rect), so sizing the cat
        // quad to FBO dims would shrink it to a small upper-left patch on
        // the default FB. ui_coords is recomputed to dst rect dims at the
        // start of the UI pass, so it tracks the right target automatically.
        const float vp_w  = ui_coords::g_screen_w_px;
        const float vp_h  = ui_coords::g_screen_h_px;
        const float diag  = sqrtf(vp_w * vp_w + vp_h * vp_h);
        // Cat disc occupies ~half the texture width (cat_radius ≈ 0.25 of
        // texture width), so its visible diameter = quad_width × 0.5. To
        // make the disc circumscribe the target (describe every corner),
        // quad_width must be 2 × target diagonal.
        const float scale = diag * 2.0F / float(DisplayWidth);

        // Temporarily override the affine so the virtual 640×480 quad
        // below maps to a centred rectangle of the chosen size. The
        // default uniform 4:3 affine would only cover the framed region.
        const float saved_xs = PolyPage::s_XScale;
        const float saved_ys = PolyPage::s_YScale;
        const float saved_xo = PolyPage::s_XOffset;
        const float saved_yo = PolyPage::s_YOffset;
        PolyPage::s_XScale  = scale;
        PolyPage::s_YScale  = scale;
        PolyPage::s_XOffset = (vp_w - float(DisplayWidth)  * scale) * 0.5f;
        PolyPage::s_YOffset = (vp_h - float(DisplayHeight) * scale) * 0.5f;

        POLY_frame_init(UC_FALSE, UC_FALSE);

        float angle = float(sdl3_get_ticks() - PANEL_fadeout_time) * angle_mul;
        float zoom = angle * zoom_mul;

        float xdu = -(float)cos(angle) * zoom * 1.33F;
        float xdv = (float)sin(angle) * zoom * 1.33F;

        float ydu = (float)sin(angle) * zoom;
        float ydv = (float)cos(angle) * zoom;

        SLONG colour;

        colour = 0xffffffff;

        POLY_Point pp[4];
        POLY_Point* quad[4];

        float fWDepthBodge = PANEL_GetNextDepthBodge();
        float fZDepthBodge = 1.0f - fWDepthBodge;

        pp[0].X = 0.0F;
        pp[0].Y = 0.0F;
        pp[0].z = fZDepthBodge;
        pp[0].Z = fWDepthBodge;
        pp[0].u = 0.5F - xdu - ydu;
        pp[0].v = 0.5F - xdv - ydv;
        pp[0].colour = colour;
        pp[0].specular = 0xff000000;

        pp[1].X = 640.0F;
        pp[1].Y = 0.0F;
        pp[1].z = fZDepthBodge;
        pp[1].Z = fWDepthBodge;
        pp[1].u = 0.5F + xdu - ydu;
        pp[1].v = 0.5F + xdv - ydv;
        pp[1].colour = colour;
        pp[1].specular = 0xff000000;

        pp[2].X = 0.0F;
        pp[2].Y = 480.0F;
        pp[2].z = fZDepthBodge;
        pp[2].Z = fWDepthBodge;
        pp[2].u = 0.5F - xdu + ydu;
        pp[2].v = 0.5F - xdv + ydv;
        pp[2].colour = colour;
        pp[2].specular = 0xff000000;

        pp[3].X = 640.0F;
        pp[3].Y = 480.0F;
        pp[3].z = fZDepthBodge;
        pp[3].Z = fWDepthBodge;
        pp[3].u = 0.5F + xdu + ydu;
        pp[3].v = 0.5F + xdv + ydv;
        pp[3].colour = colour;
        pp[3].specular = 0xff000000;

        quad[0] = &pp[0];
        quad[1] = &pp[1];
        quad[2] = &pp[2];
        quad[3] = &pp[3];

        POLY_add_quad(quad, POLY_PAGE_FADECAT, UC_FALSE, UC_TRUE);

        if (sdl3_get_ticks() > (uint64_t)PANEL_fadeout_time + 768) {
            SLONG bright;

            bright = sdl3_get_ticks() - (PANEL_fadeout_time + 768);

            SATURATE(bright, 0, 255);

            colour = bright << 24;

#define PANEL_BLACKEN_WIDTH 70

            PANEL_draw_quad(
                320.0F - PANEL_BLACKEN_WIDTH,
                240.0F - PANEL_BLACKEN_WIDTH,
                320.0F + PANEL_BLACKEN_WIDTH,
                240.0F + PANEL_BLACKEN_WIDTH,
                POLY_PAGE_ALPHA,
                colour);
        }

        POLY_frame_draw(UC_FALSE, UC_FALSE);

        // Restore caller affine.
        PolyPage::s_XScale  = saved_xs;
        PolyPage::s_YScale  = saved_ys;
        PolyPage::s_XOffset = saved_xo;
        PolyPage::s_YOffset = saved_yo;
    }
}

// Returns UC_TRUE once the fade-out effect has been active for more than 1024ms.
// uc_orig: PANEL_fadeout_finished (fallen/DDEngine/Source/panel.cpp)
SLONG PANEL_fadeout_finished(void)
{
    if (PANEL_fadeout_time) {
        if (sdl3_get_ticks() > (uint64_t)PANEL_fadeout_time + 1024) {
            return UC_TRUE;
        }
    }

    return UC_FALSE;
}

// PANEL_lsprite defined in panel_globals.cpp — see panel.h for extern declaration.

// Draws a rotated arrow or dot icon on the HUD scanner.
// angle: camera-relative bearing in radians; size: display scale.
// uc_orig: PANEL_last_arrow (fallen/DDEngine/Source/panel.cpp)
void PANEL_last_arrow(float x, float y, float angle, float size, ULONG colour, UBYTE is_dot)
{
    PANEL_Lsprite* pls;
    if (is_dot)
        pls = &PANEL_lsprite[PANEL_LSPRITE_DOT];
    else
        pls = &PANEL_lsprite[PANEL_LSPRITE_ARROW];

    float dx = sin(angle) * size;
    float dy = cos(angle) * size;

    POLY_Point pp[4];
    POLY_Point* quad[4];

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    float fWDepthBodge = PANEL_GetNextDepthBodge();
    float fZDepthBodge = 1.0f - fWDepthBodge;

    pp[0].X = x + dx + (-dy);
    pp[0].Y = y + dy + (+dx);
    pp[0].z = fZDepthBodge;
    pp[0].Z = fWDepthBodge;
    pp[0].x = 0.0F;
    pp[0].y = 0.0F;
    pp[0].u = pls->u1;
    pp[0].v = pls->v1;
    pp[0].colour = colour;
    pp[0].specular = 0xff000000;

    pp[1].X = x + dx - (-dy);
    pp[1].Y = y + dy - (+dx);
    pp[1].z = fZDepthBodge;
    pp[1].Z = fWDepthBodge;
    pp[1].x = 0.0F;
    pp[1].y = 0.0F;
    pp[1].u = pls->u2;
    pp[1].v = pls->v1;
    pp[1].colour = colour;
    pp[1].specular = 0xff000000;

    pp[2].X = x - dx + (-dy);
    pp[2].Y = y - dy + (+dx);
    pp[2].z = fZDepthBodge;
    pp[2].Z = fWDepthBodge;
    pp[2].x = 0.0F;
    pp[2].y = 0.0F;
    pp[2].u = pls->u1;
    pp[2].v = pls->v2;
    pp[2].colour = colour;
    pp[2].specular = 0xff000000;

    pp[3].X = x - dx - (-dy);
    pp[3].Y = y - dy - (+dx);
    pp[3].z = fZDepthBodge;
    pp[3].Z = fWDepthBodge;
    pp[3].x = 0.0F;
    pp[3].y = 0.0F;
    pp[3].u = pls->u2;
    pp[3].v = pls->v2;
    pp[3].colour = colour;
    pp[3].specular = 0xff000000;

    POLY_add_quad(quad, pls->page, UC_FALSE, UC_TRUE);
}

// Draws a 9-piece resizable speech bubble frame from (x1,y1) to (x2,y2).
// Uses the PANEL_LSPRITE_TEXT_BOX atlas region with 8-pixel corner pieces.
// uc_orig: PANEL_last_bubble (fallen/DDEngine/Source/panel.cpp)
void PANEL_last_bubble(float x1, float y1, float x2, float y2)
{
    POLY_Point pp[16];

#define SET_PP(qn, qx, qy, qu, qv)    \
    {                                 \
        pp[qn].x = 0.0F;              \
        pp[qn].y = 0.0F;              \
        pp[qn].X = qx;                \
        pp[qn].Y = qy;                \
        pp[qn].z = fZDepthBodge;      \
        pp[qn].Z = fWDepthBodge;      \
        pp[qn].u = qu;                \
        pp[qn].v = qv;                \
        pp[qn].colour = 0xffffffff;   \
        pp[qn].specular = 0xff000000; \
    }

    float fWDepthBodge = PANEL_GetNextDepthBodge();
    float fZDepthBodge = 1.0f - fWDepthBodge;

    PANEL_Lsprite* pls = &PANEL_lsprite[PANEL_LSPRITE_TEXT_BOX];

    SET_PP(0, x1, y1, pls->u1, pls->v1);
    SET_PP(1, x1 + 8.0F, y1, pls->u1 + (8.0F / 256.0F), pls->v1);
    SET_PP(2, x2 - 8.0F, y1, pls->u2 - (8.0F / 256.0F), pls->v1);
    SET_PP(3, x2, y1, pls->u2, pls->v1);

    SET_PP(4, x2, y1 + 8.0F, pls->u2, pls->v1 + (8.0F / 256.0F));
    SET_PP(5, x2, y2 - 8.0F, pls->u2, pls->v2 - (8.0F / 256.0F));

    SET_PP(6, x2, y2, pls->u2, pls->v2);
    SET_PP(7, x2 - 8.0F, y2, pls->u2 - (8.0F / 256.0F), pls->v2);
    SET_PP(8, x1 + 8.0F, y2, pls->u1 + (8.0F / 256.0F), pls->v2);
    SET_PP(9, x1, y2, pls->u1, pls->v2);

    SET_PP(10, x1, y2 - 8.0F, pls->u1, pls->v2 - (8.0F / 256.0F));
    SET_PP(11, x1, y1 + 8.0F, pls->u1, pls->v1 + (8.0F / 256.0F));

    SET_PP(12, x1 + 8.0F, y1 + 8.0F, pls->u1 + (8.0F / 256.0F), pls->v1 + (8.0F / 256.0F));
    SET_PP(13, x2 - 8.0F, y1 + 8.0F, pls->u2 - (8.0F / 256.0F), pls->v1 + (8.0F / 256.0F));
    SET_PP(14, x2 - 8.0F, y2 - 8.0F, pls->u2 - (8.0F / 256.0F), pls->v2 - (8.0F / 256.0F));
    SET_PP(15, x1 + 8.0F, y2 - 8.0F, pls->u1 + (8.0F / 256.0F), pls->v2 - (8.0F / 256.0F));

    SLONG i;

    POLY_Point* quad[4];

    struct
    {
        UBYTE p1;
        UBYTE p2;
        UBYTE p3;
        UBYTE p4;
    } blah[9] = {
        { 0, 1, 11, 12 },
        { 1, 2, 12, 13 },
        { 2, 3, 13, 4 },
        { 11, 12, 10, 15 },
        { 12, 13, 15, 14 },
        { 13, 4, 14, 5 },
        { 10, 15, 9, 8 },
        { 15, 14, 8, 7 },
        { 14, 5, 7, 6 }
    };

    for (i = 0; i < 9; i++) {
        quad[0] = &pp[blah[i].p1];
        quad[1] = &pp[blah[i].p2];
        quad[2] = &pp[blah[i].p3];
        quad[3] = &pp[blah[i].p4];

        POLY_add_quad(quad, pls->page, UC_FALSE, UC_TRUE);
    }
}

// Triggers a road-sign flash animation for 3 seconds (5 blinks per second).
// uc_orig: PANEL_flash_sign (fallen/DDEngine/Source/panel.cpp)
void PANEL_flash_sign(SLONG sign, SLONG flip)
{
    PANEL_sign_time = sdl3_get_ticks();
    PANEL_sign_flip = flip;
    PANEL_sign_which = sign;
}

// Displays a short info message at the bottom of the HUD panel for 2 seconds.
// uc_orig: PANEL_new_info_message (fallen/DDEngine/Source/panel.cpp)
void PANEL_new_info_message(CBYTE* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf(PANEL_info_message, fmt, ap);
    va_end(ap);

    PANEL_info_time = sdl3_get_ticks();
}

// Draws a dark transparent overlay at the right edge of the screen, x pixels wide.
// Used to indicate restricted zone boundary.
// uc_orig: PANEL_darken_screen (fallen/DDEngine/Source/panel.cpp)
void PANEL_darken_screen(SLONG x)
{
    PANEL_draw_quad(640.0F - float(x), 0.0F, 640.0F, 480.0F, POLY_PAGE_ALPHA_OVERLAY, 0x88000000);
}

// Converts a LASTPANEL_ALPHA page index to its ADD equivalent.
// uc_orig: BodgePageIntoAdd (fallen/DDEngine/Source/panel.cpp)
SLONG BodgePageIntoAdd(SLONG oldpage)
{
    if (oldpage == POLY_PAGE_LASTPANEL_ALPHA)
        return POLY_PAGE_LASTPANEL_ADD;
    return POLY_PAGE_LASTPANEL2_ADD;
}

// Draws a single weapon sprite at (x,y) in the pop-up inventory bar.
// sel != 0 = currently selected (full brightness); sel == 0 = faded.
// uc_orig: PANEL_inv_weapon (fallen/DDEngine/Source/panel.cpp)
void PANEL_inv_weapon(SLONG x, SLONG y, SLONG item, UBYTE who, SLONG rgb, UBYTE sel)
{
    SLONG sprite = -1, faded;

    switch (item) {
    case SPECIAL_GUN:
        sprite = PANEL_LSPRITE_PISTOL;
        break;
    case SPECIAL_SHOTGUN:
        sprite = PANEL_LSPRITE_SHOTGUN;
        break;
    case SPECIAL_AK47:
        sprite = PANEL_LSPRITE_AK47;
        break;
    case SPECIAL_EXPLOSIVES:
        sprite = PANEL_LSPRITE_EXPLOSIVES;
        break;
    case SPECIAL_GRENADE:
        sprite = PANEL_LSPRITE_GRENADE;
        break;
    case SPECIAL_KNIFE:
        sprite = PANEL_LSPRITE_KNIFE;
        break;
    case SPECIAL_BASEBALLBAT:
        sprite = PANEL_LSPRITE_BBB;
        break;
    default:
        sprite = PANEL_LSPRITE_FIST;
        break;
    }

    ASSERT(WITHIN(sprite, 0, PANEL_LSPRITE_NUMBER - 1));

    PANEL_Lsprite* pls = &PANEL_lsprite[sprite];

    float uwidth = (pls->u2 - pls->u1) * 256.0F;
    float vwidth = (pls->v2 - pls->v1) * 256.0F;

    faded = (rgb & 0xff) >> 1;
    faded |= (faded << 8) | (faded << 16) | (faded << 24);
    if (!sel)
        rgb = faded;

    PANEL_draw_quad(
        (float)x - uwidth * 0.25F,
        (float)y - vwidth * 0.25F,
        (float)x + uwidth * 0.25F,
        (float)y + vwidth * 0.25F,
        BodgePageIntoAdd(pls->page),
        rgb,
        pls->u1,
        pls->v1,
        pls->u2,
        pls->v2);
}

// Draws the pop-up weapon inventory panel to the right of the HUD widget.
// Only shown when the player has the inventory visible (PopupFade != 0)
// and is not driving. Highlights the currently selected weapon.
// uc_orig: PANEL_inventory (fallen/DDEngine/Source/panel.cpp)
void PANEL_inventory(Thing* darci, Thing* player)
{
#define PANEL_ADDWEAPON(item)         \
    {                                 \
        draw_list[draw_count] = item; \
        draw_count++;                 \
    }
#define ITEM_SEPERATION (150)

    SLONG rgb;
    CBYTE draw_list[10];
    UBYTE draw_count = 0;
    Thing* p_special = NULL;
    SLONG c0;
    UBYTE current_item = 0;
    SLONG sel;

    UWORD CONTROLS_inv_fade = player->Genus.Player->PopupFade;

    if (!CONTROLS_inv_fade)
        return;
    if (darci->Genus.Person->Flags & FLAG_PERSON_DRIVING)
        return;

    if (EWAY_stop_player_moving()) {
        return;
    }

    // Weapon-switch popup anchors next to the radar panel (bottom-left).
    // Must use the same LEFT_BOTTOM scope as PANEL_last's radar block so
    // the popup scales identically to the radar on non-4:3 FBOs — without
    // this, default uniform_scale (ScreenH/480) oversized the popup on
    // portrait where g_frame_scale (min of ScreenH/480, ScreenW/640) is
    // smaller.
    PolyPage::UIModeScope _inv_scope(ui_coords::UIAnchor::LEFT_BOTTOM);

    PANEL_ADDWEAPON(0);

    sel = darci->Genus.Person->SpecialUse;

    if (darci->Genus.Person->SpecialList) {
        p_special = TO_THING(darci->Genus.Person->SpecialList);

        while (p_special) {
            ASSERT(p_special->Class == CLASS_SPECIAL);
            if (SPECIAL_info[p_special->Genus.Special->SpecialType].group == SPECIAL_GROUP_ONEHANDED_WEAPON || SPECIAL_info[p_special->Genus.Special->SpecialType].group == SPECIAL_GROUP_TWOHANDED_WEAPON || p_special->Genus.Special->SpecialType == SPECIAL_EXPLOSIVES || p_special->Genus.Special->SpecialType == SPECIAL_WIRE_CUTTER) {
                if (THING_NUMBER(p_special) == darci->Genus.Person->SpecialUse) {
                    current_item = draw_count;
                    sel = p_special->Genus.Special->SpecialType;
                }
                PANEL_ADDWEAPON(p_special->Genus.Special->SpecialType);
            }
            if (p_special->Genus.Special->NextSpecial)
                p_special = TO_THING(p_special->Genus.Special->NextSpecial);
            else
                p_special = NULL;
        }
    }

    if (darci->Flags & FLAGS_HAS_GUN) {
        if (darci->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
            current_item = draw_count;
            sel = SPECIAL_GUN;
        }
        PANEL_ADDWEAPON(SPECIAL_GUN);
    }

    current_item = player->Genus.Player->ItemFocus;

    if (current_item == -1)
        return;

    if (draw_list[current_item] && !sel)
        sel = draw_list[current_item];

    int iYPos, iYInc;
    if (m_iPanelYPos > 300) {
        iYPos = m_iPanelYPos - 150;
        iYInc = -35;
    } else {
        iYPos = m_iPanelYPos + 20;
        iYInc = 35;
    }

    rgb = CONTROLS_inv_fade - 1;
    rgb |= (rgb << 8) | (rgb << 16) | (rgb << 24);
    for (c0 = 0; c0 < draw_count; c0++) {
        if (draw_list[c0] != sel) {
            PANEL_inv_weapon(m_iPanelXPos + 170, iYPos, draw_list[c0], 0, rgb, 0);
        } else {
            PANEL_inv_weapon(m_iPanelXPos + 170, iYPos, draw_list[c0], 0, rgb, 1);
        }
        iYPos += iYInc;
    }

#undef PANEL_ADDWEAPON
#undef ITEM_SEPERATION
}

// uc_orig: PANEL_last (fallen/DDEngine/Source/panel.cpp)
void PANEL_last(void)
{
    bool bPanelIsAtBottomOfScreen = (m_iPanelYPos > 300);

    if (EWAY_tutorial_string) {
        // Darken spans whole FBO (fullscreen tutorial backdrop, not framed).
        {
            PolyPage::FullscreenUIModeScope _darken_scope;
            PANEL_darken_screen(640);
            POLY_frame_draw(UC_FALSE, UC_FALSE);
        }
        POLY_frame_init(UC_FALSE, UC_FALSE);

        // Text bubble stays framed (4:3 centred) so it doesn't stretch.
        PolyPage::UIModeScope _text_scope(ui_coords::UIAnchor::CENTER_CENTER);

#define PANEL_TUT_X 16
#define PANEL_TUT_Y 16

        SLONG height = FONT2D_DrawStringWrap(
            EWAY_tutorial_string,
            PANEL_TUT_X,
            PANEL_TUT_Y,
            0xffffff);

        PANEL_last_bubble(
            PANEL_TUT_X - 6,
            PANEL_TUT_Y - 4,
            float(FONT2D_rightmost_x) + 6.0F,
            PANEL_TUT_Y + height + 4);

        return;
    }

    Thing* darci = NET_PERSON(0);

    if (darci == NULL) {
        return;
    }

    if (EWAY_stop_player_moving()) {
        PANEL_new_widescreen();

        return;
    } else {
        PANEL_wide_top_person = NULL;
        PANEL_wide_bot_person = NULL;
        PANEL_wide_top_is_talking = UC_FALSE;
        PANEL_wide_text[0] = '\000';
    }

    // HUD main panel block: radar / health / weapon / ammo / crime-rate
    // / timer / grenade / beacons. Anchored at FBO bottom-left so on
    // widescreen aspects the panel stays pinned to the corner; on
    // portrait the whole virtual 640×480 layout scales uniformly down by
    // g_frame_scale to fit the screen width. On 4:3 the frame origin is
    // (0, 0) and scale = 1 → pixel-identical to the original layout.
    // Scope closes before PANEL_new_text_process because the message
    // block uses a different anchor (CENTER_BOTTOM).
    {
    PolyPage::UIModeScope _panel_scope(ui_coords::UIAnchor::LEFT_BOTTOM);

    // Draw the panel background sprite.
    PANEL_draw_quad(
        (float)(m_iPanelXPos + 0),
        (float)(m_iPanelYPos - 165),
        (float)(m_iPanelXPos + 212),
        (float)(m_iPanelYPos - 0),
        POLY_PAGE_LASTPANEL_ALPHA,
        0xffffffff,
        0.0F,
        90.0F / 256.0F,
        212.0F / 256.0F,
        1.0F);

    // Determine current weapon/item sprite and ammo text.
    SLONG sprite = -1;
    CBYTE text[64];
    text[0] = '\000';

    if (darci->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
        sprite = PANEL_LSPRITE_LOW_GEAR;
    } else {
        if (darci->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
            sprite = PANEL_LSPRITE_PISTOL;
            if (darci->Genus.Person->ammo_packs_pistol) {
                sprintf(text, "%d\\%d", darci->Genus.Person->Ammo, darci->Genus.Person->ammo_packs_pistol / 15);
            } else {
                sprintf(text, "%d", darci->Genus.Person->Ammo);
            }
        } else if (darci->Genus.Person->SpecialUse) {
            Thing* p_special = TO_THING(darci->Genus.Person->SpecialUse);

            switch (p_special->Genus.Special->SpecialType) {
            case SPECIAL_SHOTGUN:
                sprite = PANEL_LSPRITE_SHOTGUN;
                if (darci->Genus.Person->ammo_packs_shotgun) {
                    sprintf(text, "%d\\%d", p_special->Genus.Special->ammo, darci->Genus.Person->ammo_packs_shotgun / SPECIAL_AMMO_IN_A_SHOTGUN);
                } else {
                    sprintf(text, "%d", p_special->Genus.Special->ammo);
                }
                break;

            case SPECIAL_AK47:
                sprite = PANEL_LSPRITE_AK47;
                if (darci->Genus.Person->ammo_packs_ak47) {
                    sprintf(text, "%d\\%d", p_special->Genus.Special->ammo, darci->Genus.Person->ammo_packs_ak47 / SPECIAL_AMMO_IN_A_AK47);
                } else {
                    sprintf(text, "%d", p_special->Genus.Special->ammo);
                }
                break;

            case SPECIAL_EXPLOSIVES:
                sprite = PANEL_LSPRITE_EXPLOSIVES;
                sprintf(text, "%d", p_special->Genus.Special->ammo);
                break;

            case SPECIAL_GRENADE:
                sprite = PANEL_LSPRITE_GRENADE;
                // If pin is pulled, show countdown.
                if (p_special->SubState == SPECIAL_SUBSTATE_ACTIVATED) {
                    SLONG secsleft;
                    secsleft = p_special->Genus.Special->timer / (16 * 20) + 1;
                    SATURATE(secsleft, 0, 6);

                    ULONG colour, colours[] = { 0xff3300, 0xff8800, 0x88ff00, 0x888888 };
                    if (secsleft < 1)
                        colour = *colours;
                    else if (secsleft > 3)
                        colour = colours[3];
                    else
                        colour = colours[secsleft];

                    sprintf(text, "%d", secsleft);

                    FONT2D_DrawString(
                        text,
                        m_iPanelXPos + 141,
                        m_iPanelYPos - 53,
                        colour,
                        256);
                }
                sprintf(text, "%d", p_special->Genus.Special->ammo);
                break;

            case SPECIAL_KNIFE:
                sprite = PANEL_LSPRITE_KNIFE;
                break;

            case SPECIAL_BASEBALLBAT:
                sprite = PANEL_LSPRITE_BBB;
                break;

            default:
                sprite = PANEL_LSPRITE_QMARK;
                break;
            }
        } else {
            sprite = PANEL_LSPRITE_FIST;
        }
    }

    // Draw the weapon/item icon centred in the panel.
    {
        ASSERT(WITHIN(sprite, 0, PANEL_LSPRITE_NUMBER - 1));

        PANEL_Lsprite* pls = &PANEL_lsprite[sprite];

        float uwidth = (pls->u2 - pls->u1) * 256.0F;
        float vwidth = (pls->v2 - pls->v1) * 256.0F;

        float x1 = (float)(m_iPanelXPos + 170) - uwidth * 0.5F;
        float y1 = (float)(m_iPanelYPos - 63) - vwidth * 0.5F;
        float x2 = (float)(m_iPanelXPos + 170) + uwidth * 0.5F;
        float y2 = (float)(m_iPanelYPos - 63) + vwidth * 0.5F;

        if (ftol(uwidth) & 0x1) {
            x1 -= 0.5F;
            x2 -= 0.5F;
        }
        if (ftol(vwidth) & 0x1) {
            y1 -= 0.5F;
            y2 -= 0.5F;
        }
        // Move the AK47 icon up a bit so it doesn't overlap ammo count.
        if (sprite == PANEL_LSPRITE_AK47) {
            y1 -= 8.0F;
            y2 -= 8.0F;
        }

        PANEL_draw_quad(
            x1, y1,
            x2, y2,
            BodgePageIntoAdd(pls->page),
            0xFFffffff,
            pls->u1, pls->v1,
            pls->u2, pls->v2);
    }

    // Draw the ammo count text.
    if (text) {
        FONT2D_DrawStringRightJustify(
            text,
            m_iPanelXPos + 215,
            m_iPanelYPos - 50,
            0xffffff,
            256 + 64);
    }

    // Draw crime rate if the HUD flag is set.
    if (GAME_FLAGS & GF_SHOW_CRIMERATE) {
        CBYTE crimerate[64];
        sprintf(crimerate, "%d%%", CRIME_RATE);

        if (CRIME_RATE >= 95) {
            static UWORD pulse = 0;
            SLONG colour;
            pulse += (TICK_RATIO * 80) >> TICK_SHIFT;
            colour = (SIN(pulse & 2047) >> 9) + 128;
            colour = colour | (colour << 8);
            FONT2D_DrawStringCentred(
                crimerate,
                m_iPanelXPos + 170,
                m_iPanelYPos - 116,
                0xff0000 | colour);
        } else {
            FONT2D_DrawStringCentred(
                crimerate,
                m_iPanelXPos + 170,
                m_iPanelYPos - 116,
                0xffffff);
        }
    }

    // Draw the circular health/stamina arc for the player.
    {
        SLONG i;

        float angle;

        float du;
        float dv;

        float u1;
        float v1;
        float u2;
        float v2;

        float last_u1;
        float last_v1;
        float last_u2;
        float last_v2;

        POLY_Point pp[4];
        POLY_Point* quad[4];
        POLY_Point* tri[3];

        quad[0] = &pp[0];
        quad[1] = &pp[1];
        quad[2] = &pp[2];
        quad[3] = &pp[3];

        tri[0] = &pp[0];
        tri[1] = &pp[1];
        tri[2] = &pp[2];

        static float blah1 = (-43 * 2.0F * PI / 360.0F);
        static float blah2 = (-227 * 2.0F * PI / 360.0F);
        UBYTE is_in_car = darci->Genus.Person->InCar ? 1 : 0;
        float car_offset = is_in_car ? 130.0F : 0.0F;

        Thing* the_car = is_in_car ? TO_THING(darci->Genus.Person->InCar) : 0;

#define PLH_MID_U (71.0F + car_offset)
#define PLH_MID_V (71.0F)
#define PLH_RADIUS (66.0F)
#define PLH_WIDTH (10.0F)
#define PLH_ANGLE1 blah1
#define PLH_ANGLE2 blah2
#define PLH_SEGS (8)
#define PLH_MID_X ((float)(m_iPanelXPos + 74))
#define PLH_MID_Y ((float)(m_iPanelYPos - (256 - 166)))

        angle = PLH_ANGLE1;

        float dangle;
        float fraction;

        dangle = (PLH_ANGLE2 - PLH_ANGLE1) / PLH_SEGS;

        // Fraction of health remaining (different scale for vehicles/Roper).
        fraction = is_in_car ? float(the_car->Genus.Vehicle->Health) * (1.0F / 300.0F) : float(darci->Genus.Person->Health) * ((darci->Genus.Person->PersonType == PERSON_ROPER) ? (1.0F / 400.0F) : (1.0F / 200.0F));

        SATURATE(fraction, 0.0F, 1.0F);

        dangle *= fraction;

        for (i = 0; i <= PLH_SEGS; i++) {
            du = sin(angle);
            dv = cos(angle);

            u1 = (PLH_MID_U + du * (PLH_RADIUS + PLH_WIDTH));
            v1 = (PLH_MID_V + dv * (PLH_RADIUS + PLH_WIDTH));

            u2 = (PLH_MID_U + du * (PLH_RADIUS - PLH_WIDTH));
            v2 = (PLH_MID_V + dv * (PLH_RADIUS - PLH_WIDTH));

            if (i > 0) {
                float fWDepthBodge = PANEL_GetNextDepthBodge();
                float fZDepthBodge = 1.0f - fWDepthBodge;

                pp[0].X = PLH_MID_X + (u1 - PLH_MID_U);
                pp[0].Y = PLH_MID_Y + (v1 - PLH_MID_V);
                pp[0].z = fZDepthBodge;
                pp[0].Z = fWDepthBodge;
                pp[0].x = 0.0F;
                pp[0].y = 0.0F;
                pp[0].u = u1 * (1.0F / 256.0F);
                pp[0].v = v1 * (1.0F / 256.0F);
                pp[0].colour = 0xffffffff;
                pp[0].specular = 0xff000000;

                pp[1].X = PLH_MID_X + (u2 - PLH_MID_U);
                pp[1].Y = PLH_MID_Y + (v2 - PLH_MID_V);
                pp[1].z = fZDepthBodge;
                pp[1].Z = fWDepthBodge;
                pp[1].x = 0.0F;
                pp[1].y = 0.0F;
                pp[1].u = u2 * (1.0F / 256.0F);
                pp[1].v = v2 * (1.0F / 256.0F);
                pp[1].colour = 0xffffffff;
                pp[1].specular = 0xff000000;

                pp[2].X = PLH_MID_X + (last_u1 - PLH_MID_U);
                pp[2].Y = PLH_MID_Y + (last_v1 - PLH_MID_V);
                pp[2].z = fZDepthBodge;
                pp[2].Z = fWDepthBodge;
                pp[2].x = 0.0F;
                pp[2].y = 0.0F;
                pp[2].u = last_u1 * (1.0F / 256.0F);
                pp[2].v = last_v1 * (1.0F / 256.0F);
                pp[2].colour = 0xffffffff;
                pp[2].specular = 0xff000000;

                pp[3].X = PLH_MID_X + (last_u2 - PLH_MID_U);
                pp[3].Y = PLH_MID_Y + (last_v2 - PLH_MID_V);
                pp[3].z = fZDepthBodge;
                pp[3].Z = fWDepthBodge;
                pp[3].x = 0.0F;
                pp[3].y = 0.0F;
                pp[3].u = last_u2 * (1.0F / 256.0F);
                pp[3].v = last_v2 * (1.0F / 256.0F);
                pp[3].colour = 0xffffffff;
                pp[3].specular = 0xff000000;

                POLY_add_quad(quad, POLY_PAGE_LASTPANEL2_ALPHA, UC_FALSE, UC_TRUE);
            }

            last_u1 = u1;
            last_v1 = v1;
            last_u2 = u2;
            last_v2 = v2;

            angle += dangle;
        }
    }

    // Draw the stamina pip marks (coloured squares).
    {
        UBYTE i, stamina = darci->Genus.Person->Stamina / 25;
        SLONG x = m_iPanelXPos + 107;
        SLONG y = m_iPanelYPos - 36;
        SLONG rgb[] = { 0x00FF0000, 0x00C04000, 0x00808000, 0x0040C000, 0x0000ff00 };

        SATURATE(stamina, 0, 5);

        for (i = 0; i < stamina; i++) {
            PANEL_draw_quad(
                x, y - 10,
                x + 10, y,
                POLY_PAGE_LASTPANEL_ADD,
                rgb[i],
                (243.0 / 256.0),
                (198.0 / 256.0),
                (253.0 / 256.0),
                (208.0 / 256.0));
            x += 3;
            y -= 3;
        }
    }

    // Draw navigation beacons on the mini-map scanner.
    {
        SLONG i;

        float dx;
        float dz;
        float dist;
        float dangle;
        float score;

        float x;
        float y;

        MAP_Beacon* mb;

        ULONG colour;

        SLONG best_beacon = NULL;
        float best_score = float(UC_INFINITY);

        for (i = 1; i < MAP_MAX_BEACONS; i++) {
            mb = &MAP_beacon[i];

            if (!mb->used) {
                continue;
            }

            if (mb->track_thing) {
                Thing* p_track = TO_THING(mb->track_thing);

                mb->wx = p_track->WorldPos.X >> 8;
                mb->wz = p_track->WorldPos.Z >> 8;

                extern SLONG is_person_dead(Thing * p_person);

                if (p_track->Class == CLASS_PERSON) {
                    if (p_track->State == STATE_DEAD) {
                        if (p_track->SubState == SUB_STATE_DEAD_INJURED) {
                            if (p_track->Genus.Person->pcom_ai == PCOM_AI_FIGHT_TEST) {
                                continue;
                            }
                        } else {
                            continue;
                        }
                    }
                }
            }

            colour = PANEL_beacon_colour[i % PANEL_MAX_BEACON_COLOURS] | (0xff000000);

            dx = float(mb->wx - (darci->WorldPos.X >> 8));
            dz = float(mb->wz - (darci->WorldPos.Z >> 8));

            dist = sqrt(dx * dx + dz * dz);

            {
                UBYTE is_dot = 0;

                if (PANEL_scanner_poo)
                    dangle = atan2(dx, dz) - float(darci->Draw.Tweened->Angle) * (2.0F * PI / 2048.0F);
                else
                    dangle = atan2(dx, dz) - float(FC_cam[0].yaw >> 8) * (2.0F * PI / 2048.0F);
                score = (float)fmod(dangle, 2.0F * PI) - PI;

                if (score > +PI) {
                    score -= 2.0F * PI;
                }
                if (score < -PI) {
                    score += 2.0F * PI;
                }

                dist /= 16.0F * 256.0F;

                if (dist < 1.0F)
                    is_dot = 1;

                SATURATE(dist, 0.0F, 1.0F);

#define PLS_MID_X PLH_MID_X
#define PLS_MID_Y PLH_MID_Y
#define PLS_RADIUS 50.0F

                {
                    x = PLS_MID_X + (float)sin(dangle) * PLS_RADIUS * dist;
                    y = PLS_MID_Y + (float)cos(dangle) * PLS_RADIUS * dist;

                    float size = (mb->pad && !is_dot) ? 9.0F : 6.0F;

                    int64_t alive = sdl3_get_ticks() - mb->ticks;

                    if (alive < 4096) {
                        alive &= 0x100;

                        SATURATE(alive, 0, 255);

                        colour &= 0x00ffffff;
                        colour |= alive << 24;
                    }

                    PANEL_last_arrow(x, y, dangle, size, colour, is_dot);
                }
            }

            if (fabs(score) < best_score) {
                best_score = fabs(score);
                best_beacon = i;
            }

            mb->pad = UC_FALSE;
        }

        if (PANEL_info_time > sdl3_get_ticks() - 2000) {
            SLONG x_right;

            SLONG colour_main;
            SLONG colour_shad;

            uint64_t now = sdl3_get_ticks();
            uint64_t onfor = now - PANEL_info_time;

            if (onfor < 255) {
                colour_main = onfor;
                colour_shad = onfor;
            } else if (onfor < 768) {
                colour_main = 0xff;
                colour_shad = 255 - (onfor - 256 >> 1);
            } else {
                colour_main = 0xff;
                colour_shad = 0x00;
            }

            x_right = onfor >> 1;

            SATURATE(colour_main, 0, 255);
            SATURATE(colour_shad, 0, 255);
            SATURATE(x_right, 16, 205);

            x_right += m_iPanelXPos;

            colour_main = colour_main | (colour_main << 8) | (colour_main << 16);
            colour_shad = colour_shad | (colour_shad << 8) | (colour_shad << 16);

            {
                FONT2D_DrawStringRightJustifyNoWrap(
                    PANEL_info_message,
                    x_right + 2,
                    m_iPanelYPos - 23 + 2,
                    colour_shad,
                    256);

                FONT2D_DrawStringRightJustifyNoWrap(
                    PANEL_info_message,
                    x_right,
                    m_iPanelYPos - 23,
                    colour_main,
                    256);
            }
        } else {
            if (best_beacon) {
                ASSERT(WITHIN(best_beacon, 1, MAP_MAX_BEACONS - 1));

                mb = &MAP_beacon[best_beacon];

                extern CBYTE* EWAY_get_mess(SLONG index);

                FONT2D_DrawString(
                    EWAY_get_mess(mb->index),
                    m_iPanelXPos + 12 + 2,
                    m_iPanelYPos - 23 + 2,
                    0x00000000,
                    256);

                FONT2D_DrawString(
                    EWAY_get_mess(mb->index),
                    m_iPanelXPos + 12,
                    m_iPanelYPos - 23,
                    PANEL_beacon_colour[best_beacon % PANEL_MAX_BEACON_COLOURS],
                    256);

                mb->pad = UC_TRUE;
            }
        }
    }

    // Draw hostile NPCs on the mini-map scanner dot.
    {
        SLONG i;

        float x;
        float y;
        float dx;
        float dz;
        float dist;
        float dangle;
        float size;
        float flash = fabs(sin(float(GAME_TURN) * 0.2F));
        SLONG display;
        ULONG colour;

        PANEL_Lsprite* pls = &PANEL_lsprite[PANEL_LSPRITE_DOT];

        SLONG num_found = THING_find_sphere(
            darci->WorldPos.X >> 8,
            darci->WorldPos.Y >> 8,
            darci->WorldPos.Z >> 8,
            0x1000,
            THING_array,
            THING_ARRAY_SIZE,
            1 << CLASS_PERSON);

        Thing* p_found;

        for (i = 0; i < num_found; i++) {
            p_found = TO_THING(THING_array[i]);

            if (p_found == darci) {
                continue;
            }

            if (p_found->State == STATE_DEAD) {
                continue;
            }

            display = UC_FALSE;

            switch (p_found->Genus.Person->PersonType) {
            case PERSON_THUG_RASTA:
            case PERSON_THUG_GREY:
            case PERSON_THUG_RED:
                display = UC_TRUE;
                size = 0.5F;
                colour = 0xdd2222;
                break;

            case PERSON_MIB1:
            case PERSON_MIB2:
            case PERSON_MIB3:
                display = UC_TRUE;
                size = 0.5F;
                colour = 0xdddddd;
                break;

            default:
                break;
            }

            if (PCOM_person_wants_to_kill(p_found) == THING_NUMBER(darci)) {
                display = UC_TRUE;
                size = flash;
            }

            if (display) {
                dx = float(p_found->WorldPos.X - darci->WorldPos.X >> 8);
                dz = float(p_found->WorldPos.Z - darci->WorldPos.Z >> 8);

                dist = sqrt(dx * dx + dz * dz);
                dist *= 1.0F / (16.0F * 256.0F);

                if (dist < 1.0F) {
                    if (PANEL_scanner_poo) {
                        dangle = atan2(dx, dz) - float(darci->Draw.Tweened->Angle) * (2.0F * PI / 2048.0F);
                    } else {
                        dangle = atan2(dx, dz) - float(FC_cam[0].yaw >> 8) * (2.0F * PI / 2048.0F);
                    }

                    x = PLS_MID_X + (float)sin(dangle) * PLS_RADIUS * dist;
                    y = PLS_MID_Y + (float)cos(dangle) * PLS_RADIUS * dist;

                    size += 0.5F;
                    size *= 2.0F;

                    PANEL_draw_quad(
                        x - size, y - size,
                        x + size, y + size,
                        POLY_PAGE_LASTPANEL_ADD,
                        colour,
                        pls->u1, pls->v1,
                        pls->u2, pls->v2);
                }
            }
        }
    }

#undef PLH_MID_U
#undef PLH_MID_V
#undef PLH_RADIUS
#undef PLH_WIDTH
#undef PLH_ANGLE1
#undef PLH_ANGLE2
#undef PLH_SEGS
#undef PLH_MID_X
#undef PLH_MID_Y
    } // end panel LEFT_BOTTOM scope

    // Draw floating text messages from NPCs and radio chatter.
    PANEL_new_text_process();

    // Speech / message stack. CENTER_BOTTOM anchor centres the virtual
    // 640×480 frame horizontally in the FBO; the message block (virtual
    // x=214..608) then sits exactly midway between the bottom-left HUD
    // panel and the mirrored right-side gap (algebraic identity: msg
    // centre vs FBO centre = 91×scale = gap centre vs FBO centre). On
    // 4:3 frame origin coincides with the panel-scope origin so x1..x2
    // map to their original virtual coords.
    {
    PolyPage::UIModeScope _msg_scope(ui_coords::UIAnchor::CENTER_BOTTOM);

    {
#define PLT_X 214
#define PLT_Y 360

        SLONG i;
        SLONG ybase;
        SLONG y = m_iPanelYPos - (480 - PLT_Y);
        SLONG x1 = (m_iPanelXPos < 260) ? (m_iPanelXPos + PLT_X) : (32);
        SLONG x2 = (m_iPanelXPos < 260) ? (640 - 32) : (m_iPanelXPos - 16);
        SLONG height;

        PANEL_Text* pt;

        // If the circular text buffer has wrapped past its maximum depth, advance the
        // head to discard stale entries (matches PSX behaviour).
        if ((PANEL_text_tail - PANEL_text_head) > PANEL_MAX_TEXTS) {
            PANEL_text_head = PANEL_text_tail - (PANEL_MAX_TEXTS - 1);
        }

        for (i = PANEL_text_head; i < PANEL_text_tail; i++) {
            pt = &PANEL_text[i & (PANEL_MAX_TEXTS - 1)];

            if (i == PANEL_text_head) {
                if (pt->delay == 0) {
                    PANEL_text_head += 1;
                }
            }

            if (pt->delay != 0) {
                ybase = y;

                PANEL_new_face(
                    pt->who,
                    x1,
                    ybase - 2,
                    PANEL_FACE_SMALL);

                height = FONT2D_DrawStringWrapTo(pt->text,
                             x1 + 36 + 6,
                             y + 2,
                             0xffffff,
                             256,
                             POLY_PAGE_FONT2D,
                             0,
                             x2)
                    - y;
                height += 20;

                PANEL_last_bubble(
                    x1 + 36,
                    ybase - 4,
                    float(FONT2D_rightmost_x) + 6.0F,
                    ybase - 2 + height);

                if (height < 34) {
                    height = 34;
                }

                y += height;
            }
        }

#undef PLT_X
#undef PLT_Y
    }
    } // end msg CENTER_BOTTOM scope

    // Draw road sign flashes. Framed at CENTER_CENTER so the sign stays
    // anchored inside the centred 4:3 region on non-4:3 FBOs (otherwise
    // default uniform-scale put it in the lower-left quadrant on
    // widescreen).
    {
        PolyPage::UIModeScope _sign_scope(ui_coords::UIAnchor::CENTER_CENTER);

        uint64_t dtime = sdl3_get_ticks() - PANEL_sign_time;

        if (dtime < 3000) {
            dtime %= 600;

            if (dtime < 400) {
                float du;
                float dv;

                float u_mid;
                float v_mid;

                u_mid = (PANEL_sign_which & 1) ? 0.75F : 0.25F;
                v_mid = (PANEL_sign_which & 2) ? 0.75F : 0.25F;

                du = (PANEL_sign_flip & PANEL_SIGN_FLIP_LEFT_AND_RIGHT) ? -0.25F : +0.25F;
                dv = (PANEL_sign_flip & PANEL_SIGN_FLIP_TOP_AND_BOTTOM) ? -0.25F : +0.25F;

#define PANEL_SIGN_X 320
                int iYPos;
                if (bPanelIsAtBottomOfScreen) {
                    iYPos = 100;
                } else {
                    iYPos = 480 - 100;
                }

                PANEL_draw_quad(
                    PANEL_SIGN_X - 64,
                    iYPos - 64,
                    PANEL_SIGN_X + 64,
                    iYPos + 64,
                    POLY_PAGE_SIGN,
                    0xffffffff,
                    u_mid - du,
                    v_mid - dv,
                    u_mid + du,
                    v_mid + dv);
#undef PANEL_SIGN_X
            }
        }
    }

    // Draw a progress bar when the player is in search mode. Framed at
    // CENTER_CENTER so the bubble + bar + text stay centred on screen on
    // any aspect.
    {
        PolyPage::UIModeScope _search_scope(ui_coords::UIAnchor::CENTER_CENTER);

        Thing* darci = NET_PERSON(0);

        if (darci) {
            if (darci->State == STATE_SEARCH) {
                float percent;

                percent = darci->Genus.Person->Timer1 * (1.0F / (100.0F * 256.0F));

                SATURATE(percent, 0.0F, 1.0F);

                PANEL_last_bubble(
                    320 - 80,
                    220 - 16,
                    320 + 80,
                    220 + 16);

                PANEL_draw_quad(
                    320 - 75,
                    220 - 12,
                    320 - 75 + 151 * percent,
                    220 + 13,
                    POLY_PAGE_COLOUR,
                    0x7788bb);

                if ((darci->Genus.Person->Timer1 & 0xfff) < 3000 || percent == 1.0F) {
                    CBYTE* text;

                    if (percent == 1.0F) {
                        text = XLAT_str(X_COMPLETE);
                    } else {
                        text = XLAT_str(X_SEARCHING);
                    }

                    FONT2D_DrawStringCentred(text, 320, 220 - 8, 0xffffff);
                }
            }
        }
    }

    // PSX-mode debug indicator (activated by iamapsx env var).
    static SLONG i_know = 0;
    static SLONG the_answer = 0;

    if (!i_know) {
        the_answer = ENV_get_value_number("iamapsx", UC_FALSE);
        i_know = UC_TRUE;
    }

    if (the_answer) {
        FONT2D_DrawString("PSX mode", 7, 17, 0x000000);
        FONT2D_DrawString("PSX mode", 5, 15, 0xff2288);
    }

    // Show version number overlay when V key is held. Gated by
    // allow_debug_keys — the version string is a dev/diagnostic aid,
    // regular players should never see it overlay the HUD.
    {
        extern BOOL allow_debug_keys;
        static ULONG timestamp_colour = 0;
        static CBYTE version_number[128];

        if (Keys[KB_V] && allow_debug_keys) {
            timestamp_colour = 0xf0f0f0f0;
        }

        if (timestamp_colour) {
            if (!version_number[0]) {
#ifdef NDEBUG
                sprintf(version_number, "OpenChaos v%s (%s)", OPENCHAOS_VERSION, __DATE__);
#else
                sprintf(version_number, "OpenChaos v%s DEBUG (%s)", OPENCHAOS_VERSION, __DATE__);
#endif
            }

            timestamp_colour -= 0x10101010;

            FONT2D_DrawString(
                version_number,
                22,
                22,
                0x00000000);

            FONT2D_DrawString(
                version_number,
                20,
                20,
                timestamp_colour);
        }
    }
}

// uc_orig: PANEL_draw_completion_bar (fallen/DDEngine/Source/panel.cpp)
void PANEL_draw_completion_bar(SLONG completion)
{
#define START_R 50
#define START_G 59
#define START_B 80
#define END_R 210
#define END_G 216
#define END_B 208

    SLONG r;
    SLONG g;
    SLONG b;

    POLY_frame_init(UC_FALSE, UC_FALSE);

    SLONG i;
    SLONG colour;

    for (i = 0; i < (completion >> 3); i += 1) {
        r = START_R + (END_R - START_R) * i >> 5;
        g = START_G + (END_G - START_G) * i >> 5;
        b = START_B + (END_B - START_B) * i >> 5;

        colour = (r << 16) | (g << 8) | (b << 0);

        PANEL_draw_quad(
            5 + i * 20, 455,
            23 + i * 20, 475,
            POLY_PAGE_COLOUR,
            colour);
    }

    POLY_frame_draw(UC_FALSE, UC_FALSE);

#undef START_R
#undef START_G
#undef START_B
#undef END_R
#undef END_G
#undef END_B
}

// Random number generator for screensaver bounce direction changes.
// uc_orig: dwGetRandomishNumber (fallen/DDEngine/Source/panel.cpp)
static DWORD dwGetRandomishNumber(void)
{
    dwPseudorandomSeed *= 51929;
    dwPseudorandomSeed ^= dwPseudorandomSeed >> 3;
    dwPseudorandomSeed ^= dwPseudorandomSeed << 8;
    dwPseudorandomSeed += 31415;

    return (dwPseudorandomSeed >> 8);
}

#define SCREENSAVER_SIZE 128

// uc_orig: PANEL_disable_screensaver (fallen/DDEngine/Source/panel.cpp)
void PANEL_disable_screensaver(bool bImmediately)
{
    bScreensaverEnabled = UC_FALSE;
    if (bImmediately) {
        iScreenSaverDarkness = 0;
    }
}

// uc_orig: PANEL_screensaver_draw (fallen/DDEngine/Source/panel.cpp)
void PANEL_screensaver_draw(void)
{
    if (bScreensaverEnabled) {
        iScreenSaverDarkness += 100;
        if (iScreenSaverDarkness > 0xffff) {
            iScreenSaverDarkness = 0xffff;
        }
    } else {
        iScreenSaverDarkness -= 10000;
        if (iScreenSaverDarkness < 0) {
            iScreenSaverDarkness = 0;
        }
    }

    if (iScreenSaverDarkness == 0) {
        return;
    }

    POLY_frame_init(UC_FALSE, UC_FALSE);

    // Advance screensaver position and bounce off screen edges.
    iScreensaverXPos += iScreensaverXInc;
    iScreensaverYPos += iScreensaverYInc;
    iScreensaverAngle += iScreensaverAngleInc;
    if (iScreensaverXPos > 640 - SCREENSAVER_SIZE) {
        iScreensaverXPos = 640 - SCREENSAVER_SIZE;
        iScreensaverXInc = -((signed)(dwGetRandomishNumber() & 0x3) + 2);
        iScreensaverAngleInc = ((signed)(dwGetRandomishNumber() & 0xfff) - 0x7ff);
    } else if (iScreensaverXPos < 0) {
        iScreensaverXPos = 0;
        iScreensaverXInc = ((signed)(dwGetRandomishNumber() & 0x3) + 2);
        iScreensaverAngleInc = ((signed)(dwGetRandomishNumber() & 0xfff) - 0x7ff);
    }

    if (iScreensaverYPos > 480 - SCREENSAVER_SIZE) {
        iScreensaverYPos = 480 - SCREENSAVER_SIZE;
        iScreensaverYInc = -((signed)(dwGetRandomishNumber() & 0x3) + 2);
        iScreensaverAngleInc = ((signed)(dwGetRandomishNumber() & 0xfff) - 0x7ff);
    } else if (iScreensaverYPos < 0) {
        iScreensaverYPos = 0;
        iScreensaverYInc = ((signed)(dwGetRandomishNumber() & 0x3) + 2);
        iScreensaverAngleInc = ((signed)(dwGetRandomishNumber() & 0xfff) - 0x7ff);
    }

    POLY_Point pp[4];
    POLY_Point* quad[4];

    DWORD dwColour = ((iScreenSaverDarkness & 0xff00) << 16);

    float fSinAngle, fCosAngle;
    float fAngle = (float)(iScreensaverAngle & 0xffff) * (2.0f * 3.1415927f / 65536.0f);
    fSinAngle = sinf(fAngle);
    fCosAngle = cosf(fAngle);
    fSinAngle *= 0.65f;
    fCosAngle *= 0.65f;

    // Draw the rotating UC logo sprite.
    pp[0].X = (float)iScreensaverXPos;
    pp[0].Y = (float)iScreensaverYPos;
    pp[0].z = 0.0f;
    pp[0].Z = 0.99999f;
    pp[0].u = 0.5f + fSinAngle;
    pp[0].v = 0.5f + fCosAngle;
    pp[0].colour = dwColour;
    pp[0].specular = 0xff000000;

    pp[1].X = (float)iScreensaverXPos + SCREENSAVER_SIZE;
    pp[1].Y = (float)iScreensaverYPos;
    pp[1].z = 0.0f;
    pp[1].Z = 0.99999f;
    pp[1].u = 0.5f - fCosAngle;
    pp[1].v = 0.5f + fSinAngle;
    pp[1].colour = dwColour;
    pp[1].specular = 0xff000000;

    pp[2].X = (float)iScreensaverXPos;
    pp[2].Y = (float)iScreensaverYPos + SCREENSAVER_SIZE;
    pp[2].z = 0.0f;
    pp[2].Z = 0.99999f;
    pp[2].u = 0.5f + fCosAngle;
    pp[2].v = 0.5f - fSinAngle;
    pp[2].colour = dwColour;
    pp[2].specular = 0xff000000;

    pp[3].X = (float)iScreensaverXPos + SCREENSAVER_SIZE;
    pp[3].Y = (float)iScreensaverYPos + SCREENSAVER_SIZE;
    pp[3].z = 0.0f;
    pp[3].Z = 0.99999f;
    pp[3].u = 0.5f - fSinAngle;
    pp[3].v = 0.5f - fCosAngle;
    pp[3].colour = dwColour;
    pp[3].specular = 0xff000000;

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    POLY_add_quad(quad, POLY_PAGE_FADE_MF, UC_FALSE, UC_TRUE);

    // Draw black bars around the logo.
    // Top block.
    pp[0].X = 0.0f;
    pp[0].Y = 0.0f;
    pp[1].X = 640.0f;
    pp[1].Y = 0.0f;
    pp[2].X = 0.0f;
    pp[2].Y = (float)iScreensaverYPos;
    pp[3].X = 640.0f;
    pp[3].Y = (float)iScreensaverYPos;
    POLY_add_quad(quad, POLY_PAGE_COLOUR_ALPHA, UC_FALSE, UC_TRUE);

    // Bottom block.
    pp[0].X = 0.0f;
    pp[0].Y = (float)iScreensaverYPos + SCREENSAVER_SIZE;
    pp[1].X = 640.0f;
    pp[1].Y = (float)iScreensaverYPos + SCREENSAVER_SIZE;
    pp[2].X = 0.0f;
    pp[2].Y = 480.0f;
    pp[3].X = 640.0f;
    pp[3].Y = 480.0f;
    POLY_add_quad(quad, POLY_PAGE_COLOUR_ALPHA, UC_FALSE, UC_TRUE);

    // Left block.
    pp[0].X = 0.0f;
    pp[0].Y = (float)iScreensaverYPos;
    pp[1].X = (float)iScreensaverXPos;
    pp[1].Y = (float)iScreensaverYPos;
    pp[2].X = 0.0f;
    pp[2].Y = (float)iScreensaverYPos + SCREENSAVER_SIZE;
    pp[3].X = (float)iScreensaverXPos;
    pp[3].Y = (float)iScreensaverYPos + SCREENSAVER_SIZE;
    POLY_add_quad(quad, POLY_PAGE_COLOUR_ALPHA, UC_FALSE, UC_TRUE);

    // Right block.
    pp[0].X = (float)iScreensaverXPos + SCREENSAVER_SIZE;
    pp[0].Y = (float)iScreensaverYPos;
    pp[1].X = 640.0f;
    pp[1].Y = (float)iScreensaverYPos;
    pp[2].X = (float)iScreensaverXPos + SCREENSAVER_SIZE;
    pp[2].Y = (float)iScreensaverYPos + SCREENSAVER_SIZE;
    pp[3].X = 640.0f;
    pp[3].Y = (float)iScreensaverYPos + SCREENSAVER_SIZE;
    POLY_add_quad(quad, POLY_PAGE_COLOUR_ALPHA, UC_FALSE, UC_TRUE);

    POLY_frame_draw(UC_FALSE, UC_FALSE);
}

#undef SCREENSAVER_SIZE
