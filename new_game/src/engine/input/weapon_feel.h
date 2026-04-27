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

// DualSense adaptive-trigger effect for this weapon. Single-shot guns
// (pistol) get a Weapon25 click at a narrow zone; auto-fire weapons
// (AK47) get a Machine (0x27) two-beat pulse spanning the whole press
// range so feedback stays present when the trigger is held fully down.
enum class TriggerEffectType : uint8_t {
    None = 0,   // no adaptive trigger effect (trigger free)
    Weapon,     // single click between start/end zones
    Machine,    // continuous two-amplitude pulse between start/end zones
    Vibration,  // sustained periodic vibration from `position` onwards
                // (used as a runtime override for the post-shot recoil
                // burst — not typically set as a baseline profile effect).
};

struct WeaponFeelProfile {
    // --- Haptic envelope (slow rumble motor) -------------------------------
    // haptic_wave_id: index into sound_list[] (see assets/sound_id.h). Zero
    //                 disables envelope playback for this weapon.
    // haptic_gain:    overall loudness multiplier, 0..1.
    // haptic_ceiling: hard clamp on motor value, 0..255.
    // haptic_max_seconds: envelope truncation. MUST be shorter than the
    //                 weapon's auto-fire cooldown or consecutive shots
    //                 overlap into continuous buzz.
    // The envelope is device-agnostic: on DualSense it plays on the slow
    // (left) voice-coil motor; on Xbox / generic SDL gamepads it plays
    // on the low-frequency rumble motor via sdl3_gamepad_rumble.
    // haptic_xbox_boost: multiplier applied to envelope output when the
    //                 active device is NOT a DualSense. Xbox / generic
    //                 rumble motors are less responsive to small
    //                 amplitudes, so a weapon tuned quiet on DS (low
    //                 ceiling) often feels missing on Xbox. Peak is
    //                 clamped to 255 after the multiply. 1.0 = no
    //                 change.
    uint32_t haptic_wave_id;
    float    haptic_gain;
    uint8_t  haptic_ceiling;
    float    haptic_max_seconds;
    float    haptic_xbox_boost;

    // Xbox / generic SDL rumble — per-motor routing of the envelope peak.
    // The envelope peak (0..255, boosted by xbox_boost) is scaled by
    // these percentages (0..100) when writing to the two rumble motors.
    // Defaults (100/0) send the envelope only to the low-frequency motor
    // — matches single-motor behaviour. Setting high_percent > 0 fans
    // the envelope out to the high motor too (fine-grained buzz),
    // useful for weapons where a single heavy thump feels flat (e.g.
    // AK47 held-fire). DualSense path uses its own slow-motor envelope
    // routing and is unaffected.
    uint8_t  xbox_rumble_low_percent;   // 0..100
    uint8_t  xbox_rumble_high_percent;  // 0..100

    // --- Adaptive trigger (DualSense) --------------------------------------
    // trigger_effect selects which DualSense effect runs on R2 while the
    // weapon is ready:
    //   None    — trigger stays free.
    //   Weapon  — Weapon25 click. Uses start_zone, end_zone, strength.
    //             Per Nielk1 gist with the lib packing patch:
    //               start_zone:  click zone begin (2..7)
    //               end_zone:    click zone end   (start+1..8)
    //               strength:    0..8 (hardware receives strength-1)
    //             Zero strength disables the effect.
    //   Machine — Machine (0x27) two-beat pulse. Uses start_zone, end_zone,
    //             machine_amp_a, machine_amp_b, machine_frequency,
    //             machine_period. Effect is "on" whenever R2 is inside
    //             [start_zone, end_zone]; both amplitudes at zero
    //             disables the effect. Setting end_zone=8 makes the
    //             effect cover the whole press range so the pulse
    //             doesn't cut out when the player bottoms the trigger.
    TriggerEffectType trigger_effect;
    uint8_t  trigger_start_zone;
    uint8_t  trigger_end_zone;
    uint8_t  trigger_strength;     // Weapon only
    uint8_t  machine_amp_a;        // Machine only, 0..7
    uint8_t  machine_amp_b;        // Machine only, 0..7
    uint8_t  machine_frequency;    // Machine only, pulse rate
    uint8_t  machine_period;       // Machine only, pattern period

    // --- Reload click (auto-fire weapons) ---------------------------------
    // When the weapon's magazine is empty, the adaptive trigger briefly
    // runs a Weapon25 click so the player's reload press feels like a
    // gun-mechanism click (same feel as the pistol). Active only while
    // the mag is empty; the main trigger_effect resumes after reload.
    // Zero strength disables the reload click.
    //   reload_click_start_zone: click zone begin (2..7)
    //   reload_click_end_zone:   click zone end   (start+1..8)
    //   reload_click_strength:   0..8 (hardware receives strength-1)
    uint8_t  reload_click_start_zone;
    uint8_t  reload_click_end_zone;
    uint8_t  reload_click_strength;

    // --- Post-shot trigger vibration burst (e.g. shotgun recoil feel) -----
    // Right after a shot fires, override the main trigger_effect with a
    // sustained Vibration whose amplitude decays linearly from amp_start
    // to amp_end over `seconds`, then restores the main effect. Emulates
    // trigger-side recoil. All zero/off disables the burst.
    //   post_shot_vibration_seconds:    duration (0 = disabled)
    //   post_shot_vibration_position:   vibration zone start (0..9; 0 =
    //                                   felt across the full trigger range)
    //   post_shot_vibration_frequency:  vibration rate in Hz (1..255)
    //   post_shot_vibration_amp_start:  amplitude at t=0 (0..8)
    //   post_shot_vibration_amp_end:    amplitude at t=seconds (0..8; usually 0)
    float    post_shot_vibration_seconds;
    uint8_t  post_shot_vibration_position;
    uint8_t  post_shot_vibration_frequency;
    uint8_t  post_shot_vibration_amp_start;
    uint8_t  post_shot_vibration_amp_end;

    // --- Between-shot anim interlude (for auto-fire weapons) --------------
    // aim_interlude_anim: secondary animation ID played during the latter
    //     part of each shot cycle, after the main shoot animation has
    //     finished playing but before Timer1 reaches zero and the next
    //     shot fires. Mirrors the original MuckyFoot AIM_GUN-anim visual
    //     when the state machine used to ping-pong between SUB_STATE_SHOOT_GUN
    //     and SUB_STATE_AIM_GUN: persona alternates shoot-pose ↔ aim-pose
    //     each cycle ("колбасится" effect). Set to 0 to keep the primary
    //     shoot anim running (pistol / shotgun — single-shot weapons never
    //     hit the interlude because Timer1 expires via a new PUNCH before
    //     the anim switch would matter). The ID is a raw anim_id from
    //     things/characters/anim_ids.h; the unified SHOOT_GUN handler
    //     calls set_anim on it when person_normal_animate_speed signals
    //     end==1, which then plays naturally on subsequent ticks until
    //     Timer1=0 triggers the next shot.
    int32_t  aim_interlude_anim;

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

// Decide whether the weapon's trigger effect should be active right now.
// Caller (game.cpp) supplies two primitives so weapon_feel doesn't need
// to know about Thing* / state enums:
//   * in_shot_cycle — Timer1 is counting down from a real shot AND the
//                     person is in a firing state (shot just fired,
//                     mid-cooldown).
//   * mag_empty    — current clip is 0 (or reload gate is set, meaning
//                    we're in the reload-press window before the next
//                    real shot). See game.cpp's mag_empty computation.
//
// Per-weapon policy:
//   * auto-fire weapons (AK47, Machine effect): effect represents
//     recoil from real shots, so ON while in_shot_cycle. Additionally
//     ON during mag_empty IF a reload click is configured — gamepad.cpp
//     swaps the effect to a Weapon25 click with reload params during
//     that window so the reload press has a pistol-style click.
//
//   * single-shot weapons (pistol, Weapon25 click): effect is a
//     pre-fire click the player squeezes into. OFF during shot cycle
//     so the hardware can't re-click during cooldown, ON otherwise.
//     mag_empty is ignored (pistol's main effect already is a click).
//
// Caller still wraps this with its own "weapon drawn / non-firing
// state / target surrendered" gates — those are game-level policies
// this module has no opinion on.
bool weapon_feel_trigger_effect_should_run(int32_t current_weapon, bool in_shot_cycle, bool mag_empty);

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

// Xbox / generic rumble per-motor envelope scaling for the currently
// playing envelope. Values are 0..100 (percent of envelope peak). Read
// after weapon_feel_tick_haptic to route the peak to the two motors.
// Returns defaults (low=100, high=0) when no envelope is playing.
void weapon_feel_get_active_xbox_rumble_scales(uint8_t* out_low_percent,
                                               uint8_t* out_high_percent);

// Stop envelope playback (death, level reset, menu).
void weapon_feel_stop_haptic();

// Per-frame query for the post-shot trigger vibration burst (see
// post_shot_vibration_* in WeaponFeelProfile). Returns true while a burst
// is active for the profile that fired; fills out_position, out_amplitude
// (0..8, linearly interpolated over the burst lifetime), out_frequency.
// Returns false otherwise. The returned amplitude changes every frame
// during the burst — callers should re-apply the Vibration trigger state
// whenever amplitude changes.
bool weapon_feel_tick_trigger_vibration(
    uint8_t* out_position,
    uint8_t* out_amplitude,
    uint8_t* out_frequency);

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
WeaponFireDecision weapon_feel_evaluate_fire(int32_t current_weapon, int r2, int l2, bool weapon_drawn, bool mag_empty);
