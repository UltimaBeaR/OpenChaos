#ifndef ENGINE_GRAPHICS_PIPELINE_QENG_H
#define ENGINE_GRAPHICS_PIPELINE_QENG_H

#include "world/map/qmap.h"
#include "engine/console/message.h"

// Minimal 3D engine for rendering QMAP world data using the POLY pipeline.
// Used by the level editor (qedit.cpp). Not used in the shipping game.

// Camera position set last by QENG_set_camera.
// uc_orig: QENG_mouse_over (fallen/DDEngine/Headers/qeng.h)
extern SLONG QENG_mouse_over;
// uc_orig: QENG_mouse_pos_x (fallen/DDEngine/Headers/qeng.h)
extern float QENG_mouse_pos_x;
// uc_orig: QENG_mouse_pos_y (fallen/DDEngine/Headers/qeng.h)
extern float QENG_mouse_pos_y;
// uc_orig: QENG_mouse_pos_z (fallen/DDEngine/Headers/qeng.h)
extern float QENG_mouse_pos_z;

// uc_orig: QENG_set_camera (fallen/DDEngine/Headers/qeng.h)
void QENG_set_camera(float world_x, float world_y, float world_z,
    float yaw, float pitch, float roll);

// uc_orig: QENG_clear_screen (fallen/DDEngine/Headers/qeng.h)
void QENG_clear_screen(void);

// Draws a world-space line segment. Optionally sorts to front of draw order.
// uc_orig: QENG_world_line (fallen/DDEngine/Headers/qeng.h)
void QENG_world_line(SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    bool sort_to_front);

// uc_orig: QENG_draw (fallen/DDEngine/Headers/qeng.h)
void QENG_draw(QMAP_Draw* qd);

// uc_orig: QENG_render (fallen/DDEngine/Headers/qeng.h)
void QENG_render(void);

// uc_orig: QENG_flip (fallen/DDEngine/Headers/qeng.h)
void QENG_flip(void);

#endif // ENGINE_GRAPHICS_PIPELINE_QENG_H
