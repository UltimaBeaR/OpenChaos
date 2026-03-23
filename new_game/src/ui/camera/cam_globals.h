#ifndef UI_CAMERA_CAM_GLOBALS_H
#define UI_CAMERA_CAM_GLOBALS_H

#include "core/types.h"

// Global state for the game camera (cam.cpp).

// Current lens hint (written by the camera, read by the rendering engine).
// Not used internally by the camera module — it's a signal to the engine.
// uc_orig: CAM_lens (fallen/Source/cam.cpp)
extern SLONG CAM_lens;

#endif // UI_CAMERA_CAM_GLOBALS_H
