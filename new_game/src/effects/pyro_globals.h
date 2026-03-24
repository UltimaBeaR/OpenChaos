#ifndef EFFECTS_PYRO_GLOBALS_H
#define EFFECTS_PYRO_GLOBALS_H

#include "engine/core/types.h"
#include "effects/pyro.h"

// Precalculated hemisphere vertex positions for PYRO_EXPLODE old-style explosion.
// Defined in pyro_globals.cpp, referenced by drawxtra.cpp rendering code via extern.
// uc_orig: PYRO_defaultpoints (fallen/Source/pyro.cpp)
extern RadPoint PYRO_defaultpoints[16];

// Number of concurrent PYRO_HITSPANG effects currently active.
// Capped at 5 to avoid overlapping shot impacts. Shared with hitspang callers.
// uc_orig: global_spang_count (fallen/Source/pyro.cpp)
extern SBYTE global_spang_count;

// Shared temporary buffer for sphere find results.
// Referenced by pyro.cpp (MergeSoundFX) and Person.cpp via extern declarations.
// Renamed PYRO_MAX_COL_WITH from MAX_COL_WITH to avoid name collision with local stack arrays.
// uc_orig: MAX_COL_WITH (fallen/Source/pyro.cpp)
#define PYRO_MAX_COL_WITH 16
// uc_orig: col_with (fallen/Source/pyro.cpp)
extern UWORD col_with[PYRO_MAX_COL_WITH];

// Per-type state function tables (STATE_INIT + STATE_NORMAL handlers).
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

// Dispatch table: PyroType index → state function array.
// uc_orig: PYRO_functions (fallen/Source/pyro.cpp)
extern GenusFunctions PYRO_functions[PYRO_RANGE];

// LCG state for deterministic per-frame fire element placement.
// uc_orig: pyro_seed (fallen/DDEngine/Source/drawxtra.cpp)
extern SLONG pyro_seed;

// Precalculated sphere vertex positions for PYRO_EXPLODE2 and PYRO_NEWDOME.
// 4 rings of 8 points each; flags=1 when initialised (lazy init in PYRO_draw_explosion2).
// uc_orig: PYRO_defaultpoints2 (fallen/DDEngine/Source/drawxtra.cpp)
extern RadPoint PYRO_defaultpoints2[32];

#endif // EFFECTS_PYRO_GLOBALS_H
