#ifndef WORLD_MAP_QEDIT_GLOBALS_H
#define WORLD_MAP_QEDIT_GLOBALS_H

#include "core/types.h"
#include "world/map/qmap.h"

// Camera focus point in world space (<<8 fixed-point).
// uc_orig: QEDIT_focus_x (fallen/Source/qedit.cpp)
extern SLONG QEDIT_focus_x;
// uc_orig: QEDIT_focus_y (fallen/Source/qedit.cpp)
extern SLONG QEDIT_focus_y;
// uc_orig: QEDIT_focus_z (fallen/Source/qedit.cpp)
extern SLONG QEDIT_focus_z;

// Camera euler angles (game-unit, 0–2047).
// uc_orig: QEDIT_cam_yaw (fallen/Source/qedit.cpp)
extern SLONG QEDIT_cam_yaw;
// uc_orig: QEDIT_cam_pitch (fallen/Source/qedit.cpp)
extern SLONG QEDIT_cam_pitch;
// uc_orig: QEDIT_cam_roll (fallen/Source/qedit.cpp)
extern SLONG QEDIT_cam_roll;
// uc_orig: QEDIT_cam_zoom (fallen/Source/qedit.cpp)
extern SLONG QEDIT_cam_zoom;

// Camera world position derived from focus + matrix + zoom.
// uc_orig: QEDIT_cam_x (fallen/Source/qedit.cpp)
extern SLONG QEDIT_cam_x;
// uc_orig: QEDIT_cam_y (fallen/Source/qedit.cpp)
extern SLONG QEDIT_cam_y;
// uc_orig: QEDIT_cam_z (fallen/Source/qedit.cpp)
extern SLONG QEDIT_cam_z;

// Current camera rotation matrix (3x3 row-major).
// uc_orig: QEDIT_cam_matrix (fallen/Source/qedit.cpp)
extern SLONG QEDIT_cam_matrix[9];

// Scratch QMAP_Draw used by QEDIT_draw_mapsquare.
// uc_orig: QEDIT_qmap_draw (fallen/Source/qedit.cpp)
extern QMAP_Draw QEDIT_qmap_draw;

#endif // WORLD_MAP_QEDIT_GLOBALS_H
