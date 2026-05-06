#ifndef ENGINE_EFFECTS_PSYSTEM_H
#define ENGINE_EFFECTS_PSYSTEM_H

#include "engine/core/types.h"

// Particle pool capacity — 2048 particles active simultaneously (PC build).
// uc_orig: PSYSTEM_MAX_PARTICLES (fallen/Headers/PSystem.h)
#define PSYSTEM_MAX_PARTICLES 2048

// ---- Particle behaviour flags (PFLAG_*) ----

// uc_orig: PFLAG_GRAVITY (fallen/Headers/PSystem.h)
#define PFLAG_GRAVITY 1
// uc_orig: PFLAG_BOUYANT (fallen/Headers/PSystem.h)
#define PFLAG_BOUYANT 2
// uc_orig: PFLAG_FADE (fallen/Headers/PSystem.h)
#define PFLAG_FADE 4
// uc_orig: PFLAG_FIRE (fallen/Headers/PSystem.h)
#define PFLAG_FIRE 8
// uc_orig: PFLAG_COLLIDE (fallen/Headers/PSystem.h)
#define PFLAG_COLLIDE 16
// uc_orig: PFLAG_BOUNCE (fallen/Headers/PSystem.h)
#define PFLAG_BOUNCE 32
// uc_orig: PFLAG_RESIZE (fallen/Headers/PSystem.h)
#define PFLAG_RESIZE 64
// uc_orig: PFLAG_WANDER (fallen/Headers/PSystem.h)
#define PFLAG_WANDER 128
// uc_orig: PFLAG_DRIFT (fallen/Headers/PSystem.h)
#define PFLAG_DRIFT 256
// uc_orig: PFLAG_SPRITEANI (fallen/Headers/PSystem.h)
#define PFLAG_SPRITEANI 512
// uc_orig: PFLAG_INVALPHA (fallen/Headers/PSystem.h)
#define PFLAG_INVALPHA 1024
// uc_orig: PFLAG_RESIZE2 (fallen/Headers/PSystem.h)
#define PFLAG_RESIZE2 2048
// uc_orig: PFLAG_SPRITELOOP (fallen/Headers/PSystem.h)
#define PFLAG_SPRITELOOP 4096
// uc_orig: PFLAG_FADE2 (fallen/Headers/PSystem.h)
#define PFLAG_FADE2 8192
// uc_orig: PFLAG_DAMPING (fallen/Headers/PSystem.h)
#define PFLAG_DAMPING 16384
// uc_orig: PFLAG_HURTPEOPLE (fallen/Headers/PSystem.h)
#define PFLAG_HURTPEOPLE 32768
// uc_orig: PFLAG_EXPLODE_ON_IMPACT (fallen/Headers/PSystem.h)
#define PFLAG_EXPLODE_ON_IMPACT 65536
// uc_orig: PFLAG_LEAVE_TRAIL (fallen/Headers/PSystem.h)
#define PFLAG_LEAVE_TRAIL 131072
// OpenChaos extension: exponential alpha fade — alpha *= (256 - fade)/256 per logic tick.
// Drops fast initially, lingers at low alpha. Matches retail smoke behavior where the
// dark cloud quickly becomes barely visible then takes longer to fully disappear.
// Linear PFLAG_FADE cannot reproduce this two-phase look.
#define PFLAG_FADE_EXP 262144

// ---- Convenience flag combinations ----

// uc_orig: PFLAGS_SMOKE (fallen/Headers/PSystem.h)
#define PFLAGS_SMOKE PFLAG_FADE | PFLAG_RESIZE | PFLAG_WANDER
// uc_orig: PFLAGS_BUBBLE (fallen/Headers/PSystem.h)
#define PFLAGS_BUBBLE PFLAG_BOUYANT | PFLAG_EXPAND

// ---- Particle struct ----

// Single particle in the pool. Fields are laid out for compactness.
// Colour encoding: top byte (bits 31..24) = alpha; lower 3 bytes = RGB.
// With PFLAG_INVALPHA: 0x00 = opaque, 0xFF = transparent (fade-in).
// uc_orig: Particle (fallen/Headers/PSystem.h)
struct Particle {
    SLONG x, y, z;
    SLONG colour, flags, life;
    UWORD page, sprite;
    UBYTE padding, priority;
    SBYTE fade, resize;
    SWORD dx, dy, dz;
    UWORD prev, next;
    UWORD size;
};

// ---- API ----

// uc_orig: PARTICLE_Run (fallen/Source/psystem.cpp)
void PARTICLE_Run();

// uc_orig: PARTICLE_Reset (fallen/Source/psystem.cpp)
void PARTICLE_Reset();

// uc_orig: PARTICLE_AddParticle (fallen/Source/psystem.cpp)
UWORD PARTICLE_AddParticle(Particle& p);

// uc_orig: PARTICLE_Add (fallen/Source/psystem.cpp)
UWORD PARTICLE_Add(SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, UWORD page, UWORD sprite, SLONG colour, SLONG flags, SLONG life, UBYTE size, UBYTE priority, SBYTE fade, SBYTE resize);

// uc_orig: PARTICLE_Draw (fallen/DDEngine/Source/drawxtra.cpp)
// Iterates the live-particle linked list and submits each particle as a sprite quad.
void PARTICLE_Draw();

#endif // ENGINE_EFFECTS_PSYSTEM_H
