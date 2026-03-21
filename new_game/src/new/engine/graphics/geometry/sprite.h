#ifndef ENGINE_GRAPHICS_GEOMETRY_SPRITE_H
#define ENGINE_GRAPHICS_GEOMETRY_SPRITE_H

#include "core/types.h"

// uc_orig: SPRITE_SORT_NORMAL (fallen/DDEngine/Headers/sprite.h)
#define SPRITE_SORT_NORMAL 1

// uc_orig: SPRITE_SORT_FRONT (fallen/DDEngine/Headers/sprite.h)
#define SPRITE_SORT_FRONT 2

// Draws a world-space billboard as a square sprite using the given texture page.
// sort: SPRITE_SORT_NORMAL uses world depth, SPRITE_SORT_FRONT renders in front of everything.

// uc_orig: SPRITE_draw (fallen/DDEngine/Source/sprite.cpp)
void SPRITE_draw(
    float world_x,
    float world_y,
    float world_z,
    float world_size,
    ULONG colour,
    ULONG specular,
    SLONG page,
    SLONG sort);

// Like SPRITE_draw but with explicit UV coordinates within the texture page.
// uc_orig: SPRITE_draw_tex (fallen/DDEngine/Source/sprite.cpp)
void SPRITE_draw_tex(
    float world_x,
    float world_y,
    float world_z,
    float world_size,
    ULONG colour,
    ULONG specular,
    SLONG page,
    float u, float v, float w, float h,
    SLONG sort);

// Like SPRITE_draw_tex but each corner is offset by (wx, wy) for distortion effects.
// uc_orig: SPRITE_draw_tex_distorted (fallen/DDEngine/Source/sprite.cpp)
void SPRITE_draw_tex_distorted(
    float world_x,
    float world_y,
    float world_z,
    float world_size,
    ULONG colour,
    ULONG specular,
    SLONG page,
    float u, float v, float w, float h,
    float wx1, float wy1, float wx2, float wy2, float wx3, float wy3, float wx4, float wy4,
    SLONG sort);

#endif // ENGINE_GRAPHICS_GEOMETRY_SPRITE_H
