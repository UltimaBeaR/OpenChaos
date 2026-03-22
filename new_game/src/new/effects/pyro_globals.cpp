// Global state and dispatch tables for the pyrotechnics system.
#include "game.h"
#include "effects/pyro_globals.h"
#include "statedef.h"

// uc_orig: PYRO_defaultpoints (fallen/Source/pyro.cpp)
RadPoint PYRO_defaultpoints[16];

// uc_orig: global_spang_count (fallen/Source/pyro.cpp)
SBYTE global_spang_count = 0;

// uc_orig: col_with (fallen/Source/pyro.cpp)
UWORD col_with[PYRO_MAX_COL_WITH];

// Forward declarations: state functions defined in pyro.cpp.
// uc_orig: PYRO_fn_init (fallen/Source/pyro.cpp)
extern void PYRO_fn_init(Thing*);
// uc_orig: PYRO_fn_normal (fallen/Source/pyro.cpp)
extern void PYRO_fn_normal(Thing*);
// uc_orig: PYRO_fn_init_ex (fallen/Source/pyro.cpp)
extern void PYRO_fn_init_ex(Thing*);
// uc_orig: PYRO_fn_normal_ex (fallen/Source/pyro.cpp)
extern void PYRO_fn_normal_ex(Thing*);

// uc_orig: BONFIRE_state_function (fallen/Source/pyro.cpp)
StateFunction BONFIRE_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: IMMOLATE_state_function (fallen/Source/pyro.cpp)
StateFunction IMMOLATE_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: EXPLODE_state_function (fallen/Source/pyro.cpp)
StateFunction EXPLODE_state_function[] = {
    { STATE_INIT, PYRO_fn_init_ex },
    { STATE_NORMAL, PYRO_fn_normal_ex }
};

// uc_orig: DUSTWAVE_state_function (fallen/Source/pyro.cpp)
StateFunction DUSTWAVE_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: EXPLODE2_state_function (fallen/Source/pyro.cpp)
StateFunction EXPLODE2_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: STREAMER_state_function (fallen/Source/pyro.cpp)
StateFunction STREAMER_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: TWANGER_state_function (fallen/Source/pyro.cpp)
StateFunction TWANGER_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: FIREBOMB_state_function (fallen/Source/pyro.cpp)
StateFunction FIREBOMB_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: IRONICWATERFALL_state_function (fallen/Source/pyro.cpp)
StateFunction IRONICWATERFALL_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: NEWDOME_state_function (fallen/Source/pyro.cpp)
StateFunction NEWDOME_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: PYRO_state_function (fallen/Source/pyro.cpp)
StateFunction PYRO_state_function[] = {
    { STATE_INIT, PYRO_fn_init },
    { STATE_NORMAL, PYRO_fn_normal }
};

// uc_orig: pyro_seed (fallen/DDEngine/Source/drawxtra.cpp)
SLONG pyro_seed = 0;

// Dispatch table: maps PyroType index to per-type state function array.
// uc_orig: PYRO_functions (fallen/Source/pyro.cpp)
GenusFunctions PYRO_functions[PYRO_RANGE] = {
    { PYRO_NONE, NULL },
    { PYRO_FIREWALL, PYRO_state_function },
    { PYRO_FIREPOOL, PYRO_state_function },
    { PYRO_BONFIRE, BONFIRE_state_function },
    { PYRO_IMMOLATE, IMMOLATE_state_function },
    { PYRO_EXPLODE, EXPLODE_state_function },
    { PYRO_DUSTWAVE, DUSTWAVE_state_function },
    { PYRO_EXPLODE2, EXPLODE2_state_function },
    { PYRO_STREAMER, STREAMER_state_function },
    { PYRO_TWANGER, TWANGER_state_function },
    { PYRO_FLICKER, PYRO_state_function },
    { PYRO_HITSPANG, PYRO_state_function },
    { PYRO_FIREBOMB, FIREBOMB_state_function },
    { PYRO_SPLATTERY, PYRO_state_function },
    { PYRO_IRONICWATERFALL, IRONICWATERFALL_state_function },
    { PYRO_NEWDOME, NEWDOME_state_function },
    { PYRO_WHOOMPH, PYRO_state_function },
    { PYRO_GAMEOVER, PYRO_state_function },
};
