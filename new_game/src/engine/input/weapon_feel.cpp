// ===========================================================================
// weapon_feel — module manual (read me first)
// ===========================================================================
//
// Purpose: encapsulate everything that shapes how firing a weapon FEELS on
// a gamepad. Three interlocking subsystems:
//
//   1. Haptic envelope — per-weapon amplitude envelope computed once from
//      the weapon's WAV, played on the DualSense slow-rumble motor on each
//      shot. Gives per-weapon "texture" instead of a buzz.
//
//   2. Adaptive trigger (DualSense) — physical effect on R2 selected per
//      weapon via WeaponFeelProfile::trigger_effect:
//        * Weapon  — Weapon25 single click at a narrow zone (pistol).
//        * Machine — two-beat continuous pulse spanning a wider zone
//                    (AK47; rattle stays present while R2 is held).
//        * None    — trigger free (shotgun).
//      The mode state machine lives in gamepad.cpp; this module only
//      provides the parameters.
//
//   3. Analog fire detection — converts R2/L2 to discrete "shoot" and
//      "kick" events on rising edges past fire_threshold, with rearm via
//      release below reset_threshold. Fully replaces the old constexpr
//      fire threshold block that used to live in input_actions.cpp.
//
// All three subsystems share a single WeaponFeelProfile registry keyed by
// SPECIAL_* constant. Adding a custom weapon after 1.0 = one registration
// call in weapon_feel_init(), no edits anywhere else.
//
// ===========================================================================
// Call sites and per-tick flow
// ===========================================================================
//
// Per game tick, in order (verify this matches game.cpp if refactoring):
//
//   1. gamepad_poll()                  — reads analog trigger into
//                                        gamepad_state.trigger_{left,right}
//   2. get_hardware_input()            — input_actions.cpp calls
//                                        weapon_feel_evaluate_fire(weapon,
//                                        r2, l2, weapon_drawn) to turn
//                                        analog values into discrete
//                                        PUNCH/KICK input bits
//   3. game simulation                 — processes PUNCH → actually_fire_gun
//                                        → weapon_feel_on_shot_fired()
//                                        (only on REAL fires, gated by
//                                        Timer1 inside actually_fire_gun)
//   4. gamepad_rumble_tick()           — calls weapon_feel_tick_haptic()
//                                        and max-merges the envelope into
//                                        the slow rumble motor
//   5. gamepad_triggers_update()       — reads weapon ready state and
//                                        applies AIM_GUN / NONE mode via
//                                        the Weapon25 params from the
//                                        current weapon's profile
//
// Init/shutdown are tied to gamepad_init / gamepad_shutdown.
//
// ===========================================================================
// Current fire-detection design (final)
// ===========================================================================
//
// Two paths share one evaluator, picked at runtime per weapon+device:
//
//   * ADAPTIVE-CLICK path — used when the device is a DualSense AND the
//     effective effect is Weapon25 (single-shot or auto-fire in reload-
//     press mode — see below) with non-zero strength AND the weapon is
//     currently drawn (caller passes FLAG_PERSON_GUN_OUT as
//     `weapon_drawn`). Fires when the analog trigger crosses the upper
//     edge of the Weapon25 zone going up (end_zone × R2_UNITS_PER_ZONE).
//     This puts the PC-side fire event at the same physical trigger
//     position where the DualSense motor physically clicks — tactile
//     feedback and game shot land on the same instant. No rising-edge
//     armed gate on this path; hardware naturally enforces one click
//     per press cycle. Game Timer1 still rate-limits.
//
//   * THRESHOLD fallback — used for auto-fire weapons during normal
//     firing (AK47 with Machine effect, mag not empty), effect-less
//     profiles (shotgun today), bare-hand melee (no gun out), and all
//     non-DualSense devices (Xbox, keyboard-as-gamepad). Plain rising-
//     edge: r2 ≤ reset_threshold arms, r2 > fire_threshold fires. Auto-
//     fire ignores the armed gate. For weapons with a Machine effect,
//     fire_threshold is derived from trigger_start_zone so the game
//     begins firing at the same position where the trigger pulse
//     becomes audible.
//
//   * RELOAD-PRESS mode — special case for auto-fire weapons with a
//     reload click configured (AK47). While the magazine is empty,
//     evaluate_fire behaves as if the weapon were a single-shot pistol:
//     Weapon25 effect, act-bit path, click zone from reload_click_*
//     params. Synchronises the reload HAD_TO_CHANGE_CLIP moment with
//     the hardware click (both fire at reload_click_end_zone). Reverts
//     to Machine / threshold path the moment the clip is refilled AND
//     the reload gate is cleared (see s_ak47_reload_gate in
//     input_actions.cpp).
//
// Why two paths: on DualSense the zone-upper crossing aligns with
// where the motor physically snaps (by construction — both come from
// the profile's trigger_end_zone). Threshold-based detection on r2
// position is an empirical guess at where the click point is — works
// fine without an adaptive click, but desynchronises from tactile
// feedback when a motor is present.
//
// Cooldown gating (final design, 2026-04-18): running and standing fire
// share ONE counter — Person::Timer1 — sized to the shoot animation's
// actual tick count (weapon_feel_consume_shot_cooldown_timer1 → anim
// sim). Symmetric:
//
//   * Running: set_person_running_shoot gates on strict Timer1==0,
//     fn_person_moveing decrements.
//   * Standing: SUB_STATE_SHOOT_GUN for the player decrements Timer1
//     and exits to aim when it reaches 0 (instead of anim end).
//
// game.cpp on_cooldown = Timer1 > pre_release. Motor mode=AIM_GUN
// turns on at the same threshold the game accepts fire — pre_release
// is currently 0, so they're synchronous. A non-zero pre_release
// would let the motor warm up before the game allows fire, but also
// opens a window where the motor can click during cooldown (felt as
// "phantom clicks" by the user). Kept at 0 to guarantee strict
// click⇔shot synchronization; trade-off is occasional silent presses
// in the HID RTT window (~30 ms) right after a cooldown expires.
//
// Immediate mode=NONE on the fire frame (weapon_feel_on_shot_fired
// calls gamepad_triggers_lockout(0)) kicks in so a rapid re-press
// within the HID RTT window doesn't land on a still-active motor.
//
// History of dead-ends we already walked (armed gate, release-settle,
// post-fire cooldown, split mode/fire lockout, MIN_PRESS_DURATION,
// pending buffer, pre-release fire) is in the devlog section
// "Подходы которые пробовали" and later handoff sections. Do not
// reinvent those without re-reading why each broke R1/R2/R3.
//
// ===========================================================================
// Hard-and-fast requirements — verify before any change
// ===========================================================================
//
// Full checklist lives in the devlog file (R1-R5 gameplay, P1-P5 process).
// The two most important:
//
//   R1 — natural fire rate: NEVER slow fire rate below game Timer1.
//   R2 — keyboard-like taps: presses during cooldown silently ignored,
//        MUST NOT break subsequent presses via sticky state.
//
// If a proposed change violates either of these, stop and discuss.
//
// ===========================================================================

#include "engine/input/weapon_feel.h"
#include "engine/input/gamepad.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/animation/anim_types.h"
#include "assets/formats/anim_globals.h"
#include "assets/sound_id.h"
#include "assets/sound_id_globals.h"
#include "things/characters/anim_ids.h"
#include "things/characters/person_types.h"
#include "things/items/special.h"
#include "game/game_types.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

// sound_list is defined in assets/sound_id_globals.cpp at global scope.
// Must be declared outside the anonymous namespace below, otherwise the
// extern gets internal linkage and the linker can't find it.
extern CBYTE* sound_list[];

// ===========================================================================
// Profile registry
// ===========================================================================
//
// Kept as a map from SPECIAL_* → profile + precomputed envelope. The envelope
// is built lazily: either immediately during weapon_feel_init() for built-in
// profiles, or on weapon_feel_register() if called post-init.

namespace {

// ---------------------------------------------------------------------------
// Module constants
// ---------------------------------------------------------------------------

// Haptic envelope sample period — one envelope bin covers this many seconds
// of source audio. 5 ms gives enough resolution to capture a gunshot's
// attack transient without bloating memory.
constexpr float ENV_STEP_SECONDS = 0.005f;

// Weapon25 adaptive-trigger geometry. The hardware divides the R2 travel
// (0..255) into 9 discrete zones (0..8 inclusive), one zone every 255/9 ≈
// 28 units. Profile fields trigger_start_zone / trigger_end_zone are in
// zone units; multiplying by R2_UNITS_PER_ZONE converts to an r2 value to
// compare physical trigger position against a zone boundary.
constexpr int WEAPON25_ZONE_COUNT = 9;
constexpr int R2_UNITS_PER_ZONE = 256 / WEAPON25_ZONE_COUNT; // = 28

// Threshold-path defaults for single-shot weapons. The act-bit click path
// (DualSense + Weapon25 effect) ignores these and fires on zone crossing;
// the threshold fallback uses them for keyboard, Xbox, bare-hand melee,
// and weapons with no adaptive click.
//   FIRE:  r2 at which a press is counted as a shot (well past the click
//          zone so a weak squeeze doesn't fire).
//   RESET: r2 at which rising-edge rearms (player must release this deep
//          between shots).
// Chosen to sit comfortably outside the typical zone 4..6 click range
// (~112..168 r2) so the fallback still feels similar to the hardware click.
constexpr uint8_t DEFAULT_FIRE_THRESHOLD_R2 = 200;
constexpr uint8_t DEFAULT_RESET_THRESHOLD_R2 = 80;

struct HapticEnvelope {
    std::vector<uint8_t> samples; // 0..255, one per ENV_STEP_SECONDS window
    bool loaded = false;
};

struct ProfileEntry {
    WeaponFeelProfile profile;
    HapticEnvelope envelope;
    // Anim-derived cooldown: cached number of game ticks the weapon's
    // shoot animation takes to reach its end, at the current TICK_RATIO.
    // Lazily computed on first query — anim data isn't loaded at
    // weapon_feel_init() time. Zero = not yet computed (or computation
    // failed).
    SLONG cached_anim_ticks = 0;
};

std::unordered_map<int32_t, ProfileEntry> s_profiles;
bool s_initialized = false;

// Default fallback for weapons with no registered profile. Pistol-like
// rising-edge fire with no haptic and no adaptive trigger effect.
const WeaponFeelProfile k_default_profile = {
    /*haptic_wave_id*/ 0,
    /*haptic_gain*/ 0.0f,
    /*haptic_ceiling*/ 0,
    /*haptic_max_seconds*/ 0.0f,
    /*haptic_xbox_boost*/ 1.0f,
    /*xbox_rumble_low_percent*/ 100,
    /*xbox_rumble_high_percent*/ 0,
    /*trigger_effect*/ TriggerEffectType::None,
    /*trigger_start_zone*/ 0,
    /*trigger_end_zone*/ 0,
    /*trigger_strength*/ 0,
    /*machine_amp_a*/ 0,
    /*machine_amp_b*/ 0,
    /*machine_frequency*/ 0,
    /*machine_period*/ 0,
    /*reload_click_start_zone*/ 0,
    /*reload_click_end_zone*/ 0,
    /*reload_click_strength*/ 0,
    /*post_shot_vibration_seconds*/ 0.0f,
    /*post_shot_vibration_position*/ 0,
    /*post_shot_vibration_frequency*/ 0,
    /*post_shot_vibration_amp_start*/ 0,
    /*post_shot_vibration_amp_end*/ 0,
    /*aim_interlude_anim*/ 0,
    /*fire_threshold*/ 200,
    /*reset_threshold*/ 80,
    /*auto_fire*/ false,
};

// ---------------------------------------------------------------------------
// Anim-derived shot cooldown
// ---------------------------------------------------------------------------
//
// The game's per-weapon shoot cooldown was originally hardcoded as
// Timer1=140/64/400 in shoot_get_ammo_sound_anim_time — values the
// original authors picked to roughly match the length of each weapon's
// standing-shoot animation. We derive the actual tick count from the
// animation data and use it for both the standing shot (where the
// animation still plays) and the running shot (where the same timing
// is applied to Timer1 without the anim actually playing). Single
// source of truth: the asset. If an animator adds/removes frames, both
// standing and running rate adapt automatically.

// Player character's AnimType — 0 for Darci / Roper (they share slot 0 in
// global_anim_array; the shoot animations live there). Non-player
// characters use different slots but they continue to use the original
// hardcoded Timer1 values, so this mapping only applies to player shots.
constexpr SLONG PLAYER_ANIM_TYPE = ANIM_TYPE_DARCI;

SLONG weapon_to_shoot_anim_id(int32_t special_type)
{
    switch (special_type) {
    case SPECIAL_GUN:
        return ANIM_PISTOL_SHOOT;
    case SPECIAL_NONE:
        return ANIM_PISTOL_SHOOT; // pistol profile fallback
    case SPECIAL_AK47:
        return ANIM_AK_FIRE;
    case SPECIAL_SHOTGUN:
        return ANIM_SHOTGUN_FIRE;
    }
    return 0;
}

// Simulates person_normal_animate_speed to count how many game ticks
// the given animation takes from first frame to end-of-anim (end==1).
// Mirrors the tick/tween logic in person.cpp so calculated duration
// matches the live state machine exactly.
// Returns 0 if the anim isn't loaded (e.g. not yet loaded at init time).
SLONG calc_anim_ticks(SLONG anim_type, SLONG anim_id)
{
    if (anim_type < 0 || anim_type >= 4)
        return 0;
    if (anim_id < 0 || anim_id >= 450)
        return 0;
    GameKeyFrame* first_frame = global_anim_array[anim_type][anim_id];
    if (!first_frame || !first_frame->NextFrame)
        return 0;

    GameKeyFrame* current = first_frame;
    GameKeyFrame* next = first_frame->NextFrame;
    SLONG animTween = 0;

    // Safety cap — real shoot anims complete in <20 ticks; 1024 is a
    // huge margin that still prevents runaway on malformed data.
    constexpr SLONG MAX_TICKS = 1024;

    for (SLONG tick = 1; tick <= MAX_TICKS; tick++) {
        SLONG tween_step = (current->TweenStep << 1) * TICK_RATIO >> TICK_SHIFT;
        if (tween_step <= 0)
            tween_step = 1;
        animTween += tween_step;

        while (animTween >= 256) {
            animTween -= 256;
            if (current->TweenStep != 0) {
                animTween = (animTween * next->TweenStep) / current->TweenStep;
            }
            // advance_keyframe equivalent: CurrentFrame = NextFrame;
            // then NextFrame = NextFrame->NextFrame (if exists).
            current = next;
            if (next->NextFrame) {
                next = next->NextFrame;
                if (current->Flags & ANIM_FLAG_LAST_FRAME)
                    return tick;
            } else {
                return tick;
            }
        }
    }
    return MAX_TICKS;
}

// Convert a tick count into Timer1 units. Person::Timer1 decrements by
// `16 * TICK_RATIO >> TICK_SHIFT` once per tick (see fn_person_moveing).
// Multiplying ticks by that per-tick decrement gives a Timer1 value that
// takes exactly `ticks` game ticks to reach zero.
SLONG ticks_to_timer1(SLONG ticks)
{
    const SLONG per_tick = 16 * TICK_RATIO >> TICK_SHIFT;
    return ticks * (per_tick > 0 ? per_tick : 1);
}

// Debt carried from pre-release fire. When the player presses while
// Timer1 is still counting (but inside the pre-release window), the
// shot fires early and the remaining Timer1 is stashed here. The next
// shot's Timer1 starts at `anim_duration + debt`, so average fire rate
// stays tied to the animation duration even when individual shots
// borrow forward by a tick or two.
SLONG s_cooldown_debt = 0;

// Internal: look up the weapon's shoot-anim duration in ticks, with
// lazy caching on the ProfileEntry. Safe to call every frame — first
// call does the simulation, subsequent calls read the cache. Returns
// 0 if the weapon isn't registered or its shoot anim isn't loaded yet
// (e.g. called during level transition).
SLONG weapon_feel_get_shoot_anim_ticks(int32_t special_type)
{
    auto it = s_profiles.find(special_type);
    if (it == s_profiles.end())
        return 0;
    ProfileEntry& entry = it->second;
    if (entry.cached_anim_ticks > 0)
        return entry.cached_anim_ticks;

    SLONG anim_id = weapon_to_shoot_anim_id(special_type);
    if (anim_id == 0)
        return 0;

    SLONG ticks = calc_anim_ticks(PLAYER_ANIM_TYPE, anim_id);
    if (ticks > 0)
        entry.cached_anim_ticks = ticks;
    return ticks;
}

// ---------------------------------------------------------------------------
// Envelope computation
// ---------------------------------------------------------------------------

// MFX's GetFullName prepends "./data/sfx/1622/"; we replicate locally to
// avoid coupling weapon_feel to the sound engine.
void build_sfx_path(const char* fname, char* out, size_t out_size)
{
    std::snprintf(out, out_size, "./data/sfx/1622/%s", fname);
}

// RMS-windowed amplitude envelope. Mix channels to mono, normalize to 0..255
// using per-weapon gain, clamp at ceiling. Peak is not normalized — we want
// absolute loudness to drive motor intensity so the relative feel between
// pistol and shotgun matches the audio.
void compute_envelope(const int16_t* pcm, size_t frame_count, int channels,
    int sample_rate, const WeaponFeelProfile& p,
    HapticEnvelope* out)
{
    if (frame_count == 0 || channels <= 0 || sample_rate <= 0)
        return;

    const size_t window_frames = static_cast<size_t>(
        static_cast<float>(sample_rate) * ENV_STEP_SECONDS);
    if (window_frames == 0)
        return;

    const size_t max_frames = static_cast<size_t>(
        static_cast<float>(sample_rate) * p.haptic_max_seconds);
    const size_t effective_frames = frame_count < max_frames ? frame_count : max_frames;

    const size_t env_len = (effective_frames + window_frames - 1) / window_frames;
    out->samples.resize(env_len);

    for (size_t w = 0; w < env_len; w++) {
        const size_t frame_start = w * window_frames;
        const size_t frame_end = std::min(frame_start + window_frames, effective_frames);
        double sum_sq = 0.0;
        size_t count = 0;
        for (size_t f = frame_start; f < frame_end; f++) {
            int64_t mono = 0;
            for (int c = 0; c < channels; c++) {
                mono += pcm[f * channels + c];
            }
            mono /= channels;
            const double v = static_cast<double>(mono) / 32768.0;
            sum_sq += v * v;
            count++;
        }
        const float rms = count ? static_cast<float>(std::sqrt(sum_sq / count)) : 0.0f;
        float v = rms * p.haptic_gain * 255.0f;
        const float ceiling_f = static_cast<float>(p.haptic_ceiling);
        if (v < 0.0f)
            v = 0.0f;
        if (v > ceiling_f)
            v = ceiling_f;
        out->samples[w] = static_cast<uint8_t>(v);
    }
}

bool build_envelope_from_wave(const WeaponFeelProfile& p, HapticEnvelope* out)
{
    if (p.haptic_wave_id == 0)
        return false;

    const char* fname = sound_list[p.haptic_wave_id];
    if (!fname || std::strcmp(fname, "NULL.wav") == 0)
        return false;

    char path[512];
    build_sfx_path(fname, path, sizeof(path));

    SDL3_WavData wav;
    if (!sdl3_load_wav(path, &wav)) {
        std::fprintf(stderr, "[weapon_feel] failed to load WAV for envelope: %s\n", path);
        return false;
    }

    const size_t bytes_per_frame = static_cast<size_t>(wav.channels) * 2; // 16-bit
    const size_t frame_count = wav.size / bytes_per_frame;
    const int16_t* pcm = reinterpret_cast<const int16_t*>(wav.buffer);

    compute_envelope(pcm, frame_count, wav.channels, wav.freq, p, out);
    out->loaded = !out->samples.empty();

    sdl3_free_wav(wav.buffer);
    return out->loaded;
}

// ===========================================================================
// Envelope playback state
// ===========================================================================

bool s_playing = false;
float s_play_time = 0.0f;
size_t s_prev_index = 0;
const HapticEnvelope* s_active_env = nullptr;
// Non-DS multiplier for the currently playing envelope. Latched when
// the envelope starts so a weapon swap mid-playback doesn't remix
// scaling factors on the same waveform. Applied only on non-DualSense
// devices; DualSense path reads s_active_env peaks unchanged.
float s_active_xbox_boost = 1.0f;
// Per-motor envelope scaling for the Xbox / SDL rumble path. Latched
// from the profile when an envelope starts so a mid-playback weapon
// swap doesn't remix scales. Defaults match "single-motor low-freq
// only" — pre-per-motor behaviour.
uint8_t s_active_xbox_rumble_low_percent = 100;
uint8_t s_active_xbox_rumble_high_percent = 0;

std::chrono::steady_clock::time_point s_last_tick;
bool s_tick_initialized = false;

// ===========================================================================
// Post-shot trigger vibration burst state (see WeaponFeelProfile.post_shot_vibration_*)
// ===========================================================================

bool s_tv_active = false; // burst in progress
float s_tv_elapsed = 0.0f; // seconds since burst start
float s_tv_duration = 0.0f; // total duration
uint8_t s_tv_position = 0;
uint8_t s_tv_frequency = 1;
uint8_t s_tv_amp_start = 0;
uint8_t s_tv_amp_end = 0;
std::chrono::steady_clock::time_point s_tv_last_tick;
bool s_tv_tick_initialized = false;

// ===========================================================================
// Fire detection state
// ===========================================================================
// (see module manual at top of file for design rationale and constraints)

// Rising-edge rearm flags — gate fire in the threshold-based fallback
// path (auto-fire weapons, click-less weapons, non-DualSense devices).
// Set to true when R2/L2 drops to ≤ reset_threshold. Set to false when
// a fire event is emitted. Initial value is true so the first shot
// after gamepad init works without requiring a dummy release first.
// The DualSense act-bit path doesn't consult s_r2_armed — the hardware
// naturally enforces one click per physical press cycle.
bool s_r2_armed = true;
bool s_l2_armed = true;

// Previous-tick analog values for rising-edge / zone-crossing detection.
int s_prev_r2 = 0;
int s_prev_l2 = 0;

} // namespace

// ===========================================================================
// Lifecycle
// ===========================================================================

void weapon_feel_init()
{
    // Built-in profiles — user-tuned, see
    // new_game_devlog/weapon_haptic_and_adaptive_trigger.md. Thresholds and
    // Weapon25 params are currently identical across weapons but live in the
    // profile so per-weapon tuning is a one-line change.

    // AK47 — auto-fire. Adaptive trigger runs Machine (0x27) pulse spanning
    // zones [PISTOL_CLICK_START..8] so the player feels "machine gun" rattle
    // the moment R2 enters the firing zone and it stays present even when
    // R2 is pressed to the stop. fire_threshold is derived from the same
    // start zone so the game starts firing on the same physical position
    // where the trigger effect kicks in. Machine frequency/period/amps are
    // initial guesses — user will tune by feel. haptic_max_seconds <
    // cooldown between auto-fire shots so envelopes don't merge into buzz.
    constexpr uint8_t PISTOL_CLICK_START = 4;
    // end=9 makes the pulse zone reach the trigger's full press stop.
    // With end=8 a fully bottomed-out R2 (zone 9) sits past the effect
    // and the pulse goes silent — the user felt this as "выключается
    // при вжимании в пол".
    constexpr uint8_t AK47_MACHINE_END = 9;
    // Machine params mirror the input-debug Machine tester defaults —
    // empirically confirmed to produce a clearly felt rattle on real
    // hardware. Weaker values (ampA=5, ampB=2, period=4) were tried
    // first and felt absent on the controller.
    constexpr uint8_t AK47_MACHINE_AMP_A = 7; // 0..7, pulse amplitude high beat
    constexpr uint8_t AK47_MACHINE_AMP_B = 3; // 0..7, low beat
    constexpr uint8_t AK47_MACHINE_FREQ = 8; // pulse frequency (Hz-ish)
    constexpr uint8_t AK47_MACHINE_PERIOD = 80; // pattern period; higher = slower pulse rate
    WeaponFeelProfile ak47 = {
        /*haptic_wave_id*/ S_AK47_BURST,
        /*haptic_gain*/ 0.4f,
        /*haptic_ceiling*/ 90,
        /*haptic_max_seconds*/ 0.04f,
        /*haptic_xbox_boost*/ 1.0f,
        // AK47 on Xbox: fan the envelope out to BOTH motors with the
        // low-freq thump slightly weaker than the high-freq buzz.
        // Single-motor low-only felt flat for held-fire auto bursts.
        /*xbox_rumble_low_percent*/ 75,
        /*xbox_rumble_high_percent*/ 100,
        /*trigger_effect*/ TriggerEffectType::Machine,
        /*trigger_start_zone*/ PISTOL_CLICK_START,
        /*trigger_end_zone*/ AK47_MACHINE_END,
        /*trigger_strength*/ 0, // unused for Machine
        /*machine_amp_a*/ AK47_MACHINE_AMP_A,
        /*machine_amp_b*/ AK47_MACHINE_AMP_B,
        /*machine_frequency*/ AK47_MACHINE_FREQ,
        /*machine_period*/ AK47_MACHINE_PERIOD,
        // Reload click: when mag is empty, trigger feels like the pistol
        // (single Weapon25 click) so the reload press has a physical snap.
        // Params match the pistol profile — same zone + strength.
        /*reload_click_start_zone*/ 4,
        /*reload_click_end_zone*/ 6,
        /*reload_click_strength*/ 5,
        // No post-shot trigger vibration on AK47 — the Machine pulse is
        // the ongoing feedback; a vibration burst on top would be mud.
        /*post_shot_vibration_seconds*/ 0.0f,
        /*post_shot_vibration_position*/ 0,
        /*post_shot_vibration_frequency*/ 0,
        /*post_shot_vibration_amp_start*/ 0,
        /*post_shot_vibration_amp_end*/ 0,
        // Between-shot pose anim matches what set_person_aim would have
        // assigned on the original AIM_GUN transition (SpecialUse != NULL
        // → ANIM_SHOTGUN_AIM). Unified SHOOT_GUN handler swaps to this
        // after the shoot anim ends so persona still "колбасится"
        // visually (SHOOT pose → aim pose → SHOOT pose per cycle) even
        // though the state machine no longer bounces through AIM_GUN.
        /*aim_interlude_anim*/ ANIM_SHOTGUN_AIM,
        /*fire_threshold*/ (uint8_t)(PISTOL_CLICK_START * R2_UNITS_PER_ZONE),
        /*reset_threshold*/ DEFAULT_RESET_THRESHOLD_R2,
        /*auto_fire*/ true,
    };
    // Pistol — single-shot light pop. Rising-edge fire on Weapon25 click.
    // Xbox boost compensates for the rumble motor's low-amplitude
    // insensitivity — the DS-calibrated ceiling=18 is barely perceptible
    // on Xbox without it.
    WeaponFeelProfile pistol = {
        /*haptic_wave_id*/ S_PISTOL_SHOT,
        /*haptic_gain*/ 0.05f,
        /*haptic_ceiling*/ 18,
        /*haptic_max_seconds*/ 0.035f,
        /*haptic_xbox_boost*/ 4.0f,
        /*xbox_rumble_low_percent*/ 100, // single-motor low-only (unchanged)
        /*xbox_rumble_high_percent*/ 0,
        /*trigger_effect*/ TriggerEffectType::Weapon,
        /*trigger_start_zone*/ PISTOL_CLICK_START,
        /*trigger_end_zone*/ 6,
        /*trigger_strength*/ 5,
        /*machine_amp_a*/ 0,
        /*machine_amp_b*/ 0,
        /*machine_frequency*/ 0,
        /*machine_period*/ 0,
        /*reload_click_start_zone*/ 0, // pistol's main effect already IS a click
        /*reload_click_end_zone*/ 0,
        /*reload_click_strength*/ 0,
        // No post-shot trigger vibration — pistol's single click is the
        // whole story; a buzz tail after would clash with "clean pop" feel.
        /*post_shot_vibration_seconds*/ 0.0f,
        /*post_shot_vibration_position*/ 0,
        /*post_shot_vibration_frequency*/ 0,
        /*post_shot_vibration_amp_start*/ 0,
        /*post_shot_vibration_amp_end*/ 0,
        /*aim_interlude_anim*/ 0, // single-shot — no interlude needed
        /*fire_threshold*/ DEFAULT_FIRE_THRESHOLD_R2,
        /*reset_threshold*/ DEFAULT_RESET_THRESHOLD_R2,
        /*auto_fire*/ false,
    };
    // Shotgun — heavy slam with longer decay. Weapon25 click at narrow
    // zone gives the "pump-action" trigger feel (resistance → snap). Right
    // after the click fires, a decaying Vibration burst plays across the
    // full trigger range (position=0) to emulate the recoil reverb —
    // strong on first impact, fades to nothing. All initial values are
    // first-pass tune — adjust by feel.
    WeaponFeelProfile shotgun = {
        /*haptic_wave_id*/ S_SHOTGUN_SHOT,
        /*haptic_gain*/ 0.55f,
        /*haptic_ceiling*/ 140,
        /*haptic_max_seconds*/ 0.09f,
        /*haptic_xbox_boost*/ 1.0f,
        /*xbox_rumble_low_percent*/ 100, // single-motor low-only (unchanged)
        /*xbox_rumble_high_percent*/ 0,
        /*trigger_effect*/ TriggerEffectType::Weapon,
        /*trigger_start_zone*/ 3, // resistance starts early (feel the "упор")
        /*trigger_end_zone*/ 6,
        /*trigger_strength*/ 7, // stronger than pistol (=5) — shotgun is heavier
        /*machine_amp_a*/ 0,
        /*machine_amp_b*/ 0,
        /*machine_frequency*/ 0,
        /*machine_period*/ 0,
        /*reload_click_start_zone*/ 0,
        /*reload_click_end_zone*/ 0,
        /*reload_click_strength*/ 0,
        // Post-shot vibration: linear fade 8 → 0 over 0.25s, 60Hz buzz,
        // position=0 so it's felt across the full trigger travel.
        /*post_shot_vibration_seconds*/ 0.25f,
        /*post_shot_vibration_position*/ 0,
        /*post_shot_vibration_frequency*/ 60,
        /*post_shot_vibration_amp_start*/ 8,
        /*post_shot_vibration_amp_end*/ 0,
        /*aim_interlude_anim*/ 0, // single-shot — no interlude needed
        /*fire_threshold*/ DEFAULT_FIRE_THRESHOLD_R2,
        /*reset_threshold*/ DEFAULT_RESET_THRESHOLD_R2,
        /*auto_fire*/ false,
    };

    s_initialized = true;
    weapon_feel_register(SPECIAL_AK47, ak47);
    weapon_feel_register(SPECIAL_GUN, pistol);
    weapon_feel_register(SPECIAL_NONE, pistol); // SpecialUse==null reads SpecialType as 0
    weapon_feel_register(SPECIAL_SHOTGUN, shotgun);

    s_playing = false;
    s_active_env = nullptr;
    s_prev_index = 0;
    s_tick_initialized = false;
    s_r2_armed = true;
    s_l2_armed = true;
    s_cooldown_debt = 0;
}

void weapon_feel_shutdown()
{
    s_profiles.clear();
    s_playing = false;
    s_active_env = nullptr;
    s_initialized = false;
}

void weapon_feel_register(int32_t special_type, const WeaponFeelProfile& profile)
{
    ProfileEntry& entry = s_profiles[special_type];
    entry.profile = profile;
    entry.envelope = HapticEnvelope {};
    if (s_initialized && profile.haptic_wave_id != 0) {
        build_envelope_from_wave(profile, &entry.envelope);
    }
}

const WeaponFeelProfile* weapon_feel_get_profile(int32_t special_type)
{
    auto it = s_profiles.find(special_type);
    if (it != s_profiles.end())
        return &it->second.profile;
    return &k_default_profile;
}

// ===========================================================================
// Haptic playback
// ===========================================================================

void weapon_feel_on_shot_fired(int32_t special_type, int32_t remaining_timer1)
{
    // Capture pre-release debt. Only counted when Timer1 is inside the
    // pre-release window — i.e. the player fired "early" by at most a
    // couple of ticks within a real cooldown. Stale Timer1 (e.g. a
    // frozen remainder from a running shot after the player stopped
    // running) sits far above the pre-release threshold and must NOT
    // be treated as debt, or the next cooldown would get a bogus
    // multi-hundred-unit penalty.
    // Threshold mirrors weapon_feel_pre_release_timer1(); kept inline
    // here to avoid forward-declaration churn.
    constexpr SLONG PRE_RELEASE_TICKS = 0;
    SLONG pre_release = PRE_RELEASE_TICKS * (16 * TICK_RATIO >> TICK_SHIFT);
    if (remaining_timer1 > 0 && remaining_timer1 <= pre_release) {
        s_cooldown_debt = remaining_timer1;
    } else {
        s_cooldown_debt = 0;
    }

    auto it = s_profiles.find(special_type);
    if (it == s_profiles.end())
        return;
    const ProfileEntry& entry = it->second;

    // Force mode=NONE on fire frame for single-shot weapons so the
    // motor doesn't emit a phantom click in the HID RTT window before
    // the next gamepad_triggers_update applies the post-fire cooldown
    // gate. Auto-fire weapons keep the motor armed across bursts.
    // gamepad_triggers_lockout is a no-op on non-DualSense devices.
    if (!entry.profile.auto_fire) {
        gamepad_triggers_lockout(0);
    }

    // Start post-shot trigger vibration burst if configured. Placed BEFORE
    // the envelope-loaded early-return because the two paths are
    // independent — burst runs on the trigger motor, envelope runs on
    // the slow rumble motor. Restarts on every shot (consecutive shots
    // stack by resetting the linear fade, matching the rumble envelope
    // restart below).
    if (entry.profile.post_shot_vibration_seconds > 0.0f
        && entry.profile.post_shot_vibration_amp_start > 0) {
        s_tv_active = true;
        s_tv_elapsed = 0.0f;
        s_tv_duration = entry.profile.post_shot_vibration_seconds;
        s_tv_position = entry.profile.post_shot_vibration_position;
        s_tv_frequency = entry.profile.post_shot_vibration_frequency;
        s_tv_amp_start = entry.profile.post_shot_vibration_amp_start;
        s_tv_amp_end = entry.profile.post_shot_vibration_amp_end;
        s_tv_tick_initialized = false;
    }

    // Envelope plays on every device: DualSense slow motor, Xbox / SDL
    // gamepads via the low-frequency rumble channel. gamepad_rumble_tick
    // max-merges the envelope value into whichever output path the
    // active device uses.
    if (!entry.envelope.loaded)
        return;

    // Restart envelope on each shot — matches sound retrigger behaviour for
    // auto-fire bursts.
    s_active_env = &entry.envelope;
    s_active_xbox_boost = entry.profile.haptic_xbox_boost;
    s_active_xbox_rumble_low_percent = entry.profile.xbox_rumble_low_percent;
    s_active_xbox_rumble_high_percent = entry.profile.xbox_rumble_high_percent;
    s_play_time = 0.0f;
    s_prev_index = 0;
    s_playing = true;
    // Force dt=0 on the next tick so we start from sample 0 rather than
    // jumping into stale time from a previous idle session.
    s_tick_initialized = false;
}

int32_t weapon_feel_consume_shot_cooldown_timer1(int32_t special_type)
{
    SLONG ticks = weapon_feel_get_shoot_anim_ticks(special_type);
    if (ticks <= 0) {
        // Anim not loaded yet or unknown weapon — caller falls back to
        // its own hardcoded value. Any stashed debt is cleared anyway
        // so it doesn't leak into a future query.
        s_cooldown_debt = 0;
        return 0;
    }

    // Auto-fire weapons (AK47) need a longer between-shot cooldown than
    // the shoot-anim tick count alone produces. Original MuckyFoot
    // achieved this by stacking a separate TICK_TOCK accumulator in
    // SUB_STATE_AIM_GUN on top of the SHOOT_GUN Timer1 decrement — two
    // phases per shot cycle. We removed that accumulator (player only)
    // to unify standing and running rate; to keep the user-preferred
    // slower cadence (matches the original standing feel), the anim-
    // derived Timer1 is scaled up here for auto-fire weapons. Empirical
    // factor: 2 (observed standing cycle ≈ 2× running cycle in the
    // original two-phase design). Non-auto-fire weapons (pistol)
    // unaffected — they already worked symmetrically via pure anim-
    // derived Timer1 since set_person_shoot was unified 2026-04-18.
    constexpr SLONG AUTO_FIRE_COOLDOWN_SCALE = 2;
    const WeaponFeelProfile* p = weapon_feel_get_profile(special_type);
    const SLONG scale = p->auto_fire ? AUTO_FIRE_COOLDOWN_SCALE : 1;

    SLONG base = ticks_to_timer1(ticks) * scale;
    SLONG total = base + s_cooldown_debt;
    s_cooldown_debt = 0;
    return total;
}

bool weapon_feel_trigger_effect_should_run(int32_t current_weapon, bool in_shot_cycle, bool mag_empty)
{
    const WeaponFeelProfile* p = weapon_feel_get_profile(current_weapon);
    // Auto-fire: Machine pulse = recoil from real shots. ON while a
    // shot cycle is ticking; as soon as the game stops issuing shots
    // (mag empty, no PUNCH, interrupted state) in_shot_cycle falls to
    // false and the effect dies with it. Additionally: when the clip
    // is empty AND a reload click is configured, the effect stays ON
    // (but gamepad.cpp dispatches it as Weapon25 with reload params
    // instead of Machine) so the player feels a click on the reload
    // press.
    //
    // Single-shot: Weapon25 click = pre-fire resistance. OFF during
    // cooldown (can't re-click yet), ON otherwise. Opposite polarity
    // from auto-fire by design.
    if (p->auto_fire) {
        if (in_shot_cycle)
            return true;
        if (mag_empty && p->reload_click_strength != 0)
            return true;
        return false;
    }
    // Single-shot: normally OFF during cooldown. Exception: if a post-shot
    // trigger vibration burst is active, keep the trigger effect running
    // so gamepad.cpp's Vibration override actually reaches the hardware —
    // otherwise weapon_ready falls to false during exactly the window
    // we want the recoil buzz in (Timer1 > 0 == in_shot_cycle for
    // single-shot by design).
    if (s_tv_active)
        return true;
    return !in_shot_cycle;
}

int32_t weapon_feel_pre_release_timer1()
{
    // Two ticks' worth of Timer1 decrement. One tick covers the HID
    // round-trip for mode=AIM_GUN; the second gives hardware motor
    // time to engage before the player's press reaches the click
    // point. Empirically the minimum that avoids silent shots right
    // after a cooldown on DualSense. See handoff in
    // weapon_haptic_and_adaptive_trigger.md for the reasoning.
    constexpr SLONG PRE_RELEASE_TICKS = 0;
    return PRE_RELEASE_TICKS * (16 * TICK_RATIO >> TICK_SHIFT);
}

uint8_t weapon_feel_tick_haptic(bool* out_active)
{
    if (out_active)
        *out_active = false;
    if (!s_playing || !s_active_env)
        return 0;

    const auto now = std::chrono::steady_clock::now();
    if (!s_tick_initialized) {
        s_last_tick = now;
        s_tick_initialized = true;
    }
    const float dt = std::chrono::duration<float>(now - s_last_tick).count();
    s_last_tick = now;

    s_play_time += dt;

    const size_t curr_index = static_cast<size_t>(s_play_time / ENV_STEP_SECONDS);
    const size_t env_len = s_active_env->samples.size();

    if (s_prev_index >= env_len) {
        s_playing = false;
        s_active_env = nullptr;
        s_prev_index = 0;
        return 0;
    }

    // MAX over [prev_index, curr_index] — coarse game tick (~33ms) can
    // straddle multiple 5ms envelope samples; taking the peak avoids
    // aliasing past peaks.
    const size_t scan_end = curr_index < env_len ? curr_index : env_len - 1;
    uint8_t peak = 0;
    for (size_t i = s_prev_index; i <= scan_end; i++) {
        if (s_active_env->samples[i] > peak)
            peak = s_active_env->samples[i];
    }
    s_prev_index = curr_index + 1;

    // Non-DualSense devices get the profile's Xbox boost applied — rumble
    // motors on Xbox / generic gamepads are less responsive at low
    // amplitudes so weapons tuned quiet for DS feel absent without it.
    // DS path keeps the envelope peak untouched.
    if (gamepad_get_device_type() != INPUT_DEVICE_DUALSENSE && s_active_xbox_boost != 1.0f) {
        float boosted = (float)peak * s_active_xbox_boost;
        if (boosted > 255.0f)
            boosted = 255.0f;
        peak = (uint8_t)boosted;
    }

    if (out_active)
        *out_active = true;
    return peak;
}

void weapon_feel_stop_haptic()
{
    s_playing = false;
    s_active_env = nullptr;
}

void weapon_feel_get_active_xbox_rumble_scales(uint8_t* out_low_percent,
    uint8_t* out_high_percent)
{
    if (out_low_percent)
        *out_low_percent = s_active_xbox_rumble_low_percent;
    if (out_high_percent)
        *out_high_percent = s_active_xbox_rumble_high_percent;
}

bool weapon_feel_tick_trigger_vibration(
    uint8_t* out_position,
    uint8_t* out_amplitude,
    uint8_t* out_frequency)
{
    if (!s_tv_active)
        return false;

    const auto now = std::chrono::steady_clock::now();
    if (!s_tv_tick_initialized) {
        s_tv_last_tick = now;
        s_tv_tick_initialized = true;
    }
    const float dt = std::chrono::duration<float>(now - s_tv_last_tick).count();
    s_tv_last_tick = now;
    s_tv_elapsed += dt;

    if (s_tv_duration <= 0.0f || s_tv_elapsed >= s_tv_duration) {
        s_tv_active = false;
        return false;
    }

    // Linear amplitude interp from amp_start → amp_end across the burst.
    const float t = s_tv_elapsed / s_tv_duration;
    const float a = (1.0f - t) * (float)s_tv_amp_start + t * (float)s_tv_amp_end;
    int amp_i = (int)(a + 0.5f);
    if (amp_i < 0)
        amp_i = 0;
    if (amp_i > 8)
        amp_i = 8;

    if (out_position)
        *out_position = s_tv_position;
    if (out_amplitude)
        *out_amplitude = (uint8_t)amp_i;
    if (out_frequency)
        *out_frequency = s_tv_frequency;
    return true;
}

// ===========================================================================
// Fire detection
// ===========================================================================

// Two fire-detection paths in one function:
//
//   Act-bit path (DualSense + weapon has adaptive click + single-shot
//   + weapon_drawn):
//     Fires strictly on the act-bit 1→0 transition — the direct hardware
//     signal that the Weapon25 motor clicked. No threshold fallback.
//     If the hardware doesn't click (motor rate-limited, mode=NONE
//     during cooldown gate, or trigger yanked through the zone faster
//     than act-bit sampling catches) — no shot either. The strict
//     click ⇔ shot pairing is exactly what the user asked for.
//
//   Threshold path (auto-fire, click-less profile, non-DualSense, or
//   weapon_drawn=false — i.e. bare-hand melee):
//     Classic rising-edge: R2 ≤ reset_threshold arms, R2 > fire_threshold
//     fires. Auto-fire ignores the rearm flag and fires while held.
//
// During the running-shot cooldown (game.cpp forces mode=NONE while
// `STATE_MOVEING && Timer1 > 15`), the hardware effect is off so
// presses produce neither click nor shot — keyboard-like silent
// refusal. Gate lifts one tick early so the NONE→AIM_GUN HID packet
// has time to propagate before the game starts accepting shots again.
WeaponFireDecision weapon_feel_evaluate_fire(int32_t current_weapon, int r2, int l2, bool weapon_drawn, bool mag_empty)
{
    WeaponFireDecision out = { false, false };

    const WeaponFeelProfile* p = weapon_feel_get_profile(current_weapon);

    const auto now = std::chrono::steady_clock::now();

    // ---- R2 fire channel ----

    // Rising-edge rearm for the threshold path — R2 dipping at or
    // below reset_threshold counts as a "release" and rearms the
    // rising-edge detector so the next press fires.
    const bool now_released = r2 <= p->reset_threshold;
    if (now_released) {
        s_r2_armed = true;
        // Physical trigger release also clears the AK47 reload gate
        // (set on HAD_TO_CHANGE_CLIP in person.cpp). Done here rather
        // than in continue_firing because the act-bit reload-press
        // path only sets PUNCH on the rising-edge tick, so a PUNCH-low
        // check clears prematurely while the trigger is still held.
        extern void input_actions_clear_ak47_reload_gate();
        input_actions_clear_ak47_reload_gate();
    }
    const int prev_r2_snapshot = s_prev_r2;
    s_prev_r2 = r2;

    // Reload-press mode: auto-fire weapons with a configured reload
    // click (AK47) behave like a single-shot pistol for the fire-
    // detection path while the magazine is empty. This synchronises
    // the game-side "reload fire" event with the hardware Weapon25
    // click point — both fire at reload_click_end_zone instead of
    // the Machine effect's start zone. Without this, PUNCH triggers
    // at the Machine start (r2=112), the game immediately processes
    // HAD_TO_CHANGE_CLIP and stops the effect, the trigger never
    // reaches the Weapon25 end (r2=168), and the reload happens
    // before the click is felt.
    const bool reload_press_mode = mag_empty && p->auto_fire && p->reload_click_strength != 0;

    const TriggerEffectType eff_effect = reload_press_mode ? TriggerEffectType::Weapon : p->trigger_effect;
    const uint8_t eff_end_zone = reload_press_mode ? p->reload_click_end_zone : p->trigger_end_zone;
    const uint8_t eff_strength = reload_press_mode ? p->reload_click_strength : p->trigger_strength;
    const bool eff_auto = reload_press_mode ? false : p->auto_fire;

    // Pick fire-detection path. The act-bit path requires all of:
    //   * DualSense device (act bit is a DualSense-specific signal).
    //   * Profile declares a Weapon25 click (trigger_strength > 0).
    //   * Not auto-fire (auto-fire holds instead of edge-triggering).
    //   * Weapon is currently drawn — gamepad.cpp only turns the
    //     Weapon25 effect on when the player has a gun out. With no
    //     gun drawn (bare-hand punch via R2) the hardware emits no
    //     click, so the adaptive-trigger path would block fire
    //     forever. Threshold fallback handles bare-hand punch.
    // Note: SPECIAL_NONE is registered with the pistol profile, so we
    // CANNOT distinguish bare-hand from pistol by current_weapon
    // alone — the caller passes weapon_drawn (FLAG_PERSON_GUN_OUT).
    const bool device_is_ds = (gamepad_get_device_type() == INPUT_DEVICE_DUALSENSE);
    const bool weapon_has_click = (eff_effect == TriggerEffectType::Weapon && eff_strength != 0);
    const bool use_act_path = device_is_ds && weapon_has_click && !eff_auto && weapon_drawn;

    if (use_act_path) {
        // R2-position click detection: press is registered when the
        // physical trigger crosses the upper edge of the Weapon25
        // click zone going up. Fired PUNCH is unconditional — if the
        // game is still in a post-shot cooldown it gets rejected
        // game-side silently, matching keyboard-during-cooldown.
        // zone_upper_r2 is derived from the profile so retuning the
        // Weapon25 zone or adding a new weapon doesn't need
        // hand-calibrated thresholds.
        const int zone_upper_r2 = (int)eff_end_zone * R2_UNITS_PER_ZONE;
        const bool rose_past_upper = (prev_r2_snapshot < zone_upper_r2) && (r2 >= zone_upper_r2);
        if (rose_past_upper) {
            s_r2_armed = false;
            out.shoot = true;
        }
    } else {
        // Threshold fallback — auto-fire, click-less weapons, or
        // non-DS. Plain rising-edge: r2 crosses fire_threshold while
        // armed (or auto-fire ignores the armed gate). Rate limiting
        // is game's Timer1.
        if (r2 > p->fire_threshold) {
            if (eff_auto || s_r2_armed) {
                out.shoot = true;
                s_r2_armed = false;
            }
        }
    }

    // ---- L2 kick channel — same plain rising-edge, never auto-fires ----

    const bool l2_released = l2 <= p->reset_threshold;
    if (l2_released)
        s_l2_armed = true;
    s_prev_l2 = l2;

    if (l2 > p->fire_threshold && s_l2_armed) {
        out.kick = true;
        s_l2_armed = false;
    }

    return out;
}
