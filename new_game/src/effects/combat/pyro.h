#ifndef EFFECTS_COMBAT_PYRO_H
#define EFFECTS_COMBAT_PYRO_H

// Pyrotechnics: particle-based fire, explosion, and visual effect objects.
// Each active pyro is a CLASS_PYRO Thing managed by state machine (STATE_INIT -> STATE_NORMAL).
// MAX_PYROS pyro slots (PC = 64); slots are shared across all concurrent effects.

#include "engine/core/types.h"
#include "engine/core/vector.h"
#include "things/core/state.h"

struct Thing;

// uc_orig: MAX_PYROS (fallen/Headers/pyro.h)
#define MAX_PYROS 64

// Pyro type IDs (PyroType field in Pyro struct).
// uc_orig: PYRO_NONE (fallen/Headers/pyro.h)
#define PYRO_NONE 0
// uc_orig: PYRO_FIREWALL (fallen/Headers/pyro.h)
#define PYRO_FIREWALL 1
// uc_orig: PYRO_FIREPOOL (fallen/Headers/pyro.h)
#define PYRO_FIREPOOL 2
// uc_orig: PYRO_BONFIRE (fallen/Headers/pyro.h)
#define PYRO_BONFIRE 3
// uc_orig: PYRO_IMMOLATE (fallen/Headers/pyro.h)
#define PYRO_IMMOLATE 4
// uc_orig: PYRO_EXPLODE (fallen/Headers/pyro.h)
#define PYRO_EXPLODE 5
// uc_orig: PYRO_DUSTWAVE (fallen/Headers/pyro.h)
#define PYRO_DUSTWAVE 6
// uc_orig: PYRO_EXPLODE2 (fallen/Headers/pyro.h)
#define PYRO_EXPLODE2 7
// uc_orig: PYRO_STREAMER (fallen/Headers/pyro.h)
#define PYRO_STREAMER 8
// uc_orig: PYRO_TWANGER (fallen/Headers/pyro.h)
#define PYRO_TWANGER 9
// uc_orig: PYRO_FLICKER (fallen/Headers/pyro.h)
#define PYRO_FLICKER 10
// uc_orig: PYRO_HITSPANG (fallen/Headers/pyro.h)
#define PYRO_HITSPANG 11
// uc_orig: PYRO_FIREBOMB (fallen/Headers/pyro.h)
#define PYRO_FIREBOMB 12
// uc_orig: PYRO_SPLATTERY (fallen/Headers/pyro.h)
#define PYRO_SPLATTERY 13
// uc_orig: PYRO_IRONICWATERFALL (fallen/Headers/pyro.h)
#define PYRO_IRONICWATERFALL 14
// uc_orig: PYRO_NEWDOME (fallen/Headers/pyro.h)
#define PYRO_NEWDOME 15
// uc_orig: PYRO_WHOOMPH (fallen/Headers/pyro.h)
#define PYRO_WHOOMPH 16
// uc_orig: PYRO_GAMEOVER (fallen/Headers/pyro.h)
#define PYRO_GAMEOVER 17

// uc_orig: PYRO_RANGE (fallen/Headers/pyro.h)
#define PYRO_RANGE 18

// Pyro flags (Flags field): general fire behaviour.
// uc_orig: PYRO_FLAGS_INSTANT (fallen/Headers/pyro.h)
#define PYRO_FLAGS_INSTANT 1
// uc_orig: PYRO_FLAGS_WAVE (fallen/Headers/pyro.h)
#define PYRO_FLAGS_WAVE 2
// uc_orig: PYRO_FLAGS_INFINITE (fallen/Headers/pyro.h)
#define PYRO_FLAGS_INFINITE 4
// uc_orig: PYRO_FLAGS_TOUCHPOINT (fallen/Headers/pyro.h)
#define PYRO_FLAGS_TOUCHPOINT 8

// Immolation-specific flags (mutually exclusive with the above set).
// uc_orig: PYRO_FLAGS_FLICKER (fallen/Headers/pyro.h)
#define PYRO_FLAGS_FLICKER 1
// uc_orig: PYRO_FLAGS_BONFIRE (fallen/Headers/pyro.h)
#define PYRO_FLAGS_BONFIRE 2
// uc_orig: PYRO_FLAGS_FLAME (fallen/Headers/pyro.h)
#define PYRO_FLAGS_FLAME 4
// uc_orig: PYRO_FLAGS_SMOKE (fallen/Headers/pyro.h)
#define PYRO_FLAGS_SMOKE 8
// uc_orig: PYRO_FLAGS_STATIC (fallen/Headers/pyro.h)
#define PYRO_FLAGS_STATIC 16

// Main pyro state structure. Stored in the_game.Pyros[].
// uc_orig: Pyro (fallen/Headers/pyro.h)
typedef struct
{
    UWORD Flags; // pyro flags (see PYRO_FLAGS_*)
    UBYTE PyroType; // effect type (see PYRO_* constants)
    UBYTE dlight; // dynamic light handle, if any
    Thing* thing; // back-pointer to the owning Thing
    Thing* victim; // immolation target
    GameCoord target; // fire wall endpoint, or hit location
    SLONG radius; // area effect radius
    SLONG counter; // general-purpose timer
    ULONG radii[8]; // per-type per-point data (ribbon handles, streamer data, etc.)
    SLONG scale; // size modifier
    SLONG tints[2]; // colour tints for light effects
    SLONG soundid; // active sound handle (thing number used as ID)
    SWORD Timer1; // legacy per-thing timer
    UWORD Dummy; // overloaded field: ribbon handle (FLICKER), burn stage (IMMOLATE), etc.
    UBYTE LastSmokeSpawn; // PYRO_BONFIRE: wall-clock phase for smoke spawn edge-detect (FPS-independent)
} Pyro;

// uc_orig: PyroPtr (fallen/Headers/pyro.h)
typedef Pyro* PyroPtr;

// Precalculated hemisphere point (used by PYRO_EXPLODE and PYRO_EXPLODE2 draw code).
// uc_orig: RadPoint (fallen/Headers/pyro.h)
typedef struct
{
    SLONG x, y, z, flags;
} RadPoint;

// Per-point data for old-style hemisphere explosion (PYRO_EXPLODE).
// uc_orig: PyrPoint (fallen/Headers/pyro.h)
typedef struct
{
    UWORD radius, delta;
} PyrPoint;

// Extended pyro for old hemisphere explosion (PYRO_EXPLODE, CLASS_PYRO + Pyrex cast).
// uc_orig: Pyrex (fallen/Headers/pyro.h)
typedef struct
{
    UWORD Flags;
    UBYTE PyroType;
    UBYTE padding;
    Thing* thing;
    PyrPoint points[17]; // 8 bottom ring, 8 top ring, 1 centre
} Pyrex;

// State function tables declared in pyro.cpp, referenced by the_game setup code.
// uc_orig: PYRO_functions (fallen/Source/pyro.cpp)
extern GenusFunctions PYRO_functions[PYRO_RANGE];

// uc_orig: BONFIRE_state_function (fallen/Source/pyro.cpp)
extern StateFunction BONFIRE_state_function[];
// uc_orig: IMMOLATE_state_function (fallen/Source/pyro.cpp)
extern StateFunction IMMOLATE_state_function[];
// uc_orig: EXPLODE_state_function (fallen/Source/pyro.cpp)
extern StateFunction EXPLODE_state_function[];
// uc_orig: DUSTWAVE_state_function (fallen/Source/pyro.cpp)
extern StateFunction DUSTWAVE_state_function[];
// uc_orig: EXPLODE2_state_function (fallen/Source/pyro.cpp)
extern StateFunction EXPLODE2_state_function[];
// uc_orig: STREAMER_state_function (fallen/Source/pyro.cpp)
extern StateFunction STREAMER_state_function[];
// uc_orig: TWANGER_state_function (fallen/Source/pyro.cpp)
extern StateFunction TWANGER_state_function[];
// uc_orig: FIREBOMB_state_function (fallen/Source/pyro.cpp)
extern StateFunction FIREBOMB_state_function[];
// uc_orig: IRONICWATERFALL_state_function (fallen/Source/pyro.cpp)
extern StateFunction IRONICWATERFALL_state_function[];
// uc_orig: NEWDOME_state_function (fallen/Source/pyro.cpp)
extern StateFunction NEWDOME_state_function[];
// uc_orig: PYRO_state_function (fallen/Source/pyro.cpp)
extern StateFunction PYRO_state_function[];

// Resets all pyro slots; called at level load.
// uc_orig: init_pyros (fallen/Source/pyro.cpp)
void init_pyros(void);

// Allocates a pyro slot and creates the associated CLASS_PYRO Thing.
// uc_orig: alloc_pyro (fallen/Source/pyro.cpp)
struct Thing* alloc_pyro(UBYTE type);

// Creates a pyro Thing at position pos and enters STATE_INIT.
// uc_orig: PYRO_create (fallen/Source/pyro.cpp)
Thing* PYRO_create(GameCoord pos, UBYTE type);

// Returns the Pyro struct for a CLASS_PYRO Thing.
// uc_orig: PYRO_get_pyro (fallen/Source/pyro.cpp)
Pyro* PYRO_get_pyro(struct Thing* pyro_thing);

// Creates one or more pyro effects at posn according to a types bitmask.
// Bit 1=TWANGER, 2=EXPLODE2+gust, 4=DUSTWAVE, 8=STREAMER, 16=BONFIRE,
// 32=NEWDOME+gust, 64=FIREBOMB, 128=FIREBOMB+WAVE.
// uc_orig: PYRO_construct (fallen/Source/pyro.cpp)
void PYRO_construct(GameCoord posn, SLONG types, SLONG scale);

// Creates a PYRO_HITSPANG from p_person's left hand toward a victim Thing.
// uc_orig: PYRO_hitspang (fallen/Source/pyro.cpp)
void PYRO_hitspang(Thing* p_person, Thing* victim);

// Creates a PYRO_HITSPANG from p_person's left hand toward a world coordinate.
// uc_orig: PYRO_hitspang (fallen/Source/pyro.cpp)
void PYRO_hitspang(Thing* p_person, SLONG x, SLONG y, SLONG z);

// PYRO_init_state declared in original header but never implemented — dead declaration.
// Kept here for ABI compatibility only.
// uc_orig: PYRO_init_state (fallen/Headers/pyro.h)
void PYRO_init_state(Thing* pyro_thing, UBYTE new_state);

// ---- Drawing functions (from drawxtra.cpp) ----

// uc_orig: IWouldLikeSomePyroSpritesHowManyCanIHave (fallen/DDEngine/Source/drawxtra.cpp)
// Throttling stub: returns iIWantThisMany unchanged on PC (no throttle implemented).
int IWouldLikeSomePyroSpritesHowManyCanIHave(int iIWantThisMany);

// uc_orig: Pyros_EndOfFrameMarker (fallen/DDEngine/Source/drawxtra.cpp)
// Called once per frame to reset throttle counters; no-op on PC.
void Pyros_EndOfFrameMarker(void);

// uc_orig: PYRO_draw_pyro (fallen/DDEngine/Source/drawxtra.cpp)
// Per-frame draw dispatch for a CLASS_PYRO Thing: selects draw routine by PyroType.
void PYRO_draw_pyro(Thing* p_pyro);

#endif // EFFECTS_COMBAT_PYRO_H
