#include "engine/input/weapon_feel.h"
#include "engine/input/gamepad.h"
#include "engine/platform/sdl3_bridge.h"
#include "assets/sound_id.h"
#include "assets/sound_id_globals.h"
#include "things/items/special.h"

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
    /*trigger_amplitude*/ 0,   // 0 = no adaptive click
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

bool s_r2_armed = true;
bool s_l2_armed = true;

// Settle window starting at the moment the trigger is released (R2 crosses
// downward past reset_threshold). The hardware Weapon25 effect needs
// physical time to reset its internal click state + for our mode=AIM_GUN
// HID command to propagate over Bluetooth. If the player re-presses past
// fire_threshold before this window expires, it's treated as a "wasted"
// cycle: no shot fires AND the rearm is consumed, so no more shots can
// fire until the player does another proper release. At the same time,
// armed is forced false for is_r2_armed queries so mode=NONE engages
// and the hardware stops emitting clicks. Both subsystems stay in sync.
// Needs to cover: HID RTT (~30ms BT) + physical motor engagement time to
// actually build resistance + click. Observed: 100ms was too short —
// player could still tap fast enough that the motors couldn't physically
// react in time, leading to shots with no felt click. 200ms gives the
// motors full time to engage.
constexpr auto RELEASE_SETTLE = std::chrono::milliseconds(200);

// Post-fire cooldown. The game's Timer1 blocks the actual shot for a
// weapon-specific duration after each fire; during that window our
// evaluate_fire would otherwise report shoot=true (game refuses), but
// mode=AIM_GUN stays active until the next tick and the hardware gets
// to fire a physical click on the press that produced the ignored shot.
// Treating any fire_threshold crossing during this window as a consumed
// rearm (same semantics as a too-fast release-press tap) forces
// mode=NONE early so the hardware stops trying to click. 300ms covers
// the pistol's Timer1 with margin; longer than needed is harmless
// because weapons can't physically fire faster anyway.
constexpr auto POST_FIRE_COOLDOWN = std::chrono::milliseconds(300);
std::chrono::steady_clock::time_point s_last_fire_time =
    std::chrono::steady_clock::now() - std::chrono::seconds(10);
bool s_had_fire = false;

bool is_in_post_fire_cooldown(const WeaponFeelProfile* p)
{
    if (p->auto_fire || !s_had_fire) return false;
    const auto now = std::chrono::steady_clock::now();
    return (now - s_last_fire_time) < POST_FIRE_COOLDOWN;
}
std::chrono::steady_clock::time_point s_release_time =
    std::chrono::steady_clock::now() - std::chrono::seconds(10);
bool s_armed_consumed = false;
int  s_prev_r2 = 0;
int  s_prev_l2 = 0;

bool is_release_settled(const WeaponFeelProfile* p)
{
    if (p->auto_fire) return true;
    const auto now = std::chrono::steady_clock::now();
    return (now - s_release_time) >= RELEASE_SETTLE;
}

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
        /*trigger_start_zone*/7,
        /*trigger_amplitude*/ 2,
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
        /*trigger_start_zone*/7,
        /*trigger_amplitude*/ 2,
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
        /*trigger_start_zone*/7,
        /*trigger_amplitude*/ 2,
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

WeaponFireDecision weapon_feel_evaluate_fire(int32_t current_weapon, int r2, int l2)
{
    const WeaponFeelProfile* p = weapon_feel_get_profile(current_weapon);

    WeaponFireDecision out = { false, false };

    const auto now = std::chrono::steady_clock::now();

    // Detect downward crossing of reset_threshold (trigger just entered
    // the release zone). This is the moment hardware click rearm begins.
    // Start the settle timer and reset the "consumed" flag so this
    // release cycle gets a fresh chance to fire.
    const bool now_released = r2 <= p->reset_threshold;
    const bool was_released = s_prev_r2 <= p->reset_threshold;
    if (now_released && !was_released) {
        s_release_time = now;
        s_armed_consumed = false;
    }
    if (now_released) s_r2_armed = true;
    s_prev_r2 = r2;

    const bool in_cooldown = is_in_post_fire_cooldown(p);

    if (r2 > p->fire_threshold && !s_armed_consumed) {
        if (p->auto_fire) {
            out.shoot = true;
        } else if (s_r2_armed) {
            if (in_cooldown) {
                // Game-side Timer1 will refuse this shot anyway — treat
                // the cycle as consumed so mode→NONE and the hardware
                // doesn't fire an orphan click.
                s_armed_consumed = true;
                s_r2_armed = false;
            } else if (is_release_settled(p)) {
                out.shoot = true;
                s_r2_armed = false;
                s_last_fire_time = now;
                s_had_fire = true;
            } else {
                // Too fast: the player re-pressed past fire_threshold
                // before the hardware had time to reset. Consume the
                // rearm without firing — this cycle is discarded, and
                // is_r2_armed will report false (mode→NONE) so the
                // hardware stops emitting click too.
                s_armed_consumed = true;
                s_r2_armed = false;
            }
        }
    }

    // L2 kick — same logic as R2 with independent state. Never auto-fires.
    static std::chrono::steady_clock::time_point s_l2_release_time =
        std::chrono::steady_clock::now() - std::chrono::seconds(10);
    static bool s_l2_consumed = false;
    const bool l2_now_released = l2 <= p->reset_threshold;
    const bool l2_was_released = s_prev_l2 <= p->reset_threshold;
    if (l2_now_released && !l2_was_released) {
        s_l2_release_time = now;
        s_l2_consumed = false;
    }
    if (l2_now_released) s_l2_armed = true;
    s_prev_l2 = l2;

    if (l2 > p->fire_threshold && !s_l2_consumed && s_l2_armed) {
        if ((now - s_l2_release_time) >= RELEASE_SETTLE) {
            out.kick = true;
            s_l2_armed = false;
        } else {
            s_l2_consumed = true;
            s_l2_armed = false;
        }
    }

    return out;
}

void weapon_feel_fire_reset()
{
    s_r2_armed = true;
    s_l2_armed = true;
    s_armed_consumed = false;
    s_had_fire = false;
    s_prev_r2 = 0;
    s_prev_l2 = 0;
}

bool weapon_feel_is_r2_armed(int32_t current_weapon)
{
    const WeaponFeelProfile* p = weapon_feel_get_profile(current_weapon);
    if (p->auto_fire) return true;
    // Rising-edge weapons: armed only if the rearm flag is set AND the
    // rearm hasn't been consumed AND we're not in a post-fire cooldown.
    // During cooldown mode=NONE engages so the hardware stops emitting
    // clicks that the game would ignore.
    if (s_armed_consumed || !s_r2_armed) return false;
    return !is_in_post_fire_cooldown(p);
}
