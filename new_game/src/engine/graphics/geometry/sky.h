#ifndef ENGINE_GRAPHICS_GEOMETRY_SKY_H
#define ENGINE_GRAPHICS_GEOMETRY_SKY_H

#include "engine/core/types.h"

// Initialise sky system: generates clouds and loads star positions.
// If star_file is NULL, stars are randomly generated.
// If star_file is a path to an ASCII file, stars are loaded from it.
// File format:
//   # comment
//   Star: <yaw>, <pitch>, <brightness>
//   (yaw/pitch in 2048-units, brightness 0-255)
//
// uc_orig: SKY_init (fallen/DDEngine/Source/sky.cpp)
void SKY_init(CBYTE* star_file);

// Draw stars directly to the locked screen (must call before poly rendering).
// uc_orig: SKY_draw_stars (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_stars(
    float world_camera_x,
    float world_camera_y,
    float world_camera_z,
    float max_dist);

// Draw animated cloud quads via the poly pipeline.
// uc_orig: SKY_draw_poly_clouds (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_poly_clouds(
    float world_camera_x,
    float world_camera_y,
    float world_camera_z,
    float max_dist);

// Draw the moon quad via the poly pipeline.
// uc_orig: SKY_draw_poly_moon (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_poly_moon(
    float world_camera_x,
    float world_camera_y,
    float world_camera_z,
    float max_dist);

// Draw the moon reflection (wavy) via the poly pipeline.
// Returns UC_FALSE if the moon is off-screen.
// Also outputs the bounding box of the drawn reflection.
// uc_orig: SKY_draw_moon_reflection (fallen/DDEngine/Source/sky.cpp)
SLONG SKY_draw_moon_reflection(
    float world_camera_x,
    float world_camera_y,
    float world_camera_z,
    float max_dist,
    float* moon_screen_x1,
    float* moon_screen_y1,
    float* moon_screen_x2,
    float* moon_screen_y2);

// Draw the daytime sky gradient quads via the poly pipeline.
// uc_orig: SKY_draw_poly_sky (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_poly_sky(
    float world_camera_x,
    float world_camera_y,
    float world_camera_z,
    float world_camera_yaw,
    float max_dist,
    ULONG bot_colour,
    ULONG top_colour);

// Older cylindrical sky implementation using a circle of vertical quads.
// Still called from aeng.cpp when AENG_detail_skyline is true.
// uc_orig: SKY_draw_poly_sky_old (fallen/DDEngine/Source/sky.cpp)
void SKY_draw_poly_sky_old(
    float world_camera_x,
    float world_camera_y,
    float world_camera_z,
    float world_camera_yaw,
    float max_dist,
    ULONG bot_colour,
    ULONG top_colour);

#endif // ENGINE_GRAPHICS_GEOMETRY_SKY_H
