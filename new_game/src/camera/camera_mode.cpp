#include "camera/camera_mode.h"
#include "engine/input/gamepad_globals.h" // active_input_device, INPUT_DEVICE_*

CameraMode get_active_camera_mode()
{
    // Map the live input device to the compile-time default mode.
    // Future: read a settings override and let the player force a mode
    // regardless of device. For now strictly device-driven.
    if (active_input_device == INPUT_DEVICE_KEYBOARD_MOUSE)
        return KBM_DEFAULT_CAMERA_MODE;
    return GAMEPAD_DEFAULT_CAMERA_MODE;
}

float get_camera_rubberness()
{
    return CAMERA_RUBBERNESS_DEFAULT;
}

float keep_for_rubberness(float current_default_keep)
{
    // Clamp inputs.
    float r = get_camera_rubberness();
    if (r < 0.0f)
        r = 0.0f;
    if (r > 1.0f)
        r = 1.0f;
    if (current_default_keep < 0.0f)
        current_default_keep = 0.0f;
    if (current_default_keep > 1.0f)
        current_default_keep = 1.0f;

    if (r <= 0.5f) {
        // [0..0.5]: linear from 0 (snap) up to the current shipping value.
        const float t = r * 2.0f;
        return current_default_keep * t;
    }
    // [0.5..1]: linear from current shipping value up to "max lazy".
    // max_lazy compresses the remaining "slack to 1.0" (i.e. 1 - current)
    // by a factor of 4. So at current=0.75 (rotation default), max_lazy =
    // 1 - 0.25/4 = 0.9375 → fc rotates only ~6%/tick instead of 25%. The
    // factor is arbitrary but feels distinctly lazier without being
    // unplayable.
    constexpr float MAX_LAZY_SLACK_COMPRESS = 0.25f;
    const float max_lazy = 1.0f - (1.0f - current_default_keep) * MAX_LAZY_SLACK_COMPRESS;
    const float t = (r - 0.5f) * 2.0f;
    return current_default_keep + (max_lazy - current_default_keep) * t;
}

float orbit_lag_for_rubberness(float default_lag_at_half)
{
    float r = get_camera_rubberness();
    if (r < 0.0f)
        r = 0.0f;
    if (r > 1.0f)
        r = 1.0f;
    if (default_lag_at_half < 0.0f)
        default_lag_at_half = 0.0f;

    if (r <= 0.5f) {
        // [0..0.5]: linear from 0 (snap, no residual) up to shipping default.
        const float t = r * 2.0f;
        return default_lag_at_half * t;
    }
    // [0.5..1]: linear from shipping default up to 4× — distinctly laggier.
    // Note: for STICK inputs (max ~61 angle units per tick at full
    // deflection), residual values above the input magnitude saturate at
    // sync_angle = 0 inside the mdx/stick blocks — no opposite-direction
    // motion is introduced. Larger residual still matters for MOUSE
    // inputs, which can produce angle deltas well above 61 per tick.
    constexpr float MAX_LAZY_MULT = 4.0f;
    const float max_lazy = default_lag_at_half * MAX_LAZY_MULT;
    const float t = (r - 0.5f) * 2.0f;
    return default_lag_at_half + (max_lazy - default_lag_at_half) * t;
}
