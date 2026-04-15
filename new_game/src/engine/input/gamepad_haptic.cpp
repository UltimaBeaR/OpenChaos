#include "engine/input/gamepad_haptic.h"
#include "engine/input/gamepad.h"
#include "engine/platform/sdl3_bridge.h"
#include "assets/sound_id.h"
#include "assets/sound_id_globals.h"
#include "things/items/special.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// Envelope resolution: one amplitude sample per 5ms slice of the source WAV.
static constexpr float ENV_STEP_SECONDS = 0.005f;

struct HapticEnvelope {
    std::vector<uint8_t> samples; // 0..255, one per ENV_STEP_SECONDS window
    bool loaded = false;
};

// Per-weapon tuning. Envelope is pre-computed once at init using these
// parameters, so all runtime tuning lives here.
//
// Tuning guide:
//   gain          — overall loudness multiplier (0.0..1.0). Higher = stronger.
//   ceiling       — hard clamp on motor value (0..255). Lower = softer peaks.
//   max_seconds   — envelope truncation. MUST be shorter than the weapon's
//                   auto-fire cooldown so pulses stay discrete (don't overlap
//                   into a continuous buzz).
struct WeaponHapticParams {
    float   gain;
    uint8_t ceiling;
    float   max_seconds;
};

static HapticEnvelope s_ak47_env;
static HapticEnvelope s_pistol_env;
static HapticEnvelope s_shotgun_env;

// Active playback state.
static bool  s_playing = false;
static float s_play_time = 0.0f;   // seconds since current envelope started
static size_t s_prev_index = 0;    // envelope index at previous tick (for MAX window scan)
static const HapticEnvelope* s_active_env = nullptr;

// Wall-clock timing for dt (game tick rate varies, don't trust it).
static std::chrono::steady_clock::time_point s_last_tick;
static bool s_tick_initialized = false;

// ----------------------------------------------------------------------------
// WAV loading + envelope computation
// ----------------------------------------------------------------------------

// Build the filesystem path MFX uses for sound effects. MFX's GetFullName
// prepends "./data/sfx/1622/" — we replicate it locally to avoid touching MFX.
static void build_sfx_path(const char* fname, char* out, size_t out_size)
{
    std::snprintf(out, out_size, "./data/sfx/1622/%s", fname);
}

// Compute an amplitude envelope from raw 16-bit PCM samples.
// Mixes multi-channel to mono by averaging, RMS-windowed, then normalized.
static void compute_envelope(const int16_t* pcm, size_t frame_count, int channels,
                             int sample_rate, const WeaponHapticParams& params,
                             HapticEnvelope* out)
{
    if (frame_count == 0 || channels <= 0 || sample_rate <= 0) return;

    const size_t window_frames = static_cast<size_t>(
        static_cast<float>(sample_rate) * ENV_STEP_SECONDS);
    if (window_frames == 0) return;

    // Cap total envelope length at params.max_seconds so that consecutive
    // auto-fire pulses don't overlap into a continuous buzz.
    const size_t max_frames = static_cast<size_t>(
        static_cast<float>(sample_rate) * params.max_seconds);
    const size_t effective_frames = frame_count < max_frames ? frame_count : max_frames;

    const size_t env_len = (effective_frames + window_frames - 1) / window_frames;
    out->samples.resize(env_len);

    std::vector<float> rms(env_len);

    for (size_t w = 0; w < env_len; w++) {
        const size_t frame_start = w * window_frames;
        const size_t frame_end = std::min(frame_start + window_frames, effective_frames);
        double sum_sq = 0.0;
        size_t count = 0;
        for (size_t f = frame_start; f < frame_end; f++) {
            // Mix channels to mono by averaging.
            int64_t mono = 0;
            for (int c = 0; c < channels; c++) {
                mono += pcm[f * channels + c];
            }
            mono /= channels;
            const double v = static_cast<double>(mono) / 32768.0;
            sum_sq += v * v;
            count++;
        }
        rms[w] = count ? static_cast<float>(std::sqrt(sum_sq / count)) : 0.0f;
    }

    // Normalize to 0..255 with per-weapon gain boost, clamped at ceiling.
    // Skip normalization by peak — we want absolute loudness to drive motor
    // intensity, not relative.
    const float ceiling_f = static_cast<float>(params.ceiling);
    for (size_t w = 0; w < env_len; w++) {
        float v = rms[w] * params.gain * 255.0f;
        if (v < 0.0f) v = 0.0f;
        if (v > ceiling_f) v = ceiling_f;
        out->samples[w] = static_cast<uint8_t>(v);
    }
}

// Load a WAV file and build its envelope. sdl3_load_wav returns interleaved
// 16-bit PCM in little-endian order (SDL converts if needed).
static bool load_envelope_for_wave(uint32_t wave_id, const WeaponHapticParams& params,
                                   HapticEnvelope* out)
{
    extern CBYTE* sound_list[];
    const char* fname = sound_list[wave_id];
    if (!fname || std::strcmp(fname, "NULL.wav") == 0) return false;

    char path[512];
    build_sfx_path(fname, path, sizeof(path));

    SDL3_WavData wav;
    if (!sdl3_load_wav(path, &wav)) {
        std::fprintf(stderr, "[haptic] failed to load WAV for envelope: %s\n", path);
        return false;
    }

    const size_t bytes_per_frame = static_cast<size_t>(wav.channels) * 2; // 16-bit
    const size_t frame_count = wav.size / bytes_per_frame;
    const int16_t* pcm = reinterpret_cast<const int16_t*>(wav.buffer);

    compute_envelope(pcm, frame_count, wav.channels, wav.freq, params, out);
    out->loaded = !out->samples.empty();

    sdl3_free_wav(wav.buffer);
    return out->loaded;
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

void gamepad_haptic_init()
{
    // AK47 — fast auto-fire, short per-shot kick. User-tuned, confirmed good.
    const WeaponHapticParams ak47_params  = { 0.4f,  90, 0.04f };
    // Pistol — light single-shot pop. Tuned to feel slightly lighter than AK47.
    const WeaponHapticParams pistol_params = { 0.05f, 18, 0.035f };
    // Shotgun — heavy slam with longer decay. Bigger peak, longer envelope.
    const WeaponHapticParams shotgun_params = { 0.55f, 140, 0.09f };

    load_envelope_for_wave(S_AK47_BURST,   ak47_params,    &s_ak47_env);
    load_envelope_for_wave(S_PISTOL_SHOT,  pistol_params,  &s_pistol_env);
    load_envelope_for_wave(S_SHOTGUN_SHOT, shotgun_params, &s_shotgun_env);

    s_tick_initialized = false;
    s_playing = false;
    s_active_env = nullptr;
    s_prev_index = 0;
}

void gamepad_haptic_shutdown()
{
    s_ak47_env = HapticEnvelope{};
    s_pistol_env = HapticEnvelope{};
    s_shotgun_env = HapticEnvelope{};
    s_playing = false;
    s_active_env = nullptr;
}

void gamepad_haptic_weapon_fire(int32_t weapon_type)
{
    if (gamepad_get_device_type() != INPUT_DEVICE_DUALSENSE) return;

    const HapticEnvelope* env = nullptr;
    bool is_single_shot_weapon = false;
    switch (weapon_type) {
        case SPECIAL_AK47:
            if (s_ak47_env.loaded) env = &s_ak47_env;
            // AK47 is auto-fire — no trigger click to suppress.
            break;
        case SPECIAL_SHOTGUN:
            if (s_shotgun_env.loaded) env = &s_shotgun_env;
            is_single_shot_weapon = true;
            break;
        case SPECIAL_GUN:
        case SPECIAL_NONE: // pistol with no SpecialUse — SpecialType reads as 0
            if (s_pistol_env.loaded) env = &s_pistol_env;
            is_single_shot_weapon = true;
            break;
        default:
            break;
    }

    // On single-shot weapons, immediately disarm the Weapon25 click on the
    // fire frame so there's no chance of a stale click sneaking through
    // between the game setting Timer1 and gamepad_triggers_update running.
    // The actual cooldown duration is driven by the game's Timer1.
    if (is_single_shot_weapon) {
        gamepad_triggers_lockout(0);
    }

    if (!env) return;

    // Restart playback on each shot — for auto-fire the envelope is
    // retriggered per game-tick shot, matching the sound playback behaviour.
    s_active_env = env;
    s_play_time = 0.0f;
    s_prev_index = 0;
    s_playing = true;
    // Force dt=0 on the next tick so we start from envelope sample 0 rather
    // than jumping deep into it with stale time from a previous idle session.
    s_tick_initialized = false;
}

uint8_t gamepad_haptic_tick(bool* out_active)
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

    // Envelope ended — stop.
    if (s_prev_index >= env_len) {
        s_playing = false;
        s_active_env = nullptr;
        s_prev_index = 0;
        return 0;
    }

    // Take MAX over [prev_index, curr_index] so we don't alias past peaks
    // when the game tick is coarse (~33ms) and envelope step is fine (5ms).
    const size_t scan_end = curr_index < env_len ? curr_index : env_len - 1;
    uint8_t peak = 0;
    for (size_t i = s_prev_index; i <= scan_end; i++) {
        if (s_active_env->samples[i] > peak) peak = s_active_env->samples[i];
    }
    s_prev_index = curr_index + 1;

    if (out_active) *out_active = true;
    return peak;
}

void gamepad_haptic_stop()
{
    s_playing = false;
    s_active_env = nullptr;
}
