// Attract.cpp
// Guy Simmons, 20th November 1997.
// claude-ai: ATTRACT MODE = главный цикл frontend'а. Вызывается из game_loop() при GS_ATTRACT_MODE.
// claude-ai: game_attract_mode() — основная функция:
// claude-ai:   1. Инициализация: MFX_load_wave_list(), FRONTEND_init(), D3D render states
// claude-ai:   2. Главный цикл (60fps): FRONTEND_loop() каждый кадр (если NEW_FRONTEND определён)
// claude-ai:   3. Результат STARTS_START → GS_ATTRACT_MODE выкл, GS_PLAY_GAME вкл, ATTRACT_loadscreen_init()
// claude-ai: ВАЖНО: "attract mode" = неточное название. Старый attract demo (playback .pkt) закомментирован.
// claude-ai: Реально это просто главное меню (frontend loop). Название осталось от ранней версии.
// claude-ai: auto_advance: если set → немедленно переходит в игру без frontend (для автозапуска).
// claude-ai:   STARTSCR_notify_gameover() закомментирован в Game.cpp → auto_advance никогда не устанавливается.
// claude-ai: level_won(), level_lost() → #if 0 (dead code). Заменены GAMEMENU overlay системой.
// claude-ai: ScoresDraw() → отрисовка статистики + mucky times (speedrun) таблица.
// claude-ai:   Показывает: killed(stat_killed_thug), arrested(stat_arrested_thug), at-large(живые Thug/MIB),
// claude-ai:   bonus found(stat_count_bonus), bonus missed(живые SPECIAL_TREASURE), time(h:m:s from stat_game_time).
// claude-ai:   Mucky times: хардкод ~35 миссий с рекордами разработчиков; если игрок быстрее → hash-код для верификации.
// claude-ai:   Активная таблица = DC версия (#else ветка). PC-времена в #if 0.
// claude-ai:   Вызывается ТОЛЬКО из GAMEMENU_draw() при GAMEMENU_MENU_TYPE_WON; вызов из overlay.cpp закомментирован.

#include "Game.h"
#include "cam.h"
#include "..\Headers\ddlib.h"
#include "font2d.h"
#include "poly.h"
#include "panel.h"
#include "startscr.h"
#include "briefing.h"
#define DEMO
#include "attract.h"
#include "env.h"
#include "overlay.h"
#include "sound.h"
#include "mfx.h"
#include "eway.h"
#include "interfac.h"
#include "xlat_str.h"
#include "frontend.h"
#include "statedef.h"
#include "DCLowLevel.h"
// #include "console.h"

// undef this to get the old menus back...
#define NEW_FRONTEND 1

//---------------------------------------------------------------
CBYTE demo_text[] = "Urban Chaos utilises a ground breaking graphics engine which includes 3D volumetric\n"
                    "fog, true wall hugging shadows, atomic matter simulation for real-time physical\n"
                    "modelling of dynamic object collisions and so provides the perfect environment \n"
                    "for incredible feats of acrobatic skill and total scenery interaction. \n\n"
                    "The plot of Urban Chaos is still a closely guarded secret but revolves around end\n"
                    "of the millennium chaos as predictions for Armageddon become more than mere \n"
                    "fantasy.\n\n"
                    "Urban Chaos is being developed by Mucky Foot one of the UK's hottest new \n"
                    "development teams and will be published world-wide by Eidos Interactive.";
//---------------------------------------------------------------

extern SLONG stat_killed_thug;
extern SLONG stat_killed_innocent;
extern SLONG stat_arrested_thug;
extern SLONG stat_arrested_innocent;

#define MAX_PLAYBACKS 3

CBYTE* playbacks[] = {
    "Data\\Game.pkt",
    "Data\\Game2.pkt",
    "Data\\Game3.pkt"
};
SLONG current_playback = 0;

// extern ULONG get_hardware_input(UWORD type);

extern CBYTE* playback_name;
SLONG go_into_game;
UBYTE auto_advance = 0;

//---------------------------------------------------------------

// Attract mode is the default game state, basically this function
// cycles between playing the intro, showing the high score table &
// showing the 'PRESS START' bits.

void LoadBackImage(UBYTE* image_data);
void AENG_demo_attract(SLONG x, SLONG y, CBYTE* text);
extern BOOL text_fudge;
extern SLONG do_start_menu(void);

extern DIJOYSTATE the_state;

SLONG any_button_pressed(void)
{

    if (ReadInputDevice()) {
        if (the_state.rgbButtons[0] || the_state.rgbButtons[1] || the_state.rgbButtons[2] || the_state.rgbButtons[3] || the_state.rgbButtons[4] || the_state.rgbButtons[5] || the_state.rgbButtons[6] || the_state.rgbButtons[7] || the_state.rgbButtons[8] || the_state.rgbButtons[9]) {
            //			GAME_STATE &= ~GS_ATTRACT_MODE;
            //			GAME_STATE |=  GS_PLAY_GAME;
            return (1);
        }
    }
    return (0);
}

void game_attract_mode(void)
{
    float y;
    UBYTE *image_mem = NULL,
          *image = NULL;
    SLONG height,
        image_size;
    MFFileHandle image_file;
    SLONG dont_leave_for_a_while = 25;

    //
    // make sure lighting is ok
    //
    extern void NIGHT_init();
    NIGHT_init();

    if (auto_advance) {
        go_into_game = TRUE;
        auto_advance = 0;
        GAME_STATE &= ~GS_ATTRACT_MODE;
        GAME_STATE |= GS_PLAY_GAME;
    } else
        go_into_game = FALSE;
    /*
            image_size	=	640*480*3;
            image_mem	=	(UBYTE*)MemAlloc(image_size);
            if(image_mem)
            {
                    image_file	=	FileOpen("Data\\gamelogo.tga");
                    if(image_file>0)
                    {
                            FileSeek(image_file,SEEK_MODE_BEGINNING,18);
                            image	=	image_mem+(640*479*3);
                            for(height=480;height;height--,image-=(640*3))
                            {
                                    FileRead(image_file,image,640*3);
                            }
                            FileClose(image_file);
                    }
            }
    */
    //	InitBackImage("e3title.tga");

    // load our sound fx; for the menu sound's benefit
    // (later, do a cut down version w/ only the menu sounds for speed)
    /*	#ifndef USE_A3D
                    LoadWaveList("Data\\SFX\\1622\\","Data\\SFX\\Samples.txt");
            #else
                    A3DLoadWaveList("Data\\SFX\\1622\\","Data\\SFX\\Samples.txt");
            #endif
    */
    MFX_load_wave_list();

    bool bReinitBecauseOfLanguageChange = FALSE;
reinit_because_of_language_change:

    FRONTEND_init(bReinitBecauseOfLanguageChange);

    bReinitBecauseOfLanguageChange = FALSE;

    //
    // Create a surface from this image.
    //

    //	the_display.create_background_surface(image_mem);

    LPDIRECT3DDEVICE3 dev = the_display.lp_D3D_Device;
    HRESULT hres;

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
    hres = the_display.lp_D3D_Viewport->SetViewport2(&vp);

    dev->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
    dev->SetRenderState(D3DRENDERSTATE_STIPPLEDALPHA, FALSE);
    dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    dev->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
    dev->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
    dev->SetRenderState(D3DRENDERSTATE_SUBPIXEL, TRUE);
    dev->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
    dev->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    dev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    dev->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
    dev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
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
    // dev->SetTexture ( 0, NULL );
    extern LPDIRECT3DTEXTURE2 TEXTURE_get_handle(SLONG page);
    extern SLONG TEXTURE_page_water;
    dev->SetTexture(0, TEXTURE_get_handle(TEXTURE_page_water));

    dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
    dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);

    // claude-ai: Главный цикл frontend. 60fps (lock_frame_rate(60) в конце).
    // claude-ai: Вызывает FRONTEND_loop() каждый кадр → возвращает STARTS_* код.
    // claude-ai: При STARTS_START: GS_ATTRACT_MODE выкл, GS_PLAY_GAME вкл, ATTRACT_loadscreen_init().
    // claude-ai: STARTS_EDITOR → GS_EDITOR; STARTS_EXIT → GAME_STATE=0 (выход из программы).
    y = 500;
    while (SHELL_ACTIVE && (GAME_STATE & GS_ATTRACT_MODE)) {
        if (dont_leave_for_a_while > 0) {
            dont_leave_for_a_while -= 1;
        }

        if (ControlFlag && Keys[KB_Q]) {
            GAME_STATE = 0;
            LastKey = 0;
        }
        /*
                        else if(Keys[KB_SPACE] || Keys[KB_ENTER] || go_into_game)
                        {
                                GAME_STATE	&=	~GS_ATTRACT_MODE;
                                GAME_STATE	|=	GS_PLAY_GAME;
                                break;
                        }
        */

        if (dont_leave_for_a_while == 0) {
            /*			if(any_button_pressed())
                                    {
                                            GAME_STATE &= ~GS_ATTRACT_MODE;
                                            GAME_STATE |=  GS_PLAY_GAME;
                                            go_into_game=TRUE;
                                    }*/
        }

        /*		if (Keys[KB_N])
                        {
                                Keys[KB_N] = 0;

                                //
                                // Go the network configuration screen.
                                //

                                GAME_STATE = GS_CONFIGURE_NET;
                        }*/

        //	Record a game.
        extern BOOL allow_debug_keys;
        if (allow_debug_keys) {
            if (ControlFlag && LastKey == KB_R) {
                LastKey = 0;
                GAME_STATE &= ~GS_ATTRACT_MODE;
                GAME_STATE |= GS_PLAY_GAME;
                go_into_game = TRUE;

                GAME_STATE |= GS_RECORD;
            }

            //	Playback a game.
            if (ControlFlag && LastKey == KB_P) {
                LastKey = 0;
                GAME_STATE &= ~GS_ATTRACT_MODE;
                GAME_STATE |= GS_PLAY_GAME;
                go_into_game = TRUE;

                GAME_STATE |= GS_PLAYBACK;
            }
        }

        if (y < 57.0F) {
            AENG_clear_screen();
        }

        // LoadBackImage(image_mem);
        //		the_display.blit_background_surface();

        text_fudge = TRUE;
        //		y	-=	0.25f;
        //		y--;
        if (Keys[KB_LEFT] || Keys[KB_RIGHT] || Keys[KB_UP] || Keys[KB_DOWN] || Keys[KB_SPACE] || Keys[KB_ENTER])
            y = 500;

        //		AENG_demo_attract(0,y,demo_text);
        {
            SLONG res;

            res = FRONTEND_loop();

            //			res=STARTS_PLAYBACK;

            if (res) {
                switch (res) {
                    /*
                                                    case	1:
                                                    case	2:
                                                    case	3:
                                                    case	4:
                                                    case	5:
                                                    case	6:
                                                    case	7:
                                                    case	8:
                                                                    GAME_STATE	&= ~GS_ATTRACT_MODE;
                                                                    GAME_STATE	|=	GS_PLAY_GAME;
                                                                    go_into_game =  -res;
                                                                    break;
                                                    case	9:
                                                                    GAME_STATE	=	0;
                                                                    LastKey	=	0;
                                                                    break;
                    */

                case STARTS_PSX:
                    break;
                case STARTS_START:
                    // claude-ai: STARTS_START — миссия выбрана в FRONTEND_loop() (mode≥100 + ENTER → FE_START).
                    // claude-ai: STARTSCR_mission уже установлен в FRONTEND_loop() перед возвратом STARTS_START.
                    // claude-ai: #ifdef OBEY_SCRIPT / BRIEFING_select() → МЁРТВЫЙ КОД: OBEY_SCRIPT не определён.
                    // claude-ai: Активный путь: сразу переключает GAME_STATE и показывает загрузочный экран.
                    {
                        GAME_STATE &= ~GS_ATTRACT_MODE;
                        GAME_STATE |= GS_PLAY_GAME;
                        go_into_game = TRUE;

                        ATTRACT_loadscreen_init();

                        // Stop everything.
                        stop_all_fx_and_music();

                        // but play the loading music, coz it's all in memory.
                        // (No, should already be played, and stopping then starting it
                        // makes it huccup.)
                        // DCLL_memstream_play();

                        extern void init_joypad_config(void);
                        init_joypad_config(); // incase it's been changed in front end
                    }
                    break;
                case STARTS_PLAYBACK:
                    GAME_STATE &= ~GS_ATTRACT_MODE;
                    GAME_STATE |= GS_PLAY_GAME;
                    GAME_STATE |= GS_PLAYBACK;
                    go_into_game = TRUE;
                    break;
                case STARTS_EDITOR:
                    GAME_STATE = GS_EDITOR;
                    break;
                case STARTS_MULTI:

                    NET_init();
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
                    // A language change has been made -
                    // reload all the language stuff and go to the main menu.

                    // Loading screen for a second.
                    ATTRACT_loadscreen_init();

                    // This goto makes the compiler very unhappy - it seems to get very confused.
                    // goto reinit_because_of_language_change;
                    bReinitBecauseOfLanguageChange = TRUE;
                    break;
                }
            }
        }

        if (y < 57.0F) {
            AENG_fade_out((57 - SLONG(y)) * 15);
        }

        /*
        if(y<40.0f)
        {
                text_fudge	=	FALSE;
                GAME_STATE	|=	(GS_PLAYBACK|GS_PLAY_GAME);
                playback_name	=	playbacks[current_playback];

                go_into_game = FALSE;	// Go into game could be set by game_loop()!!!
                game_loop();
                GAME_STATE	=	GS_ATTRACT_MODE;
                y	=	490.0f;
                current_playback++;
                if(current_playback>=MAX_PLAYBACKS)
                        current_playback	=	0;
                dont_leave_for_a_while = 25;
        }
        */

        extern void lock_frame_rate(SLONG fps);

        lock_frame_rate(60); // because 30 is so slow i want to gnaw my own liver out rather than go through the menus

        if ((GAME_STATE & GS_PLAY_GAME) == 0) {
            AENG_flip();
        }

        if (bReinitBecauseOfLanguageChange) {
            // Crappy compiler workaround.
            break;
        }
    }

    if (bReinitBecauseOfLanguageChange) {
        // Crappy compiler workaround.
        goto reinit_because_of_language_change;
    }

    //	Fade to logo.
    if (GAME_STATE & GS_PLAY_GAME) {
        if (go_into_game) {

            /*			AENG_clear_screen();
                                    AENG_fade_out(255);*/
            ShowBackImage(FALSE);
            AENG_flip();

        } else {
            SLONG c0;

            //			ASSERT(0);
            /*
                                    c0 = (57 - SLONG(y)) * 17;

                                    SATURATE(c0, 0, 255);

                                    c0 -= c0 % 15;

                                    for(;c0<256;c0+=15)
                                    {
                                            AENG_clear_screen();
                                            LoadBackImage(image_mem); //image_mem not valid
                                            AENG_demo_attract(0,(SLONG)y,demo_text);
                                            AENG_fade_out((UBYTE)c0);
                                            AENG_flip();
                                    }
            */
        }
    }

    text_fudge = FALSE;
    /*
            the_display.destroy_background_surface();

            if(image_mem)
            {
                    MemFree(image_mem);
            }
    */
    ResetBackImage();
}

//---------------------------------------------------------------

inline void printf2d(SLONG x, SLONG& y, CBYTE* fmt, ...)
{
    CBYTE msg[_MAX_PATH];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(msg, fmt, ap);
    va_end(ap);

    FONT2D_DrawString(msg, x, y, 0x00ff00, 256, POLY_PAGE_FONT2D);
    y += 20;
}

#define SCORE_SPACER 20

extern SLONG stat_killed_thug;
extern SLONG stat_killed_innocent;
extern SLONG stat_arrested_thug;
extern SLONG stat_arrested_innocent;
extern SLONG stat_count_bonus;
extern SLONG stat_start_time, stat_game_time;

void ScoresDraw(void)
{

    SLONG y = 35;
    SLONG count = 0, count_bonus = 0, count_bonus_left = 0, c0;
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

    //	POLY_frame_init(FALSE, FALSE);

    extern SLONG playing_real_mission(void);

    if (playing_real_mission()) {
        if (NET_PERSON(0)->Genus.Person->PersonType != PERSON_DARCI && NET_PERSON(0)->Genus.Person->PersonType != PERSON_ROPER) {
            //
            // This only works for Darci or Roper!
            //
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

        //	FONT2D_DrawString(XLAT_str(X_WON_BESTTIME)	,10,420,0xffffff,256,POLY_PAGE_FONT2D);

        if (NET_PERSON(0)->Genus.Person->PersonType != PERSON_DARCI && NET_PERSON(0)->Genus.Person->PersonType != PERSON_ROPER) {
            //
            // This only works for Darci or Roper!
            //
        } else {
            sprintf(str, ": %d", stat_killed_thug);
            FONT2D_DrawString(str, 300 + 2, 300 + 2, 0x000000, 256, POLY_PAGE_FONT2D);
            FONT2D_DrawString(str, 300, 300, 0xffffff, 256, POLY_PAGE_FONT2D);

            //	sprintf(str,": %d",stat_killed_innocent);
            //	FONT2D_DrawString(str	,300,300,0xffffff,256,POLY_PAGE_FONT2D);

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
            typedef struct
            {
                CBYTE* level_fname;
                CBYTE* level_name;
                SLONG mins;
                SLONG secs;
                CBYTE* who;

            } Mime; // A mucky time!

            // DC times, starting from scratch.
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
                        // No code for you!
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

        //	sprintf(str,": %d",stat_arrested_innocent);
        //	FONT2D_DrawString(str	,300,300,0xffffff,256,POLY_PAGE_FONT2D);
    }

    //	POLY_frame_draw(FALSE, FALSE);
}

//---------------------------------------------------------------

void ATTRACT_loadscreen_init(void)
{

    PANEL_disable_screensaver(TRUE);

    InitBackImage("e3load.tga");
    ShowBackImage(FALSE);
    AENG_flip();
    // Er... why do this again????
    InitBackImage("e3load.tga");
    ShowBackImage(FALSE);
    AENG_flip();
}

void ATTRACT_loadscreen_draw(SLONG completion) // completion is in 8-bit fixed point from 0 to 256.
{
    ShowBackImage(FALSE);
    PANEL_draw_completion_bar(completion);
    AENG_flip();
}
