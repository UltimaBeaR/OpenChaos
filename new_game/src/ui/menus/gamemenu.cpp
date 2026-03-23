// In-game pause/won/lost menus rendered as an overlay during gameplay.
// Menu type constants are private — only DO_* return codes leave this file.

#include "missions/game_types.h"

#include "ui/hud/panel.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/resources/menufont.h"
#include "assets/xlat_str.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "engine/audio/music.h"
#include "missions/memory.h"

#include "ui/menus/gamemenu.h"
#include "ui/menus/gamemenu_globals.h"

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

    // claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
    static DWORD tick_last = 0;
    static DWORD tick_now = 0;

    tick_now = GetTickCount();

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
        GAMEMENU_background += millisecs >> 0;
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

    // Darken the screen behind the menu.
    POLY_frame_init(UC_FALSE, UC_FALSE);
    PANEL_darken_screen(GAMEMENU_background);
    POLY_frame_draw(UC_FALSE, UC_FALSE);

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
