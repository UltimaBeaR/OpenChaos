#include <platform.h>
#include "engine/audio/soundenv.h"
#include "engine/audio/soundenv_globals.h"


// uc_orig: SOUNDENV_precalc (fallen/Source/soundenv.cpp)
void SOUNDENV_precalc(void)
{
    // Body is commented out in the original — sewer quad detection was disabled.
}

// Camera position externs used by SOUNDENV_upload (currently stub).
extern SLONG CAM_pos_x,
    CAM_pos_y,
    CAM_pos_z;

// uc_orig: SOUNDENV_upload (fallen/Source/soundenv.cpp)
void SOUNDENV_upload(void)
{
    // Stub in the original.
}
