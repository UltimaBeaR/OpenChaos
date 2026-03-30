#include "engine/effects/psystem_globals.h"

// uc_orig: particles (fallen/Source/psystem.cpp)
Particle particles[PSYSTEM_MAX_PARTICLES];

// uc_orig: next_free (fallen/Source/psystem.cpp)
SLONG next_free;

// uc_orig: next_used (fallen/Source/psystem.cpp)
SLONG next_used;

// uc_orig: particle_count (fallen/Source/psystem.cpp)
SLONG particle_count;

// uc_orig: prev_tick (fallen/Source/psystem.cpp)
// BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
uint64_t prev_tick;

// uc_orig: first_pass (fallen/Source/psystem.cpp)
BOOL first_pass;
