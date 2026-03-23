#ifndef ENGINE_GRAPHICS_PIPELINE_AENG_H
#define ENGINE_GRAPHICS_PIPELINE_AENG_H

#include "core/types.h"
#include "engine/graphics/resources/font.h"
#include "engine/graphics/resources/console.h"

// uc_orig: KERB_HEIGHT (fallen/DDEngine/Headers/aeng.h)
// Height of a kerb in world units (float).
#define KERB_HEIGHT 32.0F

// uc_orig: KERB_HEIGHTI (fallen/DDEngine/Headers/aeng.h)
// Integer version of KERB_HEIGHT.
#define KERB_HEIGHTI ((SLONG)(KERB_HEIGHT))

// uc_orig: SVector_F (fallen/DDEngine/Headers/aeng.h)
// Float 3D vector used for world-space geometry (crinkle projection, SMAP shadow).
typedef struct
{
    float X;
    float Y;
    float Z;

} SVector_F;

// uc_orig: AENG_dx_prim_points (fallen/DDEngine/Headers/aeng.h)
extern SVector_F AENG_dx_prim_points[];

// uc_orig: AENG_init (fallen/DDEngine/Headers/aeng.h)
void AENG_init(void);

// uc_orig: TEXTURE_choose_set (fallen/DDEngine/Headers/aeng.h)
void TEXTURE_choose_set(SLONG number);

// uc_orig: TEXTURE_load_needed (fallen/DDEngine/Headers/aeng.h)
void TEXTURE_load_needed(CBYTE* fname_level,
    int iStartCompletionBar = 0,
    int iEndCompletionBar = 0,
    int iNumberTexturesProbablyLoaded = 0);

// uc_orig: TEXTURE_load_needed_object (fallen/DDEngine/Headers/aeng.h)
void TEXTURE_load_needed_object(SLONG prim_object);

// uc_orig: AENG_create_dx_prim_points (fallen/DDEngine/Headers/aeng.h)
void AENG_create_dx_prim_points(void);

// uc_orig: TEXTURE_fix_texture_styles (fallen/DDEngine/Headers/aeng.h)
void TEXTURE_fix_texture_styles(void);

// uc_orig: TEXTURE_fix_prim_textures (fallen/DDEngine/Headers/aeng.h)
void TEXTURE_fix_prim_textures(void);

// uc_orig: TEXTURE_get_fiddled_position (fallen/DDEngine/Headers/aeng.h)
SLONG TEXTURE_get_fiddled_position(
    SLONG square_u,
    SLONG square_v,
    SLONG page,
    float* u,
    float* v);

// uc_orig: AENG_set_camera (fallen/DDEngine/Headers/aeng.h)
void AENG_set_camera(
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG yaw,
    SLONG pitch,
    SLONG roll);

// uc_orig: AENG_set_camera_radians (fallen/DDEngine/Headers/aeng.h)
void AENG_set_camera_radians(
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    float yaw,
    float pitch,
    float roll);

// uc_orig: AENG_set_camera_radians (fallen/DDEngine/Headers/aeng.h)
// Overload with explicit splitscreen mode (POLY_SPLITSCREEN_*).
void AENG_set_camera_radians(
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    float yaw,
    float pitch,
    float roll,
    SLONG splitscreen);

// uc_orig: AENG_set_draw_distance (fallen/DDEngine/Headers/aeng.h)
void AENG_set_draw_distance(SLONG dist);

// uc_orig: AENG_get_draw_distance (fallen/DDEngine/Headers/aeng.h)
SLONG AENG_get_draw_distance();

// uc_orig: AENG_world_line (fallen/DDEngine/Headers/aeng.h)
void AENG_world_line(
    SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    SLONG sort_to_front);

// uc_orig: AENG_world_line_nondebug (fallen/DDEngine/Headers/aeng.h)
void AENG_world_line_nondebug(
    SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    SLONG sort_to_front);

// uc_orig: AENG_world_line_infinite (fallen/DDEngine/Headers/aeng.h)
void AENG_world_line_infinite(
    SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    SLONG sort_to_front);

// uc_orig: AENG_draw_col_tri (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_col_tri(SLONG x0, SLONG y0, SLONG col0, SLONG x1, SLONG y1, SLONG col1, SLONG x2, SLONG z2, SLONG col2, SLONG layer);

// uc_orig: AENG_e_draw_3d_line (fallen/DDEngine/Headers/aeng.h)
void AENG_e_draw_3d_line(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);

// uc_orig: AENG_e_draw_3d_line_dir (fallen/DDEngine/Headers/aeng.h)
void AENG_e_draw_3d_line_dir(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);

// uc_orig: AENG_e_draw_3d_line_col (fallen/DDEngine/Headers/aeng.h)
void AENG_e_draw_3d_line_col(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG r, SLONG g, SLONG b);

// uc_orig: AENG_e_draw_3d_line_col_sorted (fallen/DDEngine/Headers/aeng.h)
void AENG_e_draw_3d_line_col_sorted(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG r, SLONG g, SLONG b);

// uc_orig: AENG_e_draw_3d_mapwho (fallen/DDEngine/Headers/aeng.h)
void AENG_e_draw_3d_mapwho(SLONG x1, SLONG z1);

// uc_orig: AENG_e_draw_3d_mapwho_y (fallen/DDEngine/Headers/aeng.h)
void AENG_e_draw_3d_mapwho_y(SLONG x1, SLONG y1, SLONG z1);

// uc_orig: AENG_draw_rect (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_rect(SLONG x, SLONG y, SLONG w, SLONG h, SLONG col, SLONG layer, SLONG page);

// uc_orig: e_draw_3d_line (fallen/DDEngine/Headers/aeng.h)
#define e_draw_3d_line AENG_e_draw_3d_line
// uc_orig: e_draw_3d_line_dir (fallen/DDEngine/Headers/aeng.h)
#define e_draw_3d_line_dir AENG_e_draw_3d_line_dir
// uc_orig: e_draw_3d_line_col (fallen/DDEngine/Headers/aeng.h)
#define e_draw_3d_line_col AENG_e_draw_3d_line_col
// uc_orig: e_draw_3d_line_col_sorted (fallen/DDEngine/Headers/aeng.h)
#define e_draw_3d_line_col_sorted AENG_e_draw_3d_line_col_sorted
// uc_orig: e_draw_3d_mapwho (fallen/DDEngine/Headers/aeng.h)
#define e_draw_3d_mapwho AENG_e_draw_3d_mapwho
// uc_orig: e_draw_3d_mapwho_y (fallen/DDEngine/Headers/aeng.h)
#define e_draw_3d_mapwho_y AENG_e_draw_3d_mapwho_y

// uc_orig: AENG_set_sky_nighttime (fallen/DDEngine/Headers/aeng.h)
void AENG_set_sky_nighttime(void);

// uc_orig: AENG_set_sky_daytime (fallen/DDEngine/Headers/aeng.h)
void AENG_set_sky_daytime(ULONG bottom_colour, ULONG top_colour);

// uc_orig: AENG_draw_city (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_city(void);

// uc_orig: AENG_draw_inside (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_inside(void);

// uc_orig: AENG_draw_sewer (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_sewer(void);

// uc_orig: AENG_draw_ns (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_ns(void);

// uc_orig: AENG_draw (fallen/DDEngine/Headers/aeng.h)
// Draws the scene. If draw_3d != 0, renders in red/blue stereo mode.
void AENG_draw(SLONG draw_3d);

// uc_orig: AENG_clear_viewport (fallen/DDEngine/Headers/aeng.h)
void AENG_clear_viewport();

// uc_orig: AENG_draw_scanner (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_scanner(
    SLONG screen_x1,
    SLONG screen_y1,
    SLONG screen_x2,
    SLONG screen_y2,
    SLONG map_x,
    SLONG map_z,
    SLONG map_zoom,
    SLONG map_angle);

// uc_orig: AENG_draw_power (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_power(SLONG x, SLONG y, SLONG w, SLONG h, SLONG val, SLONG max);

// uc_orig: AENG_draw_FPS (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_FPS(void);

// uc_orig: AENG_draw_messages (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_messages(void);

// uc_orig: AENG_fade_in (fallen/DDEngine/Headers/aeng.h)
void AENG_fade_in(UBYTE amount);

// uc_orig: AENG_fade_out (fallen/DDEngine/Headers/aeng.h)
void AENG_fade_out(UBYTE amount);

// uc_orig: AENG_flip (fallen/DDEngine/Headers/aeng.h)
void AENG_flip(void);

// uc_orig: AENG_blit (fallen/DDEngine/Headers/aeng.h)
void AENG_blit(void);

// uc_orig: MSG_add (fallen/DDEngine/Headers/aeng.h)
void MSG_add(CBYTE* message, ...);

// uc_orig: AENG_clear_screen (fallen/DDEngine/Headers/aeng.h)
void AENG_clear_screen(void);

// uc_orig: AENG_lock (fallen/DDEngine/Headers/aeng.h)
SLONG AENG_lock(void);

// uc_orig: FONT_draw (fallen/DDEngine/Headers/aeng.h)
SLONG FONT_draw(SLONG x, SLONG y, CBYTE* text, ...);

// uc_orig: AENG_unlock (fallen/DDEngine/Headers/aeng.h)
void AENG_unlock(void);

// uc_orig: AENG_fini (fallen/DDEngine/Headers/aeng.h)
void AENG_fini(void);

// uc_orig: AENG_raytraced_position (fallen/DDEngine/Headers/aeng.h)
SLONG AENG_raytraced_position(
    SLONG sx,
    SLONG sy,
    SLONG* world_x,
    SLONG* world_y,
    SLONG* world_z,
    SLONG indoors = 0);

// uc_orig: AENG_raytraced_y_position (fallen/DDEngine/Headers/aeng.h)
void AENG_raytraced_y_position(
    SLONG sy,
    SLONG wx,
    SLONG wz,
    SLONG* world_y);

// uc_orig: AENG_MOUSE_OVER_LIGHT_BOT (fallen/DDEngine/Headers/aeng.h)
#define AENG_MOUSE_OVER_LIGHT_BOT (1 << 0)
// uc_orig: AENG_MOUSE_OVER_LIGHT_TOP (fallen/DDEngine/Headers/aeng.h)
#define AENG_MOUSE_OVER_LIGHT_TOP (1 << 1)

// uc_orig: AENG_light_draw (fallen/DDEngine/Headers/aeng.h)
ULONG AENG_light_draw(
    SLONG sx,
    SLONG sy,
    SLONG lx,
    SLONG ly,
    SLONG lz,
    ULONG colour,
    UBYTE highlight);

// uc_orig: AENG_MOUSE_OVER_WAYPOINT (fallen/DDEngine/Headers/aeng.h)
#define AENG_MOUSE_OVER_WAYPOINT (1 << 0)

// uc_orig: AENG_waypoint_draw (fallen/DDEngine/Headers/aeng.h)
ULONG AENG_waypoint_draw(
    SLONG mx,
    SLONG my,
    SLONG lx,
    SLONG ly,
    SLONG lz,
    ULONG colour,
    UBYTE highlight);

// uc_orig: AENG_rad_trigger_draw (fallen/DDEngine/Headers/aeng.h)
ULONG AENG_rad_trigger_draw(
    SLONG mx,
    SLONG my,
    SLONG lx,
    SLONG ly,
    SLONG lz,
    ULONG rad,
    ULONG colour,
    UBYTE highlight);

// uc_orig: AENG_groundsquare_draw (fallen/DDEngine/Headers/aeng.h)
void AENG_groundsquare_draw(
    SLONG lx,
    SLONG ly,
    SLONG lz,
    ULONG colour,
    UBYTE polyinit);

// uc_orig: AENG_read_detail_levels (fallen/DDEngine/Headers/aeng.h)
void AENG_read_detail_levels();

// uc_orig: AENG_get_detail_levels (fallen/DDEngine/Headers/aeng.h)
void AENG_get_detail_levels(int* stars,
    int* shadows,
    int* moon_reflection,
    int* people_reflection,
    int* puddles,
    int* dirt,
    int* mist,
    int* rain,
    int* skyline,
    int* filter,
    int* perspective,
    int* crinkles);

// uc_orig: AENG_set_detail_levels (fallen/DDEngine/Headers/aeng.h)
void AENG_set_detail_levels(int stars,
    int shadows,
    int moon_reflection,
    int people_reflection,
    int puddles,
    int dirt,
    int mist,
    int rain,
    int skyline,
    int filter,
    int perspective,
    int crinkles);

// uc_orig: AENG_draw_sewer_editor (fallen/DDEngine/Headers/aeng.h)
void AENG_draw_sewer_editor(
    SLONG cam_x,
    SLONG cam_y,
    SLONG cam_z,
    SLONG cam_yaw,
    SLONG cam_pitch,
    SLONG cam_roll,
    SLONG mouse_x,
    SLONG mouse_y,
    SLONG* mouse_over_valid,
    SLONG* mouse_over_x,
    SLONG* mouse_over_y,
    SLONG* mouse_over_z,
    SLONG draw_prim_at_mouse,
    SLONG prim_object,
    SLONG prim_yaw);

// uc_orig: AENG_world_text (fallen/DDEngine/Headers/aeng.h)
void AENG_world_text(
    SLONG x,
    SLONG y,
    SLONG z,
    UBYTE red,
    UBYTE blue,
    UBYTE green,
    UBYTE shadowed_or_not,
    CBYTE* fmt, ...);

// panel.h forward declarations (panel.cpp not yet migrated — depends on game.h)
extern void PANEL_draw_health_bar(SLONG x, SLONG y, SLONG percentage);
extern void PANEL_draw_timer(SLONG time_in_hundreths, SLONG x, SLONG y);
extern void PANEL_draw_buffered(void);
extern void PANEL_finish(void);

#endif // ENGINE_GRAPHICS_PIPELINE_AENG_H
