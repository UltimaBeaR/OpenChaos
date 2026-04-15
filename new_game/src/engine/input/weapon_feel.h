#pragma once

// Weapon feel — unified module for everything that makes firing a weapon
// feel physical on a gamepad. Encapsulates three previously scattered
// subsystems so adding a new weapon is one WeaponFeelProfile registration
// instead of edits across four files:
//
//   1. Vibration envelope computed from the weapon's WAV sample (slow rumble
//      motor, DualSense voice-coil follows it; see
//      new_game_devlog/weapon_haptic_and_adaptive_trigger.md for background).
//   2. DualSense adaptive-trigger (Weapon25) parameters — click point and
//      resistance amplitude. The trigger state machine in gamepad.cpp asks
//      this module what to send when AIM_GUN mode is active.
//   3. Analog-trigger fire detection — rising-edge vs held-down, and the
//      R2/L2 thresholds. input_actions.cpp delegates the whole decision.
//
// All tuning for a weapon lives in its WeaponFeelProfile. The module keeps a
// small registry keyed by SPECIAL_* constant; unregistered weapons fall back
// to a default profile (no haptic, no adaptive trigger click, generic fire
// thresholds).

#include <cstdint>

struct WeaponFeelProfile {
    // --- Haptic envelope (slow rumble motor) -------------------------------
    // haptic_wave_id: index into sound_list[] (see assets/sound_id.h). Zero
    //                 disables envelope playback for this weapon.
    // haptic_gain:    overall loudness multiplier, 0..1.
    // haptic_ceiling: hard clamp on motor value, 0..255.
    // haptic_max_seconds: envelope truncation. MUST be shorter than the
    //                 weapon's auto-fire cooldown or consecutive shots
    //                 overlap into continuous buzz.
    uint32_t haptic_wave_id;
    float    haptic_gain;
    uint8_t  haptic_ceiling;
    float    haptic_max_seconds;

    // --- Adaptive trigger (DualSense Weapon25) -----------------------------
    // Per the reverse-engineered Sony Weapon (0x25) effect spec (Nielk1
    // gist) with the fixed lib packing patch in GamepadTrigger.h:
    //   trigger_start_zone: startPosition — zone where click zone begins (2..7)
    //   trigger_end_zone:   endPosition   — zone where click zone ends (start+1..8)
    //   trigger_strength:   0..8, hardware receives strength-1
    // Zero trigger_strength disables the adaptive-trigger click entirely.
    uint8_t  trigger_start_zone;
    uint8_t  trigger_end_zone;
    uint8_t  trigger_strength;

    // --- Analog fire detection ---------------------------------------------
    // fire_threshold: R2/L2 value above which the shot fires (0..255).
    //                 Should match the Weapon25 click point so shot & click
    //                 are synchronized on the rising edge of the trigger
    //                 press.
    // reset_threshold: R2/L2 value at or below which the rising-edge
    //                 detector rearms. The player must release to this
    //                 point between shots (half-taps do not arm).
    // auto_fire: true = held-down fire (machine guns, e.g. AK47); false =
    //            rising-edge (pistol, shotgun — each shot requires the
    //            trigger to drop to reset_threshold and then rise past
    //            fire_threshold again).
    uint8_t  fire_threshold;
    uint8_t  reset_threshold;
    bool     auto_fire;
};

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Register built-in profiles (pistol, AK47, shotgun) and build their
// envelopes from WAV samples. Safe to call once at gamepad init.
void weapon_feel_init();
void weapon_feel_shutdown();

// Register or override a profile for a SPECIAL_* weapon type. If the profile
// has a non-zero haptic_wave_id and weapon_feel_init() has already run, the
// envelope is built immediately.
void weapon_feel_register(int32_t special_type, const WeaponFeelProfile& profile);

// Returns the profile for a weapon type, or the default fallback profile if
// no specific one is registered. Never returns null.
const WeaponFeelProfile* weapon_feel_get_profile(int32_t special_type);

// ---------------------------------------------------------------------------
// Haptic envelope playback
// ---------------------------------------------------------------------------

// Call from actually_fire_gun() on every REAL player shot (i.e. a shot
// the game actually committed — not a blocked-by-Timer1 attempt).
// Restarts envelope playback for the weapon's profile. For single-shot
// weapons also calls gamepad_triggers_lockout() to briefly blink the
// mode to NONE on the fire frame (legacy, see gamepad.cpp comment on
// gamepad_triggers_lockout for current status). Non-DualSense devices:
// no-op.
void weapon_feel_on_shot_fired(int32_t special_type);

// Per-frame tick. Advances any active envelope and returns the current
// motor value (0..255) to be max-merged into the slow rumble motor.
// *out_active = true while an envelope is playing.
uint8_t weapon_feel_tick_haptic(bool* out_active);

// Stop envelope playback (death, level reset, menu).
void weapon_feel_stop_haptic();

// ---------------------------------------------------------------------------
// Fire detection (analog R2/L2 → discrete shoot/kick events)
// ---------------------------------------------------------------------------

struct WeaponFireDecision {
    bool shoot; // R2 produced a shot event this frame
    bool kick;  // L2 produced a kick event this frame
};

// Run one tick of fire detection for the given analog trigger positions.
// current_weapon picks the profile (auto_fire vs rising-edge, thresholds).
// L2 (kick) always uses the current profile's thresholds but never auto-fires.
WeaponFireDecision weapon_feel_evaluate_fire(int32_t current_weapon, int r2, int l2);

// Reset rising-edge armed state. Call when the player enters a car, drops
// the weapon, or in any context where triggers aren't being read as fire
// input. Prevents a stray "fire" the instant control is handed back.
void weapon_feel_fire_reset();

// True if the fire detector currently considers R2 armed for a shot.
// For rising-edge weapons this means R2 has dipped to ≤ reset_threshold
// since the last shot. For auto-fire weapons this is always true.
//
// NOTE: kept in the public API for future experiments but the current
// gamepad.cpp adaptive-trigger state machine does NOT consult it —
// mode=AIM_GUN is enabled unconditionally while the weapon is ready.
// See the devlog section "Подходы которые пробовали" → "Armed gate"
// for why the armed-gate approach was abandoned (HID latency race).
bool weapon_feel_is_r2_armed(int32_t current_weapon);

// DEBUG: append a printf-style line to weapon_feel_debug.log. Timestamped
// with ms since first call. File is opened on first call and flushed on
// every write so the last line is never lost on crash/abort/exit.
void weapon_feel_debug_log(const char* fmt, ...);
