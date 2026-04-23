#include "engine/graphics/ui_coords.h"

#include "engine/platform/uc_common.h" // DisplayWidth, DisplayHeight

namespace ui_coords {

// Initialised to 0 so callers can detect "not yet initialised" and skip
// drawing rather than render at a stale 640x480 default (which would
// briefly appear as a small image in the top-left corner of a wide
// screen before the mode-change callback runs).
float g_frame_scale = 0.0f;
float g_frame_w_px  = 0.0f;
float g_frame_h_px  = 0.0f;
float g_real_w_px   = 0.0f;
float g_real_h_px   = 0.0f;

void recompute(int real_w, int real_h)
{
    g_real_w_px = float(real_w);
    g_real_h_px = float(real_h);

    const float by_h = float(real_h) / float(DisplayHeight);
    const float by_w = float(real_w) / float(DisplayWidth);
    g_frame_scale = (by_h < by_w) ? by_h : by_w;

    g_frame_w_px = float(DisplayWidth)  * g_frame_scale;
    g_frame_h_px = float(DisplayHeight) * g_frame_scale;
}

Vec2f frame_origin_screen(UIAnchor anchor)
{
    const float fw01 = g_frame_w_px / g_real_w_px;
    const float fh01 = g_frame_h_px / g_real_h_px;

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

    return Vec2f{ ox01, oy01 };
}

Vec2f frame_to_screen(Vec2f framed01, UIAnchor anchor)
{
    const Vec2f origin = frame_origin_screen(anchor);
    const float fw01 = g_frame_w_px / g_real_w_px;
    const float fh01 = g_frame_h_px / g_real_h_px;
    return Vec2f{
        origin.x + framed01.x * fw01,
        origin.y + framed01.y * fh01,
    };
}

Vec2f screen_to_frame(Vec2f screen01, UIAnchor anchor)
{
    const Vec2f origin = frame_origin_screen(anchor);
    const float fw01 = g_frame_w_px / g_real_w_px;
    const float fh01 = g_frame_h_px / g_real_h_px;
    return Vec2f{
        (screen01.x - origin.x) / fw01,
        (screen01.y - origin.y) / fh01,
    };
}

Vec2f old_px_to_screen(float x_px640, float y_px480, UIAnchor anchor)
{
    return frame_to_screen(
        Vec2f{ x_px640 / float(DisplayWidth), y_px480 / float(DisplayHeight) },
        anchor);
}

Vec2f screen_to_pixels(Vec2f screen01)
{
    return Vec2f{ screen01.x * g_real_w_px, screen01.y * g_real_h_px };
}

Vec2f old_px_to_screen_pixels(float x_px640, float y_px480, UIAnchor anchor)
{
    return screen_to_pixels(old_px_to_screen(x_px640, y_px480, anchor));
}

} // namespace ui_coords
