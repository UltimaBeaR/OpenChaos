#ifndef ENGINE_AUDIO_SOUNDENV_GLOBALS_H
#define ENGINE_AUDIO_SOUNDENV_GLOBALS_H

#include "engine/core/types.h"

// Ground quad for 3D audio geometry: x/y are min corner, ox/oy are max corner (fixed-point, <<16 = world units).
// uc_orig: AudioGroundQuad (fallen/Source/soundenv.cpp)
struct AudioGroundQuad {
    SLONG x, y, ox, oy;
};

// uc_orig: SOUNDENV_gndctr (fallen/Source/soundenv.cpp)
extern int SOUNDENV_gndctr;

#endif // ENGINE_AUDIO_SOUNDENV_GLOBALS_H
