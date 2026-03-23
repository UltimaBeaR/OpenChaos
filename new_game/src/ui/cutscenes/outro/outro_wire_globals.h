#ifndef UI_CUTSCENES_OUTRO_OUTRO_WIRE_GLOBALS_H
#define UI_CUTSCENES_OUTRO_OUTRO_WIRE_GLOBALS_H

#include "ui/cutscenes/outro/outro_imp.h"
#include "ui/cutscenes/outro/outro_os.h"

// uc_orig: WIRE_NUM_MESHES (fallen/outro/wire.cpp)
#define WIRE_NUM_MESHES 4

// uc_orig: WIRE_mesh (fallen/outro/wire.cpp)
extern IMP_Mesh WIRE_mesh[WIRE_NUM_MESHES];

// uc_orig: WIRE_ot_line (fallen/outro/wire.cpp)
// Texture used to draw wireframe edges.
extern OS_Texture* WIRE_ot_line;

// uc_orig: WIRE_ot_dot (fallen/outro/wire.cpp)
// Texture used for specular highlight dots on wires.
extern OS_Texture* WIRE_ot_dot;

// uc_orig: WIRE_last (fallen/outro/wire.cpp)
extern SLONG WIRE_last;

// uc_orig: WIRE_now (fallen/outro/wire.cpp)
extern SLONG WIRE_now;

// The bounding box used as a z-buffer hack to mask behind the spinning mesh.
// uc_orig: WIRE_Point (fallen/outro/wire.cpp)
typedef struct
{
    float x;
    float y;
    float z;

    OS_Vert ov;

} WIRE_Point;

// uc_orig: WIRE_NUM_POINTS (fallen/outro/wire.cpp)
#define WIRE_NUM_POINTS 8

// uc_orig: WIRE_point (fallen/outro/wire.cpp)
extern WIRE_Point WIRE_point[WIRE_NUM_POINTS];

// uc_orig: WIRE_MODE_NONE_WIRE (fallen/outro/wire.cpp)
#define WIRE_MODE_NONE_WIRE 0
// uc_orig: WIRE_MODE_WIRE_BRIGHT (fallen/outro/wire.cpp)
#define WIRE_MODE_WIRE_BRIGHT 1
// uc_orig: WIRE_MODE_BRIGHT_TEXTURE (fallen/outro/wire.cpp)
#define WIRE_MODE_BRIGHT_TEXTURE 2
// uc_orig: WIRE_MODE_TEXTURE_NONE (fallen/outro/wire.cpp)
#define WIRE_MODE_TEXTURE_NONE 3

// uc_orig: WIRE_current_mesh (fallen/outro/wire.cpp)
extern SLONG WIRE_current_mesh;
// uc_orig: WIRE_current_mode (fallen/outro/wire.cpp)
extern SLONG WIRE_current_mode;
// uc_orig: WIRE_current_countdown (fallen/outro/wire.cpp)
extern SLONG WIRE_current_countdown;

#endif // UI_CUTSCENES_OUTRO_OUTRO_WIRE_GLOBALS_H
