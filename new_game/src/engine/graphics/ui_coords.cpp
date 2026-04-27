#include "engine/graphics/ui_coords.h"

#include "engine/platform/uc_common.h" // DisplayWidth, DisplayHeight

namespace ui_coords {

// Initialised to 0 so callers can detect "not yet initialised" and skip
// drawing rather than render at a stale 640x480 default (which would
// briefly appear as a small image in the top-left corner of a wide
// screen before the mode-change callback runs).
float g_frame_scale = 0.0f;
float g_frame_w_px = 0.0f;
float g_frame_h_px = 0.0f;
float g_screen_w_px = 0.0f;
float g_screen_h_px = 0.0f;

void recompute(int real_w, int real_h)
{
    g_screen_w_px = float(real_w);
    g_screen_h_px = float(real_h);

    const float by_h = float(real_h) / float(DisplayHeight);
    const float by_w = float(real_w) / float(DisplayWidth);
    g_frame_scale = (by_h < by_w) ? by_h : by_w;

    g_frame_w_px = float(DisplayWidth) * g_frame_scale;
    g_frame_h_px = float(DisplayHeight) * g_frame_scale;
}

Vec2f frame_origin_screen(UIAnchor anchor)
{
    const float fw01 = g_frame_w_px / g_screen_w_px;
    const float fh01 = g_frame_h_px / g_screen_h_px;

    float ox01;
    switch (anchor) {
    case UIAnchor::LEFT_TOP:
    case UIAnchor::LEFT_CENTER:
    case UIAnchor::LEFT_BOTTOM:
        ox01 = 0.0f;
        break;
    case UIAnchor::CENTER_TOP:
    case UIAnchor::CENTER_CENTER:
    case UIAnchor::CENTER_BOTTOM:
        ox01 = (1.0f - fw01) * 0.5f;
        break;
    case UIAnchor::RIGHT_TOP:
    case UIAnchor::RIGHT_CENTER:
    case UIAnchor::RIGHT_BOTTOM:
    default:
        ox01 = 1.0f - fw01;
        break;
    }

    float oy01;
    switch (anchor) {
    case UIAnchor::LEFT_TOP:
    case UIAnchor::CENTER_TOP:
    case UIAnchor::RIGHT_TOP:
        oy01 = 0.0f;
        break;
    case UIAnchor::LEFT_CENTER:
    case UIAnchor::CENTER_CENTER:
    case UIAnchor::RIGHT_CENTER:
        oy01 = (1.0f - fh01) * 0.5f;
        break;
    case UIAnchor::LEFT_BOTTOM:
    case UIAnchor::CENTER_BOTTOM:
    case UIAnchor::RIGHT_BOTTOM:
    default:
        oy01 = 1.0f - fh01;
        break;
    }

    return Vec2f { ox01, oy01 };
}

} // namespace ui_coords
