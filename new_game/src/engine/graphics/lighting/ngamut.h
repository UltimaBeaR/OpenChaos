#ifndef ENGINE_GRAPHICS_LIGHTING_NGAMUT_H
#define ENGINE_GRAPHICS_LIGHTING_NGAMUT_H

#include "engine/platform/uc_common.h"

// uc_orig: NGAMUT_SIZE (fallen/DDEngine/Headers/NGamut.h)
#define NGAMUT_SIZE 128
// uc_orig: NGAMUT_SIZE_LO (fallen/DDEngine/Headers/NGamut.h)
#define NGAMUT_SIZE_LO 32

// uc_orig: NGAMUT_Gamut (fallen/DDEngine/Headers/NGamut.h)
typedef struct
{
    SLONG xmin;
    SLONG xmax;

} NGAMUT_Gamut;

// uc_orig: NGAMUT_init (fallen/DDEngine/Headers/NGamut.h)
void NGAMUT_init(void);

// uc_orig: NGAMUT_add_line (fallen/DDEngine/Headers/NGamut.h)
void NGAMUT_add_line(float px1, float pz1, float px2, float pz2);

// uc_orig: NGAMUT_view_square (fallen/DDEngine/Headers/NGamut.h)
void NGAMUT_view_square(float mid_x, float mid_z, float radius);

// uc_orig: NGAMUT_calculate_point_gamut (fallen/DDEngine/Headers/NGamut.h)
void NGAMUT_calculate_point_gamut(void);

// uc_orig: NGAMUT_calculate_out_gamut (fallen/DDEngine/Headers/NGamut.h)
void NGAMUT_calculate_out_gamut(void);

// uc_orig: NGAMUT_calculate_lo_gamut (fallen/DDEngine/Headers/NGamut.h)
void NGAMUT_calculate_lo_gamut(void);

#include "ngamut_globals.h"

#endif // ENGINE_GRAPHICS_LIGHTING_NGAMUT_H
