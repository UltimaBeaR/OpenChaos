#ifndef ENGINE_GRAPHICS_UI_COORDS_H
#define ENGINE_GRAPHICS_UI_COORDS_H

// UI coordinate system for the new HUD/menu/frontend layout.
//
// Three coordinate spaces are in play:
//
//   - "framed01"  : [0..1] inside a virtual 4:3 frame fitted to the screen
//                   (g_frame_w_px x g_frame_h_px in real pixels).
//   - "screen01"  : [0..1] across the full real framebuffer.
//   - "pixels"    : real framebuffer pixels.
//
// Anchored UI elements use a UIAnchor to say where the framed region is
// pinned within the real screen. Framed content (menus, backdrops) uses
// CENTER_CENTER; HUD elements pick per-element anchors (radar = RIGHT_TOP,
// ammo = LEFT_BOTTOM, etc.). The black space around the framed region on
// non-4:3 monitors must be cleared / painted black separately — this
// module only converts coordinates.
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
extern float g_real_w_px;
extern float g_real_h_px;

// Recompute cached metrics. Call from the graphics-engine mode-change
// callback (game_mode_changed) with the real framebuffer dimensions.
void recompute(int real_w, int real_h);

// Top-left corner of the framed 4:3 region inside the real screen, in
// [0..1] screen coordinates, given the anchor.
Vec2f frame_origin_screen(UIAnchor anchor);

// [0..1] framed coord -> [0..1] screen coord, given the anchor of the
// 4:3 frame within the real screen.
Vec2f frame_to_screen(Vec2f framed01, UIAnchor anchor);

// Inverse: [0..1] screen coord -> [0..1] framed coord. Coords outside the
// framed region map to values outside [0..1] (caller decides how to handle
// e.g. mouse clicks on the black bars).
Vec2f screen_to_frame(Vec2f screen01, UIAnchor anchor);

// Migration helper: old 640x480 pixel coord -> [0..1] screen coord.
Vec2f old_px_to_screen(float x_px640, float y_px480, UIAnchor anchor);

// [0..1] screen -> real framebuffer pixels.
Vec2f screen_to_pixels(Vec2f screen01);

// Migration helper: old 640x480 pixel coord -> real framebuffer pixels.
// Most-common entry point during the bulk rewrite.
Vec2f old_px_to_screen_pixels(float x_px640, float y_px480, UIAnchor anchor);

} // namespace ui_coords

#endif // ENGINE_GRAPHICS_UI_COORDS_H
