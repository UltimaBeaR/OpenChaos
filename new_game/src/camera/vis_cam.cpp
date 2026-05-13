#include "camera/vis_cam.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"

VC_State VC_state[FC_MAX_CAMS] = {};
bool VC_test_offset_enabled = false;

// Temporary test offset in world Y units, applied when VC_test_offset_enabled
// is true. Approximately half of Darci's visible height by feel. Tune by
// number if visibly too small / too large during the test. Goes away with
// the toggle when real collision logic lands.
static constexpr SLONG VC_TEST_OFFSET_Y = 0x8000;

void VC_process(void)
{
    for (SLONG cam = 0; cam < FC_MAX_CAMS; cam++) {
        FC_Cam* fc = &FC_cam[cam];
        VC_State& vc = VC_state[cam];

        // Identity copy of FC_process output. Collision logic will mutate
        // vc.* here before it lands in render-interp.
        vc.x = fc->x;
        vc.y = fc->y;
        vc.z = fc->z;
        vc.yaw = fc->yaw;
        vc.pitch = fc->pitch;
        vc.roll = fc->roll;
        vc.lens = fc->lens;

        if (VC_test_offset_enabled) {
            vc.y += VC_TEST_OFFSET_Y;
        }
    }
}

void VC_toggle_test_offset(void)
{
    VC_test_offset_enabled = !VC_test_offset_enabled;
}
