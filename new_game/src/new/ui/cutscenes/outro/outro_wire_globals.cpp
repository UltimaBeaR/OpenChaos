#include "ui/cutscenes/outro/outro_wire_globals.h"

// uc_orig: WIRE_mesh (fallen/outro/wire.cpp)
IMP_Mesh WIRE_mesh[WIRE_NUM_MESHES];

// uc_orig: WIRE_ot_line (fallen/outro/wire.cpp)
OS_Texture* WIRE_ot_line = nullptr;

// uc_orig: WIRE_ot_dot (fallen/outro/wire.cpp)
OS_Texture* WIRE_ot_dot = nullptr;

// uc_orig: WIRE_last (fallen/outro/wire.cpp)
SLONG WIRE_last = 0;

// uc_orig: WIRE_now (fallen/outro/wire.cpp)
SLONG WIRE_now = 0;

// uc_orig: WIRE_point (fallen/outro/wire.cpp)
WIRE_Point WIRE_point[WIRE_NUM_POINTS];

// uc_orig: WIRE_current_mesh (fallen/outro/wire.cpp)
SLONG WIRE_current_mesh = 0;

// uc_orig: WIRE_current_mode (fallen/outro/wire.cpp)
SLONG WIRE_current_mode = 0;

// uc_orig: WIRE_current_countdown (fallen/outro/wire.cpp)
SLONG WIRE_current_countdown = 0;
