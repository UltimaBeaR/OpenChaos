#ifndef CAMERA_VIS_CAM_H
#define CAMERA_VIS_CAM_H

#include "engine/core/types.h"
#include "camera/fc.h" // FC_MAX_CAMS

// VisCam ("visible camera") — post-physics camera layer for collision-aware
// behaviour and any other extra manipulations on top of the default camera.
// Runs once per physics tick after FC_process and feeds render-side
// interpolation.
//
// Why this module exists at all: the default camera logic (FC_process in
// fc.cpp) is dense, interdependent, accumulator-based across ticks, and very
// easy to break by accident — past attempts to edit it directly caused
// regressions that were hard to track down. VisCam lets us layer new camera
// behaviour ON TOP without touching FC_process. The default logic keeps
// running unmodified; we only read its output and compute our own.
//
// Architecture: FC_process (legacy) writes the gameplay camera into FC_cam[*]
// as before. VC_process then reads FC_cam[*] and computes its own output values
// into VC_state[*]. Render-interp now snapshots and lerps VC_state instead of
// FC_cam, so what the player sees is VC_state. FC_cam[*] itself is never
// overwritten — its accumulator-based smoothing in FC_process keeps working
// across ticks unchanged.
//
// Currently identity copy (no real collision logic yet). VC_test_offset_enabled
// is a temporary toggle that adds an upward Y offset for testing the pipeline;
// remove together with the toggle when real logic lands.

struct VC_State {
    SLONG x, y, z;
    SLONG yaw, pitch, roll;
    SLONG lens;
};

extern VC_State VC_state[FC_MAX_CAMS];

// Temporary test toggle (bound to \ key). Adds upward Y offset to VC_state.y
// so the visible camera floats above its FC_process position. Lets us verify
// the post-physics → interpolation pipeline before real logic exists.
extern bool VC_test_offset_enabled;

// Run once per physics tick, after FC_process. Reads FC_cam[*], writes
// VC_state[*].
void VC_process(void);

// Toggle the test offset on/off.
void VC_toggle_test_offset(void);

#endif // CAMERA_VIS_CAM_H
