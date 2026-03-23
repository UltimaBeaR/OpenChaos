#ifndef UI_CUTSCENES_OUTRO_OUTRO_CAM_H
#define UI_CUTSCENES_OUTRO_OUTRO_CAM_H

#include "ui/cutscenes/outro/outro_always.h"

// uc_orig: CAM_x (fallen/outro/cam.h)
extern float CAM_x;
// uc_orig: CAM_y (fallen/outro/cam.h)
extern float CAM_y;
// uc_orig: CAM_z (fallen/outro/cam.h)
extern float CAM_z;
// uc_orig: CAM_yaw (fallen/outro/cam.h)
extern float CAM_yaw;
// uc_orig: CAM_pitch (fallen/outro/cam.h)
extern float CAM_pitch;
// uc_orig: CAM_lens (fallen/outro/cam.h)
extern float CAM_lens;
// uc_orig: CAM_matrix (fallen/outro/cam.h)
extern float CAM_matrix[9];

// uc_orig: CAM_init (fallen/outro/cam.h)
// Resets camera to default locked position looking at origin.
void CAM_init(void);

// uc_orig: CAM_process (fallen/outro/cam.h)
// Updates camera position/orientation from keyboard input each frame.
void CAM_process(void);

#endif // UI_CUTSCENES_OUTRO_OUTRO_CAM_H
