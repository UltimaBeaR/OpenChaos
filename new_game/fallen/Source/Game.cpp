// Game.cpp
// Guy Simmons, 17th October 1997

#include "demo.h"
#include "Game.h"
#include "pap.h"
#include "Attract.h"
#include "id.h"
#include "io.h"
#include "light.h"
#include "heap.h"
#include "hm.h"
#include "dirt.h"
#include "fog.h"
#include "mist.h"
#include "water.h"
#include "cnet.h"
#include "interfac.h"
#include "puddle.h"
#include "road.h"
#include "az.h"
#include "cam.h"
#include "door.h"
#include "sewer.h"
#include "drip.h"
#include "Sound.h"
#include "gamemenu.h"
#include "..\ddengine\headers\menufont.h"
#include "..\ddlibrary\headers\net.h"
#include "bang.h"
#include "mav.h"
#include "..\editor\headers\extra.h"
#include "spark.h"
#include "statedef.h"
#include "glitter.h"
#include "ob.h"
#include "morph.h"
#include "qedit.h"
#include "trip.h"
#include "pap.h"
#include "night.h"
#include "shadow.h"
#include "cloth.h"
#include "ns.h"
#include "supermap.h"
#include "..\sedit\headers\es.h"
#include "build2.h"
#include "eway.h"
#include "elev.h"
#include "pause.h"
#include "snipe.h"
#include "pow.h"
#include "Game.h"
#include "widget.h"
#include "memory.h"
#include "fc.h"
#include "startscr.h"
#include "save.h"

#include "spark.h"
#include "font2d.h"
#include "env.h"
#include "wmove.h"
#include "balloon.h"
#include "memory.h"
#include "..\DDEngine\Headers\console.h"
#include "..\DDEngine\Headers\poly.h"
#include "..\DDEngine\Headers\map.h"
#include "psystem.h"
#include "ribbon.h"
#include "overlay.H"
#include "music.h"
#include "grenade.h"
#include "drawxtra.h"
#include "..\ddlibrary\headers\ddlib.h"
#include "..\ddengine\headers\planmap.h"
#include <math.h>
#include "..\ddengine\headers\BreakTimer.h"
#include "frontend.h"
#include "..\ddengine\headers\truetype.h"
#include "panel.h"


#include "xlat_str.h"
#include "DCLowLevel.h"

#define TIMEOUT_DEMO 0
#define VERIFY_PLAYBACK 0 // !$$! set to 1 to verify demo playback

#include "mfx.h"
#include "..\ddengine\headers\superfacet.h"
#include "..\ddengine\headers\farfacet.h"
#include "..\ddengine\headers\fastprim.h"
#include "..\ddengine\headers\supercrinkle.h"

SLONG CAM_cur_x, CAM_cur_y, CAM_cur_z,
    CAM_cur_yaw, CAM_cur_pitch, CAM_cur_roll; // these are set appropriate to whichever cam


//
// The editor.
//

SLONG save_psx = 0; // this was nicked from edit.cpp in the editor...

// bool g_bGoToCreditsPleaseGameHasFinished = FALSE;

extern UBYTE editor_loop(void);
extern BOOL allow_debug_keys;
//
// Nearly everything in the whole game.
//

//
// FOR PC for GERMAN/French we only need edit this file
//

// #define	VERSION_GERMAN	1
// #define	VERSION_FRENCH	1

// claude-ai: VIOLENCE_ALLOWED — всегда 1 в стандартной сборке. 0 только для немецкой/французской PC версии.
// claude-ai: В новой игре: всегда 1, немецкую/французскую цензуру не переносить.
#define VIOLENCE_ALLOWED 1

#ifdef VERSION_GERMAN
// #error
#undef VIOLENCE_ALLOWED
#define VIOLENCE_ALLOWED 0
#endif

#ifdef VERSION_FRENCH
// #error
#undef VIOLENCE_ALLOWED
#define VIOLENCE_ALLOWED 0
#endif

Game the_game;
UBYTE VIOLENCE = VIOLENCE_ALLOWED;

extern SLONG go_into_game; // This is in attract.cpp If it is TRUE when we leave the loop, then attract goes into the game again.

// claude-ai: stop_all_fx_and_music() — останавливает всю музыку и звуковые эффекты. Вызывается при смене сцены (загрузка уровня, переход в фронтенд, смерть игрока).
void stop_all_fx_and_music()
{
    MFX_QUICK_stop();
    MUSIC_mode(0);
    MUSIC_mode_process();
    MUSIC_reset();
    MFX_stop(MFX_CHANNEL_ALL, MFX_WAVE_ALL);
}

//
// Loads data that only needs to be loaded once, or after we exit
// the editor.
//


// claude-ai: global_load() — однократная загрузка глобальных данных (аним., примитивы). Только PC/не-DC.
// claude-ai: Вызывается из game_init() и game_create_psx(). На DC не нужна из-за ограничений памяти.
void global_load(void)
{
    init_memory();

    clear_prims();
    init_draw_tweens();
    setup_people_anims();
    setup_extra_anims();
    setup_global_anim_array();
    record_prim_status();
}

//---------------------------------------------------------------
// #define	DebugText
// claude-ai: Инициализация всех подсистем: открытие дисплея, AENG_init, звук MFX_init, анимации, джойстик
// claude-ai: Устанавливает GAME_STATE = GS_ATTRACT_MODE (PC) или GS_PLAY_GAME (PSX)
void game_startup(void)
{
    GAME_STATE = 0;

    init_memory();
    FC_init();


    //
    // Do PC setup.
    //

    //	NET_init();

    /*	{
                    //
                    // Enumerate connections!
                    //

                    SLONG i;
                    SLONG num_connections = NET_get_connection_number();

                    for (i = 0; i < num_connections; i++)
                    {
                            TRACE("Connection %d: %s\n", i, NET_get_connection_name(i));
                    }
            }
    */

    if (OpenDisplay(640, 480, 16, FLAGS_USE_3D | FLAGS_USE_WORKSCREEN) == 0) {
        GAME_STATE = GS_ATTRACT_MODE;
    } else {
        MessageBox(NULL, "Unable to open display", NULL, MB_OK | MB_ICONWARNING);
        exit(1);
    }

    AENG_init();

    // Get a loading screen up as soon as possible.
    ATTRACT_loadscreen_init();

    // NOW we can load the sounds, coz we have something sensible on-screen
    // #ifdef TARGET_DC
    extern void MFX_init(void);
    MFX_init();
    // #endif

    ATTRACT_loadscreen_draw(160);

    void init_joypad_config(void);
    init_joypad_config();
    ANIM_init();

    GetInputDevice(JOYSTICK, 0, TRUE);

    MORPH_load();

    if (ENV_get_value_number("clumps", 0, "Secret")) {
        extern void make_all_clumps(void);
        make_all_clumps();
    }

    TEXTURE_load_needed("levels\\frontend.ucm", 160, 256, 65);



    //
    // Load the console font
    //

    CONSOLE_font("data\\font3d\\all\\", 0.2F);

    //
    // Load stuff in.
    //

    //	global_load();
}

//---------------------------------------------------------------

// claude-ai: Завершение работы: CloseDisplay(), NET_kill(), освобождение памяти
void game_shutdown(void)
{

    //
    // PC shutdown.
    //

    CloseDisplay();

    NET_kill();
    AENG_fini();
    ANIM_fini();

}

//---------------------------------------------------------------


//
// Playback file stuff....
//

// extern CBYTE       *playback_name = "Data\\Game.pkt";
extern CBYTE* playback_name = "C:\\Windows\\Desktop\\UrbanChaosRecordedGame.pkt";
extern MFFileHandle playback_file;

extern CBYTE* verifier_name = "C:\\Windows\\Desktop\\UrbanChaosRecordedGame.tst";
extern MFFileHandle verifier_file;


//
// The player position is loaded into here by load_level()
//

extern GameCoord player_pos;

//---------------------------------------------------------------

//
// These #defines are nicked from interface.cpp
//

#define AXIS_CENTRE 32768
#define NOISE_TOLERANCE 1024
#define AXIS_MIN (AXIS_CENTRE - NOISE_TOLERANCE)
#define AXIS_MAX (AXIS_CENTRE + NOISE_TOLERANCE)

extern DIJOYSTATE the_state;

//
// Does the game paused stuff...
//

#define PAUSED_KEY_START (1 << 0)
#define PAUSED_KEY_UP (1 << 1)
#define PAUSED_KEY_DOWN (1 << 2)
#define PAUSED_KEY_SELECT (1 << 3)

#define PAUSE_MENU_SIZE 2

// claude-ai: pause_menu[] — пауза-меню: 2 пункта "CONTINUE GAME" / "EXIT". Отображается на PSX; PC использует gamemenu.cpp (GAMEMENU_*).
CBYTE* pause_menu[PAUSE_MENU_SIZE] = {
    "CONTINUE GAME",
    "EXIT",
};

UBYTE game_paused_key;
SBYTE game_paused_highlight;

extern BOOL text_fudge;
extern ULONG text_colour;
extern void draw_centre_text_at(float x, float y, CBYTE* message, SLONG font_id, SLONG flag);
extern void draw_text_at(float x, float y, CBYTE* message, SLONG font_id);

//
// Prints up stuff to the screen...
//
#define NUM_BULLETS 15

CBYTE* bullet_point[NUM_BULLETS] = {
    "A - Punch : B - Kick : C - Jump\n",
    "Try running over a coke can...\n",
    "Press JUMP and push back to do a backwards summersault!\n",
    "Stand in a puddle and see your reflection!\n",
    "Z is your action button. Press it to climb ladders\nand to take out and put away your gun\n",
    "You can swirl the mist by running through it.\n",
    "Your shadow goes up walls!\n",
    "Press JUMP and push left or right to do a cartwheel\n",
    "Jump on fences to clamber over them.\nWatch out! Some of them are electrified.\n",
    "You can climb onto the rooftops.\nSee how high you can go!\n",
    "Hold down Y to look around in first person.\n",
    "Sneak up behind enemies and press punch.\nIf you're in the right place you'll break their necks!\n",
    "The shoulder buttons rotate the camera.\n",
    "Press action to climb up ladders.\n",
    "X is your mode change button.\nChange through RUN, WALK and SNEAK modes\n"
};

SLONG bullet_upto;
SLONG bullet_counter;

// claude-ai: process_bullet_points() — прокрутка обучающих подсказок во время загрузки (PC/не-DC).
// claude-ai: Меняет текст каждые 250 кадров, с мягким fade-in по alpha (bright = counter * 32, зажато до 255).
// claude-ai: В новой игре можно переиспользовать для экрана загрузки.
void process_bullet_points(void)
{
    bullet_counter -= 1;

    if (bullet_counter < 0) {
        bullet_upto += 1;
        bullet_upto = bullet_upto % NUM_BULLETS;
        bullet_counter = 250;
    }

    SLONG bright = bullet_counter * 32;

    if (bright > 255) {
        bright = 255;
    }

    text_fudge = FALSE;
    text_colour = bright * 0x00010101;

    draw_centre_text_at(10, 420, bullet_point[bullet_upto], 0, 0);
}

//---------------------------------------------------------------

// claude-ai: game_init() — инициализация одной миссии: сброс GAME_TURN/GAME_FLAGS, загрузка уровня через ELEV_load_user().
// claude-ai: Также инициализирует TICK_RATIO, random seed, паузу, пуль-поинты и вызывает init_stats().
// claude-ai: Возвращает ret: 1 или 5 = уровень загружен успешно, 0 = ошибка.
BOOL game_init(void)
{
    SLONG ret;

    // stop_all_fx_and_music();


    //
    // Set the seed and initialise game variables.
    //
    global_load();

    GAME_TURN = 0;
    GAME_FLAGS = 0;
    if (!CNET_network_game) {
        NO_PLAYERS = 1;
        PLAYER_ID = 0;
    }
    // claude-ai: TICK_RATIO — коэффициент масштабирования времени. Инициализируется как 1<<TICK_SHIFT (нормальная скорость).
    // claude-ai: В process_things_tick() пересчитывается каждый кадр: TICK_RATIO = (реальный_мс << TICK_SHIFT) / NORMAL_TICK_TOCK
    // claude-ai: NORMAL_TICK_TOCK = 1000/15 = 66.67мс (базовый тик 15 FPS). Усредняется по 4 кадрам через SmoothTicks().
    TICK_RATIO = (1 << TICK_SHIFT);
    DETAIL_LEVEL = 0xffff;

    ResetSmoothTicks();

    INDOORS_INDEX = 0;
    INDOORS_INDEX_NEXT = 0;
    INDOORS_INDEX_FADE_EXT_DIR = 0;
    INDOORS_INDEX_FADE_EXT = 0;
    INDOORS_DBUILDING = 0;

    SetSeed(0);
    srand(1234567);

    // Load up game settings from the env values.

    // Panel position. Numbers divided by 4 to fit in a byte.
    extern int m_iPanelXPos;
    extern int m_iPanelYPos;
    // m_iPanelXPos = 4 * ENV_get_value_number ( "panel_x", 32 / 4, "" );
    // m_iPanelYPos = 4 * ENV_get_value_number ( "panel_y", (480-32) / 4, "" );


    //
    // Open the game record/playback file.
    //


    if (GAME_STATE & GS_RECORD) {
        DebugText(" PLAYBACK GAME\n");
        playback_file = FileCreate(playback_name, TRUE);
#if VERIFY_PLAYBACK
        verifier_file = FileCreate(verifier_name, TRUE);
#else
        verifier_file = NULL;
#endif
    } else if (GAME_STATE & GS_PLAYBACK) {

        DebugText(" RECORD GAME\n");
        playback_file = FileOpen(playback_name);
#if VERIFY_PLAYBACK
        verifier_file = FileOpen(verifier_name);
#else
        verifier_file = NULL;
#endif
    }

    if (playback_file == FILE_CREATION_ERROR) {
        playback_file = NULL;
    }
    if (verifier_file == FILE_CREATION_ERROR) {
        verifier_file = NULL;
    }


    //
    // Initialise pause mode...
    //

    game_paused_key = -1;
    GAME_FLAGS &= ~GF_PAUSED;

    //
    // Initiliase the bullet points...
    //
    bullet_upto = -1;
    bullet_counter = 0;
    //
    // Load our sound effects.
    //
    /*
    #ifndef USE_A3D
            LoadWaveList("Data\\SFX\\1622\\","Data\\SFX\\Samples.txt");
    #else
            A3DLoadWaveList("Data\\SFX\\1622\\","Data\\SFX\\Samples.txt");
    #endif
    */

    extern THING_INDEX PANEL_wide_top_person;
    extern THING_INDEX PANEL_wide_bot_person;

    PANEL_wide_top_person = NULL;
    PANEL_wide_bot_person = NULL;

    if (GAME_STATE & GS_REPLAY) {
        ATTRACT_loadscreen_init();

        extern CBYTE ELEV_fname_level[];
        extern SLONG quick_load;

        quick_load = TRUE;

        ANIM_init();

        ELEV_load_name(ELEV_fname_level);

        quick_load = FALSE;

        ret = 1;


    } else {
        //
        // Load the game.
        //
        ret = ELEV_load_user(go_into_game);


        extern SLONG save_psx;
        if (save_psx)
            if (ret == 5 || ret == 1) // loaded a level
            {
                CBYTE save_wad[100];

                extern CBYTE ELEV_fname_level[];
                process_things(0);

                extern void save_whole_game(CBYTE * gamename);

                change_extension(ELEV_fname_level, "wad", save_wad);
                save_whole_game(save_wad);
            }
    }

    void init_stats(void);
    init_stats();

    EWAY_tutorial_string = NULL;


    return (ret);

    //	return ELEV_load_name("levels\\e7.ucm");
}

// claude-ai: game_create_psx() — инструментальная функция (только PC, только !TARGET_DC).
// claude-ai: Загружает .ucm миссию и немедленно сохраняет её как .wad файл для PSX.
// claude-ai: В новой игре не нужна — PSX/WAD конвертация не переносится.
BOOL game_create_psx(CBYTE* mission_name)
{
    SLONG ret;
    DebugText("PSX create psx mission %s\n", mission_name);

    //
    // Set the seed and initialise game variables.
    //
    global_load();

    GAME_TURN = 0;
    GAME_FLAGS = 0;
    if (!CNET_network_game) {
        NO_PLAYERS = 1;
        PLAYER_ID = 0;
    }
    TICK_RATIO = (1 << TICK_SHIFT);
    DETAIL_LEVEL = 0xffff;

    ResetSmoothTicks();

    INDOORS_INDEX = 0;
    INDOORS_INDEX_NEXT = 0;
    INDOORS_INDEX_FADE_EXT_DIR = 0;
    INDOORS_INDEX_FADE_EXT = 0;
    INDOORS_DBUILDING = 0;

    SetSeed(0);
    srand(1234567);
    GAME_STATE = GS_PLAY_GAME;

    extern SLONG quick_load;
    quick_load = 1;

    //
    // Initialise pause mode...
    //

    game_paused_key = -1;

    GAME_FLAGS &= ~GF_PAUSED;

    //
    // Initiliase the bullet points...
    //
    bullet_upto = -1;
    bullet_counter = 0;

    {
        //
        // Load the game.
        //
        //		ret=ELEV_load_user(go_into_game);
        ret = ELEV_load_name(mission_name);

        if (ret == 5 || ret == 1) // loaded a level
        {
            CBYTE save_wad[100];

            extern CBYTE ELEV_fname_level[];
            process_things(0);

            extern void save_whole_game(CBYTE * gamename);

            change_extension(mission_name, "wad", save_wad);
            DebugText("PSX create nad %s\n world %d", save_wad, TEXTURE_SET);
            save_whole_game(save_wad);

        } else
            ASSERT(0);
    }

    return (ret);
}

// claude-ai: make_texture_clumps() — ещё одна инструментальная утилита (только PC/!DC): загружает уровень без сохранения.
// claude-ai: Похоже, использовалась для генерации текстурных «clumps» (pre-baked texture data). Не переносить.
BOOL make_texture_clumps(CBYTE* mission_name)
{
    SLONG ret;
    DebugText("Making texture clumps %s\n", mission_name);

    //
    // Set the seed and initialise game variables.
    //
    global_load();

    GAME_TURN = 0;
    GAME_FLAGS = 0;
    if (!CNET_network_game) {
        NO_PLAYERS = 1;
        PLAYER_ID = 0;
    }
    TICK_RATIO = (1 << TICK_SHIFT);
    DETAIL_LEVEL = 0xffff;

    ResetSmoothTicks();

    INDOORS_INDEX = 0;
    INDOORS_INDEX_NEXT = 0;
    INDOORS_INDEX_FADE_EXT_DIR = 0;
    INDOORS_INDEX_FADE_EXT = 0;
    INDOORS_DBUILDING = 0;

    SetSeed(0);
    srand(1234567);
    GAME_STATE = GS_PLAY_GAME;

    extern SLONG quick_load;
    quick_load = 0;

    //
    // Initialise pause mode...
    //

    game_paused_key = -1;

    GAME_FLAGS &= ~GF_PAUSED;

    //
    // Initiliase the bullet points...
    //
    bullet_upto = -1;
    bullet_counter = 0;

    {
        //
        // Load the game.
        //
        //		ret=ELEV_load_user(go_into_game);
        ret = ELEV_load_name(mission_name);
    }

    return (ret);
}

//---------------------------------------------------------------

// claude-ai: game_fini() — очистка после завершения миссии: стоп музыки, освобождение FASTPRIM/SUPERFACET/FARFACET,
// claude-ai: очистка LRU-кэша фигур, выгрузка звуков. На DC дополнительно чистит VB/IB и перезагружает frontend текстуры.
void game_fini(void)
{

    // Stop any background music, or the disk drive thrashes horribly trying to load stuff.
    // And all the looping stuff from the game as well.
    TRACE("gf1 ");
    stop_all_fx_and_music();

    // Start the loading bar.
    ATTRACT_loadscreen_init();

    //
    // Free up the FASTPRIM memory.
    //
    TRACE("gf2 ");

    FASTPRIM_fini();

    //
    // Free up the SUPERFACET memory.
    //

    TRACE("gf4 ");

    SUPERFACET_fini();

    //
    // Free up the FARFACET memory.
    //

    TRACE("gf5 ");

    FARFACET_fini();

    TRACE("gf6 ");

    // Free up the figure caches.
    void FIGURE_clean_all_LRU_slots(void);
    FIGURE_clean_all_LRU_slots();

    //
    // Unload our sound-effects.
    //

    TRACE("gf7 ");

    if (GAME_STATE != GS_REPLAY) {
        // Don't free if replaying mission - no need.
        MFX_free_wave_list();
    }

    //	Close the game record/playback file.
    if (playback_file) {
        FileClose(playback_file);
        playback_file = NULL;
    }

    if (verifier_file) {
        FileClose(verifier_file);
        verifier_file = NULL;
    }

    TRACE("gf8 ");


    NotGoingToLoadTexturesForAWhileNowSoYouCanCleanUpABit();

    TRACE("game_fini done\n");
}

//---------------------------------------------------------------

// extern ULONG	get_hardware_input(UWORD type);

// claude-ai: Главная функция игры. Вызывает game_startup(), затем главный while-цикл:
// claude-ai:   GS_ATTRACT_MODE → game_attract_mode() (меню/фронтенд)
// claude-ai:   GS_PLAY_GAME    → game_loop() (симуляция + рендеринг)
// claude-ai:   GS_EDITOR       → editor_loop() (не нужен в новой версии)
// claude-ai:   GS_CONFIGURE_NET → CNET_configure() (не нужен в новой версии)
// claude-ai: После выхода из цикла — game_shutdown()
void game(void)
{
    game_startup();

#ifdef VERSION_DEMO
    InitBackImage("demotitle.tga");

    for (;;) {
        MSG msg;

        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            ULONG input = get_hardware_input(INPUT_TYPE_JOY) | get_hardware_input(INPUT_TYPE_KEY);

            if (input & (INPUT_MASK_JUMP | INPUT_MASK_START | INPUT_MASK_SELECT | INPUT_MASK_KICK | INPUT_MASK_PUNCH | INPUT_MASK_ACTION)) {
                break;
            }
        }

        ShowBackImage(FALSE);
        the_display.Flip(NULL, DDFLIP_WAIT);
    }

    ResetBackImage();

#endif


    // claude-ai: Главный цикл: SHELL_ACTIVE = окно живо, GAME_STATE = текущее состояние (битовое поле)
    while (SHELL_ACTIVE && GAME_STATE) {
        game_attract_mode();

#ifdef EDITOR

        if (GAME_STATE & GS_EDITOR) {
            if (editor_loop())
                GAME_STATE = 0;
            else {
                //				global_load();  //editor screws up the global load

                GAME_STATE = GS_ATTRACT_MODE;
            }
        }
#endif
        if (GAME_STATE & GS_PLAY_GAME) {
            if (game_loop())
                GAME_STATE = 0;
            else
                GAME_STATE = GS_ATTRACT_MODE;
        }

        if (GAME_STATE & GS_CONFIGURE_NET) {
            if (CNET_configure())
                GAME_STATE = 0;
            else
                GAME_STATE = GS_ATTRACT_MODE;
        }
    }

#ifdef VERSION_DEMO
    InitBackImage("demoendscreen.tga");

    for (;;) {
        MSG msg;

        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            ULONG input = get_hardware_input(INPUT_TYPE_JOY) | get_hardware_input(INPUT_TYPE_KEY);
            if (input & (INPUT_MASK_JUMP | INPUT_MASK_START | INPUT_MASK_SELECT | INPUT_MASK_KICK | INPUT_MASK_PUNCH | INPUT_MASK_ACTION)) {
                break;
            }
        }

        ShowBackImage(FALSE);
        the_display.Flip(NULL, DDFLIP_WAIT);
    }

    ResetBackImage();
#endif

    TRACE("game before shutdown\n");

    game_shutdown();

    TRACE("game after shutdown\n");
}

//---------------------------------------------------------------

#define TAB_MAP_MIN_X 9
#define TAB_MAP_MIN_Z 13

#define TAB_MAP_SIZE 448

// claude-ai: GAME_map_draw_old() — старая функция рисования карты-табло (только PC/!DC). Рисует треугольник на позиции Дарси.
// claude-ai: Использует plan_view_shot() для top-down вида. Заменена GAME_map_draw(). Не переносить.
void GAME_map_draw_old(void)
{
    Thing* darci = NET_PERSON(0);

    SLONG x, z, dx, dz, ndx, ndz, angle;

    POLY_frame_init(FALSE, FALSE);
    ShowBackImage();
    the_display.lp_D3D_Viewport->Clear(1, &the_display.ViewportRect, D3DCLEAR_ZBUFFER);

    x = darci->WorldPos.X >> 8;
    z = darci->WorldPos.Z >> 8;

    x = (x * TAB_MAP_SIZE) >> 15;
    z = (z * TAB_MAP_SIZE) >> 15;

    angle = darci->Draw.Tweened->Angle;

    dx = -SIN(angle);
    dz = -COS(angle);
    ndx = -dz;
    ndz = dx;

    dx >>= 12;
    dz >>= 12;
    ndx >>= 14;
    ndz >>= 14;

    x += TAB_MAP_MIN_X;
    z += TAB_MAP_MIN_Z;
    AENG_draw_col_tri(x + ndx, z + ndz, 0xff0000, x + dx, z + dz, 0xff0000, x - ndx, z - ndz, 0xff0000, 0);

    POLY_frame_draw(FALSE, TRUE);
}

extern void overlay_beacons(void);



UBYTE screen_mem[640 * 3][480];

// claude-ai: GAME_map_draw() — актуальная функция рисования карты (PC/!DC). Рисует top-down вид через plan_view_shot() в screen_mem,
// claude-ai: накладывает беаконы overlay_beacons(), затем блиттирует в буфер дисплея.
// claude-ai: В новой игре карта может быть реализована через отдельный orthographic render pass.
void GAME_map_draw(void)
{
    Thing* darci = NET_PERSON(0);

    plan_view_shot(darci->WorldPos.X >> 8, darci->WorldPos.Z >> 8, 1 + (MouseY >> 4), 77, 78, 401, 328, (UBYTE*)screen_mem);
    overlay_beacons();
    the_display.create_background_surface((UBYTE*)screen_mem);
    the_display.blit_background_surface();
    the_display.destroy_background_surface();
}



// claude-ai: leave_map_form_proc() — callback диалогового окна "Leave map?" (только PC/!PSX).
// claude-ai: Возвращает TRUE (закрыть диалог) при нажатии кнопки; tag=2 = да/выйти, tag=1 = нет/продолжить.
BOOL leave_map_form_proc(Form* form, Widget* widget, SLONG message)
{
    if (widget && widget->methods == &BUTTON_Methods && message == WBN_PUSH) {
        form->returncode = widget->tag;

        return TRUE; // Exit
    } else {
        return FALSE; // Don't exit
    }
}


extern void PANEL_draw_timer_do(SLONG time, SLONG x, SLONG y);

SLONG already_warned_about_leaving_map;
UBYTE draw_map_screen = 0;
UBYTE single_step = 0;
Form* form_leave_map = NULL;
SLONG form_left_map = 0;

//****************************************************************
//****************************************************************
// You are now entering the cleanup zone
// These functions are made from code ripped from game_loop
// in the hope of making game_loop() readable
//****************************************************************
//****************************************************************

//
// don't let the game run faster than this framerate by making you sit in a check the clock loop
//
// claude-ai: Ограничение FPS: spin-loop busy-wait через GetTickCount() до истечения 1000/fps мс
// claude-ai: По умолчанию fps=30 (из config.ini "max_frame_rate"). Не вызывается на PSX/DC.
void lock_frame_rate(SLONG fps)
{
    static SLONG tick1 = 0;
    SLONG tick2;
    SLONG timet;

    while (1) {
        tick2 = GetTickCount();
        timet = tick2 - tick1;

        if (timet > (1000 / fps)) {
            break;
        }
    }
    tick1 = tick2;
}

// claude-ai: demo_timeout() — если TIMEOUT_DEMO=1 и timeout истёк (из env "timeout"), устанавливает GAME_STATE=0.
// claude-ai: Используется только для демо-версий (TIMEOUT_DEMO=0 в коде → функция ничего не делает).
void demo_timeout(SLONG flag)
{
#if TIMEOUT_DEMO

    static SLONG time_start = 0;
    static SLONG timeout = 0;
    SLONG time_now;

    if (flag) {
        time_start = GetTickCount();
        timeout = ENV_get_value_number("timeout", 300) * 1000;
    } else {
        time_now = GetTickCount();

        if (time_now - time_start > timeout) {
            GAME_STATE = 0;
        }
    }
#endif
}

//
// Move off the edge of map test and dodgy widget stuff
//

//
// Provided in two parts
//
// claude-ai: do_leave_map_form() — обрабатывает и рисует диалог "Leave map?" (PC-only).
// claude-ai: При подтверждении (ret==2) устанавливает GAME_STATE=0 (выход), останавливает машину/байк игрока.
void do_leave_map_form(void)
{
    SLONG ret;

    form_left_map = 15;

    POLY_frame_init(FALSE, FALSE);

    ret = FORM_Process(form_leave_map);

    if (ret) {
        FORM_Free(form_leave_map);

        form_leave_map = NULL;

        if (ret == 2) {
            GAME_STATE = 0;
        }

        {
            Thing* darci = NET_PERSON(0);

            if (darci->Genus.Person->Flags & FLAG_PERSON_DRIVING) {
                Thing* p_vehicle = TO_THING(darci->Genus.Person->InCar);

                ASSERT(p_vehicle->Class == CLASS_VEHICLE);

                p_vehicle->Velocity = 0;
                p_vehicle->Genus.Vehicle->VelX = 0;
                p_vehicle->Genus.Vehicle->VelZ = 0;
                p_vehicle->Genus.Vehicle->VelR = 0;
            }

#ifdef BIKE

            if (darci->Genus.Person->Flags & FLAG_PERSON_BIKING) {
                Thing* p_bike = TO_THING(darci->Genus.Person->InCar);

                ASSERT(p_bike->Class == CLASS_BIKE);

                p_bike->Velocity = 0;
                p_bike->Genus.Bike->back_dx = 0;
                p_bike->Genus.Bike->back_dy = 0;
                p_bike->Genus.Bike->back_dz = 0;

                p_bike->Genus.Bike->steer = 0;
                p_bike->Genus.Bike->accel = 0;
            }

#endif
        }
    } else {
        FORM_Draw(form_leave_map);

        POLY_frame_draw(FALSE, FALSE);
    }
}

//
// psx camera stuff (the PC does it in the engine?)
//
// claude-ai: psx_camera() — устанавливает матрицу камеры на PSX. На PC всегда возвращает 0.
// claude-ai: Сначала проверяет EWAY_grab_camera() (камера катсцены), иначе использует FC_cam[0].
// claude-ai: На PC камера управляется внутри AENG_draw(). Функция по сути мертва для PC. Не переносить.
SLONG psx_camera(void)
{
    //
    // AENG_draw() understands about cameras now because it has to
    // handle the splitscreen mode properly.
    //

    /*
    // snip   ( a load of camera stuff check out sourcesafe pre 20th may to see (MikeD)
    */

    return (0);
    return 0; // <-- otherwise error C4716: 'psx_camera' : must return a value
}

//
// Get what yoiu have drawn onto the Screen
//
// claude-ai: screen_flip() — финальный шаг кадра: делает скриншот если нужно, затем AENG_blit() или AENG_flip().
// claude-ai: На primary device (не 3DFX) использует blit; на 3DFX/DC — flip. На PSX сбрасывает вибрацию на экране смерти.
// claude-ai: В новой игре: просто вызов SwapBuffers()/Present(). Логика vib-сброса вошла в платформо-специфику.
inline void screen_flip(void)
{
    //
    //	sCREENSHOT just before screen_flip
    //
    extern void AENG_screen_shot(void);
    AENG_screen_shot();

    //
    // any debug text required
    //
    if (ControlFlag && allow_debug_keys) {
        AENG_draw_messages();
        FONT_buffer_draw();
    }
    /*
            if (Keys[KB_F] && allow_debug_keys)
            {
                    AENG_draw_FPS();
            }
    */

    //
    // Blitting is faster... but looks SHITE on the 3DFX because
    // it doesn't have a hardware blitter from VIDEO to VIDEO.
    //

    if (the_display.GetDriverInfo()->IsPrimary()) {
        PreFlipTT();
        AENG_blit();
    } else {
        AENG_flip();
    }

//	FLIP(NULL,DDFLIP_WAIT);
}

// claude-ai: playback_game_keys() — в режиме GS_PLAYBACK: SPACE/ENTER/геймпад прерывают воспроизведение (GAME_STATE=0).
// claude-ai: Используется для воспроизведения записанных игровых сессий (.pkt файлы). Не переносить.
void playback_game_keys(void)
{
    if (Keys[KB_SPACE] || Keys[KB_ENTER] || Keys[KB_PENTER]) {
        Keys[KB_SPACE] = 0;
        Keys[KB_ENTER] = 0;
        Keys[KB_PENTER] = 0;

        GAME_STATE = 0;
    }

    if (ReadInputDevice()) {
        SLONG i;

        for (i = 0; i <= 9; i++) {
            if (the_state.rgbButtons[i]) {
                GAME_STATE = 0;
            }
        }
    }
}

//
// For those funny fanny keys the PC likes to use
//

// claude-ai: special_keys() — обрабатывает отладочные клавиши PC: Ctrl+E → редактор, Ctrl+Q → выход, ' → single_step.
// claude-ai: В режиме single_step запятая ',' делает один шаг process_things(0). Не переносить в новую игру как есть.
SLONG special_keys(void)
{
#ifdef EDITOR
    if (ControlFlag && Keys[KB_E]) {
        GAME_STATE = GS_EDITOR;
    }
#endif

    if (GAME_STATE & GS_PLAYBACK) {
        playback_game_keys();
    }

    if (ControlFlag && Keys[KB_Q]) {
        return 1;
    }
    if (allow_debug_keys)
        if (Keys[KB_QUOTE]) {
            Keys[KB_QUOTE] = 0;
            single_step ^= 1;
        }

    if (single_step) {
        if (Keys[KB_COMMA]) {
            Keys[KB_COMMA] = 0;

            process_things(0);
        }
    }
    /*

    if (Keys[KB_TAB] || (NET_PLAYER(0)->Genus.Player->Pressed & INPUT_MASK_START))
    {
            Keys[KB_TAB] = 0;

            draw_map_screen ^= TRUE;
    }

    */

    return (0);
}

//
// Sound stuff, ask Matt
//
// claude-ai: handle_sfx() — обновление звука каждый кадр: MUSIC_mode_process(), MFX_set_listener() по позиции игрока/камеры,
// claude-ai: process_ambient_effects() + process_weather() (PC), MFX_update().
// claude-ai: BARREL_fx_rate уменьшается на 2 каждый кадр — ограничитель частоты звука взрыва бочек.
// claude-ai: На PSX: heartbeat при STATE_DEAD игрока через S_HEART_FAIL. В новой игре аналог через audio-систему.
void handle_sfx(void)
{

    //	MUSIC_process();
    MUSIC_mode_process(); // new version, much better

    SLONG cx, cy, cz, ay, ap, ar, lens;

    SLONG x = NET_PERSON(PLAYER_ID)->WorldPos.X,
          y = NET_PERSON(PLAYER_ID)->WorldPos.Y,
          z = NET_PERSON(PLAYER_ID)->WorldPos.Z;

    if (EWAY_grab_camera(&cx, &cy, &cz, &ay, &ap, &ar, &lens)) {
        MFX_set_listener(x, y, z, -(ay >> 8), -(ar >> 8), -(ap >> 8));
    } else {
        MFX_set_listener(x, y, z, -(FC_cam[0].yaw >> 8), -(FC_cam[0].roll >> 8), -(FC_cam[0].pitch >> 8));
    }

    process_ambient_effects();
    process_weather();

    extern SLONG BARREL_fx_rate;
    if (BARREL_fx_rate > 1)
        BARREL_fx_rate -= 2;
    else
        BARREL_fx_rate = 0;
    MFX_update();
}

// claude-ai: should_i_process_game() — возвращает FALSE если игра должна быть заморожена:
// claude-ai:   - показывается EWAY_tutorial_string (обучающее сообщение)
// claude-ai:   - GAMEMENU_is_paused() (PC: меню паузы открыто)
// claude-ai:   - PSX_inv_open (инвентарь PSX открыт)
// claude-ai:   - GF_PAUSED (флаг паузы в GAME_FLAGS)
// claude-ai:   - GS_LEVEL_LOST или GS_LEVEL_WON (и PSX_eog_timer истёк)
SLONG should_i_process_game(void)
{
    if (EWAY_tutorial_string) {
        //
        // Pause the game while a tutorial message appears.
        //

        return FALSE;
    }


    if (GAMEMENU_is_paused()) {
        return FALSE;
    }

    return TRUE;

    if (!(GAME_FLAGS & (GF_PAUSED | (GF_SHOW_MAP * 0))) && !form_leave_map /* && GAME_STATE != GS_LEVEL_LOST && GAME_STATE != GS_LEVEL_WON*/)
        return (1);
    return (0);
}

//
// engine or map, psx or pc, these are some of the questions we shall be resolving at compile time and runtime in the coming lines of code
//

void draw_debug_lines(void);

// claude-ai: draw_screen() — вызывает AENG_draw() для рендеринга сцены. Если draw_map_screen=TRUE, рисует карту вместо 3D (закомментировано).
// claude-ai: Также обрабатывает form_leave_map если открыт диалог выхода с карты.
inline void draw_screen(void)
{
    extern SLONG draw_3d;


#ifdef DEBUG
    draw_debug_lines();
#endif

    if (draw_map_screen) {
        //		MAP_draw();
    } else {
        AENG_draw(draw_3d);
    }

    if (form_leave_map) {
        do_leave_map_form();
    }

}

//****************************************************************
//					End of cleanup_zone, have a nice day
//****************************************************************
// claude-ai: hardware_input_replay() — проверяет нажатие кнопки "переиграть миссию" (Triangle на PSX, R на PC).
// claude-ai: replay_waiting предотвращает двойное срабатывание сразу после смерти.
SLONG hardware_input_replay(void)
{
    if (LastKey == KB_R) {
        LastKey = 0;
        return (1);
    }

    return (0);
}

// claude-ai: hardware_input_continue() — проверяет нажатие "продолжить" (SPACE/ESC/крест на PSX, геймпад на PC).
// claude-ai: На PC также проверяет что GAMEMENU не открыто (если открыто — GAMEMENU сам обрабатывает ввод).
// claude-ai: Используется при GS_LEVEL_WON/LOST для перехода к следующему экрану.
SLONG hardware_input_continue(void)
{

    extern SLONG GAMEMENU_menu_type;
    if (GAMEMENU_menu_type == 0 /*GAMEMENU_MENU_TYPE_NONE*/) {
        // No pause menu up, so wait for a keypress.
        SLONG input = get_hardware_input(INPUT_TYPE_ALL);
        if (LastKey == KB_SPACE || LastKey == KB_ESC || LastKey == KB_Z || LastKey == KB_X || LastKey == KB_C || LastKey == KB_ENTER || (input & (INPUT_MASK_SELECT | INPUT_MASK_PUNCH | INPUT_MASK_JUMP)))
        {
            LastKey = 0;

            return (1);
        }
    }

    return (0);
}

/*
UWORD darci_dlight;
*/

// claude-ai: last_fudge_message/last_fudge_camera — временные переменные для отладочного "fudge" рендеринга (PC только).
UWORD last_fudge_message;
UWORD last_fudge_camera;
// claude-ai: the_end — флаг "конец игры" (TRUE после прохождения финала). Используется для OS_hack() (финальный экран).
UBYTE the_end;


// claude-ai: env_frame_rate — целевой FPS из config.ini "max_frame_rate" (по умолчанию 30). Используется в lock_frame_rate().
UWORD env_frame_rate;

// claude-ai: Главный игровой цикл одной миссии. Порядок каждого кадра:
// claude-ai:   1. process_controls() — ввод игрока
// claude-ai:   2. process_things(1) — обновление всех Thing (StateFn каждого объекта, включает AI)
// claude-ai:   3. FC_process() — обновление камеры
// claude-ai:   4. draw_screen() — рендеринг сцены
// claude-ai:   5. screen_flip() — вывод на экран
// claude-ai:   6. lock_frame_rate() — ограничение до 30 FPS
// claude-ai: Возвращает 0 = вернуться в attract-mode, 1 = полный выход из игры
UBYTE game_loop(void)
{
    extern void save_all_nads(void);
    extern SLONG save_psx;
    if (save_psx == 2)
        save_all_nads();

    env_frame_rate = ENV_get_value_number("max_frame_rate", 30, "Render");
    AENG_set_draw_distance(ENV_get_value_number("draw_distance", 22, "Render"));
round_again:;

#ifndef NDEBUG
    if (!(GAME_STATE & GS_PLAYBACK)) {
        //
        // allways record a game, good for debuging
        //

        GAME_STATE |= GS_RECORD;
    }
#endif

    MEMORY_quick_init();

    TRACE("game_loop init1\n");

    if (game_init()) {

        TRACE("game_loop init2\n");

        already_warned_about_leaving_map = GetTickCount();
        draw_map_screen = FALSE;
        form_leave_map = NULL;
        form_left_map = 0;
        LastKey = 0;
        last_fudge_message = 0;
        last_fudge_camera = 0;

        demo_timeout(1);

        PANEL_fadeout_init();
        GAMEMENU_init();

        // start timing
        if (GAME_STATE & GS_PLAYBACK)
            BreakStart();
        SLONG exit_game_loop = FALSE;

        TRACE("game_loop init3\n");


        //
        // Initialise the SUPERFACET cache system. Allocates memory.
        //

        TRACE("game_loop init4\n");

        SUPERFACET_init();

        //
        // Initailises the FARFACET system. Allocates memory.
        //

        TRACE("game_loop init5\n");

        FARFACET_init();

        //
        // Initialises the FASTPRIM cached prim system. Allocates memory.
        //

        TRACE("game_loop init6\n");

        FASTPRIM_init();

        TRACE("game_loop init7\n");

        // MarkZA says put this in here.
        extern void envmap_specials(void);
        envmap_specials();


        TRACE("game_loop init8\n");

        // claude-ai: Inner per-frame loop - runs while playing, exits on GS_LEVEL_WON or GS_LEVEL_LOST
        while (SHELL_ACTIVE && (GAME_STATE & (GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON))) {

            // claude-ai: GAMEMENU_process() — обновляет состояние меню паузы. Возвращает GAMEMENU_DO_* код действия:
            // claude-ai:   GAMEMENU_DO_NOTHING=0, GAMEMENU_DO_RESTART, GAMEMENU_DO_CHOOSE_NEW_MISSION, GAMEMENU_DO_NEXT_LEVEL.
            // claude-ai: exit_game_loop запоминает код: как только он != 0, запускается PANEL_fadeout_start() (затемнение).
            // claude-ai: После PANEL_fadeout_finished() применяется действие (GS_REPLAY/LEVEL_WON/LEVEL_LOST/break).
            if (!exit_game_loop) {
                exit_game_loop = GAMEMENU_process();
            }


            if (exit_game_loop) {
                PANEL_fadeout_start();
            }

            if (PANEL_fadeout_finished()) {
                switch (exit_game_loop) {
                case GAMEMENU_DO_NOTHING:
                    break;

                // claude-ai: GS_REPLAY - restart current level from beginning (triggered by menu)
                case GAMEMENU_DO_RESTART:
                    GAME_STATE = GS_REPLAY;
                    break;

                case GAMEMENU_DO_CHOOSE_NEW_MISSION:
                    GAME_STATE = GS_LEVEL_LOST;
                    break;

                case GAMEMENU_DO_NEXT_LEVEL:
                    GAME_STATE = GS_LEVEL_WON;
                    break;

                default:
                    ASSERT(0);
                    break;
                }

                break;
            }

            // claude-ai: GS_LEVEL_WON handling - advance to next mission or credits
            if (GAME_STATE & GS_LEVEL_WON) {
                //
                // Exit out of the last mission straight away.
                //

                extern SLONG playing_level(const CBYTE* name);

                if (playing_level("Finale1.ucm")) {
                    GAME_STATE = GS_LEVEL_WON;
                    break;
                }
            }

            // claude-ai: EWAY_tutorial_string — активное обучающее сообщение (задаётся из EWAY_process в eway.cpp).
            // claude-ai: Пока != NULL: should_i_process_game() возвращает FALSE (игра заморожена), игра ждёт нажатия.
            // claude-ai: Нажатие до конца появления текста (counter < 64*40) ускоряет его; после — закрывает сообщение.
            if (EWAY_tutorial_string) {
                EWAY_tutorial_counter += 64 * TICK_RATIO >> TICK_SHIFT;

                if (EWAY_tutorial_counter > 64 * 20 * 2) {
                    if (hardware_input_continue()) {
                        EWAY_tutorial_string = NULL;

                        NET_PERSON(0)->Genus.Person->Flags &= ~(FLAG_PERSON_REQUEST_KICK | FLAG_PERSON_REQUEST_PUNCH | FLAG_PERSON_REQUEST_JUMP);
                        NET_PLAYER(0)->Genus.Player->InputDone = -1;
                    }
                } else {
                    if (hardware_input_continue()) {
                        EWAY_tutorial_counter = 64 * 20 * 2;
                    }
                }
            }

            //
            // if TIMEOUT_DEMO is defined will exit the game after delay specified in config.ini
            //
            demo_timeout(0);

            //
            // some pc keys, single step etc
            //
            if (special_keys())
                return (1);

            /*
            if (darci_dlight)
            {
                    Thing *darci = NET_PERSON(0);

                    NIGHT_dlight_move(
                            darci_dlight,
                            (darci->WorldPos.X >> 8),
                            (darci->WorldPos.Y >> 8) + 0x80,
                            (darci->WorldPos.Z >> 8));
            }
            */

            //
            // stuff PSX needs, does nothing on PC
            //
            // JDW: This has been moved into AENG_draw where it can do more good, it
            // now calculates if the camera is in a warehouse for me (like it should
            // have done in the first case, and returns true if it is.
            //			psx_camera();

            //
            // Warn the player if they go too near the edge of the map.
            //
            // don't do it anymore			edge_map_warning(0);

            //
            // Some processing and keyboard input for debug stuff
            //

            // claude-ai: process_controls() — читает ввод игрока (клавиатура/геймпад) и применяет к персонажу.
            // claude-ai: Пропускается если GS_LEVEL_LOST/WON или показывается обучающее сообщение.
            if (!(GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON)) && !EWAY_tutorial_string) {
                //				TRACE("Process Controls");

                process_controls();
            }
            void check_pows(void);
            check_pows();

            //
            // Process stuff
            //
            if (should_i_process_game()) {

                if (!single_step) {
                    //					TRACE("Process things\n");

                    // claude-ai: process_things(1) → вызывает StateFn каждого активного Thing (логика, физика, AI через PCOM_process_person)
                    // claude-ai: Внутри пересчитывает TICK_RATIO на основе реального времени кадра (Thing.cpp)
                    process_things(1);
                }

                //				TRACE("Process Stuff1\n");
                // claude-ai: Subsystem updates (только при should_i_process_game() == TRUE):
                // claude-ai:   PARTICLE_Run() — частицы; OB_process() — интерактивные объекты; TRIP_process() — tripwires;
                // claude-ai:   DOOR_process() — двери; EWAY_process() — миссионная логика;
                // claude-ai:   SPARK_show_electric_fences(), RIBBON_process(), DIRT_process(), ProcessGrenades() — визуальные FX;
                // claude-ai:   MAP_process() — карта мира; POW_process() — power-ups.
                PARTICLE_Run();
                OB_process();
                TRIP_process();
                DOOR_process();

                TRACE("Eway process\n");

                EWAY_process();

                //				TRACE("Process stuff2\n");

                SPARK_show_electric_fences();
                RIBBON_process();
                DIRT_process();
                ProcessGrenades();
                WMOVE_draw();
                BALLOON_process();
                MAP_process();
                POW_process();
                FC_process(); // claude-ai: Обновление камеры (только fc.cpp; cam.cpp — мёртвый код, не переносить)

            } else {
            }

            //
            // Always process these...
            //

            {
                PUDDLE_process();
                // Still need to have drips on DC (for hydrants, pissing, etc.
                DRIP_process();
            }

            //
            // Draw the game or map or whatever is going on
            //
            BreakTime("Done thing processing");

            //			TRACE("Draw screen\n");

            draw_screen();

            //
            // Draw panel and other exciting things
            //

            // claude-ai: OVERLAY_handle() — рисует HUD: панель игрока (здоровье, оружие, радар), тексты миссии, иконки.
            // claude-ai: Реализован в overlay.cpp. На PC включает PANEL_draw() с полной информацией.
            OVERLAY_handle();

            // pausing and floating text stuff

            SLONG i_want_to_exit = FALSE;


            //
            // On screen Text
            //
            // #ifndef FINAL
            //  claude-ai: CONSOLE_draw() — рисует отладочные сообщения и floating texts на экране. Пропускается в режиме паузы.
            if (!(GAME_FLAGS & GF_PAUSED))
                CONSOLE_draw();
            // #endif

            // claude-ai: GAMEMENU_draw() — рисует меню паузы (PC): анимированное меню с опциями Continue/Restart/Exit.
            GAMEMENU_draw();

            //
            // Fading out.
            //

            PANEL_fadeout_draw();

            //
            // Lets show this stuff on the monitor
            //
            BreakTime("About to flip");

            screen_flip();

            BreakTime("Done flip");

            BreakFrame();
#ifdef MIKE
            if (Keys[KB_TAB]) {
                BreakEnd("C:\\Windows\\Desktop\\BreakTimes.txt");
                Keys[KB_TAB] = 0;
            }
#endif

            //
            // Lock frame-rate to 30 FPS
            //
#ifndef BREAKTIMER
            lock_frame_rate(env_frame_rate); // claude-ai: Spin-loop busy-wait; env_frame_rate из config.ini "max_frame_rate" (по умолч. 30)
#endif

            //
            // process moveing sfx, ambient stuff etc
            //
            handle_sfx();

            // claude-ai: GAME_TURN — глобальный счётчик кадров. Инкрементируется каждый кадр (не связан с реальным временем напрямую).
            GAME_TURN++;

            // claude-ai: Каждые 1024 кадра (0x3ff+1) на шаге 314 сбрасывается GF_DISABLE_BENCH_HEALTH.
            // claude-ai: GF_DISABLE_BENCH_HEALTH ставится когда Дарси уже сидела на скамейке — сброс позволяет снова лечиться.
            // claude-ai: ~34 секунды кулдаун при 30 FPS.
            if ((GAME_TURN & 0x3ff) == 314) {
                //
                // Clearing this flag lets Darci get health when she sits on
                // a bench.
                //

                GAME_FLAGS &= ~GF_DISABLE_BENCH_HEALTH;

#ifndef NDEBUG

                // PANEL_new_text(NET_PERSON(0), 1000, "I could do with sitting on a bench!");

#endif
            }

            if (i_want_to_exit) {
                break;
            }
        }

        // end timing
        BreakEnd("C:\\Windows\\Desktop\\BreakTimes.txt");

        //
        // Game has finished, what do we do now
        //

        TRACE("game_loop game_fini\n");

        game_fini();

        TRACE("game_loop gf done\n");

        // claude-ai: После выхода из inner loop: если GS_LEVEL_WON — проверяем какой уровень завершён для катсцен.
        // claude-ai: park2.ucm → катсцена 1 (MIB introduction cutscene).
        // claude-ai: Finale1.ucm → катсцена 3 (финальный ролик) + OS_hack() (финальный экран игры).
        // claude-ai: Все остальные уровни → переход к DarciDeadCivWarnings проверке (если убиты мирные).
        if (GAME_STATE == GS_LEVEL_WON) {

            if (strstr(ELEV_fname_level, "park2.ucm")) {
                //
                // Time to play the MIB introduction cutscene.
                //

                stop_all_fx_and_music();
                the_display.RunCutscene(1);
            } else if (strstr(ELEV_fname_level, "Finale1.ucm")) {
                //
                // Game complete!
                //
                stop_all_fx_and_music();
                the_display.RunCutscene(3);

                //
                // Go into the outro.
                //

                extern void OS_hack(void);

                the_end = TRUE;

                OS_hack();

                the_end = FALSE;
            } else

            //
            // Connected else...
            //


                // claude-ai: DarciDeadCivWarnings — экран наказания за убийство мирных жителей (RedMarks > 1).
                // claude-ai: RedMarks — счётчик убитых мирных (Player->RedMarks). Только для PERSON_DARCI (основной персонаж).
                // claude-ai: DarciDeadCivWarnings=0..2 → предупреждающие экраны; >=3 → немедленный GAME_STATE=GS_LEVEL_LOST.
                // claude-ai: the_game.DarciDeadCivWarnings инкрементируется здесь после каждого показа. Персистирует между миссиями.
                if ((NETPERSON != NULL) && (NET_PERSON(0) != NULL) && (NET_PERSON(0)->Genus.Person->PersonType == PERSON_DARCI)) {
                    if (NET_PLAYER(0)->Genus.Player->RedMarks > 1) {
                        CBYTE* mess;

                        InitBackImage("deadcivs.tga");

                    Keys[KB_ESC] = 0;
                    Keys[KB_SPACE] = 0;
                    Keys[KB_ENTER] = 0;
                    Keys[KB_PENTER] = 0;

                        while (SHELL_ACTIVE) {
                            ShowBackImage();
                            POLY_frame_init(FALSE, FALSE);

                            switch (the_game.DarciDeadCivWarnings) {
                            case 0:
                                mess = XLAT_str(X_CIVSDEAD_WARNING_1);
                                break;
                            case 1:
                                mess = XLAT_str(X_CIVSDEAD_WARNING_2);
                                break;
                            case 2:
                                mess = XLAT_str(X_CIVSDEAD_WARNING_3);
                                break;

                            default:
                            case 3:
                                GAME_STATE = GS_LEVEL_LOST;

                                MENUFONT_Draw(
                                    30, 320,
                                    192,
                                    XLAT_str(X_LEVEL_LOST),
                                    0xffffffff,
                                    0);

                                mess = XLAT_str(X_CIVSDEAD_WARNING_4);

                                break;
                            }

                            FONT2D_DrawStringWrapTo(mess, 32, 82, 0x000000, 256, POLY_PAGE_FONT2D, 0, 352);
                            FONT2D_DrawStringWrapTo(mess, 30, 80, 0xffffff, 256, POLY_PAGE_FONT2D, 0, 350);

                            // FONT2D_DrawStringWrap(mess, 10, 300, 0xffffffff);
                            POLY_frame_draw(TRUE, TRUE);
                            AENG_flip();

                        if (Keys[KB_ESC] || Keys[KB_SPACE] || Keys[KB_ENTER] || Keys[KB_PENTER]) {
                            break;
                        }
                        }

                        // Bin the memory again.
                        ResetBackImage();

                    Keys[KB_ESC] = 0;
                    Keys[KB_SPACE] = 0;
                    Keys[KB_ENTER] = 0;
                    Keys[KB_PENTER] = 0;

                        the_game.DarciDeadCivWarnings += 1;
                    }
                }
        }

        // game_fini();

        // claude-ai: switch(GAME_STATE) после выхода из inner per-frame loop:
        // claude-ai:   0           → выход (PSX сбрасывает дисплей)
        // claude-ai:   GS_REPLAY   → переустанавливает GS_PLAY_GAME|GS_REPLAY и делает goto round_again (перезапуск миссии)
        // claude-ai:   GS_LEVEL_WON  → PSX: Wadmenu_gameover(1); PC: FRONTEND_level_won() чуть ниже
        // claude-ai:   GS_LEVEL_LOST → PSX: Wadmenu_gameover(0); PC: FRONTEND_level_lost() чуть ниже
        switch (GAME_STATE) {
        case 0:
            break;

        case GS_REPLAY:
            GAME_STATE = GS_PLAY_GAME | GS_REPLAY;

            goto round_again;
        case GS_LEVEL_WON:
//				STARTSCR_notify_gameover(1);
            break;
        case GS_LEVEL_LOST:
//				STARTSCR_notify_gameover(0);
            break;
        }

        // game_fini();

        NET_PERSON(0) = NULL; // For the music system...

        if (GAME_STATE == GS_LEVEL_WON) {
            FRONTEND_level_won();
        } else {
            TRACE("game_loop LOST 5\n");
            FRONTEND_level_lost();
            TRACE("game_loop LOST 6\n");
        }

        return 0;
    }

    NET_PERSON(0) = NULL; // For the music system...

    return 1;
}

//---------------------------------------------------------------

// create SMOOTH_TICK_RATIO from TICK_RATIO averaged over 4 frames
// so the car doesn't jolt so badly

// claude-ai: tick_ratios[4] — кольцевой буфер из 4 последних TICK_RATIO значений для сглаживания.
// claude-ai: sum = сумма всех 4 значений; wptr = индекс следующей записи (0..3).
static SLONG tick_ratios[4];
static SLONG wptr;
static SLONG number;
static SLONG sum;

// claude-ai: ResetSmoothTicks() — сбрасывает кольцевой буфер при инициализации игры.
void ResetSmoothTicks()
{
    wptr = 0;
    number = 0;
    sum = 0;
}

// claude-ai: SmoothTicks() — усредняет raw_ticks по 4 кадрам (скользящее среднее).
// claude-ai: Первые 3 кадра возвращает raw_ticks напрямую (буфер ещё не полон).
// claude-ai: Начиная с 4-го: вычитает старейшее значение, добавляет новое, возвращает sum>>2 (сдвиг = деление на 4).
// claude-ai: Используется в Thing.cpp для вычисления SMOOTH_TICK_RATIO — сглаженный TICK_RATIO для движения машин.
SLONG SmoothTicks(SLONG raw_ticks)
{
    if (number < 4) {
        sum += raw_ticks;
        tick_ratios[wptr++] = raw_ticks;
        number++;
        if (number < 4) {
            return raw_ticks;
        }

        wptr = 0;
        return sum >> 2;
    }

    sum -= tick_ratios[wptr];
    tick_ratios[wptr] = raw_ticks;
    sum += raw_ticks;
    wptr = (wptr + 1) & 3;

    return sum >> 2;
}
