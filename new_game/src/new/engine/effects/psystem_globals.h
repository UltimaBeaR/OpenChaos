#ifndef ENGINE_EFFECTS_PSYSTEM_GLOBALS_H
#define ENGINE_EFFECTS_PSYSTEM_GLOBALS_H

#include <windows.h>  // BOOL — temporary dependency until Stage 8
#include "engine/effects/psystem.h"

// The full particle pool — statically allocated.
// Index 0 is a permanent sentinel; real particles are indices 1..PSYSTEM_MAX_PARTICLES-1.
// uc_orig: particles (fallen/Source/psystem.cpp)
extern Particle particles[PSYSTEM_MAX_PARTICLES];

// Head of the free list (index into particles[]).
// uc_orig: next_free (fallen/Source/psystem.cpp)
extern SLONG next_free;

// Tail of the used list — most recently added active particle.
// Iteration walks backwards through used list via Particle::prev.
// uc_orig: next_used (fallen/Source/psystem.cpp)
extern SLONG next_used;

// Number of currently active particles.
// uc_orig: particle_count (fallen/Source/psystem.cpp)
extern SLONG particle_count;

// Timestamp (GetTickCount) of the last PARTICLE_Run call — used to compute dt.
// uc_orig: prev_tick (fallen/Source/psystem.cpp)
// claude-ai: BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD
extern DWORD prev_tick;

// True on the very first PARTICLE_Run call — forces dt = one normal tick to avoid a huge initial delta.
// uc_orig: first_pass (fallen/Source/psystem.cpp)
extern BOOL first_pass;

#endif // ENGINE_EFFECTS_PSYSTEM_GLOBALS_H
