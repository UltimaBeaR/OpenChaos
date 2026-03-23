#ifndef EFFECTS_POW_H
#define EFFECTS_POW_H

#include <MFStdLib.h>      // for ASSERT
#include "core/types.h"
#include "world/map/pap.h" // Temporary: for PAP_SIZE_LO (POW_mapwho array size)

// Sprite-based visual explosion effect (no damage, purely cosmetic).
// Each explosion is a POW_Pow that owns a linked list of POW_Sprite particles.

// uc_orig: POW_Sprite (fallen/Headers/pow.h)
typedef struct
{
    UBYTE next;
    UBYTE frame;         // Current animation frame (dies when >= 16).
    UBYTE frame_speed;   // How fast it advances between frames.
    UBYTE damp;          // Motion damping exponent.
    SLONG x;
    SLONG y;
    SLONG z;
    SWORD dx;
    SWORD dy;
    SWORD dz;
    UWORD frame_counter; // Ticks remaining until next frame advance.
} POW_Sprite;

// uc_orig: POW_MAX_SPRITES (fallen/Headers/pow.h)
#define POW_MAX_SPRITES 256

// uc_orig: POW_sprite (fallen/Headers/pow.h)
extern POW_Sprite POW_sprite[POW_MAX_SPRITES];
// uc_orig: POW_sprite_free (fallen/Headers/pow.h)
extern UBYTE POW_sprite_free;

// uc_orig: POW_Pow (fallen/Headers/pow.h)
typedef struct
{
    UBYTE type;
    UBYTE next;
    UBYTE sprite;    // Head of linked list of sprites for this explosion.
    UBYTE mapwho;
    SLONG x;
    SLONG y;
    SLONG z;
    SWORD dx;
    SWORD dy;
    SWORD dz;
    UWORD timer;
    UBYTE flag;
    UBYTE time_warp;
    UWORD padding;
} POW_Pow;

// uc_orig: POW_MAX_POWS (fallen/Headers/pow.h)
#define POW_MAX_POWS 32

// uc_orig: POW_pow (fallen/Headers/pow.h)
extern POW_Pow POW_pow[POW_MAX_POWS];
// uc_orig: POW_pow_free (fallen/Headers/pow.h)
extern UBYTE POW_pow_free;
// uc_orig: POW_pow_used (fallen/Headers/pow.h)
extern UBYTE POW_pow_used;

// 1D mapwho for POW spatial lookup (only 32 bytes).
// uc_orig: POW_mapwho (fallen/Headers/pow.h)
extern UBYTE POW_mapwho[];

// Explosion type constants.
// uc_orig: POW_TYPE_UNUSED (fallen/Headers/pow.h)
#define POW_TYPE_UNUSED              0
// uc_orig: POW_TYPE_BASIC_SPHERE_LARGE (fallen/Headers/pow.h)
#define POW_TYPE_BASIC_SPHERE_LARGE  1
// uc_orig: POW_TYPE_BASIC_SPHERE_MEDIUM (fallen/Headers/pow.h)
#define POW_TYPE_BASIC_SPHERE_MEDIUM 2
// uc_orig: POW_TYPE_BASIC_SPHERE_SMALL (fallen/Headers/pow.h)
#define POW_TYPE_BASIC_SPHERE_SMALL  3
// uc_orig: POW_TYPE_SPAWN_SPHERE_LARGE (fallen/Headers/pow.h)
#define POW_TYPE_SPAWN_SPHERE_LARGE  4
// uc_orig: POW_TYPE_SPAWN_SPHERE_MEDIUM (fallen/Headers/pow.h)
#define POW_TYPE_SPAWN_SPHERE_MEDIUM 5
// uc_orig: POW_TYPE_MULTISPAWN (fallen/Headers/pow.h)
#define POW_TYPE_MULTISPAWN          6
// uc_orig: POW_TYPE_MULTISPAWN_SEMI (fallen/Headers/pow.h)
#define POW_TYPE_MULTISPAWN_SEMI     7
// uc_orig: POW_TYPE_NUMBER (fallen/Headers/pow.h)
#define POW_TYPE_NUMBER              8

// High-level explosion presets passed to POW_create().
// uc_orig: POW_CREATE_LARGE_SEMI (fallen/Headers/pow.h)
#define POW_CREATE_LARGE_SEMI 0
// uc_orig: POW_CREATE_MEDIUM (fallen/Headers/pow.h)
#define POW_CREATE_MEDIUM     1
// uc_orig: POW_CREATE_LARGE (fallen/Headers/pow.h)
#define POW_CREATE_LARGE      2

// Resets all explosion state (called on init or when pools overflow).
// uc_orig: POW_init (fallen/Headers/pow.h)
void POW_init(void);

// Creates a new explosion of the given internal type at (x,y,z) with initial velocity.
// Positions and velocities are in high-res coordinates (16 bits per map square).
// uc_orig: POW_new (fallen/Headers/pow.h)
void POW_new(SLONG type, SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz);

// Creates an explosion using a simplified 3-level preset (POW_CREATE_*).
// uc_orig: POW_create (fallen/Headers/pow.h)
inline void POW_create(SLONG which, SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz)
{
    switch (which) {
    case POW_CREATE_LARGE_SEMI:
        POW_new(POW_TYPE_MULTISPAWN_SEMI, x, y, z, dx, dy, dz);
        break;
    case POW_CREATE_MEDIUM:
        POW_new(POW_TYPE_SPAWN_SPHERE_MEDIUM, x, y, z, dx, dy, dz);
        break;
    case POW_CREATE_LARGE:
        POW_new(POW_TYPE_MULTISPAWN, x, y, z, dx, dy, dz);
        break;
    default:
        ASSERT(0);
        break;
    }
}

// Advances all active explosions by one game tick — animates sprites,
// moves them, and handles child-spawning chains.
// uc_orig: POW_process (fallen/Headers/pow.h)
void POW_process(void);

#endif // EFFECTS_POW_H
