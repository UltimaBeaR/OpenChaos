// In-game pause/won/lost menus rendered as an overlay during gameplay.
// Menu type constants are private — only DO_* return codes leave this file.

#include "game/game_types.h"
#include "engine/platform/sdl3_bridge.h"

#include "ui/hud/panel.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/poly_globals.h" // POLY_screen_clip_* — widen 2D clip for full-window help
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
#include "ui/menus/help_content.h"
#include "debug_config.h" // OC_DEBUG_INPUT_PROMPT_CATALOG — gate the dev INPUT TEST topic
#include "ui/input_glyphs/input_glyphs.h" // input_glyph_text_draw_scrolled — rich-text help body
#include "engine/input/gamepad_globals.h" // active_input_device — reset test-page scroll on device change
#include "engine/input/input_frame.h"
#include "engine/input/keyboard.h" // keyboard_key_up — synthesize release of menu-consumed keys
#include "missions/eway.h"         // EWAY_reset_cutscene_voice_tail on pause-resume
#include "game/action_map/act_menu.h" // ACT_MENU_*

// Stick navigation thresholds and auto-repeat live in input_frame
// (s_menu_dir_press_raw / release — from config gamepad.menu_stick_deadzone —
// + INPUT_REPEAT_INITIAL_MS / PERIOD_MS). Single source of truth — same cadence
// and deadzone across every menu now.

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
// Controls/how-to-play help: list of mechanics topics (OpenChaos addition).
#define GAMEMENU_MENU_TYPE_HELP_LIST 5
// Controls/how-to-play help: full-screen detail text for one topic (OpenChaos addition).
#define GAMEMENU_MENU_TYPE_HELP_DETAIL 6
// uc_orig: GAMEMENU_MENU_TYPE_NUMBER (fallen/Source/gamemenu.cpp)
#define GAMEMENU_MENU_TYPE_NUMBER 7

// Index (0-based) into HELP_TOPICS of the topic shown on the HELP_DETAIL screen.
// Set when confirming a topic in HELP_LIST. Persists while the detail screen is up.
static SLONG s_help_topic = 0;

// HELP_LIST highlighted topic index (0-based). The generic word[]-based nav
// can't drive the help list (HELP_LIST has no word[] table), so the list
// keeps its own selection bounded to [0, HELP_TOPIC_COUNT-1].
static SLONG s_help_list_selection = 0;

// Number of topics shown in the How-to-Play list. The input-prompt catalog
// (input_test) topic is dev-only — hidden from the list unless the debug flag is
// on (players never see it; the page stays in the build). Debug topics MUST be at
// the END of HELP_TOPICS so list indices still map 1:1 to topic indices.
static SLONG GAMEMENU_help_list_count()
{
    SLONG n = HELP_TOPIC_COUNT;
    if (!OC_DEBUG_INPUT_PROMPT_CATALOG) {
        for (SLONG i = 0; i < HELP_TOPIC_COUNT; ++i) {
            if (HELP_TOPICS[i].input_test) {
                --n;
            }
        }
    }
    return n;
}

// HELP_DETAIL scroll position, as the index of the first wrapped line shown in
// the window. Up/down nav pages it; the draw clamps it to the measured line
// count (so it can't run past the ends) and the rich-text drawer writes the
// clamped value back. Reset to 0 whenever a topic is opened.
static SLONG s_help_detail_scroll_line = 0;

// Window metrics measured during the last HELP_DETAIL draw: how many whole lines
// fit the window (fit) and the total wrapped line count (total). GAMEMENU_process
// reads them to size the page step and clamp scrolling (it runs before the draw,
// so it uses the previous frame's values — a one-frame lag that is invisible).
static SLONG s_help_detail_fit = 0;
static SLONG s_help_detail_total = 0;

// Lines moved per up/down nav step. Small fixed step (not a full page) so the
// auto-repeat scrolls gently while held — easier to follow than big jumps.
// Defined here (not with the other help layout constants below) because
// GAMEMENU_process uses it, ahead of that block.
#define GAMEMENU_HELP_DETAIL_SCROLL_LINES 1

// Auto-repeat cadence for holding the scroll key — its own timing, faster than
// the shared menu nav repeat, so a held key scrolls briskly without speeding up
// every other menu. INITIAL = delay before the first repeat (so a single tap
// moves one line); PERIOD = gap between repeats while held (ms).
#define GAMEMENU_HELP_DETAIL_SCROLL_INITIAL_MS 350
#define GAMEMENU_HELP_DETAIL_SCROLL_PERIOD_MS  55

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
    // into car siren toggle (which reads input_btn_press_pending(ACT_CAR_SIREN_GBTN)) when
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
    // Suppress the keyboard ESC pause-toggle while F1 debug modifier is held
    // (gameplay-input gate). Gamepad Start is left as-is — F1 is a keyboard
    // concept; gamepad continues to behave normally.
    const bool toggle_pause_kb       = can_react && input_gameplay_enabled()
                                       && input_key_press_pending(ACT_MENU_TOGGLE_PAUSE_KKEY);
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

        case GAMEMENU_MENU_TYPE_HELP_LIST:
            // Back from the topic list to the pause menu.
            GAMEMENU_initialise(GAMEMENU_MENU_TYPE_PAUSE);
            GAMEMENU_menu_selection = 1;
            break;

        case GAMEMENU_MENU_TYPE_HELP_DETAIL:
            // Back from a topic's detail screen to the topic list.
            GAMEMENU_initialise(GAMEMENU_MENU_TYPE_HELP_LIST);
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
                || input_key_just_pressed(ACT_MENU_NAV_UP_ALT_KKEY)
                || input_stick_just_pressed(ACT_MENU_NAV_GAXIS, ACT_MENU_NAV_UP_GDIR)
                || input_btn_just_pressed(ACT_MENU_NAV_UP_GBTN);
            const bool any_up_held = input_key_held(ACT_MENU_NAV_UP_KKEY)
                || input_key_held(ACT_MENU_NAV_UP_ALT_KKEY)
                || input_stick_held(ACT_MENU_NAV_GAXIS, ACT_MENU_NAV_UP_GDIR)
                || input_btn_held(ACT_MENU_NAV_UP_GBTN);
            const bool any_dn_jp = input_key_just_pressed(ACT_MENU_NAV_DOWN_KKEY)
                || input_key_just_pressed(ACT_MENU_NAV_DOWN_ALT_KKEY)
                || input_stick_just_pressed(ACT_MENU_NAV_GAXIS, ACT_MENU_NAV_DOWN_GDIR)
                || input_btn_just_pressed(ACT_MENU_NAV_DOWN_GBTN);
            const bool any_dn_held = input_key_held(ACT_MENU_NAV_DOWN_KKEY)
                || input_key_held(ACT_MENU_NAV_DOWN_ALT_KKEY)
                || input_stick_held(ACT_MENU_NAV_GAXIS, ACT_MENU_NAV_DOWN_GDIR)
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

            // Confirm edge (shared by every menu type).
            const bool confirm_kb1 = input_key_press_pending(ACT_MENU_CONFIRM_1_KKEY);
            const bool confirm_kb2 = input_key_press_pending(ACT_MENU_CONFIRM_2_KKEY);
            const bool confirm_kb3 = input_key_press_pending(ACT_MENU_CONFIRM_3_KKEY);
            const bool confirm_gp  = input_btn_press_pending(ACT_MENU_CONFIRM_GBTN);
            const bool confirm = confirm_kb1 || confirm_kb2 || confirm_kb3 || confirm_gp;
            // Drain the confirm press the same way the generic path does, so a
            // held SPACE/ENTER/gamepad button doesn't leak into gameplay or
            // re-fire on the next screen.
            auto consume_confirm = [&]() {
                if (confirm_kb1) input_key_force_release(ACT_MENU_CONFIRM_1_KKEY);
                if (confirm_kb2) input_key_force_release(ACT_MENU_CONFIRM_2_KKEY);
                if (confirm_kb3) input_key_force_release(ACT_MENU_CONFIRM_3_KKEY);
                if (confirm_gp)  input_btn_consume(ACT_MENU_CONFIRM_GBTN);
            };

            if (GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_HELP_LIST) {
                // Topic list: own 0-based selection bounded to the VISIBLE topic
                // count (dev-only topics hidden unless the debug flag is on),
                // wrapping top/bottom like the generic menu. Cancel/back is
                // handled by the pause-toggle channel above.
                const SLONG help_count = GAMEMENU_help_list_count();
                if (help_count > 0) {
                    if (nav_up) {
                        s_help_list_selection -= 1;
                        if (s_help_list_selection < 0) {
                            s_help_list_selection = help_count - 1;
                        }
                        MFX_play_stereo(0, S_MENU_CLICK_START, MFX_REPLACE);
                    }
                    if (nav_down) {
                        s_help_list_selection += 1;
                        if (s_help_list_selection > help_count - 1) {
                            s_help_list_selection = 0;
                        }
                        MFX_play_stereo(0, S_MENU_CLICK_START, MFX_REPLACE);
                    }
                    SATURATE(s_help_list_selection, 0, help_count - 1);

                    if (confirm) {
                        consume_confirm();
                        MFX_play_stereo(1, S_MENU_CLICK_END, MFX_REPLACE);
                        s_help_topic = s_help_list_selection;
                        s_help_detail_scroll_line = 0; // start each topic at the top
                        GAMEMENU_initialise(GAMEMENU_MENU_TYPE_HELP_DETAIL);
                    }
                }
            } else if (GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_HELP_DETAIL) {
                // Detail screen: up/down scroll the body by a small fixed step.
                // Uses its OWN auto-repeat (faster cadence than the shared menu
                // nav above) off the same source bools, so a held key scrolls
                // briskly without affecting other menus. total/fit come from the
                // last draw; the draw also clamps and writes the scroll position
                // back, so a one-frame lag here is harmless. Confirm goes back,
                // like cancel.
                static InputAutoRepeat ar_scroll_up;
                static InputAutoRepeat ar_scroll_down;
                bool scroll_up = ar_scroll_up.tick_combined(
                    any_up_jp, any_up_held,
                    GAMEMENU_HELP_DETAIL_SCROLL_INITIAL_MS, GAMEMENU_HELP_DETAIL_SCROLL_PERIOD_MS);
                bool scroll_down = ar_scroll_down.tick_combined(
                    any_dn_jp, any_dn_held,
                    GAMEMENU_HELP_DETAIL_SCROLL_INITIAL_MS, GAMEMENU_HELP_DETAIL_SCROLL_PERIOD_MS);
                // Antagonist suppression: both directions held = no clear intent.
                if (any_up_held && any_dn_held) {
                    scroll_up = false;
                    scroll_down = false;
                }

                if (scroll_up) {
                    s_help_detail_scroll_line -= GAMEMENU_HELP_DETAIL_SCROLL_LINES;
                }
                if (scroll_down) {
                    s_help_detail_scroll_line += GAMEMENU_HELP_DETAIL_SCROLL_LINES;
                }
                if (s_help_detail_scroll_line < 0) {
                    s_help_detail_scroll_line = 0;
                }
                SLONG max_first = s_help_detail_total - s_help_detail_fit;
                if (max_first < 0) {
                    max_first = 0;
                }
                if (s_help_detail_scroll_line > max_first) {
                    s_help_detail_scroll_line = max_first;
                }
                if (confirm) {
                    consume_confirm();
                    MFX_play_stereo(1, S_MENU_CLICK_END, MFX_REPLACE);
                    GAMEMENU_initialise(GAMEMENU_MENU_TYPE_HELP_LIST);
                }
            } else {

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

            if (confirm) {
                // Force-release in input_frame's CURRENT snapshot — see
                // comment in the ESC handler above. Otherwise SPACE held for
                // confirm leaks INPUT_MASK_JUMP (player jumps), ENTER leaks
                // INPUT_MASK_SELECT (weapon switch popup opens), etc.
                consume_confirm();

                MFX_play_stereo(1, S_MENU_CLICK_END, MFX_REPLACE);

                switch (GAMEMENU_menu[GAMEMENU_menu_type].word[GAMEMENU_menu_selection]) {
                case NULL:
                    return GAMEMENU_DO_NEXT_LEVEL;

                case X_RESUME_LEVEL:
                    GAMEMENU_initialise(GAMEMENU_MENU_TYPE_NONE);
                    break;

                case X_RESTART_LEVEL:
                    return GAMEMENU_DO_RESTART;

                case X_CONTROLS:
                    // Open the controls/how-to-play topic list. Start at the
                    // first topic (menu→menu transitions don't reset state).
                    GAMEMENU_initialise(GAMEMENU_MENU_TYPE_HELP_LIST);
                    s_help_list_selection = 0;
                    break;

                case X_ABANDON_GAME:
                    GAMEMENU_initialise(GAMEMENU_MENU_TYPE_SURE);
                    // Default the confirmation to "Okay" (first item) instead of
                    // the selection carried over from the Abandon entry, which
                    // clamps onto "Cancel". Matches the main-menu quit prompt,
                    // which also defaults to Okay. (Menu→menu transitions don't
                    // reset the selection, so set it explicitly here.)
                    GAMEMENU_menu_selection = 1;
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
            } // end generic (non-HELP) nav/confirm
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

// --- Controls/how-to-play help screens (OpenChaos addition) ---
//
// The topic LIST draws inside the framed CENTER_CENTER UI scope (virtual 640x480,
// like the generic menu). The DETAIL screen instead draws in a uniform
// full-window scope so it can use the whole window without distorting the text —
// its layout lives in a larger virtual extent (see GAMEMENU_draw_help_detail).
// Neither pushes its own POLY frame; GAMEMENU_draw wraps them.

// Vertical layout shared with the generic menu: title near top, list items
// spaced ~40px apart, mirroring "155 + i*40".
#define GAMEMENU_HELP_TITLE_Y     100
#define GAMEMENU_HELP_LIST_FIRST_Y 195 // == 155 + 1*40, matches generic first item
#define GAMEMENU_HELP_LIST_SPACING 40
#define GAMEMENU_HELP_ALPHA_SELECTED   255
#define GAMEMENU_HELP_ALPHA_UNSELECTED 128

// Literal label for the pause "Controls" item / help-list heading (no fitting
// localized string; 1.0 ships English).
#define GAMEMENU_HELP_MENU_LABEL "Controls"

// Detail-screen layout. The screen draws in a UNIFORM full-window UI scope (no
// 4:3 frame, no distortion), so its coordinates live in a virtual space whose
// visible extent is the WHOLE window: width VW = g_screen_w_px / g_frame_scale,
// height VH = g_screen_h_px / g_frame_scale (computed in the draw). The body
// gets a fixed left/right padding in VIRTUAL (pre-scale) px: a virtual unit is
// 1/g_frame_scale real px, so the same constant becomes a LARGER physical margin
// on a bigger display and a smaller one on a small window — which is what reads
// well. The wrap width is whatever is left between the two paddings.
#define GAMEMENU_HELP_DETAIL_PAD_PX       20     // virtual px (pre-scale), each side
#define GAMEMENU_HELP_DETAIL_TEXT_SCALE   192    // 256 == 1.0, so ~0.75x (smaller body)
#define GAMEMENU_HELP_DETAIL_TEXT_COLOUR  0xffffff

// Vertical layout, in virtual units measured from the window edges (same scale
// as the framed menus, so these stay constant physical insets at any size). The
// title sits near the very top; the scroll window runs from VIEW_TOP down to a
// band reserved for the ▼ arrow. Only whole lines that fit are drawn; the rest
// scroll. VIEW_BOTTOM_INSET is the gap from the window bottom to the ▼ arrow tip
// (small edge margin); the text window ends ARROW_GAP above the arrow base.
#define GAMEMENU_HELP_DETAIL_TITLE_Y           11  // title near the top
// How far LEFT of the title the reveal wipe starts (virtual px). The wipe then
// only has to travel this short lead-in before the title begins animating, so
// there's no long "nothing yet" pause while it crawls from x=0. Smaller = title
// appears sooner; larger = a longer lead-in before it starts.
#define GAMEMENU_HELP_DETAIL_TITLE_WIPE_LEAD   48
#define GAMEMENU_HELP_DETAIL_VIEW_TOP          78  // first body line (leaves a band
                                                   // for the title + the ▲ arrow;
                                                   // title→▲ gap ≈ ▼→edge gap)
#define GAMEMENU_HELP_DETAIL_VIEW_BOTTOM_INSET 14  // ▼ arrow tip to window bottom

// (GAMEMENU_HELP_DETAIL_SCROLL_LINES is defined near the scroll state up top,
// since GAMEMENU_process uses it ahead of this block.)

// Scroll arrows (▲ above the window, ▼ below it), centred horizontally at VW/2.
// Drawn as solid triangles on the menu's colour page (so they sit on top like
// the text). Sizes in virtual units; ARROW_GAP is the gap between the text and
// the arrow (kept small so the arrow hugs the text, not the window edge).
#define GAMEMENU_HELP_DETAIL_ARROW_HALF_W 14
#define GAMEMENU_HELP_DETAIL_ARROW_HEIGHT 14
#define GAMEMENU_HELP_DETAIL_ARROW_GAP    8
#define GAMEMENU_HELP_DETAIL_ARROW_COLOUR 0xD0E0F0 // cool light grey (RGB; alpha added)

// Extra full-screen dim for the detail text screen, on TOP of the menu's base
// darken (PANEL_darken_screen) but drawn in the same first pass — so it sits
// BEHIND the text and fades in with it (no hard-edged box, no abrupt pop). Black
// ARGB; tune the alpha for more/less dimming.
#define GAMEMENU_HELP_DETAIL_DIM_COLOUR 0xC0000000u // ~75% black, stacks with base

// The backdrop dim fades in over the reveal. Multiplier on the shared reveal
// progress: the dim reaches full at reveal_t = 1/SPEEDUP, so its fade duration is
// inversely proportional to this — 1.0 stretches across the whole reveal,
// higher = finishes (much) sooner.
#define GAMEMENU_HELP_DIM_FADE_SPEEDUP 7.0f

// Body/arrow fade shape: faint for a while, then a sharp ease-in burst up to full
// reached at BODY_FADE_COMPLETE of the reveal, and held at full afterwards.
//   p = reveal progress 0..1 (after the optional BODY_FADE_DELAY hold)
//   p reaches 1 at COMPLETE (then clamps), body_t = p^EASE
// Lower COMPLETE = the burst lands earlier in the reveal (0.5 = midpoint), then
// holds. EASE sets how sharp the burst is (2 soft, 3, 4+ sharper). BODY_FADE_DELAY
// = 0 (no start pause — the title's wipe starts at once).
#define GAMEMENU_HELP_DETAIL_BODY_FADE_DELAY    0.0f
#define GAMEMENU_HELP_DETAIL_BODY_FADE_COMPLETE 0.35f
#define GAMEMENU_HELP_DETAIL_BODY_FADE_EASE     6

// Reveal progress (0..1) of the detail screen. GAMEMENU_fadein_x ramps from 0 to
// (800<<8) once the overlay is partly opaque (see GAMEMENU_process); normalising
// it gives a layout-independent fade that drives the body text, the backdrop dim
// and the arrows together. (Layout-independent — the body now spans a width that
// varies with the window, so tying the fade to an x-range would misbehave.)
#define GAMEMENU_HELP_DETAIL_FADE_RANGE (800 << 8) // matches GAMEMENU_fadein_x cap
static float GAMEMENU_help_detail_reveal_t()
{
    float t = (float)GAMEMENU_fadein_x / (float)GAMEMENU_HELP_DETAIL_FADE_RANGE;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    return t;
}

// Draw a solid 2D triangle in the current UI scope, ordered on top of the scene
// like the menu text (same depth bodge as PANEL_draw_quad), on the colour-alpha
// page. colour is ARGB. Used for the scroll arrows.
static void GAMEMENU_draw_help_tri(float ax, float ay, float bx, float by,
                                   float cx, float cy, ULONG colour)
{
    const float w = PANEL_GetNextDepthBodge();
    const float z = 1.0f - w;

    POLY_Point pp[3];
    POLY_Point* tri[3] = { &pp[0], &pp[1], &pp[2] };
    for (int i = 0; i < 3; ++i) {
        pp[i].z = z;
        pp[i].Z = w;
        pp[i].u = 0.0f;
        pp[i].v = 0.0f;
        pp[i].colour = colour;
        pp[i].specular = 0xff000000;
    }
    pp[0].X = ax;
    pp[0].Y = ay;
    pp[1].X = bx;
    pp[1].Y = by;
    pp[2].X = cx;
    pp[2].Y = cy;

    POLY_add_triangle(tri, POLY_PAGE_COLOUR_ALPHA, UC_FALSE, UC_TRUE);
}

// Help topic list: heading + vertical list of topic titles, selected one bright.
static void GAMEMENU_draw_help_list()
{
    MENUFONT_fadein_draw(320, GAMEMENU_HELP_TITLE_Y, GAMEMENU_HELP_ALPHA_SELECTED,
                         (CBYTE*)GAMEMENU_HELP_MENU_LABEL);

    // Only the visible topics (dev-only INPUT TEST hidden unless the debug flag).
    const SLONG help_count = GAMEMENU_help_list_count();
    for (SLONG i = 0; i < help_count; i++) {
        const UBYTE alpha = (i == s_help_list_selection)
                                ? GAMEMENU_HELP_ALPHA_SELECTED
                                : GAMEMENU_HELP_ALPHA_UNSELECTED;
        MENUFONT_fadein_draw(320,
                             GAMEMENU_HELP_LIST_FIRST_Y + i * GAMEMENU_HELP_LIST_SPACING,
                             alpha,
                             (CBYTE*)HELP_TOPICS[i].title);
    }
}

// Help topic detail: topic title heading + scrollable rich-text body (with
// inline button glyphs). Drawn in a uniform full-window scope (the caller pushes
// it), so coordinates span the whole window without distortion. Long bodies
// scroll within a window between the title and the bottom; ▲/▼ arrows show when
// there is more above/below.
static void GAMEMENU_draw_help_detail()
{
    if (s_help_topic < 0 || s_help_topic >= HELP_TOPIC_COUNT) {
        return; // defensive — index is always set from a valid list selection
    }

    // The detail content is the ACTIVE device's (per-device body, or the catalog's
    // device-specific list), so reset the scroll to the top whenever the active
    // device changes while this screen is open — the content changes under you.
    {
        static SLONG s_last_device = -1;
        if ((SLONG)active_input_device != s_last_device) {
            s_last_device = (SLONG)active_input_device;
            s_help_detail_scroll_line = 0;
        }
    }

    // Visible virtual extent of the whole window in the uniform scope (see
    // PolyPage::push_uniform_fullscreen_ui_mode). Fall back to 640x480 if the
    // frame scale isn't ready.
    float scale = ui_coords::g_frame_scale;
    if (scale <= 0.0f) scale = 1.0f;
    const float vw = ui_coords::g_screen_w_px / scale;
    const float vh = ui_coords::g_screen_h_px / scale;
    const float cx = vw * 0.5f;

    // Widen the POLY 2D clip window to the full window's virtual extent for the
    // duration of this draw. POLY_setclip culls 2D quads whose virtual Y exceeds
    // POLY_screen_clip_bottom, which is the 640x480 virtual screen height (480) —
    // fine for the framed menus, but this screen lays out across the whole window
    // (virtual height up to g_screen_h_px/scale), so rows below virtual y=480
    // would be culled (only the top ~480 worth of text would draw). The clip
    // flags are baked at POLY_add_quad time (i.e. during the draws below), so set
    // the window here and restore it before returning. The hardware scissor
    // (full FBO, from the uniform scope) still bounds actual pixels.
    const float saved_clip_left = POLY_screen_clip_left;
    const float saved_clip_right = POLY_screen_clip_right;
    const float saved_clip_top = POLY_screen_clip_top;
    const float saved_clip_bottom = POLY_screen_clip_bottom;
    POLY_screen_clip_left = 0.0f;
    POLY_screen_clip_top = 0.0f;
    POLY_screen_clip_right = vw;
    POLY_screen_clip_bottom = vh;

    // Start the title's left→right reveal wipe just LEFT of the title itself,
    // instead of at virtual x=0. The MENUFONT wipe travels rightward across the
    // virtual screen and only reveals a glyph once it reaches that glyph's x; with
    // the title centred on a wide window (x ≈ vw/2) the wipe had to crawl across
    // the whole empty left margin first, a long "nothing visible yet" dead time.
    // Offsetting the wipe origin to the title's left edge (minus a short lead-in)
    // makes the title start animating almost immediately. TITLE_Y line below uses
    // the same centring (sum of raw glyph widths) as MENUFONT_fadein_draw.
    {
        SLONG title_w = 0;
        for (const char* c = HELP_TOPICS[s_help_topic].title; *c; ++c) {
            title_w += MENUFONT_CharWidth((CBYTE)*c, 256); // 256 == 1.0 (raw width)
        }
        const float wipe_start = cx - (float)title_w * 0.5f
                                 - (float)GAMEMENU_HELP_DETAIL_TITLE_WIPE_LEAD;
        MENUFONT_fadein_line(GAMEMENU_fadein_x + (SLONG)(wipe_start * 256.0f));
    }

    // (The dark backdrop is a full-screen dim drawn in GAMEMENU_draw's first
    // pass, behind this text — see GAMEMENU_HELP_DETAIL_DIM_COLOUR.)
    MENUFONT_fadein_draw((SLONG)cx, GAMEMENU_HELP_DETAIL_TITLE_Y,
                         GAMEMENU_HELP_ALPHA_SELECTED,
                         (CBYTE*)HELP_TOPICS[s_help_topic].title);

    // Fade the body (and the scroll arrows) in with a sharp ease-IN BURST rather
    // than linearly: a linear/early text fade reaches full opacity before the
    // title's own animated reveal, so the body visibly "beats" the title. Here the
    // body stays faint, then bursts up to full at BODY_FADE_COMPLETE of the reveal
    // and holds. p = reveal progress (after the optional BODY_FADE_DELAY hold),
    // remapped so it hits 1 at COMPLETE (then clamps), raised to BODY_FADE_EASE.
    // The backdrop dim keeps its own (fast) ramp via reveal_t.
    const float reveal_t = GAMEMENU_help_detail_reveal_t();
    float p = (reveal_t - GAMEMENU_HELP_DETAIL_BODY_FADE_DELAY)
              / (1.0f - GAMEMENU_HELP_DETAIL_BODY_FADE_DELAY);
    if (p < 0.0f) {
        p = 0.0f;
    }
    // Reach full at COMPLETE of the reveal, then hold at full (clamp): the burst
    // lands at the midpoint and brightness stops increasing afterwards.
    p /= GAMEMENU_HELP_DETAIL_BODY_FADE_COMPLETE;
    if (p > 1.0f) {
        p = 1.0f;
    }
    // Ease-in: body_t = p ^ EASE — faint, then a sharp burst up to full.
    float body_t = p;
    for (int e = 1; e < GAMEMENU_HELP_DETAIL_BODY_FADE_EASE; ++e) {
        body_t *= p;
    }
    const SWORD body_fade = (SWORD)((1.0f - body_t) * 255.0f);

    const float body_x = (float)GAMEMENU_HELP_DETAIL_PAD_PX;
    const float wrap_w = vw - 2.0f * (float)GAMEMENU_HELP_DETAIL_PAD_PX;
    const float view_top = (float)GAMEMENU_HELP_DETAIL_VIEW_TOP;

    // Reserve the bottom band for the ▼ arrow so it never clips the window edge
    // and sits a fixed small gap below the text. The arrow's apex (tip) is a
    // small edge margin above the window bottom; its base is one arrow-height up;
    // the text window ends ARROW_GAP above that base. So: text → gap → ▼ → margin
    // → window bottom, all inside the window regardless of how many whole lines
    // fit. (down_tip_y / down_base_y are reused by the arrow draw below.)
    const float down_tip_y = vh - (float)GAMEMENU_HELP_DETAIL_VIEW_BOTTOM_INSET;
    const float down_base_y = down_tip_y - (float)GAMEMENU_HELP_DETAIL_ARROW_HEIGHT;
    const float view_bottom = down_base_y - (float)GAMEMENU_HELP_DETAIL_ARROW_GAP;
    const float view_h = view_bottom - view_top;

    // Draw the scroll window. Either the rich-text body, or — for the temporary
    // INPUT TEST topic — the auto-generated input-prompt catalog for the active
    // device. Both clamp s_help_detail_scroll_line in place and report how many
    // lines fit + the total line count for the arrow / paging logic.
    SLONG fit = 0;
    SLONG total = 0;
    if (HELP_TOPICS[s_help_topic].input_test) {
        total = input_prompt_catalog_draw_scrolled(
            body_x,
            view_top,
            GAMEMENU_HELP_DETAIL_TEXT_SCALE,
            GAMEMENU_HELP_DETAIL_TEXT_COLOUR,
            body_fade,
            &s_help_detail_scroll_line,
            view_h,
            &fit);
    } else {
        // Pick the body for the active device (each topic is written per device).
        const HelpTopic& t = HELP_TOPICS[s_help_topic];
        const char* body = (active_input_device == INPUT_DEVICE_XBOX)       ? t.body_xbox
                           : (active_input_device == INPUT_DEVICE_DUALSENSE) ? t.body_ps
                                                                             : t.body_kbm;
        total = input_glyph_text_draw_scrolled(
            body,
            body_x,
            view_top,
            wrap_w,
            GAMEMENU_HELP_DETAIL_TEXT_SCALE,
            GAMEMENU_HELP_DETAIL_TEXT_COLOUR,
            body_fade,
            &s_help_detail_scroll_line,
            view_h,
            &fit);
    }

    // Share the measured metrics with GAMEMENU_process (paging step + clamping).
    s_help_detail_fit = fit;
    s_help_detail_total = total;

    // Scroll arrows, faded in with the body via the alpha byte.
    const bool more_up = s_help_detail_scroll_line > 0;
    const bool more_down = (s_help_detail_scroll_line + fit) < total;
    if (more_up || more_down) {
        SLONG a = (SLONG)(255.0f * body_t); // fade with the body, not the dim
        if (a < 0) a = 0;
        else if (a > 255) a = 255;
        const ULONG arrow_col = ((ULONG)a << 24) | (GAMEMENU_HELP_DETAIL_ARROW_COLOUR & 0x00FFFFFFu);

        if (more_up) {
            // Points up: apex above the window, base below it.
            const float base_y = view_top - GAMEMENU_HELP_DETAIL_ARROW_GAP;
            const float tip_y = base_y - GAMEMENU_HELP_DETAIL_ARROW_HEIGHT;
            GAMEMENU_draw_help_tri(cx, tip_y,
                                   cx + GAMEMENU_HELP_DETAIL_ARROW_HALF_W, base_y,
                                   cx - GAMEMENU_HELP_DETAIL_ARROW_HALF_W, base_y,
                                   arrow_col);
        }
        if (more_down) {
            // Points down: apex (tip) near the bottom edge, base above it. Both
            // precomputed with the view geometry so the text sits just above.
            GAMEMENU_draw_help_tri(cx, down_tip_y,
                                   cx + GAMEMENU_HELP_DETAIL_ARROW_HALF_W, down_base_y,
                                   cx - GAMEMENU_HELP_DETAIL_ARROW_HALF_W, down_base_y,
                                   arrow_col);
        }
    }

    // Restore the 2D clip window (see the widen at the top of this function).
    POLY_screen_clip_left = saved_clip_left;
    POLY_screen_clip_right = saved_clip_right;
    POLY_screen_clip_top = saved_clip_top;
    POLY_screen_clip_bottom = saved_clip_bottom;
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
        // Detail screen: extra full-screen dim so the body text reads clearly.
        // Smooth alpha fade-in (full screen) ramped by the same reveal progress
        // as the body text, so the dim and the text appear together gradually.
        if (GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_HELP_DETAIL) {
            float t = GAMEMENU_help_detail_reveal_t() * GAMEMENU_HELP_DIM_FADE_SPEEDUP;
            if (t > 1.0f) t = 1.0f;
            const ULONG dim_a = (ULONG)((float)(GAMEMENU_HELP_DETAIL_DIM_COLOUR >> 24) * t);
            const ULONG dim = (GAMEMENU_HELP_DETAIL_DIM_COLOUR & 0x00FFFFFFu) | (dim_a << 24);
            PANEL_draw_quad(0.0F, 0.0F, 640.0F, 480.0F, POLY_PAGE_ALPHA_OVERLAY, dim);
        }
        POLY_frame_draw(UC_FALSE, UC_FALSE);
        PolyPage::pop_ui_mode();
    }

    // Detail screen uses the WHOLE window (uniform scale, no 4:3 frame) so the
    // text can span the full width and height without distortion. Its own scope
    // + frame; nothing else to draw, so return after.
    if (GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_HELP_DETAIL) {
        PolyPage::push_uniform_fullscreen_ui_mode();
        POLY_frame_init(UC_FALSE, UC_FALSE);
        MENUFONT_fadein_line(GAMEMENU_fadein_x);
        GAMEMENU_draw_help_detail();
        POLY_frame_draw(UC_FALSE, UC_FALSE);
        PolyPage::pop_ui_mode();
        return;
    }

    // Other menus stay framed (4:3 centred region with bars).
    PolyPage::UIModeScope _ui_scope(ui_coords::UIAnchor::CENTER_CENTER);

    POLY_frame_init(UC_FALSE, UC_FALSE);

    MENUFONT_fadein_line(GAMEMENU_fadein_x);

    if (GAMEMENU_menu_type == GAMEMENU_MENU_TYPE_HELP_LIST) {
        GAMEMENU_draw_help_list();
    } else {
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
                const SLONG word = GAMEMENU_menu[GAMEMENU_menu_type].word[i];
                if (word) {
                    // The "Controls" item reuses X_CONTROLS for its id (nav /
                    // confirm), but shows a literal label — there is no fitting
                    // localized string and 1.0 ships English.
                    CBYTE* label = (word == X_CONTROLS) ? (CBYTE*)GAMEMENU_HELP_MENU_LABEL
                                                        : XLAT_str(word);
                    MENUFONT_fadein_draw(
                        320,
                        155 + i * 40,
                        (i == GAMEMENU_menu_selection) ? 255 : 128,
                        label);
                }
            }
        }
    }

    POLY_frame_draw(UC_FALSE, UC_FALSE);
}
