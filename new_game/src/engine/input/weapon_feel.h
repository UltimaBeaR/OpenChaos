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

#include <cstddef>
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
// Restarts envelope playback for the weapon's profile. Non-DualSense
// devices and unregistered weapons: no-op.
//
// remaining_timer1: Person->Timer1 value AT THE MOMENT OF FIRE, before
//   set_person_*_shoot resets it. Used for pre-release debt
//   compensation: if the player fired while Timer1 was still counting
//   (inside the pre-release window), that remainder is added on top of
//   the base cooldown for the next shot, so average rate stays pinned
//   to the animation duration. Pass 0 for non-player shots — debt is
//   captured only for the player.
void weapon_feel_on_shot_fired(int32_t special_type, int32_t remaining_timer1);

// Returns the cooldown value (in Timer1 units) that Person->Timer1 should
// be set to at the start of a shot, for the given weapon. Derived from
// the shoot animation's actual tick count at the current TICK_RATIO —
// no hardcoded per-weapon magic numbers. Includes any carried-over
// pre-release debt from the previous shot (and clears it).
//
// Returns 0 if the weapon has no registered shoot animation (e.g.
// unknown weapon type). Caller should fall back to the original
// hardcoded value in that case.
int32_t weapon_feel_consume_shot_cooldown_timer1(int32_t special_type);

// Pre-release threshold in Timer1 units. Both the fire gate and the
// DualSense mode gate open when Person->Timer1 drops to this value,
// rather than waiting for exact zero. Motor mode=AIM_GUN turns on at
// this point and the HID propagation window (~30 ms) overlaps with
// the tail of the cooldown — by Timer1=0 the motor is physically
// ready to click. If the player taps inside this window, the shot
// fires with the remainder captured as debt for the next shot, so
// average fire rate stays pinned to the animation duration.
//
// Computed from the current TICK_RATIO so it automatically tracks
// frame-rate changes. Typical value at 15 fps (TICK_RATIO=256): 32
// (= 2 ticks × decrement 16).
int32_t weapon_feel_pre_release_timer1();

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
// weapon_drawn: true iff the player's gun/weapon is currently out
// (FLAG_PERSON_GUN_OUT). When false, the adaptive trigger click is not
// active on hardware regardless of profile, and fire detection falls
// back to the threshold path (so R2 still triggers bare-hand melee).
// L2 (kick) always uses the current profile's thresholds but never auto-fires.
//
// R2 fire detection picks a path per tick based on device + profile +
// weapon state:
//   * DualSense + trigger_strength>0 + !auto_fire + weapon_drawn →
//     adaptive-click path (fires on analog crossing the upper edge of
//     the weapon's Weapon25 zone, which aligns with where the motor
//     physically clicks).
//   * Otherwise → threshold path (r2 rising past fire_threshold with
//     armed gate, auto-fire bypasses the gate). Covers bare-hand melee,
//     auto-fire weapons, click-less weapons, and non-DualSense devices.
// See the design section at the top of weapon_feel.cpp for details.
WeaponFireDecision weapon_feel_evaluate_fire(int32_t current_weapon, int r2, int l2, bool weapon_drawn);

// Reset rising-edge armed state. Call when the player enters a car, drops
// the weapon, or in any context where triggers aren't being read as fire
// input. Prevents a stray "fire" the instant control is handed back.
void weapon_feel_fire_reset();
