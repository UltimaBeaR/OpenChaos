#ifndef THINGS_VEHICLES_CHOPPER_H
#define THINGS_VEHICLES_CHOPPER_H

#include <string.h>
#include "engine/core/types.h"
#include "engine/core/vector.h"
#include "things/core/state.h"
#include "things/core/drawtype.h"


struct Thing;

// uc_orig: MAX_CHOPPERS (fallen/Headers/chopper.h)
#define MAX_CHOPPERS 4

// uc_orig: CHOPPER_NONE (fallen/Headers/chopper.h)
#define CHOPPER_NONE 0
// uc_orig: CHOPPER_CIVILIAN (fallen/Headers/chopper.h)
#define CHOPPER_CIVILIAN 1
// uc_orig: CHOPPER_NUMB (fallen/Headers/chopper.h)
#define CHOPPER_NUMB 2

// Chopper sub-state codes.
// uc_orig: CHOPPER_substate_idle (fallen/Headers/chopper.h)
#define CHOPPER_substate_idle 0
// uc_orig: CHOPPER_substate_takeoff (fallen/Headers/chopper.h)
#define CHOPPER_substate_takeoff 1
// uc_orig: CHOPPER_substate_landing (fallen/Headers/chopper.h)
#define CHOPPER_substate_landing 2
// uc_orig: CHOPPER_substate_landed (fallen/Headers/chopper.h)
#define CHOPPER_substate_landed 3
// uc_orig: CHOPPER_substate_tracking (fallen/Headers/chopper.h)
#define CHOPPER_substate_tracking 4
// uc_orig: CHOPPER_substate_homing (fallen/Headers/chopper.h)
#define CHOPPER_substate_homing 5
// uc_orig: CHOPPER_substate_patrolling (fallen/Headers/chopper.h)
#define CHOPPER_substate_patrolling 6

// uc_orig: Chopper (fallen/Headers/chopper.h)
// Per-chopper instance data stored in the_game.Choppers[].
typedef struct
{
    Thing* thing;         // Back-pointer to the Thing.
    Thing* target;        // Thing being tracked.
    GameCoord home;       // Home position — returns here when not engaged.
    ULONG dist;           // Distance to target (used for speed selection).
    SLONG radius;         // Radius around home before homing state triggers.
    SLONG patrol;         // Rotation counter for patrolling sub-state.
    SLONG spotx, spotz;   // Beam target coordinates.
    SLONG spotdx, spotdz; // Beam target deltas.
    SLONG dx, dy, dz;     // Motion vector applied each tick.
    SLONG victim;         // EWAY person index to track.
    SWORD rx, ry, rz;     // Rotation deltas.
    UWORD counter;        // General-purpose timer.
    UWORD rotors;         // Rotor rotation angle.
    UWORD rotorspeed;     // Rotor spin speed.
    UWORD speed;          // Preferred cruising speed.
    UBYTE ChopperType;    // CHOPPER_CIVILIAN etc.
    UBYTE rotorprim;      // Prim index for rotor blades.
    UBYTE substate;       // Current sub-state.
    UBYTE light;          // Spotlight brightness.
    UBYTE since_takeoff;  // Ticks since takeoff (capped at 255).
    UBYTE padding;
} Chopper;

// uc_orig: ChopperPtr (fallen/Headers/chopper.h)
typedef Chopper* ChopperPtr;

// uc_orig: CHOPPER_functions (fallen/Headers/chopper.h)
extern GenusFunctions CHOPPER_functions[CHOPPER_NUMB];

// uc_orig: CIVILIAN_state_function (fallen/Headers/chopper.h)
extern StateFunction CIVILIAN_state_function[];

// uc_orig: init_choppers (fallen/Headers/chopper.h)
// Zeros the Choppers array in the_game and resets CHOPPER_COUNT.
void init_choppers(void);

// uc_orig: alloc_chopper (fallen/Headers/chopper.h)
// Allocates a new Chopper Thing of the given type.
struct Thing* alloc_chopper(UBYTE type);

// uc_orig: CHOPPER_create (fallen/Headers/chopper.h)
// Creates a Chopper Thing at pos, adds it to the map and triggers STATE_INIT.
Thing* CHOPPER_create(GameCoord pos, UBYTE type);

// uc_orig: CHOPPER_init_state (fallen/Headers/chopper.h)
// Transitions a chopper to a new sub-state, setting up any required counters.
void CHOPPER_init_state(Thing* chopper_thing, UBYTE new_state);

// uc_orig: CHOPPER_get_chopper (fallen/Headers/chopper.h)
// Returns the Chopper struct associated with a chopper Thing.
Chopper* CHOPPER_get_chopper(Thing* chopper_thing);

// uc_orig: CHOPPER_get_drawmesh (fallen/Headers/chopper.h)
// Returns the DrawMesh struct associated with a chopper Thing.
DrawMesh* CHOPPER_get_drawmesh(Thing* chopper_thing);

// uc_orig: CHOPPER_fn_init (fallen/Source/chopper.cpp)
// STATE_INIT handler: saves home position and transitions to STATE_NORMAL.
void CHOPPER_fn_init(Thing* thing);

// uc_orig: CHOPPER_fn_normal (fallen/Source/chopper.cpp)
// STATE_NORMAL handler: main chopper AI loop — rotor sound, movement, sub-state dispatch.
void CHOPPER_fn_normal(Thing* thing);

// uc_orig: CHOPPER_altitude (fallen/Source/chopper.cpp)
// Returns the chopper's altitude above the terrain below it (in fixed-point units).
SLONG CHOPPER_altitude(Thing* thing);

// uc_orig: CHOPPER_home (fallen/Source/chopper.cpp)
// Steers the chopper toward new_pos: updates heading, speed, and avoidance logic.
void CHOPPER_home(Thing* thing, GameCoord new_pos);

// uc_orig: CHOPPER_limit (fallen/Source/chopper.cpp)
// Clamps the chopper's dx/dz momentum to its current speed, using a weighted average.
void CHOPPER_limit(Chopper* chopper);

// uc_orig: CHOPPER_damp (fallen/Source/chopper.cpp)
// Applies velocity damping to dx/dz (multiplies by ~0.9 each call).
void CHOPPER_damp(Chopper* chopper, UBYTE factor);

// uc_orig: CHOPPER_radius_broken (fallen/Source/chopper.cpp)
// Returns 1 if the distance from pnt to ctr exceeds radius.
UBYTE CHOPPER_radius_broken(GameCoord pnt, GameCoord ctr, SLONG radius);

// uc_orig: CHOPPER_predict_altitude (fallen/Source/chopper.cpp)
// Looks ahead one step and adjusts dy to maintain the desired altitude.
void CHOPPER_predict_altitude(Thing* thing, Chopper* chopper);

// uc_orig: CHOPPER_draw_chopper (fallen/DDEngine/Source/drawxtra.cpp)
// Draws the chopper mesh and rotor, then projects a cone spotlight if the searchlight is on.
void CHOPPER_draw_chopper(Thing* p_chopper);

#endif // THINGS_VEHICLES_CHOPPER_H
