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
    // Both fields are 4-bit (0..15); values outside that overflow inside
    // GamepadCore's Weapon25 encoder. Zero trigger_amplitude disables the
    // adaptive-trigger click for this weapon (trigger stays free).
    uint8_t  trigger_start_zone;
    uint8_t  trigger_amplitude;

    // --- Analog fire detection ---------------------------------------------
    // fire_threshold: R2/L2 value above which the shot fires (0..255).
    //                 Should match where the Weapon25 click point lands so
    //                 shot & click are synchronized.
    // reset_threshold: R2/L2 value below which rising-edge detector rearms.
    //                 MUST equal the entry-gate threshold used by the trigger
    //                 state machine in gamepad.cpp — this module enforces the
    //                 invariant by exposing both via the same profile.
    // auto_fire: true = held-down fire (machine guns); false = rising-edge
    //            (pistol, shotgun — must release past reset_threshold between
    //            shots).
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

// Call from actually_fire_gun() on every player shot. Starts envelope
// playback for the weapon's profile and, for single-shot weapons, also
// kicks the adaptive-trigger lockout so no stale click leaks through
// between the game setting Timer1 and gamepad_triggers_update running.
// Non-DualSense devices: no-op.
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
