// Top-level game orchestrator: startup, shutdown, per-mission init/fini, main game loop.
// All mission lifecycle management lives here: loading levels, running the per-frame loop,
// handling level win/loss, and dispatching to attract mode.

#include "game/game.h"
#include "game/game_globals.h"
#include "engine/platform/uc_common.h"
#include "engine/platform/sdl3_bridge.h"
#include "game/game_types.h"
#include "things/core/thing_globals.h" // playback_file, verifier_file
#include "things/items/special.h" // SPECIAL_* constants for weapon_feel
#include "engine/input/weapon_feel.h" // weapon_feel_pre_release_timer1
#include "game/game_tick.h" // process_controls
#include "buildings/prim.h" // clear_prims

// These modules are not yet fully migrated:
#include "assets/formats/level_loader.h"
#include "game/input_actions.h"
#include "effects/combat/spark.h"
#include "things/core/statedef.h"
#include "map/ob.h"
#include "engine/animation/morph.h"
#include "map/supermap.h"
#include "effects/combat/pow.h"
#include "ui/menus/widget.h"
#include "missions/memory.h" // MEMORY_quick_init, init_memory
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "camera/vis_cam.h" // VC_process — post-physics camera layer
#include "things/items/balloon.h"
#include "engine/io/env.h"
#include "navigation/wmove.h"
#include "engine/console/console.h" // CONSOLE_draw, CONSOLE_font
#include "engine/graphics/pipeline/poly.h" // POLY_frame_init, POLY_frame_draw, POLY_reset_render_states
#include "assets/formats/tga.h" // TGA_load, OpenTGAClump, CloseTGAClump
#include "ui/hud/eng_map.h" // MAP_process
#include "engine/graphics/text/menufont.h" // MENUFONT_Draw
#include "engine/core/timer.h" // BreakStart, BreakTime, BreakEnd, BreakFrame
#include "engine/graphics/geometry/superfacet.h" // SUPERFACET_init, SUPERFACET_fini
#include "engine/graphics/geometry/farfacet.h" // FARFACET_init, FARFACET_fini
#include "engine/graphics/geometry/fastprim.h" // FASTPRIM_init, FASTPRIM_fini

#include "assets/formats/elev.h" // ELEV_load_user, ELEV_load_name, ELEV_fname_level
#include "assets/formats/elev_globals.h"
#include "missions/eway.h" // EWAY_process, EWAY_grab_camera, EWAY_tutorial_string, EWAY_tutorial_counter
#include "missions/eway_globals.h"

#include "ui/frontend/attract.h" // ATTRACT_loadscreen_init, ATTRACT_loadscreen_draw, game_attract_mode
#include "ui/frontend/attract_globals.h" // go_into_game
#include "ui/menus/gamemenu.h"
#include "ui/hud/overlay.h" // OVERLAY_handle
#include "game/ui_render.h" // ui_render_post_composition
#include "game/debug_timing_overlay.h" // debug_timing_overlay_render_font2d (no-op when OC_DEBUG_PHYSICS_TIMING=false)
#include "engine/input/gamepad.h" // gamepad_rumble_tick, gamepad_triggers_update
#include "engine/debug/input_debug/input_debug.h" // modal input debug panel (F11)
#include "engine/debug/debug_help/debug_help.h" // F1 debug hotkey legend
#include "engine/graphics/graphics_engine/backend_opengl/postprocess/crt_effect.h" // F2 CRT toggle
#include "things/characters/anim_ids.h" // ANIM_HANDS_UP* for adaptive trigger check

#include "things/core/thing.h" // process_things, TICK_RATIO, TICK_SHIFT
#include "assets/formats/anim.h" // ANIM_init, ANIM_fini, init_draw_tweens
#include "assets/texture.h" // TEXTURE_load_needed

#include "effects/environment/ribbon.h" // RIBBON_process
#include "effects/environment/tracks.h" // (transitively)
#include "engine/effects/psystem.h" // PARTICLE_Run

#include "world_objects/dirt.h"
#include "things/items/grenade.h"
#include "effects/weather/drip.h"
#include "assets/xlat_str.h"
#include "engine/graphics/text/font2d.h" // FONT2D_DrawStringWrapTo
#include "engine/audio/music.h"
#include "ui/hud/panel.h" // PANEL_wide_top_person, PANEL_wide_bot_person
#include "ui/hud/panel_globals.h"
#include "ui/frontend/frontend.h"
#include "things/characters/snipe.h"
#include "world_objects/tripwire.h"
#include "world_objects/door.h"
#include "world_objects/puddle.h"

#include "engine/audio/sound.h" // MFX_QUICK_stop, MFX_stop, MFX_set_listener, MFX_update, MFX_free_wave_list, MFX_CHANNEL_ALL, MFX_WAVE_ALL

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/pipeline/polypage.h" // PolyPage::SetScaling (mode change callback)
#include "engine/graphics/ui_coords.h" // ui_coords::recompute (mode change callback)
#include "engine/graphics/pipeline/aeng.h" // AENG_init, AENG_fini, AENG_draw, AENG_flip, AENG_blit, AENG_screen_shot, AENG_draw_messages
#include "engine/graphics/render_interp.h" // g_render_alpha, render_interp_capture, render_interp_reset
#include "engine/input/keyboard.h" // Keys, KB_*
#include "engine/input/keyboard_globals.h"
#include "engine/input/input_frame.h"

#include <math.h>
#include <string.h> // strstr
#include <stdlib.h> // exit, srand

extern BOOL allow_debug_keys;
extern BOOL g_farfacet_debug;
extern BOOL text_fudge;
extern void draw_debug_lines(void);
extern SLONG draw_3d;
extern SLONG GAMEMENU_menu_type;
extern SLONG BARREL_fx_rate;
extern UBYTE combo_display;
extern UBYTE stealth_debug;

// Physics-tick count executed in the most recent render frame. Written by
// the main game loop, read by the debug timing overlay. Zero outside the
// game loop.
int  g_last_phys_tick_count = 0;

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
    // Uniform scale (by vertical) — preserves aspect ratio. The extra
    // horizontal space on widescreen is filled by the wider virtual render
    // width set up in POLY_begin ("Hor+" FOV extension, not geometry
    // stretching). HUD drawn through this path ends up pillarboxed; the
    // new ui_coords pipeline (via PolyPage::push_ui_mode at draw time)
    // bypasses this scaling for UI call sites.
    const float uniform_scale = float(height) / float(DisplayHeight);
    PolyPage::SetScaling(uniform_scale, uniform_scale);
    ui_coords::recompute(width, height);
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
    if (!*out_pixels)
        return false;

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
        if (clump_file && clump_file[0])
            OpenTGAClump(clump_file, clump_size, true);
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
    ge_set_post_composition_callback(ui_render_post_composition);
#if OC_DEBUG_PHYSICS_TIMING
    ge_set_diagnostic_overlay_callback(debug_timing_overlay_render_font2d);
#endif
    ge_set_mode_change_callback(game_mode_changed);
    ge_set_polys_drawn_callback(game_polys_drawn);
    ge_set_render_states_reset_callback(game_render_states_reset);
    ge_set_tga_load_callback(game_tga_load);
    ge_set_texture_reload_prepare_callback(game_texture_reload_prepare);
    ge_set_data_dir(DATA_DIR);

    if (OpenDisplay(640, 480, 16, FLAGS_USE_3D) == 0) {
        GAME_STATE = GS_ATTRACT_MODE;
    } else {
        fprintf(stderr, "Unable to open display\n");
        exit(1);
    }

    ge_init();
    AENG_init();

    ATTRACT_loadscreen_init();

    ATTRACT_loadscreen_draw(160);

    void init_joypad_config(void);
    init_joypad_config();
    ANIM_init();

    gamepad_init();

    // Play intro FMV (Eidos, Mucky Foot, intro) — skippable, respects play_movie config.
    // Must be after gamepad_init so DualSense is initialized for skip.
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
    gamepad_shutdown();

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

    // TICK_RATIO is scaled each frame by the real frame time:
    // (real_ms << TICK_SHIFT) / THING_TICK_BASE_MS, where THING_TICK_BASE_MS
    // = 1000 / UC_PHYSICS_DESIGN_HZ = 50 ms (20 Hz reference, see thing.cpp).
    // Smoothed over 4 frames via SmoothTicks().
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

// uc_orig: lock_frame_rate (fallen/Source/Game.cpp)
//
// Original code was a pure busy-wait `while (1) { tick = ...; if (...) break; }`
// that spun one CPU core at 100% until the frame budget elapsed. Fine on Win9x
// (no power management, no DWM, fullscreen DDraw) but wasteful on modern
// targets — Steam Deck, laptops, integrated GPUs — where it cooks the CPU and
// drains battery for no reason.
//
// Modern approach:
//   1. Use the performance counter (sub-µs precision) instead of ms ticks.
//      Integer-ms ticks for fps=30 give target = 1000/30 = 33 (truncated from
//      33.333), which makes the game run at 30.3 FPS instead of 30.0. PC
//      counter tracks the exact period in counter units — zero drift.
//   2. Sleep for (remaining − SPIN), then busy-wait the last SPIN. SDL_Delay
//      yields the core to the OS; the tail spin absorbs sleep jitter so
//      frame pacing stays tight.
//
// If a frame ran long (elapsed >= target) we return immediately — no sleep,
// no spin — same as the original behavior for that path.
void lock_frame_rate(SLONG fps)
{
    // Frame-to-frame tick (in performance-counter units).
    // BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
    static uint64_t tick1 = 0;

    // fps <= 0 means "unlimited" — no cap, just record the timestamp so the
    // next call has a fresh baseline if a cap gets re-enabled mid-session.
    if (fps <= 0) {
        tick1 = sdl3_get_performance_counter();
        return;
    }

    const uint64_t pc_freq = sdl3_get_performance_frequency(); // counter ticks per second
    const uint64_t target_pc = pc_freq / (uint32_t)fps; // ticks per frame, exact

    // Sleep precision is typically 1-2 ms on modern OS (SDL3 uses Windows
    // high-resolution waitable timers / clock_nanosleep). 2 ms of tail spin
    // covers scheduler jitter without giving up the CPU savings.
    const uint64_t SPIN_PC = pc_freq / 500; // 2 ms in counter units (1/500 of a second)

    for (;;) {
        uint64_t now = sdl3_get_performance_counter();
        uint64_t elapsed = now - tick1;

        if (elapsed >= target_pc) {
            tick1 = now;
            return;
        }

        uint64_t remain = target_pc - elapsed;
        if (remain > SPIN_PC) {
            // Convert remaining (minus the tail-spin reserve) to ms. Round
            // DOWN so sleep never overshoots past the SPIN_PC safety margin.
            uint64_t sleep_pc = remain - SPIN_PC;
            uint32_t sleep_ms = uint32_t(sleep_pc * 1000 / pc_freq);
            if (sleep_ms > 0) {
                sdl3_delay_ms(sleep_ms);
            }
        }
        // else: within SPIN_PC of the deadline — fall through and loop.
        // The tight loop back to the top IS the tail-spin; no explicit sleep.
    }
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

    // Ctrl-press toggle for the debug overlay flag stays here (pure input
    // processing, not a draw). The AENG_draw_messages queue and the
    // FONT_buffer flush moved to ui_render_post_composition — they draw
    // text that must land on the default FB at native resolution, after
    // composition, so FXAA / bilinear upscale don't soften the glyphs.
    if (allow_debug_keys && input_key_just_pressed(KB_LCONTROL)) {
        debug_overlay_locked_on = !debug_overlay_locked_on;
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
    if (input_key_just_pressed(KB_SPACE) || input_key_just_pressed(KB_ENTER) || input_key_just_pressed(KB_PENTER)) {
        GAME_STATE = 0;
    }

    if (input_gamepad_connected()) {
        for (SLONG i = 0; i <= 9; i++) {
            if (input_btn_held(i)) {
                GAME_STATE = 0;
            }
        }
    }
}

#if OC_DEBUG_PHYSICS_TIMING
void check_debug_timing_keys(void)
{
    constexpr SLONG PHYS_HZ_TOGGLE_LOW   = 5;
    constexpr SLONG PHYS_HZ_FINE_MIN     = 1;
    constexpr SLONG PHYS_HZ_FINE_MAX     = UC_PHYSICS_DESIGN_HZ;

    // 30 deliberately: matches UC_VISUAL_CADENCE_HZ (PS1 hardware lock) so the
    // throttled debug mode reproduces the original visual cadence. Do not
    // change to a "rounder" number like 25 without checking with the user —
    // 30 is the meaningful value here.
    constexpr SLONG RENDER_FPS_TOGGLE_LOW = 30;

    if (input_key_just_pressed(KB_1)) {
        g_physics_hz = (g_physics_hz == UC_PHYSICS_DESIGN_HZ)
            ? PHYS_HZ_TOGGLE_LOW
            : UC_PHYSICS_DESIGN_HZ;
    }

    // DualSense touchpad click duplicates key 2 — same FPS-cap toggle without
    // reaching for the keyboard during gamepad debugging. The DS gamepad path
    // mirrors touchpad_click into rgbButtons[17], so input_btn_just_pressed
    // gives the rising-edge directly with no local edge-detect needed.
    if (input_key_just_pressed(KB_2) || input_btn_just_pressed(17)) {
        g_render_fps_cap = (g_render_fps_cap == RENDER_FPS_DEFAULT_CAP)
            ? RENDER_FPS_TOGGLE_LOW
            : RENDER_FPS_DEFAULT_CAP;
    }

    if (input_key_just_pressed(KB_3)) {
        g_render_interp_enabled = !g_render_interp_enabled;
    }

    if (input_key_just_pressed(KB_9)) {
        g_physics_hz -= 1;
        if (g_physics_hz < PHYS_HZ_FINE_MIN) g_physics_hz = PHYS_HZ_FINE_MIN;
    }

    if (input_key_just_pressed(KB_0)) {
        g_physics_hz += 1;
        if (g_physics_hz > PHYS_HZ_FINE_MAX) g_physics_hz = PHYS_HZ_FINE_MAX;
    }
}
#endif // OC_DEBUG_PHYSICS_TIMING

// uc_orig: special_keys (fallen/Source/Game.cpp)
SLONG special_keys(void)
{
    if (GAME_STATE & GS_PLAYBACK) {
        playback_game_keys();
    }

    // Ctrl+Q quit: edge-trigger via input_frame so a held Ctrl+Q triggers
    // exactly one quit attempt (was level-trigger — would loop returning 1
    // every frame while held, but caller exits on first 1, so behaviour
    // identical in practice). ControlFlag stays as the modifier source —
    // it's a separate level-state Ctrl tracker, out of scope for this
    // discrete-event migration.
    if (allow_debug_keys && ControlFlag && input_key_just_pressed(KB_Q)) {
        return 1;
    }

    // F2: toggle CRT scanline shader. Gated behind bangunsnotgames.
    if (allow_debug_keys && input_key_just_pressed(KB_F2)) {
        g_crt_enabled ^= 1;
        if (g_crt_enabled)
            CONSOLE_text((CBYTE*)"CRT shader on", 3000);
        else
            CONSOLE_text((CBYTE*)"CRT shader off", 3000);
    }

    // F8 toggles single-step mode. Originally bound to the quote key
    // (') — rebound because punctuation keys are opaque in the help
    // legend ("what does ' even mean?"). F8 is the usual debugger
    // "pause/continue" key, which matches intent.
    if (allow_debug_keys && input_key_just_pressed(KB_F8)) {
        single_step ^= 1;
    }

    // F10: toggle far-facet debug mode (skip level geometry + shader
    // debug-split colours on far-facets). Only active after bangunsnotgames
    // cheat. See stage12_farfacets.md.
    if (allow_debug_keys && input_key_just_pressed(KB_F10)) {
        g_farfacet_debug ^= 1;
        if (g_farfacet_debug)
            CONSOLE_text("farfacet debug on", 3000);
        else
            CONSOLE_text("farfacet debug off", 3000);
    }

    // F11: toggle modal input debug panel. Gated behind bangunsnotgames
    // so only developers hit it — regular players never see the panel
    // even by accident.
    if (allow_debug_keys && input_key_just_pressed(KB_F11)) {
        input_debug_toggle();
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
    if (allow_debug_keys && single_step && input_key_just_pressed(KB_INS)) {
        process_things(0);
    }

#if OC_DEBUG_PHYSICS_TIMING
    check_debug_timing_keys();
#endif

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
    // Render-side interpolation: writes interpolated WorldPos / Tweened
    // angles / FC_cam[0] into the live structs for the duration of the draw,
    // then restores authoritative values on scope exit. Every reader inside
    // the render path sees interpolated state without per-call-site plumbing.
    RenderInterpFrame interp_frame;

    if (draw_map_screen) {
        // MAP_draw() was here — removed in original
    } else {
        AENG_draw(draw_3d);
    }

    if (form_leave_map) {
        do_leave_map_form();
    }
}

// uc_orig: hardware_input_continue (fallen/Source/Game.cpp)
SLONG hardware_input_continue(void)
{
    if (GAMEMENU_menu_type == 0 /*GAMEMENU_MENU_TYPE_NONE*/) {
        SLONG input = get_hardware_input(INPUT_TYPE_ALL);
        const UBYTE last_key = input_last_key();
        if (last_key == KB_SPACE || last_key == KB_ESC || last_key == KB_Z || last_key == KB_X || last_key == KB_C || last_key == KB_ENTER || (input & (INPUT_MASK_SELECT | INPUT_MASK_PUNCH | INPUT_MASK_JUMP))) {
            input_last_key_consume();

            return (1);
        }
    }

    return (0);
}

// uc_orig: game_loop (fallen/Source/Game.cpp)
UBYTE game_loop(void)
{
    extern void save_all_nads(void);

    // max_frame_rate hardcoded: env_frame_rate default (30) from game_globals.cpp
round_again:;

    MEMORY_quick_init();

    if (game_init()) {

        already_warned_about_leaving_map = sdl3_get_ticks();
        draw_map_screen = UC_FALSE;
        form_leave_map = NULL;
        form_left_map = 0;
        input_last_key_consume();
        // Drain a stale KB_ESC press_pending carried over from the frontend so
        // GAMEMENU_process can't open the pause menu on the very first tick of
        // the level just because the user closed a frontend dialog with ESC.
        input_key_force_release(KB_ESC);
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

        uint64_t prev_frame_ms = sdl3_get_ticks();
        // Pre-charge the accumulator with one physics step so the first
        // iteration of the loop runs a physics tick BEFORE the first render
        // frame. Without this, mission_init's spawn poses (e.g. arrested
        // cops in Urban Shakedown set up standing-then-fall by their state
        // machines on the first tick) are visible for one frame in their
        // pre-tick state. Cost: mission timer / EWAY scripted events start
        // ~50 ms earlier — within frame margin, not gameplay-significant.
        // Cap on frame_dt_ms — limits physics_acc_ms growth so a long stall
        // (debugger pause, thermal throttle, OS preemption) does not enqueue
        // more than MAX_PHYSICS_TICKS_PER_FRAME physics ticks on recovery.
        static constexpr double FRAME_DT_MAX_MS = 200.0;

        // Maximum physics ticks per render frame. Derived:
        // FRAME_DT_MAX_MS / (1000 / UC_PHYSICS_DESIGN_HZ) = 200 / 50 = 4.
        // An explicit counter below makes the cap visible in the timing
        // overlay and guarantees it even when g_physics_hz is tuned at
        // runtime (debug keys 9/0).
        static constexpr int MAX_PHYSICS_TICKS_PER_FRAME =
            static_cast<int>(FRAME_DT_MAX_MS / (1000.0 / UC_PHYSICS_DESIGN_HZ));

        double   physics_acc_ms = 1000.0 / double(g_physics_hz);

        // Drop any stale interpolation snapshots from the previous mission
        // so the first capture re-seeds prev = curr (no startup judder).
        render_interp_reset();

        while (SHELL_ACTIVE && (GAME_STATE & (GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON))) {

            {
                uint64_t now_ms = sdl3_get_ticks();
                double frame_dt_ms = double(now_ms - prev_frame_ms);
                if (frame_dt_ms > FRAME_DT_MAX_MS) frame_dt_ms = FRAME_DT_MAX_MS;
                prev_frame_ms = now_ms;
                physics_acc_ms += frame_dt_ms;
                // Publish wall-clock dt for render-side effects (rain
                // density, per-puddle drip spawn) — see g_frame_dt_ms.
                g_frame_dt_ms = float(frame_dt_ms);
                // Tick the 30 Hz visual cadence counter for GAME_TURN-
                // gated visuals (siren flash, wheel rotation, etc.) —
                // see VISUAL_TURN.
                visual_turn_tick(g_frame_dt_ms);
            }

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

                // Reset UI state on exit. Both of these are latent bugs
                // pre-existing the split — the post-composition UI pass
                // now renders on top of the default FB after composition,
                // so whatever the next mode (main menu frontend) draws
                // into the scene FBO can't overwrite stale UI state. Reset
                // here so the next frame's UI pass sees clean flags.
                //   PANEL_fadeout_time — black cat-iris accumulating quad
                //       would keep drawing and cover the whole screen.
                //   GAMEMENU_menu_type — "Are you sure? OK / Cancel"
                //       dialog would persist on top of the main menu.
                PANEL_fadeout_init();
                GAMEMENU_init();

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

                // Minimum display time (~1s; counter ticks at 960/s) before
                // close/skip input is accepted — prevents instant dismissal
                // when a button is already held as the dialog appears.
                // hardware_input_continue() is called unconditionally to
                // drain LastKey so a stale press from before the dialog
                // doesn't leak into the post-grace close check.
                const SLONG EWAY_TUT_INPUT_GRACE = 960;
                SLONG continue_pressed = hardware_input_continue();

                if (EWAY_tutorial_counter >= EWAY_TUT_INPUT_GRACE) {
                    if (EWAY_tutorial_counter > 64 * 20 * 2) {
                        if (continue_pressed) {
                            EWAY_tutorial_string = NULL;

                            NET_PERSON(0)->Genus.Person->Flags &= ~(FLAG_PERSON_REQUEST_KICK | FLAG_PERSON_REQUEST_PUNCH | FLAG_PERSON_REQUEST_JUMP);
                            NET_PLAYER(0)->Genus.Player->InputDone = -1;
                        }
                    } else {
                        if (continue_pressed) {
                            EWAY_tutorial_counter = 64 * 20 * 2;
                        }
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

            {
                const double phys_step_ms  = 1000.0 / double(g_physics_hz);
                const SLONG  phys_tick_diff = 1000 / g_physics_hz;
                const float  phys_dt_ms    = float(phys_step_ms);

                g_last_phys_tick_count = 0;
                while (should_i_process_game() && physics_acc_ms >= phys_step_ms
                       && g_last_phys_tick_count < MAX_PHYSICS_TICKS_PER_FRAME) {
                    g_last_phys_tick_count++;

                    OVERLAY_begin_physics_tick();

                    if (!single_step) {
                        process_things(1, phys_tick_diff);
                    }

                    PARTICLE_Run();
                    OB_process();
                    TRIP_process();
                    DOOR_process();

                    EWAY_process();

                    // SPARK_show_electric_fences and RIBBON_process are
                    // called once per render frame (below, after the
                    // physics loop) with the wall-clock frame dt — both
                    // are pure visual effects calibrated against the
                    // 30 Hz visual cadence, must be independent of
                    // physics rate. Same with DRIP_process / PUDDLE_process.
                    DIRT_process();
                    ProcessGrenades();
                    WMOVE_draw();
                    BALLOON_process();
                    MAP_process();
                    POW_process();
                    FC_process();

                    // VisCam layer: post-physics camera computation.
                    // Reads FC_cam[*], writes VC_state[*]. Render-interp
                    // snapshots VC_state for the visible frame.
                    VC_process();

                    GAME_TURN++;
                    // GAME_TURN ticks once per physics tick (= UC_PHYSICS_DESIGN_HZ
                    // = 20/sec by default). Original ticked GAME_TURN per render
                    // frame (PS1 = 30/sec, PC config default = 30/sec, PC retail
                    // ~22/sec) — moving it here makes it strictly game-state
                    // (paused on pause, decoupled from render). Visual effects
                    // that used GAME_TURN as a "render cadence" counter are
                    // migrated to VISUAL_TURN (separate 30/sec wall-clock
                    // counter) — see new_game_devlog/fps_unlock/original_tick_rates/.
                    //
                    // Bench-health re-enable: 1024 GAME_TURN cycle = ~51 sec at
                    // 20 Hz (was ~34 sec at 30 Hz on PS1). 314 magic constant is
                    // an arbitrary phase offset, not load-bearing.
                    if ((GAME_TURN & 0x3ff) == 314) {
                        GAME_FLAGS &= ~GF_DISABLE_BENCH_HEALTH;
                    }

                    // Render-side interpolation: snapshot post-tick state of
                    // moving Things and the camera. Done at the end of each
                    // physics tick so that on multi-tick frames snap.prev/curr
                    // always span the most recent tick boundary.
                    //
                    // Camera is captured so body-vs-camera stay on the same
                    // render-time clock — without this, the interpolated body
                    // snaps relative to the discretely-updated camera at every
                    // tick boundary.
                    {
                        // Walk per-class linked lists for movement-bearing
                        // classes only. Each class is a small list (<400
                        // entries combined), much cheaper than scanning all
                        // 701 Thing slots.
                        const UBYTE moving_classes[] = {
                            CLASS_PERSON,
                            CLASS_VEHICLE,
                            CLASS_PROJECTILE,
                            CLASS_ANIMAL,
                            CLASS_PYRO,
                            CLASS_CHOPPER,
                            CLASS_PLAT,
                            CLASS_BARREL,
                            CLASS_BAT,
                            CLASS_BIKE,
                            CLASS_SPECIAL,  // grenades thrown can move
                        };
                        for (UBYTE c : moving_classes) {
                            UWORD t_idx = thing_class_head[c];
                            while (t_idx) {
                                Thing* p = TO_THING(t_idx);
                                render_interp_capture(p);
                                t_idx = p->NextLink;
                            }
                        }
                        render_interp_capture_camera(&FC_cam[0]);
                        // Cutscene camera lives in EWAY_cam_* globals (separate
                        // from FC_cam), and EWAY_grab_camera copies them into
                        // fc->* inside the renderer, overwriting our FC_cam apply
                        // with raw post-tick state. Snapshot them too so the
                        // renderer can substitute interpolated values right after
                        // EWAY_grab_camera writes. EWAY_process inside this same
                        // physics tick has already updated EWAY_cam_*.
                        render_interp_capture_eway_camera();
                        // DIRT pool (leaves, brass, cans, etc) — separate from
                        // THINGS[]. DIRT_process above advanced positions /
                        // angles for this tick; snapshot them so the renderer
                        // can lerp between ticks instead of showing them
                        // judder-stepping at physics rate.
                        render_interp_capture_dirt();
                        // Grenade pool (GrenadeArray) — separate from THINGS[]
                        // and DIRT, rendered directly via DrawGrenades().
                        // ProcessGrenades above has already advanced positions
                        // and angles for this tick.
                        render_interp_capture_grenades();
                    }

                    physics_acc_ms -= phys_step_ms;
                }
                // If the per-frame tick cap fired, discard leftover accumulator
                // so it does not carry over as an extra burst next frame.
                // (At design rate this is belt-and-suspenders alongside the
                // FRAME_DT_MAX_MS clamp; it matters if g_physics_hz is tuned
                // to a higher rate via debug keys while a stall is in progress.)
                if (g_last_phys_tick_count >= MAX_PHYSICS_TICKS_PER_FRAME)
                    physics_acc_ms = 0.0;

                const bool game_frozen = !should_i_process_game();
                if (game_frozen) physics_acc_ms = 0.0;

                // Alpha = how far we are between the last and next physics
                // tick. Read by RenderInterpFrame at draw time. Clamped to
                // [0, 1] so lerp never overshoots curr.
                //
                // On pause: clamp to 1 so the world is rendered at the latest
                // captured snapshot instead of falling back to the older
                // "prev" tick (acc=0 → alpha=0 → lerp returns prev). Without
                // this, the moment of pausing visibly snaps moving objects
                // back ~50 ms (one physics tick).
                {
                    double a = game_frozen ? 1.0 : (physics_acc_ms / phys_step_ms);
                    if (a < 0.0) a = 0.0;
                    else if (a > 1.0) a = 1.0;
                    g_render_alpha = float(a);
                }
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
                        // Player has a firearm in hand. Two storage paths in the
                        // original game: pistol uses FLAG_PERSON_GUN_OUT (no
                        // Thing* — pistol is implicit), heavy weapons (AK47,
                        // shotgun) live in SpecialUse with FLAG_PERSON_GUN_OUT
                        // cleared (see set_person_draw_item in person.cpp).
                        // Treat either as "gun out" for adaptive-trigger purposes.
                        bool has_pistol_out = (darci_t->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) != 0;
                        bool has_heavy_out = false;
                        // Also compute current_weapon here so we can look up the
                        // WeaponFeelProfile early and branch on auto-fire below.
                        int32_t current_weapon = SPECIAL_NONE;
                        if (darci_t->Genus.Person->SpecialUse) {
                            Thing* p_su = TO_THING(darci_t->Genus.Person->SpecialUse);
                            if (p_su) {
                                current_weapon = p_su->Genus.Special->SpecialType;
                                has_heavy_out = (current_weapon == SPECIAL_AK47 || current_weapon == SPECIAL_SHOTGUN);
                            }
                        }
                        bool has_gun = has_pistol_out || has_heavy_out;
                        // Cooldown signals taken directly from game state —
                        // no wall-clock timers or per-weapon magic numbers.
                        // Timer1 is the unified post-shot cooldown counter
                        // for BOTH running and standing fire: set to the
                        // shoot animation's derived tick count, decremented
                        // in STATE_MOVEING SUB_STATE_RUNNING and in
                        // STATE_GUN SUB_STATE_SHOOT_GUN (player only).
                        // State is still checked so a frozen Timer1 left
                        // over from a running shot the player interrupted
                        // by stopping doesn't wrongly gate an unrelated
                        // future state.
                        //
                        // Pre-release: motor mode=AIM_GUN turns on while
                        // Timer1 is in the last couple of ticks of the
                        // cooldown, so the HID packet has time to
                        // propagate before the player's next press reaches
                        // the click point. weapon_feel captures any remaining
                        // Timer1 at fire as debt so average rate stays
                        // pinned to the anim duration.
                        const SLONG pre_release = weapon_feel_pre_release_timer1();
                        const bool in_running_cooldown = (darci_t->State == STATE_MOVEING && darci_t->Genus.Person->Timer1 > pre_release);
                        const bool in_standing_cooldown = (darci_t->State == STATE_GUN && darci_t->SubState == SUB_STATE_SHOOT_GUN && darci_t->Genus.Person->Timer1 > pre_release);
                        bool on_cooldown = in_running_cooldown || in_standing_cooldown;

                        // States where the player physically can't fire. In these
                        // states pulling R2 doesn't produce a shot, so there
                        // shouldn't be a click either.
                        SLONG st = darci_t->State;
                        bool non_firing_state = st == STATE_JUMPING || st == STATE_FALLING || st == STATE_DYING || st == STATE_DEAD || st == STATE_DOWN || st == STATE_HIT || st == STATE_HIT_RECOIL || st == STATE_CLIMBING || st == STATE_CLIMB_LADDER || st == STATE_DANGLING || st == STATE_GRAPPLING || st == STATE_USE_SCENERY || st == STATE_CHANGE_LOCATION || st == STATE_STAND_UP || st == STATE_FIGHTING || st == STATE_FIGHT;

                        // Disable weapon trigger effect when target has surrendered (hands up)
                        // or is an innocent cop — game will "talk" instead of shoot.
                        // Only applies when the player is standing still: set_person_shoot
                        // handles the surrender case (makes the target lie down / shows
                        // "can't shoot cop") so no real shot fires — no click either.
                        // On the run, set_person_running_shoot ignores target status and
                        // always fires, so the click must still happen; we skip the
                        // surrender gate in that state.
                        // mag_empty: weapon's current clip is 0 (and the weapon
                        // is one that has a clip to be empty). Used by the
                        // adaptive trigger to switch from Machine to a
                        // pistol-style Weapon25 click so the reload press
                        // feels mechanical. Pistol also tracks ammo but its
                        // own effect is already a click, so mag_empty
                        // doesn't change anything for it (profile has
                        // reload_click_strength=0).
                        //
                        // Extended mag_empty: also TRUE while the reload gate
                        // is set. Without this extension, the hardware click
                        // window is too short: the game detects PUNCH at the
                        // Machine-start zone (r2=112) and immediately refills
                        // the clip + sends mode=NONE, but the Weapon25 click
                        // fires at the reload-click end zone (r2=168) —
                        // trigger hasn't reached that point yet. Keeping the
                        // Weapon25 effect active through the reload-gate
                        // window (from reload press until next fire or R2
                        // release) gives the hardware enough time to complete
                        // the click.
                        bool mag_empty = false;
                        if (darci_t->Genus.Person->SpecialUse) {
                            Thing* p_su2 = TO_THING(darci_t->Genus.Person->SpecialUse);
                            if (p_su2 && p_su2->Genus.Special->ammo == 0) {
                                mag_empty = true;
                            }
                        } else if (has_pistol_out && darci_t->Genus.Person->Ammo == 0) {
                            mag_empty = true;
                        }
                        if (input_actions_ak47_reload_gate_set()) {
                            mag_empty = true;
                        }

                        // Per-weapon trigger effect policy lives in weapon_feel.
                        // Auto-fire (AK47 Machine): ON while in_shot_cycle —
                        // represents recoil from real shots, so the pulse
                        // dies when shooting stops. Additionally ON when
                        // mag_empty AND the profile has a reload click
                        // configured (gamepad swaps to Weapon25 with reload
                        // params). Single-shot (pistol Weapon25): OFF while
                        // in_shot_cycle (same gate as before — click can't
                        // fire during cooldown), ON otherwise.
                        // in_shot_cycle is the existing on_cooldown signal
                        // (Timer1 > pre_release AND matching state).
                        const bool trigger_effect_active = weapon_feel_trigger_effect_should_run(current_weapon, on_cooldown, mag_empty);
                        bool weapon_ready = has_gun && !non_firing_state && trigger_effect_active;
                        const bool is_running = (darci_t->State == STATE_MOVEING);
                        if (weapon_ready && !is_running && darci_t->Genus.Person->Target) {
                            Thing* tgt = TO_THING(darci_t->Genus.Person->Target);
                            if (tgt->Class == CLASS_PERSON) {
                                SLONG anim = tgt->Draw.Tweened->CurrentAnim;
                                if (anim == ANIM_HANDS_UP || anim == ANIM_HANDS_UP_LOOP) {
                                    weapon_ready = false;
                                }
                                if (tgt->Genus.Person->PersonType == PERSON_COP && !(tgt->Genus.Person->Flags2 & FLAG2_PERSON_GUILTY)) {
                                    weapon_ready = false;
                                }
                            }
                        }

                        // current_weapon computed up top alongside has_heavy_out.
                        gamepad_triggers_update(in_car, weapon_ready, current_weapon, mag_empty);
                    }
                }

            } // end of "!input_debug_is_active()" block — gamepad outputs

            // Render-side wall-clock animation: puddle splash blobs,
            // drip ripples, ribbons (electric wires / convect smoke
            // tendrils) and electric-fence spark bursts all run at a
            // fixed 30 Hz cadence regardless of physics or render rate.
            PUDDLE_process(g_frame_dt_ms);
            DRIP_process(g_frame_dt_ms);
            RIBBON_process(g_frame_dt_ms);
            SPARK_show_electric_fences(g_frame_dt_ms);

            BreakTime("Done thing processing");

            draw_screen();

            OVERLAY_handle();

            SLONG i_want_to_exit = UC_FALSE;

            // input_debug_render, debug_help_render, CONSOLE_draw,
            // GAMEMENU_draw, PANEL_fadeout_draw moved to
            // ui_render_post_composition — they run after composition_blit
            // so their text/graphics aren't softened by FXAA.
            // See split_ui_from_scene_plan.md.

            BreakTime("About to flip");

            screen_flip();

            // End render pass — geometry added after this point (next game tick)
            // will be dropped by AENG_world_line to prevent VB pool corruption.
            extern void AENG_set_render_pass(bool active);
            AENG_set_render_pass(false);

            lock_frame_rate(g_render_fps_cap);

            BreakTime("Done flip");

            BreakFrame();

            handle_sfx();

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
                {
                    extern void video_play_cutscene(int);
                    video_play_cutscene(1);
                }
            } else if (strstr(ELEV_fname_level, "Finale1.ucm")) {
                // Final credits cutscene. See note in elev.cpp Finale1 load:
                // cutscene 2/3 are swapped vs. prerelease — retail files have
                // outro in New_PCcutscene2_300.bik.
                stop_all_fx_and_music();
                {
                    extern void video_play_cutscene(int);
                    video_play_cutscene(2);
                }

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

                        // Clear any pending press carried in from the previous
                        // screen so the warning loop doesn't dismiss instantly.
                        input_key_force_release(KB_ESC);
                        input_key_force_release(KB_SPACE);
                        input_key_force_release(KB_ENTER);
                        input_key_force_release(KB_PENTER);

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

                            if (input_key_just_pressed(KB_ESC) || input_key_just_pressed(KB_SPACE)
                                || input_key_just_pressed(KB_ENTER) || input_key_just_pressed(KB_PENTER)) {
                                break;
                            }
                        }

                        ge_reset_back_image();

                        input_key_force_release(KB_ESC);
                        input_key_force_release(KB_SPACE);
                        input_key_force_release(KB_ENTER);
                        input_key_force_release(KB_PENTER);

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
