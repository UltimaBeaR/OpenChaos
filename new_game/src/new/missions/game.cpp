// Top-level game orchestrator: startup, shutdown, per-mission init/fini, main game loop.
// All mission lifecycle management lives here: loading levels, running the per-frame loop,
// handling level win/loss, and dispatching to attract mode.

#include "missions/game.h"
#include "fallen/Headers/Game.h"

// Temporary: until these modules are migrated
#include "fallen/Headers/attract.h"   // ATTRACT_loadscreen_init, ATTRACT_loadscreen_draw
#include "fallen/Headers/id.h"       // SetSeed
#include "fallen/Headers/io.h"       // FileCreate, FileOpen, FileClose, FileRead, FileWrite, FILE_CREATION_ERROR
#include "fallen/Headers/heap.h"     // init_memory (via MEMORY_quick_init)
#include "fallen/Headers/mav.h"      // MAVHEIGHT (transitively)
#include "fallen/Headers/fog.h"      // (included via game.h chain)
#include "fallen/Headers/mist.h"
#include "fallen/Headers/cnet.h"     // CNET_network_game, CNET_configure
#include "fallen/Headers/interfac.h" // DIJOYSTATE, ReadInputDevice, get_hardware_input, process_controls, process_ambient_effects, process_weather, ControlFlag
#include "fallen/Headers/gamemenu.h" // GAMEMENU_init, GAMEMENU_process, GAMEMENU_draw, GAMEMENU_is_paused, GAMEMENU_DO_*
#include "fallen/Headers/bang.h"     // (transitively)
#include "fallen/Headers/spark.h"    // SPARK_show_electric_fences
#include "fallen/Headers/statedef.h" // GS_*, GF_*
#include "fallen/Headers/ob.h"       // OB_process
#include "fallen/Headers/morph.h"    // MORPH_load
#include "fallen/Headers/qedit.h"    // (transitively)
#include "fallen/Headers/night.h"    // NIGHT_dlight_move (referenced in comment)
#include "fallen/Headers/shadow.h"   // (transitively)
#include "fallen/Headers/ns.h"       // (transitively)
#include "fallen/Headers/supermap.h" // (transitively)
#include "fallen/Headers/build2.h"   // (transitively)
#include "fallen/Headers/pow.h"      // POW_process, check_pows
#include "fallen/Headers/widget.h"   // Form, Widget, FORM_Process, FORM_Free, FORM_Draw
#include "fallen/Headers/memory.h"   // MEMORY_quick_init, init_memory
#include "fallen/Headers/fc.h"       // FC_init, FC_cam, FC_process
#include "fallen/Headers/save.h"     // (transitively)
#include "fallen/Headers/balloon.h"  // BALLOON_process
#include "fallen/Headers/env.h"      // ENV_get_value_number
#include "fallen/Headers/wmove.h"    // WMOVE_draw
#include "fallen/DDEngine/Headers/console.h"  // CONSOLE_draw, CONSOLE_font
#include "fallen/DDEngine/Headers/poly.h"     // POLY_frame_init, POLY_frame_draw
#include "fallen/DDEngine/Headers/map.h"      // MAP_process
#include "fallen/DDEngine/Headers/menufont.h" // MENUFONT_Draw
#include "fallen/DDEngine/Headers/BreakTimer.h" // BreakStart, BreakTime, BreakEnd, BreakFrame
#include "fallen/DDEngine/Headers/truetype.h" // PreFlipTT (if defined)
#include "fallen/DDEngine/Headers/superfacet.h" // SUPERFACET_init, SUPERFACET_fini
#include "fallen/DDEngine/Headers/farfacet.h"   // FARFACET_init, FARFACET_fini
#include "fallen/DDEngine/Headers/fastprim.h"   // FASTPRIM_init, FASTPRIM_fini
#include "fallen/DDLibrary/Headers/net.h"       // NET_kill, NET_PERSON, NET_PLAYER, NO_PLAYERS, PLAYER_ID

#include "missions/elev.h"      // ELEV_load_user, ELEV_load_name, ELEV_fname_level
#include "missions/elev_globals.h"
#include "missions/eway.h"      // EWAY_process, EWAY_grab_camera, EWAY_tutorial_string, EWAY_tutorial_counter
#include "missions/eway_globals.h"

#include "world/environment/build2.h" // (transitively, if needed)
#include "world/map/supermap.h"

#include "ui/attract.h"         // ATTRACT_loadscreen_init, ATTRACT_loadscreen_draw, game_attract_mode
#include "ui/attract_globals.h" // go_into_game
#include "ui/menus/gamemenu.h"
#include "ui/pause.h"           // PANEL_fadeout_init, PANEL_fadeout_start, PANEL_fadeout_finished, PANEL_fadeout_draw, PANEL_draw_timer_do
#include "ui/hud/overlay.h"     // OVERLAY_handle
#include "ui/camera/fc.h"       // FC_init, FC_process, FC_cam

#include "actors/core/thing.h"  // process_things, TICK_RATIO, TICK_SHIFT
#include "assets/anim.h"        // ANIM_init, ANIM_fini, init_draw_tweens
#include "assets/anim_loader.h" // setup_people_anims, setup_extra_anims, setup_global_anim_array
#include "assets/level_loader.h"// (transitively)
#include "assets/texture.h"     // TEXTURE_load_needed

#include "effects/ribbon.h"     // RIBBON_process
#include "effects/tracks.h"     // (transitively)
#include "engine/effects/psystem.h" // PARTICLE_Run

// Temporary: dirt and grenade (not yet migrated)
#include "fallen/Headers/dirt.h"    // DIRT_process
#include "fallen/Headers/grenade.h" // ProcessGrenades
#include "fallen/Headers/ribbon.h"  // (old header, if needed)
#include "fallen/Headers/water.h"   // PUDDLE_process
#include "fallen/Headers/drip.h"    // DRIP_process
#include "fallen/Headers/xlat_str.h"// XLAT_str, X_*
#include "fallen/DDEngine/Headers/font2d.h"  // FONT2D_DrawStringWrapTo
#include "fallen/Headers/overlay.h" // OVERLAY_handle (old header)
#include "fallen/Headers/music.h"   // MUSIC_mode, MUSIC_mode_process, MUSIC_reset, MUSIC_WORLD
#include "fallen/DDEngine/Headers/panel.h"   // PANEL_wide_top_person, PANEL_wide_bot_person
#include "fallen/Headers/frontend.h"// FRONTEND_level_won, FRONTEND_level_lost, InitBackImage, ResetBackImage, ShowBackImage
#include "fallen/Headers/snipe.h"   // (transitively)
#include "fallen/Headers/trip.h"    // TRIP_process
#include "fallen/Headers/door.h"    // DOOR_process
#include "fallen/Headers/puddle.h"  // PUDDLE_process (if separate)
#include "fallen/Headers/pap.h"     // plan_view_shot
#include "fallen/Headers/cam.h"     // plan_view_shot (if here)

#include "engine/audio/sound.h"     // MFX_QUICK_stop, MFX_stop, MFX_set_listener, MFX_update, MFX_free_wave_list, MFX_CHANNEL_ALL, MFX_WAVE_ALL

#include "engine/graphics/graphics_api/gd_display.h" // the_display
#include "engine/graphics/pipeline/aeng.h" // AENG_init, AENG_fini, AENG_draw, AENG_flip, AENG_blit, AENG_set_draw_distance, AENG_screen_shot, AENG_draw_messages
#include "engine/input/keyboard.h"  // Keys, LastKey, KB_*
#include "engine/input/keyboard_globals.h"
#include "engine/input/mouse.h"     // MouseY, MouseX
#include "engine/input/mouse_globals.h"
#include "engine/input/joystick.h"  // GetInputDevice, JOYSTICK
#include "engine/input/joystick_globals.h"
#include "engine/io/drive.h"        // (transitively)

#include "assets/startscr.h"        // (transitively)

#include <math.h>
#include <string.h>  // strstr
#include <stdlib.h>  // exit, srand

// plan_view_shot renders the world top-down into a provided buffer (Controls.cpp variant with args).
extern void plan_view_shot(SLONG wx, SLONG wz, SLONG pixelw, SLONG sx, SLONG sy, SLONG w, SLONG h, UBYTE* buf);

extern BOOL allow_debug_keys;
extern BOOL text_fudge;
extern ULONG text_colour;
extern void draw_centre_text_at(float x, float y, CBYTE* message, SLONG font_id, SLONG flag);
extern void draw_text_at(float x, float y, CBYTE* message, SLONG font_id);
extern void draw_debug_lines(void);
extern void overlay_beacons(void);
extern SLONG draw_3d;
extern DIJOYSTATE the_state;
extern SLONG GAMEMENU_menu_type;
extern SLONG BARREL_fx_rate;
extern UBYTE combo_display;
extern UBYTE stealth_debug;

// uc_orig: stop_all_fx_and_music (fallen/Source/Game.cpp)
void stop_all_fx_and_music(void)
{
    MFX_QUICK_stop();
    MUSIC_mode(0);
    MUSIC_mode_process();
    MUSIC_reset();
    MFX_stop(MFX_CHANNEL_ALL, MFX_WAVE_ALL);
}

// uc_orig: global_load (fallen/Source/Game.cpp)
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

// uc_orig: game_startup (fallen/Source/Game.cpp)
void game_startup(void)
{
    GAME_STATE = 0;

    init_memory();
    FC_init();

    if (OpenDisplay(640, 480, 16, FLAGS_USE_3D | FLAGS_USE_WORKSCREEN) == 0) {
        GAME_STATE = GS_ATTRACT_MODE;
    } else {
        MessageBox(NULL, "Unable to open display", NULL, MB_OK | MB_ICONWARNING);
        exit(1);
    }

    AENG_init();

    ATTRACT_loadscreen_init();

    extern void MFX_init(void);
    MFX_init();

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

    CONSOLE_font("data\\font3d\\all\\", 0.2F);
}

// uc_orig: game_shutdown (fallen/Source/Game.cpp)
void game_shutdown(void)
{
    CloseDisplay();

    NET_kill();
    AENG_fini();
    ANIM_fini();
}

// uc_orig: make_texture_clumps (fallen/Source/Game.cpp)
BOOL make_texture_clumps(CBYTE* mission_name)
{
    SLONG ret;

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

    game_paused_key = -1;

    GAME_FLAGS &= ~GF_PAUSED;

    bullet_upto = -1;
    bullet_counter = 0;

    {
        ret = ELEV_load_name(mission_name);
    }

    return (ret);
}

// uc_orig: game_init (fallen/Source/Game.cpp)
BOOL game_init(void)
{
    SLONG ret;

    global_load();

    GAME_TURN = 0;
    GAME_FLAGS = 0;
    if (!CNET_network_game) {
        NO_PLAYERS = 1;
        PLAYER_ID = 0;
    }

    // TICK_RATIO is scaled each frame by the real frame time: (real_ms << TICK_SHIFT) / NORMAL_TICK_TOCK.
    // NORMAL_TICK_TOCK = 66.67ms (15 FPS base). Smoothed over 4 frames via SmoothTicks().
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

    extern int m_iPanelXPos;
    extern int m_iPanelYPos;

    if (GAME_STATE & GS_RECORD) {
        playback_file = FileCreate(playback_name, TRUE);
        verifier_file = NULL;
    } else if (GAME_STATE & GS_PLAYBACK) {
        playback_file = FileOpen(playback_name);
        verifier_file = NULL;
    }

    if (playback_file == FILE_CREATION_ERROR) {
        playback_file = NULL;
    }
    if (verifier_file == FILE_CREATION_ERROR) {
        verifier_file = NULL;
    }

    game_paused_key = -1;
    GAME_FLAGS &= ~GF_PAUSED;

    bullet_upto = -1;
    bullet_counter = 0;

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
        ret = ELEV_load_user(go_into_game);
    }

    void init_stats(void);
    init_stats();

    EWAY_tutorial_string = NULL;

    return (ret);
}

// uc_orig: game_fini (fallen/Source/Game.cpp)
void game_fini(void)
{
    stop_all_fx_and_music();

    ATTRACT_loadscreen_init();

    FASTPRIM_fini();

    SUPERFACET_fini();

    FARFACET_fini();

    void FIGURE_clean_all_LRU_slots(void);
    FIGURE_clean_all_LRU_slots();

    if (GAME_STATE != GS_REPLAY) {
        MFX_free_wave_list();
    }

    if (playback_file) {
        FileClose(playback_file);
        playback_file = NULL;
    }

    if (verifier_file) {
        FileClose(verifier_file);
        verifier_file = NULL;
    }

    NotGoingToLoadTexturesForAWhileNowSoYouCanCleanUpABit();
}

// uc_orig: game (fallen/Source/Game.cpp)
void game(void)
{
    game_startup();

    while (SHELL_ACTIVE && GAME_STATE) {
        game_attract_mode();

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

    game_shutdown();
}

// uc_orig: TAB_MAP_MIN_X (fallen/Source/Game.cpp)
#define TAB_MAP_MIN_X 9
// uc_orig: TAB_MAP_MIN_Z (fallen/Source/Game.cpp)
#define TAB_MAP_MIN_Z 13
// uc_orig: TAB_MAP_SIZE (fallen/Source/Game.cpp)
#define TAB_MAP_SIZE 448

// uc_orig: GAME_map_draw_old (fallen/Source/Game.cpp)
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

// uc_orig: GAME_map_draw (fallen/Source/Game.cpp)
void GAME_map_draw(void)
{
    Thing* darci = NET_PERSON(0);

    plan_view_shot(darci->WorldPos.X >> 8, darci->WorldPos.Z >> 8, 1 + (MouseY >> 4), 77, 78, 401, 328, (UBYTE*)screen_mem);
    overlay_beacons();
    the_display.create_background_surface((UBYTE*)screen_mem);
    the_display.blit_background_surface();
    the_display.destroy_background_surface();
}

// uc_orig: leave_map_form_proc (fallen/Source/Game.cpp)
BOOL leave_map_form_proc(Form* form, Widget* widget, SLONG message)
{
    if (widget && widget->methods == &BUTTON_Methods && message == WBN_PUSH) {
        form->returncode = widget->tag;

        return TRUE;
    } else {
        return FALSE;
    }
}

// uc_orig: process_bullet_points (fallen/Source/Game.cpp)
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

// uc_orig: lock_frame_rate (fallen/Source/Game.cpp)
void lock_frame_rate(SLONG fps)
{
    // BUGFIX: tick1/tick2 must be DWORD (same type as GetTickCount return value).
    // If uptime > ~24.8 days, GetTickCount() exceeds INT_MAX, making SLONG negative.
    // timet = (negative) - 0 = large negative → condition never true → infinite loop.
    // DWORD subtraction handles wraparound correctly via unsigned arithmetic.
    static DWORD tick1 = 0;
    DWORD tick2;
    DWORD timet;

    while (1) {
        tick2 = GetTickCount();
        timet = tick2 - tick1;

        if (timet > (1000 / fps)) {
            break;
        }
    }
    tick1 = tick2;
}

// uc_orig: demo_timeout (fallen/Source/Game.cpp)
void demo_timeout(SLONG flag)
{
    // TIMEOUT_DEMO is 0 in all builds — intentionally empty.
}

// uc_orig: do_leave_map_form (fallen/Source/Game.cpp)
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
        }
    } else {
        FORM_Draw(form_leave_map);

        POLY_frame_draw(FALSE, FALSE);
    }
}

// uc_orig: screen_flip (fallen/Source/Game.cpp)
inline void screen_flip(void)
{
    extern void AENG_screen_shot(void);
    AENG_screen_shot();

    if (ControlFlag && allow_debug_keys) {
        AENG_draw_messages();
        FONT_buffer_draw();
    }

    // Blitting is faster than flipping, but 3DFX hardware has no video-to-video blitter.
    if (the_display.GetDriverInfo()->IsPrimary()) {
        PreFlipTT();
        AENG_blit();
    } else {
        AENG_flip();
    }
}

// uc_orig: playback_game_keys (fallen/Source/Game.cpp)
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

// uc_orig: special_keys (fallen/Source/Game.cpp)
SLONG special_keys(void)
{
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

    return (0);
}

// uc_orig: handle_sfx (fallen/Source/Game.cpp)
void handle_sfx(void)
{
    MUSIC_mode_process();

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

    if (BARREL_fx_rate > 1)
        BARREL_fx_rate -= 2;
    else
        BARREL_fx_rate = 0;
    MFX_update();
}

// uc_orig: should_i_process_game (fallen/Source/Game.cpp)
SLONG should_i_process_game(void)
{
    if (EWAY_tutorial_string) {
        return FALSE;
    }

    if (GAMEMENU_is_paused()) {
        return FALSE;
    }

    return TRUE;

    // Dead code below: original had more conditions that were commented out.
    if (!(GAME_FLAGS & (GF_PAUSED | (GF_SHOW_MAP * 0))) && !form_leave_map)
        return (1);
    return (0);
}

// uc_orig: draw_screen (fallen/Source/Game.cpp)
inline void draw_screen(void)
{
    if (draw_map_screen) {
        // MAP_draw() was here — removed in original
    } else {
        AENG_draw(draw_3d);
    }

    if (form_leave_map) {
        do_leave_map_form();
    }
}

// uc_orig: hardware_input_replay (fallen/Source/Game.cpp)
SLONG hardware_input_replay(void)
{
    if (LastKey == KB_R) {
        LastKey = 0;
        return (1);
    }

    return (0);
}

// uc_orig: hardware_input_continue (fallen/Source/Game.cpp)
SLONG hardware_input_continue(void)
{
    if (GAMEMENU_menu_type == 0 /*GAMEMENU_MENU_TYPE_NONE*/) {
        SLONG input = get_hardware_input(INPUT_TYPE_ALL);
        if (LastKey == KB_SPACE || LastKey == KB_ESC || LastKey == KB_Z || LastKey == KB_X || LastKey == KB_C || LastKey == KB_ENTER || (input & (INPUT_MASK_SELECT | INPUT_MASK_PUNCH | INPUT_MASK_JUMP))) {
            LastKey = 0;

            return (1);
        }
    }

    return (0);
}

// uc_orig: game_loop (fallen/Source/Game.cpp)
UBYTE game_loop(void)
{
    extern void save_all_nads(void);

    env_frame_rate = ENV_get_value_number("max_frame_rate", 30, "Render");
    AENG_set_draw_distance(ENV_get_value_number("draw_distance", 22, "Render"));
round_again:;

    MEMORY_quick_init();

    if (game_init()) {

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

        if (GAME_STATE & GS_PLAYBACK)
            BreakStart();
        SLONG exit_game_loop = FALSE;

        SUPERFACET_init();

        FARFACET_init();

        FASTPRIM_init();

        extern void envmap_specials(void);
        envmap_specials();

        while (SHELL_ACTIVE && (GAME_STATE & (GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON))) {

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

            // Exit immediately after the final mission without waiting for input.
            if (GAME_STATE & GS_LEVEL_WON) {
                extern SLONG playing_level(const CBYTE* name);

                if (playing_level("Finale1.ucm")) {
                    GAME_STATE = GS_LEVEL_WON;
                    break;
                }
            }

            // While a tutorial string is displayed, the game is frozen.
            // Input before the message finishes (counter < 64*40) speeds it up;
            // input after closes it.
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

            demo_timeout(0);

            if (special_keys())
                return (1);

            if (!(GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON)) && !EWAY_tutorial_string) {
                process_controls();
            }
            void check_pows(void);
            check_pows();

            if (should_i_process_game()) {

                if (!single_step) {
                    process_things(1);
                }

                PARTICLE_Run();
                OB_process();
                TRIP_process();
                DOOR_process();

                EWAY_process();

                SPARK_show_electric_fences();
                RIBBON_process();
                DIRT_process();
                ProcessGrenades();
                WMOVE_draw();
                BALLOON_process();
                MAP_process();
                POW_process();
                FC_process();

            } else {
            }

            {
                PUDDLE_process();
                DRIP_process();
            }

            BreakTime("Done thing processing");

            draw_screen();

            OVERLAY_handle();

            SLONG i_want_to_exit = FALSE;

            if (!(GAME_FLAGS & GF_PAUSED))
                CONSOLE_draw();

            GAMEMENU_draw();

            PANEL_fadeout_draw();

            BreakTime("About to flip");

            screen_flip();

            BreakTime("Done flip");

            BreakFrame();

            lock_frame_rate(env_frame_rate);

            handle_sfx();

            GAME_TURN++;

            // Every ~34 seconds at 30 FPS (1024 frames), re-enable bench healing.
            if ((GAME_TURN & 0x3ff) == 314) {
                GAME_FLAGS &= ~GF_DISABLE_BENCH_HEALTH;
            }

            if (i_want_to_exit) {
                break;
            }
        }

        BreakEnd("C:\\Windows\\Desktop\\BreakTimes.txt");

        game_fini();

        if (GAME_STATE == GS_LEVEL_WON) {

            if (strstr(ELEV_fname_level, "park2.ucm")) {
                // MIB introduction cutscene after park2 mission.
                stop_all_fx_and_music();
                the_display.RunCutscene(1);
            } else if (strstr(ELEV_fname_level, "Finale1.ucm")) {
                // Final credits cutscene.
                stop_all_fx_and_music();
                the_display.RunCutscene(3);

                extern void OS_hack(void);

                the_end = TRUE;

                OS_hack();

                the_end = FALSE;
            } else

                // Warn the player if they killed too many civilians (RedMarks > 1).
                // DarciDeadCivWarnings counts shown warnings; 3 warnings causes mission failure.
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

                            POLY_frame_draw(TRUE, TRUE);
                            AENG_flip();

                            if (Keys[KB_ESC] || Keys[KB_SPACE] || Keys[KB_ENTER] || Keys[KB_PENTER]) {
                                break;
                            }
                        }

                        ResetBackImage();

                        Keys[KB_ESC] = 0;
                        Keys[KB_SPACE] = 0;
                        Keys[KB_ENTER] = 0;
                        Keys[KB_PENTER] = 0;

                        the_game.DarciDeadCivWarnings += 1;
                    }
                }
        }

        switch (GAME_STATE) {
        case 0:
            break;

        case GS_REPLAY:
            GAME_STATE = GS_PLAY_GAME | GS_REPLAY;

            goto round_again;
        case GS_LEVEL_WON:
            break;
        case GS_LEVEL_LOST:
            break;
        }

        NET_PERSON(0) = NULL;

        if (GAME_STATE == GS_LEVEL_WON) {
            FRONTEND_level_won();
        } else {
            FRONTEND_level_lost();
        }

        return 0;
    }

    NET_PERSON(0) = NULL;

    return 1;
}


// uc_orig: ResetSmoothTicks (fallen/Source/Game.cpp)
void ResetSmoothTicks(void)
{
    wptr = 0;
    number = 0;
    sum = 0;
}

// uc_orig: SmoothTicks (fallen/Source/Game.cpp)
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
