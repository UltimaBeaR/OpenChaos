#ifndef THINGS_CHARACTERS_SNIPE_H
#define THINGS_CHARACTERS_SNIPE_H

#include "engine/core/types.h"

// Snipe (rifle scope) mode — activates a zoomed camera and directional snipe aiming.

// uc_orig: SNIPE_mode_on (fallen/Headers/snipe.h)
// Activates snipe mode at the given world position with the given initial yaw (0–2047).
void SNIPE_mode_on(SLONG x, SLONG y, SLONG z, SLONG initial_yaw);

// uc_orig: SNIPE_mode_off (fallen/Headers/snipe.h)
void SNIPE_mode_off(void);

// Turn direction bitmask flags for SNIPE_turn.
// uc_orig: SNIPE_TURN_LEFT (fallen/Headers/snipe.h)
#define SNIPE_TURN_LEFT (1 << 0)
// uc_orig: SNIPE_TURN_RIGHT (fallen/Headers/snipe.h)
#define SNIPE_TURN_RIGHT (1 << 1)
// uc_orig: SNIPE_TURN_UP (fallen/Headers/snipe.h)
#define SNIPE_TURN_UP (1 << 2)
// uc_orig: SNIPE_TURN_DOWN (fallen/Headers/snipe.h)
#define SNIPE_TURN_DOWN (1 << 3)

// uc_orig: SNIPE_turn (fallen/Headers/snipe.h)
// Accumulates angular velocity deltas for the given turn bitmask.
void SNIPE_turn(SLONG turn);

// uc_orig: SNIPE_process (fallen/Headers/snipe.h)
// Updates snipe camera orientation by integrating angular velocities. Call once per game turn.
void SNIPE_process(void);

// uc_orig: SNIPE_shoot (fallen/Headers/snipe.h)
// Fires the snipe rifle — finds a target in the aim direction and applies a hit.
void SNIPE_shoot(void);

#endif // THINGS_CHARACTERS_SNIPE_H
