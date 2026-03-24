#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "engine/graphics/pipeline/aeng.h"  // AENG_flip, AENG_fade_out, AENG_clear_screen
#include "camera/cam.h"
#include "engine/graphics/graphics_api/gd_display.h"   // the_display
#include "engine/graphics/text/font2d.h"
#include "engine/graphics/pipeline/poly.h"
#include "ui/hud/panel.h"
#include "assets/startscr.h"
#define DEMO
#include "ui/attract.h"
#include "engine/io/env.h"
#include "ui/hud/overlay.h"
#include "engine/audio/sound.h"
#include "engine/audio/mfx.h"
#include "missions/eway.h"
#include "game/input_actions.h"
#include "assets/xlat_str.h"
#include "ui/frontend.h"
#include "things/core/statedef.h"

// Enable the new frontend menu system (vs old attract demo playback).
// uc_orig: NEW_FRONTEND (fallen/Source/Attract.cpp)
#define NEW_FRONTEND 1

// Forward declarations for functions still in old/.
// uc_orig: LoadBackImage (fallen/DDLibrary/Source/GDisplay.cpp)
void LoadBackImage(UBYTE* image_data);

// uc_orig: AENG_demo_attract (fallen/DDEngine/Source/aeng.cpp)
void AENG_demo_attract(SLONG x, SLONG y, CBYTE* text);

extern BOOL text_fudge;

// uc_orig: do_start_menu (fallen/Source/frontend.cpp)
extern SLONG do_start_menu(void);

// Main attract/frontend loop. Runs the main menu (FRONTEND_loop) until the player starts a mission.
// Sets GAME_STATE flags and calls ATTRACT_loadscreen_init() when a mission is selected.
// uc_orig: game_attract_mode (fallen/Source/Attract.cpp)
void game_attract_mode(void)
{
    float y;
    SLONG dont_leave_for_a_while = 25;

    extern void NIGHT_init();
    NIGHT_init();

    if (auto_advance) {
        go_into_game = UC_TRUE;
        auto_advance = 0;
        GAME_STATE &= ~GS_ATTRACT_MODE;
        GAME_STATE |= GS_PLAY_GAME;
    } else
        go_into_game = UC_FALSE;

    MFX_load_wave_list();

    bool bReinitBecauseOfLanguageChange = UC_FALSE;
reinit_because_of_language_change:

    FRONTEND_init(bReinitBecauseOfLanguageChange);

    bReinitBecauseOfLanguageChange = UC_FALSE;

    LPDIRECT3DDEVICE3 dev = the_display.lp_D3D_Device;
    D3DVIEWPORT2 vp;
    vp.dwSize = sizeof(vp);
    vp.dwX = the_display.ViewportRect.x1;
    vp.dwY = the_display.ViewportRect.y1;
    vp.dwWidth = the_display.ViewportRect.x2 - the_display.ViewportRect.x1;
    vp.dwHeight = the_display.ViewportRect.y2 - the_display.ViewportRect.y1;
    vp.dvClipX = (float)vp.dwX;
    vp.dvClipY = (float)vp.dwY;
    vp.dvClipWidth = (float)vp.dwWidth;
    vp.dvClipHeight = (float)vp.dwHeight;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    the_display.lp_D3D_Viewport->SetViewport2(&vp);

    dev->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
    dev->SetRenderState(D3DRENDERSTATE_STIPPLEDALPHA, UC_FALSE);
    dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    dev->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, UC_TRUE);
    dev->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, UC_FALSE);
    dev->SetRenderState(D3DRENDERSTATE_SUBPIXEL, UC_TRUE);
    dev->SetRenderState(D3DRENDERSTATE_ZENABLE, UC_TRUE);
    dev->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    dev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, UC_TRUE);
    dev->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRENDERSTATE_FOGENABLE, UC_FALSE);
    dev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, UC_FALSE);
    dev->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_ALWAYS);
    dev->SetRenderState(D3DRENDERSTATE_ANTIALIAS, D3DANTIALIAS_NONE);
    dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    dev->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    dev->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    dev->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_NONE);
    dev->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
    extern LPDIRECT3DTEXTURE2 TEXTURE_get_handle(SLONG page);
    extern SLONG TEXTURE_page_water;
    dev->SetTexture(0, TEXTURE_get_handle(TEXTURE_page_water));

    dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, UC_FALSE);
    dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
    dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);

    y = 500;
    while (SHELL_ACTIVE && (GAME_STATE & GS_ATTRACT_MODE)) {
        if (dont_leave_for_a_while > 0) {
            dont_leave_for_a_while -= 1;
        }

        if (ControlFlag && Keys[KB_Q]) {
            GAME_STATE = 0;
            LastKey = 0;
        }

        if (y < 57.0F) {
            AENG_clear_screen();
        }

        text_fudge = UC_TRUE;

        if (Keys[KB_LEFT] || Keys[KB_RIGHT] || Keys[KB_UP] || Keys[KB_DOWN] || Keys[KB_SPACE] || Keys[KB_ENTER])
            y = 500;

        {
            SLONG res;

            res = FRONTEND_loop();

            if (res) {
                switch (res) {
                case STARTS_PSX:
                    break;
                case STARTS_START:
                    {
                        GAME_STATE &= ~GS_ATTRACT_MODE;
                        GAME_STATE |= GS_PLAY_GAME;
                        go_into_game = UC_TRUE;

                        ATTRACT_loadscreen_init();

                        stop_all_fx_and_music();

                        extern void init_joypad_config(void);
                        init_joypad_config();
                    }
                    break;
                case STARTS_PLAYBACK:
                    GAME_STATE &= ~GS_ATTRACT_MODE;
                    GAME_STATE |= GS_PLAY_GAME;
                    GAME_STATE |= GS_PLAYBACK;
                    go_into_game = UC_TRUE;
                    break;
                case STARTS_EDITOR:
                    GAME_STATE = GS_EDITOR;
                    break;
                case STARTS_MULTI:
                    GAME_STATE = GS_CONFIGURE_NET;
                    break;
                case STARTS_EXIT:
                    GAME_STATE = 0;
                    LastKey = 0;
                    break;
                case STARTS_HOST:
                    break;
                case STARTS_JOIN:
                    break;
                case STARTS_LANGUAGE_CHANGE:
                    ATTRACT_loadscreen_init();
                    bReinitBecauseOfLanguageChange = UC_TRUE;
                    break;
                }
            }
        }

        if (y < 57.0F) {
            AENG_fade_out((57 - SLONG(y)) * 15);
        }

        extern void lock_frame_rate(SLONG fps);
        lock_frame_rate(60);

        if ((GAME_STATE & GS_PLAY_GAME) == 0) {
            AENG_flip();
        }

        if (bReinitBecauseOfLanguageChange) {
            break;
        }
    }

    if (bReinitBecauseOfLanguageChange) {
        goto reinit_because_of_language_change;
    }

    if (GAME_STATE & GS_PLAY_GAME) {
        if (go_into_game) {
            ShowBackImage(UC_FALSE);
            AENG_flip();
        }
    }

    text_fudge = UC_FALSE;
    ResetBackImage();
}


// uc_orig: SCORE_SPACER (fallen/Source/Attract.cpp)
#define SCORE_SPACER 20

// Draws the end-of-mission score screen. Called by GAMEMENU_draw() at GAMEMENU_MENU_TYPE_WON.
// Shows kill/arrest counts, bonus counts, time taken, and "Mucky times" developer records.
// uc_orig: ScoresDraw (fallen/Source/Attract.cpp)
void ScoresDraw(void)
{
    extern SLONG stat_killed_thug;
    extern SLONG stat_killed_innocent;
    extern SLONG stat_arrested_thug;
    extern SLONG stat_arrested_innocent;
    extern SLONG stat_count_bonus;
    // claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
    extern DWORD stat_start_time, stat_game_time;

    SLONG count = 0, count_bonus_left = 0, c0;
    SLONG ticks, h, m, s;
    ticks = stat_game_time;

    h = ticks / (1000 * 60 * 60);
    ticks -= h * 1000 * 60 * 60;
    m = ticks / (1000 * 60);
    ticks -= m * 1000 * 60;
    s = ticks / (1000);

    for (c0 = 0; c0 < MAX_THINGS; c0++) {
        Thing* p_thing;

        p_thing = TO_THING(c0);

        if (p_thing->Class == CLASS_PERSON) {
            if (p_thing->State != STATE_DEAD) {
                switch (p_thing->Genus.Person->PersonType) {
                case PERSON_THUG_RASTA:
                case PERSON_THUG_GREY:
                case PERSON_THUG_RED:
                case PERSON_MIB1:
                case PERSON_MIB2:
                case PERSON_MIB3:
                    count++;
                    break;
                }
            }
        }
        if (p_thing->Class == CLASS_SPECIAL) {
            if (p_thing->Genus.Special->SpecialType == SPECIAL_TREASURE) {
                count_bonus_left++;
            }
        }
    }

    CBYTE str[128];

    extern SLONG playing_real_mission(void);

    if (playing_real_mission()) {
        if (NET_PERSON(0)->Genus.Person->PersonType != PERSON_DARCI && NET_PERSON(0)->Genus.Person->PersonType != PERSON_ROPER) {
            // Score display only works for Darci or Roper.
        } else {
            FONT2D_DrawString(XLAT_str(X_WON_KILLED), 10 + 2, 300 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(XLAT_str(X_WON_ARRESTED), 10 + 2, 320 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(XLAT_str(X_WON_AT_LARGE), 10 + 2, 340 + 2, 0x000000, 256, POLY_PAGE_FONT2D);

            FONT2D_DrawString(XLAT_str(X_WON_KILLED), 10, 300, 0xffffff, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(XLAT_str(X_WON_ARRESTED), 10, 320, 0xffffff, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(XLAT_str(X_WON_AT_LARGE), 10, 340, 0xffffff, 256, POLY_PAGE_FONT2D);
        }

        FONT2D_DrawString(XLAT_str(X_WON_BONUS_FOUND), 10 + 2, 360 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
        FONT2D_DrawString(XLAT_str(X_WON_BONUS_MISSED), 10 + 2, 380 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
        FONT2D_DrawString(XLAT_str(X_WON_TIMETAKEN), 10 + 2, 400 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
        FONT2D_DrawString(XLAT_str(X_WON_MUCKYTIME), 10 + 2, 420 + 2, 0x000000, 256, POLY_PAGE_FONT2D);

        FONT2D_DrawString(XLAT_str(X_WON_BONUS_FOUND), 10, 360, 0xffffff, 256, POLY_PAGE_FONT2D);
        FONT2D_DrawString(XLAT_str(X_WON_BONUS_MISSED), 10, 380, 0xffffff, 256, POLY_PAGE_FONT2D);
        FONT2D_DrawString(XLAT_str(X_WON_TIMETAKEN), 10, 400, 0xffffff, 256, POLY_PAGE_FONT2D);
        FONT2D_DrawString(XLAT_str(X_WON_MUCKYTIME), 10, 420, 0xffffff, 256, POLY_PAGE_FONT2D);

        if (NET_PERSON(0)->Genus.Person->PersonType != PERSON_DARCI && NET_PERSON(0)->Genus.Person->PersonType != PERSON_ROPER) {
            // Score display only works for Darci or Roper.
        } else {
            sprintf(str, ": %d", stat_killed_thug);
            FONT2D_DrawString(str, 300 + 2, 300 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(str, 300, 300, 0xffffff, 256, POLY_PAGE_FONT2D);

            sprintf(str, ": %d", stat_arrested_thug);
            FONT2D_DrawString(str, 300 + 2, 320 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(str, 300, 320, 0xffffff, 256, POLY_PAGE_FONT2D);

            sprintf(str, ": %d", count);
            FONT2D_DrawString(str, 300 + 2, 340 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(str, 300, 340, 0xffffff, 256, POLY_PAGE_FONT2D);
        }

        sprintf(str, ": %d", stat_count_bonus);
        FONT2D_DrawString(str, 300 + 2, 360 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
        FONT2D_DrawString(str, 300, 360, 0xffffff, 256, POLY_PAGE_FONT2D);

        sprintf(str, ": %d", count_bonus_left);
        FONT2D_DrawString(str, 300 + 2, 380 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
        FONT2D_DrawString(str, 300, 380, 0xffffff, 256, POLY_PAGE_FONT2D);

        CBYTE code[32];

        {
            // Developer speedrun records per mission. If player beats them and hasn't cheated, a code is shown.
            // uc_orig: Mime (fallen/Source/Attract.cpp)
            typedef struct
            {
                CBYTE* level_fname;
                CBYTE* level_name;
                SLONG mins;
                SLONG secs;
                CBYTE* who;

            } Mime;

            // uc_orig: mucky_times (fallen/Source/Attract.cpp)
            static Mime mucky_times[] = {
                { "\\FTutor1.ucm", "Combat Tutorial", 0, 0 },
                { "\\Assault1.ucm", "Physical Training", 0, 0 },
                { "\\testdrive1a.ucm", "Driving Bronze", 0, 0 },
                { "\\fight1.ucm", "Combat Bronze", 0, 0 },
                { "\\police1.ucm", "RTA", 1, 5, "Alex Hood" },
                { "\\testdrive2.ucm", "Driving Silver", 0, 0 },
                { "\\fight2.ucm", "Combat Silver", 0, 0 },
                { "\\police2.ucm", "The Jump", 1, 32, "Joe Neate" },
                { "\\testdrive3.ucm", "Driving Gold", 0, 0 },
                { "\\Bankbomb1.ucm", "Nitro Car", 5, 0, "Kenny Tang" },
                { "\\police3.ucm", "Gun Hunt", 1, 37, "Alex Hood" },
                { "\\Gangorder2.ucm", "Rat Catch", 3, 14, "David Babajee" },
                { "\\police4.ucm", "Trouble in the Park", 1, 24, "Kaoski" },
                { "\\bball2.ucm", "Gatecrasher", 0, 45, "Tom Forsyth" },
                { "\\wstores1.ucm", "Arms Breaker", 4, 3, "Joe Neate" },
                { "\\e3.ucm", "Media Trouble", 5, 54, "Kwesi Moodie" },
                { "\\westcrime1.ucm", "Urban Shakedown", 6, 4, "Alex Hood" },
                { "\\carbomb1.ucm", "Auto-Destruct", 4, 18, "David Babajee" },
                { "\\botanicc.ucm", "Grim Gardens", 6, 29, "Kwesi Moodie" },
                { "\\Semtex.ucm", "Semtex", 16, 3, "Kaoski" },
                { "\\AWOL1.ucm", "Cop Killers", 10, 30, "Kenny Tang" },
                { "\\mission2.ucm", "Southside Offensive", 7, 49, "Alex Hood" },
                { "\\park2.ucm", "Psycho Park", 6, 45, "Tom Forsyth" },
                { "\\MIB.ucm", "The Fallen", 26, 53, "Phil Maskell" },
                { "\\skymiss2.ucm", "Stern Revenge", 9, 28, "David Babajee" },
                { "\\factory1.ucm", "Transmission Terminated", 8, 35, "Kwesi Moodie" },
                { "\\estate2.ucm", "Estate of Emergency", 8, 14, "Phil Maskell" },
                { "\\Stealtst1.ucm", "Seek, Sneak and Seize", 5, 28, "Kaoski" },
                { "\\snow2.ucm", "Target UC", 2, 26, "Kaoski" },
                { "\\Gordout1.ucm", "Headline Hostage", 5, 42, "Kenny Tang" },
                { "\\Baalrog3.ucm", "Insane Assault", 2, 38, "Mike Diskett" },
                { "\\Finale1.ucm", "Day Of Reckoning", 40, 00, "Beat me" },
                { "\\Gangorder1.ucm", "Assassin", 5, 21, "Joe Neate" },
                { "\\FreeCD1.ucm", "Demo map", 40, 00, "Beat me" },
                { "\\Album1.ucm", "Breakout!", 15, 38, "Tom Forsyth" },
                { NULL }
            };

            CBYTE par[128];

            sprintf(par, "No time yet. Email TomF your time!");

            SLONG i = 0;

            while (1) {
                Mime* mm = &mucky_times[i];

                if (!mm->level_fname) {
                    break;
                }

                extern CBYTE ELEV_fname_level[];

                if (strstr(ELEV_fname_level, mm->level_fname) && mm->who) {
                    SLONG mucky_time;
                    SLONG their_time;

                    mucky_time = mm->mins * 60 + mm->secs;
                    their_time = (h * 60 + m) * 60 + s;

                    extern BOOL dkeys_have_been_used;
                    extern bool g_bPunishMePleaseICheatedOnThisLevel;

                    if (their_time >= mucky_time || dkeys_have_been_used || g_bPunishMePleaseICheatedOnThisLevel) {
                        code[0] = '\000';
                    } else {
                        SLONG hash;

                        hash = (i + 1) * (m + 1) * (s + 1) * 3141;
                        hash = hash % 12345;
                        hash += 0x9a2f;

                        sprintf(code, ": CODE <%X> ", hash);
                    }

                    sprintf(par, ": (00:%02d:%02d)  %s", mm->mins, mm->secs, mm->who);

                    break;
                }

                i++;
            }

            sprintf(str, ": (%02d:%02d:%02d) %s", h, m, s, code);
            FONT2D_DrawString(str, 300 + 2, 400 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(str, 300, 400, 0xffffff, 256, POLY_PAGE_FONT2D);

            FONT2D_DrawString(par, 300 + 2, 420 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(par, 300, 420, 0xffffff, 256, POLY_PAGE_FONT2D);
        }
    }
}

// Initialises the loading screen: loads e3load.tga and flips twice (belt-and-braces).
// uc_orig: ATTRACT_loadscreen_init (fallen/Source/Attract.cpp)
void ATTRACT_loadscreen_init(void)
{
    PANEL_disable_screensaver(UC_TRUE);

    InitBackImage("e3load.tga");
    ShowBackImage(UC_FALSE);
    AENG_flip();
    InitBackImage("e3load.tga");
    ShowBackImage(UC_FALSE);
    AENG_flip();
}

// Draws the loading bar at the given completion value (0-256 in 8-bit fixed point).
// uc_orig: ATTRACT_loadscreen_draw (fallen/Source/Attract.cpp)
void ATTRACT_loadscreen_draw(SLONG completion)
{
    ShowBackImage(UC_FALSE);
    PANEL_draw_completion_bar(completion);
    AENG_flip();
}

// Stub — replaced by GAMEMENU overlay system.
// uc_orig: level_won (fallen/Headers/attract.h)
void level_won(void) {}

// Stub — replaced by GAMEMENU overlay system.
// uc_orig: level_lost (fallen/Headers/attract.h)
void level_lost(void) {}

