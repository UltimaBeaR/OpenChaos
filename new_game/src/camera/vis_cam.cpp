#include "camera/vis_cam.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "engine/console/console.h" // CONSOLE_text_at — temporary debug overlay
#include "engine/graphics/pipeline/poly.h" // POLY_screen_width/height — for centre calc
#include "engine/graphics/pipeline/aeng.h" // AENG_world_text — temporary focus marker

VC_State VC_state[FC_MAX_CAMS] = {};

// Focus point passed into the processing layer. World-space, shifted units
// (matching FC_cam fields and WorldPos). Source of the value is decided in
// VC_process; the processing function below treats it as opaque input. If
// we later swap the source for a better point, only the assignment in
// VC_process changes — processing stays put.
struct VC_FocusPoint {
    SLONG x;
    SLONG y;
    SLONG z;
};

// Processing layer for one camera. Reads the identity-copy `vc` plus the
// focus point and mutates `vc.*`. This is where the new collision logic
// will live.
static void vc_process_one(FC_Cam* fc, VC_State& vc, const VC_FocusPoint& focus)
{
    (void)fc;
    (void)vc;
    (void)focus;

    // Example of what a processing mutation looks like — unconditional
    // upward nudge of the visible camera by ~half a Darci height:
    //
    //     vc.y += 0x8000;
}

void VC_process(void)
{
    for (SLONG cam = 0; cam < FC_MAX_CAMS; cam++) {
        FC_Cam* fc = &FC_cam[cam];
        VC_State& vc = VC_state[cam];

        // Identity copy of FC_process output. Processing layer mutates vc.*
        // below.
        vc.x = fc->x;
        vc.y = fc->y;
        vc.z = fc->z;
        vc.yaw = fc->yaw;
        vc.pitch = fc->pitch;
        vc.roll = fc->roll;
        vc.lens = fc->lens;

        // Focus point passed into processing. Currently the FC_look_at_focus
        // want-target: focus_x/z (object position) and focus_y + lookabove
        // (Y-target the camera aims at). Treated as opaque by vc_process_one
        // — if a better focus source is found, only this construction
        // changes.
        VC_FocusPoint focus = {
            fc->focus_x,
            fc->focus_y + fc->lookabove,
            fc->focus_z,
        };

        vc_process_one(fc, vc, focus);
    }

    // TEMPORARY: anchor text at the logical screen centre. Top-left corner
    // of the text sits at the centre point. Centre is derived dynamically
    // from POLY_screen_width/height — POLY_screen_width = DisplayHeight ×
    // aspect, so on 16:9 it is ~853 and the fixed 320 hardcode would be far
    // off centre. Used as a visible reference while wiring up the new
    // collision logic. Remove together with VC_test_offset_* when real
    // logic lands.
    SLONG cx = (SLONG)(POLY_screen_width * 0.5F);
    SLONG cy = (SLONG)(POLY_screen_height * 0.5F);
    CONSOLE_text_at(cx, cy, 1000, (CBYTE*)"center");
}

// TEMPORARY focus marker — see header note.
void VC_debug_draw_focus(void)
{
    FC_Cam* fc = &FC_cam[0];
    if (fc->focus == NULL) return;

    // FC_calc_focus stores focus_x/y/z as the focus *object* position (for a
    // person on the ground that's WorldPos, i.e. model root — feet level).
    // The point the camera actually aims at is (focus_x, focus_y + lookabove,
    // focus_z) — FC_look_at_focus adds lookabove (~0xa000, decays to 0 on
    // death) to focus_y. So lookabove lifts the aim point to roughly
    // chest/head level. This is the real "what the camera looks at" point,
    // and the one we want as raycast origin for collision.
    //
    // focus_x/y/z are in shifted world units (matching WorldPos).
    // AENG_world_text takes unshifted units (>> 8).
    // Color params are R, B, G (NOT RGB): pass red=0xff, blue=0x00,
    // green=0x00 for pure red.
    AENG_world_text(
        fc->focus_x >> 8,
        (fc->focus_y + fc->lookabove) >> 8,
        fc->focus_z >> 8,
        0xff,           // red
        0x00,           // blue
        0x00,           // green
        UC_TRUE,        // shadow for contrast
        (CBYTE*)"FOCUS");
}
