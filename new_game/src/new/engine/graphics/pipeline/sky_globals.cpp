#include "engine/graphics/pipeline/sky_globals.h"

// uc_orig: SKY_star (fallen/DDEngine/Source/sky.cpp)
SKY_Star SKY_star[SKY_MAX_STARS];

// uc_orig: SKY_star_upto (fallen/DDEngine/Source/sky.cpp)
SLONG SKY_star_upto;

// uc_orig: SKY_texture (fallen/DDEngine/Source/sky.cpp)
SKY_Texture SKY_texture[SKY_NUM_TEXTURES] = {
    { 0.000F, 0.000F, 1.000F, 0.234F },
    { 0.000F, 0.234F, 0.566F, 0.375F },
    { 0.566F, 0.234F, 1.000F, 0.375F },
    { 0.000F, 0.375F, 1.000F, 0.648F },
    { 0.000F, 0.648F, 1.000F, 1.000F }
};

// uc_orig: SKY_cloud (fallen/DDEngine/Source/sky.cpp)
SKY_Cloud SKY_cloud[SKY_NUM_CLOUDS];
