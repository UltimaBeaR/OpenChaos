// Top-level game orchestrator: startup, shutdown, per-mission init/fini, main game loop.
// All mission lifecycle management lives here: loading levels, running the per-frame loop,
// handling level win/loss, and dispatching to attract mode.

#include "game/game.h"
#include "engine/platform/uc_common.h"
#include "engine/platform/sdl3_bridge.h"
#include "game/game_types.h"
#include "things/core/thing_globals.h"  // playback_file, verifier_file
#include "things/items/special.h"       // SPECIAL_* constants for weapon_feel
#include "game/game_tick.h"                // process_controls
#include "buildings/prim.h"    // clear_prims

// These modules are not yet fully migrated:
#include "ui/frontend/attract.h"
#include "buildings/id.h"
#include "assets/formats/level_loader.h"
#include "assets/formats/level_loader_globals.h"
#include "assets/formats/anim_loader.h"
#include "assets/formats/anim_loader_globals.h"
#include "engine/core/heap.h"
#include "ai/mav.h"
#include "effects/weather/fog.h"
#include "effects/weather/fog_globals.h"
#include "effects/weather/mist.h"
#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "ui/menus/gamemenu.h"
#include "effects/combat/spark.h"
#include "things/core/statedef.h"
#include "map/ob.h"
#include "map/ob_globals.h"
#include "engine/animation/morph.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "engine/graphics/lighting/shadow.h"
#include "engine/graphics/lighting/shadow_globals.h"
#include "map/sewers.h"
#include "map/supermap.h"
#include "buildings/build2.h"
#include "effects/combat/pow.h"
#include "effects/combat/pow_globals.h"
#include "ui/menus/widget.h"
#include "ui/menus/widget_globals.h"
#include "missions/memory_globals.h"
#include "missions/memory.h"         // MEMORY_quick_init, init_memory
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "missions/save.h"
#include "things/items/balloon.h"
#include "things/items/balloon_globals.h"
#include "engine/io/env.h"
#include "navigation/wmove.h"
#include "navigation/wmove_globals.h"
#include "engine/console/console.h"  // CONSOLE_draw, CONSOLE_font
#include "engine/graphics/pipeline/poly.h"  // POLY_frame_init, POLY_frame_draw, POLY_reset_render_states
#include "assets/formats/tga.h"              // TGA_load, OpenTGAClump, CloseTGAClump
#include "ui/hud/eng_map.h"  // MAP_process
#include "ui/hud/eng_map_globals.h"
#include "engine/graphics/text/menufont.h"  // MENUFONT_Draw
#include "engine/graphics/text/font.h"
#include "engine/core/timer.h"  // BreakStart, BreakTime, BreakEnd, BreakFrame
#include "engine/graphics/geometry/superfacet.h"  // SUPERFACET_init, SUPERFACET_fini
#include "engine/graphics/geometry/farfacet.h"  // FARFACET_init, FARFACET_fini
#include "engine/graphics/geometry/fastprim.h"  // FASTPRIM_init, FASTPRIM_fini
#include "engine/graphics/geometry/fastprim_globals.h"

#include "assets/formats/elev.h"      // ELEV_load_user, ELEV_load_name, ELEV_fname_level
#include "assets/formats/elev_globals.h"
#include "missions/eway.h"      // EWAY_process, EWAY_grab_camera, EWAY_tutorial_string, EWAY_tutorial_counter
#include "missions/eway_globals.h"

#include "buildings/build2.h" // (transitively, if needed)
#include "map/supermap.h"

#include "ui/frontend/attract.h"         // ATTRACT_loadscreen_init, ATTRACT_loadscreen_draw, game_attract_mode
#include "ui/frontend/attract_globals.h" // go_into_game
#include "ui/menus/gamemenu.h"
#include "ui/menus/pause.h"           // PANEL_fadeout_init, PANEL_fadeout_start, PANEL_fadeout_finished, PANEL_fadeout_draw, PANEL_draw_timer_do
#include "ui/hud/overlay.h"     // OVERLAY_handle
#include "camera/fc.h"       // FC_init, FC_process, FC_cam
#include "engine/input/gamepad.h"    // gamepad_rumble_tick, gamepad_triggers_update
#include "engine/debug/input_debug/input_debug.h" // modal input debug panel (F11)
#include "engine/debug/debug_help/debug_help.h"   // F1 debug hotkey legend
#include "things/characters/anim_ids.h" // ANIM_HANDS_UP* for adaptive trigger check

#include "things/core/thing.h"  // process_things, TICK_RATIO, TICK_SHIFT
#include "assets/formats/anim.h"        // ANIM_init, ANIM_fini, init_draw_tweens
#include "assets/formats/anim_loader.h" // setup_people_anims, setup_extra_anims, setup_global_anim_array
#include "assets/formats/level_loader.h"// (transitively)
#include "assets/texture.h"     // TEXTURE_load_needed

#include "effects/environment/ribbon.h"     // RIBBON_process
#include "effects/environment/tracks.h"     // (transitively)
#include "engine/effects/psystem.h" // PARTICLE_Run

#include "world_objects/dirt.h"
#include "things/items/grenade.h"
#include "effects/environment/ribbon.h"
#include "effects/weather/drip.h"
#include "assets/xlat_str.h"
#include "engine/graphics/text/font2d.h"  // FONT2D_DrawStringWrapTo
#include "ui/hud/overlay.h"
#include "engine/audio/music.h"
#include "ui/hud/panel.h"  // PANEL_wide_top_person, PANEL_wide_bot_person
#include "ui/hud/panel_globals.h"
#include "ui/frontend/frontend.h"
#include "ui/frontend/frontend_globals.h"
#include "things/characters/snipe.h"
#include "things/characters/snipe_globals.h"
#include "world_objects/tripwire.h"
#include "world_objects/tripwire_globals.h"
#include "world_objects/door.h"
#include "world_objects/door_globals.h"
#include "world_objects/puddle.h"
#include "world_objects/puddle_globals.h"
#include "map/pap_globals.h"
#include "camera/cam.h"

#include "engine/audio/sound.h"     // MFX_QUICK_stop, MFX_stop, MFX_set_listener, MFX_update, MFX_free_wave_list, MFX_CHANNEL_ALL, MFX_WAVE_ALL

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/pipeline/polypage.h"  // PolyPage::SetScaling (mode change callback)
#include "engine/graphics/pipeline/aeng.h" // AENG_init, AENG_fini, AENG_draw, AENG_flip, AENG_blit, AENG_set_draw_distance, AENG_screen_shot, AENG_draw_messages
#include "engine/input/keyboard.h"  // Keys, LastKey, KB_*
#include "engine/input/keyboard_globals.h"
#include "engine/input/mouse.h"     // MouseY, MouseX
#include "engine/input/mouse_globals.h"
#include "engine/input/joystick.h"  // GetInputDevice, JOYSTICK
#include "engine/input/joystick_globals.h"

#include "ui/frontend/startscr.h"        // (transitively)

#include <math.h>
#include <string.h>  // strstr
#include <stdlib.h>  // exit, srand

// plan_view_shot renders the world top-down into a provided buffer (Controls.cpp variant with args).
extern void plan_view_shot(SLONG wx, SLONG wz, SLONG pixelw, SLONG sx, SLONG sy, SLONG w, SLONG h, UBYTE* buf);

extern BOOL allow_debug_keys;
extern BOOL g_farfacet_debug;
extern BOOL text_fudge;
extern ULONG text_colour;
extern void draw_centre_text_at(float x, float y, CBYTE* message, SLONG font_id, SLONG flag);
extern void draw_text_at(float x, float y, CBYTE* message, SLONG font_id);
extern void draw_debug_lines(void);
extern void overlay_beacons(void);
extern SLONG draw_3d;
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

// Pre-flip callback: called by the graphics backend just before frame flip.
// Handles depth reset and screensaver overlay.
static void game_pre_flip()
{
    PANEL_ResetDepthBodge();
    PANEL_screensaver_draw();
}

// Mode change callback: adjusts polygon scaling when resolution changes.
static void game_mode_changed(int32_t width, int32_t height)
{
    PolyPage::SetScaling(float(width) / 640.0f, float(height) / 480.0f);
}

// Polys-drawn callback: accumulates poly count for debug stats.
static void game_polys_drawn(int32_t count)
{
    extern int AENG_total_polys_drawn;
    AENG_total_polys_drawn += count;
}

// Render states reset callback: called by backend after texture reload.
static void game_render_states_reset()
{
    POLY_reset_render_states();
}

// TGA load callback: backend calls this to load texture pixels.
// Game code handles TGA format; backend receives raw BGRA pixels.
static bool game_tga_load(const char* name, uint32_t id, bool can_shrink,
                           uint8_t** out_pixels, int32_t* out_width, int32_t* out_height,
                           bool* out_contains_alpha)
{
    // Allocate max-size buffer (256x256 BGRA).
    *out_pixels = (uint8_t*)MemAlloc(256 * 256 * 4);
    if (!*out_pixels) return false;

    TGA_Info ti = TGA_load(name, 256, 256, (TGA_Pixel*)*out_pixels, id, can_shrink ? UC_TRUE : UC_FALSE);
    if (!ti.valid) {
        MemFree(*out_pixels);
        *out_pixels = NULL;
        return false;
    }
    *out_width = ti.width;
    *out_height = ti.height;
    *out_contains_alpha = (ti.contains_alpha != 0);
    return true;
}

// Texture reload prepare callback: opens/closes TGA clump archive for device-lost recovery.
static void game_texture_reload_prepare(bool begin, const char* clump_file, size_t clump_size)
{
    if (begin) {
        if (clump_file && clump_file[0]) OpenTGAClump(clump_file, clump_size, true);
    } else {
        CloseTGAClump();
    }
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

    // Register callbacks BEFORE OpenDisplay — SetDisplay() inside OpenDisplay()
    // calls mode_change_callback, and Flip() calls pre_flip_callback.
    ge_set_pre_flip_callback(game_pre_flip);
    ge_set_mode_change_callback(game_mode_changed);
    ge_set_polys_drawn_callback(game_polys_drawn);
    ge_set_render_states_reset_callback(game_render_states_reset);
    ge_set_tga_load_callback(game_tga_load);
    ge_set_texture_reload_prepare_callback(game_texture_reload_prepare);
    ge_set_data_dir(DATA_DIR);

    if (OpenDisplay(640, 480, 16, FLAGS_USE_3D | FLAGS_USE_WORKSCREEN) == 0) {
        GAME_STATE = GS_ATTRACT_MODE;
    } else {
        fprintf(stderr, "Unable to open display\n");
        exit(1);
    }

    ge_init();
    AENG_init();

    ATTRACT_loadscreen_init();

    extern void MFX_init(void);
    MFX_init();

    ATTRACT_loadscreen_draw(160);

    void init_joypad_config(void);
    init_joypad_config();
    ANIM_init();

    GetInputDevice(JOYSTICK, 0, UC_TRUE);

    // Play intro FMV (Eidos, Mucky Foot, intro) — skippable, respects play_movie config.
    // Must be after GetInputDevice so DualSense is initialized for skip.
    {
        extern void video_play_intro(void);
        video_play_intro();
    }

    MORPH_load();

    if (ENV_get_value_number("clumps", 0, "Secret")) {
        extern void make_all_clumps(void);
        make_all_clumps();
    }

    TEXTURE_load_needed("levels/frontend.ucm", 160, 256, 65);

    CONSOLE_font("data/font3d/all/", 0.2F);
}

// uc_orig: game_shutdown (fallen/Source/Game.cpp)
void game_shutdown(void)
{
    CloseDisplay();

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

    // Reset DualSense adaptive triggers at mission start (clears residual from previous mission).
    gamepad_triggers_off();

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
        playback_file = FileCreate(playback_name, UC_TRUE);
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

        quick_load = UC_TRUE;

        ANIM_init();

        ELEV_load_name(ELEV_fname_level);

        quick_load = UC_FALSE;

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
    gamepad_rumble_stop();
    gamepad_led_reset();
    gamepad_triggers_off();
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

    ge_texture_loading_done();
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
            // Networking removed — always fall back to attract mode.
            GAME_STATE = GS_ATTRACT_MODE;
        }
    }

    game_shutdown();
}

// uc_orig: GAME_map_draw (fallen/Source/Game.cpp)
void GAME_map_draw(void)
{
    Thing* darci = NET_PERSON(0);

    plan_view_shot(darci->WorldPos.X >> 8, darci->WorldPos.Z >> 8, 1 + (MouseY >> 4), 77, 78, 401, 328, (UBYTE*)screen_mem);
    overlay_beacons();
    ge_create_background_surface((UBYTE*)screen_mem);
    ge_blit_background_surface();
    ge_destroy_background_surface();
}

// uc_orig: leave_map_form_proc (fallen/Source/Game.cpp)
BOOL leave_map_form_proc(Form* form, Widget* widget, SLONG message)
{
    if (widget && widget->methods == &BUTTON_Methods && message == WBN_PUSH) {
        form->returncode = widget->tag;

        return UC_TRUE;
    } else {
        return UC_FALSE;
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

    text_fudge = UC_FALSE;
    text_colour = bright * 0x00010101;

    draw_centre_text_at(10, 420, bullet_point[bullet_upto], 0, 0);
}

// uc_orig: lock_frame_rate (fallen/Source/Game.cpp)
void lock_frame_rate(SLONG fps)
{
    // BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
    static uint64_t tick1 = 0;
    uint64_t tick2;
    uint64_t timet;

    while (1) {
        tick2 = sdl3_get_ticks();
        timet = tick2 - tick1;

        if (timet > (1000 / fps)) {
            break;
        }
    }
    tick1 = tick2;
}

// uc_orig: do_leave_map_form (fallen/Source/Game.cpp)
void do_leave_map_form(void)
{
    SLONG ret;

    form_left_map = 15;

    POLY_frame_init(UC_FALSE, UC_FALSE);

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

        POLY_frame_draw(UC_FALSE, UC_FALSE);
    }
}

// uc_orig: screen_flip (fallen/Source/Game.cpp)
void screen_flip(void)
{
    extern void AENG_screen_shot(void);
    AENG_screen_shot();

    // Toggle debug overlay: Ctrl press toggles debug_overlay_locked_on,
    // which forces ControlFlag=1 every frame so all debug draws activate.
    {
        static bool ctrl_was_pressed = false;
        if (Keys[KB_LCONTROL] && allow_debug_keys) {
            if (!ctrl_was_pressed) {
                ctrl_was_pressed = true;
                debug_overlay_locked_on = !debug_overlay_locked_on;
            }
        } else if (!Keys[KB_LCONTROL]) {
            ctrl_was_pressed = false;
        }
        if (ControlFlag && allow_debug_keys) {
            AENG_draw_messages();
        }
        FONT_buffer_draw();
    }

    // Blitting is faster than flipping, but 3DFX hardware has no video-to-video blitter.
    if (ge_is_primary_driver()) {
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

    if (allow_debug_keys && ControlFlag && Keys[KB_Q]) {
        return 1;
    }
    // F8 toggles single-step mode. Originally bound to the quote key
    // (') — rebound because punctuation keys are opaque in the help
    // legend ("what does ' even mean?"). F8 is the usual debugger
    // "pause/continue" key, which matches intent.
    if (allow_debug_keys)
        if (Keys[KB_F8]) {
            Keys[KB_F8] = 0;
            single_step ^= 1;
        }

    // F10: toggle far-facet debug mode (skip level geometry + shader
    // debug-split colours on far-facets). Only active after bangunsnotgames
    // cheat. See stage12_farfacets.md.
    {
        static bool f10_was_pressed = false;
        if (Keys[KB_F10] && allow_debug_keys) {
            if (!f10_was_pressed) {
                f10_was_pressed = true;
                g_farfacet_debug ^= 1;
                if (g_farfacet_debug)
                    CONSOLE_text("farfacet debug on", 3000);
                else
                    CONSOLE_text("farfacet debug off", 3000);
            }
        } else if (!Keys[KB_F10]) {
            f10_was_pressed = false;
        }
    }

    // F11: toggle modal input debug panel. Gated behind bangunsnotgames
    // so only developers hit it — regular players never see the panel
    // even by accident.
    if (allow_debug_keys) {
        static bool f11_was_pressed = false;
        if (Keys[KB_F11]) {
            if (!f11_was_pressed) {
                f11_was_pressed = true;
                input_debug_toggle();
            }
            Keys[KB_F11] = 0;
        } else {
            f11_was_pressed = false;
        }
    }

    // Drive the panel's own input (ESC exit, future page controls) before
    // the rest of the game sees any keys. Runs unconditionally so that
    // when the panel is active it owns all the subsequent input for the
    // frame (via key consumption + process_controls gate below).
    input_debug_tick();

    // F1 debug-hotkey legend — tick the 5-second visibility timer and
    // catch F1 edge presses. Tick unconditionally so the timer decays
    // even if the user toggles bangunsnotgames off while the overlay
    // is up; the F1 edge-detect itself is gated internally.
    debug_help_tick();

    // Step once while in single-step mode. Was comma (`,`) — rebound to
    // Insert for the same legend-readability reason as the F8 toggle.
    if (allow_debug_keys && single_step) {
        if (Keys[KB_INS]) {
            Keys[KB_INS] = 0;

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
        return UC_FALSE;
    }

    if (GAMEMENU_is_paused()) {
        return UC_FALSE;
    }

    return UC_TRUE;

    // Dead code below: original had more conditions that were commented out.
    if (!(GAME_FLAGS & (GF_PAUSED | (GF_SHOW_MAP * 0))) && !form_leave_map)
        return (1);
    return (0);
}

// uc_orig: draw_screen (fallen/Source/Game.cpp)
void draw_screen(void)
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

        already_warned_about_leaving_map = sdl3_get_ticks();
        draw_map_screen = UC_FALSE;
        form_leave_map = NULL;
        form_left_map = 0;
        LastKey = 0;
        last_fudge_message = 0;
        last_fudge_camera = 0;

        PANEL_fadeout_init();
        GAMEMENU_init();

        if (GAME_STATE & GS_PLAYBACK)
            BreakStart();
        SLONG exit_game_loop = UC_FALSE;

        SUPERFACET_init();

        FARFACET_init();

        FASTPRIM_init();

        extern void envmap_specials(void);
        envmap_specials();

        while (SHELL_ACTIVE && (GAME_STATE & (GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON))) {

            if (!exit_game_loop && !input_debug_is_active()) {
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

            if (special_keys())
                return (1);

            if (!(GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON)) && !EWAY_tutorial_string
                && !input_debug_is_active()) {
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

            // Gamepad output (rumble, LED, adaptive triggers) is fully
            // suspended while the input debug panel is open. The panel
            // owns the controller outputs and clears them on open — see
            // input_debug_open.
            if (!input_debug_is_active()) {

            // Update rumble motor decay and send to controller every frame.
            gamepad_rumble_tick();

            // Update DualSense LED lightbar based on player health / siren.
            // In pause menu — show default blue instead of health.
            if (GAMEMENU_is_paused()) {
                gamepad_led_reset();
            } else {
                Thing* darci = NET_PERSON(0);
                if (darci && darci->Genus.Person) {
                    float fraction;
                    bool siren_on = false;
                    if (darci->Genus.Person->InCar) {
                        Thing* car = TO_THING(darci->Genus.Person->InCar);
                        fraction = float(car->Genus.Vehicle->Health) * (1.0f / 300.0f);
                        siren_on = car->Genus.Vehicle->Siren != 0;
                    } else {
                        float max_hp = (darci->Genus.Person->PersonType == PERSON_ROPER) ? 400.0f : 200.0f;
                        fraction = float(darci->Genus.Person->Health) / max_hp;
                    }
                    gamepad_led_update(fraction, siren_on);
                }
            }

            // Update DualSense adaptive triggers based on gameplay context.
            if (GAMEMENU_is_paused()) {
                gamepad_triggers_off();
            } else {
                Thing* darci_t = NET_PERSON(0);
                if (darci_t && darci_t->Genus.Person) {
                    bool in_car = darci_t->Genus.Person->InCar != 0;
                    bool has_gun = (darci_t->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) != 0;
                    // Timer1 cooldown is deliberately NOT gated here.
                    // Keeping mode=AIM_GUN alive across the brief cooldown
                    // window avoids a Bluetooth HID latency race: if we
                    // forced mode=NONE during cooldown, then when Timer1
                    // expired we'd have to send mode=AIM_GUN again and the
                    // round-trip (~7-30ms) could miss the very next press
                    // the player makes if they're firing just above the
                    // cooldown rate.
                    //
                    // The game-side Timer1 check inside actually_fire_gun
                    // (person.cpp) still refuses the actual shot during
                    // cooldown, so no extra shot slips through — it's
                    // only the adaptive-trigger MODE that we leave on.
                    // Hardware clicks during cooldown ARE still possible
                    // (click-without-shot); acceptable per requirement R4
                    // in new_game_devlog/weapon_haptic_and_adaptive_trigger.md.
                    //
                    // `on_cooldown` is kept as a local for readability even
                    // though it's always false — leaving it visible so the
                    // next person reading this code sees the deliberate
                    // decision rather than an absence.
                    bool on_cooldown = false;

                    // States where the player physically can't fire. In these
                    // states pulling R2 doesn't produce a shot, so there
                    // shouldn't be a click either.
                    SLONG st = darci_t->State;
                    bool non_firing_state =
                        st == STATE_JUMPING || st == STATE_FALLING ||
                        st == STATE_DYING   || st == STATE_DEAD    ||
                        st == STATE_DOWN    || st == STATE_HIT     ||
                        st == STATE_HIT_RECOIL ||
                        st == STATE_CLIMBING || st == STATE_CLIMB_LADDER ||
                        st == STATE_DANGLING || st == STATE_GRAPPLING ||
                        st == STATE_USE_SCENERY || st == STATE_CHANGE_LOCATION ||
                        st == STATE_STAND_UP || st == STATE_FIGHTING ||
                        st == STATE_FIGHT;

                    // Disable weapon trigger effect when target has surrendered (hands up)
                    // or is an innocent cop — game will "talk" instead of shoot.
                    bool weapon_ready = has_gun && !on_cooldown && !non_firing_state;
                    if (weapon_ready && darci_t->Genus.Person->Target) {
                        Thing* tgt = TO_THING(darci_t->Genus.Person->Target);
                        if (tgt->Class == CLASS_PERSON) {
                            SLONG anim = tgt->Draw.Tweened->CurrentAnim;
                            if (anim == ANIM_HANDS_UP || anim == ANIM_HANDS_UP_LOOP) {
                                weapon_ready = false;
                            }
                            if (tgt->Genus.Person->PersonType == PERSON_COP &&
                                !(tgt->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY)) {
                                weapon_ready = false;
                            }
                        }
                    }

                    // Current weapon drives the WeaponFeelProfile (click feel
                    // + fire thresholds). SpecialUse == null means bare-hand
                    // pistol — SPECIAL_NONE in the profile registry.
                    int32_t current_weapon = SPECIAL_NONE;
                    if (darci_t->Genus.Person->SpecialUse) {
                        Thing* p_special = TO_THING(darci_t->Genus.Person->SpecialUse);
                        if (p_special) current_weapon = p_special->Genus.Special->SpecialType;
                    }
                    gamepad_triggers_update(in_car, weapon_ready, current_weapon);
                }
            }

            } // end of "!input_debug_is_active()" block — gamepad outputs

            {
                PUDDLE_process();
                DRIP_process();
            }

            BreakTime("Done thing processing");

            draw_screen();

            OVERLAY_handle();

            // Modal input debug overlay (F11). Same point as other HUD
            // overlays — AENG_draw_rect primitives queued here get
            // flushed with the rest of the HUD 2D batch. Queuing after
            // GAMEMENU_draw is too late because GAMEMENU_draw calls
            // POLY_frame_init internally, which clears anything queued
            // after it. See new_game_devlog/shadow_corruption_investigation.md.
            input_debug_render();

            // Debug-hotkey legend (F1 / auto-show on bangunsnotgames).
            // Pixel-coord text so it draws into the same FONT_buffer
            // stream as the rest of the HUD overlays.
            debug_help_render();

            SLONG i_want_to_exit = UC_FALSE;

            if (!(GAME_FLAGS & GF_PAUSED))
                CONSOLE_draw();

            GAMEMENU_draw();

            PANEL_fadeout_draw();

            BreakTime("About to flip");

            screen_flip();

            // End render pass — geometry added after this point (next game tick)
            // will be dropped by AENG_world_line to prevent VB pool corruption.
            extern void AENG_set_render_pass(bool active);
            AENG_set_render_pass(false);

            lock_frame_rate(env_frame_rate);

            BreakTime("Done flip");

            BreakFrame();

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

        BreakEnd("BreakTimes.txt"); // uc-abs-path: was "C:\Windows\Desktop\BreakTimes.txt"

        game_fini();

        if (GAME_STATE == GS_LEVEL_WON) {

            if (strstr(ELEV_fname_level, "park2.ucm")) {
                // MIB introduction cutscene after park2 mission.
                stop_all_fx_and_music();
                { extern void video_play_cutscene(int); video_play_cutscene(1); }
            } else if (strstr(ELEV_fname_level, "Finale1.ucm")) {
                // Final credits cutscene.
                stop_all_fx_and_music();
                { extern void video_play_cutscene(int); video_play_cutscene(3); }

                extern void OS_hack(void);
                the_end = UC_TRUE;
                OS_hack();
                the_end = UC_FALSE;
            } else

                // Warn the player if they killed too many civilians (RedMarks > 1).
                // DarciDeadCivWarnings counts shown warnings; 3 warnings causes mission failure.
                if ((NETPERSON != NULL) && (NET_PERSON(0) != NULL) && (NET_PERSON(0)->Genus.Person->PersonType == PERSON_DARCI)) {
                    if (NET_PLAYER(0)->Genus.Player->RedMarks > 1) {
                        CBYTE* mess;

                        ge_init_back_image("deadcivs.tga");

                        Keys[KB_ESC] = 0;
                        Keys[KB_SPACE] = 0;
                        Keys[KB_ENTER] = 0;
                        Keys[KB_PENTER] = 0;

                        while (SHELL_ACTIVE) {
                            ge_show_back_image();
                            POLY_frame_init(UC_FALSE, UC_FALSE);

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

                            POLY_frame_draw(UC_TRUE, UC_TRUE);
                            AENG_flip();

                            if (Keys[KB_ESC] || Keys[KB_SPACE] || Keys[KB_ENTER] || Keys[KB_PENTER]) {
                                break;
                            }
                        }

                        ge_reset_back_image();

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
