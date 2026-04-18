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

    // Post-fire cooldown in milliseconds during which the adaptive-trigger
    // effect stays disabled (mode=NONE) so the hardware can't emit clicks
    // while the game is still in its per-weapon cooldown window. Matches
    // the natural fire rate of the weapon: for the pistol this is the
    // running-shot Timer1 duration (~583 ms). Set to 0 for weapons that
    // don't have an adaptive click (AK47, shotgun today) — the gate is
    // still consulted but mode stays NONE regardless.
    //
    // Tracked locally in weapon_feel via wall-clock rather than reading
    // Person::Timer1 because Timer1 is per-person state reused for
    // several purposes and only decrements in specific states (notably
    // STATE_MOVEING) — after a running shot followed by standing still,
    // Timer1 would freeze at its cooldown value forever. Standing-still
    // fire (set_person_shoot) doesn't set Timer1 at all. Local
    // wall-clock is the only signal that works across all firing paths.
    uint32_t fire_cooldown_ms;
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
// weapon_drawn: true iff the player's gun/weapon is currently out
// (FLAG_PERSON_GUN_OUT). When false, the adaptive trigger click is not
// active on hardware regardless of profile, and fire detection falls
// back to the threshold path (so R2 still triggers bare-hand melee).
// L2 (kick) always uses the current profile's thresholds but never auto-fires.
//
// R2 fire detection picks a path per tick based on device + profile +
// weapon state:
//   * DualSense + trigger_strength>0 + !auto_fire + weapon_drawn →
//     act-bit path (fires on the hardware click signal, reads
//     effect-active from gamepad_get_right_trigger_effect_active()).
//   * Otherwise → threshold path (r2 rising past fire_threshold with
//     armed gate, auto-fire bypasses the gate). Covers bare-hand melee,
//     auto-fire weapons, click-less weapons, and non-DualSense devices.
// See the design section at the top of weapon_feel.cpp for details.
WeaponFireDecision weapon_feel_evaluate_fire(int32_t current_weapon, int r2, int l2, bool weapon_drawn);

// Reset rising-edge armed state. Call when the player enters a car, drops
// the weapon, or in any context where triggers aren't being read as fire
// input. Prevents a stray "fire" the instant control is handed back.
void weapon_feel_fire_reset();

// True if a real shot fired within the weapon's fire_cooldown_ms window.
// Consulted by gamepad.cpp (via game.cpp) to force mode=NONE during the
// cooldown so the DualSense adaptive trigger doesn't emit phantom clicks
// while the game is still in its per-weapon fire-rate gate. Always
// false after the cooldown expires or if no shot has ever fired. Safe
// to call without a DualSense connected — it's just a timer check.
bool weapon_feel_is_in_fire_cooldown();

// Called once per game tick from game.cpp with the current game-side
// fire-readiness state.
//
//   in_cooldown: true when the game is not yet ready to accept a new
//     shot — either the running-shot Timer1 is still counting, or the
//     standing-shoot animation is still playing. A gamepad click in
//     this window is detected (so we don't silently drop it) but buffered
//     as a pending shot and emitted as PUNCH the moment the flag clears.
//     Mode=AIM_GUN is kept off during cooldown (by game.cpp's
//     weapon_ready gate) so the hardware doesn't produce tactile
//     feedback for these queued presses.
//
//   weapon_ready: true when the player is in a state where firing is
//     otherwise legal (gun drawn, not in cutscene, not jumping, etc.).
//     Any pending shot is cancelled as soon as this goes false — so a
//     click queued during cooldown won't auto-fire after a jump or
//     cutscene.
void weapon_feel_set_game_state(bool in_cooldown, bool weapon_ready);

// True if the threshold-path fire detector considers R2 armed for a shot.
// For rising-edge weapons this means R2 has dipped to ≤ reset_threshold
// since the last shot; auto-fire weapons are always true.
//
// NOTE: the DualSense act-bit path does NOT consult this flag — the
// hardware itself enforces one click per physical press. gamepad.cpp
// also doesn't gate mode on it; mode=AIM_GUN stays continuously enabled
// while the weapon is ready. Kept in the public API for any future
// experiments / debug overlays that want to introspect the threshold
// state.
bool weapon_feel_is_r2_armed(int32_t current_weapon);

// DEBUG: append a printf-style line to weapon_feel_debug.log. Timestamped
// with ms since first call. File is opened on first call and flushed on
// every write so the last line is never lost on crash/abort/exit.
void weapon_feel_debug_log(const char* fmt, ...);

// DEBUG: format a compact string with the last N shot intervals and their
// input source, for on-screen comparison of keyboard vs gamepad fire rate.
// Example output: "gaps ms: [kb]283 [kb]275 [ds]650 [ds]680".
// Source is sampled at the moment of the shot from the active input device.
// Records older than the buffer capacity are overwritten. Safe to call
// every frame — pure formatter, no state mutation.
void weapon_feel_format_shot_history(char* out, size_t out_size);

// DEBUG: format average gap between the last 5 shots, split per device.
// Example output: "avg kb: 280ms (n=4) | avg ds: 620ms (n=5)".
// n is the number of intervals actually averaged (min 1, max 5). Devices
// with fewer than 2 shots in the history buffer show "-". Safe to call
// every frame — pure formatter, no state mutation.
void weapon_feel_format_shot_averages(char* out, size_t out_size);
