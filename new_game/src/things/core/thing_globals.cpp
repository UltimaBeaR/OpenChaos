#include "things/core/thing.h"
#include "things/core/thing_globals.h"

// uc_orig: THING_array (fallen/Source/Thing.cpp)
THING_INDEX THING_array[THING_ARRAY_SIZE];

// uc_orig: thing_class_head (fallen/Source/Thing.cpp)
UWORD* thing_class_head;

// uc_orig: tick_tock_unclipped (fallen/Source/Thing.cpp)
SLONG tick_tock_unclipped = 0;

// uc_orig: class_priority (fallen/Source/Thing.cpp)
UWORD class_priority[] = {
    CLASS_CAMERA,
    CLASS_BARREL,
    CLASS_BARREL,
    CLASS_PLAT,
    CLASS_PROJECTILE,
    CLASS_ANIMAL,
    CLASS_SWITCH,
    CLASS_VEHICLE,
    CLASS_SPECIAL,
    CLASS_ANIM_PRIM,
    CLASS_CHOPPER,
    CLASS_PYRO,
    CLASS_BIKE,
    CLASS_BAT,
    0
};

// uc_orig: class_check (fallen/Source/Thing.cpp)
UWORD class_check[] = {
    CLASS_PLAYER,
    CLASS_PERSON,
    CLASS_CAMERA,
    CLASS_BARREL,
    CLASS_BARREL,
    CLASS_PLAT,
    CLASS_PROJECTILE,
    CLASS_ANIMAL,
    CLASS_SWITCH,
    CLASS_VEHICLE,
    CLASS_SPECIAL,
    CLASS_ANIM_PRIM,
    CLASS_CHOPPER,
    CLASS_PYRO,
    CLASS_BIKE,
    CLASS_BAT,
    0
};

// uc_orig: playback_file (fallen/Source/Thing.cpp)
MFFileHandle playback_file;

// uc_orig: verifier_file (fallen/Source/Thing.cpp)
MFFileHandle verifier_file;

// uc_orig: input_type (fallen/Source/Thing.cpp)
UBYTE input_type[] = { 1, 2, 0, 0, 0, 0, 0, 0 };

// uc_orig: slow_mo (fallen/Source/Thing.cpp)
UWORD slow_mo = 0;

// uc_orig: REAL_TICK_RATIO (fallen/Source/Thing.cpp)
SLONG REAL_TICK_RATIO = 256;

// uc_orig: hit_player (fallen/Source/Thing.cpp)
UBYTE hit_player = 0;
