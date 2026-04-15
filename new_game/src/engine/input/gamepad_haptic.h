#pragma once

// Weapon haptic feedback for DualSense.
//
// Drives the rumble motors with an amplitude envelope computed from the
// actual weapon WAV sample. Not "true" Sony audio-haptic (see
// new_game_devlog/dualsense_audio_haptic_investigation.md for why that path
// is not available), but close enough on DualSense because its voice-coil
// actuators can follow fast amplitude modulation.
//
// Per-weapon profiles live here — future work will give pistol / shotgun
// their own playback style and combine vibration with adaptive triggers.

#include <cstdint>

void gamepad_haptic_init();
void gamepad_haptic_shutdown();

// Trigger haptic feedback for a weapon shot by the player. weapon_type is a
// SPECIAL_* constant (see things/characters/special.h). Non-DualSense and
// non-mapped weapon types are no-ops.
void gamepad_haptic_weapon_fire(int32_t weapon_type);

// Per-frame update. Advances any active haptic playback and returns the
// current envelope value (0..255) to be max-merged into the slow rumble motor.
// `*out_active` is set to true if a haptic envelope is currently playing.
uint8_t gamepad_haptic_tick(bool* out_active);

// Stop any active haptic playback. Call on death, level restart, menu.
void gamepad_haptic_stop();
