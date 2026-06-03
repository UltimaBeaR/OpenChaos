#ifndef OUTRO_OUTRO_BACK_GLOBALS_H
#define OUTRO_OUTRO_BACK_GLOBALS_H

#include "outro/core/outro_os.h"

// uc_orig: BACK_ot_roper (fallen/outro/back.cpp)
extern OS_Texture* BACK_ot_roper;

// uc_orig: BACK_ot_darci (fallen/outro/back.cpp)
extern OS_Texture* BACK_ot_darci;

// uc_orig: BACK_ot_mib (fallen/outro/back.cpp)
extern OS_Texture* BACK_ot_mib;

// uc_orig: BACK_ot_line (fallen/outro/back.cpp)
extern OS_Texture* BACK_ot_line;

// OpenChaos: OS_ticks baseline for the backdrop timeline. The credits now open
// with an OpenChaos block (section 0) drawn on plain black; the original game's
// backdrop only starts at the first original section (MUCKYFOOT). This is set
// to OS_ticks() at that moment so BACK_draw's time-based fade-in and image
// cycling begin exactly then — as they did before the block was prepended.
// 0 means "not started yet" (the backdrop isn't drawn during section 0).
extern SLONG BACK_start_ticks;

#endif // OUTRO_OUTRO_BACK_GLOBALS_H
