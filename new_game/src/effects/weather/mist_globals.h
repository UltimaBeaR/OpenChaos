#ifndef EFFECTS_WEATHER_MIST_GLOBALS_H
#define EFFECTS_WEATHER_MIST_GLOBALS_H

#include "effects/weather/mist.h"

// Shared point pool used by all mist layers.
// uc_orig: MIST_point (fallen/Source/mist.cpp)
extern MIST_Point MIST_point[MIST_MAX_POINTS];
// uc_orig: MIST_point_upto (fallen/Source/mist.cpp)
extern SLONG MIST_point_upto;

// Active mist layer array.
// uc_orig: MIST_mist (fallen/Source/mist.cpp)
extern MIST_Mist MIST_mist[MIST_MAX_MIST];
// uc_orig: MIST_mist_upto (fallen/Source/mist.cpp)
extern SLONG MIST_mist_upto;

// Renderer iterator state for MIST_get_start / MIST_get_detail / MIST_get_point.
// uc_orig: MIST_get_upto (fallen/Source/mist.cpp)
extern SLONG MIST_get_upto;
// uc_orig: MIST_get_dx (fallen/Source/mist.cpp)
extern SLONG MIST_get_dx;
// uc_orig: MIST_get_dz (fallen/Source/mist.cpp)
extern SLONG MIST_get_dz;
// uc_orig: MIST_get_du (fallen/Source/mist.cpp)
extern float MIST_get_du;
// uc_orig: MIST_get_dv (fallen/Source/mist.cpp)
extern float MIST_get_dv;
// uc_orig: MIST_get_turn (fallen/Source/mist.cpp)
extern SLONG MIST_get_turn;
// uc_orig: MIST_get_base_u (fallen/Source/mist.cpp)
extern float MIST_get_base_u;
// uc_orig: MIST_get_base_v (fallen/Source/mist.cpp)
extern float MIST_get_base_v;
// uc_orig: MIST_off_u_odd (fallen/Source/mist.cpp)
extern float MIST_off_u_odd;
// uc_orig: MIST_off_v_odd (fallen/Source/mist.cpp)
extern float MIST_off_v_odd;
// uc_orig: MIST_off_u_even (fallen/Source/mist.cpp)
extern float MIST_off_u_even;
// uc_orig: MIST_off_v_even (fallen/Source/mist.cpp)
extern float MIST_off_v_even;
// uc_orig: MIST_get_mist (fallen/Source/mist.cpp)
extern MIST_Mist* MIST_get_mist;

#endif // EFFECTS_WEATHER_MIST_GLOBALS_H
