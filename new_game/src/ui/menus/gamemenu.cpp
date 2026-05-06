// In-game pause/won/lost menus rendered as an overlay during gameplay.
// Menu type constants are private — only DO_* return codes leave this file.

#include "game/game_types.h"
#include "engine/platform/sdl3_bridge.h"

#include "ui/hud/panel.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/polypage.h" // PolyPage::UIModeScope
#include "engine/graphics/ui_coords.h" // UIAnchor
#include "engine/graphics/text/menufont.h"
#include "assets/xlat_str.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "engine/audio/music.h"
#include "missions/memory.h"

#include "ui/menus/gamemenu.h"
#include "ui/menus/gamemenu_globals.h"
#include "engine/input/gamepad.h"
#include "engine/input/input_frame.h"
#include "engine/input/keyboard.h" // keyboard_key_up — synthesize release of menu-consumed keys

// Stick navigation thresholds and auto-repeat live in input_frame
// (STICK_DIR_PRESS_RAW / RELEASE_RAW + INPUT_REPEAT_INITIAL_MS / PERIOD_MS).
// Single source of truth — same cadence across every menu now.

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
        gamepad_consume_until_released(0); // Cross/A
        gamepad_consume_until_released(3); // Triangle/Y
        gamepad_consume_until_released(6); // Start
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

    // Lower clamp at 0 (was 10): on render > 100 FPS the per-frame wall-clock
    // delta drops below 10 ms, so a floor of 10 caused the menu animations
    // (background fade, text fade-in) to advance faster than real time
    // (e.g. 240 FPS × 10 ms = 2400 ms/sec instead of 1000). Sub-ms frames now
    // contribute 0 — next frame catches up. Upper clamp 200 ms guards alt-tab.
    SATURATE(millisecs, 0, 200);

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

    // Gamepad → keyboard event bridge for the existing menu handler below.
    // Edge-detect (just_pressed) and auto-repeat (just_pressed_or_repeat) are
    // managed centrally in input_frame — set in LibShellActive each render
    // frame, identical cadence across menus.
    //
    // Start (button 6) is always active to open the menu from gameplay; the
    // rest fire only while a menu is shown so gameplay buttons (Triangle =
    // KICK, Cross = JUMP) don't leak into menu actions when no menu is up.
    //
    // No prev-state reset on menu open is needed: input_frame's just_pressed
    // requires a rising edge in the snapshot, so a button held across the
    // menu transition won't fire until released and re-pressed.
    {
        // Start → ESC (toggle pause).
        if (input_btn_just_pressed(6))
            input_frame_inject_key_press(KB_ESC);

        if (GAMEMENU_menu_type != GAMEMENU_MENU_TYPE_NONE) {
            // Triangle/Y (button 3) → ESC (back/cancel). Drain press_pending
            // so the press doesn't leak into car siren toggle (which reads
            // input_btn_press_pending(3)) when the menu closes — physics
            // doesn't run during pause, so apply_button_input_car can't drain
            // the pending itself, and a stale flag would fire on the first
            // tick after resume.
            if (input_btn_just_pressed(3)) {
                input_btn_consume(3);
                input_frame_inject_key_press(KB_ESC);
            }

            // Cross/A (button 0) → Enter (confirm). No current consumer of
            // input_btn_press_pending(0), but consume defensively for symmetry
            // with Triangle — any future physics-tick consumer of that flag
            // would otherwise see the menu-confirm press as a pending input.
            if (input_btn_just_pressed(0)) {
                input_btn_consume(0);
                input_frame_inject_key_press(KB_ENTER);
            }

            // Up/Down nav (keyboard and stick) is handled below inside the
            // GAMEMENU_background > 200 gate — single unified auto-repeat
            // for both sources, no double-throttle.
        }
    }

    // Pause-toggle channel: real keyboard ESC presses (event hook sets
    // press_pending) and synthesised gamepad presses (Start / Triangle
    // bridge above calls input_frame_inject_key_press(KB_ESC)) both land
    // on the same input_frame slot. Single read picks up either source.
    if (input_key_press_pending(KB_ESC)) {
        // Force a synthesised release in input_frame's CURRENT snapshot so
        // same-frame downstream consumers (e.g. weapon switch reading
        // input_key_just_pressed(KB_ENTER) in process_controls, or JUMP
        // level read in get_hardware_input) don't see the menu-consumed
        // press leak into gameplay. force_release clears
        // curr/event_held/pressed_during_frame/press_pending all at once
        // — subsumes input_key_consume.
        input_key_force_release(KB_ESC);

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
            // Unified up/down navigation: all sources treated as one logical
            // input. InputAutoRepeat applies a SINGLE auto-repeat throttle on
            // the OR of source held-states — combined doesn't fire faster
            // than any single source, even when several are held at once
            // (independent per-source throttles would interleave to ~2× rate).
            //
            // Three independent sources per direction:
            //  - keyboard up/down
            //  - left-stick virtual up/down (reads pre-D-Pad-override raw axis)
            //  - D-Pad up/down via rgbButtons[11]/[12]
            //
            // D-Pad must be read separately, NOT taken implicitly through the
            // stick mirror: the gamepad layer clamps lX/lY when the D-Pad is
            // held, which would hide a concurrent stick deflection in the
            // opposite direction (breaking antagonist suppression for the
            // stick + D-Pad case). With three independent signals OR'd into
            // the combined held bool, antagonist works for every pair.
            //
            // Static instances persist between calls — InputAutoRepeat tracks
            // its own rising/falling edge of the combined boolean and timer.
            static InputAutoRepeat ar_up;
            static InputAutoRepeat ar_down;

            const bool any_up_jp = input_key_just_pressed(KB_UP)
                || input_stick_just_pressed(INPUT_STICK_LEFT, INPUT_STICK_DIR_UP)
                || input_btn_just_pressed(11);
            const bool any_up_held = input_key_held(KB_UP)
                || input_stick_held(INPUT_STICK_LEFT, INPUT_STICK_DIR_UP)
                || input_btn_held(11);
            const bool any_dn_jp = input_key_just_pressed(KB_DOWN)
                || input_stick_just_pressed(INPUT_STICK_LEFT, INPUT_STICK_DIR_DOWN)
                || input_btn_just_pressed(12);
            const bool any_dn_held = input_key_held(KB_DOWN)
                || input_stick_held(INPUT_STICK_LEFT, INPUT_STICK_DIR_DOWN)
                || input_btn_held(12);

            bool nav_up   = ar_up.tick_combined(any_up_jp, any_up_held);
            bool nav_down = ar_down.tick_combined(any_dn_jp, any_dn_held);

            // Antagonist suppression: both opposite directions held = no clear
            // intent, suppress output. Timers continue ticking normally so that
            // releasing one direction immediately resumes navigation in the
            // other without a fresh 400ms initial delay.
            if (any_up_held && any_dn_held) {
                nav_up = false;
                nav_down = false;
            }

            if (nav_up) {
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

            if (nav_down) {
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

            if (input_key_press_pending(KB_ENTER) || input_key_press_pending(KB_SPACE) || input_key_press_pending(KB_PENTER)) {
                // Force-release in input_frame's CURRENT snapshot — see
                // comment in the ESC handler above. Otherwise SPACE held for
                // confirm leaks INPUT_MASK_JUMP (player jumps), ENTER leaks
                // INPUT_MASK_SELECT (weapon switch popup opens), etc.
                input_key_force_release(KB_ENTER);
                input_key_force_release(KB_SPACE);
                input_key_force_release(KB_PENTER);

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
