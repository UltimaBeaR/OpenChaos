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
//                                        weapon_feel_evaluate_fire(r2, l2)
//                                        to turn analog values into
//                                        discrete PUNCH/KICK input bits
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
// Current fire-detection design (as of 2026-04-15)
// ===========================================================================
//
// THE SIMPLEST POSSIBLE version that satisfies the user's core
// requirements (R1 — natural fire rate, R2 — keyboard-like tap behaviour):
//
//   * Pure rising-edge fire detection: R2 ≤ reset_threshold arms,
//     R2 > fire_threshold fires. No wall-clock cooldown, no settle timer,
//     no consume flag. The game's Timer1 (inside actually_fire_gun) is
//     the single source of rate limiting.
//
//   * Adaptive-trigger mode is kept continuously enabled by gamepad.cpp
//     while the weapon is ready. Mode never flips during gameplay, so
//     there is no Bluetooth HID-round-trip race at every shot.
//
// This design has two known residual issues, documented in detail in
// the devlog file. Summary:
//
//   I1. At VERY rapid tapping (faster than the hardware Weapon25 click
//       rate limit, whose exact value we don't know) some clicks can be
//       physically dropped — the motor can't engage in time. The game
//       still fires. Appears as rare "shot without click" at fast tap.
//       Does NOT happen at normal aimed-firing pace.
//
//   I2. If the player presses R2 during the game's Timer1 cooldown
//       window, the hardware fires a click (mode is AIM_GUN) but the
//       game refuses the shot. Appears as "click without shot" during
//       cooldown. The user explicitly said this is tolerable
//       ("в целом тут тоже ок").
//
// EVERY more elaborate approach we tried broke R1 or R2 or made the
// bugs worse. Read the devlog section "Подходы которые пробовали" for
// the full list (armed gate, release-settle, post-fire cooldown,
// split mode/fire lockout, MIN_PRESS_DURATION). Do NOT re-invent any
// of these without reading why they failed first.
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
//   FIRE r2=X             — fire event emitted to input pipeline
//   FIRE-NOARM r2=X       — R2 past fire_threshold but not armed (no
//                           deep release since last shot)
//   L2-DIP / KICK         — same for L2 kick channel
//   tick r2=...           — per-tick state snapshot, ONLY when R2 value
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
#include "engine/input/weapon_feel_test.h"
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

constexpr float ENV_STEP_SECONDS = 0.005f;

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

// Rising-edge rearm flags — the only state that actually gates fire.
// Set to true when R2/L2 drops to ≤ reset_threshold. Set to false when
// a fire event is emitted. Initial value is true so the first shot
// after gamepad init works without requiring a dummy release first.
bool s_r2_armed = true;
bool s_l2_armed = true;

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
        /*fire_threshold*/    200,
        /*reset_threshold*/   80,
        /*auto_fire*/         true,
    };
    // Pistol — single-shot light pop. Rising-edge fire.
    WeaponFeelProfile pistol = {
        /*haptic_wave_id*/    S_PISTOL_SHOT,
        /*haptic_gain*/       0.05f,
        /*haptic_ceiling*/    18,
        /*haptic_max_seconds*/0.035f,
        /*trigger_start_zone*/4,
        /*trigger_end_zone*/  6,
        /*trigger_strength*/  5,
        /*fire_threshold*/    200,
        /*reset_threshold*/   80,
        /*auto_fire*/         false,
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
        /*fire_threshold*/    200,
        /*reset_threshold*/   80,
        /*auto_fire*/         false,
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
    if (gamepad_get_device_type() != INPUT_DEVICE_DUALSENSE) return;

    auto it = s_profiles.find(special_type);
    if (it == s_profiles.end()) return;
    const ProfileEntry& entry = it->second;

    // Single-shot weapons immediately disarm the Weapon25 click on the fire
    // frame so there's no chance of a stale click sneaking through between
    // the game setting Timer1 and gamepad_triggers_update running. Auto-fire
    // weapons keep the click live for the next shot in the burst.
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

// Simplest stable version: plain rising-edge fire detection with no
// artificial wall-clock timers or consume logic. Rate limiting is
// entirely the responsibility of the game's Timer1 inside
// actually_fire_gun. The adaptive-trigger mode is left continuously
// enabled by gamepad.cpp (no armed gate) so the hardware handles its
// own click state without any HID flipping.
//
// See comments at the top of the "Fire detection state" block for
// the known issues at this level (rapid-tap click drop, click during
// cooldown) and why we settled on this version.
WeaponFireDecision weapon_feel_evaluate_fire(int32_t current_weapon, int r2, int l2)
{
    WeaponFireDecision out = { false, false };

    // Hardware motor delay test suppresses live firing so the character
    // does not actually shoot while the test cycles through delay values.
    if (weapon_feel_test_is_active()) return out;

    const WeaponFeelProfile* p = weapon_feel_get_profile(current_weapon);

    const auto now = std::chrono::steady_clock::now();

    // ---- R2 fire channel ----

    // Per-tick state snapshot for debug — only when R2 changes.
    if (r2 != s_prev_r2) {
        const auto since_release = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - s_release_time).count();
        const auto since_fire = s_had_fire
            ? std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_fire_time).count()
            : -1LL;
        weapon_feel_debug_log("tick r2=%d prev=%d armed=%d since_rel=%lldms since_fire=%lldms",
            r2, s_prev_r2, s_r2_armed ? 1 : 0, since_release, since_fire);
    }

    // Detect downward crossing of reset_threshold — this is when
    // rising-edge rearms. Reset s_release_time for log timestamps.
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
    s_prev_r2 = r2;

    // Fire: plain rising-edge. Either auto-fire (ignores rearm) or
    // rising-edge (requires s_r2_armed). No artificial cooldown — game
    // Timer1 enforces the rate.
    if (r2 > p->fire_threshold) {
        if (p->auto_fire || s_r2_armed) {
            out.shoot = true;
            s_r2_armed = false;
            s_last_fire_time = now;
            s_had_fire = true;
            weapon_feel_debug_log("FIRE r2=%d", r2);
        } else {
            static int s_last_noarm_r2 = -1;
            if (s_last_noarm_r2 != r2) {
                weapon_feel_debug_log("FIRE-NOARM r2=%d (no deep release since last shot)", r2);
                s_last_noarm_r2 = r2;
            }
        }
    }

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
