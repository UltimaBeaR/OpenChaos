#ifndef MISSIONS_SAVE_GLOBALS_H
#define MISSIONS_SAVE_GLOBALS_H

#include <stdio.h>
#include "engine/core/types.h"

// Shared file handle used by all SAVE_out_data / LOAD_in_data calls during a save or load.
// uc_orig: SAVE_handle (fallen/Source/save.cpp)
extern FILE* SAVE_handle;

// Tag byte written for things that should be skipped on load (e.g. wandering civs/drivers).
// uc_orig: skip (fallen/Source/save.cpp)
extern UBYTE skip;

// Tag byte written when a CLASS_NONE thing slot is encountered.
// uc_orig: skip_class_none (fallen/Source/save.cpp)
extern UBYTE skip_class_none;

#endif // MISSIONS_SAVE_GLOBALS_H
