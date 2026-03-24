#ifndef THINGS_CORE_THING_H
#define THINGS_CORE_THING_H

#include "engine/core/types.h"
#include "engine/core/vector.h"          // GameCoord
#include "things/core/drawtype.h" // DrawTween, DrawMesh
// *Ptr typedef headers for all class-specific data stored in Thing.Genus.
// Use the lowest-level header that defines the struct and *Ptr typedef to avoid
// pulling in function declarations (which might conflict with forward decls in .cpp files).
#include "things/vehicles/vehicle.h"   // Vehicle, VehiclePtr
#include "things/vehicles/furn.h"
#include <string.h>                       // anim_types.h uses strcpy in inline methods
#include "things/characters/person_types.h"  // Person, PersonPtr
#include "things/animals/animal.h"     // Animal, AnimalPtr
#include "things/vehicles/chopper.h"   // Chopper, ChopperPtr
#include "effects/combat/pyro.h"              // Pyro, PyroPtr
#include "things/core/player.h"        // Player, PlayerPtr
#include "things/items/projectile.h"   // Projectile, ProjectilePtr
#include "things/items/special.h"      // Special, SpecialPtr
#include "things/core/switch.h"        // Switch, SwitchPtr
#include "effects/environment/tracks.h"            // Track, TrackPtr
#include "world/environment/plat.h"    // Plat, PlatPtr
#include "things/items/barrel.h"       // Barrel, BarrelPtr
#include "things/animals/bat.h"        // Bat, BatPtr

// Object pool limits: 400 primary (characters, vehicles, etc.) + 300 secondary (switches, tracks).
// uc_orig: MAX_PRIMARY_THINGS (fallen/Headers/Thing.h)
#define MAX_PRIMARY_THINGS (400)
// uc_orig: MAX_SECONDARY_THINGS (fallen/Headers/Thing.h)
#define MAX_SECONDARY_THINGS (300)
// uc_orig: MAX_THINGS (fallen/Headers/Thing.h)
#define MAX_THINGS (MAX_PRIMARY_THINGS + MAX_SECONDARY_THINGS + 1)

// Thing.Flags bit definitions.
// uc_orig: FLAGS_ON_MAPWHO (fallen/Headers/Thing.h)
#define FLAGS_ON_MAPWHO (1 << 0)
// uc_orig: FLAGS_IN_VIEW (fallen/Headers/Thing.h)
#define FLAGS_IN_VIEW (1 << 1)
// uc_orig: FLAGS_TWEEN_TO_QUEUED_FRAME (fallen/Headers/Thing.h)
#define FLAGS_TWEEN_TO_QUEUED_FRAME (1 << 2)
// uc_orig: FLAGS_PROJECTILE_MOVEMENT (fallen/Headers/Thing.h)
#define FLAGS_PROJECTILE_MOVEMENT (1 << 3)
// uc_orig: FLAGS_LOCKED_ANIM (fallen/Headers/Thing.h)
#define FLAGS_LOCKED_ANIM (1 << 4)
// uc_orig: FLAGS_IN_BUILDING (fallen/Headers/Thing.h)
#define FLAGS_IN_BUILDING (1 << 5)
// uc_orig: FLAGS_INDOORS_GENERATED (fallen/Headers/Thing.h)
#define FLAGS_INDOORS_GENERATED (1 << 6)
// uc_orig: FLAGS_LOCKED (fallen/Headers/Thing.h)
#define FLAGS_LOCKED (1 << 7)
// uc_orig: FLAGS_CABLE_BUILDING (fallen/Headers/Thing.h)
#define FLAGS_CABLE_BUILDING (1 << 8)
// uc_orig: FLAGS_COLLIDED (fallen/Headers/Thing.h)
#define FLAGS_COLLIDED (1 << 9)
// uc_orig: FLAGS_IN_SEWERS (fallen/Headers/Thing.h)
#define FLAGS_IN_SEWERS (1 << 10)
// uc_orig: FLAGS_SWITCHED_ON (fallen/Headers/Thing.h)
#define FLAGS_SWITCHED_ON (1 << 11)
// uc_orig: FLAGS_PLAYED_FOOTSTEP (fallen/Headers/Thing.h)
#define FLAGS_PLAYED_FOOTSTEP (1 << 12)
// uc_orig: FLAGS_HAS_GUN (fallen/Headers/Thing.h)
#define FLAGS_HAS_GUN (1 << 13)
// uc_orig: FLAGS_BURNING (fallen/Headers/Thing.h)
#define FLAGS_BURNING (1 << 14)
// uc_orig: FLAGS_HAS_ATTACHED_SOUND (fallen/Headers/Thing.h)
#define FLAGS_HAS_ATTACHED_SOUND (1 << 15)
// uc_orig: FLAGS_PERSON_BEEN_SHOT (fallen/Headers/Thing.h)
#define FLAGS_PERSON_BEEN_SHOT (1 << 16)
// uc_orig: FLAGS_PERSON_AIM_AND_RUN (fallen/Headers/Thing.h)
#define FLAGS_PERSON_AIM_AND_RUN (1 << 6)
// uc_orig: FLAG_EDIT_PRIM_ON_FLOOR (fallen/Headers/Thing.h)
#define FLAG_EDIT_PRIM_ON_FLOOR (1 << 2)
// uc_orig: FLAG_EDIT_PRIM_JUST_FLOOR (fallen/Headers/Thing.h)
#define FLAG_EDIT_PRIM_JUST_FLOOR (1 << 3)
// uc_orig: FLAG_EDIT_PRIM_INSIDE (fallen/Headers/Thing.h)
#define FLAG_EDIT_PRIM_INSIDE (1 << 4)
// uc_orig: FLAG_SPECIAL_UNTOUCHED (fallen/Headers/Thing.h)
#define FLAG_SPECIAL_UNTOUCHED (1 << 2)
// uc_orig: FLAG_SPECIAL_HIDDEN (fallen/Headers/Thing.h)
#define FLAG_SPECIAL_HIDDEN (1 << 3)

// The base game object. All interactive entities in the world are Things.
// Polymorphic via StateFn (per-frame state handler) and the Genus union (class-specific data).
// WorldPos coordinates: 1 unit = 1/256 of a map square (256 fixed-point units = 1 map cell).
// uc_orig: Thing (fallen/Headers/Thing.h)
struct Thing {
    UBYTE Class,
        State,
        OldState,
        SubState;
    ULONG Flags;
    THING_INDEX Child,
        Parent,
        LinkChild,
        LinkParent;

    GameCoord WorldPos;

    // Per-frame state handler; set by set_state_function() when state changes.
    void (*StateFn)(Thing*);

    // Rendering data: either a vertex-morphing tween mesh (animated characters) or
    // a static oriented mesh. DrawType determines which union member is active.
    union {
        DrawTween* Tweened;
        DrawMesh* Mesh;
    } Draw;

    // Class-specific data. Active member is determined by Thing.Class.
    union {
        VehiclePtr Vehicle;
        FurniturePtr Furniture;
        PersonPtr Person;
        AnimalPtr Animal;
        ChopperPtr Chopper;
        PyroPtr Pyro;
        ProjectilePtr Projectile;
        PlayerPtr Player;
        SpecialPtr Special;
        SwitchPtr Switch;
        TrackPtr Track;
        PlatPtr Plat;
        BarrelPtr Barrel;
        BatPtr Bat;
    } Genus;

    UBYTE DrawType;
    UBYTE Lead;

    // Velocity-related fields (used by physics code).
    SWORD Velocity;
    SWORD DeltaVelocity;
    SWORD RequiredVelocity;
    SWORD DY;

    UWORD Index;
    SWORD OnFace;

    UWORD NextLink;   // Next thing of the same class in the class linked list.
    UWORD DogPoo1;

    THING_INDEX DogPoo2;  // Temporary field for building unlock switches.
};

// uc_orig: Thing (fallen/Headers/Thing.h)
typedef struct Thing Thing;

// Head of the singly-linked list for each CLASS_* type.
// Iterate by following Thing.NextLink until 0.
// uc_orig: thing_class_head (fallen/Source/Thing.cpp)
extern UWORD* thing_class_head;

// uc_orig: init_things (fallen/Headers/Thing.h)
void init_things(void);
// uc_orig: alloc_primary_thing (fallen/Headers/Thing.h)
THING_INDEX alloc_primary_thing(UWORD thing_class);
// uc_orig: free_primary_thing (fallen/Headers/Thing.h)
void free_primary_thing(THING_INDEX thing);
// uc_orig: alloc_secondary_thing (fallen/Headers/Thing.h)
THING_INDEX alloc_secondary_thing(UWORD secondary_thing);
// uc_orig: free_secondary_thing (fallen/Headers/Thing.h)
void free_secondary_thing(THING_INDEX thing);
// uc_orig: add_thing_to_map (fallen/Headers/Thing.h)
void add_thing_to_map(Thing* t_thing);
// uc_orig: remove_thing_from_map (fallen/Headers/Thing.h)
void remove_thing_from_map(Thing* t_thing);
// uc_orig: move_thing_on_map (fallen/Headers/Thing.h)
void move_thing_on_map(Thing* t_thing, GameCoord* new_position);
// uc_orig: move_thing_on_map_dxdydz (fallen/Source/Thing.cpp)
void move_thing_on_map_dxdydz(Thing* t_thing, SLONG dx, SLONG dy, SLONG dz);
// uc_orig: process_things (fallen/Headers/Thing.h)
void process_things(SLONG f_r_i);
// uc_orig: process_things_tick (fallen/Source/Thing.cpp)
void process_things_tick(SLONG frame_rate_independant);

// uc_orig: log_primary_used_list (fallen/Headers/Thing.h)
void log_primary_used_list(void);
// uc_orig: log_primary_unused_list (fallen/Headers/Thing.h)
void log_primary_unused_list(void);
// uc_orig: log_secondary_used_list (fallen/Source/Thing.cpp)
void log_secondary_used_list(void);
// uc_orig: log_secondary_unused_list (fallen/Source/Thing.cpp)
void log_secondary_unused_list(void);

// uc_orig: alloc_thing (fallen/Headers/Thing.h)
Thing* alloc_thing(SBYTE classification);
// uc_orig: free_thing (fallen/Headers/Thing.h)
void free_thing(Thing* t_thing);

// Inline helper to set world position directly.
// uc_orig: set_thing_pos (fallen/Headers/Thing.h)
inline void set_thing_pos(Thing* t, SLONG x, SLONG y, SLONG z)
{
    t->WorldPos.X = x;
    t->WorldPos.Y = y;
    t->WorldPos.Z = z;
}

// uc_orig: THING_dist_between (fallen/Headers/Thing.h)
SLONG THING_dist_between(Thing* p_thing_a, Thing* p_thing_b);
// uc_orig: THING_kill (fallen/Headers/Thing.h)
void THING_kill(Thing* t);

// Thing class identifiers. Each Thing has a Class field matching one of these.
// CLASS_* values are used as bit-shift positions in the THING_FIND_* bitmasks.
// uc_orig: CLASS_NONE (fallen/Headers/Game.h)
#define CLASS_NONE        0
// uc_orig: CLASS_PLAYER (fallen/Headers/Game.h)
#define CLASS_PLAYER      1
// uc_orig: CLASS_CAMERA (fallen/Headers/Game.h)
#define CLASS_CAMERA      2
// uc_orig: CLASS_PROJECTILE (fallen/Headers/Game.h)
#define CLASS_PROJECTILE  3
// uc_orig: CLASS_BUILDING (fallen/Headers/Game.h)
#define CLASS_BUILDING    4
// uc_orig: CLASS_PERSON (fallen/Headers/Game.h)
#define CLASS_PERSON      5
// uc_orig: CLASS_ANIMAL (fallen/Headers/Game.h)
#define CLASS_ANIMAL      6
// uc_orig: CLASS_FURNITURE (fallen/Headers/Game.h)
#define CLASS_FURNITURE   7
// uc_orig: CLASS_SWITCH (fallen/Headers/Game.h)
#define CLASS_SWITCH      8
// uc_orig: CLASS_VEHICLE (fallen/Headers/Game.h)
#define CLASS_VEHICLE     9
// uc_orig: CLASS_SPECIAL (fallen/Headers/Game.h)
#define CLASS_SPECIAL    10
// uc_orig: CLASS_ANIM_PRIM (fallen/Headers/Game.h)
#define CLASS_ANIM_PRIM  11
// uc_orig: CLASS_CHOPPER (fallen/Headers/Game.h)
#define CLASS_CHOPPER    12
// uc_orig: CLASS_PYRO (fallen/Headers/Game.h)
#define CLASS_PYRO       13
// uc_orig: CLASS_TRACK (fallen/Headers/Game.h)
#define CLASS_TRACK      14
// uc_orig: CLASS_PLAT (fallen/Headers/Game.h)
#define CLASS_PLAT       15
// uc_orig: CLASS_BARREL (fallen/Headers/Game.h)
#define CLASS_BARREL     16
// uc_orig: CLASS_BIKE (fallen/Headers/Game.h)
#define CLASS_BIKE       17
// uc_orig: CLASS_BAT (fallen/Headers/Game.h)
#define CLASS_BAT        18
// uc_orig: CLASS_END (fallen/Headers/Game.h)
#define CLASS_END        19

// Scratch array for spatial queries (size 32).
// uc_orig: THING_ARRAY_SIZE (fallen/Headers/Thing.h)
#define THING_ARRAY_SIZE 32
// uc_orig: THING_array (fallen/Headers/Thing.h)
extern THING_INDEX THING_array[THING_ARRAY_SIZE];

// Class bitmask helpers for spatial search functions.
// uc_orig: THING_FIND_EVERYTHING (fallen/Headers/Thing.h)
#define THING_FIND_EVERYTHING (0xffffffff)
// uc_orig: THING_FIND_PEOPLE (fallen/Headers/Thing.h)
#define THING_FIND_PEOPLE (1 << CLASS_PERSON)
// uc_orig: THING_FIND_LIVING (fallen/Headers/Thing.h)
#define THING_FIND_LIVING ((1 << CLASS_PERSON) | (1 << CLASS_ANIMAL))
// uc_orig: THING_FIND_MOVING (fallen/Headers/Thing.h)
#define THING_FIND_MOVING ((1 << CLASS_PERSON) | (1 << CLASS_ANIMAL) | (1 << CLASS_PROJECTILE))

// Finds all things of the given class bitmask within 'radius' units of (pos_x, pos_y, pos_z).
// Writes up to array_size THING_INDEXes into array and returns the count found.
// uc_orig: THING_find_sphere (fallen/Headers/Thing.h)
SLONG THING_find_sphere(
    SLONG pos_x,
    SLONG pos_y,
    SLONG pos_z,
    SLONG radius,
    THING_INDEX* array,
    SLONG array_size,
    ULONG classes);

// Like THING_find_sphere but uses a 2D axis-aligned box (X/Z plane, Y is ignored).
// uc_orig: THING_find_box (fallen/Headers/Thing.h)
SLONG THING_find_box(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    THING_INDEX* array,
    SLONG array_size,
    ULONG classes);

// Returns the THING_INDEX of the nearest thing of the given class within radius, or 0 if none.
// uc_orig: THING_find_nearest (fallen/Headers/Thing.h)
SLONG THING_find_nearest(
    SLONG centre_x,
    SLONG centre_y,
    SLONG centre_z,
    SLONG radius,
    ULONG classes);

// Like THING_find_nearest but excludes p_person from the results.
// Not declared in the original thing.h; referenced via extern in Person.cpp.
// uc_orig: THING_find_nearest_person (fallen/Source/Thing.cpp)
SLONG THING_find_nearest_person(Thing* p_person, SLONG radius, ULONG classes);

// Tick-independent replay helpers: store/check thing states to/from verifier_file.
// uc_orig: wait_ticks (fallen/Source/Thing.cpp)
void wait_ticks(SLONG wait);
// uc_orig: do_packets (fallen/Source/Thing.cpp)
void do_packets(void);
// uc_orig: set_slow_motion (fallen/Source/Thing.cpp)
void set_slow_motion(UWORD motion);
// uc_orig: copy_important_thing_bits (fallen/Source/Thing.cpp)
void copy_important_thing_bits(const Thing* src, Thing* dst);
// uc_orig: store_thing (fallen/Source/Thing.cpp)
void store_thing(Thing* p_thing);
// uc_orig: check_thing (fallen/Source/Thing.cpp)
void check_thing(Thing* p_thing);
// uc_orig: for_things (fallen/Source/Thing.cpp)
void for_things(void (*fn)(Thing* p_thing));
// uc_orig: store_thing_data (fallen/Source/Thing.cpp)
void store_thing_data(void);
// uc_orig: check_thing_data (fallen/Source/Thing.cpp)
void check_thing_data(void);

#endif // THINGS_CORE_THING_H
