//
// Drawing sprites...
//

#ifndef _SPRITE_
#define _SPRITE_

#define SPRITE_SORT_NORMAL 1
#define SPRITE_SORT_FRONT 2

void SPRITE_draw(
    float world_x,
    float world_y,
    float world_z,
    float world_size,
    ULONG colour,
    ULONG specular,
    SLONG page,
    SLONG sort);

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


#endif
