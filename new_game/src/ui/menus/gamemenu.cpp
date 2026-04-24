// In-game pause/won/lost menus rendered as an overlay during gameplay.
// Menu type constants are private — only DO_* return codes leave this file.

#include "game/game_types.h"
#include "engine/platform/sdl3_bridge.h"

#include "ui/hud/panel.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypage.h"  // PolyPage::UIModeScope
#include "engine/graphics/ui_coords.h"          // UIAnchor
#include "engine/graphics/text/menufont.h"
#include "assets/xlat_str.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "engine/audio/music.h"
#include "missions/memory.h"

#include "ui/menus/gamemenu.h"
#include "ui/menus/gamemenu_globals.h"
#include "engine/input/joystick.h"
#include "engine/input/gamepad.h"
#include "engine/input/gamepad_globals.h"

// Joystick axis deadzone for menu navigation.
#define GM_AXIS_CENTRE 32768
#define GM_NOISE_TOLERANCE 4096
#define GM_AXIS_MIN (GM_AXIS_CENTRE - GM_NOISE_TOLERANCE)
#define GM_AXIS_MAX (GM_AXIS_CENTRE + GM_NOISE_TOLERANCE)

extern void process_things_tick(SLONG frame_rate_independant);

// Menu type constants — file private.
// uc_orig: GAMEMENU_MENU_TYPE_NONE (fallen/Source/gamemenu.cpp)
#define GAMEMENU_MENU_TYPE_NONE 0
// uc_orig: GAMEMENU_MENU_TYPE_PAUSE (fallen/Source/gamemenu.cpp)
#define GAMEMENU_MENU_TYPE_PAUSE 1
// uc_orig: GAMEMENU_MENU_TYPE_WON (fallen/Source/gamemenu.cpp)
#define GAMEMENU_MENU_TYPE_WON 2
// uc_orig: GAMEMENU_MENU_TYPE_LOST (fallen/Source/gamemenu.cpp)
#define GAMEMENU_MENU_TYPE_LOST 3
// uc_orig: GAMEMENU_MENU_TYPE_SURE (fallen/Source/gamemenu.cpp)
#define GAMEMENU_MENU_TYPE_SURE 4
// uc_orig: GAMEMENU_MENU_TYPE_NUMBER (fallen/Source/gamemenu.cpp)
#define GAMEMENU_MENU_TYPE_NUMBER 5

// Resets fade/animation state and sets the active menu type.
// When switching between non-NONE menus only the text re-fades; overlay stays.
// uc_orig: GAMEMENU_initialise (fallen/Source/gamemenu.cpp)
void GAMEMENU_initialise(SLONG menu)
{
    // When closing the menu, consume held gamepad buttons so they don't leak
    // to gameplay (e.g. Triangle=KICK, Cross=JUMP, Start would re-trigger).
    if (GAMEMENU_menu_type != GAMEMENU_MENU_TYPE_NONE && menu == GAMEMENU_MENU_TYPE_NONE) {
        gamepad_consume_until_released(0);  // Cross/A
        gamepad_consume_until_released(3);  // Triangle/Y
        gamepad_consume_until_released(6);  // Start
    }

    if (GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_NONE || menu == GAMEMENU_MENU_TYPE_NONE) {
        MENUFONT_fadein_init();

        GAMEMENU_background = 0;
        GAMEMENU_fadein_x = 0;
        GAMEMENU_slowdown = 0x10000;
        GAMEMENU_menu_selection = 1;

        ResetSmoothTicks();
    } else {
        // Moving from one menu to another — re-fade text only, keep background and slowdown.
        GAMEMENU_fadein_x = 0;
    }

    GAMEMENU_menu_type = menu;
}

// uc_orig: GAMEMENU_init (fallen/Source/gamemenu.cpp)
void GAMEMENU_init()
{
    MENUFONT_fadein_init();

    GAMEMENU_background = 0;
    GAMEMENU_fadein_x = 0;
    GAMEMENU_slowdown = 0x10000;
    GAMEMENU_menu_selection = 1;
    GAMEMENU_wait = 0;
    GAMEMENU_level_lost_reason = NULL;

    GAMEMENU_initialise(GAMEMENU_MENU_TYPE_NONE);
}

// Processes input and game-state transitions each frame.
// Returns a DO_* code when the player makes a selection that requires a game-loop response.
// uc_orig: GAMEMENU_process (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_process()
{
    SLONG i;

    SLONG millisecs;

    // BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
    static uint64_t tick_last = 0;
    static uint64_t tick_now = 0;

    tick_now = sdl3_get_ticks();

    millisecs = tick_now - tick_last;
    tick_last = tick_now;

    SATURATE(millisecs, 10, 200);

    // Automatically count up a delay before showing won/lost menu.
    if (GAME_STATE & (GS_LEVEL_LOST | GS_LEVEL_WON)) {
        if (MFX_QUICK_still_playing() && !MUSIC_is_playing()) {
            // Speech is still playing — hold.
        } else {
            GAMEMENU_wait += 64 * TICK_RATIO >> TICK_SHIFT;
        }
    }

    if (GAMEMENU_wait > (64 * 20 * 2) && GAMEMENU_menu_type != GAMEMENU_MENU_TYPE_SURE) {
        if (GAME_STATE & GS_LEVEL_LOST) {
            if (GAMEMENU_menu_type != GAMEMENU_MENU_TYPE_LOST) {
                GAMEMENU_initialise(GAMEMENU_MENU_TYPE_LOST);
                if (NET_PERSON(0)->Genus.Person->Health > 0)
                    MUSIC_mode(MUSIC_MODE_CHAOS);
                else
                    MUSIC_mode(MUSIC_MODE_GAMELOST);
            }
        } else if (GAME_STATE & GS_LEVEL_WON) {
            if (GAMEMENU_menu_type != GAMEMENU_MENU_TYPE_WON) {
                GAMEMENU_initialise(GAMEMENU_MENU_TYPE_WON);
                MUSIC_mode(MUSIC_MODE_GAMEWON);
            }
        }

        if (GAME_STATE & (GS_LEVEL_WON | GS_LEVEL_LOST)) {
            if (GAMEMENU_is_paused())
                process_things_tick(1);
            MUSIC_mode_process();
        }

        // Ensure widescreen cam and cut-scene mode are off.
        extern SLONG EWAY_cam_freeze;
        extern UBYTE GAME_cut_scene;

        EWAY_cam_freeze = UC_FALSE;
        GAME_cut_scene = UC_FALSE;
    }

    // Poll gamepad and translate to keyboard keys for menu input.
    {
        static UBYTE gm_last_start = 0;
        static UBYTE gm_last_triangle = 0;
        static UBYTE gm_last_cross = 0;
        static SLONG gm_last_dir = 0;
        static uint64_t gm_dir_next_fire = 0;

        ReadInputDevice();

        // Start button (index 6) → ESC (toggle pause). Always active (even during gameplay).
        UBYTE start_now = the_state.rgbButtons[6] ? 1 : 0;
        if (start_now && !gm_last_start)
            Keys[KB_ESC] = 1;
        gm_last_start = start_now;

        // Navigation/confirm/cancel — only when a menu is actually active.
        // Setting Keys[] during gameplay would cause infinite walking (Keys never auto-clear).
        if (GAMEMENU_menu_type != GAMEMENU_MENU_TYPE_NONE) {
            // Triangle/Y (index 3) → ESC (back/cancel). Edge-detect.
            UBYTE triangle_now = the_state.rgbButtons[3] ? 1 : 0;
            if (triangle_now && !gm_last_triangle)
                Keys[KB_ESC] = 1;
            gm_last_triangle = triangle_now;

            // Cross/A (index 0) → Enter (confirm). Edge-detect.
            UBYTE cross_now = the_state.rgbButtons[0] ? 1 : 0;
            if (cross_now && !gm_last_cross)
                Keys[KB_ENTER] = 1;
            gm_last_cross = cross_now;

            // Stick/D-Pad → Up/Down with time-based auto-repeat.
            SLONG dir = 0;
            if (the_state.lY < GM_AXIS_MIN)
                dir = 1; // up
            else if (the_state.lY > GM_AXIS_MAX)
                dir = 2; // down

            {
                uint64_t now = sdl3_get_ticks();
                if (dir) {
                    if (dir != gm_last_dir) {
                        if (dir == 1) Keys[KB_UP] = 1;
                        else Keys[KB_DOWN] = 1;
                        gm_dir_next_fire = now + 400;
                    } else if (now >= gm_dir_next_fire) {
                        if (dir == 1) Keys[KB_UP] = 1;
                        else Keys[KB_DOWN] = 1;
                        gm_dir_next_fire = now + 150;
                    }
                }
            }
            gm_last_dir = dir;
        } else {
            // Menu not active — reset edge-detect state so first press is clean on menu open.
            gm_last_triangle = the_state.rgbButtons[3] ? 1 : 0;
            gm_last_cross = the_state.rgbButtons[0] ? 1 : 0;
            gm_last_dir = 0;
            gm_dir_next_fire = 0;
        }
    }

    if (Keys[KB_ESC]) {
        Keys[KB_ESC] = 0;

        switch (GAMEMENU_menu_type) {
        case GAMEMENU_MENU_TYPE_NONE:
            MFX_stop(MFX_CHANNEL_ALL, MFX_WAVE_ALL);
            GAMEMENU_initialise(GAMEMENU_MENU_TYPE_PAUSE);
            break;

        case GAMEMENU_MENU_TYPE_PAUSE:
            GAMEMENU_initialise(GAMEMENU_MENU_TYPE_NONE);
            break;

        case GAMEMENU_MENU_TYPE_WON:
            return GAMEMENU_DO_NEXT_LEVEL;

        case GAMEMENU_MENU_TYPE_LOST:
            return GAMEMENU_DO_CHOOSE_NEW_MISSION;

        case GAMEMENU_MENU_TYPE_SURE:
            GAMEMENU_initialise(GAMEMENU_MENU_TYPE_PAUSE);
            break;

        default:
            ASSERT(0);
            break;
        }
    }

    if (GAMEMENU_menu_type != GAMEMENU_MENU_TYPE_NONE) {
        GAMEMENU_background += millisecs;
        GAMEMENU_slowdown -= millisecs << 6;
        GAMEMENU_fadein_x += millisecs << 7;

        // Delay text fade until the background overlay is partially opaque.
        if (GAMEMENU_background < 400) {
            GAMEMENU_fadein_x = 0;
        }

        SATURATE(GAMEMENU_background, 0, 640);
        SATURATE(GAMEMENU_slowdown, 0, 0x10000);
        SATURATE(GAMEMENU_fadein_x, 0, 800 << 8);

        if (GAMEMENU_background > 200) {
            // Keyboard repeat delay (time-based, same values as controller).
            {
                static uint64_t kb_next_fire = 0;
                static UBYTE kb_last_dir = 0;
                UBYTE kb_dir = (Keys[KB_UP] ? 1 : 0) | (Keys[KB_DOWN] ? 2 : 0);
                uint64_t now = sdl3_get_ticks();

                if (kb_dir) {
                    if (kb_dir != kb_last_dir) {
                        kb_next_fire = now + 400;
                    } else if (now < kb_next_fire) {
                        Keys[KB_UP] = Keys[KB_DOWN] = 0;
                    } else {
                        kb_next_fire = now + 150;
                    }
                }
                kb_last_dir = kb_dir;
            }

            if (Keys[KB_UP]) {
                Keys[KB_UP] = 0;

                GAMEMENU_menu_selection -= 1;

                if (GAMEMENU_menu_selection < 1) {
                    for (i = 1; i <= 7; i++) {
                        if (GAMEMENU_menu[GAMEMENU_menu_type].word[i] != NULL) {
                            GAMEMENU_menu_selection = i;
                        }
                    }
                }

                MFX_play_stereo(0, S_MENU_CLICK_START, MFX_REPLACE);
            }

            if (Keys[KB_DOWN]) {
                Keys[KB_DOWN] = 0;

                GAMEMENU_menu_selection += 1;

                if ((GAMEMENU_menu_selection > 7) || (GAMEMENU_menu[GAMEMENU_menu_type].word[GAMEMENU_menu_selection] == NULL)) {
                    GAMEMENU_menu_selection = 1;
                }

                MFX_play_stereo(0, S_MENU_CLICK_START, MFX_REPLACE);
            }

            SATURATE(GAMEMENU_menu_selection, 1, 7);

            if (GAMEMENU_menu[GAMEMENU_menu_type].word[GAMEMENU_menu_selection] == NULL) {
                GAMEMENU_menu_selection -= 1;

                if (GAMEMENU_menu_selection == 0) {
                    GAMEMENU_menu_selection = 1;
                }
            }

            if (Keys[KB_ENTER] || Keys[KB_SPACE] || Keys[KB_PENTER]) {
                Keys[KB_ENTER] = 0;
                Keys[KB_SPACE] = 0;
                Keys[KB_PENTER] = 0;

                MFX_play_stereo(1, S_MENU_CLICK_END, MFX_REPLACE);

                switch (GAMEMENU_menu[GAMEMENU_menu_type].word[GAMEMENU_menu_selection]) {
                case NULL:
                    return GAMEMENU_DO_NEXT_LEVEL;

                case X_RESUME_LEVEL:
                    GAMEMENU_initialise(GAMEMENU_MENU_TYPE_NONE);
                    break;

                case X_RESTART_LEVEL:
                    return GAMEMENU_DO_RESTART;

                case X_ABANDON_GAME:
                    GAMEMENU_initialise(GAMEMENU_MENU_TYPE_SURE);
                    break;

                case X_OKAY:
                    return GAMEMENU_DO_CHOOSE_NEW_MISSION;

                case X_CANCEL:
                    GAMEMENU_initialise(GAMEMENU_MENU_TYPE_PAUSE);
                    break;

                case X_SAVE_GAME:
                    MEMORY_quick_save();
                    GAMEMENU_initialise(GAMEMENU_MENU_TYPE_NONE);
                    break;

                case X_LOAD_GAME:
                    if (!MEMORY_quick_load()) {
                        return GAMEMENU_DO_RESTART;
                    }
                    break;

                default:
                    ASSERT(0);
                    break;
                }
            }
        }
    }

    return GAMEMENU_DO_NOTHING;
}

// uc_orig: GAMEMENU_is_paused (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_is_paused()
{
    return GAMEMENU_slowdown == 0;
}

// uc_orig: GAMEMENU_slowdown_mul (fallen/Source/gamemenu.cpp)
SLONG GAMEMENU_slowdown_mul()
{
    return GAMEMENU_slowdown >> 8;
}

// uc_orig: GAMEMENU_set_level_lost_reason (fallen/Source/gamemenu.cpp)
void GAMEMENU_set_level_lost_reason(CBYTE* reason)
{
    GAMEMENU_level_lost_reason = reason;
}

// Renders the menu overlay: background dimming, title, selectable items, or scores on win.
// uc_orig: GAMEMENU_draw (fallen/Source/gamemenu.cpp)
void GAMEMENU_draw()
{
    SLONG i;

    if (GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_NONE) {
        return;
    }

    // Darken the scene FBO behind the menu — covers the entire FBO
    // (including inner bars around the framed 4:3 region when the FBO
    // aspect is not 4:3), not just the framed region itself. Invariant I5:
    // full-screen effects cover the whole FBO. push_fullscreen_ui_mode
    // stretches virtual (0..640, 0..480) non-uniformly across the FBO
    // so the darken quad fills it; the framed 4:3 affine is then restored
    // for the menu text draws below.
    {
        PolyPage::push_fullscreen_ui_mode();
        POLY_frame_init(UC_FALSE, UC_FALSE);
        PANEL_darken_screen(GAMEMENU_background);
        POLY_frame_draw(UC_FALSE, UC_FALSE);
        PolyPage::pop_ui_mode();
    }

    // Menu text stays framed (4:3 centred region with bars).
    PolyPage::UIModeScope _ui_scope(ui_coords::UIAnchor::CENTER_CENTER);

    POLY_frame_init(UC_FALSE, UC_FALSE);

    MENUFONT_fadein_line(GAMEMENU_fadein_x);
    MENUFONT_fadein_draw(320, 100, 255, XLAT_str(GAMEMENU_menu[GAMEMENU_menu_type].word[0]));

    bool bDrawMainPartOfMenu = UC_TRUE;

    if (GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_LOST) {
        if (GAMEMENU_level_lost_reason) {
            // Text rendering was disabled in the original (oversized, all-caps, doesn't fit).
            // MENUFONT_fadein_draw(320, 120, 255, GAMEMENU_level_lost_reason);
        }
    } else if (GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_WON) {
        extern void ScoresDraw(void);
        ScoresDraw();
    }

    if (bDrawMainPartOfMenu) {
        for (i = 1; i < 8; i++) {
            if (GAMEMENU_menu[GAMEMENU_menu_type].word[i]) {
                MENUFONT_fadein_draw(
                    320,
                    155 + i * 40,
                    (i == GAMEMENU_menu_selection) ? 255 : 128,
                    XLAT_str(GAMEMENU_menu[GAMEMENU_menu_type].word[i]));
            }
        }
    }

    POLY_frame_draw(UC_FALSE, UC_FALSE);
}
