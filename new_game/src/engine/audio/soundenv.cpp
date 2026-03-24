#include <platform.h>
#include "engine/audio/soundenv.h"
#include "engine/audio/soundenv_globals.h"


// uc_orig: SOUNDENV_precalc (fallen/Source/soundenv.cpp)
void SOUNDENV_precalc(void)
{
    // Stub — sewer quad detection was disabled.
}

// Camera position externs used by SOUNDENV_upload (currently stub).
extern SLONG CAM_pos_x,
    CAM_pos_y,
    CAM_pos_z;

// uc_orig: SOUNDENV_upload (fallen/Source/soundenv.cpp)
void SOUNDENV_upload(void)
{
    // Stub — Direct3D handles the upload.
}
