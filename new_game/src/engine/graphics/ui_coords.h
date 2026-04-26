#ifndef ENGINE_GRAPHICS_UI_COORDS_H
#define ENGINE_GRAPHICS_UI_COORDS_H

// UI coordinate system for the new HUD/menu/frontend layout.
//
// "The screen" here = the scene FBO. The game treats the FBO as its entire
// screen; the real backbuffer is only known to the composition layer.
// When the physical window aspect is outside [FOV_MIN_ASPECT,
// FOV_CAP_ASPECT], the FBO is narrower/shorter than the window; the
// composition layer paints outer pillar/letterbox bars over the real
// backbuffer around the FBO. This module is blind to that — it only knows
// about pixels inside the FBO.
//
// Three coordinate spaces are in play:
//
//   - "framed01"  : [0..1] inside a virtual 4:3 frame fitted inside the FBO
//                   (g_frame_w_px x g_frame_h_px in FBO pixels).
//   - "screen01"  : [0..1] across the full FBO.
//   - "pixels"    : FBO pixels.
//
// Anchored UI elements use a UIAnchor to say where the framed region is
// pinned within the FBO. Framed content (menus, backdrops) uses
// CENTER_CENTER; HUD elements pick per-element anchors (radar = RIGHT_TOP,
// ammo = LEFT_BOTTOM, etc.). The inner bars around the framed 4:3 region
// on non-4:3 FBOs are painted by the game itself as ordinary UI draws —
// this module only converts coordinates.
//
// Migration helper old_px_to_screen_pixels() lets call sites that today
// pass virtual 640x480 pixels to a draw call switch over with a one-line
// change; native [0..1] adoption is per-element follow-up work.
//
// See new_game_devlog/fullscreen_transition/ui_coords_plan.md.

namespace ui_coords {

struct Vec2f {
    float x;
    float y;
};

enum class UIAnchor {
    LEFT_TOP,    CENTER_TOP,    RIGHT_TOP,
    LEFT_CENTER, CENTER_CENTER, RIGHT_CENTER,
    LEFT_BOTTOM, CENTER_BOTTOM, RIGHT_BOTTOM,
};

// Cached frame metrics. Recomputed by ui_coords_recompute().
//   g_frame_scale = min(real_h / 480, real_w / 640)
//   g_frame_w_px  = 640 * g_frame_scale
//   g_frame_h_px  = 480 * g_frame_scale
extern float g_frame_scale;
extern float g_frame_w_px;
extern float g_frame_h_px;
extern float g_screen_w_px;
extern float g_screen_h_px;

// Recompute cached metrics. Call from the graphics-engine mode-change
// callback (game_mode_changed) with the scene FBO dimensions.
void recompute(int real_w, int real_h);

// Top-left corner of the framed 4:3 region inside the FBO, in
// [0..1] screen coordinates, given the anchor.
Vec2f frame_origin_screen(UIAnchor anchor);

} // namespace ui_coords

#endif // ENGINE_GRAPHICS_UI_COORDS_H
