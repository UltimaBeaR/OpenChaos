#ifndef ENGINE_GRAPHICS_GEOMETRY_SKY_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_SKY_GLOBALS_H

#include "engine/core/types.h"

// Star entry: position as yaw/pitch angles + precomputed world vector.
// uc_orig: SKY_Star (fallen/DDEngine/Source/sky.cpp)
typedef struct {
    UBYTE colour;
    UBYTE spread;
    UWORD shit;
    float yaw;
    float pitch;
    float vector[3];
} SKY_Star;

// uc_orig: SKY_MAX_STARS (fallen/DDEngine/Source/sky.cpp)
#define SKY_MAX_STARS 4096

// UV region of a cloud texture within the sky texture atlas.
// uc_orig: SKY_Texture (fallen/DDEngine/Source/sky.cpp)
typedef struct {
    float u1, v1;
    float u2, v2;
} SKY_Texture;

// uc_orig: SKY_NUM_TEXTURES (fallen/DDEngine/Source/sky.cpp)
#define SKY_NUM_TEXTURES 5

// A single animated cloud billboard.
// uc_orig: SKY_Cloud (fallen/DDEngine/Source/sky.cpp)
typedef struct {
    UBYTE texture;
    UBYTE flip; // non-zero => mirror the texture in u
    UBYTE width;
    UBYTE height;
    float yaw;
    float pitch;
    float dyaw; // angular velocity
} SKY_Cloud;

// uc_orig: SKY_NUM_CLOUDS (fallen/DDEngine/Source/sky.cpp)
#define SKY_NUM_CLOUDS 200

// uc_orig: SKY_star (fallen/DDEngine/Source/sky.cpp)
extern SKY_Star SKY_star[SKY_MAX_STARS];

// uc_orig: SKY_star_upto (fallen/DDEngine/Source/sky.cpp)
extern SLONG SKY_star_upto;

// uc_orig: SKY_texture (fallen/DDEngine/Source/sky.cpp)
extern SKY_Texture SKY_texture[SKY_NUM_TEXTURES];

// uc_orig: SKY_cloud (fallen/DDEngine/Source/sky.cpp)
extern SKY_Cloud SKY_cloud[SKY_NUM_CLOUDS];

#endif // ENGINE_GRAPHICS_GEOMETRY_SKY_GLOBALS_H
