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
//   2. Adaptive trigger (DualSense Weapon25 click) — physical click on R2
//      at a per-weapon zone/amplitude. The mode state machine lives in
//      gamepad.cpp; this module just provides the parameters via the
//      WeaponFeelProfile and exposes weapon_feel_is_r2_armed() for any
//      future armed-gate approach.
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
// Current fire-detection design (as of 2026-04-17)
// ===========================================================================
//
// Two paths share one evaluator, picked at runtime per weapon+device:
//
//   * ACT-BIT path — used when the device is a DualSense AND the
//     weapon's profile has an adaptive click (trigger_strength > 0)
//     AND it's single-shot AND the weapon is currently drawn (the
//     caller passes FLAG_PERSON_GUN_OUT as `weapon_drawn`). The
//     DualSense trigger motor reports an "effect active" bit (act) in
//     the input report: act=1 while the trigger is inside the
//     Weapon25 resistance zone, act=0 otherwise. The 1→0 edge fires
//     once per physical click. We consult
//     gamepad_get_right_trigger_effect_active() for act and fire on
//     the edge with r2 > reset_threshold (high r2 disambiguates a
//     click from a release inside the zone). The hardware naturally
//     enforces one click per press cycle — no PC-side armed gate,
//     no wall-clock timers. Game Timer1 still rate-limits.
//
//   * THRESHOLD fallback — used for auto-fire weapons (AK47), click-less
//     profiles (shotgun today), bare-hand melee (no gun out), and all
//     non-DualSense devices (Xbox, keyboard-as-gamepad). Plain
//     rising-edge: r2 ≤ reset_threshold arms, r2 > fire_threshold
//     fires. Auto-fire ignores the armed gate. Same code that used to
//     run for everyone.
//
// Why two paths: on DualSense the act bit is the direct hardware
// signal "the trigger just clicked", so fire and click are perfectly
// synchronised by construction. Threshold-based detection on r2
// position was always an empirical guess at where the click point sat
// — it desynchronises whenever the player's finger moves faster or
// slower than the motor does. Since the full act report is only
// produced for full-name Weapon25/Feedback/Vibration/etc. effects
// (not Simple/Limited variants), the path requires trigger_strength>0
// AND that gamepad.cpp is actually sending Weapon25 (which it is,
// via the AIM_GUN trigger mode).
//
// Cooldown gating: gamepad.cpp forces mode=NONE during a post-fire
// window so the hardware doesn't click on presses the game will refuse.
// Two mechanisms work together:
//
//   * Running-shot Timer1 gate (in game.cpp): when the player is in
//     STATE_MOVEING with Timer1 > 15 (one tick's worth of decrement),
//     on_cooldown=true → weapon_ready=false → mode=NONE. The "> 15"
//     pre-releases the gate one tick early so the NONE→AIM_GUN HID
//     packet has time to propagate (~30 ms RTT) before Timer1
//     actually reaches 0 and the game accepts the next shot.
//   * Immediate mode=NONE on the fire frame (weapon_feel_on_shot_fired
//     calls gamepad_triggers_lockout(0)): sends the NONE HID packet
//     right away so a rapid re-press within the HID RTT window
//     doesn't land on a still-active motor. Without this the first
//     ~30 ms after a shot could still produce a phantom click.
//
// The act-bit path fires strictly on the hardware act-edge — no
// threshold fallback. A threshold fallback was tried to cover rapid
// taps the motor can't keep up with, but it re-introduced shots
// without clicks in the HID-latency window around mode transitions.
// The accepted trade-off: if a fast tap bypasses the motor's ability
// to click, neither click nor shot happens. Every shot is synchronised
// with a tactile click — the user's explicit requirement.
//
// Why not read Person::Timer1 for standing-shot too: Timer1 only
// decrements in fn_person_moveing (STATE_MOVEING). Outside that state
// it freezes and can't be used as a fire-rate signal. The game itself
// doesn't rate-limit standing shots via Timer1 either — set_person_shoot
// fires on every accepted press without a Timer1 check. So there's
// nothing for the adaptive trigger to sync to on standing, and mode=AIM_GUN
// stays active continuously: each press → click → shot.
//
// History of dead-ends we already walked (armed gate, release-settle,
// post-fire cooldown, split mode/fire lockout, MIN_PRESS_DURATION)
// is in the devlog section "Подходы которые пробовали". Do not reinvent
// those without re-reading why each broke R1/R2/R3.
//
// ===========================================================================
// Debug logging
// ===========================================================================
//
// Everything interesting is logged via weapon_feel_debug_log() into
// weapon_feel_debug.log in the exe directory. File is truncated on
// each run (open with "w"), timestamps are ms since first log call,
// each line is flushed — so the last line is never lost on crash or
// abort. Logged events:
//
//   DIP r2=X              — R2 crossed downward past reset_threshold
//                           (rising-edge arms, release cycle starts)
//   RISE-past-reset r2=X  — R2 crossed upward past reset_threshold
//                           (press phase starts)
//   FIRE-CLICK r2=X       — act-bit path: hardware click detected
//                           (act 1→0 with r2>reset), fire emitted
//   act 1->0 release r2=X — act-bit path: edge seen but r2 low, treated
//                           as release-without-click (no fire)
//   FIRE r2=X             — threshold path: auto-fire / non-DS fire
//   FIRE-NOARM r2=X       — threshold path: R2 past fire_threshold but
//                           not armed (no deep release since last shot)
//   L2-DIP / KICK         — same for L2 kick channel (threshold only)
//   tick r2=... act=...   — per-tick state snapshot, ONLY when R2 value
//                           actually changes (so the log doesn't get
//                           spammed when the player sits idle)
//   ON_SHOT_FIRED         — callback from game side: actually_fire_gun
//                           accepted the shot (Timer1 was 0)
//
// The debug log is currently left in production code deliberately — the
// user has not given permission to remove it yet. When they do, remove
// every weapon_feel_debug_log call and the g_weapon_feel_log machinery.
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
#include "assets/sound_id.h"
#include "assets/sound_id_globals.h"
#include "things/items/special.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <vector>

// ===========================================================================
// Debug logger
// ===========================================================================

static FILE* g_weapon_feel_log = nullptr;
static std::chrono::steady_clock::time_point g_weapon_feel_log_start;

void weapon_feel_debug_log(const char* fmt, ...)
{
    if (!g_weapon_feel_log) {
        g_weapon_feel_log = std::fopen("weapon_feel_debug.log", "w");
        g_weapon_feel_log_start = std::chrono::steady_clock::now();
        if (g_weapon_feel_log) {
            std::fprintf(g_weapon_feel_log, "=== weapon_feel debug log ===\n");
            std::fflush(g_weapon_feel_log);
        }
    }
    if (!g_weapon_feel_log) return;
    const auto now = std::chrono::steady_clock::now();
    const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - g_weapon_feel_log_start).count();
    std::fprintf(g_weapon_feel_log, "[%6lldms] ", (long long)ms);
    va_list args;
    va_start(args, fmt);
    std::vfprintf(g_weapon_feel_log, fmt, args);
    va_end(args);
    std::fputc('\n', g_weapon_feel_log);
    std::fflush(g_weapon_feel_log);
}

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
constexpr int WEAPON25_ZONE_COUNT    = 9;
constexpr int R2_UNITS_PER_ZONE      = 256 / WEAPON25_ZONE_COUNT; // = 28

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
constexpr uint8_t DEFAULT_FIRE_THRESHOLD_R2  = 200;
constexpr uint8_t DEFAULT_RESET_THRESHOLD_R2 = 80;

struct HapticEnvelope {
    std::vector<uint8_t> samples; // 0..255, one per ENV_STEP_SECONDS window
    bool loaded = false;
};

struct ProfileEntry {
    WeaponFeelProfile profile;
    HapticEnvelope    envelope;
};

std::unordered_map<int32_t, ProfileEntry> s_profiles;
bool s_initialized = false;

// Default fallback for weapons with no registered profile. Pistol-like
// rising-edge fire with no haptic and no adaptive trigger click.
const WeaponFeelProfile k_default_profile = {
    /*haptic_wave_id*/    0,
    /*haptic_gain*/       0.0f,
    /*haptic_ceiling*/    0,
    /*haptic_max_seconds*/0.0f,
    /*trigger_start_zone*/0,
    /*trigger_end_zone*/  0,
    /*trigger_strength*/  0,   // 0 = no adaptive click
    /*fire_threshold*/    200,
    /*reset_threshold*/   80,
    /*auto_fire*/         false,
    /*fire_cooldown_ms*/  0,
};

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
    if (frame_count == 0 || channels <= 0 || sample_rate <= 0) return;

    const size_t window_frames = static_cast<size_t>(
        static_cast<float>(sample_rate) * ENV_STEP_SECONDS);
    if (window_frames == 0) return;

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
        if (v < 0.0f) v = 0.0f;
        if (v > ceiling_f) v = ceiling_f;
        out->samples[w] = static_cast<uint8_t>(v);
    }
}

bool build_envelope_from_wave(const WeaponFeelProfile& p, HapticEnvelope* out)
{
    if (p.haptic_wave_id == 0) return false;

    const char* fname = sound_list[p.haptic_wave_id];
    if (!fname || std::strcmp(fname, "NULL.wav") == 0) return false;

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

bool   s_playing = false;
float  s_play_time = 0.0f;
size_t s_prev_index = 0;
const HapticEnvelope* s_active_env = nullptr;

std::chrono::steady_clock::time_point s_last_tick;
bool s_tick_initialized = false;

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

// Previous-tick DualSense trigger effect-active bit for R2. Used by the
// act-bit path to detect the 1→0 edge that marks a physical trigger
// event (click at the top of the zone, or release from inside the zone).
bool s_prev_act_r2 = false;

// Timer1-compensation buffer (2026-04-18 design).
//
// When a gamepad click happens during the game's Timer1 cooldown, we fire
// the shot IMMEDIATELY rather than deferring it. The remaining Timer1
// (what would have been the rest of the cooldown) is stored here and
// added on top of the fresh Timer1=140 at the next set_person_running_shoot,
// so the average fire rate matches the natural Timer1 cadence exactly —
// early clicks borrow against future cooldown time. Keyboard input does
// not go through weapon_feel_evaluate_fire, so compensation stays zero
// for keyboard shots and the original Timer1>0 reject path keeps held-down
// keys from spamming.
// Pending-shot buffer. Click detection is driven strictly by physical
// r2 crossing, independent of mode state, so a press during any
// cooldown (running Timer1 or standing animation) still registers.
// If the game isn't ready to accept the shot yet, the click is held as
// pending and emitted the instant the cooldown clears. Cancelled if
// the player leaves a firing-capable state before then (jump, cutscene,
// holster) so a queued click doesn't auto-fire in an unrelated state.
bool s_in_cooldown            = false;
bool s_weapon_ready_for_fire  = false;
bool s_pending_shot           = false;

// Wall-clock end of the post-fire cooldown window. game.cpp consults
// weapon_feel_is_in_fire_cooldown() to force mode=NONE during this
// window so the hardware doesn't emit clicks while the game is still
// in its per-weapon fire-rate gate. Set by weapon_feel_on_shot_fired
// based on the active profile's fire_cooldown_ms. Initialised deep
// in the past so the check returns false until the first real shot.
std::chrono::steady_clock::time_point s_fire_cooldown_end =
    std::chrono::steady_clock::now() - std::chrono::seconds(10);

// Pure debug log / crossing detection state — none of this gates fire.
// s_prev_r2 / s_prev_l2 hold the previous tick's analog values so we
// can detect downward and upward crossings of reset_threshold (for
// DIP / RISE-past-reset log events).
// s_release_time / s_last_fire_time / s_had_fire are used only to
// compute the "since_rel" and "since_fire" fields in the debug log
// so a human reading the log can see timing between events at a glance.
std::chrono::steady_clock::time_point s_release_time =
    std::chrono::steady_clock::now() - std::chrono::seconds(10);
std::chrono::steady_clock::time_point s_last_fire_time =
    std::chrono::steady_clock::now() - std::chrono::seconds(10);
bool s_had_fire = false;
int  s_prev_r2 = 0;
int  s_prev_l2 = 0;

// Shot history ring buffer for the on-screen "fire rate" diagnostic. Each
// real shot (as reported by weapon_feel_on_shot_fired) pushes an entry with
// the wall-clock timestamp and the input device active at that moment.
// weapon_feel_format_shot_history turns this into a single line for display.
struct ShotRecord {
    std::chrono::steady_clock::time_point time;
    InputDeviceType device;
    bool valid;
};
constexpr size_t kShotHistorySize = 16;
ShotRecord s_shot_history[kShotHistorySize] = {};
size_t s_shot_history_head = 0;

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

    // AK47 — auto-fire, short per-shot kick. haptic_max_seconds < cooldown
    // between auto-fire shots, otherwise envelopes merge into buzz.
    WeaponFeelProfile ak47 = {
        /*haptic_wave_id*/    S_AK47_BURST,
        /*haptic_gain*/       0.4f,
        /*haptic_ceiling*/    90,
        /*haptic_max_seconds*/0.04f,
        /*trigger_start_zone*/0,
        /*trigger_end_zone*/  0,
        /*trigger_strength*/  0,
        /*fire_threshold*/    DEFAULT_FIRE_THRESHOLD_R2,
        /*reset_threshold*/   DEFAULT_RESET_THRESHOLD_R2,
        /*auto_fire*/         true,
        /*fire_cooldown_ms*/  0,   // no adaptive click → mode stays NONE anyway
    };
    // Pistol — single-shot light pop. Rising-edge fire.
    //
    // fire_cooldown_ms=0: running and standing fire both use game-side
    // state signals for cooldown (Timer1 + pre-release for running,
    // SUB_STATE_SHOOT_GUN for standing), not wall-clock timers, so the
    // compensation / fire-gate logic stays robust to per-weapon tuning
    // and tick-rate changes.
    WeaponFeelProfile pistol = {
        /*haptic_wave_id*/    S_PISTOL_SHOT,
        /*haptic_gain*/       0.05f,
        /*haptic_ceiling*/    18,
        /*haptic_max_seconds*/0.035f,
        /*trigger_start_zone*/4,
        /*trigger_end_zone*/  6,
        /*trigger_strength*/  5,
        /*fire_threshold*/    DEFAULT_FIRE_THRESHOLD_R2,
        /*reset_threshold*/   DEFAULT_RESET_THRESHOLD_R2,
        /*auto_fire*/         false,
        /*fire_cooldown_ms*/  0,
    };
    // Shotgun — heavy slam with longer decay.
    WeaponFeelProfile shotgun = {
        /*haptic_wave_id*/    S_SHOTGUN_SHOT,
        /*haptic_gain*/       0.55f,
        /*haptic_ceiling*/    140,
        /*haptic_max_seconds*/0.09f,
        /*trigger_start_zone*/0,
        /*trigger_end_zone*/  0,
        /*trigger_strength*/  0,
        /*fire_threshold*/    DEFAULT_FIRE_THRESHOLD_R2,
        /*reset_threshold*/   DEFAULT_RESET_THRESHOLD_R2,
        /*auto_fire*/         false,
        /*fire_cooldown_ms*/  0,   // no adaptive click → mode stays NONE anyway
    };

    s_initialized = true;
    weapon_feel_register(SPECIAL_AK47,    ak47);
    weapon_feel_register(SPECIAL_GUN,     pistol);
    weapon_feel_register(SPECIAL_NONE,    pistol); // SpecialUse==null reads SpecialType as 0
    weapon_feel_register(SPECIAL_SHOTGUN, shotgun);

    s_playing = false;
    s_active_env = nullptr;
    s_prev_index = 0;
    s_tick_initialized = false;
    s_r2_armed = true;
    s_l2_armed = true;
    s_prev_act_r2 = false;
    s_in_cooldown = false;
    s_weapon_ready_for_fire = false;
    s_pending_shot = false;
    s_fire_cooldown_end = std::chrono::steady_clock::now() - std::chrono::seconds(10);
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
    entry.envelope = HapticEnvelope{};
    if (s_initialized && profile.haptic_wave_id != 0) {
        build_envelope_from_wave(profile, &entry.envelope);
    }
}

const WeaponFeelProfile* weapon_feel_get_profile(int32_t special_type)
{
    auto it = s_profiles.find(special_type);
    if (it != s_profiles.end()) return &it->second.profile;
    return &k_default_profile;
}

// ===========================================================================
// Haptic playback
// ===========================================================================

void weapon_feel_on_shot_fired(int32_t special_type)
{
    weapon_feel_debug_log("ON_SHOT_FIRED special=%d", (int)special_type);

    // Record the shot into the history ring buffer for the on-screen fire-rate
    // diagnostic. Captured here (not inside evaluate_fire) because this callback
    // fires only on REAL game-accepted shots — same code path for keyboard and
    // gamepad, so intervals are directly comparable.
    s_shot_history[s_shot_history_head].time   = std::chrono::steady_clock::now();
    s_shot_history[s_shot_history_head].device = gamepad_get_device_type();
    s_shot_history[s_shot_history_head].valid  = true;
    s_shot_history_head = (s_shot_history_head + 1) % kShotHistorySize;

    // Cooldown is tracked regardless of device type — it's a simple wall-clock
    // window that gamepad.cpp consults via weapon_feel_is_in_fire_cooldown().
    // Non-DualSense devices just don't have adaptive triggers to gate, so the
    // cooldown is harmless (and the envelope playback below is gated on
    // DualSense explicitly).
    const WeaponFeelProfile* p = weapon_feel_get_profile(special_type);
    if (p->fire_cooldown_ms > 0) {
        s_fire_cooldown_end = std::chrono::steady_clock::now() +
            std::chrono::milliseconds(p->fire_cooldown_ms);
    }

    if (gamepad_get_device_type() != INPUT_DEVICE_DUALSENSE) return;

    auto it = s_profiles.find(special_type);
    if (it == s_profiles.end()) return;
    const ProfileEntry& entry = it->second;

    // Force mode=NONE on fire frame for single-shot weapons so the
    // motor doesn't emit a phantom click in the HID RTT window before
    // the next gamepad_triggers_update applies the post-fire cooldown
    // gate. Auto-fire weapons keep the motor armed across bursts.
    if (!entry.profile.auto_fire) {
        gamepad_triggers_lockout(0);
    }

    if (!entry.envelope.loaded) return;

    // Restart envelope on each shot — matches sound retrigger behaviour for
    // auto-fire bursts.
    s_active_env = &entry.envelope;
    s_play_time = 0.0f;
    s_prev_index = 0;
    s_playing = true;
    // Force dt=0 on the next tick so we start from sample 0 rather than
    // jumping into stale time from a previous idle session.
    s_tick_initialized = false;
}

uint8_t weapon_feel_tick_haptic(bool* out_active)
{
    if (out_active) *out_active = false;
    if (!s_playing || !s_active_env) return 0;

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
        if (s_active_env->samples[i] > peak) peak = s_active_env->samples[i];
    }
    s_prev_index = curr_index + 1;

    if (out_active) *out_active = true;
    return peak;
}

void weapon_feel_stop_haptic()
{
    s_playing = false;
    s_active_env = nullptr;
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
WeaponFireDecision weapon_feel_evaluate_fire(int32_t current_weapon, int r2, int l2, bool weapon_drawn)
{
    WeaponFireDecision out = { false, false };

    const WeaponFeelProfile* p = weapon_feel_get_profile(current_weapon);

    const auto now = std::chrono::steady_clock::now();

    // ---- R2 fire channel ----

    // Per-tick state snapshot for debug — only when R2 changes. Includes
    // act bit so the log tells the whole story of a press cycle.
    const bool act_now = gamepad_get_right_trigger_effect_active();
    if (r2 != s_prev_r2) {
        const auto since_release = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - s_release_time).count();
        const auto since_fire = s_had_fire
            ? std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_fire_time).count()
            : -1LL;
        weapon_feel_debug_log("tick r2=%d prev=%d armed=%d act=%d since_rel=%lldms since_fire=%lldms",
            r2, s_prev_r2, s_r2_armed ? 1 : 0, act_now ? 1 : 0, since_release, since_fire);
    }

    // Detect downward crossing of reset_threshold — this is when
    // rising-edge rearms (threshold path only). Reset s_release_time
    // for log timestamps.
    const bool now_released = r2 <= p->reset_threshold;
    const bool was_released = s_prev_r2 <= p->reset_threshold;
    if (now_released && !was_released) {
        s_release_time = now;
        weapon_feel_debug_log("DIP r2=%d prev=%d", r2, s_prev_r2);
    }
    if (!now_released && was_released) {
        weapon_feel_debug_log("RISE-past-reset r2=%d prev=%d armed=%d",
            r2, s_prev_r2, s_r2_armed ? 1 : 0);
    }
    if (now_released) s_r2_armed = true;
    const int prev_r2_snapshot = s_prev_r2;
    s_prev_r2 = r2;

    // Pick fire-detection path. The act-bit path requires all of:
    //   * DualSense device (act bit is a DualSense-specific signal).
    //   * Profile declares a Weapon25 click (trigger_strength > 0).
    //   * Not auto-fire (auto-fire holds instead of edge-triggering).
    //   * Weapon is currently drawn — because gamepad.cpp only turns the
    //     Weapon25 effect on when the player has a gun out. With no gun
    //     drawn (bare-hand punch via R2) hardware emits no click, so
    //     the act bit never flips and the act-bit path would block fire
    //     forever. Threshold fallback handles bare-hand punch correctly.
    // Note: SPECIAL_NONE is registered with the pistol profile, so we
    // CANNOT distinguish bare-hand from pistol by current_weapon alone —
    // the caller passes weapon_drawn (FLAG_PERSON_GUN_OUT) for this.
    const bool device_is_ds     = (gamepad_get_device_type() == INPUT_DEVICE_DUALSENSE);
    const bool weapon_has_click = p->trigger_strength != 0;
    const bool use_act_path     = device_is_ds && weapon_has_click && !p->auto_fire && weapon_drawn;

    if (use_act_path) {
        // R2-position click detection: a press is registered when the
        // physical trigger position crosses the upper edge of the
        // Weapon25 click zone going up. Detection is independent of the
        // hardware mode state — clicks are captured even if mode=NONE
        // during the cooldown animation, so the player's press isn't
        // silently dropped just because the motor happens to be off.
        //
        // zone_upper_r2 is derived from the profile (end_zone ×
        // R2_UNITS_PER_ZONE) so retuning the Weapon25 zone or adding a
        // new weapon doesn't require hand-calibrated thresholds.
        //
        // After a press is registered, routing depends on game state:
        //   * Standing animation still playing → queue as pending. The
        //     shot flushes to PUNCH automatically the moment the
        //     animation finishes. Pending is cancelled if the player
        //     loses fire readiness (jump, cutscene, holster) before
        //     that happens — we don't want an auto-fire 1 s later in
        //     an unrelated state.
        //   * Otherwise (running pre-release window, or stand-idle):
        //     fire immediately. Running case also captures Timer1 as
        //     compensation — see set_person_running_shoot.
        //
        // The act bit is still logged (ZONE_ENTER / ZONE_EXIT below)
        // to keep hardware behaviour visible in diagnostics.
        const int zone_upper_r2 = (int)p->trigger_end_zone * R2_UNITS_PER_ZONE;
        const bool rose_past_upper =
            (prev_r2_snapshot < zone_upper_r2) && (r2 >= zone_upper_r2);
        if (rose_past_upper) {
            s_r2_armed = false;
            s_last_fire_time = now;
            s_had_fire = true;
            if (s_in_cooldown) {
                s_pending_shot = true;
                weapon_feel_debug_log(
                    "FIRE-PENDING r2=%d prev=%d (crossed zone_upper=%d) cooldown=1",
                    r2, prev_r2_snapshot, zone_upper_r2);
            } else {
                out.shoot = true;
                weapon_feel_debug_log(
                    "FIRE r2=%d prev=%d (crossed zone_upper=%d)",
                    r2, prev_r2_snapshot, zone_upper_r2);
            }
        }

        // Pending-shot housekeeping. Flush when the cooldown clears and
        // the player is still fire-capable. Cancel if readiness is lost
        // (any pending should not auto-fire after jump / cutscene /
        // holster interrupts the shot).
        if (s_pending_shot) {
            if (!s_weapon_ready_for_fire) {
                s_pending_shot = false;
                weapon_feel_debug_log("FIRE-PENDING CANCELLED (not ready)");
            } else if (!s_in_cooldown) {
                out.shoot = true;
                s_pending_shot = false;
                weapon_feel_debug_log("FIRE-PENDING FLUSH");
            }
        }
        // Debug-only act-edge logging — useful for verifying the motor
        // fires in sync with the r2-crossing detector above.
        if (!s_prev_act_r2 && act_now) {
            weapon_feel_debug_log("ZONE_ENTER r2=%d", r2);
        }
        if (s_prev_act_r2 && !act_now) {
            weapon_feel_debug_log("ZONE_EXIT r2=%d", r2);
        }
    } else {
        // Threshold fallback — auto-fire, click-less weapons, or non-DS.
        // Plain rising-edge: r2 crosses fire_threshold while armed (or
        // auto-fire ignores the armed gate). Rate limiting is game's
        // Timer1.
        if (r2 > p->fire_threshold) {
            if (p->auto_fire || s_r2_armed) {
                out.shoot = true;
                s_r2_armed = false;
                s_last_fire_time = now;
                s_had_fire = true;
                weapon_feel_debug_log("FIRE r2=%d (threshold path)", r2);
            } else {
                static int s_last_noarm_r2 = -1;
                if (s_last_noarm_r2 != r2) {
                    weapon_feel_debug_log("FIRE-NOARM r2=%d (no deep release since last shot)", r2);
                    s_last_noarm_r2 = r2;
                }
            }
        }
    }

    s_prev_act_r2 = act_now;

    // ---- L2 kick channel — same plain rising-edge, never auto-fires ----

    const bool l2_released = l2 <= p->reset_threshold;
    const bool l2_was_released = s_prev_l2 <= p->reset_threshold;
    if (l2_released && !l2_was_released) {
        weapon_feel_debug_log("L2-DIP l2=%d", l2);
    }
    if (l2_released) s_l2_armed = true;
    s_prev_l2 = l2;

    if (l2 > p->fire_threshold && s_l2_armed) {
        out.kick = true;
        s_l2_armed = false;
        weapon_feel_debug_log("KICK l2=%d", l2);
    }

    return out;
}

void weapon_feel_fire_reset()
{
    s_r2_armed = true;
    s_l2_armed = true;
    s_had_fire = false;
    s_prev_r2 = 0;
    s_prev_l2 = 0;
    s_prev_act_r2 = false;
    s_in_cooldown = false;
    s_weapon_ready_for_fire = false;
    s_pending_shot = false;
    s_fire_cooldown_end = std::chrono::steady_clock::now() - std::chrono::seconds(10);
}

void weapon_feel_set_game_state(bool in_cooldown, bool weapon_ready)
{
    s_in_cooldown = in_cooldown;
    s_weapon_ready_for_fire = weapon_ready;
}

// NOTE: kept in the public API for future use but currently NOT consulted
// by gamepad.cpp — the mode state machine in gamepad_triggers_update
// unconditionally enables AIM_GUN when the weapon is ready, regardless
// of the rising-edge rearm state. This avoids the HID-race class of
// bugs at the cost of occasional "click during cooldown" (acceptable
// per requirement R4 in the devlog). Any future armed-gate approach
// should re-consult this function.
bool weapon_feel_is_r2_armed(int32_t current_weapon)
{
    const WeaponFeelProfile* p = weapon_feel_get_profile(current_weapon);
    return p->auto_fire || s_r2_armed;
}

bool weapon_feel_is_in_fire_cooldown()
{
    return std::chrono::steady_clock::now() < s_fire_cooldown_end;
}

void weapon_feel_format_shot_history(char* out, size_t out_size)
{
    if (!out || out_size == 0) return;
    out[0] = '\0';

    // Collect valid records in chronological order. s_shot_history_head
    // points at the next slot to overwrite, i.e. the oldest entry.
    ShotRecord recs[kShotHistorySize];
    size_t n = 0;
    for (size_t i = 0; i < kShotHistorySize; i++) {
        const size_t idx = (s_shot_history_head + i) % kShotHistorySize;
        if (s_shot_history[idx].valid) recs[n++] = s_shot_history[idx];
    }

    if (n == 0) {
        std::snprintf(out, out_size, "fire gaps: (no shots yet)");
        return;
    }

    size_t written = std::snprintf(out, out_size, "gaps ms:");
    if (written >= out_size) return;

    // Pairwise intervals between consecutive shots. The tag reflects the
    // device active at the moment of the LATER shot in each pair — that's
    // what "produced" this particular gap.
    for (size_t i = 1; i < n; i++) {
        const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            recs[i].time - recs[i-1].time).count();
        const char* tag = "?";
        switch (recs[i].device) {
        case INPUT_DEVICE_KEYBOARD_MOUSE: tag = "kb"; break;
        case INPUT_DEVICE_DUALSENSE:      tag = "ds"; break;
        case INPUT_DEVICE_XBOX:           tag = "xb"; break;
        default: break;
        }
        const int wrote = std::snprintf(out + written, out_size - written,
                                        " [%s]%lld", tag, ms);
        if (wrote <= 0 || (size_t)wrote >= out_size - written) break;
        written += (size_t)wrote;
    }
}

namespace {

// Helper: walk chronologically through the history, pull records whose
// device matches, take the last up-to `max_records`, return count of
// records copied into `out`. Caller uses out[i].time to compute gaps.
size_t collect_recent_for_device(InputDeviceType target,
                                 ShotRecord* out, size_t max_records)
{
    ShotRecord all[kShotHistorySize];
    size_t n = 0;
    for (size_t i = 0; i < kShotHistorySize; i++) {
        const size_t idx = (s_shot_history_head + i) % kShotHistorySize;
        if (s_shot_history[idx].valid && s_shot_history[idx].device == target) {
            all[n++] = s_shot_history[idx];
        }
    }
    const size_t start = n > max_records ? n - max_records : 0;
    const size_t count = n - start;
    for (size_t i = 0; i < count; i++) out[i] = all[start + i];
    return count;
}

// Format "avg <tag>: <Nms> (n=X)" or "avg <tag>: -" if fewer than 2 shots.
// Averages over up to 5 intervals (= 6 records). Returns bytes written
// (excluding null terminator), or 0 on failure.
size_t write_avg_for_device(InputDeviceType target, const char* tag,
                            char* out, size_t out_size)
{
    ShotRecord recs[6];
    const size_t n = collect_recent_for_device(target, recs, 6);
    if (n < 2) {
        const int w = std::snprintf(out, out_size, "avg %s: -", tag);
        return (w > 0 && (size_t)w < out_size) ? (size_t)w : 0;
    }
    long long sum_ms = 0;
    for (size_t i = 1; i < n; i++) {
        sum_ms += std::chrono::duration_cast<std::chrono::milliseconds>(
            recs[i].time - recs[i-1].time).count();
    }
    const long long intervals = (long long)(n - 1);
    const long long avg = sum_ms / intervals;
    const int w = std::snprintf(out, out_size, "avg %s: %lldms (n=%lld)",
                                tag, avg, intervals);
    return (w > 0 && (size_t)w < out_size) ? (size_t)w : 0;
}

} // namespace

void weapon_feel_format_shot_averages(char* out, size_t out_size)
{
    if (!out || out_size == 0) return;
    out[0] = '\0';

    size_t written = write_avg_for_device(INPUT_DEVICE_KEYBOARD_MOUSE, "kb",
                                          out, out_size);
    if (written + 4 < out_size) {
        written += (size_t)std::snprintf(out + written, out_size - written, " | ");
    }
    write_avg_for_device(INPUT_DEVICE_DUALSENSE, "ds",
                         out + written, out_size - written);
}
