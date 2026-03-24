#ifndef ACTORS_ITEMS_HOOK_H
#define ACTORS_ITEMS_HOOK_H

#include "engine/core/types.h"

// uc_orig: HOOK_NUM_POINTS (fallen/Headers/hook.h)
#define HOOK_NUM_POINTS 256

// uc_orig: HOOK_STATE_STILL (fallen/Headers/hook.h)
#define HOOK_STATE_STILL 0
// uc_orig: HOOK_STATE_SPINNING (fallen/Headers/hook.h)
#define HOOK_STATE_SPINNING 1
// uc_orig: HOOK_STATE_FLYING (fallen/Headers/hook.h)
#define HOOK_STATE_FLYING 2

// uc_orig: HOOK_REELED_CONTINUE (fallen/Headers/hook.h)
#define HOOK_REELED_CONTINUE 0
// uc_orig: HOOK_REELED_IN (fallen/Headers/hook.h)
#define HOOK_REELED_IN 1
// uc_orig: HOOK_REELED_TAUT (fallen/Headers/hook.h)
#define HOOK_REELED_TAUT 2

// Initialises the grappling hook coiled at the given map position.
// uc_orig: HOOK_init (fallen/Headers/hook.h)
void HOOK_init(SLONG x, SLONG z);

// Returns the current hook state (STILL, SPINNING, or FLYING).
// uc_orig: HOOK_get_state (fallen/Headers/hook.h)
SLONG HOOK_get_state(void);

// Begins spinning the hook around point (x,y,z) at the given yaw.
// speed_or_minus_pitch: positive = spin speed, negative = actual pitch of the hook.
// uc_orig: HOOK_spin (fallen/Headers/hook.h)
void HOOK_spin(SLONG x, SLONG y, SLONG z, SLONG yaw, SLONG speed_or_minus_pitch);

// Releases the hook from spinning, transitioning it to FLYING state.
// uc_orig: HOOK_release (fallen/Headers/hook.h)
void HOOK_release(void);

// Advances hook physics simulation by one game tick.
// uc_orig: HOOK_process (fallen/Headers/hook.h)
void HOOK_process(void);

// Returns the world position and orientation of the grapple head (point 0).
// Positions are in high-res coordinates (16 bits per map square).
// uc_orig: HOOK_pos_grapple (fallen/Headers/hook.h)
void HOOK_pos_grapple(SLONG* x, SLONG* y, SLONG* z, SLONG* yaw, SLONG* pitch, SLONG* roll);

// Returns the world position of a specific string point (0 = grapple head, 255 = tail).
// uc_orig: HOOK_pos_point (fallen/Headers/hook.h)
void HOOK_pos_point(SLONG point, SLONG* x, SLONG* y, SLONG* z);

#endif // ACTORS_ITEMS_HOOK_H
