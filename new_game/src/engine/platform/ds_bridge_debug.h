#pragma once

// Debug-only extension of ds_bridge.
//
// The main ds_bridge.h keeps libDualsense types out of the public game
// API — game code consumes the `DS_InputState` game-normalized struct.
// The DualSense in-game tester (input_debug panel) wants access to
// fields the game doesn't use (raw stick/trigger bytes, IMU samples,
// battery level %, headphone flag, full feedback byte, touchpad finger
// IDs) and to one-shot feature reports (firmware info, PCBA, serials,
// etc). Rather than replicate every lib type in ds_bridge.h, this
// header exposes the raw `oc::dualsense::Device*` and the raw
// `InputState` snapshot the bridge already owns, so the tester can
// call libDualsense APIs directly for debug-only needs.
//
// Include from debug code only — never from regular game code.

#include <libDualsense/ds_device.h>
#include <libDualsense/ds_input.h>

// Returns the bridge-owned DualSense device pointer (never null; check
// `->connected` before use). The tester feeds this into libDualsense
// feature-report / test-command APIs that need direct device access.
oc::dualsense::Device* ds_debug_get_device();

// Returns the bridge-owned raw input snapshot — populated every tick
// by ds_update_input from libDualsense's parsed InputState. All fields
// are in raw device units (sticks 0..255, triggers 0..255, motion
// int16, timestamp uint32). Never null.
const oc::dualsense::InputState* ds_debug_get_raw_input();
