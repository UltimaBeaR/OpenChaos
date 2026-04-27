#ifndef ENGINE_GRAPHICS_LIGHTING_LIGHT_H
#define ENGINE_GRAPHICS_LIGHTING_LIGHT_H

#include "engine/core/types.h"
#include "engine/core/vector.h" // GameCoord
#include "buildings/prim_types.h" // RMAX_PRIM_POINTS

// uc_orig: LIGHT_Save (fallen/Headers/light.h)
// On-disk format for a single light, used during level save/load.
typedef struct
{
    SLONG x;
    SLONG y;
    SLONG z;
    UBYTE red;
    UBYTE green;
    UBYTE blue;
    UBYTE range;
    UBYTE type;
    UBYTE param;
    UWORD dummy;

} LIGHT_Save;

// uc_orig: LIGHT_Colour (fallen/Headers/light.h)
// RGB colour value for a light, each channel 0..LIGHT_MAX_BRIGHT.
typedef struct
{
    UBYTE red;
    UBYTE green;
    UBYTE blue;

} LIGHT_Colour;

// uc_orig: LIGHT_MAX_BRIGHT (fallen/Headers/light.h)
// UBYTE light values go from 0 to LIGHT_MAX_BRIGHT; values above are clamped to maximum.
#define LIGHT_MAX_BRIGHT 64

// uc_orig: LIGHT_MAX_RANGE (fallen/Headers/light.h)
// Maximum range of a light in world coordinates.
#define LIGHT_MAX_RANGE 0x600

// uc_orig: LIGHT_Map (fallen/Headers/light.h)
// Callback table used to apply lighting to a height-field.
// The LIGHT_Map passed to LIGHT_set_hf is copied internally so it may live on the stack.
typedef struct
{
    SLONG width;
    SLONG height;

    SLONG (*get_height)(SLONG x, SLONG z); // 0 <= x < width, 0 <= z < height
    LIGHT_Colour (*get_light)(SLONG x, SLONG z);
    void (*set_light)(SLONG x, SLONG z, LIGHT_Colour light);

} LIGHT_Map;

// uc_orig: LIGHT_set_hf (fallen/Headers/light.h)
void LIGHT_set_hf(LIGHT_Map* map);

// uc_orig: LIGHT_set_ambient (fallen/Headers/light.h)
// Sets ambient colour and directional normal (length 255).
void LIGHT_set_ambient(
    LIGHT_Colour amb_colour,
    SLONG amb_norm_x,
    SLONG amb_norm_y,
    SLONG amb_norm_z);

// uc_orig: LIGHT_init (fallen/Headers/light.h)
// Removes all lights and clears all cached light data.
void LIGHT_init(void);

// uc_orig: LIGHT_recalc_hf (fallen/Headers/light.h)
// Recalculates all lighting on the current height-field and fills LIGHT_building_point[].
void LIGHT_recalc_hf(void);

// uc_orig: LIGHT_Index (fallen/Headers/light.h)
typedef UWORD LIGHT_Index;

// uc_orig: LIGHT_TYPE_NORMAL (fallen/Headers/light.h)
#define LIGHT_TYPE_NORMAL 1
// uc_orig: LIGHT_TYPE_BROKEN (fallen/Headers/light.h)
// param = flicker-off frequency.
#define LIGHT_TYPE_BROKEN 2
// uc_orig: LIGHT_TYPE_PULSE (fallen/Headers/light.h)
// param = flash speed.
#define LIGHT_TYPE_PULSE 3
// uc_orig: LIGHT_TYPE_FADE (fallen/Headers/light.h)
// param = lifetime in ticks.
#define LIGHT_TYPE_FADE 4

// uc_orig: LIGHT_create (fallen/Headers/light.h)
LIGHT_Index LIGHT_create(
    GameCoord where,
    LIGHT_Colour colour,
    UBYTE range,
    UBYTE type,
    UBYTE param);

// uc_orig: LIGHT_destroy (fallen/Headers/light.h)
void LIGHT_destroy(LIGHT_Index l_index);

// uc_orig: LIGHT_pos_set (fallen/Headers/light.h)
void LIGHT_pos_set(LIGHT_Index l_index, GameCoord pos);

// uc_orig: LIGHT_process (fallen/Headers/light.h)
void LIGHT_process(void);

// uc_orig: LIGHT_prim (fallen/Headers/light.h)
// Lights the given prim, writing results into LIGHT_point_colour[].
// Requires calc_prim_normals() to have been called first.
void LIGHT_prim(THING_INDEX t_index);
// uc_orig: LIGHT_prim_use_normals (fallen/Headers/light.h)
void LIGHT_prim_use_normals(THING_INDEX t_index);

// uc_orig: LIGHT_building_use_normals (fallen/Headers/light.h)
void LIGHT_building_use_normals(THING_INDEX t_index);

// uc_orig: LIGHT_MAX_POINTS (fallen/Headers/light.h)
#define LIGHT_MAX_POINTS 2560

// uc_orig: LIGHT_point_colour (fallen/Headers/light.h)
extern LIGHT_Colour LIGHT_point_colour[LIGHT_MAX_POINTS];
#endif // ENGINE_GRAPHICS_LIGHTING_LIGHT_H
