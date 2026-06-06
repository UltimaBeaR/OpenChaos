#include "camera/look_sensitivity.h"
#include "engine/io/oc_config.h"

// Runtime camera-look settings, read once from config.json on first use.
// Config is loaded at startup (well before any camera tick), so a lazy read
// is safe and avoids a per-tick map lookup. There is no runtime settings UI
// for these yet; when one lands, add a reload that re-runs ensure_loaded().

namespace {

constexpr float SENS_DEFAULT = 0.4f; // matches the config defaults block

bool s_loaded = false;
float s_mouse_sens;
float s_gamepad_sens;
bool s_mouse_invert_y;
bool s_gamepad_invert_y;

void ensure_loaded()
{
    if (s_loaded)
        return;
    s_loaded = true;
    s_mouse_sens = OC_CONFIG_get_float("mouse", "camera_orbit_sensitivity", SENS_DEFAULT, 0.0f, 1.0f);
    s_gamepad_sens = OC_CONFIG_get_float("gamepad", "camera_orbit_sensitivity", SENS_DEFAULT, 0.0f, 1.0f);
    // Stored as a JSON boolean; OC_CONFIG_get_int returns 0/1 for booleans.
    s_mouse_invert_y = OC_CONFIG_get_int("mouse", "camera_orbit_invert_y", 0) != 0;
    s_gamepad_invert_y = OC_CONFIG_get_int("gamepad", "camera_orbit_invert_y", 0) != 0;
}

} // namespace

float CAM_mouse_orbit_sensitivity()
{
    ensure_loaded();
    return s_mouse_sens;
}
float CAM_gamepad_orbit_sensitivity()
{
    ensure_loaded();
    return s_gamepad_sens;
}
bool CAM_mouse_invert_y()
{
    ensure_loaded();
    return s_mouse_invert_y;
}
bool CAM_gamepad_invert_y()
{
    ensure_loaded();
    return s_gamepad_invert_y;
}
