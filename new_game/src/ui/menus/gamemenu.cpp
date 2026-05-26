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
#include "engine/input/input_frame.h"
#include "engine/input/keyboard.h" // keyboard_key_up — synthesize release of menu-consumed keys
#include "missions/eway.h"         // EWAY_reset_cutscene_voice_tail on pause-resume
#include "game/action_map/act_menu.h" // ACT_MENU_*

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
    const bool entering_menu = GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_NONE && menu != GAMEMENU_MENU_TYPE_NONE;
    const bool leaving_menu  = GAMEMENU_menu_type != GAMEMENU_MENU_TYPE_NONE && menu == GAMEMENU_MENU_TYPE_NONE;

    // Gameplay → menu: drain sticky press_pending only. Held inputs
    // (stick deflection, held buttons, held triggers, mouse motion) are
    // LEFT UNTOUCHED so the gameplay tick during the slowdown ramp
    // continues to read the player's current input and the character
    // animates out of its current motion (run / jump / fall) naturally
    // instead of snapping to a freeze. Without this open-side drain a
    // gameplay press_pending (e.g. Cross for jump, Triangle for siren,
    // F5/F6/F7/END/DEL/PGDN for camera mode) would carry through to the
    // menu and mis-fire Resume / Cancel on the first menu-active frame.
    if (entering_menu) {
        input_drain_all_press_pending();
    }

    // Menu → gameplay: full wait-for-release on every held input plus
    // press_pending drain. Anything held during the menu (R2 trigger,
    // R1 kick, stick deflection, accumulated mouse motion, etc.) is
    // suppressed in gameplay reads until physically released and
    // pressed again — see input_consume_all_held_until_released for
    // the full list.
    if (leaving_menu) {
        input_consume_all_held_until_released();
        // Clear the cutscene voice-tail timestamp. Wall-clock kept advancing
        // through the pause while the EWAY tick that updates it didn't, so
        // the stored timestamp would now read as "voice ended just now" and
        // make the post-voice silence window block another half second on the
        // first tick after resume, even though the line finished mid-pause.
        EWAY_reset_cutscene_voice_tail();
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

    // Pause-toggle channel: combined read of three sources — keyboard ESC
    // (event hook sets press_pending), gamepad Start (always active, opens
    // menu from gameplay), and gamepad Triangle (only while a menu is open
    // — back / cancel). The Triangle source is gated so gameplay-time
    // Triangle presses (menu cancel + car siren) don't trigger a spurious
    // open-pause when no menu is up.
    //
    // Triangle consume drains its press_pending so the press doesn't leak
    // into car siren toggle (which reads input_btn_press_pending(3)) when
    // the menu closes — physics doesn't run during pause, so the car-siren
    // consumer can't drain the pending itself, and a stale flag would fire
    // on the first tick after resume.
    //
    // can_react gates all three sources while the slowdown ramp is in
    // flight (menu open but not yet fully paused). During the ramp the
    // gameplay tick is still running with a scaled TICK_RATIO and the
    // character is finishing its current motion — accepting Start /
    // Triangle / ESC mid-ramp would cancel the menu (or re-toggle it)
    // before the player has had a chance to see what they opened, and
    // would also snap the character out of its in-progress decel. When
    // menu is closed (NONE), can_react is true so Start/ESC always
    // open the menu without delay. Press_pending lingers across the
    // ramp and is dropped by the slowdown→0 transition drain in
    // GAMEMENU_process so the player has to press fresh after the
    // menu becomes reactive.
    const bool menu_open = (GAMEMENU_menu_type != GAMEMENU_MENU_TYPE_NONE);
    const bool can_react = !menu_open || GAMEMENU_is_paused();
    const bool toggle_pause_kb       = can_react && input_key_press_pending(ACT_MENU_TOGGLE_PAUSE_KKEY);
    const bool toggle_pause_gp_start = can_react && input_btn_press_pending(ACT_MENU_TOGGLE_PAUSE_GBTN);
    const bool cancel_in_open_menu   = can_react && menu_open
                                       && input_btn_press_pending(ACT_MENU_CANCEL_GBTN);
    if (toggle_pause_kb || toggle_pause_gp_start || cancel_in_open_menu) {
        // Force-release in input_frame's CURRENT snapshot so same-frame
        // downstream consumers (e.g. weapon switch reading
        // input_key_just_pressed(KKEY_ENTER) in process_controls, or JUMP
        // level read in get_hardware_input) don't see the menu-consumed
        // press leak into gameplay. force_release clears
        // curr/event_held/pressed_during_frame/press_pending all at once.
        if (toggle_pause_kb)       input_key_force_release(ACT_MENU_TOGGLE_PAUSE_KKEY);
        if (toggle_pause_gp_start) input_btn_consume(ACT_MENU_TOGGLE_PAUSE_GBTN);
        if (cancel_in_open_menu)   input_btn_consume(ACT_MENU_CANCEL_GBTN);

        switch (GAMEMENU_menu_type) {
        case GAMEMENU_MENU_TYPE_NONE:
            // Don't stop sounds when entering pause: in pre-release MuckyFoot
            // called MFX_stop(MFX_CHANNEL_ALL, MFX_WAVE_ALL) here with their
            // own "Which one of these should I use?" comment, but in retail PC
            // they dropped it — pausing during a cutscene must not cut the
            // voice line off. Ambient gameplay sounds keep humming through
            // the pause as a side effect; matches retail behaviour.
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

        // Detect the slowdown ramp finishing (slowdown crossing to 0 from
        // a non-zero value) — that's the frame the menu transitions from
        // "fading in while gameplay decelerates" to "fully active and
        // reading input". Drain press_pending here so any press the
        // player made DURING the ramp (Cross for jump, Triangle for
        // siren, etc.) doesn't surface to the menu's first reactive
        // frame and mis-fire Resume / Cancel. The open-side drain in
        // GAMEMENU_initialise covers pre-pause presses; this covers
        // ramp-time presses that landed after the open-side drain.
        {
            static SLONG s_prev_slowdown = -1;
            if (s_prev_slowdown != 0 && GAMEMENU_slowdown == 0) {
                input_drain_all_press_pending();
            }
            s_prev_slowdown = GAMEMENU_slowdown;
        }

        // Menu input is gated by GAMEMENU_is_paused() — i.e. wait until the
        // gameplay slowdown ramp has finished (slowdown decays from 0x10000
        // to 0 over ~1 s after the menu opens). During the ramp the
        // physics tick keeps running with a scaled TICK_RATIO so the
        // character eases out of running / jumping / falling animations
        // naturally. If the menu accepted nav / confirm while the ramp
        // was still in flight, the player would hit Resume mid-deceleration
        // and snap the character to whatever pose the animation was on,
        // making the pause-resume feel jerky.
        //
        // is_paused is strictly stronger than the previous "background > 200"
        // gate (~200 ms after open vs ~1 s) — once is_paused becomes true,
        // GAMEMENU_background has long since saturated to its cap.
        if (GAMEMENU_is_paused()) {
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

            const bool any_up_jp = input_key_just_pressed(ACT_MENU_NAV_UP_KKEY)
                || input_stick_just_pressed(GAXIS_LEFT, GDIR_UP)
                || input_btn_just_pressed(ACT_MENU_NAV_UP_GBTN);
            const bool any_up_held = input_key_held(ACT_MENU_NAV_UP_KKEY)
                || input_stick_held(GAXIS_LEFT, GDIR_UP)
                || input_btn_held(ACT_MENU_NAV_UP_GBTN);
            const bool any_dn_jp = input_key_just_pressed(ACT_MENU_NAV_DOWN_KKEY)
                || input_stick_just_pressed(GAXIS_LEFT, GDIR_DOWN)
                || input_btn_just_pressed(ACT_MENU_NAV_DOWN_GBTN);
            const bool any_dn_held = input_key_held(ACT_MENU_NAV_DOWN_KKEY)
                || input_stick_held(GAXIS_LEFT, GDIR_DOWN)
                || input_btn_held(ACT_MENU_NAV_DOWN_GBTN);

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

            const bool confirm_kb1 = input_key_press_pending(ACT_MENU_CONFIRM_KKEY_1);
            const bool confirm_kb2 = input_key_press_pending(ACT_MENU_CONFIRM_KKEY_2);
            const bool confirm_kb3 = input_key_press_pending(ACT_MENU_CONFIRM_KKEY_3);
            const bool confirm_gp  = input_btn_press_pending(ACT_MENU_CONFIRM_GBTN);
            if (confirm_kb1 || confirm_kb2 || confirm_kb3 || confirm_gp) {
                // Force-release in input_frame's CURRENT snapshot — see
                // comment in the ESC handler above. Otherwise SPACE held for
                // confirm leaks INPUT_MASK_JUMP (player jumps), ENTER leaks
                // INPUT_MASK_SELECT (weapon switch popup opens), etc.
                if (confirm_kb1) input_key_force_release(ACT_MENU_CONFIRM_KKEY_1);
                if (confirm_kb2) input_key_force_release(ACT_MENU_CONFIRM_KKEY_2);
                if (confirm_kb3) input_key_force_release(ACT_MENU_CONFIRM_KKEY_3);
                if (confirm_gp)  input_btn_consume(ACT_MENU_CONFIRM_GBTN);

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
