#ifndef AI_MAV_GLOBALS_H
#define AI_MAV_GLOBALS_H

// MFStdLib.h must come before mav.h: mav.h -> pap.h -> structs.h -> anim.h uses strcpy.
#include <MFStdLib.h>
#include "fallen/Headers/mav.h"

// uc_orig: MAV_opt (fallen/Source/mav.cpp)
extern MAV_Opt* MAV_opt;

// uc_orig: MAV_opt_upto (fallen/Source/mav.cpp)
extern SLONG MAV_opt_upto;

// uc_orig: MAV_nav (fallen/Source/mav.cpp)
extern UWORD* MAV_nav;

// uc_orig: MAV_nav_pitch (fallen/Source/mav.cpp)
extern SLONG MAV_nav_pitch;

#endif // AI_MAV_GLOBALS_H
