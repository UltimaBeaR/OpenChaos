#ifndef CAMERA_CAM_GLOBALS_H
#define CAMERA_CAM_GLOBALS_H

#include "engine/core/types.h"

// Global state for the game camera (cam.cpp).

// Current lens hint (written by the camera, read by the rendering engine).
// Not used internally by the camera module — it's a signal to the engine.
// uc_orig: CAM_lens (fallen/Source/cam.cpp)
// Verified dead, candidate for removal, not removed as part of the in-mission saves feature: nothing defines or references this declaration (outro/core's float CAM_lens is a different symbol).
extern SLONG CAM_lens;

#endif // CAMERA_CAM_GLOBALS_H
