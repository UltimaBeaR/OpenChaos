#ifndef ACTORS_CORE_THING_GLOBALS_H
#define ACTORS_CORE_THING_GLOBALS_H

#include "core/types.h"
#include "engine/io/file.h"   // MFFileHandle

// Need THING_INDEX without pulling in all of Game.h
#ifndef THING_INDEX
#define THING_INDEX UWORD
#endif

// Maximum number of things that can be found in a single spatial query.
// uc_orig: THING_ARRAY_SIZE (fallen/Headers/Thing.h)
#define THING_ARRAY_SIZE 32

// Array used as scratch buffer for THING_find_sphere / THING_find_box results.
// uc_orig: THING_array (fallen/Source/Thing.cpp)
extern THING_INDEX THING_array[THING_ARRAY_SIZE];

// Head of the per-class linked list of Things (indexed by CLASS_*).
// uc_orig: thing_class_head (fallen/Source/Thing.cpp)
extern UWORD* thing_class_head;

// Elapsed time in ms for the current tick, unclipped by TICK_TOCK bounds.
// uc_orig: tick_tock_unclipped (fallen/Source/Thing.cpp)
extern SLONG tick_tock_unclipped;

// Class priority ordering for process_things — determines which classes are processed first.
// uc_orig: class_priority (fallen/Source/Thing.cpp)
extern UWORD class_priority[];

// Class list used by for_things() when iterating all active things.
// uc_orig: class_check (fallen/Source/Thing.cpp)
extern UWORD class_check[];

// File handles for gameplay recording and verification (replay system).
// uc_orig: playback_file (fallen/Source/Thing.cpp)
extern MFFileHandle playback_file;
// uc_orig: verifier_file (fallen/Source/Thing.cpp)
extern MFFileHandle verifier_file;

// Maps player slot index to input device type ID (keyboard/joystick).
// uc_orig: input_type (fallen/Source/Thing.cpp)
extern UBYTE input_type[];

// Slow-motion counter: when non-zero, TICK_RATIO is forced to 32 and decremented each frame.
// uc_orig: slow_mo (fallen/Source/Thing.cpp)
extern UWORD slow_mo;

// Frame-rate-independent TICK_RATIO that keeps running even during slowdown/pause.
// uc_orig: REAL_TICK_RATIO (fallen/Source/Thing.cpp)
extern SLONG REAL_TICK_RATIO;

// When set, prevents player things from being returned by THING_find_sphere during cutscenes.
// uc_orig: hit_player (fallen/Source/Thing.cpp)
extern UBYTE hit_player;

#endif // ACTORS_CORE_THING_GLOBALS_H
