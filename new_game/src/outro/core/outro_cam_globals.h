#ifndef OUTRO_CORE_OUTRO_CAM_GLOBALS_H
#define OUTRO_CORE_OUTRO_CAM_GLOBALS_H

#include "outro/core/outro_always.h"

// uc_orig: CAM_type (fallen/outro/cam.cpp)
// 0 = locked-to-focus, 1 = free-fly.
extern SLONG CAM_type;

// uc_orig: CAM_x (fallen/outro/cam.cpp)
extern float CAM_x;
// uc_orig: CAM_y (fallen/outro/cam.cpp)
extern float CAM_y;
// uc_orig: CAM_z (fallen/outro/cam.cpp)
extern float CAM_z;
// uc_orig: CAM_yaw (fallen/outro/cam.cpp)
extern float CAM_yaw;
// uc_orig: CAM_pitch (fallen/outro/cam.cpp)
extern float CAM_pitch;
// uc_orig: CAM_lens (fallen/outro/cam.cpp)
extern float CAM_lens;
// uc_orig: CAM_dist (fallen/outro/cam.cpp)
// Distance from focus point when in locked mode.
extern float CAM_dist;
// uc_orig: CAM_matrix (fallen/outro/cam.cpp)
extern float CAM_matrix[9];
// uc_orig: CAM_focus_x (fallen/outro/cam.cpp)
extern float CAM_focus_x;
// uc_orig: CAM_focus_y (fallen/outro/cam.cpp)
extern float CAM_focus_y;
// uc_orig: CAM_focus_z (fallen/outro/cam.cpp)
extern float CAM_focus_z;

#endif // OUTRO_CORE_OUTRO_CAM_GLOBALS_H
