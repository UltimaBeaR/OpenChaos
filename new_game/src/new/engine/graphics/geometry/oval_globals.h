#ifndef ENGINE_GRAPHICS_GEOMETRY_OVAL_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_OVAL_GLOBALS_H

// Current oval center position in world space (set by OVAL_add, used by OVAL_get_uv).
// uc_orig: OVAL_mid_x (fallen/DDEngine/Source/oval.cpp)
extern float OVAL_mid_x;
// uc_orig: OVAL_mid_y (fallen/DDEngine/Source/oval.cpp)
extern float OVAL_mid_y;
// uc_orig: OVAL_mid_z (fallen/DDEngine/Source/oval.cpp)
extern float OVAL_mid_z;

// UV gradient vectors for the oval texture mapping (set by OVAL_add).
// uc_orig: OVAL_dudx (fallen/DDEngine/Source/oval.cpp)
extern float OVAL_dudx;
// uc_orig: OVAL_dvdx (fallen/DDEngine/Source/oval.cpp)
extern float OVAL_dvdx;
// uc_orig: OVAL_dudz (fallen/DDEngine/Source/oval.cpp)
extern float OVAL_dudz;
// uc_orig: OVAL_dvdz (fallen/DDEngine/Source/oval.cpp)
extern float OVAL_dvdz;

#endif // ENGINE_GRAPHICS_GEOMETRY_OVAL_GLOBALS_H
